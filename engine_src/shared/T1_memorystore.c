#include "T1_memorystore.h"

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

uint32_t page_size = 4096;

static void * unmanaged_memory = NULL;
static uint64_t unmanaged_memory_size = UNMANAGED_MEMORY_SIZE;
// static void * managed_memory = NULL;
// static void * managed_memory_end = NULL;

static uint32_t malloc_mutex_id = UINT32_MAX;

void (* memstore_mutex_lock)(const uint32_t mutex_id) = NULL;
void (* memstore_mutex_unlock)(const uint32_t mutex_id) = NULL;

static void set_pagesize(void) {
    long query_result = sysconf(_SC_PAGESIZE);
    if (query_result < 0 || query_result > UINT32_MAX) {
        assert(0);
        page_size = 4096;
    } else {
        page_size = (uint32_t)query_result;
    }
}

void get_memory_usage_summary_string(
    char * recipient,
    const uint32_t recipient_cap)
{
    #ifdef COMMON_IGNORE_ASSERTS
    (void)recipient_cap;
    #endif
    
    common_strcpy_capped(
        recipient,
        recipient_cap,
        "Unmanaged memory use: ");
    common_strcat_uint_capped(
        recipient,
        recipient_cap,
        UNMANAGED_MEMORY_SIZE - (uint32_t)unmanaged_memory_size);
    common_strcat_capped(
        recipient,
        recipient_cap,
        " of: ");
    common_strcat_uint_capped(
        recipient,
        recipient_cap,
        UNMANAGED_MEMORY_SIZE);
    common_strcat_capped(
        recipient,
        recipient_cap,
        " (");
    common_strcat_uint_capped(
        recipient,
        recipient_cap,
        (uint32_t)(
            (float)(UNMANAGED_MEMORY_SIZE - (uint32_t)unmanaged_memory_size)
                / (float)UNMANAGED_MEMORY_SIZE * 100.0f));
}

static void align_pointer(void ** to_align) {
    uint32_t leftover = ((uintptr_t)*to_align) % MEM_ALIGNMENT_BYTES;
    uint32_t padding = 0;
    if (leftover > 0) {
        padding = MEM_ALIGNMENT_BYTES - leftover;
    }
    
    *to_align = (void *)((char *)*to_align + padding);
    log_assert((uintptr_t)*to_align % MEM_ALIGNMENT_BYTES == 0);
}


void memorystore_init(
    void * ptr_unmanaged_memory_block,
    uint32_t (* memstore_init_mutex_and_return_id)(void),
    void (* ptr_mutex_lock)(const uint32_t mutex_id),
    void (* ptr_mutex_unlock)(const uint32_t mutex_id))
{
    malloc_mutex_id  = memstore_init_mutex_and_return_id();
    
    memstore_mutex_lock = ptr_mutex_lock;
    memstore_mutex_unlock = ptr_mutex_unlock;
    
    unmanaged_memory = ptr_unmanaged_memory_block;
    align_pointer(&unmanaged_memory);
    common_memset_char(unmanaged_memory, 0, UNMANAGED_MEMORY_SIZE);
    
    set_pagesize();
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
        log_assert(0);
        return NULL;
    }
    
    unmanaged_memory = ((char *)unmanaged_memory + size);
    unmanaged_memory_size -= size;
    
    return return_value;
}

void * malloc_from_unmanaged_aligned(
    const uint64_t size,
    const uint32_t aligned_to)
{
    memstore_mutex_lock(malloc_mutex_id);
    
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
    
    void * return_value = malloc_from_unmanaged_without_aligning(size);
    
    log_assert((uintptr_t)(void *)return_value % aligned_to == 0);
    
    memstore_mutex_unlock(malloc_mutex_id);
    
    return return_value;
}

// __attribute__((used, noinline))
void * malloc_from_unmanaged(size_t size) {
    log_assert(size > 0);
    void * return_value = malloc_from_unmanaged_aligned(
        size,
        MEM_ALIGNMENT_BYTES);
    
    return return_value;
}

void malloc_from_managed_page_aligned(
    void ** base_pointer_for_freeing,
    void ** aligned_subptr,
    const size_t subptr_size)
{
    uint32_t aligned_to = page_size;
    
    *base_pointer_for_freeing = malloc_from_managed(subptr_size + aligned_to);
    
    uint32_t padding = aligned_to -
        (((uintptr_t)*base_pointer_for_freeing) % aligned_to);
    
    if (padding == aligned_to) { padding = 0; }
    
    *aligned_subptr = ((char *)*base_pointer_for_freeing) + padding;
    
    log_assert(*base_pointer_for_freeing != NULL);
    log_assert(*aligned_subptr != NULL);
    
    #ifndef LOGGER_IGNORE_ASSERTS
    uint32_t alignment_miss =
        (uintptr_t)(*aligned_subptr) % aligned_to;
    log_assert(alignment_miss == 0);
    #endif
}

void * malloc_from_managed_infoless(size_t size) {
    return malloc_from_managed_internal(size, "", "");
}

void * malloc_from_managed_internal(
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
    
    #ifndef LOGGER_IGNORE_ASSERTS
    if (managed_stack->size > 0) {
        log_assert((uintptr_t)managed_stack->pointers[managed_stack->size - 1] <= (uintptr_t)return_value);
    }
    #endif
    log_assert(managed_stack->size < MANAGED_MEMORY_STACK_SIZE);
    managed_stack->pointers[managed_stack->size] = return_value;
    common_strcpy_capped(
        managed_stack->sources[managed_stack->size],
        512,
        called_from_file);
    common_strcat_capped(
        managed_stack->sources[managed_stack->size],
        512,
        " : ");
    common_strcat_capped(
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

void free_from_managed(void * to_free) {
    
    #if 1
    free(to_free);
    #else
    
    memstore_mutex_lock(malloc_mutex_id);
    
    log_assert(to_free != NULL);
    log_assert(managed_memory != NULL);
    
    #ifndef LOGGER_IGNORE_ASSERTS
    bool32_t found = false;
    #endif
    for (uint32_t i = 0; i < managed_stack->size; i++) {
        if (managed_stack->used[i] &&
            managed_stack->pointers[i] == to_free)
        {
            managed_stack->used[i] = 0;
            managed_stack->sources[i][0] = '\0';
            log_assert(!found);
            #ifndef LOGGER_IGNORE_ASSERTS
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
