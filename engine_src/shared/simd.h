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

// TODO: implement int8 lanes, also maybe think about ditching mmx
// since it doesn't have min/max and SSE onward do have those
//#if defined(__MMX__)
//#define SIMD_INT8_LANES                    8
//#define SIMD_INT8                          __m64
//#define simd_load_int8s(int16sptr)         _m_from_int64(__int64 a)
//#define simd_set1_int8s(i8)                _mm_set1_pi8(i8) // MMX
//#define simd_store_int8s(recip, from)      (recip)[0] = _m_to_int64(from) // MMX
//#define simd_add_int8s(a, b)               _mm_add_pi8(a, b) // MMX
//#define simd_sub_int8s(a, b)               _mm_sub_pi8(a, b) // MMX
//#endif

// int16 lanes
#if defined(__ARM_NEON)
#include "arm_neon.h"
#define SIMD_INT16_LANES                    8
#define SIMD_INT16                          int16x8_t
#define simd_load_int16s(int16sptr)         vld1q_s16(int16sptr)
#define simd_set1_int16s(i16)               vld1q_dup_s16(&i16)
#define simd_store_int16s(recip, from)      vst1q_s16(recip, from)
#define simd_mul_int16s(a, b)               vmulq_s16(a, b)
#define simd_add_int16s(a, b)               vaddq_s16(a, b)
#define simd_sub_int16s(a, b)               vsubq_s16(a, b)
#define simd_max_int16s(a, b)               vmaxq_s16(a, b)
#define simd_and_int16s(a, b)               vandq_s16(a, b)
#define simd_min_int16s(a, b)               vminq_s16(a, b)

// cmpgt stands for "Compare Greater". This returns 255 when true, not 1.
#define simd_cmpgt_int16s(a, b)             vcgtq_s16(a, b)
#define simd_cmplt_int16s(a, b)             vcltq_s16(a, b)
#define simd_cmpeq_int16s(a, b)             vandq_s16(vcgtq_s16(a, b), vcltq_s16(a, b))
#define simd_testz_int16s(a, b)             (vmaxvq_s16(a) == 0 && vmaxvq_s16(b) == 0)
#define simd_test_all_bitsset_int16s(a)     (vmaxvq_s16(a) != 0)

/*
// TODO: implement AVX2 when we're on a CPU that supports it
#elif defined(__AVX__) && defined(__AVX2__)

#include "immintrin.h"
#define SIMD_INT16_LANES                    16
#define SIMD_INT16                          __m256i
#define simd_load_int16s(int16sptr)         _mm256_loadu_si256(int16sptr) // AVX
#define simd_set1_int16s(i16)               _mm256_set1_epi16 // AVX
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
#define simd_test_all_bitsset_int16s(a)        // annoyingly doesn't exist
*/
#elif defined(__SSE2__) && defined(__SSE4_1__)

#include "immintrin.h"
#define SIMD_INT16_LANES                    8
#define SIMD_INT16                          __m128i
#define simd_load_int16s(int16sptr)         _mm_loadu_si128((const __m128i *)(int16sptr)) // SSE2
#define simd_set1_int16s(i16)               _mm_set1_epi16(i16) // SSE2
#define simd_store_int16s(recip, from)      _mm_storeu_si128((__m128i *)recip, from) // SSE2
#define simd_mul_int16s(a, b)               _mm_mullo_epi16(a, b) // SSE2
#define simd_add_int16s(a, b)               _mm_add_epi16(a, b) // SSE2
#define simd_sub_int16s(a, b)               _mm_sub_epi16(a, b) // SSE2
#define simd_max_int16s(a, b)               _mm_max_epi16(a, b) // SSE2
#define simd_and_int16s(a, b)               _mm_and_si128(a, b)
#define simd_min_int16s(a, b)               _mm_min_epi16(a, b) // SSE2

// cmpgt stands for "Compare Greater". This returns 255 when true, not 1.
#define simd_cmpgt_int16s(a, b)             _mm_cmpgt_epi16(a, b) // SSE2
#define simd_cmplt_int16s(a, b)             _mm_cmplt_epi16(a, b) // SSE2
#define simd_cmpeq_int16s(a, b)             _mm_cmpeq_epi16(a, b) // SSE2
#define simd_testz_int16s(a, b)             _mm_testz_si128(a, b) // SSE4.1
#define simd_test_all_bitsset_int16s(a)     _mm_test_all_ones(a)  // SSE4.1
#else

