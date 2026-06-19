#ifndef T1_TRIANGLE_H
#define T1_TRIANGLE_H

#include "T1_simd.h"
#include "T1_std.h"

#ifdef __cplusplus
extern "C" {
#endif

void
T1_triangle_cross_vertices(
    f32 * a,
    f32 * b,
    f32 * recip);

SIMD_VEC4F
T1_triangle_normalize_vertex_vec4f(
    SIMD_VEC4F to_normalize_xyz);

void
T1_triangle_normalize_zvertex_f3(
    f32 to_normalize_xyz[3]);

void
T1_triangle_normalize_vertex(
    f32 * to_normalize);

SIMD_VEC4F
T1_triangle_x_rotate_vec4f_known_cossin(
    SIMD_VEC4F xyz,
    f32 cos_x_angle,
    f32 sin_x_angle);

void
T1_triangle_x_rotate_f3_known_cossin(
    f32 * xyz,
    f32 cos_x_angle,
    f32 sin_x_angle);

void
T1_triangle_x_rotate_f3(
    f32 * vertices,
    f32 x_angle);

SIMD_VEC4F
T1_triangle_y_rotate_vec4f_known_cossin(
    SIMD_VEC4F xyz,
    f32 cos_y_angle,
    f32 sin_y_angle);

void
T1_triangle_y_rotate_f3_known_cossin(
    f32 * xyz,
    f32 cos_y_angle,
    f32 sin_y_angle);

void
T1_triangle_y_rotate_f3(
    f32 * xyz,
    f32 y_angle);

SIMD_VEC4F
T1_triangle_z_rotate_vec4f_known_cossin(
    SIMD_VEC4F xyz,
    f32 cos_z_angle,
    f32 sin_z_angle);

void
T1_triangle_z_rotate_f3_known_cossin(
    f32 * xyz,
    f32 cos_z_angle,
    f32 sin_z_angle);

void
T1_triangle_z_rotate_f3(
    f32 * xyz,
    f32 z_angle);

#ifdef __cplusplus
}
#endif

#endif // T1_TRIANGLE_H
