#include "T1_std.h"
#include "T1_mem.h"
#include "T1_linalg3d.h"
#include "T1_global.h"
#include "T1_log.h"
#include "T1_zlight.h"
#include "T1_render_view.h"
#include "T1_platform_layer.h"

/*
render_views[0] is the global camera

In a 2D game, move the x to the left to move all of
your zsprites to the right

render_views[1] and up are alternative camera texture
outputs that you can render to, for example to make
a shadow map (set write_type to
T1RENDERVIEW_WRITE_DEPTH) or to make a reflection or
security camera (set write_type
T1RENDERVIEW_WRITE_RGBA)
*/
T1RenderViewCollection * T1_render_views = NULL;
T1CPURenderView * T1_cam = NULL;

void T1_render_view_init(void) {
    T1_log_assert(T1_RENDER_VIEW_CAP > 0);
    T1_render_views = T1_mem_malloc_unmanaged(
        sizeof(T1RenderViewCollection));
    
    T1_cam = T1_render_views->cpu; // convenience
    
    T1_std_memset(
        T1_render_views,
        0,
        sizeof(T1RenderViewCollection));
    
    for (uint32_t i = 0; i < T1_RENDER_VIEW_CAP; i++) {
        T1_render_views->cpu[i].write_array_i = -1;
        T1_render_views->cpu[i].write_slice_i = -1;
        T1_render_views->cpu[i].clamped_to_zsprite_id = -1;
    }
}

typedef struct {
    float    last_drag_start_camera_x;
    float    last_drag_start_camera_y;
    float    last_drag_start_x;
    float    last_drag_start_y;
    float    total_dragged_x;
    float    total_dragged_y;
} T1CamState;

void T1_render_view_reset(int i)
{
    T1CPURenderView * rv = T1_render_views->cpu + i;
    
    T1_log_assert(!isnan(rv->dest_xyz[0]));
    T1_log_assert(!isnan(rv->dest_xyz[1]));
    T1_log_assert(!isnan(rv->dest_xyz[2]));
    T1_log_assert(!isnan(rv->dest_angle_xyz[0]));
    T1_log_assert(!isnan(rv->dest_angle_xyz[1]));
    T1_log_assert(!isnan(rv->dest_angle_xyz[2]));
    
    rv->us_to_destination = 0;
    rv->xyz[0] = 0.0f;
    rv->xyz[1] = 0.0f;
    rv->xyz[2] = 0.0f;
    rv->angle_xyz[0] = 0.0f;
    rv->angle_xyz[1] = 0.0f;
    rv->angle_xyz[2] = 0.0f;
    rv->dest_xyz[0] = 0.0f;
    rv->dest_xyz[1] = 0.0f;
    rv->dest_xyz[2] = 0.0f;
    rv->dest_angle_xyz[0] = 0.0f;
    rv->dest_angle_xyz[1] = 0.0f;
    rv->dest_angle_xyz[2] = 0.0f;
    rv->min_xyz[0] = T1_F32_MIN;
    rv->min_xyz[1] = T1_F32_MIN;
    rv->min_xyz[2] = T1_F32_MIN;
    rv->max_xyz[0] = T1_F32_MAX;
    rv->max_xyz[1] = T1_F32_MAX;
    rv->max_xyz[2] = T1_F32_MAX;
    
    T1_log_assert(!isnan(rv->dest_xyz[0]));
    T1_log_assert(!isnan(rv->dest_xyz[1]));
    T1_log_assert(!isnan(rv->dest_xyz[2]));
    T1_log_assert(!isnan(rv->dest_angle_xyz[0]));
    T1_log_assert(!isnan(rv->dest_angle_xyz[1]));
    T1_log_assert(!isnan(rv->dest_angle_xyz[2]));
}

