#ifndef T1_MEM_H
#define T1_MEM_H

#include <stddef.h>

#include "T1_stdint.h"
#include "T1_types_public.h"

/*
T1 allocates through the functions the client passed to T1_init():
"unmanaged" memory is never freed, "managed" memory is freed with
T1_mem_free_managed(). The client's functions only need to align like
malloc(), T1 aligns further itself.
*/

extern u32 T1_mem_page_size;

void T1_mem_init(
    void * (* alloc_forever)(size_t size),
    void * (* alloc_temp)(size_t size),
    void   (* free_temp)(void * ptr),
    const u32 page_size);

// Allocations from here on lock this mutex around the client's functions
void T1_mem_init_mutex(
    u32 (* init_mutex_and_return_id)(void),
    void (* mutex_lock)(const u32 mutex_id),
    void (* mutex_unlock)(const u32 mutex_id));

void * T1_mem_malloc_unmanaged_aligned(
    const u64 size,
    const u32 aligned_to);

// aligned to 64 bytes
void * T1_mem_malloc_unmanaged(size_t size);

void T1_mem_malloc_managed_page_aligned(
    void ** base_pointer_for_freeing,
    void ** aligned_subptr,
    const u64 subptr_size);

b8 T1_mem_is_page_aligned(void * to_check);

void * T1_mem_malloc_managed(size_t size);

void T1_mem_free_managed(void * to_free);

#endif // T1_MEM_H
