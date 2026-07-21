#include "T1.h"

#include "decode_png.h"
#include "T1_log.h"
#include "T1_tex_array.h"
#include "T1_render_view.h"
#include "T1_texquad.h"
#include "T1_zlight.h"
#include "T1_io.h"
#include "T1_platform_layer.h"

void T1_assert(b8 condition) {
    T1_log_assert(condition);
}

void T1_cam_set_us_to_dest(s32 cam_i, u64 us) {
    T1_log_assert(cam_i == 0); // TODO: remove this debug check
    T1_render_views->cpu[cam_i].us_to_destination = us; }
f32  T1_cam_get_angle_xyz(s32 cam_i, s32 i) {
    return T1_render_views->cpu[cam_i].angle_xyz[i]; }
void T1_cam_set_dest_xyz(s32 cam_i, s32 i, f32 newval) {
    T1_render_views->cpu[cam_i].dest_xyz[i] = newval; }
void T1_cam_add_dest_xyz(s32 cam_i, s32 i, f32 newval) {
    T1_render_views->cpu[cam_i].dest_xyz[i] += newval; }
void T1_cam_set_dest_angle_xyz(s32 cam_i, s32 i, f32 val) {
    T1_render_views->cpu[cam_i].dest_angle_xyz[i] = val; }
void T1_cam_add_dest_angle_xyz(s32 cam_i, s32 i, f32 val) {
    T1_render_views->cpu[cam_i].dest_angle_xyz[i] += val; }
void T1_cam_set_min_xyz(s32 cam_i, s32 i, f32 val) {
    T1_render_views->cpu[cam_i].min_xyz[i] = val; }
void T1_cam_set_max_xyz(s32 cam_i, s32 i, f32 val) {
    T1_render_views->cpu[cam_i].max_xyz[i] = val; }
void T1_cam_set_angle_xyz_min(s32 cam_i, s32 i, f32 val) {
    T1_render_views->cpu[cam_i].min_angle_xyz[i] = val; }
void T1_cam_set_angle_xyz_max(s32 cam_i, s32 i, f32 val) {
    T1_render_views->cpu[cam_i].max_angle_xyz[i] = val; }
void T1_cam_set_clamped_to_T1_id(s32 i, u32 T1_id) {
    T1_render_views->cpu[i].clamped_to_T1_id = T1_id; }
void T1_cam_set_movement_enabled(s32 i, u8 newval) {
    T1_render_views->cpu[i].movement_enabled = newval; }
T1Tex T1_cam_get_write_tex(s32 i) {
    return T1_render_views->cpu[i].write_tex; }
void T1_cam_reset(s32 at_i) {
    return T1_render_view_reset(at_i); }
void T1_cam_delete(s32 rv_i) {
    T1Tex tex = T1_render_views->cpu[rv_i].write_tex;
    if (tex != T1_TEX_NONE) {
        T1_tex_array_delete_slice(
            T1_tex_to_array_i(tex),
            T1_tex_to_slice_i(tex));
        T1_render_views->cpu[rv_i].write_tex = T1_TEX_NONE;
    }
    
    T1_render_view_delete(rv_i);
}
void T1_cam_delete_all(void) {
    // We want to delete even above T1_render_views->size
    // to ensure that attached render target textures
    // are also deleted
    for (s32 i = 0; i < T1_RENDER_VIEW_CAP; i++) {
        T1_cam_delete(i);
    }
}
f32 T1_screen_x_to_x(f32 ss_x, f32 z) {
    return T1_render_view_screen_x_to_x(ss_x, z); }
f32 T1_screen_y_to_y(f32 ss_y, f32 z) {
    return T1_render_view_screen_y_to_y(ss_y, z); }
f32 T1_x_to_screen_x(f32 x, f32 z) {
    return T1_render_view_x_to_screen_x(x, z); }
f32 T1_y_to_screen_y(f32 y, f32 z) {
    return T1_render_view_y_to_screen_y(y, z); }
f32 T1_screen_x_to_x_noz(f32 ss_x) {
    return T1_render_view_screen_x_to_x_noz(ss_x); }
f32 T1_screen_y_to_y_noz(f32 ss_y) {
    return T1_render_view_screen_y_to_y_noz(ss_y); }
f32 T1_x_to_screen_x_noz(f32 y) {
    return T1_render_view_x_to_screen_x_noz(y); }
f32 T1_y_to_screen_y_noz(f32 y) {
    return T1_render_view_y_to_screen_y_noz(y); }
f32 T1_screen_width_to_width(f32 ss_w, f32 at_z) {
    return T1_render_view_screen_width_to_width(ss_w, at_z); }
f32 T1_screen_height_to_height(f32 ss_h, f32 at_z) {
    return T1_render_view_screen_height_to_height(ss_h, at_z); }
f32 T1_screen_width_to_width_noz(f32 ss_w) {
    return T1_render_view_screen_width_to_width_noz(ss_w); }
f32 T1_screen_height_to_height_noz(f32 ss_h) {
    return T1_render_view_screen_height_to_height_noz(ss_h); }

