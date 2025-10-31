#ifndef T1_LINALG3D_H
#define T1_LINALG3D_H

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
} T1float4;

typedef struct {
    T1float4 rows[4];
} T1float4x4;

void T1_linalg3d_float4x4_construct_from_ptr(
    T1float4x4 * to_construct,
    float * vals);

void T1_linalg3d_construct_identity(
    T1float4x4 * to_construct);

void T1_linalg3d_float4x4_construct(
    T1float4x4 * to_construct,
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

void T1_linalg3d_float4x4_construct_x_rotation(
    T1float4x4 * to_construct,
    const float x_angle);

void T1_linalg3d_float4x4_construct_y_rotation(
    T1float4x4 * to_construct,
    const float y_angle);

void T1_linalg3d_float4x4_construct_z_rotation(
    T1float4x4 * to_construct,
    const float z_angle);

void T1_linalg3d_float4x4_construct_xyz_rotation(
    T1float4x4 * to_construct,
    const float x_angle,
    const float y_angle,
    const float z_angle);

void T1_linalg3d_float4x4_mul_float4x4(
    const T1float4x4 * a,
    const T1float4x4 * b,
    T1float4x4 * out);

#endif // T1_LINALG3D_H

