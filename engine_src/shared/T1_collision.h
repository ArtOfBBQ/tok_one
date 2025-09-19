#ifndef T1_COLLISION_H
#define T1_COLLISION_H

#include <math.h>

#define T1_COLLISION_SILENCE
#ifndef T1_COLLISION_SILENCE
#include <stdio.h>
#endif

#define T1_COLLISION_IGNORE_ASSERTS
#ifndef T1_COLLISION_IGNORE_ASSERTS
#include <assert.h>
#endif

#ifndef T1_COL_FLT_MAX
#define T1_COL_FLT_MAX 3.402823466e+38F
#endif

int T1_collision_point_hits_AArect(
    const float point[2],
    const float rect_bounds_min[2],
    const float rect_bounds_max[2]);

float T1_collision_ray_hits_AArect(
    const float ray_origin[2],
    const float ray_direction[2],
    const float rect_bounds_min[2],
    const float rect_bounds_max[2],
    float * collision_recipient);

int T1_collision_point_hits_AAbox(
    const float point[3],
    const float rect_bounds_min[3],
    const float rect_bounds_max[3]);

float T1_collision_ray_hits_AAbox(
    const float ray_origin[3],
    const float ray_direction[3],
    const float box_bounds_min[3],
    const float box_bounds_max[3],
    float * collision_recipient);

float T1_collision_normalized_ray_hits_sphere(
    const float ray_origin[3],
    const float normalized_ray_direction[3],
    const float sphere_origin[3],
    const float sphere_radius,
    float * collision_recipient);

int T1_collision_point_hits_triangle(
    const float point_xy[2],
    const float triangle_vertex_1_xy[2],
    const float triangle_vertex_2_xy[2],
    const float triangle_vertex_3_xy[2]);

int T1_collision_point_hits_triangle_3D(
    const float point_xyz[3],
    const float tri_vertex_1_xyz[3],
    const float tri_vertex_2_xyz[3],
    const float tri_vertex_3_xyz[3],
    const float tri_normal_xyz[3]);

float T1_collision_ray_hits_plane(
    const float ray_origin[3],
    const float ray_direction[3],
    const float plane_point[3],
    const float plane_normal[3],
    float * collision_recipient);

float T1_collision_ray_hits_triangle(
    const float ray_origin[3],
    const float ray_direction[3],
    const float triangle_vertex_1[3],
    const float triangle_vertex_2[3],
    const float triangle_vertex_3[3],
    const float triangle_normal[3],
    float * collision_recipient);

#endif // T1_COLLISION_H
