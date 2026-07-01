#include "T1_mem.h"

#include <stdlib.h>
#include <unistd.h>
#include "T1_std.h"
#include "T1_log.h"


#define MEM_ALIGNMENT_BYTES 64

//#define MANAGED_MEMORY_STACK_SIZE 1000
//typedef struct ManagedMemoryStack {
//    void * pointers[MANAGED_MEMORY_STACK_SIZE];
//    char sources[MANAGED_MEMORY_STACK_SIZE][512];
//    bool32_t used[MANAGED_MEMORY_STACK_SIZE];
//    u32 size;
//} ManagedMemoryStack;
//static ManagedMemoryStack * managed_stack = NULL;

// u32 memorystore_page_size = 4000;

u32 T1_mem_page_size = 4096;

static void * unmanaged_memory = NULL;
static u64 unmanaged_memory_size = T1_UNMANAGED_MEM_CAP;
// static void * managed_memory = NULL;
// static void * managed_memory_end = NULL;

static u32 malloc_mutex_id = UINT32_MAX;

void (* T1_mem_mutex_lock)(const u32 mutex_id) = NULL;
void (* T1_mem_mutex_unlock)(const u32 mutex_id) = NULL;

static void T1_mem_set_pagesize(void) {
    long query_result = sysconf(_SC_PAGESIZE);
    if (query_result < 0 || query_result > UINT32_MAX) {
        assert(0);
        T1_mem_page_size = 4096;
    } else {
        T1_mem_page_size = (u32)query_result;
    }
}

void T1_mem_get_usage_summary_string(
    char * recipient,
    const u32 recipient_cap)
{
    #if T1_STD_ASSERTS_ACTIVE == T1_ACTIVE
    (void)recipient_cap;
    #elif T1_STD_ASSERTS_ACTIVE == T1_INACTIVE
    #else
    #error
    #endif
    
    T1_std_strcpy_cap(
        recipient,
        recipient_cap,
        "Unmanaged memory use: ");
    T1_std_strcat_u32_cap(
        recipient,
        recipient_cap,
        T1_UNMANAGED_MEM_CAP - (u32)unmanaged_memory_size);
    T1_std_strcat_cap(
        recipient,
        recipient_cap,
        " of: ");
    T1_std_strcat_u32_cap(
        recipient,
        recipient_cap,
        T1_UNMANAGED_MEM_CAP);
    T1_std_strcat_cap(
        recipient,
        recipient_cap,
        " (");
    T1_std_strcat_u32_cap(
        recipient,
        recipient_cap,
        (u32)(
            (f32)(T1_UNMANAGED_MEM_CAP - (u32)unmanaged_memory_size)
                / (f32)T1_UNMANAGED_MEM_CAP * 100.0f));
}

static void T1_mem_align_pointer(void ** to_align) {
    u32 leftover = ((uintptr_t)*to_align) % MEM_ALIGNMENT_BYTES;
    u32 padding = 0;
    if (leftover > 0) {
        padding = MEM_ALIGNMENT_BYTES - leftover;
    }
    
    *to_align = (void *)((char *)*to_align + padding);
    T1_log_assert((uintptr_t)*to_align % MEM_ALIGNMENT_BYTES == 0);
}

void T1_mem_init(
    void * ptr_unmanaged_memory_block,
    u32 (* memstore_init_mutex_and_return_id)(void),
    void (* ptr_mutex_lock)(const u32 mutex_id),
    void (* ptr_mutex_unlock)(const u32 mutex_id))
{
    malloc_mutex_id  = memstore_init_mutex_and_return_id();
    
    T1_mem_mutex_lock = ptr_mutex_lock;
    T1_mem_mutex_unlock = ptr_mutex_unlock;
    
    unmanaged_memory = ptr_unmanaged_memory_block;
    T1_mem_align_pointer(&unmanaged_memory);
    T1_std_memset(unmanaged_memory, 0, T1_UNMANAGED_MEM_CAP);
    
    T1_mem_set_pagesize();
}

