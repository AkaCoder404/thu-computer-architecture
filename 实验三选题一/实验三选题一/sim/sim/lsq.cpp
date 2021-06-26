#include "lsq.h"


LSQ lsq;

void clear_LSQ_entry(LSQ &Que, int i){
    que_t &cur_ins = Que.queue[i];
    cur_ins.valid = 0;
    cur_ins.opc = -1;
    cur_ins.is_ready = 0;
    cur_ins.is_retire = 0;
    cur_ins.is_indirect = 0;
    cur_ins.arrive_clock = MAX_CLOCK_CYCLES;
    cur_ins.retired_clock = MAX_CLOCK_CYCLES;
    cur_ins.dispatch_clock = MAX_CLOCK_CYCLES;
    cur_ins.translation_ready_clock = MAX_CLOCK_CYCLES;
    cur_ins.data_ready_clock = MAX_CLOCK_CYCLES;
    cur_ins.vaddr = 0;
    cur_ins.paddr = 0;
    cur_ins.data = 0;
    cur_ins.transient_data_ready = 0;
    cur_ins.datasource = DATA_NOT_READY;
    cur_ins.forward_from = MAX_CLOCK_CYCLES;
    cur_ins.translate_ready = 0;
    cur_ins.need_rerun = 0;

    cur_ins.shift_from = -1;
}

void Init_LSQ(LSQ &Que){
    Que.in_queue = 0;
    register int i;
    for(i = 0; i < QUEUE_SIZE; ++i){
        clear_LSQ_entry(Que, i);
    }
}

//return the last entry that can be replaced
int count_lsq_slots(LSQ &Que){
    
    int res = 0;
    int idx = -1;
    register int i;
    for(i = 0; i < QUEUE_SIZE; ++i){
        if(!Que.queue[i].valid){
            idx = i;
            res ++;
        }
    }
    Que.in_queue = QUEUE_SIZE - res;
    return idx;
}


int check_for_hazard(LSQ &Que, que_t cur_ins){
    register int i;
    uint64_t res = MAX_CLOCK_CYCLES;
    int j = -1;
    for(i = 0; i < QUEUE_SIZE; ++i){
        que_t &prev_ins = Que.queue[i];
        if(prev_ins.valid && (prev_ins.arrive_clock < cur_ins.arrive_clock)){
            if(prev_ins.vaddr == cur_ins.vaddr && res > prev_ins.arrive_clock){
                res = prev_ins.arrive_clock;
                j = i;
            }
            
        }
    }
    return j;
}

int checkIfOldest(LSQ &Que, que_t commit_ins){
    register int i;
    for(i = 0; i < QUEUE_SIZE; ++i){
        que_t &cur_ins = Que.queue[i];
        if(cur_ins.is_retire == 0 && cur_ins.arrive_clock < commit_ins.arrive_clock)
            return 0;
    }
    return 1;
}

int re_run(LSQ &Que){
    register int i;
    int min_index = -1;
    uint64_t min_clock = MAX_CLOCK_CYCLES;
    for(i = 0; i < QUEUE_SIZE; ++i){
        que_t &cur_ins = Que.queue[i];
        if(cur_ins.need_rerun == 1){
            if(cur_ins.arrive_clock < min_clock){
                min_index = i;
                min_clock = cur_ins.arrive_clock;
            }
        }
    }
    return min_index;
}
