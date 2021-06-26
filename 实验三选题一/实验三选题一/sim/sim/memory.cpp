#include "memory.h"

cache_block_t cache_blocks[CACHE_SIZE][CACHE_BLOCK_SIZE];
cache_t cache[CACHE_SIZE];
BYTE physical_memory[(PHYSICAL_MEMORY_SIZE) * PAGE_SIZE];
long long int cache_hits;

// for caches misses
long long int cache_misses_load;
long long int cache_misses_store;

BYTE loadFromMemory(Paddr_t paddr){
    return physical_memory[paddr];
}

void writeBack2Memory(Paddr_t paddr, uint32_t value){
    physical_memory[paddr] = (value & 0XFF);
}

void flush_cache(){
    register int i;
    for(i = 0; i < CACHE_SIZE; ++i){
        cache[i] &= CACHE_INVALIDATE;
    }
}

//return a cache line index
int updateCache(Paddr_t paddr){
    uint32_t block_offset = paddr & PADDR_BLOCK_OFFSET_MASK;
    uint32_t set = (paddr & PADDR_SET_MASK) >> 6;
    uint32_t input_tag = (paddr & PADDR_TAG_MASK) >> 12;

    //4 ways aligned
    set <<= 2;
    register int i,j;
    //search for invalid entry to dispose
    for(i = set; i < set + CACHE_ASSOCIATION; ++i){
        uint8_t accessed = (cache[i] & CACHE_REP_ACCESSED_MASK) >> 2;
        uint8_t rep_dirty = (cache[i] & CACHE_REP_DIRTY_MASK) >> 3;
        int valid = (cache[i] & CACHE_VALID) >> 1;
        if(!valid)  return i;
    }
    //else to replace one entry
    int cnt = 4;
    while(cnt--){
        for(i = set; i < set + CACHE_ASSOCIATION; ++i){
            uint8_t accessed = (cache[i] & CACHE_REP_ACCESSED_MASK) >> 2;
            uint8_t rep_dirty = (cache[i] & CACHE_REP_DIRTY_MASK) >> 3;
            if((accessed == 0) && (rep_dirty == 0)){
                cnt = 0;
                break;
            }
            else if((cnt == 3) && (accessed == 1) && (rep_dirty == 0))
                cache[i] ^= CACHE_REP_ACCESSED_MASK;
            else if(cnt == 2 && (accessed == 0) && (rep_dirty == 1)){
                cache[i] ^= CACHE_REP_DIRTY_MASK;
                for(j = 0; j < CACHE_BLOCK_SIZE; ++j){
					uint32_t tag = (cache[i] & CACHE_TAG_MASK) >> 4;
					tag = (tag << 12);
					tag |= (paddr & PADDR_SET_MASK);
                    tag |= j;
                    writeBack2Memory(tag, cache_blocks[i][j]);
                }
            }
            else if(cnt == 1 && (accessed == 1) && (rep_dirty == 1)){
                cache[i] ^= CACHE_REP_DIRTY_MASK;
                cache[i] ^= CACHE_REP_ACCESSED_MASK;
                for(j = 0; j < CACHE_BLOCK_SIZE; ++j){
					uint32_t tag = (cache[i] & CACHE_TAG_MASK) >> 4;
					tag = (tag << 12);
					tag |= (paddr & PADDR_SET_MASK);
                    tag |= j;
                    writeBack2Memory(tag, cache_blocks[i][j]);
                }
            }
        }
    }
    return i;
}

