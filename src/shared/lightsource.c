#include "lightsource.h"

// The global camera
// In a 2D game, move the x to the left to move all of your
// sprites to the right
zCamera camera = {0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f};

zLightSource * zlights_to_apply = (zLightSource *)malloc_from_unmanaged(
    sizeof(zLightSource) * ZLIGHTS_TO_APPLY_ARRAYSIZE);
zLightSource * zlights_transformed = (zLightSource *)malloc_from_unmanaged(
    sizeof(zLightSource) * ZLIGHTS_TO_APPLY_ARRAYSIZE);
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

void clean_deleted_lights()
{
    while (
        zlights_to_apply_size > 0
        && zlights_to_apply[zlights_to_apply_size - 1].deleted)
    {
        zlights_to_apply_size--;
    }
}

// move each light so the camera becomes position 0,0,0
void translate_lights()
{
    assert(zlights_to_apply_size < ZLIGHTS_TO_APPLY_ARRAYSIZE);
    
    zlights_transformed_size = 0;
    zVertex translated_light_pos;
    for (uint32_t i = 0; i < zlights_to_apply_size; i++)
    {
        translated_light_pos.x = zlights_to_apply[i].x - camera.x;
        translated_light_pos.y = zlights_to_apply[i].y - camera.y;
        translated_light_pos.z = zlights_to_apply[i].z - camera.z;
        
        if (
            translated_light_pos.x - zlights_to_apply[i].reach >= window_width
            || translated_light_pos.x + zlights_to_apply[i].reach <= 0
            || translated_light_pos.y - zlights_to_apply[i].reach >= window_height
            || translated_light_pos.y + zlights_to_apply[i].reach <= 0)
        {
            continue;
        }
        
        translated_light_pos = x_rotate_zvertex(
            &translated_light_pos,
            -1 * camera.x_angle);
        translated_light_pos = y_rotate_zvertex(
            &translated_light_pos,
            -1 * camera.y_angle);
        translated_light_pos = z_rotate_zvertex(
            &translated_light_pos,
            -1 * camera.z_angle);
        
        zlights_transformed[zlights_transformed_size]   = zlights_to_apply[i];
        zlights_transformed[zlights_transformed_size].x = translated_light_pos.x;
        zlights_transformed[zlights_transformed_size].y = translated_light_pos.y;
        zlights_transformed[zlights_transformed_size].z = translated_light_pos.z;
        zlights_transformed_size++;
    }
}

void update_camera_position() {
    camera.x += camera.next_frame_x_delta;
    camera.y += camera.next_frame_y_delta;
    camera.z += camera.next_frame_z_delta;
    
    camera.next_frame_x_delta = 0.0f;
    camera.next_frame_y_delta = 0.0f;
    camera.next_frame_z_delta = 0.0f;
}
