#include "T1_std.h"
#include "T1_mem.h"
#include "T1_linalg3d.h"
#include "T1_global.h"
#include "T1_log.h"
#include "T1_zlight.h"
#include "T1_render_view.h"
#include "T1_os.h"
#include "T1_id.h"
#include "T1_settings.h"

/*
Cameras: a viewpoint with a size that draw passes draw from (see
T1_pass.h). Every camera has its own depth buffer, at its size.
*/
T1RenderViewCollection * T1_render_views = NULL;

void T1_render_view_init(void) {
    T1_log_assert(T1_RENDER_VIEW_CAP > 0);
    T1_render_views = T1_mem_malloc_unmanaged(
        sizeof(T1RenderViewCollection));
    
    T1_std_memset(
        T1_render_views,
        0,
        sizeof(T1RenderViewCollection));
    
    T1_render_view_reset(0);
    
    for (u32 i = 0; i < T1_RENDER_VIEW_CAP; i++) {
        T1_render_views->cpu[i].clamped_to_T1_id = T1_ID_NONE;
        T1_render_views->cpu[i].reflect_cam = T1_ID_NONE;
    }
}

typedef struct {
    f32    last_drag_start_camera_x;
    f32    last_drag_start_camera_y;
    f32    last_drag_start_x;
    f32    last_drag_start_y;
    f32    total_dragged_x;
    f32    total_dragged_y;
} T1CamState;

void T1_render_view_reset(s32 i)
{
    T1CPURenderView * rv = T1_render_views->cpu + i;
    
    T1_log_assert(!isnan(rv->f32s.dest_xyz[0]));
    T1_log_assert(!isnan(rv->f32s.dest_xyz[1]));
    T1_log_assert(!isnan(rv->f32s.dest_xyz[2]));
    T1_log_assert(!isnan(rv->f32s.dest_angle_xyz[0]));
    T1_log_assert(!isnan(rv->f32s.dest_angle_xyz[1]));
    T1_log_assert(!isnan(rv->f32s.dest_angle_xyz[2]));
    
    rv->us_to_destination = 0;
    rv->f32s.xyz[0] = 0.0f;
    rv->f32s.xyz[1] = 0.0f;
    rv->f32s.xyz[2] = 0.0f;
    rv->f32s.angle_xyz[0] = 0.0f;
    rv->f32s.angle_xyz[1] = 0.0f;
    rv->f32s.angle_xyz[2] = 0.0f;
    rv->f32s.dest_xyz[0] = 0.0f;
    rv->f32s.dest_xyz[1] = 0.0f;
    rv->f32s.dest_xyz[2] = 0.0f;
    rv->f32s.dest_angle_xyz[0] = 0.0f;
    rv->f32s.dest_angle_xyz[1] = 0.0f;
    rv->f32s.dest_angle_xyz[2] = 0.0f;
    rv->f32s.min_xyz[0] = T1_F32_MIN;
    rv->f32s.min_xyz[1] = T1_F32_MIN;
    rv->f32s.min_xyz[2] = T1_F32_MIN;
    rv->f32s.max_xyz[0] = T1_F32_MAX;
    rv->f32s.max_xyz[1] = T1_F32_MAX;
    rv->f32s.max_xyz[2] = T1_F32_MAX;
    
    rv->f32s.min_angle_xyz[0] = T1_F32_MIN;
    rv->f32s.min_angle_xyz[1] = T1_F32_MIN;
    rv->f32s.min_angle_xyz[2] = T1_F32_MIN;
    rv->f32s.max_angle_xyz[0] = T1_F32_MAX;
    rv->f32s.max_angle_xyz[1] = T1_F32_MAX;
    rv->f32s.max_angle_xyz[2] = T1_F32_MAX;
    
    T1_log_assert(!isnan(rv->f32s.dest_xyz[0]));
    T1_log_assert(!isnan(rv->f32s.dest_xyz[1]));
    T1_log_assert(!isnan(rv->f32s.dest_xyz[2]));
    T1_log_assert(!isnan(rv->f32s.dest_angle_xyz[0]));
    T1_log_assert(!isnan(rv->f32s.dest_angle_xyz[1]));
    T1_log_assert(!isnan(rv->f32s.dest_angle_xyz[2]));
}