static void T1_render_view_construct(
    T1CPURenderView * cpu,
    T1GPURenderView * gpu,
    const uint32_t height,
    const uint32_t width)
{
    T1_log_assert(!isnan(cpu->dest_xyz[0]));
    T1_log_assert(!isnan(cpu->dest_xyz[1]));
    T1_log_assert(!isnan(cpu->dest_xyz[2]));
    
    T1_log_assert(!isnan(cpu->dest_angle_xyz[0]));
    T1_log_assert(!isnan(cpu->dest_angle_xyz[1]));
    T1_log_assert(!isnan(cpu->dest_angle_xyz[2]));
    
    T1_std_memset(cpu, 0, sizeof(T1CPURenderView));
    T1_std_memset(gpu, 0, sizeof(T1GPURenderView));
    
    cpu->clamped_to_zsprite_id = -1;
    cpu->write_array_i = -1;
    cpu->write_slice_i = -1;
    
    cpu->height = height;
    cpu->width  = width;
    
    cpu->passes_size = 2;
    cpu->passes[0].type =
        T1RENDERPASS_DIAMOND_ALPHA;
    cpu->passes[1].type =
        T1RENDERPASS_ALPHA_BLEND;
    
    gpu->cull_below_z = T1_F32_MIN;
    gpu->cull_above_z = T1_F32_MAX;
    
    T1GPUProjectConsts * pjc = &cpu->project;
    
    pjc->znear = 0.1f;
    pjc->zfar  = T1_ZFAR;
    
    float field_of_view = 75.0f;
    pjc->field_of_view_rad = (
        (field_of_view * 0.5f) / 180.0f) * 3.14159f;
    
    pjc->field_of_view_modifier = 1.0f /
        tanf(pjc->field_of_view_rad);
    
    pjc->aspect_ratio =
        (float)cpu->width / (float)cpu->height;
    
    pjc->x_multiplier =
        pjc->aspect_ratio *
        pjc->field_of_view_modifier;
    
    pjc->y_multiplier = pjc->field_of_view_modifier;
    
    T1_log_assert(!isnan(cpu->dest_xyz[0]));
    T1_log_assert(!isnan(cpu->dest_xyz[1]));
    T1_log_assert(!isnan(cpu->dest_xyz[2]));
    
    T1_log_assert(!isnan(cpu->dest_angle_xyz[0]));
    T1_log_assert(!isnan(cpu->dest_angle_xyz[1]));
    T1_log_assert(!isnan(cpu->dest_angle_xyz[2]));
}

int32_t T1_render_view_fetch_next(
    const uint32_t width,
    const uint32_t height)
{
    int32_t ret = -1;
    
    for (
        int32_t rv_i = 0;
        rv_i < (int32_t)T1_render_views->size;
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
        ret = (int32_t)T1_render_views->size;
        T1_render_views->size += 1;
    }
    
    T1_log_assert(ret >= 0);
    T1_log_assert(ret < T1_RENDER_VIEW_CAP);
    
    T1_render_view_construct(
        T1_render_views->cpu + ret,
        T1_render_views->gpu + ret,
        height,
        width);
    
    return ret;
}

