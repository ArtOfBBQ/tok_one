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

void x_rotate_f3(
    float * xyz,
    float x_angle)
{
    float cos_angle = cosf(x_angle);
    float sin_angle = sinf(x_angle);
    
    float new_y =
        xyz[1] * cos_angle -
        xyz[2] * sin_angle;
    
    xyz[2] =
        xyz[1] * sin_angle +
        xyz[2] * cos_angle;
    
    xyz[1] = new_y;
    
    return;
}

void y_rotate_f3(
    float * xyz,
    float y_angle)
{
    float cos_angle = cosf(y_angle);
    float sin_angle = sinf(y_angle);
    
    float new_x =
        xyz[0] * cos_angle +
        xyz[2] * sin_angle;
    
    xyz[2] =
        xyz[2] * cos_angle -
        xyz[0] * sin_angle;
    
    xyz[0] = new_x;
    
    return;
}

void z_rotate_f3(
    float * xyz,
    float z_angle)
{
    float cos_angle = cosf(z_angle);
    float sin_angle = sinf(z_angle);
    
    float new_x =
        (xyz[0] * cos_angle) -
        (xyz[1] * sin_angle);
    
    xyz[1] =
        (xyz[1] * cos_angle) +
        (xyz[0] * sin_angle);
    
    xyz[0] = new_x;
    
    return;
}
