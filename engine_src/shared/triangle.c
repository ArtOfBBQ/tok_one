#include "triangle.h"

inline static float get_magnitude_f3(float input_xyz[3]) {
    float sum_squares =
        (input_xyz[0] * input_xyz[0]) +
        (input_xyz[1] * input_xyz[1]) +
        (input_xyz[2] * input_xyz[2]);
    
    #ifndef LOGGER_IGNORE_ASSERTS
    sum_squares = isnan(sum_squares) || !isfinite(sum_squares) ?
        FLOAT32_MAX : sum_squares;
    #endif
    
    float return_value = sqrtf(sum_squares);
    
    log_assert(!isnan(return_value));
    
    return return_value;
}

static float get_magnitude(zVertex input) {
    float x = (input.x * input.x);
    float y = (input.y * input.y);
    float z = (input.z * input.z);
    
    float sum_squares = x + y + z;
    
    sum_squares = isnan(sum_squares) || !isfinite(sum_squares) ?
        FLOAT32_MAX : sum_squares;
    
    float return_value = sqrtf(sum_squares);
    
    log_assert(!isnan(return_value));
    
    return return_value;
}

static float get_vertex_magnitude(float * input) {
    float x = (input[0] * input[0]);
    float y = (input[1] * input[1]);
    float z = (input[2] * input[2]);
    
    float sum_squares = x + y + z;
    
    sum_squares = isnan(sum_squares) || !isfinite(sum_squares) ?
        FLOAT32_MAX : sum_squares;
    
    float return_value = sqrtf(sum_squares);
    
    log_assert(!isnan(return_value));
    
    return return_value;
}

void normalize_vertex(
    float * to_normalize)
{
    float magnitude = get_vertex_magnitude(to_normalize);
    if (magnitude < 0.0001f && magnitude > -0.0001f) {
        magnitude = 0.0001f;
    }
    
    log_assert(!isnan(to_normalize[0]));
    to_normalize[0] /= magnitude;
    log_assert(!isnan(to_normalize[0]));
    
    log_assert(!isnan(to_normalize[1]));
    to_normalize[1] /= magnitude;
    log_assert(!isnan(to_normalize[1]));
    
    log_assert(!isnan(to_normalize[2]));
    to_normalize[2] /= magnitude;
    log_assert(!isnan(to_normalize[2]));
}

void normalize_zvertex_f3(
    float to_normalize_xyz[3])
{
    float magnitude = get_magnitude_f3(to_normalize_xyz);
    if (magnitude < 0.0001f && magnitude > -0.0001f) {
        magnitude = 0.0001f;
    }
    
    to_normalize_xyz[0] /= magnitude;
    to_normalize_xyz[1] /= magnitude;
    to_normalize_xyz[2] /= magnitude;
}

void normalize_zvertex(
    zVertex * to_normalize)
{
    float magnitude = get_magnitude(*to_normalize);
    if (magnitude < 0.0001f && magnitude > -0.0001f) {
        magnitude = 0.0001f;
    }
    
    log_assert(!isnan(to_normalize->x));
    to_normalize->x /= magnitude;
    log_assert(!isnan(to_normalize->x));
    
    log_assert(!isnan(to_normalize->y));
    to_normalize->y /= magnitude;
    log_assert(!isnan(to_normalize->y));
    
    log_assert(!isnan(to_normalize->z));
    to_normalize->z /= magnitude;
    log_assert(!isnan(to_normalize->z));
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
    #ifndef LOGGER_IGNORE_ASSERTS
    int32_t largest_start_vertex_i = -1;
    int32_t largest_end_vertex_i = -1;
    #endif
    
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
            #ifndef LOGGER_IGNORE_ASSERTS
            largest_start_vertex_i = start_vertex_i;
            largest_end_vertex_i = end_vertex_i;
            #endif
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
