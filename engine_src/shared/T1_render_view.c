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
    
    gpu->cull_below_z = T1_F32_MIN;
    gpu->cull_above_z = T1_F32_MAX;
    
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

static void
T1_render_view_pos_point_to_pos_to_angle3(
    float * recip_xyz_angle,
    const float * from_pos_xyz,
    const float * point_to_xyz)
{
    float dir[3];
    dir[0] =
        point_to_xyz[0] - from_pos_xyz[0];
    dir[1] =
        point_to_xyz[1] - from_pos_xyz[1];
    dir[2] =
        point_to_xyz[2] - from_pos_xyz[2];
    
    float length = sqrtf(
        dir[0] * dir[0] +
        dir[1] * dir[1] +
        dir[2] * dir[2]);
    
    if (length < 1e-6f) {
        log_assert(0);
        recip_xyz_angle[0] = 0.0f;
        recip_xyz_angle[1] = 0.0f;
        recip_xyz_angle[2] = 0.0f;
        return;
    }
    
    float inv_length = 1.0f / length;
    dir[0] *= inv_length;
    dir[1] *= inv_length;
    dir[2] *= inv_length;
    
    recip_xyz_angle[0] = -atan2f(
        dir[1],
        sqrtf(dir[0]*dir[0] + dir[2]*dir[2]));;
    recip_xyz_angle[1] = atan2f(
        dir[0], dir[2]);
    recip_xyz_angle[2] = 0.0f;
}

static void
T1_render_view_angle3_to_direction(
    float * recip_dir,
    const float * angle_xyz)
{
    float pitch = angle_xyz[0]; // X
    float yaw   = angle_xyz[1]; // Y
    
    float cp = cosf(pitch);
    float sp = sinf(pitch);
    float cy = cosf(yaw);
    float sy = sinf(yaw);
    
    recip_dir[0] =  sy * cp;
    recip_dir[1] = -sp;
    recip_dir[2] =  cy * cp;
}

static void
T1_render_view_get_arbitrary_lookat_point(
    float * new_pos,
    const float * angle_xyz,
    const float * from_pos_xyz)
{
    // get an arbitrary look-at position
    float look_at_dist = 1.0f; // arbitrary dist
    
    float forward[3];
    T1_render_view_angle3_to_direction(
        forward,
        angle_xyz);
    
    new_pos[0] = from_pos_xyz[0] + forward[0] * look_at_dist;
    new_pos[1] = from_pos_xyz[1] + forward[1] * look_at_dist;
    new_pos[2] = from_pos_xyz[2] + forward[2] * look_at_dist;
}

void T1_render_view_update_positions(
    void)
{
    if (T1_global->block_render_view_pos_updates) {
        return;
    }
    
    for (
        uint32_t rv_i = 0;
        rv_i < T1_render_views->size;
        rv_i++)
    {
        if (
            T1_render_views->cpu[rv_i].
                reflect_around_plane)
        {
            // Assuming the main camera is always at 0
            // Assuming the main camera can't be a
            // reflection view
            log_assert(rv_i != 0);
            
            // Supporting only z-planes for now:
            float plane_z = T1_render_views->cpu[rv_i].
                refl_cam_around_plane_xyz[2];
            
            // Reflect position (flip Z over the plane)
            T1_render_views->cpu[rv_i].xyz[0] = T1_camera->xyz[0];
            T1_render_views->cpu[rv_i].xyz[1] = T1_camera->xyz[1];
            T1_render_views->cpu[rv_i].xyz[2] = 2.0f * plane_z - T1_camera->xyz[2];
            
            T1_render_views->cpu[rv_i].xyz_angle[0] =
                -(3.14159265359f +
                    T1_camera->xyz_angle[0]);
            T1_render_views->cpu[rv_i].xyz_angle[1] =
                T1_camera->xyz_angle[1];
            T1_render_views->cpu[rv_i].xyz_angle[2] =
                T1_camera->xyz_angle[2];
            
            T1_render_views->gpu[rv_i].
                cull_above_z =
                    T1_render_views->cpu[rv_i].
                        refl_cam_around_plane_xyz[2];
        }
    }
}

void T1_render_view_delete(
    const int32_t rv_i)
{
    log_assert(rv_i >= 0);
    log_assert(rv_i < T1_RENDER_VIEW_CAP);
    
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
