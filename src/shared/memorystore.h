#ifndef MEMORY_STORE_H
#define MEMORY_STORE_H

// 500mb ->                   500...000
#define UNMANAGED_MEMORY_SIZE 500000000
// 500mb ->                   250...000
#define   MANAGED_MEMORY_SIZE 500000000

#include "common.h"
#include "logger.h"
#include "stdlib.h"

extern uint8_t * unmanaged_memory;
extern uint64_t unmanaged_memory_size;
extern uint8_t * managed_memory;
extern uint64_t managed_memory_size;

#define malloc_struct_from_unmanaged(type) (type *)malloc_from_unmanaged(sizeof(type))
uint8_t * malloc_from_unmanaged(uint64_t size);

uint8_t * malloc_from_managed(uint64_t size);
void free_from_managed(uint8_t * to_free);

#endif
