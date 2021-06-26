#ifndef _MEMORY_H_
#define _MEMORY_H_

#include "define.h"

BYTE loadFromMemory(Paddr_t paddr);
void writeBack2Memory(Paddr_t paddr, uint32_t value);
void flush_cache();

int updateCache(Paddr_t paddr);
int access_cache(Paddr_t paddr);
int loadFromCache(Paddr_t paddr, uint32_t &data);
int store2Cache(Paddr_t paddr, uint32_t value);
int invalidateCachelineByPaddr(Paddr_t paddr);

extern BYTE physical_memory[(PHYSICAL_MEMORY_SIZE) * PAGE_SIZE];

#endif