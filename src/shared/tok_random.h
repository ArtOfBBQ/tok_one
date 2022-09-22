#ifndef TOK_RANDOM_H
#define TOK_RANDOM_H

#include "common.h"

void init_rand_with_seed(const uint64_t seed);
uint32_t tok_rand();
void shuffle_array(void * array, const uint32_t array_size, const uint32_t element_size);

#endif