#define SIMD_INT16_LANES                    1
#define SIMD_INT16                          int16_t
#define simd_load_int16s(int16sptr)         (int16sptr)[0]
#define simd_set1_int16s(i16)               i16
#define simd_store_int16s(recip, from)      (recip)[0] = from
#define simd_cmpgt_int16s(a, b)             (a > b ? ((int16_t)0xFFFF) : 0)
#define simd_cmplt_int16s(a, b)             (a < b ? ((int16_t)0xFFFF) : 0)
#define simd_cmpeq_int16s(a, b)             (a == b ? ((int16_t)0xFFFF) : 0)
#define simd_mul_int16s(a, b)               a * b
#define simd_add_int16s(a, b)               a + b
#define simd_sub_int16s(a, b)               a - b
#define simd_test_all_bitsset_int16s(a)     a == 1
#define simd_max_int16s(a, b)               ((a>b)*a)+((a<=b)*b)
#define simd_and_int16s(a, b)               a & b
#define simd_min_int16s(a, b)               ((a>b)*b)+((a<=b)*a)
#endif

// Float lanes
#if defined(__ARM_NEON)

#include "arm_neon.h"
#define SIMD_FLOAT_LANES                    4
#define SIMD_FLOAT                          float32x4_t
#define simd_load_floats(floatsptr)         vld1q_f32(floatsptr)
#define simd_set1_float(float)              vld1q_dup_f32(&float)
#define simd_store_floats(to_ptr, from)     vst1q_f32(to_ptr, from)
#define simd_mul_floats(a, b)               vmulq_f32(a, b)
#define simd_div_floats(a, b)               vdivq_f32(a, b)
#define simd_add_floats(a, b)               vaddq_f32(a, b)
#define simd_sub_floats(a, b)               vsubq_f32(a, b)
#define simd_max_floats(a, b)               vmaxq_f32(a, b)
// The arm comparison functions (like cmpeq) all return unsigned ints
// we don't use the bitwise and with floats except immediately after passing
// the result of a logical comparison, so this will be OK
// TODO: maybe consider rewriting all logical comparisons to just return 1 or 0
// TODO: and bypass the 255 values alltogether
#define simd_and_floats(a, b)               (float32x4_t)(vandq_u32(a, b))
#define simd_sqrt_floats(a)                 vsqrtq_f32(a)
#define simd_cmpeq_floats(a, b)             vceq_f32(a, b)
#define simd_cmplt_floats(a, b)             vcltq_f32(a, b)

#elif defined(__AVX__)

#include "immintrin.h"
#define SIMD_FLOAT_LANES                    8
#define SIMD_FLOAT                          __m256
#define simd_load_floats(floatsptr)         _mm256_loadu_ps(floatsptr)
#define simd_set1_float(float)              _mm256_set1_ps(float)
#define simd_store_floats(floatsptr, from)  _mm256_storeu_ps(floatsptr, from)
#define simd_mul_floats(a, b)               _mm256_mul_ps(a, b)
#define simd_div_floats(a, b)               _mm256_div_ps(a, b)
#define simd_add_floats(a, b)               _mm256_add_ps(a, b)
#define simd_sub_floats(a, b)               _mm256_sub_ps(a, b)
#define simd_max_floats(a, b)               _mm256_max_ps(a, b)
#define simd_and_floats(a, b)               _mm256_and_ps(a, b)
#define simd_sqrt_floats(a)                 _mm256_sqrt_ps(a)
#define simd_cmpeq_floats(a, b)             _mm256_cmp_ps(a, b, _CMP_EQ_UQ)
#define simd_cmplt_floats(a, b)             _mm256_cmp_ps(a, b, _CMP_LT_OQ)

#elif defined(__SSE__)

#include "immintrin.h"
#define SIMD_FLOAT_LANES                    4
#define SIMD_FLOAT                          __m128
#define simd_load_floats(floatsptr)         _mm_loadu_ps(floatsptr)
#define simd_set1_float(float)              _mm_set1_ps(float)
#define simd_store_floats(floatsptr, from)  _mm_storeu_ps(floatsptr, from)
#define simd_mul_floats(a, b)               _mm_mul_ps(a, b)
#define simd_div_floats(a, b)               _mm_div_ps(a, b)
#define simd_add_floats(a, b)               _mm_add_ps(a, b)
#define simd_sub_floats(a, b)               _mm_sub_ps(a, b)
#define simd_max_floats(a, b)               _mm_max_ps(a, b)
#define simd_and_floats(a, b)               _mm_and_ps(a, b)
#define simd_sqrt_floats(a)                 _mm_sqrt_ps(a)
#define simd_cmpeq_floats(a, b)             _mm_cmp_ps(a, b, _CMP_EQ_UQ)
#define simd_cmplt_floats(a, b)             _mm_cmp_ps(a, b, _CMP_LT_OQ)
#else

