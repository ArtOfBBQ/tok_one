#ifndef MEMORY_STORE_H
#define MEMORY_STORE_H

#ifndef MEMORY_STORE_IGNORE_ASSERTS
#include <assert.h>
#endif

#include "clientlogic_macro_settings.h"
#include "common.h"
#include "logger.h"
#include "platform_layer.h"

#ifdef __cplusplus
extern "C" {
#endif

void init_memory_store(void);

uint8_t * malloc_from_unmanaged_aligned(
    const uint64_t size,
    const uint32_t aligned_to);

void * malloc_from_unmanaged(size_t size);
void * malloc_from_managed(size_t size);

void free_from_managed(uint8_t * to_free);

#ifdef __cplusplus
}
#endif

#endif // MEMORY_STORE_H
