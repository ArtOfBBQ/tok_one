#include "T1_mem.h"

#define MEM_ALIGNMENT_BYTES 64

//#define MANAGED_MEMORY_STACK_SIZE 1000
//typedef struct ManagedMemoryStack {
//    void * pointers[MANAGED_MEMORY_STACK_SIZE];
//    char sources[MANAGED_MEMORY_STACK_SIZE][512];
//    bool32_t used[MANAGED_MEMORY_STACK_SIZE];
//    uint32_t size;
//} ManagedMemoryStack;
//static ManagedMemoryStack * managed_stack = NULL;

// uint32_t memorystore_page_size = 4000;

uint32_t T1_mem_page_size = 4096;

static void * unmanaged_memory = NULL;
static uint64_t unmanaged_memory_size = UNMANAGED_MEMORY_SIZE;
// static void * managed_memory = NULL;
// static void * managed_memory_end = NULL;

static uint32_t malloc_mutex_id = UINT32_MAX;

void (* T1_mem_mutex_lock)(const uint32_t mutex_id) = NULL;
void (* T1_mem_mutex_unlock)(const uint32_t mutex_id) = NULL;

static void T1_mem_set_pagesize(void) {
    long query_result = sysconf(_SC_PAGESIZE);
    if (query_result < 0 || query_result > UINT32_MAX) {
        assert(0);
        T1_mem_page_size = 4096;
    } else {
        T1_mem_page_size = (uint32_t)query_result;
    }
}

void T1_mem_get_usage_summary_string(
    char * recipient,
    const uint32_t recipient_cap)
{
    #ifdef COMMON_IGNORE_ASSERTS
    (void)recipient_cap;
    #endif
    
    T1_std_strcpy_cap(
        recipient,
        recipient_cap,
        "Unmanaged memory use: ");
    T1_std_strcat_uint_cap(
        recipient,
        recipient_cap,
        UNMANAGED_MEMORY_SIZE - (uint32_t)unmanaged_memory_size);
    T1_std_strcat_cap(
        recipient,
        recipient_cap,
        " of: ");
    T1_std_strcat_uint_cap(
        recipient,
        recipient_cap,
        UNMANAGED_MEMORY_SIZE);
    T1_std_strcat_cap(
        recipient,
        recipient_cap,
        " (");
    T1_std_strcat_uint_cap(
        recipient,
        recipient_cap,
        (uint32_t)(
            (float)(UNMANAGED_MEMORY_SIZE - (uint32_t)unmanaged_memory_size)
                / (float)UNMANAGED_MEMORY_SIZE * 100.0f));
}

static void T1_mem_align_pointer(void ** to_align) {
    uint32_t leftover = ((uintptr_t)*to_align) % MEM_ALIGNMENT_BYTES;
    uint32_t padding = 0;
    if (leftover > 0) {
        padding = MEM_ALIGNMENT_BYTES - leftover;
    }
    
    *to_align = (void *)((char *)*to_align + padding);
    log_assert((uintptr_t)*to_align % MEM_ALIGNMENT_BYTES == 0);
}

void T1_mem_init(
    void * ptr_unmanaged_memory_block,
    uint32_t (* memstore_init_mutex_and_return_id)(void),
    void (* ptr_mutex_lock)(const uint32_t mutex_id),
    void (* ptr_mutex_unlock)(const uint32_t mutex_id))
{
    malloc_mutex_id  = memstore_init_mutex_and_return_id();
    
    T1_mem_mutex_lock = ptr_mutex_lock;
    T1_mem_mutex_unlock = ptr_mutex_unlock;
    
    unmanaged_memory = ptr_unmanaged_memory_block;
    T1_mem_align_pointer(&unmanaged_memory);
    T1_std_memset(unmanaged_memory, 0, UNMANAGED_MEMORY_SIZE);
    
    T1_mem_set_pagesize();
}

static void * T1_mem_malloc_from_unmanaged_without_aligning(
    const uint64_t size)
{
    void * return_value = unmanaged_memory;
    if (size >= unmanaged_memory_size) {
        log_append("Tried to malloc_from_unamanged for: ");
        log_append_uint((uint32_t)(size / 1000000));
        log_append("MB, but you only had: ");
        log_append_uint((uint32_t)(unmanaged_memory_size / 1000000));
        log_append("MB remaining");
        log_assert(0);
        return NULL;
    }
    
    unmanaged_memory = ((char *)unmanaged_memory + size);
    unmanaged_memory_size -= size;
    
    return return_value;
}

void * T1_mem_malloc_from_unmanaged_aligned(
    const uint64_t size,
    const uint32_t aligned_to)
{
    T1_mem_mutex_lock(malloc_mutex_id);
    
    if (
        aligned_to < sizeof(void*) ||
        (aligned_to & (aligned_to - 1)) != 0)
    {
        assert(0); // this alignment is not allowed!
        return NULL;
    }
    
    log_assert(unmanaged_memory != NULL);
    log_assert(size > 0);
    
    uint32_t padding = aligned_to - ((uintptr_t)unmanaged_memory % aligned_to);
    if (padding == aligned_to) padding = 0;
    
    unmanaged_memory = ((char *)unmanaged_memory + padding);
    unmanaged_memory_size -= padding;
    
    log_assert(padding < aligned_to);
    log_assert((uintptr_t)(void *)unmanaged_memory % aligned_to == 0);
    
    void * return_value = T1_mem_malloc_from_unmanaged_without_aligning(size);
    
    log_assert((uintptr_t)(void *)return_value % aligned_to == 0);
    
    T1_mem_mutex_unlock(malloc_mutex_id);
    
    return return_value;
}

