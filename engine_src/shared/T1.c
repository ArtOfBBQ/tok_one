#include "T1.h"

#include "T1_stdint.h"
#include "T1_conditional_includes.h"

#include "T1_assert.h"
#include "T1_tex_array.h"
#include "T1_render_view.h"
#include "T1_texquad.h"
#include "T1_zlight.h"
#include "T1_io.h"
#include "T1_os.h"

u32 T1_cam_new(u32 width, u32 height) {
    T1_assert(width > 0);
    T1_assert(height > 0);
    
    s32 rv_i = T1_render_view_fetch_next(width, height);
    T1_assert(rv_i >= 0);
    if (rv_i < 0) { return T1_ID_NONE; }
    
    // its depth buffer
    T1_os_gpu_update_internal_render_viewport(rv_i);
    
    T1_render_view_validate();
    
    return T1_render_views->cpu[rv_i].T1_id;
}
void T1_cam_reset(u32 T1_id) {
    s32 rv_i = T1_render_view_T1_id_to_i(T1_id);
    T1_assert(rv_i >= 0);
    if (rv_i >= 0) { T1_render_view_reset(rv_i); } }
void T1_cam_delete_all(void) {
    for (s32 i = 0; i < T1_RENDER_VIEW_CAP; i++) {
        T1_render_view_delete(i);
    }
}
f32 T1_screen_x_to_x(u32 cam_T1_id, f32 ss_x, f32 z) {
    f32 wh[2];
    T1_render_view_get_wh(cam_T1_id, wh);
    return T1_render_view_screen_x_to_x(wh, ss_x, z);
}
f32 T1_screen_y_to_y(u32 cam_T1_id, f32 ss_y, f32 z) {
    f32 wh[2];
    T1_render_view_get_wh(cam_T1_id, wh);
    return T1_render_view_screen_y_to_y(wh, ss_y, z);
}
f32 T1_screen_width_to_width(u32 cam_T1_id, f32 ss_w, f32 at_z) {
    f32 wh[2];
    T1_render_view_get_wh(cam_T1_id, wh);
    return T1_render_view_screen_width_to_width(wh, ss_w, at_z);
}
f32 T1_screen_height_to_height(u32 cam_T1_id, f32 ss_h, f32 at_z) {
    f32 wh[2];
    T1_render_view_get_wh(cam_T1_id, wh);
    return T1_render_view_screen_height_to_height(wh, ss_h, at_z);
}

u32 T1_tex_read_start(T1Tex tex, u32 x, u32 y, u32 w, u32 h) {
    T1_assert(tex != T1_TEX_NONE && tex != T1_TEX_WINDOW);
    T1_assert(T1_TEX_ARRAY_I(tex) < T1_TEXARRAYS_CAP);
    T1_assert(w > 0 && h > 0);
    T1_assert(x + w <= T1_tex_get_width(tex));
    T1_assert(y + h <= T1_tex_get_height(tex));
    if (
        tex == T1_TEX_NONE ||
        tex == T1_TEX_WINDOW ||
        T1_TEX_ARRAY_I(tex) >= T1_TEXARRAYS_CAP ||
        w == 0 || h == 0 ||
        x + w > T1_tex_get_width(tex) ||
        y + h > T1_tex_get_height(tex))
    {
        return T1_BUF_READ_NONE;
    }
    
    return T1_os_gpu_tex_read_start(tex, x, y, w, h);
}

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

/*
Files
*/
void T1_os_res_path(const char * filename, char * recip, u32 recip_cap) {
    T1_os_res_filename_to_pathfile(filename, recip, recip_cap);
}
void T1_os_writable_path(const char * filename, char * recip, u32 recip_cap) {
    T1_os_writable_filename_to_pathfile(filename, recip, recip_cap);
}
u64 T1_os_file_size(const char * path) {
    return T1_os_get_filesize(path);
}
void T1_os_file_read(
    const char * path,
    char * recip,
    u64 recip_cap,
    char ** const fatal_error)
{
    u32 size = 0;
    T1_os_read_file(path, recip, &size, recip_cap, fatal_error);
}
void T1_os_file_write(
    const char * path,
    const char * data,
    u32 size,
    u8 * good)
{
    T1_os_write_file(path, data, size, good);
}
void T1_os_file_delete(const char * path) {
    T1_os_del_file(path);
}
void T1_os_file_copy(const char * src_path, const char * dst_path) {
    T1_os_copy_file(src_path, dst_path);
}
void T1_os_dir_filenames(
    const char * path,
    char * filenames,
    u32 filenames_cap)
{
    T1_os_get_filenames_in(path, filenames, filenames_cap);
}
void T1_os_dir_show(const char * path) {
    T1_os_open_dir_in_file_explorer_window_if_possible(path);
}
