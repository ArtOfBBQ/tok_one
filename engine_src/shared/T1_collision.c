#include "T1_collision.h"
#include "T1_assert.h"
#include "T1_conditional_includes.h"

#ifndef T1_COL_FLT_MAX
#define T1_COL_FLT_MAX 3.402823466e+38F
#endif

#include <math.h>

#if T1_COLLISION_PRINTF_ACTIVE == T1_ACTIVE
#define col_printf(...) printf(__VA_ARGS__)
#elif T1_COLLISION_PRINTF_ACTIVE == T1_INACTIVE
#define col_printf(...)
#else
#error
#endif

static f32 dot(const f32 A[3], const f32 B[3])
{
    return (A[0]*B[0])+(A[1]*B[1])+(A[2]*B[2]);
}

static void cross(const f32 A[3], const f32 B[3], f32 * recipient)
{
    recipient[0] = (A[1]*B[2])-(A[2]*B[1]);
    recipient[1] = (A[2]*B[0])-(A[0]*B[2]);
    recipient[2] = (A[0]*B[1])-(A[1]*B[0]);
    
    return;
}

s32 T1_collision_point_hits_AArect(
    const f32 point[2],
    const f32 rect_bounds_min[2],
    const f32 rect_bounds_max[2])
{
    return
        point[0] >= (rect_bounds_min[0] - 0.001f) &&
        point[1] >= (rect_bounds_min[1] - 0.001f) &&
        point[0] <= (rect_bounds_max[0] + 0.001f) &&
        point[1] <= (rect_bounds_max[1] + 0.001f);
}

// returns distance to collision, or FLT_MAX if no hit, or
// a negative f32 if hit is behind the ray
f32 T1_collision_ray_hits_AArect(
    const f32 ray_origin[2],
    const f32 ray_direction[2],
    const f32 rect_bounds_min[2],
    const f32 rect_bounds_max[2],
    f32 * collision_recipient)
{
    /*
                y       -----
               5|       |   |
               4|     / |   |
               3|   /   -----
               2| /    4
               1|
    -x ------------------------- x
                |1 2 3 4 5 6 7
                |
                |
                |
                |
               -y
    
    For each of the box's 4 edges, extended into a line (a plane in 3D):
    1. the distance from the ray origin to it, along its axis
    2. divided by the ray's direction along that axis: the steps along the
       ray to reach it, e.g. direction [2, 3] reaches y + 9 in 3 steps
    3. that point on the ray must also be on the box itself
    4. keep that distance
    The smallest distance >= 0 is the closest collision.
    */
    
    col_printf(
        "Box is at: [%f,%f] to [%f, %f]\n",
        rect_bounds_min[0],
        rect_bounds_min[1],
        rect_bounds_max[0],
        rect_bounds_max[1]);
    
    f32 nearest_dist_found = T1_COL_FLT_MAX;
    
    for (s32 plane_i = 0; plane_i < 4; plane_i++) {
        s32 axis_i = plane_i / 2;
        
        const f32 * bounds = (plane_i % 2 == 1) ?
            rect_bounds_min :
            rect_bounds_max;
        
        f32 axis_diff =
            bounds[axis_i] - ray_origin[axis_i];
        
        f32 steps_taken = axis_diff / ray_direction[axis_i];
        col_printf("Plane %i steps taken: %f\n", plane_i,
            steps_taken);
        
        f32 collision_point[2];
        collision_point[0] = ray_origin[0];
        collision_point[1] = ray_origin[1];
        
        collision_point[0] += (ray_direction[0] * steps_taken);
        collision_point[1] += (ray_direction[1] * steps_taken);
        
        col_printf("Plane %i collision point: [%f, %f]\n", plane_i,
            collision_point[0], collision_point[1]);
        
        if (
            T1_collision_point_hits_AArect(
                collision_point,
                rect_bounds_min,
                rect_bounds_max))
        {
            f32 dist_ray_to_hit =
                (
                    (ray_origin[0] - collision_point[0]) *
                    (ray_origin[0] - collision_point[0])) +
                (
                    (ray_origin[1] - collision_point[1]) *
                    (ray_origin[1] - collision_point[1]));
            
            if (dist_ray_to_hit <= nearest_dist_found) {
                nearest_dist_found = dist_ray_to_hit;
                collision_recipient[0] = collision_point[0];
                collision_recipient[1] = collision_point[1];
            }
            
            col_printf(
                "Plane %i collided after: %f\n",
                plane_i,
                dist_ray_to_hit);
        } else {
            col_printf(
                "No collision found for plane %i\n",
                plane_i);
        }
    }
    
    return sqrtf(nearest_dist_found);
}

