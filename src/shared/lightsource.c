#include "lightsource.h"

// The global camera
// In a 2D game, move the x to the left to move all of your
// sprites to the right
zCamera camera = {0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f};

zLightSource * zlights_to_apply = NULL;
zLightSource * zlights_transformed = NULL;
uint32_t zlights_to_apply_size = 0;
uint32_t zlights_transformed_size = 0;

zVertex x_rotate_zvertex(
    const zVertex * input,
    const float angle)
{
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
    float * vec_to_rotate_y,
    float * vec_to_rotate_z,
    float * working_memory_1,
    float * working_memory_2,
    const uint32_t vec_to_rotate_size,
    const float * cos_angles,
    const float * sin_angles)
{    
    for (uint32_t i = 0; i < vec_to_rotate_size; i++) {
        working_memory_1[i] = vec_to_rotate_y[i];
        working_memory_2[i] = vec_to_rotate_z[i];
    }
    
    platform_256_mul(working_memory_1, cos_angles, vec_to_rotate_size);
    platform_256_mul(working_memory_2, sin_angles, vec_to_rotate_size);
    platform_256_sub(working_memory_1, working_memory_2, vec_to_rotate_size);
    // working_memory_1 now contains the final y values, so leave it untouched
    
    for (uint32_t i = 0; i < vec_to_rotate_size; i++) {
        working_memory_2[i] = vec_to_rotate_y[i];
    }
    
    platform_256_mul(vec_to_rotate_z, cos_angles, vec_to_rotate_size);
    platform_256_mul(working_memory_2, sin_angles, vec_to_rotate_size);
    platform_256_add(vec_to_rotate_z, working_memory_2, vec_to_rotate_size);
    
    for (uint32_t i = 0; i < vec_to_rotate_size; i++) {
        vec_to_rotate_y[i] = working_memory_1[i];
    }
}

zVertex y_rotate_zvertex(
    const zVertex * input,
    const float angle)
{
    zVertex return_value = *input;
    
    return_value.x =
        (input->x * cosf(angle))
        + (input->z * sinf(angle));
    
    return_value.z =
        (input->z * cosf(angle)) -
        (input->x * sinf(angle));
    
    return return_value;
}

void y_rotate_zvertices_inplace(
    float * vec_to_rotate_x,
    float * vec_to_rotate_z,
    float * working_memory_1,
    float * working_memory_2,
    const uint32_t vec_to_rotate_size,
    const float * cos_angles,
    const float * sin_angles)
{        
    for (uint32_t i = 0; i < vec_to_rotate_size; i++) {
        working_memory_1[i] = vec_to_rotate_x[i];
        working_memory_2[i] = vec_to_rotate_z[i];
    }
    
    platform_256_mul(working_memory_1, cos_angles, vec_to_rotate_size);
    platform_256_mul(working_memory_2, sin_angles, vec_to_rotate_size);
    platform_256_add(working_memory_1, working_memory_2, vec_to_rotate_size);
    // working_memory_1 now contains the final x values, so leave it untouched
    
    for (uint32_t i = 0; i < vec_to_rotate_size; i++) {
        working_memory_2[i] = vec_to_rotate_x[i];
    }
    
    platform_256_mul(vec_to_rotate_z, cos_angles, vec_to_rotate_size);
    platform_256_mul(working_memory_2, sin_angles, vec_to_rotate_size);
    platform_256_sub(vec_to_rotate_z, working_memory_2, vec_to_rotate_size);
    
    for (uint32_t i = 0; i < vec_to_rotate_size; i++) {
        vec_to_rotate_x[i] = working_memory_1[i];
    }
}

zVertex z_rotate_zvertex(
    const zVertex * input,
    const float angle)
{
    zVertex return_value = *input;
    
    return_value.x =
        (input->x * cosf(angle))
        - (input->y * sinf(angle));
    
    return_value.y =
        (input->y * cosf(angle)) +
        (input->x * sinf(angle));
    
    return return_value;
}

void z_rotate_zvertices_inplace(
    float * vec_to_rotate_x,
    float * vec_to_rotate_y,
    float * working_memory_1,
    float * working_memory_2,
    const uint32_t vec_to_rotate_size,
    const float * cos_angles,
    const float * sin_angles)
{        
    for (uint32_t i = 0; i < vec_to_rotate_size; i++) {
        working_memory_1[i] = vec_to_rotate_x[i];
        working_memory_2[i] = vec_to_rotate_y[i];
    }
    
    platform_256_mul(working_memory_1, cos_angles, vec_to_rotate_size);
    platform_256_mul(working_memory_2, sin_angles, vec_to_rotate_size);
    platform_256_sub(working_memory_1, working_memory_2, vec_to_rotate_size);
    // working_memory_1 now contains the final x values, so leave it untouched
    
    for (uint32_t i = 0; i < vec_to_rotate_size; i++) {
        working_memory_2[i] = vec_to_rotate_x[i];
    }
    
    platform_256_mul(vec_to_rotate_y, cos_angles, vec_to_rotate_size);
    platform_256_mul(working_memory_2, sin_angles, vec_to_rotate_size);
    platform_256_add(vec_to_rotate_y, working_memory_2, vec_to_rotate_size);
    
    for (uint32_t i = 0; i < vec_to_rotate_size; i++) {
        vec_to_rotate_x[i] = working_memory_1[i];
    }
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

// move each light so the camera becomes position 0,0,0
void translate_lights(void)
{
    assert(zlights_to_apply_size < ZLIGHTS_TO_APPLY_ARRAYSIZE);
    
    zlights_transformed_size = 0;
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
        
        zlights_transformed[zlights_transformed_size]   = zlights_to_apply[i];
        zlights_transformed[zlights_transformed_size].x = translated_light_pos.x;
        zlights_transformed[zlights_transformed_size].y = translated_light_pos.y;
        zlights_transformed[zlights_transformed_size].z = translated_light_pos.z;
        zlights_transformed_size++;
    }
    
    if (
        camera.x == 0.0f &&
        camera.y == 0.0f &&
        camera.z == 0.0f &&
        camera.x_angle == 0.0f &&
        camera.y_angle == 0.0f &&
        camera.z_angle == 0.0f)
    {
        for (uint32_t _ = 0; _ < zlights_transformed_size; _++) {
            log_assert(zlights_transformed[_].x == zlights_to_apply[_].x);
            log_assert(zlights_transformed[_].y == zlights_to_apply[_].y);
            log_assert(zlights_transformed[_].z == zlights_to_apply[_].z);
        }
    }
}
