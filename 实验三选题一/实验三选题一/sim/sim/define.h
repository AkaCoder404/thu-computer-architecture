#ifndef _DEFINE_H_
#define _DEFINE_H_

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <unistd.h>
#include <string>
#include <algorithm>
#include <cmath>
#include <fstream>

typedef uint8_t pte_t;
typedef uint32_t tlb_t;

typedef uint8_t cache_t;
typedef uint8_t cache_block_t;

typedef uint32_t Vaddr_t;
typedef uint32_t Paddr_t;
typedef uint32_t PageNo_t;
typedef uint32_t FrameNo_t;
typedef uint32_t PageOffset_t;

typedef uint64_t cpu_clock_t;
typedef uint8_t BYTE;

#define DEBUG                               1 //set if we want to print out debug trace
#define SPECTRE_TESTING                     1 //set when we do the spectre v4 demo
#define COMMIT_LOAD_TO_PROGRAMER            1 //set if we want to see the load results

#define MAX_CLOCK_CYCLES                    0X3F3F3F3F3F3F3F3F


#define PAGE_SIZE                           256
#define PHYSICAL_MEMORY_SIZE                (1<<6)  //For simplification, physical memory size equals the virtual memory size.
#define VIRTUAL_MEMORY_SIZE                 (1<<6)
#define PHYSICAL_FRAME_NO_MASK              0X3F00
#define VIRTUAL_PAGE_NO_MASK                0X3F00
#define PAGE_OFFSET_MASK                    0XFF
#define PAGE_OFFSET_SHIT                    8


#define QUEUE_SIZE                          6
#define TLB_SIZE                            16
#define STORE_BUFFER_SIZE                   10
#define LOAD_BUFFER_SIZE                    10


#define CACHE_BLOCK_SIZE                    64      //cache block size is 64 Bytes
#define CACHE_SIZE                          (1<<8)  //the cache blocks counts in the whole phusical memory
#define CACHE_SET_NUM                       (1<<6)
#define CACHE_ASSOCIATION                   4       //4 ways associated

/**********************Cache line format*******************
 *+------------------+--+--+-+-+
 *|        TAG       |RD|RA|V|D|
 *|         2        | 1| 1|1|1|
 *+------------------+--+--+-+-+
 *@TAG: Two most significant bits in the physical address
 *@RD: We use the round robin replacement algorithm for the cache, 
       RD is set when this entry is recently write
 *@RA: set when this entry is recently accessed
 *@V: valid bit
 *@D: dirty bit
 */
#define CACHE_TAG_MASK                      0X30
#define CACHE_REP_ACCESSED_MASK             0X4
#define CACHE_REP_DIRTY_MASK                0X8
#define CACHE_VALID                         0X2
#define CACHE_INVALIDATE                    0XFD
#define CACHE_DIRTY                         0X1

#define PADDR_BLOCK_OFFSET_MASK             0X3F
#define PADDR_SET_MASK                      0XFC0
#define PADDR_TAG_MASK                      0X3000

/**********************TLB entry format********************
 *+-+-+------------------+------------------+
 *|V|D|        PNO       |        FNO       |
 *|1|1|         6        |         6        |
 *+-+-+------------------+------------------+
 *@V: valid bit
 *@D: dirty bit
 *@PNO: virtual page number
 *@FNO: physical frame number
 * We use direct map between virtual page number and TLB entry.
*/
#define TLB_VALID_MASK                      (1<<13)
#define TLB_DIRTY_MASK                      (1<<12)
#define TLB_VADDR_MASK                      0XFC0
#define TLB_PADDR_MASK                      0X3F
#define TLB_INDEX_MASK                      0XF00
#define TLB_VALID_SHIFT                     13
#define TLB_VADDR_SHIFT                     6
#define TLB_INDEX_SHIFT                     8

/********************Page Table entry format*****************
 *+-+------------------+
 *|V|        PNO       |
 *|1|         6        |
 *+-+------------------+
 *@V: valid bit, use for page replacement between disk and memory
 *@PNO: page frame number
 *Since we did not implement the Device, and there is no page fault,
 *therefore, the page table entry is always valid.
 */
#define PAGETABLE_VALID_MASK                (1<<6)
#define PAGETABLE_FRAME_NUM_MASK            0x3F

/*
 * Denoting the transient value source in an in-flight load instruction
 */
#define DATA_FROM_FORWARD                   1
#define DATA_FROM_MEMORY                    2
#define DATA_NOT_READY                      3

/*
 * cpu cycles for specific operation respectively
 */
#define ADDRESSING_TIME                     6
#define TLB_HIT_TIME                        2
#define TLB_MISS_TIME                       10
#define CACHE_HIT_TIME                      2
#define CACHE_MISS_TIME                     10
#define STORE_TO_LOAD_FORWARD_TIME          1
#define COMMIT_TIME                         1


#endif