void T1_render_view_update_positions(
    const uint64_t elapsed)
{
    if (T1_global->block_render_view_pos_updates) {
        return;
    }
    
    for (
        uint32_t rv_i = 0;
        rv_i < T1_render_views->size;
        rv_i++)
    {
        T1CPURenderView * rv = T1_render_views->cpu + rv_i;
        
        if (rv->us_to_destination < 1) {
            continue;
        }
        
        T1_log_assert(!isnan(rv->dest_xyz[0]));
        T1_log_assert(!isnan(rv->dest_xyz[1]));
        T1_log_assert(!isnan(rv->dest_xyz[2]));
        
        T1_log_assert(!isnan(rv->dest_angle_xyz[0]));
        T1_log_assert(!isnan(rv->dest_angle_xyz[1]));
        T1_log_assert(!isnan(rv->dest_angle_xyz[2]));
        
        uint64_t us_actual;
        if (elapsed > rv->us_to_destination) {
            us_actual = rv->us_to_destination;
        } else {
            us_actual = elapsed;
        }
        
        float elapsed_pct = (float)us_actual / (float)rv->us_to_destination;
        T1_log_assert(elapsed_pct >= 0.0f);
        T1_log_assert(elapsed_pct <= 1.0f);
        T1_log_assert(!isnan(elapsed_pct));
        rv->us_to_destination -= us_actual;
        T1_log_assert(rv->us_to_destination < 5000000);
        
        rv->xyz[0] =
            (rv->xyz[0]      * (1.0f - elapsed_pct)) +
            (rv->dest_xyz[0] * elapsed_pct);
        rv->xyz[1] =
            (rv->xyz[1]      * (1.0f - elapsed_pct)) +
            (rv->dest_xyz[1] * elapsed_pct);
        rv->xyz[2] =
            (rv->xyz[2]      * (1.0f - elapsed_pct)) +
            (rv->dest_xyz[2] * elapsed_pct);
        T1_log_assert(!isnan(rv->xyz[0]));
        T1_log_assert(!isnan(rv->xyz[1]));
        T1_log_assert(!isnan(rv->xyz[2]));
        
        rv->angle_xyz[0] =
            ((float)rv->angle_xyz[0] * (1.0f - elapsed_pct)) +
            ((float)rv->dest_angle_xyz[0] * elapsed_pct);
        rv->angle_xyz[1] =
            ((float)rv->angle_xyz[1] * (1.0f - elapsed_pct)) +
            ((float)rv->dest_angle_xyz[1] * elapsed_pct);
        rv->angle_xyz[2] =
            ((float)rv->angle_xyz[2] * (1.0f - elapsed_pct)) +
            ((float)rv->dest_angle_xyz[2] * elapsed_pct);
        
        T1_log_assert(!isnan(rv->dest_xyz[0]));
        T1_log_assert(!isnan(rv->dest_xyz[1]));
        T1_log_assert(!isnan(rv->dest_xyz[2]));
        
        T1_log_assert(!isnan(rv->dest_angle_xyz[0]));
        T1_log_assert(!isnan(rv->dest_angle_xyz[1]));
        T1_log_assert(!isnan(rv->dest_angle_xyz[2]));
        
        if (rv->reflect_around_plane)
        {
            // Assuming the main camera is always at 0
            // Assuming the main camera can't be a
            // reflection view
            T1_log_assert(rv_i != 0);
            
            // Supporting only z-planes for now:
            float plane_z = rv->refl_cam_around_plane_xyz[2];
            
            // Reflect position (flip Z over the plane)
            rv->xyz[0] = T1_cam->xyz[0];
            rv->xyz[1] = T1_cam->xyz[1];
            rv->xyz[2] = 2.0f * plane_z - T1_cam->xyz[2];
            
            rv->angle_xyz[0] = -(3.14159f +
                T1_cam->angle_xyz[0]);
            rv->angle_xyz[1] = T1_cam->angle_xyz[1];
            rv->angle_xyz[2] = T1_cam->angle_xyz[2];
            
            T1_render_views->gpu[rv_i].cull_above_z =
                rv->refl_cam_around_plane_xyz[2];
        }
        
        T1_log_assert(!isnan(rv->dest_xyz[0]));
        T1_log_assert(!isnan(rv->dest_xyz[1]));
        T1_log_assert(!isnan(rv->dest_xyz[2]));
        
        T1_log_assert(!isnan(rv->dest_angle_xyz[0]));
        T1_log_assert(!isnan(rv->dest_angle_xyz[1]));
        T1_log_assert(!isnan(rv->dest_angle_xyz[2]));
    }
}

