#include "triangle.h"

static float get_magnitude(zVertex input) {
    float sum_squares =
        (input.x * input.x) +
        (input.y * input.y) +
        (input.z * input.z);
    
    // TODO: this square root is a performance bottleneck
    return sqrtf(sum_squares);
}

void normalize_zvertex(
    zVertex * to_normalize)
{
    float magnitude = get_magnitude(*to_normalize);
    to_normalize->x /= magnitude;
    to_normalize->y /= magnitude;
    to_normalize->z /= magnitude;
}

zVertex crossproduct_of_zvertices(
    const zVertex * a,
    const zVertex * b)
{
    /*
    cx = aybz − azby
    cy = azbx − axbz
    cz = axby − aybx
    */
    zVertex result;
    
    result.x = (a->y * b->z) - (a->z * b->y);
    result.y = (a->z * b->x) - (a->x * b->z);
    result.z = (a->x * b->y) - (a->y * b->x);
    
    return result;
}

float get_squared_distance(
    const zVertex a,
    const zVertex b)
{
    return
        ((a.x - b.x) * (a.x - b.x)) +
        ((a.y - b.y) * (a.y - b.y)) +
        ((a.z - b.z) * (a.z - b.z));
}

/* the largest length amongst any dimension be it x, y, or z */
float get_squared_triangle_length(
    const zTriangle * subject)
{
    float largest_squared_dist = FLOAT32_MIN;
    int32_t largest_start_vertex_i = -1;
    int32_t largest_end_vertex_i = -1;
    
    for (int32_t start_vertex_i = 0; start_vertex_i < 3; start_vertex_i++) {
        
        int32_t end_vertex_i = (start_vertex_i + 1) % 3;
        
        float squared_x =
            ((subject->vertices[start_vertex_i].x -
                subject->vertices[end_vertex_i].x) *
            (subject->vertices[start_vertex_i].x -
                subject->vertices[end_vertex_i].x));
        float squared_y =
            ((subject->vertices[start_vertex_i].y -
                subject->vertices[end_vertex_i].y) *
            (subject->vertices[start_vertex_i].y -
                subject->vertices[end_vertex_i].y));
        float squared_z =
            ((subject->vertices[start_vertex_i].z -
                subject->vertices[end_vertex_i].z) *
            (subject->vertices[start_vertex_i].z -
                subject->vertices[end_vertex_i].z));
        
        float new_squared_dist =
            squared_x +
            squared_y +
            squared_z;
        
        log_assert(new_squared_dist > 0.0f);
        
        if (new_squared_dist > largest_squared_dist) {
            largest_squared_dist = new_squared_dist;
            largest_start_vertex_i = start_vertex_i;
            largest_end_vertex_i = end_vertex_i;
            log_assert(largest_start_vertex_i != largest_end_vertex_i);
        }
    }
    
    log_assert(largest_start_vertex_i != largest_end_vertex_i);
    
    return largest_squared_dist;
}

float get_triangle_area(
    const zTriangle * subject)
{
    zVertex zero_to_one;
    zero_to_one.x = subject->vertices[1].x - subject->vertices[0].x;
    zero_to_one.y = subject->vertices[1].y - subject->vertices[0].y;
    zero_to_one.z = subject->vertices[1].z - subject->vertices[0].z;
    
    zVertex zero_to_two;
    zero_to_two.x = subject->vertices[2].x - subject->vertices[0].x;
    zero_to_two.y = subject->vertices[2].y - subject->vertices[0].y;
    zero_to_two.z = subject->vertices[2].z - subject->vertices[0].z;
    
    zVertex crossproduct = crossproduct_of_zvertices(
        &zero_to_one,
        &zero_to_two);
    
    float return_value = get_magnitude(crossproduct) * 0.5f;
    
    log_assert(return_value > 0.0f);
    
    return return_value;
}
