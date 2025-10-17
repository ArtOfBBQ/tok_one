#ifndef TOK_RANDOM_H
#define TOK_RANDOM_H

#include "T1_simd.h"
#include "T1_std.h"

// #define RANDOM_IGNORE_ASSERTS
#ifndef RANDOM_IGNORE_ASSERTS
#include "assert.h"
#endif

#ifdef __cplusplus
extern "C" {
#endif

#define FLOAT_SEQUENCE_SIZE  4760
#define RANDOM_SEQUENCE_SIZE  999

extern uint32_t random_seed;

int32_t T1_rand(void);
int32_t T1_rand_at_i(const uint64_t index);

SIMD_FLOAT T1_rand_simd_at_i(const uint64_t index);

void T1_rand_shuffle_array(
    void * array,
    const uint32_t array_size,
    const uint32_t element_size);

#ifdef __cplusplus
}
#endif

#endif // TOK_RANDOM_H
