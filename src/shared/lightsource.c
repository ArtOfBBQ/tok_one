#include "lightsource.h"

// The global camera
// In a 2D game, move the x to the left to move all of your
// sprites to the right
zCamera camera = {0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f};

zLightSource zlights_to_apply[ZLIGHTS_TO_APPLY_ARRAYSIZE];
uint32_t zlights_to_apply_size = 0;

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

// move each light so the camera becomes position 0,0,0
void translate_lights(
    const zLightSource * originals,
    zLightSource * out_translated,
    const uint32_t lights_count)
{
    zVertex translated_light_pos;
    for (uint32_t i = 0; i < lights_count; i++)
    {
        translated_light_pos.x =
            originals[i].x - camera.x;
        translated_light_pos.y =
            originals[i].y - camera.y;
        translated_light_pos.z =
            originals[i].z - camera.z;
        translated_light_pos = x_rotate_zvertex(
            &translated_light_pos,
            -1 * camera.x_angle);
        translated_light_pos = y_rotate_zvertex(
            &translated_light_pos,
            -1 * camera.y_angle);
        translated_light_pos = z_rotate_zvertex(
            &translated_light_pos,
            -1 * camera.z_angle);
        
        out_translated[i] = originals[i];
        out_translated[i].x = translated_light_pos.x;
        out_translated[i].y = translated_light_pos.y;
        out_translated[i].z = translated_light_pos.z;
    }
}

