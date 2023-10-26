#include "memorystore.h"

#define MEM_ALIGNMENT_BYTES 256

static void * unmanaged_memory = NULL;
static uint64_t unmanaged_memory_size = UNMANAGED_MEMORY_SIZE;
static void * managed_memory = NULL;
static uint64_t managed_memory_size = MANAGED_MEMORY_SIZE;
static uint32_t malloc_mutex_id;

void get_memory_usage_summary_string(
    char * recipient,
    const uint32_t recipient_cap)
{
    strcpy_capped(
        recipient,
        recipient_cap,
        "Unmanaged memory use: ");
    strcat_uint_capped(
        recipient,
        recipient_cap,
        UNMANAGED_MEMORY_SIZE - (uint32_t)unmanaged_memory_size);
    strcat_capped(
        recipient,
        recipient_cap,
        " of: ");
    strcat_uint_capped(
        recipient,
        recipient_cap,
        UNMANAGED_MEMORY_SIZE);
    strcat_capped(
        recipient,
        recipient_cap,
        " (");
    strcat_uint_capped(
        recipient,
        recipient_cap,
        (uint32_t)(
            (float)(UNMANAGED_MEMORY_SIZE - (uint32_t)unmanaged_memory_size)
                / (float)UNMANAGED_MEMORY_SIZE * 100.0f));
    
    strcat_capped(
        recipient,
        recipient_cap,
        "%)\nManaged memory use: ");
    strcat_uint_capped(
        recipient,
        recipient_cap,
        MANAGED_MEMORY_SIZE - (uint32_t)managed_memory_size);
    strcat_capped(
        recipient,
        recipient_cap,
        " of: ");
    strcat_uint_capped(
        recipient,
        recipient_cap,
        MANAGED_MEMORY_SIZE);
    strcat_capped(
        recipient,
        recipient_cap,
        " (");
    strcat_uint_capped(
        recipient,
        recipient_cap,
        (uint32_t)(
            (float)(MANAGED_MEMORY_SIZE - (uint32_t)managed_memory_size)
                / (float)MANAGED_MEMORY_SIZE * 100.0f));
    strcat_capped(
        recipient,
        recipient_cap,
        "%)\n");
}

void init_memory_store(void) {
    malloc_mutex_id = platform_init_mutex_and_return_id();
    unmanaged_memory = platform_malloc_unaligned_block(UNMANAGED_MEMORY_SIZE);
    managed_memory   = platform_malloc_unaligned_block(MANAGED_MEMORY_SIZE  );
    
    for (uint32_t i = 0; i < UNMANAGED_MEMORY_SIZE; i++) {
        ((uint8_t *)unmanaged_memory)[i] = 0;
    }
    
    for (uint32_t i = 0; i < MANAGED_MEMORY_SIZE; i++) {
        ((uint8_t *)managed_memory)[i] = 0;
    }
}

static void * malloc_from_unmanaged_without_aligning(
    const uint64_t size)
{
    void * return_value = unmanaged_memory;
    if (size >= unmanaged_memory_size) {
        log_append("Tried to malloc_from_unamanged for: ");
        log_append_uint((uint32_t)(size / 1000000));
        log_append("MB, but you only had: ");
        log_append_uint((uint32_t)(unmanaged_memory_size / 1000000));
        log_append("MB remaining");
        assert(0);
        return NULL;
    }
    
    unmanaged_memory += size;
    unmanaged_memory_size -= size;
    
    return return_value;
}

void * malloc_from_unmanaged_aligned(
    const uint64_t size,
    const uint32_t aligned_to)
{
    platform_mutex_lock(malloc_mutex_id);
    
    assert(unmanaged_memory != NULL);
    assert(size > 0);
    
    uint32_t padding = 0;
    assert(unmanaged_memory_size >= aligned_to);
    while (
        (uintptr_t)(void *)unmanaged_memory % aligned_to != 0)
    {
        unmanaged_memory += 1;
        padding += 1;
    }
    assert(unmanaged_memory_size > padding);
    unmanaged_memory_size -= padding;
    assert(padding < aligned_to);
    assert((uintptr_t)(void *)unmanaged_memory % aligned_to == 0);
    
    void * return_value = malloc_from_unmanaged_without_aligning(size);
    
    assert((uintptr_t)(void *)return_value % aligned_to == 0);
    
    platform_mutex_unlock(malloc_mutex_id);
        
    return return_value;
};

void * malloc_from_unmanaged(size_t size) {
    void * return_value = malloc_from_unmanaged_aligned(
        size,
        MEM_ALIGNMENT_BYTES);
    
    return return_value;
};

void * malloc_from_managed(size_t size) {
    
    platform_mutex_lock(malloc_mutex_id);
    
    assert(managed_memory != NULL);
    assert(size > 0);
    
    uint32_t padding = 0;
    while (
        (uintptr_t)(void *)managed_memory % MEM_ALIGNMENT_BYTES != 0)
    {
        managed_memory += 1;
        padding += 1;
    }
    managed_memory_size -= padding;
    assert(padding < MEM_ALIGNMENT_BYTES);
    assert((uintptr_t)(void *)managed_memory % MEM_ALIGNMENT_BYTES == 0);
    
    void * return_value = managed_memory;
    
    assert(managed_memory_size >= size);
    managed_memory += size;
    managed_memory_size -= size;
    
    platform_mutex_unlock(malloc_mutex_id);
    
    return return_value;
};

void free_from_managed(void * to_free) {
    (void)to_free;
    
    // TODO: free up memory from the managed store
    return;
};