//return the certain index if hit, else return -1
int access_cache(Paddr_t paddr){
    int idx = -1;
    uint32_t block_offset = paddr & PADDR_BLOCK_OFFSET_MASK;
    uint32_t set = (paddr & PADDR_SET_MASK) >> 6;
    uint32_t input_tag = (paddr & PADDR_TAG_MASK) >> 12;

    //4 ways aligned
    set <<= 2;
    register int i;
    for(i = set; i < set + CACHE_ASSOCIATION; ++i){
        int valid = (cache[i] & CACHE_VALID) >> 1;
        int dirty = (cache[i] & CACHE_DIRTY);
        uint8_t accessed = (cache[i] & CACHE_REP_ACCESSED_MASK) >> 2;
        uint32_t tag = (cache[i] & CACHE_TAG_MASK) >> 4;
        if(valid && (input_tag == tag)){
            idx = i;
            return idx;
        }
    }
    return idx;
}


int invalidateCachelineByPaddr(Paddr_t paddr){
    int idx = access_cache(paddr);
    if(idx != -1){
        cache[idx] &= CACHE_INVALIDATE;
        return 1;
    }
    return 0;
}

/**********************************TODO***********************************
 *actual execution on cache, find the data, and update related entries
 *@paddr: the physical address
 *@data: put the data load from memory to this reference
 *return 1 if hit, 0 if miss
 */

int loadFromCache(Paddr_t paddr, uint32_t &data){

    uint32_t block_offset = paddr & PADDR_BLOCK_OFFSET_MASK;
    uint32_t set = (paddr & PADDR_SET_MASK) >> 6;
    uint32_t input_tag = (paddr & PADDR_TAG_MASK) >> 12;

    int idx = access_cache(paddr);
    //if cache hit
    if(idx != -1){
        cache_hits++;
        /*
         *TODO: what should we do if cache hit
         */
        data = cache_blocks[idx][block_offset];
        cache[idx] = cache[idx] | CACHE_REP_ACCESSED_MASK;
        return 1;
    }

    /*
     *TODO: what about miss?
     * load from the memory, update cache
     */
    cache_misses_load = cache_misses_load + 1;
    int temp = updateCache(paddr);
    cache[temp] = (input_tag << 4) | CACHE_VALID;

    // update cache after loading from memory
    // look 
    for (int i = 0; i < CACHE_BLOCK_SIZE; i++) {
        auto temp_tag = input_tag;
        temp_tag = (temp_tag << 12) | (paddr & PADDR_SET_MASK);
        temp_tag = temp_tag | i;
        cache_blocks[temp][i] = loadFromMemory(temp_tag);
    }
    data = cache_blocks[temp][block_offset];
    return 0;
}


/**********************************TODO***********************************
 *actual execution on cache, find the data, and update related entries
 *@paddr: the physical address
 *@value: store the value to specified location
 *return 1 if hit, 0 if miss
 */
int store2Cache(Paddr_t paddr, uint32_t value){
    uint32_t block_offset = paddr & PADDR_BLOCK_OFFSET_MASK;
    uint32_t set = (paddr & PADDR_SET_MASK) >> 6;
    uint32_t input_tag = (paddr & PADDR_TAG_MASK) >> 12;

    int idx = access_cache(paddr);
    if(idx != -1){
        cache_hits++;
        /*
         *TODO: what should we do if cache hit
         */
        // look
        cache_blocks[idx][block_offset] = value;
        cache[idx] = cache[idx] | CACHE_REP_ACCESSED_MASK;
        cache[idx] = cache[idx] | CACHE_DIRTY;
        return 1;
    }

      /*
     *TODO: what about miss?
     * load from the memory, update cache, use the write back and write allocate
     * strategy.
     */
    cache_misses_store = cache_misses_store + 1;
    int temp = updateCache(paddr);
    cache[temp] = input_tag << 4;
    cache[temp] = cache[temp] | CACHE_VALID;

    // update cache after writing to memory
    for(int i = 0; i < CACHE_BLOCK_SIZE; i++) {
        auto temp_tag = input_tag;
        temp_tag = (temp_tag << 12);
        temp_tag = temp_tag | (paddr & PADDR_SET_MASK);
        temp_tag = temp_tag | i;
        cache_blocks[temp][i] = loadFromMemory(temp_tag);
    }
    cache_blocks[temp][block_offset] = value;

    return 0;
}

