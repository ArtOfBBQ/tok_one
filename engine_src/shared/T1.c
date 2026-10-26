#include "T1.h"

#include "T1_stdint.h"
#include "T1_conditional_includes.h"

#include "T1_log.h"
#include "T1_tex_array.h"
#include "T1_render_view.h"
#include "T1_texquad.h"
#include "T1_zlight.h"
#include "T1_io.h"
#include "T1_os.h"

u32 T1_cam_new(u32 width, u32 height) {
    T1_log_assert(width > 0);
    T1_log_assert(height > 0);
    
    s32 rv_i = T1_render_view_fetch_next(width, height);
    T1_log_assert(rv_i >= 0);
    if (rv_i < 0) { return T1_ID_NONE; }
    
    // its depth buffer
    T1_os_gpu_update_internal_render_viewport(rv_i);
    
    T1_render_view_validate();
    
    return T1_render_views->cpu[rv_i].T1_id;
}
void T1_cam_reset(u32 T1_id) {
    s32 rv_i = T1_render_view_T1_id_to_i(T1_id);
    T1_log_assert(rv_i >= 0);
    if (rv_i >= 0) { T1_render_view_reset(rv_i); } }
void T1_cam_delete_all(void) {
    for (s32 i = 0; i < T1_RENDER_VIEW_CAP; i++) {
        T1_render_view_delete(i);
    }
}
f32 T1_screen_x_to_x(f32 ss_x, f32 z) {
    return T1_render_view_screen_x_to_x(ss_x, z); }
f32 T1_screen_y_to_y(f32 ss_y, f32 z) {
    return T1_render_view_screen_y_to_y(ss_y, z); }
f32 T1_screen_width_to_width(f32 ss_w, f32 at_z) {
    return T1_render_view_screen_width_to_width(ss_w, at_z); }
f32 T1_screen_height_to_height(f32 ss_h, f32 at_z) {
    return T1_render_view_screen_height_to_height(ss_h, at_z); }

void T1_touch_set_tex(T1Tex tex) {
    T1_os_gpu_set_touch_tex(tex);
}

/*
3D models
*/
f32 T1_get_x_mul_for_width(s32 for_mesh_id, f32 for_width) {
    return T1_global_get_x_mul_for_width(for_mesh_id, for_width); }
f32 T1_get_y_mul_for_height(s32 for_mesh_id, f32 for_height) {
    return T1_global_get_y_mul_for_height(for_mesh_id, for_height); }
f32 T1_get_z_mul_for_depth(s32 for_mesh_id, f32 for_depth) {
    return T1_global_get_z_mul_for_depth(for_mesh_id, for_depth); }
