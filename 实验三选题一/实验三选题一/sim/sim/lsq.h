#ifndef _LSQ_H_
#define _LSQ_H_

#include "define.h"

typedef struct que_t {
    int valid;
    int opc;
    int is_ready;
    int is_retire;
    int is_indirect;
    // uint8_t value;
    uint64_t arrive_clock;///// can be used for the identification
    uint64_t dispatch_clock;///when the addressing mode is resolved
    uint64_t translation_ready_clock;/// when the paddr is ready
    uint64_t data_ready_clock;/// when the data is successfully fetched
    uint64_t retired_clock;/// when the ins is ready to retire
    Vaddr_t vaddr;
    Paddr_t paddr;
    uint32_t data; /// oprand data store to memory, or the data load from memory
    int transient_data_ready;
    int datasource; /// where is the data from
    uint64_t forward_from; /// if it is forwarded, from which instruction exactly.
    int translate_ready;
    int need_rerun;

    int shift_from;
} que_t;


typedef struct LSQ{
    que_t queue[QUEUE_SIZE];
    int in_queue;
}LSQ;

extern LSQ lsq;


void clear_LSQ_entry(LSQ &Que, int i);
void Init_LSQ(LSQ &Que);
int count_lsq_slots(LSQ &Que);
int check_for_hazard(LSQ &Que, que_t cur_ins);
int checkIfOldest(LSQ &Que, que_t commit_ins);
int re_run(LSQ &Que);

#endif