// __attribute__((used, noinline))
void * T1_mem_malloc_from_unmanaged(size_t size) {
    log_assert(size > 0);
    
    void * return_value = T1_mem_malloc_from_unmanaged_aligned(
        size,
        MEM_ALIGNMENT_BYTES);
    
    return return_value;
}

void T1_mem_malloc_from_managed_page_aligned(
    void ** base_pointer_for_freeing,
    void ** aligned_subptr,
    const size_t subptr_size)
{
    uint32_t aligned_to = T1_mem_page_size;
    
    *base_pointer_for_freeing = T1_mem_malloc_from_managed(
        subptr_size + aligned_to);
    
    uint32_t padding = aligned_to -
        (((uintptr_t)*base_pointer_for_freeing) % aligned_to);
    
    if (padding == aligned_to) { padding = 0; }
    
    *aligned_subptr = ((char *)*base_pointer_for_freeing) + padding;
    
    log_assert(*base_pointer_for_freeing != NULL);
    log_assert(*aligned_subptr != NULL);
    
    #if T1_MEM_ASSERTS_ACTIVE == T1_ACTIVE
    uint32_t alignment_miss =
        (uintptr_t)(*aligned_subptr) % aligned_to;
    log_assert(alignment_miss == 0);
    #elif T1_MEM_ASSERTS_ACTIVE == T1_INACTIVE
    #else
    #error
    #endif
}

void * T1_mem_malloc_from_managed_infoless(size_t size) {
    return T1_mem_malloc_from_managed_internal(size, "", "");
}

void * T1_mem_malloc_from_managed_internal(
    size_t size,
    char * called_from_file,
    char * called_from_func)
{
    #if 1
    (void)called_from_file;
    (void)called_from_func;
    
    return malloc(size);
    #else
    memstore_mutex_lock(malloc_mutex_id);
    
    log_assert(managed_memory != NULL);
    log_assert(size > 0);
     
    void * return_value = managed_memory;
    log_assert((uintptr_t)return_value % MEM_ALIGNMENT_BYTES == 0);
    
    uint32_t leftover = size % MEM_ALIGNMENT_BYTES;
    uint32_t endpadding = 0;
    if (leftover > 0) {
        endpadding = MEM_ALIGNMENT_BYTES - leftover;
    }
    
    managed_memory = (void *)((char *)return_value + size + endpadding);
    
    log_assert((uintptr_t)managed_memory < (uintptr_t)managed_memory_end);
    
    #if T1_MEM_ASSERTS_ACTIVE == T1_ACTIVE
    if (managed_stack->size > 0) {
        log_assert((uintptr_t)managed_stack->pointers[managed_stack->size - 1] <= (uintptr_t)return_value);
    }
    #elif T1_MEM_ASSERTS_ACTIVE == T1_INACTIVE
    #else
    #error
    #endif
    log_assert(managed_stack->size < MANAGED_MEMORY_STACK_SIZE);
    managed_stack->pointers[managed_stack->size] = return_value;
    T1_std_strcpy_cap(
        managed_stack->sources[managed_stack->size],
        512,
        called_from_file);
    T1_std_strcat_cap(
        managed_stack->sources[managed_stack->size],
        512,
        " : ");
    T1_std_strcat_cap(
        managed_stack->sources[managed_stack->size],
        512,
        called_from_func);
    managed_stack->used[managed_stack->size] = 1;
    managed_stack->size += 1;
    
    memstore_mutex_unlock(malloc_mutex_id);
    
    log_assert(return_value != NULL);
    return return_value;
    #endif
}

void T1_mem_free_from_managed(void * to_free) {
    
    #if 1
    free(to_free);
    #else
    
    memstore_mutex_lock(malloc_mutex_id);
    
    log_assert(to_free != NULL);
    log_assert(managed_memory != NULL);
    
    #ifndef T1_MEM_NO_ASSERTS
    bool32_t found = false;
    #endif
    for (uint32_t i = 0; i < managed_stack->size; i++) {
        if (managed_stack->used[i] &&
            managed_stack->pointers[i] == to_free)
        {
            managed_stack->used[i] = 0;
            managed_stack->sources[i][0] = '\0';
            log_assert(!found);
            #ifndef T1_MEM_NO_ASSERTS
            found = true;
            #endif
        }
    }
    log_assert(found);
    
    void * lowest_unused = managed_memory;
    while (
        managed_stack->size > 0 &&
        !managed_stack->used[managed_stack->size - 1])
    {
        log_assert(managed_stack->pointers[managed_stack->size - 1] != NULL);
        log_assert(
            managed_stack->pointers[managed_stack->size - 1] < lowest_unused);
        lowest_unused = managed_stack->pointers[managed_stack->size - 1];
        managed_stack->pointers[managed_stack->size - 1] = NULL;
        managed_stack->size -= 1;
    }
    
    log_assert(lowest_unused <= managed_memory);
    if (lowest_unused < managed_memory) {
        managed_memory = lowest_unused;
    }
    log_assert(
        ((uintptr_t)managed_memory_end - (uintptr_t)managed_memory) <=
            MANAGED_MEMORY_SIZE);
    
    memstore_mutex_unlock(malloc_mutex_id);
    #endif
    return;
}