static void * T1_mem_malloc_from_unmanaged_without_aligning(
    const u64 size)
{
    void * return_value = unmanaged_memory;
    if (size >= unmanaged_memory_size) {
        T1_log_append("Tried to malloc_from_unamanged for: ");
        T1_log_append_u32((u32)(size / 1000000));
        T1_log_append("MB, but you only had: ");
        T1_log_append_u32((u32)(unmanaged_memory_size / 1000000));
        T1_log_append("MB remaining");
        T1_log_assert(0);
        return NULL;
    }
    
    unmanaged_memory = ((char *)unmanaged_memory + size);
    unmanaged_memory_size -= size;
    
    return return_value;
}

void * T1_mem_malloc_unmanaged_aligned(
    const u64 size,
    const u32 aligned_to)
{
    T1_mem_mutex_lock(malloc_mutex_id);
    
    if (
        aligned_to < sizeof(void*) ||
        (aligned_to & (aligned_to - 1)) != 0)
    {
        assert(0); // this alignment is not allowed!
        return NULL;
    }
    
    T1_log_assert(unmanaged_memory != NULL);
    T1_log_assert(size > 0); // don't malloc for 0
    
    u32 padding = aligned_to - ((uintptr_t)unmanaged_memory % aligned_to);
    if (padding == aligned_to) padding = 0;
    
    unmanaged_memory = ((char *)unmanaged_memory + padding);
    unmanaged_memory_size -= padding;
    
    T1_log_assert(padding < aligned_to);
    T1_log_assert((uintptr_t)(void *)unmanaged_memory % aligned_to == 0);
    
    void * return_value = T1_mem_malloc_from_unmanaged_without_aligning(size);
    
    T1_log_assert((uintptr_t)(void *)return_value % aligned_to == 0);
    
    T1_mem_mutex_unlock(malloc_mutex_id);
    
    return return_value;
}

// __attribute__((used, noinline))
void * T1_mem_malloc_unmanaged(u64 size) {
    T1_log_assert(size > 0);
    
    void * return_value = T1_mem_malloc_unmanaged_aligned(
        size,
        MEM_ALIGNMENT_BYTES);
    
    return return_value;
}

void T1_mem_malloc_managed_page_aligned(
    void ** base_pointer_for_freeing,
    void ** aligned_subptr,
    const u64 subptr_size)
{
    u32 aligned_to = T1_mem_page_size;
    
    *base_pointer_for_freeing = T1_mem_malloc_managed(
        subptr_size + aligned_to);
    
    u32 padding = aligned_to -
        (((uintptr_t)*base_pointer_for_freeing) % aligned_to);
    
    if (padding == aligned_to) { padding = 0; }
    
    *aligned_subptr = ((char *)*base_pointer_for_freeing) + padding;
    
    T1_log_assert(*base_pointer_for_freeing != NULL);
    T1_log_assert(*aligned_subptr != NULL);
    
    #if T1_MEM_ASSERTS_ACTIVE == T1_ACTIVE
    u32 alignment_miss =
        (uintptr_t)(*aligned_subptr) % aligned_to;
    T1_log_assert(alignment_miss == 0);
    #elif T1_MEM_ASSERTS_ACTIVE == T1_INACTIVE
    #else
    #error
    #endif
}

b8 T1_mem_is_page_aligned(void * to_check)
{
    return (uintptr_t)to_check % T1_mem_page_size == 0;
}

void * T1_mem_malloc_managed(
    u64 size)
{
    #if 1
    return malloc(size);
    #else
    memstore_mutex_lock(malloc_mutex_id);
    
    log_assert(managed_memory != NULL);
    log_assert(size > 0);
     
    void * return_value = managed_memory;
    log_assert((uintptr_t)return_value % MEM_ALIGNMENT_BYTES == 0);
    
    u32 leftover = size % MEM_ALIGNMENT_BYTES;
    u32 endpadding = 0;
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

void T1_mem_free_managed(void * to_free) {
    
    #if 1
    free(to_free);
    #else
    
    memstore_mutex_lock(malloc_mutex_id);
    
    log_assert(to_free != NULL);
    log_assert(managed_memory != NULL);
    
    #ifndef T1_MEM_NO_ASSERTS
    bool32_t found = false;
    #endif
    for (u32 i = 0; i < managed_stack->size; i++) {
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
