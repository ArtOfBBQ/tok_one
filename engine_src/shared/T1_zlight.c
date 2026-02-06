#include "T1_zlight.h"

T1zLight * T1_zlights = NULL;
uint32_t T1_zlights_size = 0;

static void
T1_zlight_construct(T1zLight * to_construct)
{
    to_construct->xyz[0]        = 0.0f;
    to_construct->xyz[1]        = 0.0f;
    to_construct->xyz[2]        = 0.0f;
    to_construct->xyz_offset[0] = 0.0f;
    to_construct->xyz_offset[1] = 0.0f;
    to_construct->xyz_offset[2] = 0.0f;
    to_construct->RGBA[0]       = 1.0f;
    to_construct->RGBA[1]       = 1.0f;
    to_construct->RGBA[2]       = 1.0f;
    to_construct->RGBA[3]       = 1.0f;
    to_construct->diffuse       = 1.00f;
    to_construct->specular      = 0.50f; // mimics blender's behavior
    to_construct->deleted       = false;
    to_construct->committed     = false;
    
    to_construct->shadow_map_depth_texture_i = -1;
}

T1zLight *
T1_zlight_next(void)
{
    T1zLight * return_value = NULL;
    for (uint32_t i = 0; i < T1_zlights_size; i++) {
        if (T1_zlights[i].deleted) {
            return_value = &T1_zlights[i];
            T1_zlight_construct(return_value);
            return return_value;
        }
    }
    
    log_assert(T1_zlights_size + 1 < T1_ZLIGHTS_CAP);
    return_value = &T1_zlights[T1_zlights_size];
    return_value->committed = false;
    T1_zlights_size += 1;
    
    T1_zlight_construct(return_value);
    
    return return_value;
}

void
T1_zlight_commit(T1zLight * to_request)
{
    log_assert(!to_request->deleted);
    to_request->committed = true;
}

void
T1_zlight_clean_all_deleted(void)
{
    while (
        T1_zlights_size > 0
        && T1_zlights[T1_zlights_size - 1].deleted)
    {
        T1_zlights_size--;
    }
}

void
T1_zlight_copy_all(
    T1GPULight * lights,
    uint32_t * lights_size)
{
    *lights_size = 0;
    for (uint32_t i = 0; i < T1_zlights_size; i++)
    {
        if (!T1_zlights[i].deleted) {
            lights[*lights_size].xyz[0] =
                T1_zlights[i].xyz[0] +
                T1_zlights[i].xyz_offset[0];
            lights[*lights_size].xyz[1] =
                T1_zlights[i].xyz[1] +
                T1_zlights[i].xyz_offset[1];
            lights[*lights_size].xyz[2] =
                T1_zlights[i].xyz[2] +
                T1_zlights[i].xyz_offset[2];
            
            lights[*lights_size].angle_xyz[0] =
                T1_zlights[i].xyz_angle[0];
            lights[*lights_size].angle_xyz[1] =
                T1_zlights[i].xyz_angle[1];
            lights[*lights_size].angle_xyz[2] =
                T1_zlights[i].xyz_angle[2];
            
            lights[*lights_size].diffuse =
                T1_zlights[i].diffuse;
            lights[*lights_size].specular =
                T1_zlights[i].specular;
            lights[*lights_size].reach =
                T1_zlights[i].reach;
            
            lights[*lights_size].rgb[0] =
                T1_zlights[i].RGBA[0];
            lights[*lights_size].rgb[1] =
                T1_zlights[i].RGBA[1];
            lights[*lights_size].rgb[2] =
                T1_zlights[i].RGBA[2];
            
            lights[*lights_size].shadow_map_depth_tex_i =
                T1_zlights[i].shadow_map_depth_texture_i;
            lights[*lights_size].shadow_map_render_view_i =
                T1_zlights[i].shadow_map_render_view_i;
            
            *lights_size += 1;
        }
    }
}

#if 0
void
T1_zlight_translate_all(
    T1GPULight * lights,
    uint32_t * lights_size)
{
    assert(T1_zlights_size < T1_ZLIGHTS_CAP);
    
    float translated_light_pos[3];
    
    for (uint32_t i = 0; i < T1_zlights_size; i++)
    {
        translated_light_pos[0] =
            T1_zlights[i].xyz[0] - T1_camera->xyz[0];
        translated_light_pos[1] =
            T1_zlights[i].xyz[1] - T1_camera->xyz[1];
        translated_light_pos[2] =
            T1_zlights[i].xyz[2] - T1_camera->xyz[2];
        
        x_rotate_zvertex_f3(
            translated_light_pos,
            -T1_camera->xyz_angle[0]);
        y_rotate_zvertex_f3(
            translated_light_pos,
            -T1_camera->xyz_angle[1]);
        z_rotate_zvertex_f3(
            translated_light_pos,
            -T1_camera->xyz_angle[2]);
        
        lights[i].xyz[0] = translated_light_pos[0];
        lights[i].xyz[1] = translated_light_pos[1];
        lights[i].xyz[2] = translated_light_pos[2];
        
        lights[i].rgb[0] = T1_zlights[i].RGBA[0];
        lights[i].rgb[1] = T1_zlights[i].RGBA[1];
        lights[i].rgb[2] = T1_zlights[i].RGBA[2];
        
        lights[i].diffuse = T1_zlights[i].diffuse;
        lights[i].reach = T1_zlights[i].reach;
    }
    *lights_size = T1_zlights_size;
}
#endif