void T1_make_shadowmap_and_attach_to_light(
    u32 light_T1_id,
    u32 new_cam_width,
    u32 new_cam_height)
{
    #if T1_SHADOWS_ACTIVE == T1_ACTIVE
    s32 zl_i = -1;
    for (
        s32 i = 0;
        i < (s32)T1_zlights_size;
        i++)
    {
        if (T1_zlights[i].T1_id == light_T1_id) {
            zl_i = i;
        }
    }
    
    T1_log_assert(zl_i >= 0);
    if (zl_i < 0) { return; }
    
    s32 new_rv_i = T1_render_view_fetch_next(
        new_cam_width,
        new_cam_height);
    T1_log_assert(new_rv_i >= 0);
    if (new_rv_i < 0) { return; }
    
    s16 slice_i = T1_os_gpu_make_depth_tex(
        /* u32 width: */
            T1_render_views->cpu[new_rv_i].width,
        /* u32 height: */
            T1_render_views->cpu[new_rv_i].height);
    
    T1_log_assert(slice_i >= 0);
    if (slice_i < 0) {
        T1_render_view_delete(new_rv_i);
        return;
    }
    
    T1CPURenderView * shadowm_cpu =
        T1_render_views->cpu + new_rv_i;
    T1_log_assert(shadowm_cpu->width == (u32)new_cam_width);
    T1_log_assert(shadowm_cpu->height == (u32)new_cam_height);
    shadowm_cpu->write_type = T1RENDERVIEW_WRITE_DEPTH;
    shadowm_cpu->write_tex = 0;
    T1_tex_set_array_i(&shadowm_cpu->write_tex, T1_DEPTH_TEXTUREARRAYS_I);
    T1_tex_set_slice_i(&shadowm_cpu->write_tex, slice_i);
    
    shadowm_cpu->passes_size = 2;
    shadowm_cpu->passes[0].type = T1RENDERPASS_DIAMOND_ALPHA;
    shadowm_cpu->passes[1].type = T1RENDERPASS_ALPHA_BLEND;
    
    T1_os_gpu_update_internal_render_viewport(new_rv_i);
    
    T1_log_assert(slice_i >= 0);
    T1_zlights[zl_i].shadow_map_depth_texture_i = slice_i;
    T1_zlights[zl_i].shadow_map_render_view_i = new_rv_i;
    
    T1_render_view_validate();
    #elif T1_SHADOWS_ACTIVE == T1_INACTIVE
    (void)light_T1_id;
    (void)new_cam_width;
    (void)new_cam_height;
    #else
    #error
    #endif
}

void T1_cam_create_main_view(
    u32 new_cam_width,
    u32 new_cam_height)
{
    T1_log_assert(T1_render_views->size == 0);
    
    s32 rv_i = T1_tex_array_create_new_render_view(
        new_cam_width,
        new_cam_height);
    
    T1_log_assert(
        !isnan(T1_render_views->cpu[rv_i].dest_xyz[0]));
    T1_log_assert(
        !isnan(T1_render_views->cpu[rv_i].dest_xyz[1]));
    T1_log_assert(
        !isnan(T1_render_views->cpu[rv_i].dest_xyz[2]));
    
    T1_log_assert(rv_i == 0);
    T1_log_assert(T1_render_views->size > 0);
    
    T1_os_gpu_update_internal_render_viewport(0);
    
    T1_os_gpu_update_window_viewport();
}

#if T1_REFLECTION_ACTIVE == T1_ACTIVE
void T1_make_reflection_cam(
    u32 new_cam_w,
    u32 new_cam_h,
    f32 reflection_z)
{
    s32 new_rv_i =
        T1_tex_array_create_new_render_view(
            /* u32 width: */
                new_cam_w,
            /* u32 height: */
                new_cam_h);
    
    T1_log_assert(new_rv_i >= 0);
    if (new_rv_i < 0) { return; }
    
    T1_log_assert(new_rv_i == 1);
    
    T1CPURenderView * refl_cpu =
        T1_render_views->cpu + new_rv_i;
    refl_cpu->write_type = T1RENDERVIEW_WRITE_RGBA;
    
    T1_log_assert(refl_cpu->width == new_cam_w);
    T1_log_assert(refl_cpu->height == new_cam_h);
    T1_log_assert(refl_cpu->write_tex != T1_TEX_NONE);
    
    T1_log_assert(T1_tex_to_array_i(refl_cpu->write_tex) < T1_TEXARRAYS_CAP);
    T1_log_assert(T1_tex_to_slice_i(refl_cpu->write_tex) >= 0);
    T1_log_assert(T1_tex_to_slice_i(refl_cpu->write_tex) <
        (s32)T1_tex_arrays[T1_tex_to_array_i(refl_cpu->write_tex)].images_size);
    
    refl_cpu->reflect_around_plane_z = true;
    refl_cpu->refl_cam_around_plane_z = reflection_z;
    
    refl_cpu->passes_size = 2;
    refl_cpu->passes[0].type = T1RENDERPASS_DIAMOND_ALPHA;
    refl_cpu->passes[1].type = T1RENDERPASS_ALPHA_BLEND;
    
    T1_os_gpu_update_internal_render_viewport(new_rv_i);
    
    T1_render_view_validate();
}
#elif T1_REFLECTION_ACTIVE == T1_INACTIVE
void T1_make_reflection_cam(
    u32 new_cam_w,
    u32 new_cam_h,
    f32 reflection_z) {}
#else
#error
#endif

void
T1_png_get_width_height(
    const u8 * compressed_input,
    u64 compressed_input_size,
    u32 * const out_width,
    u32 * const out_height,
    u8 * out_good)
{
    decode_png_get_width_height(
        compressed_input,
        compressed_input_size,
        out_width,
        out_height,
        out_good);
}
void T1_png_decode(
    const u8 * compressed_input,
    u64 compressed_input_size,
    u8 * out_rgba_values,
    u64 rgba_values_size,
    u32 thread_id,
    u8 * out_good)
{
    decode_png(
        compressed_input,
        compressed_input_size,
        out_rgba_values,
        rgba_values_size,
        thread_id,
        out_good);
}
