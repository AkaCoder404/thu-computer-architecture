#include "tlb.h"

tlb_t tlb[TLB_SIZE];
pte_t page_table[VIRTUAL_MEMORY_SIZE];
long long int tlb_hits;
long long int tlb_misses;

PageNo_t getPageNo(Vaddr_t addr){
    return (addr&(VIRTUAL_PAGE_NO_MASK))>>PAGE_OFFSET_SHIT;
}

FrameNo_t getFrameNo(Paddr_t addr){
    return (addr&PHYSICAL_FRAME_NO_MASK)>>PAGE_OFFSET_SHIT;
}

PageOffset_t getPageOffset(Vaddr_t addr){
    return (addr&PAGE_OFFSET_MASK);
}

/********************************TODO******************************
 *get physical frame from input virtual address
 *this function is just query function, did not change the function unit's state and data
 *return frame number if TLB hit, else return NULL(0).
 */

FrameNo_t getFrameNoFromTLB(Vaddr_t vaddr){
    int valid;
    FrameNo_t frame;
    PageNo_t page;
    PageNo_t input_page = getPageNo(vaddr);
    register int i = (vaddr & TLB_INDEX_MASK) >> TLB_INDEX_SHIFT;

    /*
     *TODO 
     *
     */

    valid = (tlb[i] & TLB_VALID_MASK) >> TLB_VALID_SHIFT;
    page = (tlb[i] & TLB_VADDR_MASK) >> TLB_VADDR_SHIFT; 
    frame = (tlb[i] & TLB_PADDR_MASK);

    if(DEBUG) printf("input page %x, tlb entry page: %x\n",input_page, page);

    if(valid && input_page == page){
        // tlb_hits = tlb_hits + 1;
        return frame;
    }

////// the vaddr of 0 refers to NULL, as well as the first virtual page,
////// dereference the NULL address will cause an exception
////// the initial address input does not contain any of them
    return 0;
}


/********************************TODO******************************
 *@vaddr: virtual address
 *@page: virtual page
 *@frame: physical frame
 */
void updateTLB(Vaddr_t vaddr,  PageNo_t page, FrameNo_t frame){
    /*
     *TODO: you need to finish the virtual address map to the TLB index
     *      and then use the direct map strategy to update TLB entry
     */
    register int i = (vaddr & TLB_INDEX_MASK) >> TLB_INDEX_SHIFT;
    int valid = (tlb[i]&TLB_VALID_MASK)>>TLB_VALID_SHIFT;

    tlb[i] = 1 << 13;
    tlb[i] = tlb[i] | (page << 6);
    tlb[i] = tlb[i] | frame;

    tlb_t tmp = tlb[i];

    if(DEBUG) printf("updated tlb entry: 0x%x, valid: %d\n", tmp, valid);
}



FrameNo_t getFrameNoFromPageTable(Vaddr_t vaddr){
    PageNo_t page = getPageNo(vaddr);
    pte_t pte = page_table[page];
    int valid = (pte&PAGETABLE_VALID_MASK) >> 6;
    FrameNo_t frame = pte & PAGETABLE_FRAME_NUM_MASK;

    updateTLB(vaddr, page, frame);
    return frame;
}


void setupPageTable(){
    register int i;
    for(i = 0; i < VIRTUAL_MEMORY_SIZE; ++i){
        pte_t &pte = page_table[i];
        pte = PAGETABLE_VALID_MASK;
        pte |= (PAGETABLE_FRAME_NUM_MASK & i);
    }
}