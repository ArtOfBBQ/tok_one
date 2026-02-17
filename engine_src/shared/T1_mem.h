#ifndef T1_MEM_H
#define T1_MEM_H

#if T1_MEM_ASSERTS_ACTIVE == T1_ACTIVE
#include <assert.h>
#elif T1_MEM_ASSERTS_ACTIVE == T1_INACTIVE
#else
#error
#endif

#include <stdlib.h>
#include <unistd.h>

#include "T1_std.h"
#include "T1_log.h"

#ifdef __cplusplus
extern "C" {
#endif

extern uint32_t T1_mem_page_size;

void T1_mem_init(
    void * ptr_unmanaged_memory_block,
    uint32_t (* memstore_init_mutex_and_return_id)(void),
    void (* ptr_mutex_lock)(const uint32_t mutex_id),
    void (* ptr_mutex_unlock)(const uint32_t mutex_id));

void T1_mem_get_usage_summary_string(
    char * recipient,
    const uint32_t recipient_cap);

void * T1_mem_malloc_unmanaged_aligned(
    const uint64_t size,
    const uint32_t aligned_to);

// __attribute__((used, noinline))
void * T1_mem_malloc_unmanaged(size_t size);

void T1_mem_malloc_managed_page_aligned(
    void ** base_pointer_for_freeing,
    void ** aligned_subptr,
    const size_t subptr_size);

bool8_t
T1_mem_is_page_aligned(void * to_check);

#define T1_mem_malloc_managed(size) T1_mem_malloc_managed_internal(size, (char *)__FILE__, (char *)__func__);
void * T1_mem_malloc_managed_internal(
    size_t size,
    char * called_from_file,
    char * called_from_func);
void * T1_mem_malloc_managed_infoless(size_t size);

void T1_mem_free_managed(void * to_free);

#ifdef __cplusplus
}
#endif

#endif // T1_MEM_H
