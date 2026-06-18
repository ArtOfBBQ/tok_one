#include "T1.h"

#include <stdio.h>

#include "decode_png.h"
#include "T1_log.h"
#include "T1_tex_array.h"
#include "T1_render_view.h"
#include "T1_texquad.h"
#include "T1_zlight.h"
#include "T1_io.h"
#include "T1_platform_layer.h"

void T1_cam_set_us_to_dest(int cam_i, const uint64_t us) {
    T1_render_views->cpu[cam_i].us_to_destination = us; }
void T1_cam_set_dest_xyz(int cam_i, int i, float newval) {
    T1_render_views->cpu[cam_i].dest_xyz[i] = newval; }
void T1_cam_add_dest_xyz(int cam_i, int i, float newval) {
    T1_render_views->cpu[cam_i].dest_xyz[i] += newval; }
void T1_cam_set_dest_angle_xyz(int cam_i, int i, float val) {
    T1_render_views->cpu[cam_i].dest_angle_xyz[i] = val; }
void T1_cam_add_dest_angle_xyz(int cam_i, int i, float val) {
    T1_render_views->cpu[cam_i].dest_angle_xyz[i] += val; }
void T1_cam_set_min_xyz(int cam_i, int i, float val) {
    T1_render_views->cpu[cam_i].min_xyz[i] = val; }
void T1_cam_set_max_xyz(int cam_i, int i, float val) {
    T1_render_views->cpu[cam_i].max_xyz[i] = val; }
void T1_cam_set_clamped_to_T1_id(int i, int32_t zsprite_id) {
    T1_render_views->cpu[i].clamped_to_zsprite_id = zsprite_id; }
void T1_cam_set_movement_enabled(int i, uint8_t newval) {
    T1_render_views->cpu[i].movement_enabled = newval; }
int32_t T1_cam_get_write_array_i(int i) {
    return T1_render_views->cpu[i].write_array_i; }
int32_t T1_cam_get_write_slice_i(int i) {
    return T1_render_views->cpu[i].write_slice_i; }
void T1_cam_reset(int at_i) {
    return T1_render_view_reset(at_i); }
void T1_cam_delete(const int rv_i) {
    if (
        T1_render_views->cpu[rv_i].
            write_array_i >= 0 &&
        T1_render_views->cpu[rv_i].
            write_slice_i >= 0)
    {
        T1_tex_array_delete_slice(
            /* const int32_t array_i: */
                T1_render_views->cpu[rv_i].
                    write_array_i,
            /* const int32_t slice_i: */
                T1_render_views->cpu[rv_i].
                    write_slice_i);
        T1_render_views->cpu[rv_i].
            write_array_i = -1;
        T1_render_views->cpu[rv_i].
            write_slice_i = -1;
    }
    
    T1_render_view_delete(rv_i);
}
void T1_cam_delete_all(void) {
    for (int i = 0; i < (int)T1_render_views->size; i++) {
        T1_cam_delete(i);
    }
}
float T1_screen_x_to_x(const float ss_x, const float z) {
    return T1_render_view_screen_x_to_x(ss_x, z); }
float T1_screen_y_to_y(const float ss_y, const float z) {
    return T1_render_view_screen_y_to_y(ss_y, z); }
float T1_x_to_screen_x(const float x, const float z) {
    return T1_render_view_x_to_screen_x(x, z); }
float T1_y_to_screen_y(const float y, const float z) {
    return T1_render_view_y_to_screen_y(y, z); }
float T1_screen_x_to_x_noz(const float ss_x) {
    return T1_render_view_screen_x_to_x_noz(ss_x); }
float T1_screen_y_to_y_noz(const float ss_y) {
    return T1_render_view_screen_y_to_y_noz(ss_y); }
float T1_x_to_screen_x_noz(const float y) {
    return T1_render_view_x_to_screen_x_noz(y); }
float T1_y_to_screen_y_noz(const float y) {
    return T1_render_view_y_to_screen_y_noz(y); }
float T1_screen_width_to_width(const float ss_w, const float at_z) {
    return T1_render_view_screen_width_to_width(ss_w, at_z); }
