#include "collision.h"

// evaporate printf statements if COLLISION_SILENCE is set
#ifndef COLLISION_SILENCE
#define col_printf(...) printf(__VA_ARGS__)
#else
#define col_printf(...)
#endif

static float dot(const float A[3], const float B[3])
{
    return (A[0]*B[0])+(A[1]*B[1])+(A[2]*B[2]);
}

static void cross(const float A[3], const float B[3], float * recipient)
{
    recipient[0] = (A[1]*B[2])-(A[2]*B[1]);
    recipient[1] = (A[2]*B[0])-(A[0]*B[2]);
    recipient[2] = (A[0]*B[1])-(A[1]*B[0]);
    
    return;
}

int point_hits_AArect(
    const float point[2],
    const float rect_bounds_min[2],
    const float rect_bounds_max[2])
{
    return
        point[0] >= (rect_bounds_min[0] - 0.001f) &&
        point[1] >= (rect_bounds_min[1] - 0.001f) &&
        point[0] <= (rect_bounds_max[0] + 0.001f) &&
        point[1] <= (rect_bounds_max[1] + 0.001f);
}

// returns distance to collision, or FLT_MAX if no hit, or
// a negative float if hit is behind the ray
float ray_hits_AArect(
    const float ray_origin[2],
    const float ray_direction[2],
    const float rect_bounds_min[2],
    const float rect_bounds_max[2],
    float * collision_recipient)
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
    
    FOR EACH BOUNDING PLANE (so in 2D there are 4 - it's the
    left/right/top/bottom of the box extended as planes)
    1. Find the absolute distance along that axis from the ray
      origin to rect (Just simple subtraction)
    2. Next, we need to find the distance if we were traveling
      along the ray
        
      If we're looking for the distance to a y-aligned plane, we
      can just read off the X difference
     
      If we're looking for the distance to an x-aligned plane, we
      can read off the Y difference
      
      Then we can deduce the total distance to plane collision
      E.g. if direction is [2.0f,3.0f] and the distance on the Y
      plane was 9.0f, then we took 9.0f/3.0f = 3.0 steps, and we
      must take 3.0 steps in the X direction also
      
    3. I suppose we now have to move along the ray, and check if
       that point is touching not only the plane but also the
       bounding box itself?
    4. Store that result (distance) and move on to the next
    END FOR
    
    Next, if there are any candidates, we take the smallest
    distance (that was >= 0.0f). That must be the closest collision
    */
    
    col_printf(
        "Box is at: [%f,%f] to [%f, %f]\n",
        rect_bounds_min[0],
        rect_bounds_min[1],
        rect_bounds_max[0],
        rect_bounds_max[1]);
    
    float nearest_dist_found = COL_FLT_MAX;
    
    for (int plane_i = 0; plane_i < 4; plane_i++) {
        int axis_i = plane_i / 2;
        
        const float * bounds = (plane_i % 2 == 1) ?
            rect_bounds_min :
            rect_bounds_max;
        
        float axis_diff =
            bounds[axis_i] - ray_origin[axis_i];
        
        float steps_taken = axis_diff / ray_direction[axis_i];
        col_printf("Plane %i steps taken: %f\n", plane_i,
            steps_taken);
        
        float collision_point[2];
        collision_point[0] = ray_origin[0];
        collision_point[1] = ray_origin[1];
        
        collision_point[0] += (ray_direction[0] * steps_taken);
        collision_point[1] += (ray_direction[1] * steps_taken);
        
        col_printf("Plane %i collision point: [%f, %f]\n", plane_i,
            collision_point[0], collision_point[1]);
        
        if (
            point_hits_AArect(
                collision_point,
                rect_bounds_min,
                rect_bounds_max))
        {
            float dist_ray_to_hit =
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

int point_hits_AAbox(
    const float point[3],
    const float rect_bounds_min[3],
    const float rect_bounds_max[3])
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
// a negative float if hit is behind the ray
float ray_hits_AAbox(
    const float ray_origin[3],
    const float ray_direction[3],
    const float box_bounds_min[3],
    const float box_bounds_max[3],
    float * collision_recipient)
{
    float nearest_dist_found = COL_FLT_MAX;
    
    for (int plane_i = 0; plane_i < 6; plane_i++) {
        int axis_i = plane_i / 2;
        
        const float * bounds = (plane_i % 2 == 1) ?
            box_bounds_min :
            box_bounds_max;
        
        float axis_diff = bounds[axis_i] - ray_origin[axis_i];
        
        float steps_taken = axis_diff / ray_direction[axis_i];
        
        float collision_point[3];
        collision_point[0] = ray_origin[0];
        collision_point[1] = ray_origin[1];
        collision_point[2] = ray_origin[2];
        
        collision_point[0] += (ray_direction[0] * steps_taken);
        collision_point[1] += (ray_direction[1] * steps_taken);
        collision_point[2] += (ray_direction[2] * steps_taken);
        
        if (
            point_hits_AAbox(
                collision_point,
                box_bounds_min,
                box_bounds_max))
        {
            float dist_ray_to_hit =
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

float normalized_ray_hits_sphere(
    const float ray_origin[3],
    const float normalized_ray_direction[3],
    const float sphere_origin[3],
    const float sphere_radius,
    float * collision_recipient)
{
    #ifndef COLLISION_IGNORE_ASSERTS
    assert(dot(normalized_ray_direction, normalized_ray_direction) < 1.02f);
    assert(dot(normalized_ray_direction, normalized_ray_direction) > 0.98f);
    #endif
    
    /*
    points in the ray (rO means ray origin, rD means ray direction, t means
    time or distance traversed):
    rO + (rD * t)
    
    points on the outmost layer of the sphere
    (R means radius of sphere, SO means Sphere Origin):
    R^2 == (xyz - SO)^2
    
    We can save ourselves from learning any math by assuming the Origin is 0,
    because we can start our algorithm by moving both the sphere and the ray
    by an amount that sets the sphere at [0, 0, 0]
    
    rO -= SO
    SO = 0
    
    now:
    R^2 == (xyz - SO)^2
    can be rewritten as:
    R^2 == (xyz - 0)^2
    or:
    R^2 == xyz^2
    
    Next, we assume that the point on the outside of the sphere is also a
    point on the ray. Then it must be true that xyz = rO + (t * rD)
    
    R^2 == (rO + t * rD)^2
    
    the only unknown in this equation is 't'
    
    Next we "simplify" the function with basic algebra rules that we supposedly
    memorized in school. I can never remember any of these rules, so I write a
    'School flashback' every time I need one to temporarily buff myself to the
    level of a 10-year old.
    
    //
    ** School flashback **
    (rO + (rD*t))^2 is like (x + y)^2
    
    which is like (x + y)*(x + y)
    which is like (x * x)+(x * y)+(y * x)+(y * y)
    which is like x^2 + y^2 + 2xy
    ... so (x + y)^2 = x^2 + y^2 + 2xy
    //
    
    R^2 == (rO + t * rD)^2
    
    R^2 == rO^2 + (t * rD)^2 + 2*rO*t*rD
    
    //
    ** School flashback **
    next, note that (rD*t)^2 is like (x * y)^2
    (x * y)^2 =
    (x * y) * (x * y) =
    x * x * y * y =
    x^2 * y^2
    
    so (x * y)^2 = x^2 * y^2
    //
    
    R^2 == rO^2 + (t * rD)^2 + 2*rO*t*rD
    
    R^2 == rO^2 + t^2*rD^2 + 2*rO*t*rD
    
    0 == rD^2*t^2 + 2*rO*rD*t + rO^2 - R^2
         [ ax^2 ] + [   bx  ] + [    c   ]
    
    x = t
    a = rD^2
    b = 2 * rO * rD
    c = rO^2 - R^2
    
    but we know that rD is a normalized normal, so rD^2 = 1
    
    x = t
    a = 1
    b = 2 * rO * rD
    c = rO^2 - R^2
    
    the quadratic formula:
    (-b +- sqrt(b^2 - 4ac)) / 4a
    
    the b^2 - 4ac part is the 'discriminant' that determines whether we're
    supposed to add, subtract, or assume no solutions
    -> discr > 0.0f = 2 solutions, a negative and a positive
    -> discr is exactly 0 = there is only 1 solution (because +0 and -0 same)
    -> discr is below 0 = there is no solution
    
    From the perspective of programming option 1) and 2) are the same and should
    be treated as the same to avoid branching
    */
    
    // This translation allows us to pretend the sphere is at [0, 0, 0]
    float ray_origin_translated[3];
    ray_origin_translated[0] = ray_origin[0] - sphere_origin[0];
    ray_origin_translated[1] = ray_origin[1] - sphere_origin[1];
    ray_origin_translated[2] = ray_origin[2] - sphere_origin[2];
    
    float b = (
        (normalized_ray_direction[0] * ray_origin_translated[0]) +
        (normalized_ray_direction[1] * ray_origin_translated[1]) +
        (normalized_ray_direction[2] * ray_origin_translated[2]))
            * 2.0f;
    
    float c = (
        (ray_origin_translated[0] * ray_origin_translated[0]) +
        (ray_origin_translated[1] * ray_origin_translated[1]) +
        (ray_origin_translated[2] * ray_origin_translated[2])) -
        (sphere_radius * sphere_radius);
    
    // float delta = b^2 - 4ac
    // we know our a is 1, so that can be deleted
    float discr = (b * b) - (4 * c);
    
    float t = COL_FLT_MAX;
    
    if (discr >= 0.0f) {
        // 1 or more collisions exist, we pick the closest one
        
        float result_1 = (-b + sqrtf(discr)) / 2.0f;
        float result_2 = (-b - sqrtf(discr)) / 2.0f;
        
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

int point_hits_triangle(
    const float P[2],
    const float A[2],
    const float B[2],
    const float C[2])
{
    /*
    I copied this solution from this excellent video:
    https://www.youtube.com/watch?v=HYAgJN3x4GA&t=44s
    highly recommended if you want to learn why this works
    */
    
    float w1 =
        (
        (A[0]*(C[1]-A[1])) + ((P[1]-A[1])*(C[0]-A[0])) - (P[0]*(C[1]-A[1]))) /
            (((B[1] - A[1])*(C[0] - A[0])) - ((B[0]-A[0])*(C[1]-A[1])));
    
    float w2 = (P[1]-A[1]-(w1*(B[1]-A[1]))) / (C[1]-A[1]);
    
    return
        w1 >= 0.0f &&
        w2 >= 0.0f &&
        (w1 + w2) <= 1.0f;
}

static float triangle_get_area(
    const float A[3],
    const float B[3],
    const float C[3])
{
    float AB[3];
    AB[0] = B[0] - A[0];
    AB[1] = B[1] - A[1];
    AB[2] = B[2] - A[2];
    
    float AC[3];
    AC[0] = C[0] - A[0];
    AC[1] = C[1] - A[1];
    AC[2] = C[2] - A[2];
    
    float AB_cross_AC[3];
    cross(AB, AC, AB_cross_AC);
    
    return 0.5f * sqrtf(
        (AB_cross_AC[0]*AB_cross_AC[0])+
        (AB_cross_AC[1]*AB_cross_AC[1])+
        (AB_cross_AC[2]*AB_cross_AC[2]));
}

static int coplanar_point_hits_triangle_3D(
    const float P[3],
    const float A[3],
    const float B[3],
    const float C[3])
{
    /*
    We're assuming the point P is already determined to share a plane with
    the triangle ABC
    
    We expect the area of ABP + ACP + BCP to be about equal to the area of
    ABC
    */
    
    float entire_area = triangle_get_area(A, B, C);
    
    float diff = entire_area - (
        triangle_get_area(A, B, P) +
        triangle_get_area(A, C, P) +
        triangle_get_area(B, C, P));
    
    return diff < 0.0001f && diff > -0.0001f;
}

float ray_hits_plane(
    const float ray_origin[3],
    const float ray_direction[3],
    const float plane_point[3],
    const float plane_normal[3],
    float * collision_recipient)
{
    // Assuming vectors are all normalized
    #ifndef COLLISION_IGNORE_ASSERTS
    assert(dot(plane_normal, plane_normal) < 1.05f);
    assert(dot(plane_normal, plane_normal) > -0.01f);
    assert(dot(ray_direction, ray_direction) < 1.05f);
    assert(dot(ray_direction, ray_direction) > -0.01f);
    #endif
    
    /*
    float denom = dot(n, raydir);
    if (denom > 1e-6) {
        Vec3f to_plane_point = plane_point - rayor;
        t = dot(to_plane_point, n) / denom;
        return (t >= 0);
    }
    
    return false;
    */
    
    
    float t;
    
    float denom = dot(plane_normal, ray_direction);
    
    #define THRESHOLD 1e-6
    if (denom > THRESHOLD || denom < -THRESHOLD) {
        float to_plane_point[3];
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
    
    return COL_FLT_MAX;
}

float ray_hits_triangle(
    const float ray_origin[3],
    const float ray_direction[3],
    const float triangle_vertex_1[3],
    const float triangle_vertex_2[3],
    const float triangle_vertex_3[3],
    const float triangle_normal[3],
    float * collision_recipient)
{
    /*
    float D = -(N.x * A.x + N.y * A.y + N.z * A.z);
    
    float t = -(dot(N, orig) + D) / dot(N, dir);
    */
    
    float nearest_dist_found = ray_hits_plane(
        /* const float ray_origin[3]: */
            ray_origin,
        /* const float ray_direction[3]: */
            ray_direction,
        /* const float plane_point[3]: */
            triangle_vertex_1,
        /* const float plane_normal[3]: */
            triangle_normal,
        /* float * collision_recipient: */
            collision_recipient);
    
     if (
         coplanar_point_hits_triangle_3D(
             /* const float P[2]: */
                 collision_recipient,
             /* const float A[2]: */
                 triangle_vertex_1,
             /* const float B[2]: */
                 triangle_vertex_2,
             /* const float C[2]: */
                 triangle_vertex_3))
    {
        #ifndef COLLISION_IGNORE_ASSERTS
        assert(nearest_dist_found > 0.0f);
        #endif
        return nearest_dist_found;
    }
    
    collision_recipient[0] = 0.0f;
    collision_recipient[1] = 0.0f;
    collision_recipient[2] = 0.0f;
    
    return COL_FLT_MAX;
}