s32 T1_collision_point_hits_AAbox(
    const f32 point[3],
    const f32 rect_bounds_min[3],
    const f32 rect_bounds_max[3])
{
    return
        point[0] >= (rect_bounds_min[0] - 0.001f) &&
        point[1] >= (rect_bounds_min[1] - 0.001f) &&
        point[2] >= (rect_bounds_min[2] - 0.001f) &&
        point[0] <= (rect_bounds_max[0] + 0.001f) &&
        point[1] <= (rect_bounds_max[1] + 0.001f) &&
        point[2] <= (rect_bounds_max[2] - 0.001f);
}

// returns distance to collision, or FLT_MAX if no hit, or
// a negative f32 if hit is behind the ray
f32 T1_collision_ray_hits_AAbox(
    const f32 ray_origin[3],
    const f32 ray_direction[3],
    const f32 box_bounds_min[3],
    const f32 box_bounds_max[3],
    f32 * collision_recipient)
{
    f32 nearest_dist_found = T1_COL_FLT_MAX;
    
    for (s32 plane_i = 0; plane_i < 6; plane_i++) {
        s32 axis_i = plane_i / 2;
        
        const f32 * bounds = (plane_i % 2 == 1) ?
            box_bounds_min :
            box_bounds_max;
        
        f32 axis_diff = bounds[axis_i] - ray_origin[axis_i];
        
        f32 steps_taken = axis_diff / ray_direction[axis_i];
        
        f32 collision_point[3];
        collision_point[0] = ray_origin[0];
        collision_point[1] = ray_origin[1];
        collision_point[2] = ray_origin[2];
        
        collision_point[0] += (ray_direction[0] * steps_taken);
        collision_point[1] += (ray_direction[1] * steps_taken);
        collision_point[2] += (ray_direction[2] * steps_taken);
        
        if (
            T1_collision_point_hits_AAbox(
                collision_point,
                box_bounds_min,
                box_bounds_max))
        {
            f32 dist_ray_to_hit =
                (
                    (ray_origin[0] - collision_point[0]) *
                    (ray_origin[0] - collision_point[0])) +
                (
                    (ray_origin[1] - collision_point[1]) *
                    (ray_origin[1] - collision_point[1])) +
                (
                    (ray_origin[2] - collision_point[2]) *
                    (ray_origin[2] - collision_point[2]));
            
            if (dist_ray_to_hit <= nearest_dist_found) {
                nearest_dist_found = dist_ray_to_hit;
                collision_recipient[0] = collision_point[0];
                collision_recipient[1] = collision_point[1];
                collision_recipient[2] = collision_point[2];
            }
        }
    }
    
    return sqrtf(nearest_dist_found);
}

