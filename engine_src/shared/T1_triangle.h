#ifndef TRIANGLE_H
#define TRIANGLE_H

#include <math.h>

#include "T1_std.h"
#include "T1_logger.h"

#ifdef __cplusplus
extern "C" {
#endif

#if 0
typedef struct zVertex {
    float x;
    float y;
    float z;
    float uv[2];         // texture coords, ignored if untextured
} zVertex;

typedef struct zTriangle {
    zVertex vertices[3];
    zVertex normal;
    int32_t parent_material_i;
} zTriangle;
#endif

SIMD_VEC4F normalize_vertex_vec4f(SIMD_VEC4F to_normalize_xyz);
void normalize_zvertex_f3(
    float to_normalize_xyz[3]);
void normalize_vertex(
    float * to_normalize);

SIMD_VEC4F x_rotate_vec4f_known_cossin(
    SIMD_VEC4F xyz,
    float cos_x_angle,
    float sin_x_angle);
void x_rotate_f3_known_cossin(
    float * xyz,
    float cos_x_angle,
    float sin_x_angle);
void x_rotate_f3(
    float * vertices,
    float x_angle);
SIMD_VEC4F y_rotate_vec4f_known_cossin(
    SIMD_VEC4F xyz,
    float cos_y_angle,
    float sin_y_angle);
void y_rotate_f3_known_cossin(
    float * xyz,
    float cos_y_angle,
    float sin_y_angle);
void y_rotate_f3(
    float * xyz,
    float y_angle);
SIMD_VEC4F z_rotate_vec4f_known_cossin(
    SIMD_VEC4F xyz,
    float cos_z_angle,
    float sin_z_angle);
void z_rotate_f3_known_cossin(
    float * xyz,
    float cos_z_angle,
    float sin_z_angle);
void z_rotate_f3(
    float * xyz,
    float z_angle);

#ifdef __cplusplus
}
#endif

#endif // TRIANGLE_H