static void T1_render_view_construct(
    T1CPURenderView * cpu,
    T1GPURenderView * gpu,
    const u32 height,
    const u32 width)
{
    T1_log_assert(!isnan(cpu->f32s.dest_xyz[0]));
    T1_log_assert(!isnan(cpu->f32s.dest_xyz[1]));
    T1_log_assert(!isnan(cpu->f32s.dest_xyz[2]));
    
    T1_log_assert(!isnan(cpu->f32s.dest_angle_xyz[0]));
    T1_log_assert(!isnan(cpu->f32s.dest_angle_xyz[1]));
    T1_log_assert(!isnan(cpu->f32s.dest_angle_xyz[2]));
    
    T1_std_memset(cpu, 0, sizeof(T1CPURenderView));
    T1_std_memset(gpu, 0, sizeof(T1GPURenderView));
    
    cpu->T1_id = T1_ID_NONE;
    
    cpu->f32s.min_xyz[0] = T1_F32_MIN;
    cpu->f32s.min_xyz[1] = T1_F32_MIN;
    cpu->f32s.min_xyz[2] = T1_F32_MIN;
    cpu->f32s.max_xyz[0] = T1_F32_MAX;
    cpu->f32s.max_xyz[1] = T1_F32_MAX;
    cpu->f32s.max_xyz[2] = T1_F32_MAX;
    
    cpu->clamped_to_T1_id = T1_ID_NONE;
    cpu->reflect_cam = T1_ID_NONE;
    cpu->visible_layers = 0xFFFFFFFF;
    
    cpu->height = height;
    cpu->width  = width;
    
    gpu->cull_below_z = T1_F32_MIN;
    gpu->cull_above_z = T1_F32_MAX;
    
    T1GPUProjectConsts * pjc = &cpu->project;
    
    pjc->znear = 0.1f;
    pjc->zfar  = T1_ZFAR;
    
    f32 field_of_view = 75.0f;
    pjc->field_of_view_rad = (
        (field_of_view * 0.5f) / 180.0f) * 3.14159f;
    
    pjc->field_of_view_modifier = 1.0f /
        tanf(pjc->field_of_view_rad);
    
    pjc->aspect_ratio =
        (f32)cpu->width / (f32)cpu->height;
    
    pjc->x_multiplier =
        pjc->aspect_ratio *
        pjc->field_of_view_modifier;
    
    pjc->y_multiplier = pjc->field_of_view_modifier;
    
    T1_log_assert(!isnan(cpu->f32s.dest_xyz[0]));
    T1_log_assert(!isnan(cpu->f32s.dest_xyz[1]));
    T1_log_assert(!isnan(cpu->f32s.dest_xyz[2]));
    
    T1_log_assert(!isnan(cpu->f32s.dest_angle_xyz[0]));
    T1_log_assert(!isnan(cpu->f32s.dest_angle_xyz[1]));
    T1_log_assert(!isnan(cpu->f32s.dest_angle_xyz[2]));
}

s32 T1_render_view_fetch_next(
    const u32 width,
    const u32 height)
{
    s32 ret = -1;
    
    for (
        s32 rv_i = 0;
        rv_i < (s32)T1_render_views->size;
        rv_i++)
    {
        if (T1_render_views->cpu[rv_i].deleted) {
            ret = rv_i;
        }
    }
    
    if (
        ret < 0 &&
        T1_render_views->size < T1_RENDER_VIEW_CAP)
    {
        ret = (s32)T1_render_views->size;
        T1_render_views->size += 1;
    }
    
    T1_log_assert(ret >= 0);
    T1_log_assert(ret < T1_RENDER_VIEW_CAP);
    
    T1_render_view_construct(
        T1_render_views->cpu + ret,
        T1_render_views->gpu + ret,
        height,
        width);
    T1_render_views->cpu[ret].T1_id = T1_id_next_nonui_id();
    
    return ret;
}

