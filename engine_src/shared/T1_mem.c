#include "T1_mem.h"

#include "T1_std.h"
#include "T1_log.h"

#define MEM_ALIGNMENT_BYTES 64

u32 T1_mem_page_size = 4096;

static void * (* T1_mem_alloc_forever)(size_t size) = NULL;
static void * (* T1_mem_alloc_temp)(size_t size) = NULL;
static void   (* T1_mem_free_temp)(void * ptr) = NULL;

static u32 T1_mem_mutex_id = UINT32_MAX;
static void (* T1_mem_mutex_lock)(const u32 mutex_id) = NULL;
static void (* T1_mem_mutex_unlock)(const u32 mutex_id) = NULL;

void T1_mem_init(
    void * (* alloc_forever)(size_t size),
    void * (* alloc_temp)(size_t size),
    void   (* free_temp)(void * ptr),
    const u32 page_size)
{
    T1_mem_alloc_forever = alloc_forever;
    T1_mem_alloc_temp = alloc_temp;
    T1_mem_free_temp = free_temp;

    T1_log_assert(page_size > 0 && (page_size & (page_size - 1)) == 0);
    T1_mem_page_size = page_size;
}

void T1_mem_init_mutex(
    u32 (* init_mutex_and_return_id)(void),
    void (* mutex_lock)(const u32 mutex_id),
    void (* mutex_unlock)(const u32 mutex_id))
{
    T1_mem_mutex_id = init_mutex_and_return_id();
    T1_mem_mutex_lock = mutex_lock;
    T1_mem_mutex_unlock = mutex_unlock;
}

static void T1_mem_lock(void) {
    if (T1_mem_mutex_lock != NULL) {
        T1_mem_mutex_lock(T1_mem_mutex_id);
    }
}

static void T1_mem_unlock(void) {
    if (T1_mem_mutex_unlock != NULL) {
        T1_mem_mutex_unlock(T1_mem_mutex_id);
    }
}

void * T1_mem_malloc_unmanaged_aligned(
    u64 size,
    u32 aligned_to)
{
    T1_log_assert(T1_mem_alloc_forever != NULL);
    T1_log_assert(size > 0); // don't malloc for 0
    T1_log_assert(
        aligned_to >= sizeof(void*) &&
        (aligned_to & (aligned_to - 1)) == 0);

    // The client's function aligns like malloc(), so we may need to skip
    // up to aligned_to - 1 bytes
    T1_mem_lock();
    void * base = T1_mem_alloc_forever(size + aligned_to - 1);
    T1_mem_unlock();

    if (base == NULL) {
        T1_log_append("Tried to malloc_unmanaged for: ");
        T1_log_append_u32((u32)(size / 1000000));
        T1_log_append("MB, but the client's function returned NULL\n");
        T1_log_assert(0);
        return NULL;
    }

    u32 padding = aligned_to - ((uintptr_t)base % aligned_to);
    if (padding == aligned_to) { padding = 0; }

    void * return_value = (char *)base + padding;
    T1_log_assert((uintptr_t)return_value % aligned_to == 0);

    T1_std_memset(return_value, 0xCD, size);

    return return_value;
}

void * T1_mem_malloc_unmanaged(size_t size) {
    return T1_mem_malloc_unmanaged_aligned(size, MEM_ALIGNMENT_BYTES);
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
    T1_log_assert((uintptr_t)(*aligned_subptr) % aligned_to == 0);
}

b8 T1_mem_is_page_aligned(void * to_check)
{
    return (uintptr_t)to_check % T1_mem_page_size == 0;
}

void * T1_mem_malloc_managed(size_t size) {
    T1_log_assert(T1_mem_alloc_temp != NULL);
    T1_log_assert(size > 0);

    T1_mem_lock();
    void * out = T1_mem_alloc_temp(size);
    T1_mem_unlock();

    T1_log_assert(out != NULL);
    if (out != NULL) {
        T1_std_memset(out, 0xCD, size);
    }
    return out;
}

void T1_mem_free_managed(void * to_free) {
    T1_log_assert(T1_mem_free_temp != NULL);

    T1_mem_lock();
    T1_mem_free_temp(to_free);
    T1_mem_unlock();
}
