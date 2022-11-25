#ifndef MEMORY_STORE_H
#define MEMORY_STORE_H

#ifndef MEMORY_STORE_IGNORE_ASSERTS
#include <assert.h>
#endif

#include "platform_layer.h"
#include "common.h"


#ifdef __cplusplus
extern "C" {
#endif

void init_memory_store(void);

uint8_t * malloc_from_unmanaged_aligned(
    const uint64_t size,
    const uint32_t aligned_to);

#define malloc_struct_from_unmanaged(type) (type *)malloc_from_unmanaged(sizeof(type))
uint8_t * malloc_from_unmanaged(const uint64_t size);

uint8_t * malloc_from_managed(const uint64_t size);
void free_from_managed(uint8_t * to_free);

#ifdef __cplusplus
}
#endif

#endif // MEMORY_STORE_H
