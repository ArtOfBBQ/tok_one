#ifndef T1_RAND_H
#define T1_RAND_H

#include "T1_stdint.h"
#include "T1_simd.h"

#ifdef __cplusplus
extern "C" {
#endif

#define FLOAT_SEQUENCE_SIZE  4760
#define T1_RAND_SEQUENCE_SIZE  999

extern u32 T1_rand_seed;

void T1_rand_init(const u32 seed);
s32 T1_rand(void);
s32 T1_rand_at_i(const u64 index);

SIMD_FLOAT T1_rand_simd_at_i(const u64 index);

void T1_rand_shuf_array(
    void * array,
    const u32 array_size,
    const u32 element_size);

#ifdef __cplusplus
}
#endif

#endif // T1_RANDOM_H