f32 T1_collision_normalized_ray_hits_sphere(
    const f32 ray_origin[3],
    const f32 normalized_ray_direction[3],
    const f32 sphere_origin[3],
    const f32 sphere_radius,
    f32 * collision_recipient)
{
    #if T1_ASSERTS_ACTIVE == T1_ACTIVE
    T1_assert(dot(normalized_ray_direction, normalized_ray_direction) < 1.02f);
    T1_assert(dot(normalized_ray_direction, normalized_ray_direction) > 0.98f);
    #elif T1_ASSERTS_ACTIVE == T1_INACTIVE
    #else
    #error
    #endif
    
    /*
    Ray: rO + t * rD, sphere: |xyz - SO|^2 = R^2. Moving both so the sphere
    is at the origin (rO -= SO) and putting the ray into the sphere:
        (rO + t * rD)^2 = R^2
        rD^2 * t^2 + 2 * (rO . rD) * t + rO^2 - R^2 = 0
    a quadratic in t with a = rD^2 = 1 (rD is normalized), b = 2 * rO . rD
    and c = rO^2 - R^2. A discriminant b^2 - 4ac below 0 is a miss, else
    t = (-b +- sqrt(b^2 - 4ac)) / 2a. One solution (a discriminant of 0) is
    handled like two, to avoid branching.
    */
    
    // This translation allows us to pretend the sphere is at [0, 0, 0]
    f32 ray_origin_translated[3];
    ray_origin_translated[0] = ray_origin[0] - sphere_origin[0];
    ray_origin_translated[1] = ray_origin[1] - sphere_origin[1];
    ray_origin_translated[2] = ray_origin[2] - sphere_origin[2];
    
    f32 b = (
        (normalized_ray_direction[0] * ray_origin_translated[0]) +
        (normalized_ray_direction[1] * ray_origin_translated[1]) +
        (normalized_ray_direction[2] * ray_origin_translated[2]))
            * 2.0f;
    
    f32 c = (
        (ray_origin_translated[0] * ray_origin_translated[0]) +
        (ray_origin_translated[1] * ray_origin_translated[1]) +
        (ray_origin_translated[2] * ray_origin_translated[2])) -
        (sphere_radius * sphere_radius);
    
    // f32 delta = b^2 - 4ac
    // we know our a is 1, so that can be deleted
    f32 discr = (b * b) - (4 * c);
    
    f32 t = T1_COL_FLT_MAX;
    
    if (discr >= 0.0f) {
        // 1 or more collisions exist, we pick the closest one
        
        f32 result_1 = (-b + sqrtf(discr)) / 2.0f;
        f32 result_2 = (-b - sqrtf(discr)) / 2.0f;
        
        t =
           ((result_1 <  t) * result_1) +
           ((result_1 >= t) * t);
        t =
           ((result_2 <  t) * result_2) +
           ((result_2 >= t) * t);
    }
    
    collision_recipient[0] =
        ray_origin[0] + (normalized_ray_direction[0] * t);
    collision_recipient[1] =
        ray_origin[1] + (normalized_ray_direction[1] * t);
    collision_recipient[2] =
        ray_origin[2] + (normalized_ray_direction[2] * t);
    
    return t;
}

s32 T1_collision_point_hits_triangle(
    const f32 P[2],
    const f32 A[2],
    const f32 B[2],
    const f32 C[2])
{
    // barycentric coordinates, see https://www.youtube.com/watch?v=HYAgJN3x4GA
    
    f32 w1 =
        (
        (A[0]*(C[1]-A[1])) + ((P[1]-A[1])*(C[0]-A[0])) - (P[0]*(C[1]-A[1]))) /
            (((B[1] - A[1])*(C[0] - A[0])) - ((B[0]-A[0])*(C[1]-A[1])));
    
    f32 w2 = (P[1]-A[1]-(w1*(B[1]-A[1]))) / (C[1]-A[1]);
    
    return
        w1 >= 0.0f &&
        w2 >= 0.0f &&
        (w1 + w2) <= 1.0f;
}

static f32 triangle_get_area(
    const f32 A[3],
    const f32 B[3],
    const f32 C[3])
{
    f32 AB[3];
    AB[0] = B[0] - A[0];
    AB[1] = B[1] - A[1];
    AB[2] = B[2] - A[2];
    
    f32 AC[3];
    AC[0] = C[0] - A[0];
    AC[1] = C[1] - A[1];
    AC[2] = C[2] - A[2];
    
    f32 AB_cross_AC[3];
    cross(AB, AC, AB_cross_AC);
    
    return 0.5f * sqrtf(
        (AB_cross_AC[0]*AB_cross_AC[0])+
        (AB_cross_AC[1]*AB_cross_AC[1])+
        (AB_cross_AC[2]*AB_cross_AC[2]));
}

