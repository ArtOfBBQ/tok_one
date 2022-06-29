#include "memorystore.h"

#define MEM_ALIGNMENT_BYTES 8

uint8_t * unmanaged_memory = (uint8_t *)malloc(UNMANAGED_MEMORY_SIZE);
uint64_t unmanaged_memory_size = UNMANAGED_MEMORY_SIZE;
uint8_t * managed_memory = (uint8_t *)malloc(MANAGED_MEMORY_SIZE);
uint64_t managed_memory_size = MANAGED_MEMORY_SIZE;

uint8_t * malloc_from_unmanaged(uint64_t size) {
    
    log_assert(unmanaged_memory_size >= size);
    
    uint32_t padding = 0;
    while ((uintptr_t)(void *)unmanaged_memory %
        MEM_ALIGNMENT_BYTES != 0)
    {
        unmanaged_memory += 1;
        padding += 1;
    }
    assert(padding < MEM_ALIGNMENT_BYTES);
    assert((uintptr_t)(void *)unmanaged_memory % MEM_ALIGNMENT_BYTES == 0);
    
    uint8_t * return_value = (uint8_t *)(void *)unmanaged_memory;
    
    unmanaged_memory += size;
    unmanaged_memory_size -= size;
    
    return return_value;
};

uint8_t * malloc_from_managed(uint64_t size) {
    
    log_assert(managed_memory_size >= size);

    uint32_t padding = 0; 
    while ((uintptr_t)(void *)managed_memory % MEM_ALIGNMENT_BYTES != 0) {
        managed_memory += 1;
        padding += 1;
    }
    assert(padding < MEM_ALIGNMENT_BYTES);
    assert((uintptr_t)(void *)managed_memory % MEM_ALIGNMENT_BYTES == 0);
    
    uint8_t * return_value = managed_memory;
    managed_memory += size;
    managed_memory_size -= size;
    
    return return_value;
};

void free_from_managed(uint8_t * to_free) {
    // TODO: free up memory from the managed store
    return;
};