#define SIMD_FLOAT_LANES                    1
#define SIMD_FLOAT                          float
#define simd_load_floats(floatsptr)         (floatsptr)[0]
#define simd_set1_float(float)              float
#define simd_store_floats(floatsptr, from)  (floatsptr)[0] = from
#define simd_mul_floats(a, b)               a * b
#define simd_div_floats(a, b)               a / b
#define simd_add_floats(a, b)               a + b
#define simd_sub_floats(a, b)               a - b
#define simd_max_floats(a, b)               tok_fmaxf(a, b)
#define simd_and_floats(a, b)               a & b
#define simd_sqrt_floats(a)                 sqrtf(a)
#define simd_cmpeq_floats(a, b)             a == b
#define simd_cmplt_floats(a, b)             a < b

#endif // Float lanes

// ***********************************
// **    Exactly 4 floats (vec4f)   **
// ***********************************
#if defined(__SSE__)
#include "immintrin.h"
#define SIMD_VEC4F                         __m128
#define simd_load_vec4f(floatsptr)         _mm_loadu_ps(floatsptr)
#define simd_set1_vec4f(float)             _mm_set1_ps(float)
#define simd_set_vec4f(a,b,c,d)            _mm_set_ps(d,c,b,a)
#define simd_store_vec4f(floatsptr, from)  _mm_storeu_ps(floatsptr, from)
#define simd_mul_vec4f(a, b)               _mm_mul_ps(a, b)
#define simd_div_vec4f(a, b)               _mm_div_ps(a, b)
#define simd_add_vec4f(a, b)               _mm_add_ps(a, b)
#define simd_sub_vec4f(a, b)               _mm_sub_ps(a, b)
#define simd_max_vec4f(a, b)               _mm_max_ps(a, b)
#define simd_and_vec4f(a, b)               _mm_and_ps(a, b)
#define simd_sqrt_vec4f(a)                 _mm_sqrt_ps(a)
#define simd_cmpeq_vec4f(a, b)             _mm_cmp_ps(a, b, _CMP_EQ_UQ)
#define simd_cmplt_vec4f(a, b)             _mm_cmp_ps(a, b, _CMP_LT_OQ)
#define simd_extract_vec4f(a, lane)        _mm_cvtss_f32(_mm_shuffle_ps(a, a, _MM_SHUFFLE(0, 0, 0, lane)))
#elif defined(__ARM_NEON)
#include "arm_neon.h"
#define SIMD_VEC4F                         float32x4_t
#define simd_load_vec4f(floatsptr)         vld1q_f32(floatsptr)
#define simd_set1_float(float)             vld1q_dup_f32(&float)
#define simd_store_vec4f(to_ptr, from)     vst1q_f32(to_ptr, from)
#define simd_mul_vec4f(a, b)               vmulq_f32(a, b)
#define simd_div_vec4f(a, b)               vdivq_f32(a, b)
#define simd_add_vec4f(a, b)               vaddq_f32(a, b)
#define simd_sub_vec4f(a, b)               vsubq_f32(a, b)
#define simd_max_vec4f(a, b)               vmaxq_f32(a, b)
// The arm comparison functions (like cmpeq) all return unsigned ints
// we don't use the bitwise and with floats except immediately after passing
// the result of a logical comparison, so this will be OK
// TODO: maybe consider rewriting all logical comparisons to just return 1 or 0
// TODO: and bypass the 255 values alltogether
#define simd_and_vec4f(a, b)               (float32x4_t)(vandq_u32(a, b))
#define simd_sqrt_vec4f(a)                 vsqrtq_f32(a)
#define simd_cmpeq_vec4f(a, b)             vceq_f32(a, b)
#define simd_cmplt_vec4f(a, b)             vcltq_f32(a, b)
#endif

#endif // TOKONE_SIMD_H