float T1_screen_height_to_height(const float ss_h, const float at_z) {
    return T1_render_view_screen_height_to_height(ss_h, at_z); }
float T1_screen_width_to_width_noz(const float ss_w) {
    return T1_render_view_screen_width_to_width_noz(ss_w); }
float T1_screen_height_to_height_noz(const float ss_h) {
    return T1_render_view_screen_height_to_height_noz(ss_h); }

void T1_make_shadowmap_and_attach_to_light(
    const int32_t light_zsprite_id,
    const uint32_t new_cam_width,
    const uint32_t new_cam_height)
{
    #if T1_SHADOWS_ACTIVE == T1_ACTIVE
    int32_t zl_i = -1;
    for (
        int32_t i = 0;
        i < (int32_t)T1_zlights_size;
        i++)
    {
        if (
            T1_zlights[i].T1_id == light_zsprite_id)
        {
            zl_i = i;
        }
    }
    
    T1_log_assert(zl_i >= 0);
    if (zl_i < 0) { return; }
    
    int32_t new_rv_i =
        T1_render_view_fetch_next(
            new_cam_width,
            new_cam_height);
    T1_log_assert(new_rv_i >= 0);
    if (new_rv_i < 0) { return; }
    
    int32_t slice_i =
        T1_platform_gpu_make_depth_tex(
            /* uint32_t width: */
                T1_render_views->cpu[new_rv_i].
                    width,
            /* uint32_t height: */
                T1_render_views->cpu[new_rv_i].
                    height);
    
    T1_log_assert(slice_i >= 0);
    if (slice_i < 0) {
        T1_render_view_delete(new_rv_i);
        return;
    }
    
    T1CPURenderView * shadowm_cpu =
        T1_render_views->cpu + new_rv_i;
    T1_log_assert(shadowm_cpu->width == (uint32_t)
        new_cam_width);
    T1_log_assert(shadowm_cpu->height == (uint32_t)
        new_cam_height);
    shadowm_cpu->write_type =
        T1RENDERVIEW_WRITE_DEPTH;
    shadowm_cpu->write_array_i =
        T1_DEPTH_TEXTUREARRAYS_I;
    shadowm_cpu->write_slice_i = slice_i;
    
    shadowm_cpu->passes_size = 2;
    shadowm_cpu->passes[0].type =
        T1RENDERPASS_DIAMOND_ALPHA;
    shadowm_cpu->passes[1].type =
        T1RENDERPASS_ALPHA_BLEND;
    
    T1_os_gpu_update_internal_render_viewport(
        new_rv_i);
    
    T1_zlights[zl_i].
        shadow_map_depth_texture_i = slice_i;
    T1_zlights[zl_i].
        shadow_map_render_view_i = new_rv_i;
    
    T1_render_view_validate();
    #elif T1_SHADOWS_ACTIVE == T1_INACTIVE
    #else
    #error
    #endif
}

