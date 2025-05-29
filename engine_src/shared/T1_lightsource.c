#include "T1_lightsource.h"

// The global camera
// In a 2D game, move the x to the left to move all of your
// sprites to the right
GPUCamera camera;

zLightSource * zlights_to_apply = NULL;
uint32_t zlights_to_apply_size = 0;
uint32_t shadowcaster_light_i = 0;

static void construct_zlight(zLightSource * to_construct) {
    to_construct->xyz[0]        = 0.0f;
    to_construct->xyz[1]        = 0.0f;
    to_construct->xyz[2]        = 0.0f;
    to_construct->xyz_offset[0] = 0.0f;
    to_construct->xyz_offset[1] = 0.0f;
    to_construct->xyz_offset[2] = 0.0f;
    to_construct->RGBA[0]       = 0.1f;
    to_construct->RGBA[1]       = 0.1f;
    to_construct->RGBA[2]       = 0.1f;
    to_construct->RGBA[3]       = 1.0f;
    to_construct->ambient       = 1.00f;
    to_construct->diffuse       = 1.00f;
    to_construct->specular      = 1.00f;
    to_construct->deleted       = false;
    to_construct->committed     = false;
}

zLightSource * next_zlight(void) {
    zLightSource * return_value = NULL;
    for (uint32_t i = 0; i < zlights_to_apply_size; i++) {
        if (zlights_to_apply[i].deleted) {
            return_value = &zlights_to_apply[i];
            construct_zlight(return_value);
            return return_value;
        }
    }
    
    log_assert(zlights_to_apply_size + 1 < MAX_LIGHTS_PER_BUFFER);
    return_value = &zlights_to_apply[zlights_to_apply_size];
    return_value->committed = false;
    zlights_to_apply_size += 1;
    
    construct_zlight(return_value);
    
    return return_value;
}

void commit_zlight(zLightSource * to_request)
{
    log_assert(!to_request->deleted);
    to_request->committed = true;
}

void x_rotate_zvertex_f3_known_cossin(
    float inout_xyz[3],
    const float cosf_angle,
    const float sinf_angle)
{
    float y =
        (inout_xyz[1] * cosf_angle) -
        (inout_xyz[2] * sinf_angle);
    
    inout_xyz[2] =
        (inout_xyz[1] * sinf_angle) +
        (inout_xyz[2] * cosf_angle);
    
    inout_xyz[1] = y;
}

