#ifndef TOKONE_SIMD_H
#define TOKONE_SIMD_H

/*
A wrapper around Intel AVX instructions and Arm Neon instructions, for floats
only, so that you can write basic simd instructions and have them be
interpreted as:
- Arm Neon instructions where supported
- Intel AVX2 instructions where supported
- Bog-standard 32-bit floating point math failing the above

I want function-like macros instead of function definitions to avoid the tiny
overhead of calling a function, and I don't want to rely on compiler inlining
because I don't know much about how that works or how reliable it is
*/

// TODO: implement int8 lanes
/*
#ifdef __ARM_NEON_


#elif defined(__SSE2__)

#include "immintrin.h"
#define SIMD_INT8_WIDTH                     16
#define SIMD_INT8                           __m128i
#define simd_load_int8s(int8sptr)           _mm_loadu_si128((const __m128i_u *)(int8sptr))
#define simd_set_int8s(i8)                  _mm_set_epi8(i8, i8, i8, i8, i8, i8, i8, i8, i8, i8, i8, i8, i8, i8, i8, i8)
#define simd_store_int8s(recip, from)       _mm_storeu_si128((__m128i_u *)recip, from)
#define simd_add_int8s(a, b)                _mm_adds_epi8(a, b)
#define simd_sub_int8s(a, b)                _mm_sub_epi8(a, b)

#define simd_cmpgt_int8s(a, b)              _mm_cmpgt_epi8(a, b)
#define simd_cmplt_int8s(a, b)              _mm_cmplt_epi8(a, b)
#define simd_cmpeq_int8s(a, b)              _mm_cmpeq_epi8(a, b)
#define simd_test_all_ones_int8s(a)         _mm_test_all_ones(a)
#define simd_testz_int8s(a, b)              _mm_testz_si128(a, b)
#else

#define SIMD_INT8_WIDTH                     1
#define SIMD_INT8                           int8_t
#define simd_load_int8s(int8sptr)           (int8sptr)[0]
#define simd_set_int8s(i8)                  i8
#define simd_store_int8s(recip, from)       (recip)[0] = from
#define simd_mul_int8s(a, b)                a * b
#define simd_div_int8s(a, b)                a / b
#define simd_add_int8s(a, b)                a + b
#define simd_sub_int8s(a, b)                a - b
#define simd_max_int16s(a, b)               ((a > b)*a)+((b <= a)*b)
#define simd_min_int16s(a, b)               ((a > b)*b)+((b <= a)*a)

#endif // int8 lanes
*/


// int16 lanes
#ifdef __ARM_NEON
#include "arm_neon.h"
#define SIMD_INT16_WIDTH                    8
#define SIMD_INT16                          int16x8_t
#define simd_load_int16s(int16sptr)         vld1q_s16(int16sptr)
#define simd_set_int16s(i16)                vld1q_dup_s16(&i16)
#define simd_store_int16s(recip, from)      vst1q_s16(recip, from)
#define simd_mul_int16s(a, b)               vmulq_s16(a, b)
#define simd_add_int16s(a, b)               vaddq_s16(a, b)
#define simd_sub_int16s(a, b)               vsubq_s16(a, b)
#define simd_max_int16s(a, b)               vmaxq_s16(a, b)
#define simd_min_int16s(a, b)               vminq_s16(a, b)

// cmpgt stands for "Compare Greater". This returns 255 when true, not 1.
#define simd_cmpgt_int16s(a, b)             vcgtq_s16(a, b)
#define simd_cmplt_int16s(a, b)             vcltq_s16(a, b)
#define simd_cmpeq_int16s(a, b)             vcleq_s16(a, b)
#define simd_testz_int16s(a, b)             (vmaxvq_s16(a) == 0 && vmaxvq_s16(b) == 0)
#define simd_test_all_ones_int16s(a)        (vmaxvq_s16(a) != 0)

/*
// TODO: implement AVX2 when we're on a CPU that supports it
#elif defined(__AVX__) && defined(__AVX2__)

#include "immintrin.h"
#define SIMD_INT16_WIDTH                    16
#define SIMD_INT16                          __m256i
#define simd_load_int16s(int16sptr)         _mm256_loadu_si256(int16sptr) // AVX
#define simd_set_int16s(i16)                _mm256_set1_epi16 // AVX
#define simd_store_int16s(recip, from)      _mm256_storeu_si256(__m256i_u *)recip, from) // AVX
#define simd_mul_int16s(a, b)               _mm256_mullo_epi16(a, b) // AVX2
#define simd_add_int16s(a, b)               _mm256_add_epi16(a, b) // AVX2
#define simd_sub_int16s(a, b)               _mm256_sub_epi16(a, b) // AVX2
#define simd_max_int16s(a, b)               _mm256_max_epi16(a, b) // AVX2
#define simd_min_int16s(a, b)               _mm256_min_epi16(a, b) // AVX2

#define simd_cmpgt_int16s(a, b)             _mm256_cmpgt_epi16(a, b) // AVX2
#define simd_cmplt_int16s(a, b)             _mm256_cmplt_epi16(a, b) // Annoyingly doesn't exist :/
#define simd_cmpeq_int16s(a, b)             _mm256_cmpeq_epi16(a, b) // AVX2
#define simd_testz_int16s(a, b)             // annoyingly doesn't exist
#define simd_test_all_ones_int16s(a)        // annoyingly doesn't exist
*/
#elif defined(__SSE2__) && defined(__SSE4_1__)