void T1_render_view_update_positions(
    const u64 elapsed)
{
    if (T1_global->block_render_view_pos_updates) {
        return;
    }
    
    for (
        u32 rv_i = 0;
        rv_i < T1_render_views->size;
        rv_i++)
    {
        T1CPURenderView * rv = T1_render_views->cpu + rv_i;
        
        #if T1_REFLECTION_ACTIVE == T1_ACTIVE
        s32 mirrored_i = T1_render_view_T1_id_to_i(rv->reflect_cam);
        if (mirrored_i >= 0)
        {
            T1_log_assert(mirrored_i != (s32)rv_i);
            const T1CPURenderView * mirrored =
                T1_render_views->cpu + mirrored_i;
            
            // Supporting only z-planes for now:
            f32 plane_z = rv->f32s.reflect_plane_z;
            
            rv->us_to_destination = 0;
            
            // Reflect position (flip Z over the plane)
            rv->f32s.xyz[0] = mirrored->f32s.xyz[0];
            rv->f32s.xyz[1] = mirrored->f32s.xyz[1];
            rv->f32s.xyz[2] = 2.0f * plane_z - mirrored->f32s.xyz[2];
            
            rv->f32s.angle_xyz[0] = -(3.14159f +
                mirrored->f32s.angle_xyz[0]);
            rv->f32s.angle_xyz[1] = mirrored->f32s.angle_xyz[1];
            rv->f32s.angle_xyz[2] = mirrored->f32s.angle_xyz[2];
            
            T1_render_views->gpu[rv_i].cull_above_z = plane_z;
        }
        #elif T1_REFLECTION_ACTIVE == T1_INACTIVE
        #else
        #error
        #endif
        
        if (rv->us_to_destination < 1)
        {
            continue;
        }
        
        T1_log_assert(!isnan(rv->f32s.dest_xyz[0]));
        T1_log_assert(!isnan(rv->f32s.dest_xyz[1]));
        T1_log_assert(!isnan(rv->f32s.dest_xyz[2]));
        
        T1_log_assert(!isnan(rv->f32s.dest_angle_xyz[0]));
        T1_log_assert(!isnan(rv->f32s.dest_angle_xyz[1]));
        T1_log_assert(!isnan(rv->f32s.dest_angle_xyz[2]));
        
        u64 us_actual;
        if (elapsed > rv->us_to_destination) {
            us_actual = rv->us_to_destination;
        } else {
            us_actual = elapsed + ((rv->us_to_destination -
                elapsed) / 7);
        }
        
        T1_log_assert(rv->f32s.max_xyz[0] >= rv->f32s.min_xyz[0]);
        T1_log_assert(rv->f32s.max_xyz[1] >= rv->f32s.min_xyz[1]);
        T1_log_assert(rv->f32s.max_xyz[2] >= rv->f32s.min_xyz[2]);
        
        if (rv->f32s.dest_angle_xyz[0] < rv->f32s.min_angle_xyz[0]) { rv->f32s.dest_angle_xyz[0] = rv->f32s.min_angle_xyz[0]; } 
        if (rv->f32s.dest_angle_xyz[0] > rv->f32s.max_angle_xyz[0]) { rv->f32s.dest_angle_xyz[0] = rv->f32s.max_angle_xyz[0]; }
        if (rv->f32s.dest_angle_xyz[1] < rv->f32s.min_angle_xyz[1]) { rv->f32s.dest_angle_xyz[1] = rv->f32s.min_angle_xyz[1]; } 
        if (rv->f32s.dest_angle_xyz[1] > rv->f32s.max_angle_xyz[1]) { rv->f32s.dest_angle_xyz[1] = rv->f32s.max_angle_xyz[1]; }
        if (rv->f32s.dest_angle_xyz[2] < rv->f32s.min_angle_xyz[2]) { rv->f32s.dest_angle_xyz[2] = rv->f32s.min_angle_xyz[2]; } 
        if (rv->f32s.dest_angle_xyz[2] > rv->f32s.max_angle_xyz[2]) { rv->f32s.dest_angle_xyz[2] = rv->f32s.max_angle_xyz[2]; }
        
        if (rv->f32s.dest_xyz[0] < rv->f32s.min_xyz[0]) { rv->f32s.dest_xyz[0] = rv->f32s.min_xyz[0]; } 
        if (rv->f32s.dest_xyz[0] > rv->f32s.max_xyz[0]) { rv->f32s.dest_xyz[0] = rv->f32s.max_xyz[0]; }
        if (rv->f32s.dest_xyz[1] < rv->f32s.min_xyz[1]) { rv->f32s.dest_xyz[1] = rv->f32s.min_xyz[1]; } 
        if (rv->f32s.dest_xyz[1] > rv->f32s.max_xyz[1]) { rv->f32s.dest_xyz[1] = rv->f32s.max_xyz[1]; }
        if (rv->f32s.dest_xyz[2] < rv->f32s.min_xyz[2]) { rv->f32s.dest_xyz[2] = rv->f32s.min_xyz[2]; } 
        if (rv->f32s.dest_xyz[2] > rv->f32s.max_xyz[2]) { rv->f32s.dest_xyz[2] = rv->f32s.max_xyz[2]; }
        
        f32 elapsed_pct = (f32)us_actual / (f32)rv->us_to_destination;
        T1_log_assert(elapsed_pct >= 0.0f);
        T1_log_assert(elapsed_pct <= 1.0f);
        T1_log_assert(!isnan(elapsed_pct));
        rv->us_to_destination -= (u32)us_actual;
        T1_log_assert(rv->us_to_destination < 5000000);
        
        if (rv->movement_enabled) {
            rv->f32s.xyz[0] =
                (rv->f32s.xyz[0]      * (1.0f - elapsed_pct)) +
                (rv->f32s.dest_xyz[0] * elapsed_pct);
            rv->f32s.xyz[1] =
                (rv->f32s.xyz[1]      * (1.0f - elapsed_pct)) +
                (rv->f32s.dest_xyz[1] * elapsed_pct);
            rv->f32s.xyz[2] =
                (rv->f32s.xyz[2]      * (1.0f - elapsed_pct)) +
                (rv->f32s.dest_xyz[2] * elapsed_pct);
            T1_log_assert(!isnan(rv->f32s.xyz[0]));
            T1_log_assert(!isnan(rv->f32s.xyz[1]));
            T1_log_assert(!isnan(rv->f32s.xyz[2]));
            
            rv->f32s.angle_xyz[0] =
                ((f32)rv->f32s.angle_xyz[0] * (1.0f - elapsed_pct)) +
                ((f32)rv->f32s.dest_angle_xyz[0] * elapsed_pct);
            rv->f32s.angle_xyz[1] =
                ((f32)rv->f32s.angle_xyz[1] * (1.0f - elapsed_pct)) +
                ((f32)rv->f32s.dest_angle_xyz[1] * elapsed_pct);
            rv->f32s.angle_xyz[2] =
                ((f32)rv->f32s.angle_xyz[2] * (1.0f - elapsed_pct)) +
                ((f32)rv->f32s.dest_angle_xyz[2] * elapsed_pct);
        }
        
        T1_log_assert(!isnan(rv->f32s.dest_xyz[0]));
        T1_log_assert(!isnan(rv->f32s.dest_xyz[1]));
        T1_log_assert(!isnan(rv->f32s.dest_xyz[2]));
        
        T1_log_assert(!isnan(rv->f32s.dest_angle_xyz[0]));
        T1_log_assert(!isnan(rv->f32s.dest_angle_xyz[1]));
        T1_log_assert(!isnan(rv->f32s.dest_angle_xyz[2]));
    }
}

