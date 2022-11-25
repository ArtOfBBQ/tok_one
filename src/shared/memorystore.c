#include "memorystore.h"

#define MEM_ALIGNMENT_BYTES 16

// 500mb ->                   500...000
#define UNMANAGED_MEMORY_SIZE 500000000
// 150mb ->                   150...000
#define MANAGED_MEMORY_SIZE   150000000

static uint8_t * unmanaged_memory = NULL;
static uint64_t unmanaged_memory_size = UNMANAGED_MEMORY_SIZE;
static uint8_t * managed_memory = NULL;
static uint64_t managed_memory_size = MANAGED_MEMORY_SIZE;
static uint32_t malloc_mutex_id;

// We can consider mmap instead of malloc
// // #import <sys/mman.h>
//(Vertex *)mmap(
//    0,
//    buffered_vertex_size,
//    PROT_READ | PROT_WRITE,
//    MAP_PRIVATE | MAP_ANON,
//    -1,
//    0);

void init_memory_store(void) {
    malloc_mutex_id = platform_init_mutex_and_return_id();
    unmanaged_memory = platform_malloc_unaligned_block(UNMANAGED_MEMORY_SIZE);
    managed_memory = platform_malloc_unaligned_block(MANAGED_MEMORY_SIZE);
}

static uint8_t * malloc_from_unmanaged_without_aligning(
    const uint64_t size)
{
    uint8_t * return_value = (uint8_t *)(void *)unmanaged_memory;
    if (size >= unmanaged_memory_size) {
        log_append("Tried to malloc_from_unamanged for: ");
        log_append_uint((uint32_t)(size / 1000000));
        log_append("MB, but you only had: ");
        log_append_uint((uint32_t)(unmanaged_memory_size / 1000000));
        log_append("MB remaining");
        assert(0);
    }
    unmanaged_memory += size;
    unmanaged_memory_size -= size;
    
    return return_value;
}

uint8_t * malloc_from_unmanaged_aligned(
    const uint64_t size,
    const uint32_t aligned_to)
{
    platform_mutex_lock(malloc_mutex_id);
    
    assert(unmanaged_memory != NULL);
    assert(size > 0);    
    
    uint32_t padding = 0;
    assert(unmanaged_memory_size >= aligned_to);
    while ((uintptr_t)(void *)unmanaged_memory %
        aligned_to != 0)
    {
        unmanaged_memory += 1;
        padding += 1;
    }
    unmanaged_memory_size -= padding;
    assert(padding < aligned_to);
    assert((uintptr_t)(void *)unmanaged_memory % aligned_to == 0);
    
    uint8_t * return_value =
        malloc_from_unmanaged_without_aligning(size);
    
    assert((uintptr_t)(void *)return_value % aligned_to == 0);
    
    platform_mutex_unlock(malloc_mutex_id);
        
    return return_value;
};

uint8_t * malloc_from_unmanaged(const uint64_t size) {
    uint8_t * return_value = malloc_from_unmanaged_aligned(
        size,
        MEM_ALIGNMENT_BYTES);
    
    return return_value;
};

uint8_t * malloc_from_managed(const uint64_t size) {
    
    platform_mutex_lock(malloc_mutex_id);
    
    assert(managed_memory != NULL);
    assert(size > 0);
    
    uint32_t padding = 0; 
    while ((uintptr_t)(void *)managed_memory % MEM_ALIGNMENT_BYTES != 0) {
        managed_memory += 1;
        padding += 1;
    }
    managed_memory_size -= padding;
    assert(padding < MEM_ALIGNMENT_BYTES);
    assert((uintptr_t)(void *)managed_memory % MEM_ALIGNMENT_BYTES == 0);
    
    uint8_t * return_value = managed_memory;
    
    assert(managed_memory_size >= size);
    managed_memory += size;
    managed_memory_size -= size;
    
    platform_mutex_unlock(malloc_mutex_id);
    
    return return_value;
};

void free_from_managed(uint8_t * to_free) {
    (void)to_free;
    
    // TODO: free up memory from the managed store
    return;
};
