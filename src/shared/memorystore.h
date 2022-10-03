#ifndef MEMORY_STORE_H
#define MEMORY_STORE_H

#include "platform_layer.h"
#include "common.h"
#include "logger.h"
#include "stdlib.h"

// extern uint8_t * unmanaged_memory;
// extern uint64_t unmanaged_memory_size;
// extern uint8_t * managed_memory;
// extern uint64_t managed_memory_size;

void init_memory_store();

#define malloc_struct_from_unmanaged(type) (type *)malloc_from_unmanaged(sizeof(type))
uint8_t * malloc_from_unmanaged(uint64_t size);

uint8_t * malloc_from_managed(uint64_t size);
void free_from_managed(uint8_t * to_free);

#endif
