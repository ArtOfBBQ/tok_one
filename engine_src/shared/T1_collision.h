#ifndef T1_COLLISION_H
#define T1_COLLISION_H

#include "T1_stdint.h"

int
T1_collision_point_hits_AArect(
    const f32 point[2],
    const f32 rect_bounds_min[2],
    const f32 rect_bounds_max[2]);

f32
T1_collision_ray_hits_AArect(
    const f32 ray_origin[2],
    const f32 ray_direction[2],
    const f32 rect_bounds_min[2],
    const f32 rect_bounds_max[2],
    f32 * collision_recipient);

int
T1_collision_point_hits_AAbox(
    const f32 point[3],
    const f32 rect_bounds_min[3],
    const f32 rect_bounds_max[3]);

f32
T1_collision_ray_hits_AAbox(
    const f32 ray_origin[3],
    const f32 ray_direction[3],
    const f32 box_bounds_min[3],
    const f32 box_bounds_max[3],
    f32 * collision_recipient);

f32
T1_collision_normalized_ray_hits_sphere(
    const f32 ray_origin[3],
    const f32 normalized_ray_direction[3],
    const f32 sphere_origin[3],
    const f32 sphere_radius,
    f32 * collision_recipient);

int
T1_collision_point_hits_triangle(
    const f32 point_xy[2],
    const f32 triangle_vertex_1_xy[2],
    const f32 triangle_vertex_2_xy[2],
    const f32 triangle_vertex_3_xy[2]);

int
T1_collision_point_hits_triangle_3D(
    const f32 point_xyz[3],
    const f32 tri_vertex_1_xyz[3],
    const f32 tri_vertex_2_xyz[3],
    const f32 tri_vertex_3_xyz[3],
    const f32 tri_normal_xyz[3]);

f32
T1_collision_ray_hits_plane(
    const f32 ray_origin[3],
    const f32 ray_direction[3],
    const f32 plane_point[3],
    const f32 plane_normal[3],
    f32 * collision_recipient);

f32
T1_collision_ray_hits_triangle(
    const f32 ray_origin[3],
    const f32 ray_direction[3],
    const f32 triangle_vertex_1[3],
    const f32 triangle_vertex_2[3],
    const f32 triangle_vertex_3[3],
    const f32 triangle_normal[3],
    f32 * collision_recipient);

#endif // T1_COLLISION_H
