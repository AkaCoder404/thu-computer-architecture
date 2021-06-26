#ifndef _CONTROLLER_H_
#define _CONTROLLER_H_

#include "define.h"
#include "lsq.h"
#include "tlb.h"
#include "memory.h"


extern long long int tlb_hits;
extern long long int tlb_misses;

//**************************************Controller*****************************************
/*
 * It schedules the whole system
 * showing when the instruction retired.
 */

/**********************************TODO***********************************
 *you need to search the lsq, and find the possible store entry to forward to load
 *@Que: the LSQ
 *@load_ins: the load instruction that need to be forwarded.
 *return 1 if forward successfully, else return 0.
 */
int store2loadForward(LSQ &Que, que_t &load_ins){
    register int i;
    //find the latest store entry that has finish addressing
    int idx = -1;
    uint64_t max_clock = 0;
    for(i = 0; i < QUEUE_SIZE; ++i){
        que_t &cur_ins = Que.queue[i];
        /*
         *TODO:
         *On what condition to forward data.
         */
        if(cur_ins.vaddr == load_ins.vaddr 
            && cur_ins.valid == 1 
            && cur_ins.arrive_clock < load_ins.arrive_clock 
            && cur_ins.is_ready == 1 
            && cur_ins.opc == 1
            ){
            if(max_clock <= cur_ins.arrive_clock){
                idx = i;
                max_clock = cur_ins.arrive_clock;
            }
        }
        /**/
    }
    if(idx != -1){
        load_ins.data = Que.queue[idx].data;
        load_ins.datasource = DATA_FROM_FORWARD;
        load_ins.forward_from = max_clock;
        return 1;
    }
    
    return 0;
}

/**********************************TODO***********************************
 *We need to flush some instruction in the pipeline, but not all of them.
 *@Que: LSQ
 *@cpu_clock: current cpu clock cycle
 *@load_ins: the load instruction that cause the pipeline flushing
 */
void flushPipeline(LSQ &Que, cpu_clock_t cpu_clock, que_t load_ins){
     register int i;
    for(i = 0; i < QUEUE_SIZE; ++i){
        que_t &cur_ins = Que.queue[i];
        /*
         *TODO:
         *On what condition to flush pipeline data.
         */
        if(cur_ins.arrive_clock >= load_ins.arrive_clock 
            && cur_ins.valid == 1){
            cur_ins.need_rerun = 1;
            cur_ins.valid = 0;
        }
    }
}

/**********************************TODO***********************************
 *On every store commit time, you need to verify every store-to-load forward
 * to the latter load instruction. If the load instruction depends on this store,
 *however it forward data from older stores, or from the memory, such load instruction
 * and the instructions after it have to be rerun.
 *@Que: LSQ
 *@commit_ins: the instruction that reach the commit stage.
 *@cpu_clock: current cpu clock cycle.
 */
void commitStore(LSQ &Que, que_t &commit_ins, cpu_clock_t cpu_clock){
    //current instruction can commit whatever
    commit_ins.is_retire = 1;
    commit_ins.valid = 0;
	
	store2Cache(commit_ins.paddr, commit_ins.data);

    register int i;

    //However, we need to flush the pipeline on certain condition.
    for(i = 0; i < QUEUE_SIZE; ++i){
        que_t &cur_ins = Que.queue[i];
        /*
         *TODO:
         *On what condition to flush pipeline data.
         */
        if( cur_ins.arrive_clock > commit_ins.arrive_clock 
            && cur_ins.valid == 1
            && cur_ins.is_ready == 1
            && ((cur_ins.datasource == DATA_FROM_MEMORY) || (cur_ins.forward_from < commit_ins.arrive_clock))
            && cur_ins.opc == 0 
            && cur_ins.vaddr == commit_ins.vaddr
            ){
            flushPipeline(Que, cpu_clock, cur_ins);
            //break;
        }
    }
}

