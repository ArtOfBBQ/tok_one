#ifndef T1_TRIANGLE_H
#define T1_TRIANGLE_H

#include <math.h>

#include "T1_std.h"
#include "T1_logger.h"

#ifdef __cplusplus
extern "C" {
#endif

void
T1_triangle_cross_vertices(
    float * a,
    float * b,
    float * recip);

SIMD_VEC4F
T1_triangle_normalize_vertex_vec4f(
    SIMD_VEC4F to_normalize_xyz);

void
T1_triangle_normalize_zvertex_f3(
    float to_normalize_xyz[3]);

void
T1_triangle_normalize_vertex(
    float * to_normalize);

SIMD_VEC4F
T1_triangle_x_rotate_vec4f_known_cossin(
    SIMD_VEC4F xyz,
    float cos_x_angle,
    float sin_x_angle);

void
T1_triangle_x_rotate_f3_known_cossin(
    float * xyz,
    float cos_x_angle,
    float sin_x_angle);

void
T1_triangle_x_rotate_f3(
    float * vertices,
    float x_angle);

SIMD_VEC4F
T1_triangle_y_rotate_vec4f_known_cossin(
    SIMD_VEC4F xyz,
    float cos_y_angle,
    float sin_y_angle);

void
T1_triangle_y_rotate_f3_known_cossin(
    float * xyz,
    float cos_y_angle,
    float sin_y_angle);

void
T1_triangle_y_rotate_f3(
    float * xyz,
    float y_angle);

SIMD_VEC4F
T1_triangle_z_rotate_vec4f_known_cossin(
    SIMD_VEC4F xyz,
    float cos_z_angle,
    float sin_z_angle);

void
T1_triangle_z_rotate_f3_known_cossin(
    float * xyz,
    float cos_z_angle,
    float sin_z_angle);

void
T1_triangle_z_rotate_f3(
    float * xyz,
    float z_angle);

#ifdef __cplusplus
}
#endif

#endif // T1_TRIANGLE_H