void x_rotate_zvertex_f3(
    float inout_xyz[3],
    const float angle)
{
    x_rotate_zvertex_f3_known_cossin(
        /* float input_xyz[3]: */
            inout_xyz,
        /* const float cosf_angle: */
            cosf(angle),
        /* const float sinf_angle: */
            sinf(angle));
    
    return;
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

void y_rotate_zvertex_known_cossin(
    float inout_xyz[3],
    const float cos_angle,
    const float sin_angle)
{
    float x =
        (inout_xyz[0] * cos_angle) +
        (inout_xyz[2] * sin_angle);
    
    inout_xyz[2] =
        (inout_xyz[2] * cos_angle) -
        (inout_xyz[0] * sin_angle);
    
    inout_xyz[0] = x;
    
    return;
}

void y_rotate_zvertex_f3(
    float inout_xyz[3],
    const float angle)
{
    y_rotate_zvertex_known_cossin(
        /* float inout_xyz[3]: */
            inout_xyz,
        /* const float cos_angle: */
            cosf(angle),
        /* const float sin_angle: */
            sinf(angle));
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

void z_rotate_zvertex_f3_known_cossin(
    float inout_xyz[3],
    const float angle_cos,
    const float angle_sin)
{
    float x =
        (inout_xyz[0] * angle_cos) -
        (inout_xyz[1] * angle_sin);
    
    inout_xyz[1] =
        (inout_xyz[1] * angle_cos) +
        (inout_xyz[0] * angle_sin);
    
    inout_xyz[0] = x;
    
    return;
}

void z_rotate_zvertex_f3(
    float inout_xyz[3],
    const float angle)
{
    z_rotate_zvertex_f3_known_cossin(
       /* float inout_xyz[3]: */
           inout_xyz,
       /* const float angle_cos: */
           cosf(angle),
       /* const float angle_sin: */
           sinf(angle));
    
    return;
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
    GPUProjectionConstants * pjc = &engine_globals->projection_constants;
    
    float x_multiplier =
        engine_globals->aspect_ratio * pjc->field_of_view_modifier;
    float y_multiplier = pjc->field_of_view_modifier;
    float z_multiplier = (pjc->zfar / (pjc->zfar - pjc->znear));
    float z_addition = (1.0f * (-pjc->zfar * pjc->znear) /
        (pjc->zfar - pjc->znear));
    
    *position_x *= x_multiplier;
    *position_y *= y_multiplier;
    *position_z *= z_multiplier;
    *position_z += z_addition;
}

void copy_lights(
    GPULight * lights,
    uint32_t * lights_size,
    uint32_t * shadowcaster_i)
{
    *lights_size = 0;
    *shadowcaster_i = shadowcaster_light_i;
    for (uint32_t i = 0; i < zlights_to_apply_size; i++)
    {
        if (!zlights_to_apply[i].deleted) {
            lights[*lights_size].xyz[0] =
                zlights_to_apply[i].xyz[0] +
                zlights_to_apply[i].xyz_offset[0];
            lights[*lights_size].xyz[1] =
                zlights_to_apply[i].xyz[1] +
                zlights_to_apply[i].xyz_offset[1];
            lights[*lights_size].xyz[2] =
                zlights_to_apply[i].xyz[2] +
                zlights_to_apply[i].xyz_offset[2];
            
            lights[*lights_size].angle_xyz[0] =
                zlights_to_apply[i].xyz_angle[0];
            lights[*lights_size].angle_xyz[1] =
                zlights_to_apply[i].xyz_angle[1];
            lights[*lights_size].angle_xyz[2] =
                zlights_to_apply[i].xyz_angle[2];
            
            lights[*lights_size].rgb[0] =
                zlights_to_apply[i].RGBA[0];
            lights[*lights_size].rgb[1] =
                zlights_to_apply[i].RGBA[1];
            lights[*lights_size].rgb[2] =
                zlights_to_apply[i].RGBA[2];
            
            lights[*lights_size].ambient =
                zlights_to_apply[i].ambient;
            lights[*lights_size].diffuse =
                zlights_to_apply[i].diffuse;
            lights[*lights_size].specular =
                zlights_to_apply[i].specular;
            lights[*lights_size].reach =
                zlights_to_apply[i].reach;
            
            *lights_size += 1;
        }
    }
}

// move each light so the camera becomes position 0,0,0
void translate_lights(
    GPULight * lights,
    uint32_t * lights_size)
{
    assert(zlights_to_apply_size < MAX_LIGHTS_PER_BUFFER);
    
    float translated_light_pos[3];
    
    for (uint32_t i = 0; i < zlights_to_apply_size; i++)
    {
        translated_light_pos[0] =
            zlights_to_apply[i].xyz[0] - camera.xyz[0];
        translated_light_pos[1] =
            zlights_to_apply[i].xyz[1] - camera.xyz[1];
        translated_light_pos[2] =
            zlights_to_apply[i].xyz[2] - camera.xyz[2];
        
        x_rotate_zvertex_f3(
            translated_light_pos,
            -camera.xyz_angle[0]);
        y_rotate_zvertex_f3(
            translated_light_pos,
            -camera.xyz_angle[1]);
        z_rotate_zvertex_f3(
            translated_light_pos,
            -camera.xyz_angle[2]);
        
        lights[i].xyz[0] = translated_light_pos[0];
        lights[i].xyz[1] = translated_light_pos[1];
        lights[i].xyz[2] = translated_light_pos[2];
        
        lights[i].rgb[0] = zlights_to_apply[i].RGBA[0];
        lights[i].rgb[1] = zlights_to_apply[i].RGBA[1];
        lights[i].rgb[2] = zlights_to_apply[i].RGBA[2];
        
        lights[i].ambient = zlights_to_apply[i].ambient;
        lights[i].diffuse = zlights_to_apply[i].diffuse;
        lights[i].reach = zlights_to_apply[i].reach;
    }
    *lights_size = zlights_to_apply_size;
}

void delete_zlight(const int32_t with_object_id) {
    for (uint32_t i = 0; i < zlights_to_apply_size; i++) {
        if (zlights_to_apply[i].object_id == with_object_id)
        {
            zlights_to_apply[i].deleted     = true;
            zlights_to_apply[i].object_id   = -1;
        }
    }
}
