#ifndef _TLB_H_
#define _TLB_H_

#include "define.h"





PageNo_t getPageNo(Vaddr_t addr);
FrameNo_t getFrameNo(Paddr_t addr);
PageOffset_t getPageOffset(Vaddr_t addr);

void setupPageTable();
FrameNo_t getFrameNoFromTLB(Vaddr_t vaddr);
void updateTLB(Vaddr_t vaddr,  PageNo_t page, FrameNo_t frame);
FrameNo_t getFrameNoFromPageTable(Vaddr_t vaddr);


#endif