void T1_render_view_delete(
    const s32 rv_i)
{
    T1_log_assert(rv_i >= 0);
    T1_log_assert(rv_i < T1_RENDER_VIEW_CAP);
    
    T1_render_view_construct(
        T1_render_views->cpu + rv_i,
        T1_render_views->gpu + rv_i,
        0,
        0);
    T1_render_views->cpu[rv_i].deleted = 1;
    
    while (
        T1_render_views->size > 0 &&
        T1_render_views->cpu[T1_render_views->size-1].
            deleted)
    {
        T1_render_views->size -= 1;
    }
}

void T1_render_view_validate(void) {
    T1_log_assert(T1_render_views != NULL);
    T1_log_assert(
        T1_render_views->size <= T1_RENDER_VIEW_CAP);
    
    for (u32 i = 0; i < T1_render_views->size; i++) {
        if (T1_render_views->cpu[i].deleted) {
            continue;
        }
        
        T1_log_assert(T1_render_views->cpu[i].width > 0);
        T1_log_assert(T1_render_views->cpu[i].height > 0);
        T1_log_assert(
            T1_render_views->cpu[i].project.aspect_ratio > 0.01f);
    }
    
    T1_log_assert(
        T1_render_views->size == 0 ||
        !T1_render_views->cpu[T1_render_views->size-1].deleted);
}

f32 T1_render_view_screen_x_to_x(
    const f32 screenspace_x,
    const f32 given_z)
{
    f32 viewport_width  = (f32)T1_settings_get_render_width();
    f32 viewport_height = (f32)T1_settings_get_render_height();
    
    f32 aspect_ratio =
        viewport_width / viewport_height;
    
    const f32 vertical_fov_degrees = 75.0f;
    f32 half_fov_y_rad =
        (vertical_fov_degrees * 0.5f) *
            (3.14159265359f / 180.0f);
    f32 tan_half_fov_y = tanf(half_fov_y_rad);
    
    f32 full_frustum_width_at_z =
        2.0f * given_z * tan_half_fov_y * aspect_ratio;
    
    f32 ndc_x =
        (screenspace_x / viewport_width) * 2.0f - 1.0f;
    
    f32 view_space_x =
        ndc_x * (full_frustum_width_at_z * 0.5f);
    
    return view_space_x;
}