void T1_cam_create_main_view(
    uint32_t new_cam_width,
    uint32_t new_cam_height)
{
    T1_log_assert(T1_render_views->size == 0);
    
    int32_t rv_i = T1_tex_array_create_new_render_view(
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
    
    T1_os_gpu_update_internal_render_viewport(rv_i);
}

void T1_make_reflection_cam(
    const uint32_t new_cam_w,
    const uint32_t new_cam_h,
    const float pos_x,
    const float pos_y,
    const float pos_z,
    const float angle_x, 
    const float angle_y,
    const float angle_z,
    const float reflection_z)
{
    int32_t new_rv_i =
        T1_tex_array_create_new_render_view(
            /* const uint32_t width: */
                new_cam_w,
            /* const uint32_t height: */
                new_cam_h);
    
    T1_log_assert(new_rv_i >= 0);
    if (new_rv_i < 0) { return; }
    
    T1_log_assert(new_rv_i == 1);
    
    T1CPURenderView * refl_cpu =
        T1_render_views->cpu + new_rv_i;
    refl_cpu->write_type =
        T1RENDERVIEW_WRITE_RGBA;
    
    T1_log_assert(refl_cpu->width == new_cam_w);
    T1_log_assert(refl_cpu->height == new_cam_h);
    T1_log_assert(refl_cpu->write_array_i >= 0);
    T1_log_assert(refl_cpu->write_array_i < T1_TEXARRAYS_CAP);
    T1_log_assert(refl_cpu->write_slice_i >= 0);
    T1_log_assert(refl_cpu->write_slice_i < (int32_t)T1_tex_arrays[refl_cpu->write_array_i].images_size);
    
    refl_cpu->xyz[0]       = pos_x;
    refl_cpu->xyz[1]       = pos_y;
    refl_cpu->xyz[2]       = pos_z;
    refl_cpu->angle_xyz[0] = angle_x;
    refl_cpu->angle_xyz[1] = angle_y;
    refl_cpu->angle_xyz[2] = angle_z;
        
    refl_cpu->reflect_around_plane = true;
    refl_cpu->refl_cam_around_plane_xyz[2] = reflection_z;
    
    refl_cpu->passes_size = 2;
    refl_cpu->passes[0].type =
        T1RENDERPASS_DIAMOND_ALPHA;
    refl_cpu->passes[1].type =
        T1RENDERPASS_ALPHA_BLEND;
    // refl_cpu->passes[2].type = T1RENDERPASS_BLOOM;
    
    T1_os_gpu_update_internal_render_viewport(
        new_rv_i);
        
    T1_render_view_validate();
}

void
T1_png_get_width_height(
    const uint8_t * compressed_input,
    const uint64_t compressed_input_size,
    uint32_t * out_width,
    uint32_t * out_height,
    uint8_t * out_good)
{
    decode_png_get_width_height(
        compressed_input,
        compressed_input_size,
        out_width,
        out_height,
        out_good);
}
void T1_png_decode(
    const uint8_t * compressed_input,
    const uint64_t compressed_input_size,
    const uint8_t * out_rgba_values,
    const uint64_t rgba_values_size,
    const uint32_t thread_id,
    uint8_t * out_good)
{
    decode_png(
        compressed_input,
        compressed_input_size,
        out_rgba_values,
        rgba_values_size,
        thread_id,
        out_good);
}

void T1_makerequest_construct(T1MakeRequest * to_construct) {
    T1_std_memset(to_construct, 0, sizeof(T1MakeRequest));
}

/*
TexQuads (textured 2D quads)
*/
void T1_texquad_make(T1MakeRequest * request) {
    T1FlatTexQuadRequest tq_req;
    T1_texquad_fetch_next(&tq_req);
    tq_req.cpu->T1_id = request->T1_id;
    tq_req.gpu->f32.xyz[0] = request->xyz[0];
    tq_req.gpu->f32.xyz[1] = request->xyz[1];
    tq_req.gpu->f32.xyz[2] = request->xyz[2];
    tq_req.gpu->f32.wh[0] = request->wh[0];
    tq_req.gpu->f32.wh[1] = request->wh[1];
    tq_req.gpu->i32.reserved_and_tex = 0x00000000 | request->tex;
    tq_req.gpu->f32.rgba[0] = request->rgba[0];
    tq_req.gpu->f32.rgba[1] = request->rgba[1];
    tq_req.gpu->f32.rgba[2] = request->rgba[2];
    tq_req.gpu->f32.rgba[3] = request->rgba[3];
    T1_texquad_commit(&tq_req);
}

/*
INPUTS FROM MOUSE, KEYBOARD, GAMEPAD
*/
float T1_io_get_mouse_scroll_pos(void) {
    return T1_io->mouse_scroll_pos;
}
void  T1_io_set_mouse_scroll_pos(float new_val) {
    T1_io->mouse_scroll_pos = new_val;
}
b8 T1_io_key_is_down_and_unhandled(T1IOKey key) {
    return T1_io->keymap[key];
}
void T1_io_key_mark_handled(T1IOKey key) {
    T1_io->keymap[key] = 0;
}
