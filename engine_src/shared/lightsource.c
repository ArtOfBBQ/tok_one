#include "lightsource.h"

// The global camera
// In a 2D game, move the x to the left to move all of your
// sprites to the right
zCamera camera = {0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f};

zLightSource * zlights_to_apply = NULL;
uint32_t zlights_to_apply_size = 0;

void request_zlightsource(zLightSource * to_request)
{
    for (uint32_t i = 0; i < zlights_to_apply_size; i++) {
        if (zlights_to_apply[i].deleted) {
            zlights_to_apply[i] = *to_request;
            return;
        }
    }
    
    log_assert(zlights_to_apply_size + 1 < ZLIGHTS_TO_APPLY_ARRAYSIZE);
    zlights_to_apply[zlights_to_apply_size] = *to_request;
    zlights_to_apply_size += 1;
}

zVertex x_rotate_zvertex(
    const zVertex * input,
    const float angle)
{
    //    float4 rotated_vertices = vertices;
    //    float cos_angle = cos(x_angle);
    //    float sin_angle = sin(x_angle);
    //    
    //    rotated_vertices[1] =
    //        vertices[1] * cos_angle -
    //        vertices[2] * sin_angle;
    //    rotated_vertices[2] =
    //        vertices[1] * sin_angle +
    //        vertices[2] * cos_angle;
    
    zVertex return_value = *input;
    
    return_value.y =
        (input->y * cosf(angle))
        - (input->z * sinf(angle));
    
    return_value.z =
        (input->y * sinf(angle)) +
        (input->z * cosf(angle));
    
    return return_value;
}

void x_rotate_zvertices_inplace(
    SIMD_FLOAT * vec_to_rotate_y,
    SIMD_FLOAT * vec_to_rotate_z,
    const SIMD_FLOAT cos_angles,
    const SIMD_FLOAT sin_angles)
{
    SIMD_FLOAT mul_cosangles = simd_mul_floats(*vec_to_rotate_y, cos_angles);
    SIMD_FLOAT mul_sinangles = simd_mul_floats(*vec_to_rotate_z, sin_angles);  
    SIMD_FLOAT rotated_y = simd_sub_floats(mul_cosangles, mul_sinangles);      
    
    mul_cosangles = simd_mul_floats(*vec_to_rotate_z, cos_angles);
    mul_sinangles = simd_mul_floats(*vec_to_rotate_y, sin_angles);
    *vec_to_rotate_z = simd_add_floats(mul_cosangles, mul_sinangles);
    
    *vec_to_rotate_y = rotated_y;
}

zVertex y_rotate_zvertex(
    const zVertex * input,
    const float angle)
{
    // float4 rotated_vertices = vertices;
    // float cos_angle = cos(y_angle);
    // float sin_angle = sin(y_angle);
    // 
    // rotated_vertices[0] =
    //     vertices[0] * cos_angle +
    //     vertices[2] * sin_angle;
    // rotated_vertices[2] =
    //     vertices[2] * cos_angle -
    //     vertices[0] * sin_angle;
    
    zVertex return_value = *input;
    
    return_value.x =
        (input->x * cosf(angle)) +
        (input->z * sinf(angle));
    
    return_value.z =
        (input->z * cosf(angle)) -
        (input->x * sinf(angle));
    
    return return_value;
}

void y_rotate_zvertices_inplace(
    SIMD_FLOAT * vec_to_rotate_x,
    SIMD_FLOAT * vec_to_rotate_z,
    const SIMD_FLOAT cos_angles,
    const SIMD_FLOAT sin_angles)
{
    SIMD_FLOAT mul_cosangles = simd_mul_floats(*vec_to_rotate_x, cos_angles);
    SIMD_FLOAT mul_sinangles = simd_mul_floats(*vec_to_rotate_z, sin_angles);  
    SIMD_FLOAT rotated_x = simd_add_floats(mul_cosangles, mul_sinangles);      
    
    mul_cosangles = simd_mul_floats(*vec_to_rotate_z, cos_angles);
    mul_sinangles = simd_mul_floats(*vec_to_rotate_x, sin_angles);
    *vec_to_rotate_z = simd_sub_floats(mul_cosangles, mul_sinangles);
    
    *vec_to_rotate_x = rotated_x;
}

zVertex z_rotate_zvertex(
    const zVertex * input,
    const float angle)
{
    //    float4 rotated_vertices = vertices;
    //    float cos_angle = cos(z_angle);
    //    float sin_angle = sin(z_angle);
    //    
    //    rotated_vertices[0] =
    //        vertices[0] * cos_angle -
    //        vertices[1] * sin_angle;
    //    rotated_vertices[1] =
    //        vertices[1] * cos_angle +
    //        vertices[0] * sin_angle;
    
    zVertex return_value = *input;
    
    return_value.x =
        (input->x * cosf(angle)) -
        (input->y * sinf(angle));
    
    return_value.y =
        (input->y * cosf(angle)) +
        (input->x * sinf(angle));
    
    return return_value;
}