f32 T1_render_view_screen_y_to_y(
    const f32 screenspace_y,
    const f32 given_z)
{
    f32 viewport_height = (f32)T1_settings_get_render_height();
    
    const f32 vertical_fov_degrees = 75.0f;
    f32 half_fov_y_rad = (vertical_fov_degrees * 0.5f) * (3.14159265359f / 180.0f);
    f32 tan_half_fov_y = tanf(half_fov_y_rad);
    
    f32 full_frustum_height_at_z = 2.0f * given_z * tan_half_fov_y;
    
    f32 ndc_y = 1.0f -
        (screenspace_y / viewport_height) * 2.0f;
    
    f32 view_space_y = ndc_y *
        (full_frustum_height_at_z * 0.5f);
    
    return -view_space_y;
}

f32 T1_render_view_screen_height_to_height(
    const f32 screenspace_height,
    const f32 given_z)
{
    f32 viewport_height = (f32)T1_settings_get_render_height();
    
    const f32 vertical_fov_degrees = 75.0f;
    f32 half_fov_y_rad = (vertical_fov_degrees * 0.5f) * (3.14159265359f / 180.0f);
    f32 tan_half_fov_y = tanf(half_fov_y_rad);
    
    f32 full_frustum_height_at_z =
        2.0f * given_z * tan_half_fov_y;
    
    f32 screen_fraction =
        screenspace_height / viewport_height;
    
    return screen_fraction * full_frustum_height_at_z;
}

f32 T1_render_view_screen_width_to_width(
    const f32 screenspace_width,
    const f32 given_z)
{
    f32 viewport_width = (f32)T1_settings_get_render_width();
    f32 aspect_ratio =
        viewport_width / (f32)T1_settings_get_render_height();
    
    const f32 vertical_fov_deg = 75.0f;
    f32 half_fov_y_rad =
        (vertical_fov_deg * 0.5f) *
            (3.14159265359f / 180.0f);
    f32 tan_half_fov_y = tanf(half_fov_y_rad);
    
    f32 full_frustum_width_at_z = 2.0f * given_z * tan_half_fov_y * aspect_ratio;
    
    f32 screen_fraction = screenspace_width / viewport_width;
    
    return screen_fraction * full_frustum_width_at_z;
}

s32 T1_render_view_T1_id_to_i(const u32 T1_id)
{
    if (T1_id == T1_ID_NONE) { return -1; }
    
    for (s32 i = 0; i < (s32)T1_render_views->size; i++) {
        if (
            !T1_render_views->cpu[i].deleted &&
            T1_render_views->cpu[i].T1_id == T1_id)
        {
            return i;
        }
    }
    
    return -1;
}

void T1_render_view_delete_T1_id(const u32 T1_id)
{
    s32 rv_i = T1_render_view_T1_id_to_i(T1_id);
    if (rv_i >= 0) {
        T1_render_view_delete(rv_i);
    }
}

static void * T1_render_view_store_struct(
    const s32 rv_i,
    const u8 store)
{
    switch (store) {
        case T1_PROP_STORE_CAM_F32:
            return &T1_render_views->cpu[rv_i].f32s;
        case T1_PROP_STORE_CAM_U32:
            return &T1_render_views->cpu[rv_i];
        default:
            T1_log_assert(0);
            return NULL;
    }
}

u32 T1_render_view_set_property(
    const u32 T1_id,
    const T1Property * property,
    const u32 bits)
{
    s32 rv_i = T1_render_view_T1_id_to_i(T1_id);
    if (rv_i < 0) { return 0; }
    
    T1_property_write(
        T1_render_view_store_struct(rv_i, property->store),
        property,
        bits);
    return 1;
}

void T1_render_view_rename_T1_id(const u32 from_T1_id, const u32 to_T1_id)
{
    for (u32 i = 0; i < T1_render_views->size; i++) {
        if (T1_render_views->cpu[i].T1_id == from_T1_id) {
            T1_render_views->cpu[i].T1_id = to_T1_id;
        }
    }
}

static b8 T1_render_view_is_anim_target(const u32 i, const u32 T1_id)
{
    return
        !T1_render_views->cpu[i].deleted &&
        (T1_id == T1_ANIM_HIT_EVERYTHING ||
            T1_render_views->cpu[i].T1_id == T1_id);
}

void T1_render_view_apply_anim_step(
    const u32 T1_id,
    const u8 store,
    const T1AnimStep * step)
{
    for (u32 i = 0; i < T1_render_views->size; i++) {
        if (!T1_render_view_is_anim_target(i, T1_id)) { continue; }
        
        T1_property_apply_anim_step(
            T1_render_view_store_struct((s32)i, store),
            step);
    }
}