void T1_render_view_delete(
    const int32_t rv_i)
{
    T1_log_assert(rv_i >= 0);
    T1_log_assert(rv_i < T1_RENDER_VIEW_CAP);
    
    T1_render_view_construct(
        T1_render_views->cpu + rv_i,
        T1_render_views->gpu + rv_i,
        0,
        0);
    T1_render_views->cpu[rv_i].deleted = true;
    
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
        T1_render_views->size > 0);
    T1_log_assert(
        T1_render_views->size <= T1_RENDER_VIEW_CAP);
    
    for (
        uint32_t i = 1;
        i < T1_render_views->size;
        i++)
    {
        if (T1_render_views->cpu[i].deleted) {
            continue;
        }
        
        for (
            int32_t pass_i = 0;
            pass_i < T1_render_views->cpu[i].
                passes_size;
            pass_i++)
        {
            T1_log_assert(T1_render_views->cpu[i].
                passes[pass_i].type >
                    T1RENDERPASS_BELOWBOUNDS);
            T1_log_assert(T1_render_views->cpu[i].
                passes[pass_i].type <
                    T1RENDERPASS_ABOVEBOUNDS);
        }
        
        T1_log_assert(
            T1_render_views->cpu[i].write_type >
                T1RENDERVIEW_WRITE_BELOWBOUNDS);
        T1_log_assert(
            T1_render_views->cpu[i].write_type <
                T1RENDERVIEW_WRITE_ABOVEBOUNDS);
        T1_log_assert(
            T1_render_views->cpu[i].write_type !=
                T1RENDERVIEW_WRITE_RENDER_TARGET);
        
        T1_log_assert(
            T1_render_views->cpu[i].project.aspect_ratio > 0.1f);
    }
    
    T1_log_assert(!T1_render_views->
        cpu[T1_render_views->size-1].deleted);
    
    T1_log_assert(
        T1_render_views->cpu[0].write_type ==
            T1RENDERVIEW_WRITE_RENDER_TARGET);
}

float T1_render_view_x_to_screen_x(
    const float x,
    const float given_z)
{
    const T1CPURenderView * cpu =
        &T1_render_views->cpu[0];
    
    float viewport_width  = (float)cpu->width;
    float viewport_height = (float)cpu->height;
    float aspect_ratio =
        viewport_width / viewport_height;
    
    const float vertical_fov_degrees = 75.0f;
    float half_fov_y_rad =
        (vertical_fov_degrees * 0.5f) *
            (3.14159265359f / 180.0f);
    float tan_half_fov_y = tanf(half_fov_y_rad);
    
    float full_frustum_width_at_z =
        2.0f * given_z * tan_half_fov_y * aspect_ratio;
    
    float ndc_x = x / (full_frustum_width_at_z * 0.5f);
    
    float screen_x = (ndc_x + 1.0f) * 0.5f * viewport_width;
    
    return screen_x;
}

float T1_render_view_x_to_screen_x_noz(
    const float x)
{
    return ((x * 2.0f) - 1.0f) * (float)T1_render_views->cpu[0].width;
}

float T1_render_view_screen_x_to_x(
    const float screenspace_x,
    const float given_z)
{
    T1_log_assert(T1_render_views->cpu[0].width > 0);
    T1_log_assert(T1_render_views->cpu[0].height > 0);
    
    const T1CPURenderView * cpu =
        &T1_render_views->cpu[0];
    
    float viewport_width  = (float)cpu->width;
    float viewport_height = (float)cpu->height;
    
    float aspect_ratio =
        viewport_width / viewport_height;
    
    const float vertical_fov_degrees = 75.0f;
    float half_fov_y_rad =
        (vertical_fov_degrees * 0.5f) *
            (3.14159265359f / 180.0f);
    float tan_half_fov_y = tanf(half_fov_y_rad);
    
    float full_frustum_width_at_z =
        2.0f * given_z * tan_half_fov_y * aspect_ratio;
    
    float ndc_x =
        (screenspace_x / viewport_width) * 2.0f - 1.0f;
    
    float view_space_x =
        ndc_x * (full_frustum_width_at_z * 0.5f);
    
    return view_space_x;
}

