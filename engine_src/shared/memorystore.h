#ifndef MEMORY_STORE_H
#define MEMORY_STORE_H

#ifndef MEMORY_STORE_IGNORE_ASSERTS
#include <assert.h>
#endif

#include "clientlogic_macro_settings.h"
#include "common.h"
#include "logger.h"

#ifdef __cplusplus
extern "C" {
#endif

void get_memory_usage_summary_string(
    char * recipient,
    const uint32_t recipient_cap);

void init_memory_store(
    void * ptr_unmanaged_memory_block,
    void * ptr_managed_memory_block);

void * malloc_from_unmanaged_aligned(
    const uint64_t size,
    const uint32_t aligned_to);

void * malloc_from_unmanaged(size_t size);
#define malloc_from_managed(size) malloc_from_managed_internal(size, __FILE__, __func__);
void * malloc_from_managed_internal(
    size_t size,
    char * called_from_file,
    char * called_from_func);
void * malloc_from_managed_infoless(size_t size);

void free_from_managed(void * to_free);

#ifdef __cplusplus
}
#endif

#endif // MEMORY_STORE_H
