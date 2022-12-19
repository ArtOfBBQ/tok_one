#ifndef TOKONE_SIMD_H
#define TOKONE_SIMD_H

/*
A wrapper around Intel AVX instructions and Arm Neon instructions, for floats
only, so that you can write basic simd instructions and have them be
interpreted as:
- Arm Neon instructions where supported
- Intel instructions where supported
- Bog-standard 32-bit floating point math failing the above

I want function-like macros instead of function definitions to avoid the tiny
overhead of calling a function, and I don't want to rely on compiler inlining
because I don't know much about how that works
*/

#ifdef __ARM_NEON
#include "arm_neon.h"
#define SIMD_FLOAT_WIDTH 4
#define SIMD_FLOAT float32x4_t

#define simd_load_floats(floatsptr) vld1q_f32(floatsptr)
#define simd_set_float(float) vld1q_dup_f32(&float)
#define simd_store_floats(to_floatsptr, from) vst1q_f32(to_floatsptr, from)
#define simd_mul_floats(a, b) vmulq_f32(a, b)
#define simd_div_floats(a, b) vdivq_f32(a, b)
#define simd_add_floats(a, b) vaddq_f32(a, b)
#define simd_sub_floats(a, b) vsubq_f32(a, b)
#define simd_max_floats(a, b) vmaxq_f32(a, b)
#define simd_sqrt_floats(a) vsqrtq_f32(a)

#elif defined(__AVX__)
#include "immintrin.h"
#define SIMD_FLOAT_WIDTH 8
#define SIMD_FLOAT __m256

#define simd_load_floats(floatsptr) _mm256_load_ps(floatsptr)
#define simd_set_float(float) _mm256_set_ps(float, float, float, float, float, float, float, float)
#define simd_store_floats(floatsptr, from) _mm256_store_ps(floatsptr, from)
#define simd_mul_floats(a, b) _mm256_mul_ps(a, b)
#define simd_div_floats(a, b) _mm256_div_ps(a, b)
#define simd_add_floats(a, b) _mm256_add_ps(a, b)
#define simd_sub_floats(a, b) _mm256_sub_ps(a, b)
#define simd_max_floats(a, b) _mm256_max_ps(a, b)
#define simd_sqrt_floats(a) _mm256_sqrt_ps(a)

#else
#define SIMD_FLOAT_WIDTH 1
#define SIMD_FLOAT float

#define simd_load_floats(floatsptr) *floatsptr
#define simd_set_float(float) float
#define simd_store_floats(floatsptr, from) (floatsptr)[0] = from
#define simd_mul_floats(a, b) a * b
#define simd_div_floats(a, b) a / b
#define simd_add_floats(a, b) a + b
#define simd_sub_floats(a, b) a - b
#define simd_max_floats(a, b) fmax(a, b)
#define simd_sqrt_floats(a) sqrtf(a)
#endif

#endif