float T1_render_view_y_to_screen_y(
    const float y,
    const float given_z)
{
    const T1CPURenderView * cpu = &T1_render_views->cpu[0];
    
    float viewport_height = (float)cpu->height;
    
    const float vertical_fov_degrees = 75.0f;
    float half_fov_y_rad = (vertical_fov_degrees * 0.5f) * (3.14159265359f / 180.0f);
    float tan_half_fov_y = tanf(half_fov_y_rad);
    
    float full_frustum_height_at_z =
        2.0f * given_z * tan_half_fov_y;
    
    float ndc_y = y / (full_frustum_height_at_z * 0.5f);
    
    float screen_y = (1.0f - ndc_y) * 0.5f * viewport_height;
    
    return screen_y;
}

float T1_render_view_y_to_screen_y_noz(
    const float y)
{
    return ((y * 2.0f) - 1.0f) * (float)T1_render_views->cpu[0].height;
}

float T1_render_view_screen_y_to_y(
    const float screenspace_y,
    const float given_z)
{
    const T1CPURenderView * cpu =
        &T1_render_views->cpu[0];
    
    float viewport_height = (float)cpu->height;
    
    const float vertical_fov_degrees = 75.0f;
    float half_fov_y_rad = (vertical_fov_degrees * 0.5f) * (3.14159265359f / 180.0f);
    float tan_half_fov_y = tanf(half_fov_y_rad);
    
    float full_frustum_height_at_z = 2.0f * given_z * tan_half_fov_y;
    
    float ndc_y = 1.0f -
        (screenspace_y / viewport_height) * 2.0f;
    
    float view_space_y = ndc_y *
        (full_frustum_height_at_z * 0.5f);
    
    return -view_space_y;
}

float T1_render_view_screen_height_to_height(
    const float screenspace_height,
    const float given_z)
{
    const T1CPURenderView * cpu =
        &T1_render_views->cpu[0];
    
    float viewport_height = (float)cpu->height;
    
    const float vertical_fov_degrees = 75.0f;
    float half_fov_y_rad = (vertical_fov_degrees * 0.5f) * (3.14159265359f / 180.0f);
    float tan_half_fov_y = tanf(half_fov_y_rad);
    
    float full_frustum_height_at_z =
        2.0f * given_z * tan_half_fov_y;
    
    float screen_fraction =
        screenspace_height / viewport_height;
    
    return screen_fraction * full_frustum_height_at_z;
}

float T1_render_view_screen_x_to_x_noz(
    const float screen_x)
{
    return -1.0f + (screen_x /
        ((float)T1_render_views->cpu[0].width) *
            2.0f);
}

float T1_render_view_screen_y_to_y_noz(
    const float screen_y)
{
    return -1.0f + (screen_y /
        ((float)T1_render_views->cpu[0].height)
            * 2.0f);
}

float T1_render_view_screen_width_to_width_noz(
    const float screenspace_width)
{
    return (screenspace_width /
        (float)T1_render_views->cpu[0].width) *
            2.0f;
}

float T1_render_view_screen_height_to_height_noz(
    const float screenspace_height)
{
    return (screenspace_height /
        (float)T1_render_views->cpu[0].height) *
            2.0f;
}

float T1_render_view_screen_width_to_width(
    const float screenspace_width,
    const float given_z)
{
    const T1CPURenderView * cpu =
        &T1_render_views->cpu[0];
    
    float viewport_width =
        (float)cpu->width;
    float aspect_ratio =
        viewport_width / (float)cpu->height;
    
    const float vertical_fov_deg = 75.0f;
    float half_fov_y_rad =
        (vertical_fov_deg * 0.5f) *
            (3.14159265359f / 180.0f);
    float tan_half_fov_y = tanf(half_fov_y_rad);
    
    float full_frustum_width_at_z = 2.0f * given_z * tan_half_fov_y * aspect_ratio;
    
    float screen_fraction = screenspace_width / viewport_width;
    
    return screen_fraction * full_frustum_width_at_z;
}
