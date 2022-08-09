#include "memorystore.h"

#define MEM_ALIGNMENT_BYTES 16

uint8_t * unmanaged_memory = NULL;
uint64_t unmanaged_memory_size = UNMANAGED_MEMORY_SIZE;
uint8_t * managed_memory = NULL;
uint64_t managed_memory_size = MANAGED_MEMORY_SIZE;

uint8_t * malloc_from_unmanaged(uint64_t size) {

    log_assert(unmanaged_memory != NULL);
    log_assert(size > 0);
    
    uint32_t padding = 0;
    log_assert(unmanaged_memory_size >= MEM_ALIGNMENT_BYTES);
    while ((uintptr_t)(void *)unmanaged_memory %
        MEM_ALIGNMENT_BYTES != 0)
    {
        unmanaged_memory += 1;
        padding += 1;
    }
    unmanaged_memory_size -= padding;
    log_assert(padding < MEM_ALIGNMENT_BYTES);
    log_assert((uintptr_t)(void *)unmanaged_memory % MEM_ALIGNMENT_BYTES == 0);
    
    uint8_t * return_value = (uint8_t *)(void *)unmanaged_memory;
    log_assert(unmanaged_memory_size >= size);
    unmanaged_memory += size;
    unmanaged_memory_size -= size;
    
    return return_value;
};

uint8_t * malloc_from_managed(uint64_t size) {

    log_assert(managed_memory != NULL);
    log_assert(size > 0);
    
    uint32_t padding = 0; 
    while ((uintptr_t)(void *)managed_memory % MEM_ALIGNMENT_BYTES != 0) {
        managed_memory += 1;
        padding += 1;
    }
    managed_memory_size -= padding;
    log_assert(padding < MEM_ALIGNMENT_BYTES);
    log_assert((uintptr_t)(void *)managed_memory % MEM_ALIGNMENT_BYTES == 0);
    
    uint8_t * return_value = managed_memory;
    
    log_assert(managed_memory_size >= size);
    managed_memory += size;
    managed_memory_size -= size;
    
    return return_value;
};

void free_from_managed(uint8_t * to_free) {
    // TODO: free up memory from the managed store
    return;
};

uint64_t get_remaining_memory_checksum() {
    uint64_t checksum = 0;
    
    uint64_t countdown = 0;
    while (countdown < unmanaged_memory_size) {
        checksum += (unmanaged_memory[countdown] % 9);
        countdown++;
    }
    
    countdown = 0;
    while (countdown < managed_memory_size) {
        checksum += (managed_memory[countdown] % 9);
        countdown++;
    }
    
    return checksum;
}
