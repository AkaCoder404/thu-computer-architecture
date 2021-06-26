#include "define.h"
#include "lsq.h"
#include "tlb.h"
#include "memory.h"
#include "agu.h"
#include "controller.h"

extern long long int cache_hits;
extern long long int tlb_hits;
extern long long int cache_misses_load;
extern long long int cache_misses_store;

//**************************************************TODO*******************************************//

int results[16];
/*
 *TODO:
 *implement you cache reload gadget here. (Task 3)
 */

void Reload(){
    for (int i = 0; i < 16; i++) {
        Vaddr_t vaddr = probe_start_addr + 64 * i;
        PageOffset_t offset = getPageOffset(vaddr);
        FrameNo_t frame = getFrameNoFromPageTable(vaddr);
        results[i] = results[i] + access_cache((frame << PAGE_OFFSET_SHIT) + offset);
    }   
}


//*******************************************************************************************************//

void PrintStatistics(){
/*
 *TODO:
 *print your profile statistic here.
 */ 
    printf("\nPrint Statistics\n");
    printf("Total TLB cache misses: %lld\n", tlb_misses);
    printf("Total TLB cache hits: %lld\n", tlb_hits);
    printf("Total cache hits: %lld\n", cache_hits);
    printf("Total cache misses: %lld\n", cache_misses_load + cache_misses_store);
    printf("Total load cache misses: %lld, store misses: %lld\n", cache_misses_load, cache_misses_store);
}

void PrintLSQEntry(que_t ins){
    printf("arrive clock %lu \n", ins.arrive_clock);
    printf("dispatch clock %lu \n", ins.dispatch_clock);
    printf("translation clock %lu \n", ins.translation_ready_clock);
    printf("data ready clock %lu \n", ins.data_ready_clock);
    printf("retire clock %lu \n", ins.retired_clock);

    printf("opcode: %d\n", ins.opc);
    printf("data: %x\n", ins.data);
    printf("virtual address: %x\n", ins.vaddr);
    printf("indirect: %d\n", ins.is_indirect);
    printf("need rerun %d\n", ins.need_rerun);
    puts("");
}

void Clock_Handler(char *filename){
    FILE *fp = fopen(filename, "r");
    int flag = -1; // denoting whether the input is EOF or not
    if(fp == NULL){
        puts("Cannot open file!");
        return;
    }
    //every load data can be put into the data slot in que_t entry,
    //but we can not read it in transient windows. We can set the COMMIT_LOAD_TO_PROGRAMER
    // macro to read it in log.
    for(cpu_clock_t cpu_clock = 1; cpu_clock < MAX_CLOCK_CYCLES; ++cpu_clock){
    
        check_lsq(lsq, cpu_clock);

        //one instruction at a clock, and rerun instruction first.
        int idx = re_run(lsq);
        if(idx != -1){
            que_t &cur_ins = lsq.queue[idx];
            cur_ins.arrive_clock = cpu_clock;
            cur_ins.is_indirect = 0;
            cur_ins.valid = 1;

            cur_ins.is_ready = 0;
            cur_ins.is_retire = 0;
            cur_ins.retired_clock = MAX_CLOCK_CYCLES;
            cur_ins.dispatch_clock = MAX_CLOCK_CYCLES;
            cur_ins.translation_ready_clock = MAX_CLOCK_CYCLES;
            cur_ins.data_ready_clock = MAX_CLOCK_CYCLES;
            cur_ins.datasource = DATA_NOT_READY;
            cur_ins.forward_from = MAX_CLOCK_CYCLES;
            cur_ins.translate_ready = 0;
            cur_ins.need_rerun = 0;
        }
        else{
            //check the lsq if not full.
            int nxt_slot = count_lsq_slots(lsq);
            if(nxt_slot != -1){
                char input_opcode[10];
                char input_data[10];
                char input_addr[10];
                //while there is no input, we still need to wait for the instructions in the pipline
                if(flag == -1){
                    if(fscanf(fp, "%s", input_opcode) == EOF){
                        flag = 0;
                    }
                        
                }
                register int k;
                for(k = 0; k < QUEUE_SIZE; ++k){
                    que_t tmpins = lsq.queue[k];
                    if(tmpins.valid){
                        break;
                    }
                }
                
                // While EOF and empty pipeline, we can shut down.
                if(flag == 0 && k >= QUEUE_SIZE)   break;
                // printf("input opcode %s, input addr %s\n", input_opcode, input_addr);
                if(flag == -1){
                    //scanf an instruction from the input trace, and allocate one entry
                    clear_LSQ_entry(lsq, nxt_slot);
                    que_t &cur_ins = lsq.queue[nxt_slot];
                    
                    //**********Decode*******
                    if(input_opcode[0] == 'l'){
                        fscanf(fp, "%s", input_addr);
                        cur_ins.opc = 0;
                        cur_ins.arrive_clock = cpu_clock;
                        cur_ins.valid = 1;
                    }
                    else if(input_opcode[0] == 's'){
                        fscanf(fp, "%x %s", &cur_ins.data, input_addr);
                        cur_ins.opc = 1;
                        cur_ins.arrive_clock = cpu_clock;
                        cur_ins.valid = 1;
                    }
                    if(input_addr[0] == '['){
                        cur_ins.is_indirect = 1;
                        sscanf(input_addr, "[%x]", &cur_ins.vaddr);
                    }else if(input_addr[0] != '\0'){
                        sscanf(input_addr, "%x", &cur_ins.vaddr);
                    }

                    if(SPECTRE_TESTING){
                        if(input_opcode[0] == 'p'){
                            fscanf(fp, "%d", &cur_ins.shift_from);
                            cur_ins.opc = 0;
                            cur_ins.arrive_clock = cpu_clock;
                            cur_ins.valid = 1;
                        }
                        else if(input_opcode[0] == 'f'){
                            flush_cache();
                        }
                    }
                   
                }
            }
        }

        if(SPECTRE_TESTING){
            Reload();
        }

        if(DEBUG){
            printf("Dumping at clock %lu\n", cpu_clock);
            register int k;
            for(k = 0; k < QUEUE_SIZE; ++k){
                que_t tmpins = lsq.queue[k];
                if(tmpins.valid){
                    PrintLSQEntry(tmpins);
                }
            }printf("\n\n\n");
        }
        

    }
    fclose(fp);
    PrintStatistics();
}


int main(int argc, char ** argv){
    memset(results, 0, sizeof(results));
    memset(physical_memory, 0, sizeof(physical_memory));
    Init_LSQ(lsq);
    probe_start_addr = 0x0;
    setupPageTable();
    char input_trace[50] = "./trace/spectreV4.trace";
    if(argc > 1) sscanf(argv[1], "%s", input_trace);

    Clock_Handler(input_trace);

    int highest = -1;
    for (int i = 0; i < 16; i++) {
        if (highest < 0 || results[highest] < results[i]) {
            highest = i;
        }
    }
    printf("highest %x\n", highest);

    return 0;
}