#ifndef T1_LINAL_H
#define T1_LINAL_H

/*
Linear algebra
- Data is stored row-major
- Vectors are considered to be column vectors
*/
#if defined(__ARM_NEON)
#include <arm_neon.h>
#elif defined(__SSE2__)
#include <immintrin.h>
#endif

#include "T1_stdint.h"

typedef struct {
    union {
        f32 data[4];
        #if defined(__ARM_NEON)
        float32x4_t neon_f4;
        #elif defined(__SSE2__)
        __m128 sse_f4;
        #endif
    };
} T1_linal_f32x4;

f32
T1_linal_f32x4_dot(
    const T1_linal_f32x4 a,
    const T1_linal_f32x4 b);

T1_linal_f32x4
T1_linal_f32x4_cross(
    const T1_linal_f32x4 a,
    const T1_linal_f32x4 b);

typedef struct {
    T1_linal_f32x4 rows[3];
} T1_linal_f32x3x3;

typedef struct {
    T1_linal_f32x4 rows[4];
} T1_linal_f32x4x4;

void
T1_linal_f32x3x3_construct(
    T1_linal_f32x3x3 * to_construct,
    const f32 row1val1,
    const f32 row1val2,
    const f32 row1val3,
    const f32 row2val1,
    const f32 row2val2,
    const f32 row2val3,
    const f32 row3val1,
    const f32 row3val2,
    const f32 row3val3);

void
T1_linal_f32x4x4_construct(
    T1_linal_f32x4x4 * to_construct,
    const f32 row1val1,
    const f32 row1val2,
    const f32 row1val3,
    const f32 row1val4,
    const f32 row2val1,
    const f32 row2val2,
    const f32 row2val3,
    const f32 row2val4,
    const f32 row3val1,
    const f32 row3val2,
    const f32 row3val3,
    const f32 row3val4,
    const f32 row4val1,
    const f32 row4val2,
    const f32 row4val3,
    const f32 row4val4);

void
T1_linal_f32x3x3_construct_from_ptr(
    T1_linal_f32x3x3 * to_construct,
    f32 * vals);

void
T1_linal_f32x4x4_construct_from_ptr(
    T1_linal_f32x4x4 * to_construct,
    f32 * vals);

void
T1_linal_f32x3x3_construct_identity(
    T1_linal_f32x3x3 * to_construct);

void
T1_linal_f32x4x4_construct_identity(
    T1_linal_f32x4x4 * to_construct);

void
T1_linal_f32x4x4_extract_f32x3x3(
    const T1_linal_f32x4x4 * in,
    const s32 omit_row_i,
    const s32 omit_col_i,
    T1_linal_f32x3x3 * out);

f32
T1_linal_f32x3x3_get_determinant(
    const T1_linal_f32x3x3 * a);

f32
T1_linal_f32x4x4_get_determinant(
    const T1_linal_f32x4x4 * a);

#if 0
void T1_linal_f323x3_get_inverse(
    T1_linal_f323x3 * in,
    T1_linal_f323x3 * out);
#endif

void
T1_linal_f32x3x3_inverse_transpose_inplace(
    T1_linal_f32x3x3 * m);

void
T1_linal_f324x4_get_inverse(
    T1_linal_f32x4x4 * in,
    T1_linal_f32x4x4 * out);

void
T1_linal_f32x4x4_inverse_inplace(
    T1_linal_f32x4x4 * m);

void
T1_linal_f32x4x4_inverse_transpose_inplace(
    T1_linal_f32x4x4 * m);

void
T1_linal_f32x4x4_construct_x_rotation(
    T1_linal_f32x4x4 * to_construct,
    const f32 x_angle);

void
T1_linal_f32x4x4_construct_y_rotation(
    T1_linal_f32x4x4 * to_construct,
    const f32 y_angle);

void
T1_linal_f32x4x4_construct_z_rotation(
    T1_linal_f32x4x4 * to_construct,
    const f32 z_angle);

void
T1_linal_f32x4x4_construct_xyz_rotation(
    T1_linal_f32x4x4 * to_construct,
    const f32 x_angle,
    const f32 y_angle,
    const f32 z_angle);

void
T1_linal_f32x3x3_mul_f32x3x3(
    const T1_linal_f32x3x3 * a,
    const T1_linal_f32x3x3 * b,
    T1_linal_f32x3x3 * out);

T1_linal_f32x4
T1_linal_f32x4x4_mul_f32x4(
    const T1_linal_f32x4x4 * m,
    const T1_linal_f32x4 v);

void
T1_linal_f32x4x4_mul_f32x4x4(
    const T1_linal_f32x4x4 * a,
    const T1_linal_f32x4x4 * b,
    T1_linal_f32x4x4 * out);

void
T1_linal_f32x3x3_mul_f32x3x3_inplace(
    T1_linal_f32x3x3 * a,
    const T1_linal_f32x3x3 * b);

void
T1_linal_f32x4x4_mul_f32x4x4_inplace(
    T1_linal_f32x4x4 * a,
    const T1_linal_f32x4x4 * b);

void
T1_linal_f32x3x3_transpose(
    const T1_linal_f32x3x3 * m,
    T1_linal_f32x3x3 * out);

void
T1_linal_f32x4x4_transpose(
    const T1_linal_f32x4x4 * m,
    T1_linal_f32x4x4 * out);

void
T1_linal_f32x3x3_transpose_inplace(
    T1_linal_f32x3x3 * m);

void
T1_linal_f32x4x4_transpose_inplace(
    T1_linal_f32x4x4 * m);

#endif // T1_LINAL_H