void z_rotate_zvertices_inplace(
    SIMD_FLOAT * vec_to_rotate_x,
    SIMD_FLOAT * vec_to_rotate_y,
    const SIMD_FLOAT cos_angles,
    const SIMD_FLOAT sin_angles)
{
    SIMD_FLOAT mul_cosangles = simd_mul_floats(*vec_to_rotate_x, cos_angles);
    SIMD_FLOAT mul_sinangles = simd_mul_floats(*vec_to_rotate_y, sin_angles);  
    SIMD_FLOAT rotated_x = simd_sub_floats(mul_cosangles, mul_sinangles);      
    
    mul_cosangles = simd_mul_floats(*vec_to_rotate_y, cos_angles);
    mul_sinangles = simd_mul_floats(*vec_to_rotate_x, sin_angles);
    *vec_to_rotate_y = simd_add_floats(mul_cosangles, mul_sinangles);
    
    *vec_to_rotate_x = rotated_x;
}

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

void clean_deleted_lights(void)
{
    while (
        zlights_to_apply_size > 0
        && zlights_to_apply[zlights_to_apply_size - 1].deleted)
    {
        zlights_to_apply_size--;
    }
}

void project_float4_to_2d_inplace(
    float * position_x,
    float * position_y,
    float * position_z)
{
    GPU_ProjectionConstants * pjc = &window_globals->projection_constants;
    
    float x_multiplier =
        window_globals->aspect_ratio * pjc->field_of_view_modifier;
    float y_multiplier = pjc->field_of_view_modifier;
    float z_multiplier = (pjc->far / (pjc->far - pjc->near));
    float z_addition = (1.0f * (-pjc->far * pjc->near) /
        (pjc->far - pjc->near));
    
    *position_x *= x_multiplier;
    *position_y *= y_multiplier;
    *position_z *= z_multiplier;
    *position_z += z_addition;
}

void copy_lights(
    GPU_LightCollection * lights_for_gpu)
{
    lights_for_gpu->lights_size = 0;
    for (uint32_t i = 0; i < zlights_to_apply_size; i++)
    {
        if (!zlights_to_apply[i].deleted) {
            lights_for_gpu->light_x[lights_for_gpu->lights_size]   = zlights_to_apply[i].x;
            lights_for_gpu->light_y[lights_for_gpu->lights_size]   = zlights_to_apply[i].y;
            lights_for_gpu->light_z[lights_for_gpu->lights_size]   = zlights_to_apply[i].z;
            lights_for_gpu->red[lights_for_gpu->lights_size]       = zlights_to_apply[i].RGBA[0];
            lights_for_gpu->green[lights_for_gpu->lights_size]     = zlights_to_apply[i].RGBA[1];
            lights_for_gpu->blue[lights_for_gpu->lights_size]      = zlights_to_apply[i].RGBA[2];
            
            lights_for_gpu->ambient[lights_for_gpu->lights_size]   = zlights_to_apply[i].ambient; 
            lights_for_gpu->diffuse[lights_for_gpu->lights_size]   = zlights_to_apply[i].diffuse;
            
            lights_for_gpu->reach[lights_for_gpu->lights_size]     =
                zlights_to_apply[i].reach * zlights_to_apply[i].RGBA[3];
            
            lights_for_gpu->lights_size += 1;
        }
    }
}

// move each light so the camera becomes position 0,0,0
void translate_lights(
    GPU_LightCollection * lights_for_gpu)
{
    assert(zlights_to_apply_size < ZLIGHTS_TO_APPLY_ARRAYSIZE);
    
    zVertex translated_light_pos;
    
    for (uint32_t i = 0; i < zlights_to_apply_size; i++)
    {
        translated_light_pos.x = zlights_to_apply[i].x - camera.x;
        translated_light_pos.y = zlights_to_apply[i].y - camera.y;
        translated_light_pos.z = zlights_to_apply[i].z - camera.z;
        
        translated_light_pos = x_rotate_zvertex(
            &translated_light_pos,
            -camera.x_angle);
        translated_light_pos = y_rotate_zvertex(
            &translated_light_pos,
            -camera.y_angle);
        translated_light_pos = z_rotate_zvertex(
            &translated_light_pos,
            -camera.z_angle);
        
        lights_for_gpu->light_x[i]   = translated_light_pos.x;
        lights_for_gpu->light_y[i]   = translated_light_pos.y;
        lights_for_gpu->light_z[i]   = translated_light_pos.z;
        
        lights_for_gpu->red[i]       = zlights_to_apply[i].RGBA[0];
        lights_for_gpu->green[i]     = zlights_to_apply[i].RGBA[1];
        lights_for_gpu->blue[i]      = zlights_to_apply[i].RGBA[2];
        
        lights_for_gpu->ambient[i]   = zlights_to_apply[i].ambient; 
        lights_for_gpu->diffuse[i]   = zlights_to_apply[i].diffuse;
        
        lights_for_gpu->reach[i]     = zlights_to_apply[i].reach;
        
        lights_for_gpu->lights_size += 1;
    }
}