#include "immintrin.h"
#define SIMD_INT16_WIDTH                    8
#define SIMD_INT16                          __m128i
#define simd_load_int16s(int16sptr)         _mm_loadu_si128((const __m128i_u *)(int16sptr)) // SSE2
#define simd_set_int16s(i16)                _mm_set1_epi16(i16) // SSE2
#define simd_store_int16s(recip, from)      _mm_storeu_si128((__m128i_u *)recip, from) // SSE2
#define simd_mul_int16s(a, b)               _mm_mullo_epi16(a, b) // SSE2
#define simd_add_int16s(a, b)               _mm_add_epi16(a, b) // SSE2
#define simd_sub_int16s(a, b)               _mm_sub_epi16(a, b) // SSE2
#define simd_max_int16s(a, b)               _mm_max_epi16(a, b) // SSE2
#define simd_min_int16s(a, b)               _mm_min_epi16(a, b) // SSE2

// cmpgt stands for "Compare Greater". This returns 255 when true, not 1.
#define simd_cmpgt_int16s(a, b)             _mm_cmpgt_epi16(a, b) // SSE2
#define simd_cmplt_int16s(a, b)             _mm_cmplt_epi16(a, b) // SSE2
#define simd_cmpeq_int16s(a, b)             _mm_cmpeq_epi16(a, b) // SSE2
#define simd_testz_int16s(a, b)             _mm_testz_si128(a, b) // SSE4.1
#define simd_test_all_ones_int16s(a)        _mm_test_all_ones(a)  // SSE4.1
#else

#define SIMD_INT16_WIDTH                    1
#define SIMD_INT16                          int16_t
#define simd_load_int16s(int16sptr)         (int16sptr)[0]
#define simd_set_int16s(i16)                i16
#define simd_store_int16s(recip, from)      (recip)[0] = from
#define simd_cmpgt_int16s(a, b)             a > b
#define simd_cmplt_int16s(a, b)             a < b
#define simd_cmpeq_int16s(a, b)             a == b
#define simd_mul_int16s(a, b)               a * b
#define simd_add_int16s(a, b)               a + b
#define simd_sub_int16s(a, b)               a - b
#define simd_test_all_ones_int16s(a)        a == 1
#define simd_max_int16s(a, b)               ((a>b)*a)+((a<=b)*b)
#define simd_min_int16s(a, b)               ((a>b)*b)+((a<=b)*a)
#endif

// Float lanes
#ifdef __ARM_NEON

#include "arm_neon.h"
#define SIMD_FLOAT_WIDTH                    4
#define SIMD_FLOAT                          float32x4_t
#define simd_load_floats(floatsptr)         vld1q_f32(floatsptr)
#define simd_set_float(float)               vld1q_dup_f32(&float)
#define simd_store_floats(to_ptr, from)     vst1q_f32(to_ptr, from)
#define simd_mul_floats(a, b)               vmulq_f32(a, b)
#define simd_div_floats(a, b)               vdivq_f32(a, b)
#define simd_add_floats(a, b)               vaddq_f32(a, b)
#define simd_sub_floats(a, b)               vsubq_f32(a, b)
#define simd_max_floats(a, b)               vmaxq_f32(a, b)
#define simd_sqrt_floats(a)                 vsqrtq_f32(a)

#elif defined(__AVX__)

#include "immintrin.h"
#define SIMD_FLOAT_WIDTH                    8
#define SIMD_FLOAT                          __m256
#define simd_load_floats(floatsptr)         _mm256_load_ps(floatsptr)
#define simd_set_float(float)               _mm256_set_ps(float, float, float, float, float, float, float, float)
#define simd_store_floats(floatsptr, from)  _mm256_store_ps(floatsptr, from)
#define simd_mul_floats(a, b)               _mm256_mul_ps(a, b)
#define simd_div_floats(a, b)               _mm256_div_ps(a, b)
#define simd_add_floats(a, b)               _mm256_add_ps(a, b)
#define simd_sub_floats(a, b)               _mm256_sub_ps(a, b)
#define simd_max_floats(a, b)               _mm256_max_ps(a, b)
#define simd_sqrt_floats(a)                 _mm256_sqrt_ps(a)

#else

#define SIMD_FLOAT_WIDTH                    1
#define SIMD_FLOAT                          float
#define simd_load_floats(floatsptr)         (floatsptr)[0]
#define simd_set_float(float)               float
#define simd_store_floats(floatsptr, from)  (floatsptr)[0] = from
#define simd_mul_floats(a, b)               a * b
#define simd_div_floats(a, b)               a / b
#define simd_add_floats(a, b)               a + b
#define simd_sub_floats(a, b)               a - b
#define simd_max_floats(a, b)               tok_fmaxf(a, b)
#define simd_sqrt_floats(a)                 sqrtf(a)

#endif // Float lanes

#endif // TOKONE_SIMD_H
