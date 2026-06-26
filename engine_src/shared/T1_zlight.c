#include "T1_zlight.h"

#include "T1_simd.h"
#include "T1_log.h"
#include "T1_tex.h"
#include "T1_render_view.h"

T1zLight * T1_zlights = NULL;
u32 T1_zlights_size = 0;

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
    for (u32 i = 0; i < T1_zlights_size; i++) {
        if (T1_zlights[i].deleted) {
            return_value = &T1_zlights[i];
            T1_zlight_construct(return_value);
            return return_value;
        }
    }
    
    T1_log_assert(T1_zlights_size + 1 < T1_ZLIGHTS_CAP);
    return_value = &T1_zlights[T1_zlights_size];
    return_value->committed = false;
    T1_zlights_size += 1;
    
    T1_zlight_construct(return_value);
    
    return return_value;
}

void
T1_zlight_commit(T1zLight * to_request)
{
    T1_log_assert(!to_request->deleted);
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
    T1GPULight * l,
    u32 * l_size)
{
    *l_size = 0;
    for (u32 i = 0; i < T1_zlights_size; i++)
    {
        if (!T1_zlights[i].deleted) {
            l[*l_size].xyz[0] = T1_zlights[i].xyz[0] + T1_zlights[i].xyz_offset[0];
            l[*l_size].xyz[1] = T1_zlights[i].xyz[1] + T1_zlights[i].xyz_offset[1];
            l[*l_size].xyz[2] = T1_zlights[i].xyz[2] + T1_zlights[i].xyz_offset[2];
            
            l[*l_size].angle_xyz[0] = T1_zlights[i].xyz_angle[0];
            l[*l_size].angle_xyz[1] = T1_zlights[i].xyz_angle[1];
            l[*l_size].angle_xyz[2] = T1_zlights[i].xyz_angle[2];
            
            l[*l_size].diffuse = T1_zlights[i].diffuse;
            l[*l_size].specular = T1_zlights[i].specular;
            l[*l_size].reach = T1_zlights[i].reach;
            
            l[*l_size].rgb[0] = T1_zlights[i].RGBA[0];
            l[*l_size].rgb[1] = T1_zlights[i].RGBA[1];
            l[*l_size].rgb[2] = T1_zlights[i].RGBA[2];
            
            l[*l_size].shadow_map_depth_tex_i =
                T1_zlights[i].shadow_map_depth_texture_i;
            l[*l_size].shadow_map_render_view_i =
                T1_zlights[i].shadow_map_render_view_i;
            
            *l_size += 1;
        }
    }
}

void
T1_zlight_delete(
    const s32 with_object_id)
{
    for (
        u32 i = 0;
        i < T1_zlights_size;
        i++)
    {
        if (
            T1_zlights[i].T1_id ==
                with_object_id)
        {
            T1_zlights[i].deleted   = true;
            T1_zlights[i].T1_id = -1;
        }
    }
}

/*
All 3 arguments to this function are a pointer to 3 f32s
Lights by default point to negative Z, this function points them
to look at point_to_xyz instead

from_pos_xyz is the current position of the light
*/
void
T1_zlight_point_light_to_location(
    f32 * recipient_xyz_angle,
    const f32 * from_pos_xyz,
    const f32 * point_to_xyz)
{
    // Compute direction vector: point_to_xyz - from_pos_xyz
    f32 dir_x = point_to_xyz[0] - from_pos_xyz[0];
    f32 dir_y = point_to_xyz[1] - from_pos_xyz[1];
    f32 dir_z = point_to_xyz[2] - from_pos_xyz[2];
    
    // Compute length of direction vector
    f32 length = sqrtf(dir_x * dir_x + dir_y * dir_y + dir_z * dir_z);
    
    // Handle edge case: if positions are the same, keep default orientation (0, 0, 0)
    if (length < 1e-6f) {
        recipient_xyz_angle[0] = 0.0f; // X rotation (pitch)
        recipient_xyz_angle[1] = 0.0f; // Y rotation (yaw)
        recipient_xyz_angle[2] = 0.0f; // Z rotation (roll)
        return;
    }
    
    // Normalize direction vector
    f32 inv_length = 1.0f / length;
    dir_x *= inv_length;
    dir_y *= inv_length;
    dir_z *= inv_length;
    
    // Compute Euler angles for XYZ order to rotate (0, 0, -1) to (dir_x, dir_y, dir_z)
    // Yaw (Y-axis rotation): align in XZ plane
    f32 yaw = atan2f(dir_x, -dir_z); // atan2(x, -z) for default (0, 0, -1)
    
    // Pitch (X-axis rotation): align Y component
    f32 xz_length = sqrtf(dir_x * dir_x + dir_z * dir_z);
    f32 pitch = (xz_length > 1e-6f) ? atan2f(dir_y, xz_length) : (dir_y > 0.0f ? 1.57079632679489661923f : -1.57079632679489661923f);
    
    // Roll (Z-axis rotation): set to 0, as light direction doesn't require roll
    f32 roll = 0.0f;
    
    // Store angles in recipient_xyz_angle (X, Y, Z order)
    recipient_xyz_angle[0] = pitch; // X rotation (pitch)
    recipient_xyz_angle[1] = yaw;   // Y rotation (yaw)
    recipient_xyz_angle[2] = roll;  // Z rotation (roll)
}

void
T1_zlight_update_all_attached_render_views(void)
{
    for (
        u32 zl_i = 0;
        zl_i < T1_zlights_size;
        zl_i++)
    {
        s32 depth_i = T1_zlights[zl_i].
            shadow_map_depth_texture_i;
        
        if (depth_i < 0) { continue; }
        
        T1_log_assert(depth_i < T1_RENDER_VIEW_CAP);
        
        s32 rv_i = -1;
        
        for (
            s32 i = 0;
            i < (s32)T1_render_views->size;
            i++)
        {
            if (
                T1_tex_to_array_i(
                    T1_render_views->cpu[i].write_tex) ==
                        T1_DEPTH_TEXTUREARRAYS_I &&
                T1_tex_to_slice_i(
                    T1_render_views->cpu[i].write_tex) ==
                    depth_i)
            {
                rv_i = i;
            }
        }
        
        if (rv_i < 0) {
            continue;
        }
        
        T1_log_assert(rv_i <
            (s32)T1_render_views->size);
        T1_log_assert(rv_i < T1_RENDER_VIEW_CAP);
        
        T1_render_views->cpu[rv_i].xyz[0] =
            T1_zlights[zl_i].xyz[0];
        T1_render_views->cpu[rv_i].xyz[1] =
            T1_zlights[zl_i].xyz[1];
        T1_render_views->cpu[rv_i].xyz[2] =
            T1_zlights[zl_i].xyz[2];
        T1_render_views->cpu[rv_i].angle_xyz[0] =
            T1_zlights[zl_i].xyz_angle[0];
        T1_render_views->cpu[rv_i].angle_xyz[1] =
            T1_zlights[zl_i].xyz_angle[1];
        T1_render_views->cpu[rv_i].angle_xyz[2] =
            T1_zlights[zl_i].xyz_angle[2];
    }
}