static s32 coplanar_point_hits_triangle_3D(
    const f32 P[3],
    const f32 A[3],
    const f32 B[3],
    const f32 C[3])
{
    /*
    We're assuming the point P is already determined to share a plane with
    the triangle ABC
    
    We expect the area of ABP + ACP + BCP to be about equal to the area of
    ABC
    */
    
    f32 entire_area = triangle_get_area(A, B, C);
    
    f32 diff = entire_area - (
        triangle_get_area(A, B, P) +
        triangle_get_area(A, C, P) +
        triangle_get_area(B, C, P));
    
    f32 threshold = entire_area * 0.00001f;
    
    return
        diff < threshold &&
        diff > -threshold;
}

f32 T1_collision_ray_hits_plane(
    const f32 ray_origin[3],
    const f32 ray_direction[3],
    const f32 plane_point[3],
    const f32 plane_normal[3],
    f32 * collision_recipient)
{
    // Assuming vectors are all normalized
    #if T1_ASSERTS_ACTIVE == T1_ACTIVE
    T1_assert(dot(plane_normal, plane_normal) < 1.05f);
    T1_assert(dot(plane_normal, plane_normal) > -0.01f);
    T1_assert(dot(ray_direction, ray_direction) < 1.05f);
    T1_assert(dot(ray_direction, ray_direction) > -0.01f);
    #elif T1_ASSERTS_ACTIVE == T1_INACTIVE
    #else
    #error
    #endif
    
    /*
    f32 denom = dot(n, raydir);
    if (denom > 1e-6) {
        Vec3f to_plane_point = plane_point - rayor;
        t = dot(to_plane_point, n) / denom;
        return (t >= 0);
    }
    
    return false;
    */
    
    
    f32 t;
    
    f32 denom = dot(plane_normal, ray_direction);
    
    #define THRESHOLD 1e-6
    if (denom > THRESHOLD || denom < -THRESHOLD) {
        f32 to_plane_point[3];
        to_plane_point[0] = plane_point[0] - ray_origin[0];
        to_plane_point[1] = plane_point[1] - ray_origin[1];
        to_plane_point[2] = plane_point[2] - ray_origin[2];
        
        t = dot(to_plane_point, plane_normal) / denom;
        if (t >= 0.0f) {
            collision_recipient[0] = ray_origin[0] + (ray_direction[0] * t);
            collision_recipient[1] = ray_origin[1] + (ray_direction[1] * t);
            collision_recipient[2] = ray_origin[2] + (ray_direction[2] * t);
            return t;
        }
    }
    
    return T1_COL_FLT_MAX;
}

f32 T1_collision_ray_hits_triangle(
    const f32 ray_origin[3],
    const f32 ray_direction[3],
    const f32 triangle_vertex_1[3],
    const f32 triangle_vertex_2[3],
    const f32 triangle_vertex_3[3],
    const f32 triangle_normal[3],
    f32 * collision_recipient)
{
    /*
    f32 D = -(N.x * A.x + N.y * A.y + N.z * A.z);
    
    f32 t = -(dot(N, orig) + D) / dot(N, dir);
    */
    
    f32 nearest_dist_found = T1_collision_ray_hits_plane(
        /* const f32 ray_origin[3]: */
            ray_origin,
        /* const f32 ray_direction[3]: */
            ray_direction,
        /* const f32 plane_point[3]: */
            triangle_vertex_1,
        /* const f32 plane_normal[3]: */
            triangle_normal,
        /* f32 * collision_recipient: */
            collision_recipient);
    
     if (
         coplanar_point_hits_triangle_3D(
             /* const f32 P[2]: */
                 collision_recipient,
             /* const f32 A[2]: */
                 triangle_vertex_1,
             /* const f32 B[2]: */
                 triangle_vertex_2,
             /* const f32 C[2]: */
                 triangle_vertex_3))
    {
        #if T1_ASSERTS_ACTIVE == T1_ACTIVE
        T1_assert(nearest_dist_found > 0.0f);
        #elif T1_ASSERTS_ACTIVE == T1_INACTIVE
        #else
        #error
        #endif
        return nearest_dist_found;
    }
    
    collision_recipient[0] = 0.0f;
    collision_recipient[1] = 0.0f;
    collision_recipient[2] = 0.0f;
    
    return T1_COL_FLT_MAX;
}
