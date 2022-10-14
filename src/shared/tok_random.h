#ifndef TOK_RANDOM_H
#define TOK_RANDOM_H

#include "common.h"

#ifdef __cplusplus
extern "C" {
#endif

void init_rand_with_seed(const uint64_t seed);
uint32_t tok_rand(void);
void shuffle_array(void * array, const uint32_t array_size, const uint32_t element_size);

#ifdef __cplusplus
}
#endif

#endif // TOK_RANDOM_H
