#ifndef TOK_RANDOM_H
#define TOK_RANDOM_H

#include "common.h"

#define RANDOM_IGNORE_ASSERTS
#ifndef RANDOM_IGNORE_ASSERTS
#include "assert.h"
#endif

#ifdef __cplusplus
extern "C" {
#endif

#define RANDOM_SEQUENCE_SIZE 999

extern uint32_t random_seed;

int32_t tok_rand(void);
int32_t tok_rand_at_i(const uint64_t index);

void shuffle_array(
    void * array,
    const uint32_t array_size,
    const uint32_t element_size);

#ifdef __cplusplus
}
#endif

#endif // TOK_RANDOM_H
