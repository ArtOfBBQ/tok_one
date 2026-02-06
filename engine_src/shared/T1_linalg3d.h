#ifndef T1_LINAL_H
#define T1_LINAL_H

/*
Linear algebra
- Data is stored row-major
- Vectors are considered to be column vectors
*/

#include <assert.h>
#include <stdint.h>
#include <math.h>

#if defined(__ARM_NEON)
#include <arm_neon.h>
#elif defined(__SSE2__)
#include <immintrin.h>
#endif

typedef struct {
    union {
        float data[4];
        #if defined(__ARM_NEON)
        float32x4_t neon_f4;
        #elif defined(__SSE2__)
        __m128 sse_f4;
        #endif
    };
} T1_linal_float4;

float
T1_linal_float4_dot(
    const T1_linal_float4 a,
    const T1_linal_float4 b);

T1_linal_float4
T1_linal_float4_cross(
    const T1_linal_float4 a,
    const T1_linal_float4 b);

typedef struct {
    T1_linal_float4 rows[3];
} T1_linal_float3x3;

typedef struct {
    T1_linal_float4 rows[4];
} T1_linal_float4x4;

void
T1_linal_float3x3_construct(
    T1_linal_float3x3 * to_construct,
    const float row1val1,
    const float row1val2,
    const float row1val3,
    const float row2val1,
    const float row2val2,
    const float row2val3,
    const float row3val1,
    const float row3val2,
    const float row3val3);

void
T1_linal_float4x4_construct(
    T1_linal_float4x4 * to_construct,
    const float row1val1,
    const float row1val2,
    const float row1val3,
    const float row1val4,
    const float row2val1,
    const float row2val2,
    const float row2val3,
    const float row2val4,
    const float row3val1,
    const float row3val2,
    const float row3val3,
    const float row3val4,
    const float row4val1,
    const float row4val2,
    const float row4val3,
    const float row4val4);

void
T1_linal_float3x3_construct_from_ptr(
    T1_linal_float3x3 * to_construct,
    float * vals);

void
T1_linal_float4x4_construct_from_ptr(
    T1_linal_float4x4 * to_construct,
    float * vals);

void
T1_linal_float3x3_construct_identity(
    T1_linal_float3x3 * to_construct);

void
T1_linal_float4x4_construct_identity(
    T1_linal_float4x4 * to_construct);

void
T1_linal_float4x4_extract_float3x3(
    const T1_linal_float4x4 * in,
    const int omit_row_i,
    const int omit_col_i,
    T1_linal_float3x3 * out);

float
T1_linal_float3x3_get_determinant(
    const T1_linal_float3x3 * a);

float
T1_linal_float4x4_get_determinant(
    const T1_linal_float4x4 * a);

#if 0
void T1_linal_float3x3_get_inverse(
    T1_linal_float3x3 * in,
    T1_linal_float3x3 * out);
#endif

void
T1_linal_float3x3_inverse_transpose_inplace(
    T1_linal_float3x3 * m);

void
T1_linal_float4x4_get_inverse(
    T1_linal_float4x4 * in,
    T1_linal_float4x4 * out);

void
T1_linal_float4x4_inverse_inplace(
    T1_linal_float4x4 * m);

void
T1_linal_float4x4_inverse_transpose_inplace(
    T1_linal_float4x4 * m);

void
T1_linal_float4x4_construct_x_rotation(
    T1_linal_float4x4 * to_construct,
    const float x_angle);

void
T1_linal_float4x4_construct_y_rotation(
    T1_linal_float4x4 * to_construct,
    const float y_angle);

void
T1_linal_float4x4_construct_z_rotation(
    T1_linal_float4x4 * to_construct,
    const float z_angle);

void
T1_linal_float4x4_construct_xyz_rotation(
    T1_linal_float4x4 * to_construct,
    const float x_angle,
    const float y_angle,
    const float z_angle);

void
T1_linal_float3x3_mul_float3x3(
    const T1_linal_float3x3 * a,
    const T1_linal_float3x3 * b,
    T1_linal_float3x3 * out);

T1_linal_float4
T1_linal_float4x4_mul_float4(
    const T1_linal_float4x4 * m,
    const T1_linal_float4 v);

void
T1_linal_float4x4_mul_float4x4(
    const T1_linal_float4x4 * a,
    const T1_linal_float4x4 * b,
    T1_linal_float4x4 * out);

void
T1_linal_float3x3_mul_float3x3_inplace(
    T1_linal_float3x3 * a,
    const T1_linal_float3x3 * b);

void
T1_linal_float4x4_mul_float4x4_inplace(
    T1_linal_float4x4 * a,
    const T1_linal_float4x4 * b);

void
T1_linal_float3x3_transpose(
    const T1_linal_float3x3 * m,
    T1_linal_float3x3 * out);

void
T1_linal_float4x4_transpose(
    const T1_linal_float4x4 * m,
    T1_linal_float4x4 * out);

void
T1_linal_float3x3_transpose_inplace(
    T1_linal_float3x3 * m);

void
T1_linal_float4x4_transpose_inplace(
    T1_linal_float4x4 * m);

#endif // T1_LINAL_H
