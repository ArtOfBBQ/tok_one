#include "T1_triangle.h"

static float get_magnitude_vec4f(
    const SIMD_VEC4F input_xyz)
{
    SIMD_VEC4F squared = simd_mul_vec4f(input_xyz, input_xyz);
    
    float sum =
        simd_extract_vec4f(squared, 0) +
        simd_extract_vec4f(squared, 1) +
        simd_extract_vec4f(squared, 2);
    
    return sqrtf(sum);
}

inline
static float
T1_triangle_get_magnitude_f3(float input_xyz[3]) {
    
    float sum_squares =
        (input_xyz[0] * input_xyz[0]) +
        (input_xyz[1] * input_xyz[1]) +
        (input_xyz[2] * input_xyz[2]);
    
    float return_value = sqrtf(sum_squares);
    
    return return_value;
}

static float
T1_triangle_get_vertex_magnitude(
    float * input)
{
    float x = (input[0] * input[0]);
    float y = (input[1] * input[1]);
    float z = (input[2] * input[2]);
    
    float sum_squares = x + y + z;
    
    float return_value = sqrtf(sum_squares);
    
    return return_value;
}

void
T1_triangle_cross_vertices(
    float * a,
    float * b,
    float * recip)
{
    recip[0] = a[1]*b[2]-a[2]*b[1];
    recip[1] = a[2]*b[0]-a[0]*b[2];
    recip[2] = a[0]*b[1]-a[1]*b[0];
}

void
T1_triangle_normalize_vertex(
    float * to_normalize)
{
    float magnitude =
        T1_triangle_get_vertex_magnitude(
            to_normalize);
    if (magnitude < 0.0001f && magnitude > -0.0001f) {
        magnitude = 0.0001f;
    }
    
    to_normalize[0] /= magnitude;
    to_normalize[1] /= magnitude;
    to_normalize[2] /= magnitude;
}

SIMD_VEC4F
T1_triangle_normalize_vertex_vec4f(
    SIMD_VEC4F to_normalize_xyz)
{
    float magnitude = get_magnitude_vec4f(to_normalize_xyz);
    if (magnitude < 0.0001f && magnitude > -0.0001f) {
        magnitude = 0.0001f;
    }
    
    return simd_div_vec4f(to_normalize_xyz, simd_set1_vec4f(magnitude));
}

void
T1_triangle_normalize_zvertex_f3(
    float to_normalize_xyz[3])
{
    float magnitude =
        T1_triangle_get_magnitude_f3(
            to_normalize_xyz);
    if (magnitude < 0.0001f && magnitude > -0.0001f) {
        magnitude = 0.0001f;
    }
    
    to_normalize_xyz[0] /= magnitude;
    to_normalize_xyz[1] /= magnitude;
    to_normalize_xyz[2] /= magnitude;
}

SIMD_VEC4F
T1_triangle_x_rotate_vec4f_known_cossin(
    SIMD_VEC4F xyz,
    float cos_x_angle,
    float sin_x_angle)
{
    /*
    float new_y =
        xyz[1] * cos_x_angle -
        xyz[2] * sin_x_angle;
    
    xyz[2] =
        xyz[2] * cos_x_angle +
        xyz[1] * sin_x_angle;
    
    xyz[1] = new_y;
    */
    
    float new_vals[4];
    new_vals[0] = simd_extract_vec4f(xyz, 0);
    new_vals[1] =
        simd_extract_vec4f(xyz, 1) * cos_x_angle -
        simd_extract_vec4f(xyz, 2) * sin_x_angle;
    new_vals[2] =
        simd_extract_vec4f(xyz, 2) * cos_x_angle +
        simd_extract_vec4f(xyz, 1) * sin_x_angle;
    new_vals[3] = 0.0f;
    
    SIMD_VEC4F return_value = simd_load_vec4f(new_vals);
    
    return return_value;
}

void T1_triangle_x_rotate_f3_known_cossin(
    float * xyz,
    float cos_x_angle,
    float sin_x_angle)
{
    float new_y =
        xyz[1] * cos_x_angle -
        xyz[2] * sin_x_angle;
    
    xyz[2] =
        xyz[2] * cos_x_angle +
        xyz[1] * sin_x_angle;
    
    xyz[1] = new_y;
}

void T1_triangle_x_rotate_f3(
    float * xyz,
    float x_angle)
{
    T1_triangle_x_rotate_f3_known_cossin(
        /* float * xyz: */
            xyz,
        /* float cos_x_angle: */
            cosf(x_angle),
        /* float sin_x_angle: */
            sinf(x_angle));
    
    return;
}

SIMD_VEC4F T1_triangle_y_rotate_vec4f_known_cossin(
    SIMD_VEC4F xyz,
    float cos_y_angle,
    float sin_y_angle)
{
    float new_vals[4];
    new_vals[0] =
        (simd_extract_vec4f(xyz, 0) * cos_y_angle) +
        (simd_extract_vec4f(xyz, 2) * sin_y_angle);
    new_vals[1] = simd_extract_vec4f(xyz, 1);
    new_vals[2] =
        (simd_extract_vec4f(xyz, 2) * cos_y_angle) -
        (simd_extract_vec4f(xyz, 0) * sin_y_angle);;
    new_vals[3] = 0.0f;
    
    SIMD_VEC4F return_value = simd_load_vec4f(new_vals);
    
    return return_value;
}

void T1_triangle_y_rotate_f3_known_cossin(
    float * xyz,
    float cos_y_angle,
    float sin_y_angle)
{
    float new_x =
        xyz[0] * cos_y_angle +
        xyz[2] * sin_y_angle;
    
    xyz[2] =
        xyz[2] * cos_y_angle -
        xyz[0] * sin_y_angle;
    
    xyz[0] = new_x;
}

void T1_triangle_y_rotate_f3(
    float * xyz,
    float y_angle)
{
    T1_triangle_y_rotate_f3_known_cossin(
        /* float * xyz: */
            xyz,
        /* float cos_x_angle: */
            cosf(y_angle),
        /* float sin_x_angle: */
            sinf(y_angle));
    
    return;
}

SIMD_VEC4F T1_triangle_z_rotate_vec4f_known_cossin(
    SIMD_VEC4F xyz,
    float cos_z_angle,
    float sin_z_angle)
{
    float new_vals[4];
    new_vals[0] =
        (simd_extract_vec4f(xyz, 0) * cos_z_angle) -
        (simd_extract_vec4f(xyz, 1) * sin_z_angle);
    new_vals[1] =
        (simd_extract_vec4f(xyz, 1) * cos_z_angle) +
        (simd_extract_vec4f(xyz, 0) * sin_z_angle);;
    new_vals[2] = simd_extract_vec4f(xyz, 2);
    new_vals[3] = 0.0f;
    
    SIMD_VEC4F return_value = simd_load_vec4f(new_vals);
    
    return return_value;
}

void T1_triangle_z_rotate_f3_known_cossin(
    float * xyz,
    float cos_z_angle,
    float sin_z_angle)
{
    float new_x =
        (xyz[0] * cos_z_angle) -
        (xyz[1] * sin_z_angle);
    
    xyz[1] =
        (xyz[1] * cos_z_angle) +
        (xyz[0] * sin_z_angle);
    
    xyz[0] = new_x;
    
    return;
}

void T1_triangle_z_rotate_f3(
    float * xyz,
    float z_angle)
{
    T1_triangle_z_rotate_f3_known_cossin(
        /* float * xyz: */
            xyz,
        /* float cos_x_angle: */
            cosf(z_angle),
        /* float sin_x_angle: */
            sinf(z_angle));
}