/**********************************TODO***********************************
 *This is the main controller function.
 *On every cycle, it checks the LSQ, and do the certain operation based on current
 *CPU clock.
 *******************MAIN JOBS******************
 *1. Check ordering failure on every commit of store instruction
 *2. Genrate virtual address base on the instruction and update the dispatch clock
    on its addressing mode
 *3. Calculate address translation time base on the instruction virtual address
    and update the translation clock to LSQ entry respectively
 *4. Caculate data fetching or writing time whether the cache hit or not. Similarly,
    update the data ready clock to LSQ entry.
 *5. Do the actual translation or data manipulation base on current cpu clock and
    instruction operation events.
 *6. Retire instructions that meet the requirements in arriving order.
 ***********************PARAMETERS*************
 *@Que: LSQ
 *@cpu_clock: current cpu clock cycle.
 ***********************NOTES******************
 *Never change the architecture unit inside transient windows(before retirement).
 */
void check_lsq(LSQ &Que, cpu_clock_t cpu_clock){
    register int i;
    for(i = 0; i < QUEUE_SIZE; ++i){
        que_t &cur_ins = Que.queue[i];
        if(cur_ins.valid){
            //check if this instruction depends on former instruction
            int idx = check_for_hazard(Que, cur_ins);
            if(DEBUG)   printf("hazard index: %d\n", idx);
            generateEA(Que, cur_ins, cur_ins.shift_from, cpu_clock);
            /*
            *TODO:
            *On what condition to calculate delay on translation?
            */
            if(cur_ins.dispatch_clock <= cpu_clock 
                && cur_ins.is_ready == 0){
                cur_ins.is_ready = 1;

                FrameNo_t frame = getFrameNoFromTLB(cur_ins.vaddr);

                if(frame == 0){
                    cur_ins.translation_ready_clock = cur_ins.dispatch_clock + TLB_MISS_TIME;
                    tlb_misses++;
                }
                else{
					cur_ins.translation_ready_clock = cur_ins.dispatch_clock + TLB_HIT_TIME;
					tlb_hits++;
				}
            }
            //check store instruction first, and then load ins
            if(cur_ins.opc == 1){
                //check if this instruction is the oldest in the pipeline
                int is_oldest = checkIfOldest(Que, cur_ins);
                /*
                *TODO:
                *retire store if ?
                */
                if(is_oldest && cur_ins.retired_clock <= cpu_clock){
                    if(DEBUG)   printf("retire at %lu\n",cpu_clock);
                    commitStore(Que, cur_ins, cpu_clock);
                    continue;
                }
                //since we just disambiguate the RAW hazard, we can block all other
                //hazards.
                if(idx != -1){
                    que_t &hazard_ins = Que.queue[idx];
                    cur_ins.dispatch_clock = std::max(cur_ins.dispatch_clock, hazard_ins.dispatch_clock + 1);
                }
                //When the instruction addressing finished.
                if(cur_ins.dispatch_clock <= cpu_clock){
                    if(DEBUG)   printf("dispatch ready\n");
                    
                    if(!cur_ins.translate_ready && cur_ins.translation_ready_clock <= cpu_clock){
                        /*
                        *TODO:
                        *What should we do when translation clock ready?
                        *what should we do on different TLB reaction?
                        *Caculate the physical address
                        */
                        Vaddr_t vaddr = cur_ins.vaddr;
                        FrameNo_t frame = getFrameNoFromTLB(vaddr);
                        
                        if(frame == 0){
                            frame = getFrameNoFromPageTable(vaddr);
                        }
                        Paddr_t &paddr = cur_ins.paddr;
                        paddr = (frame << 8) | getPageOffset(vaddr);
                        if(DEBUG){
                            printf("instruction:%lx, vaddr:0x%x, paddr:0x%x\n",cur_ins.arrive_clock ,vaddr, paddr);
                        }
                        cur_ins.translate_ready = 1;
                    }
                    if(cur_ins.translate_ready){
                        /*
                        *TODO:
                        *Caculate the store instruction retire time on different cache behaviors.
                        */
						if(cur_ins.data_ready_clock >= MAX_CLOCK_CYCLES){
							if(access_cache(cur_ins.paddr) != -1){
								cur_ins.data_ready_clock = cur_ins.translation_ready_clock + CACHE_HIT_TIME;
							}
							else    cur_ins.data_ready_clock = cur_ins.translation_ready_clock + CACHE_MISS_TIME;
							//commit delay is 1 clock cycle to write back to register
							cur_ins.retired_clock = cur_ins.data_ready_clock + COMMIT_TIME;
						}
						
                    }
                }
                
            }
            else if(cur_ins.opc == 0){
                int is_oldest = checkIfOldest(Que, cur_ins);
                /*
                *TODO:
                *retire load if ?
                */
                if(is_oldest && cur_ins.retired_clock <= cpu_clock){
                    cur_ins.valid = 0;
                    cur_ins.is_retire = 1;
                    if(COMMIT_LOAD_TO_PROGRAMER){
                        printf("Data: %x of ldr %x\n",cur_ins.data, cur_ins.vaddr);
                    }
                }

                if(idx != -1){
                    que_t &hazard_ins = Que.queue[idx];
                    //if RAR hazard, we simply delay the instruction dispatch clock
                    if(hazard_ins.opc == 0){
                        cur_ins.dispatch_clock = std::max(cur_ins.dispatch_clock, hazard_ins.dispatch_clock + 1);
                    }
                    //if RAW, send it into pipeline.
                }

                //if the load data was not ready, then forward or access memory
                if(cur_ins.datasource == DATA_NOT_READY && cur_ins.dispatch_clock <= cpu_clock){
                    int succ = store2loadForward(Que, cur_ins);
                    if(!succ){
                        if(!cur_ins.translate_ready && cur_ins.translation_ready_clock <= cpu_clock){
                            //如果翻译完成，更新TLB和页表
                            /*
                            *TODO:
                            *What should we do when translation clock ready?
                            *what should we do on different TLB reaction?
                            *Caculate the physical address
                            */
                            Vaddr_t vaddr = cur_ins.vaddr;
                            FrameNo_t frame = getFrameNoFromTLB(vaddr);
                            
                            if(frame == 0){
                                frame = getFrameNoFromPageTable(vaddr);
                            }
                            Paddr_t &paddr = cur_ins.paddr;
                            paddr = (frame << 8) | getPageOffset(vaddr);
                            if(DEBUG){
                                printf("instruction:%lx, vaddr:0x%x, paddr:0x%x\n",cur_ins.arrive_clock ,vaddr, paddr);
                            }
                            cur_ins.translate_ready = 1;

                            
                        }
                        if(cur_ins.translate_ready){
							if(cur_ins.data_ready_clock >= MAX_CLOCK_CYCLES){
								/*
								*TODO:
								*Caculate the load instruction retire time on different cache behaviors.
								*/
								if(access_cache(cur_ins.paddr) != -1){
									cur_ins.data_ready_clock = cur_ins.translation_ready_clock + CACHE_HIT_TIME;
								}
								else    cur_ins.data_ready_clock = cur_ins.translation_ready_clock + CACHE_MISS_TIME;
								cur_ins.retired_clock = cur_ins.data_ready_clock + COMMIT_TIME;
                            }
							if(cur_ins.data_ready_clock <= cpu_clock && cur_ins.datasource == DATA_NOT_READY){
								/*
								*THINKING:
								*Does the behavior here meet the requirement that never change the physical
								*memory in transient windows?
								*/
								loadFromCache(cur_ins.paddr, cur_ins.data);
								cur_ins.transient_data_ready = 1;
								cur_ins.datasource = DATA_FROM_MEMORY;
							}
                        }
                    }
                    else{
                        cur_ins.translate_ready = 1;
                        cur_ins.translation_ready_clock = cur_ins.dispatch_clock + STORE_TO_LOAD_FORWARD_TIME;

                        cur_ins.data_ready_clock = cur_ins.translation_ready_clock;
                        cur_ins.transient_data_ready = 1;

                        cur_ins.retired_clock = cur_ins.data_ready_clock + COMMIT_TIME;
                    }
                }
                
            }
        }
    }
}


#endif