void
T1_zlight_delete(
    const int32_t with_object_id)
{
    for (
        uint32_t i = 0;
        i < T1_zlights_size;
        i++)
    {
        if (
            T1_zlights[i].object_id ==
                with_object_id)
        {
            T1_zlights[i].deleted   = true;
            T1_zlights[i].object_id = -1;
        }
    }
}

/*
All 3 arguments to this function are a pointer to 3 floats
Lights by default point to negative Z, this function points them
to look at point_to_xyz instead

from_pos_xyz is the current position of the light
*/
void
T1_zlight_point_light_to_location(
    float * recipient_xyz_angle,
    const float * from_pos_xyz,
    const float * point_to_xyz)
{
    // Compute direction vector: point_to_xyz - from_pos_xyz
    float dir_x = point_to_xyz[0] - from_pos_xyz[0];
    float dir_y = point_to_xyz[1] - from_pos_xyz[1];
    float dir_z = point_to_xyz[2] - from_pos_xyz[2];
    
    // Compute length of direction vector
    float length = sqrtf(dir_x * dir_x + dir_y * dir_y + dir_z * dir_z);
    
    // Handle edge case: if positions are the same, keep default orientation (0, 0, 0)
    if (length < 1e-6f) {
        recipient_xyz_angle[0] = 0.0f; // X rotation (pitch)
        recipient_xyz_angle[1] = 0.0f; // Y rotation (yaw)
        recipient_xyz_angle[2] = 0.0f; // Z rotation (roll)
        return;
    }
    
    // Normalize direction vector
    float inv_length = 1.0f / length;
    dir_x *= inv_length;
    dir_y *= inv_length;
    dir_z *= inv_length;
    
    // Compute Euler angles for XYZ order to rotate (0, 0, -1) to (dir_x, dir_y, dir_z)
    // Yaw (Y-axis rotation): align in XZ plane
    float yaw = atan2f(dir_x, -dir_z); // atan2(x, -z) for default (0, 0, -1)
    
    // Pitch (X-axis rotation): align Y component
    float xz_length = sqrtf(dir_x * dir_x + dir_z * dir_z);
    float pitch = (xz_length > 1e-6f) ? atan2f(dir_y, xz_length) : (dir_y > 0.0f ? 1.57079632679489661923f : -1.57079632679489661923f);
    
    // Roll (Z-axis rotation): set to 0, as light direction doesn't require roll
    float roll = 0.0f;
    
    // Store angles in recipient_xyz_angle (X, Y, Z order)
    recipient_xyz_angle[0] = pitch; // X rotation (pitch)
    recipient_xyz_angle[1] = yaw;   // Y rotation (yaw)
    recipient_xyz_angle[2] = roll;  // Z rotation (roll)
}

void
T1_zlight_update_all_attached_render_views(void)
{
    for (
        uint32_t zl_i = 0;
        zl_i < T1_zlights_size;
        zl_i++)
    {
        int32_t depth_i = T1_zlights[zl_i].
            shadow_map_depth_texture_i;
        
        if (depth_i < 0) { continue; }
        
        log_assert(depth_i < T1_RENDER_VIEW_CAP);
        
        int32_t rv_i = -1;
        
        for (
            int32_t i = 0;
            i < (int32_t)T1_render_views->size;
            i++)
        {
            if (
                T1_render_views->cpu[i].write_array_i ==
                    DEPTH_TEXTUREARRAYS_I &&
                T1_render_views->cpu[i].write_slice_i ==
                    depth_i)
            {
                rv_i = i;
            }
        }
        
        if (rv_i < 0) {
            continue;
        }
        
        log_assert(rv_i <
            (int32_t)T1_render_views->size);
        log_assert(rv_i < T1_RENDER_VIEW_CAP);
        
        T1_render_views->cpu[rv_i].xyz[0] =
            T1_zlights[zl_i].xyz[0];
        T1_render_views->cpu[rv_i].xyz[1] =
            T1_zlights[zl_i].xyz[1];
        T1_render_views->cpu[rv_i].xyz[2] =
            T1_zlights[zl_i].xyz[2];
        T1_render_views->cpu[rv_i].xyz_angle[0] =
            T1_zlights[zl_i].xyz_angle[0];
        T1_render_views->cpu[rv_i].xyz_angle[1] =
            T1_zlights[zl_i].xyz_angle[1];
        T1_render_views->cpu[rv_i].xyz_angle[2] =
            T1_zlights[zl_i].xyz_angle[2];
    }
}
