#ifndef _AGU_H_
#define _AGU_H_

#include "define.h"
#include "lsq.h"
Vaddr_t probe_start_addr;
/*************************************TODO********************************
 * Address Generation Unit
 * It determine the effective address by different addressing mode.
 * For this simulator, we use it to calculate the address ready clock,
 * and there is one special load instruction for it to decode
 *
 * @cur_ins: the instruction that need to be resolved.
 * @idx: it is used for the spectre demo, no need to worry in Exp1
 *      do nothing if equals -1
 *      use the result of the ldq.queue[idx] to caculate EA
 * @cpu_clock: current cpu clock
 */
void generateEA(LSQ &Que, que_t &cur_ins, int idx, cpu_clock_t cpu_clock){
    if(idx != -1){
        /*
         *TODO:
         * How to encode the stale value into probe_array and get its virtual address
         */
        que_t &prev_ins = Que.queue[idx];
        if(prev_ins.datasource != DATA_NOT_READY){
            cur_ins.vaddr = 0; // probe_start_addr + CACHE_BLOCK_SIZE * prev_ins.data;
            if(cur_ins.dispatch_clock >= MAX_CLOCK_CYCLES){
                cur_ins.dispatch_clock = cur_ins.arrive_clock + 1;
                if(cur_ins.is_indirect)
                    cur_ins.dispatch_clock += ADDRESSING_TIME;
            }
        }
    }
    else
        //if the dispatch clock was not ready yet
        if(cur_ins.dispatch_clock >= MAX_CLOCK_CYCLES){
            cur_ins.dispatch_clock = cur_ins.arrive_clock + 1;
            if(cur_ins.is_indirect)
                cur_ins.dispatch_clock += ADDRESSING_TIME;
        }
}


#endif