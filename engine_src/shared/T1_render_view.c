#include "T1_render_view.h"

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
T1CPURenderView * T1_camera = NULL;

void T1_render_view_init(void) {
    log_assert(T1_RENDER_VIEW_CAP > 0);
    T1_render_views = T1_mem_malloc_from_unmanaged(
        sizeof(T1RenderViewCollection));
    
    T1_camera = T1_render_views->cpu; // convenience
    
    T1_std_memset(
        T1_render_views,
        0,
        sizeof(T1RenderViewCollection));
    
    for (uint32_t i = 0; i < T1_RENDER_VIEW_CAP; i++) {
        T1_render_views->cpu[i].write_array_i = -1;
        T1_render_views->cpu[i].write_slice_i = -1;
    }
}

static void T1_render_view_construct(
    T1CPURenderView * cpu,
    T1GPURenderView * gpu,
    const uint32_t height,
    const uint32_t width)
{
    T1_std_memset(cpu, 0, sizeof(T1CPURenderView));
    T1_std_memset(gpu, 0, sizeof(T1GPURenderView));
    
    cpu->write_array_i = -1;
    cpu->write_slice_i = -1;
    
    cpu->height = height;
    cpu->width  = width;
    
    T1GPUProjectConsts * pjc = &cpu->project;
    
    pjc->znear =  0.1f;
    pjc->zfar  =  6.0f;
    
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
}

int32_t T1_render_view_fetch_next(
    const uint32_t height,
    const uint32_t width)
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
    } else {
        return ret;
    }
    
    log_assert(ret >= 0);
    log_assert(ret < T1_RENDER_VIEW_CAP);
    
    T1_render_view_construct(
        T1_render_views->cpu + ret,
        T1_render_views->gpu + ret,
        height,
        width);
    
    return ret;
}

void T1_render_view_delete(
    const int32_t rv_i)
{
    log_assert(rv_i >= 0);
    log_assert(rv_i < T1_RENDER_VIEW_CAP);
    
    T1_render_view_construct(
        T1_render_views->cpu,
        T1_render_views->gpu,
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
    log_assert(T1_render_views != NULL);
    log_assert(
        T1_render_views->size > 0);
    log_assert(
        T1_render_views->size <= T1_RENDER_VIEW_CAP);
    
    for (
        uint32_t i = 1;
        i < T1_render_views->size;
        i++)
    {
        if (T1_render_views->cpu[i].deleted) {
            continue;
        }
        
        log_assert(
            T1_render_views->cpu[i].write_type >
                T1RENDERVIEW_WRITE_BELOWBOUNDS);
        log_assert(
            T1_render_views->cpu[i].write_type <
                T1RENDERVIEW_WRITE_ABOVEBOUNDS);
        log_assert(
            T1_render_views->cpu[i].write_type !=
                T1RENDERVIEW_WRITE_RENDER_TARGET);
        
        log_assert(
            T1_render_views->cpu[i].project.aspect_ratio > 0.1f);
    }
    
    log_assert(!T1_render_views->
        cpu[T1_render_views->size-1].deleted);
    
    log_assert(
        T1_render_views->cpu[0].write_type ==
            T1RENDERVIEW_WRITE_RENDER_TARGET);
    log_assert(T1_render_views->cpu[0].width == T1_global->window_width);
    log_assert(T1_render_views->cpu[0].height == T1_global->window_height);
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

float T1_render_view_screen_x_to_x(
    const float screenspace_x,
    const float given_z)
{
    log_assert(T1_render_views->cpu[0].width > 0);
    log_assert(T1_render_views->cpu[0].height > 0);
    
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
