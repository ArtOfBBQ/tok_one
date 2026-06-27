#ifndef T1_H
#define T1_H

#include "T1_public_types.h"

/*
This is an encapsulation header - it exposes the functions you
need to make your application or game, and should be the only T1
file you #include.

Migrating here is a work in progress - it's less chaotic than
needing to understand the entire engine, but still overcomplicated.

- 'Width' and 'Height' are generally abbreviated to 'w' and 'h'
- "x", "y", or "z" positions indicate positions in world space
- "screen_x", "screen_y" are screenspace positions
*/

/*
GLOBAL SETTINGS
TODO: remove this from the public API
*/
extern T1Globals * T1_global;

/*
CAMERA MANIPULATION
*/
void T1_cam_set_us_to_dest(s32 cam_i, const u64 us);
void T1_cam_set_dest_xyz(s32 cam_i, s32 i, f32 newval);
void T1_cam_add_dest_xyz(s32 cam_i, s32 i, f32 newval);
void T1_cam_set_dest_angle_xyz(s32 cam_i, s32 i, f32 newval);
void T1_cam_add_dest_angle_xyz(s32 cam_i, s32 i, f32 val);
void T1_cam_set_min_xyz(s32 cam_i, s32 i, f32 val);
void T1_cam_set_max_xyz(s32 cam_i, s32 i, f32 val);
// to unclamp, clamp to T1_id = -1
void T1_cam_set_clamped_to_T1_id(s32 cam_i, s32 T1_id);
void T1_cam_set_movement_enabled(s32 cam_i, u8 newval);
void T1_cam_delete(const s32 cam_i);
void T1_cam_delete_all(void);
T1Tex T1_cam_get_write_tex(s32 cam_i);
void T1_cam_reset(s32 at_i);

// To convert from our screenspace system to 'world x' that is used for
// the position of zpolygons
f32 T1_screen_x_to_x(const f32 screen_x, const f32 given_z);
f32 T1_screen_y_to_y(const f32 screen_y, const f32 at_z);
f32 T1_x_to_screen_x(const f32 screen_x, const f32 at_z);
f32 T1_y_to_screen_y(const f32 y, const f32 given_z);
f32 T1_screen_x_to_x_noz(const f32 screen_x);
f32 T1_screen_y_to_y_noz(const f32 screen_y);
f32 T1_x_to_screen_x_noz(const f32 y);
f32 T1_y_to_screen_y_noz(const f32 y);
f32 T1_screen_height_to_height(const f32 screen_h, const f32 at_z);
f32 T1_screen_width_to_width(const f32 screen_w, const f32 at_z);
f32 T1_screen_width_to_width_noz(const f32 screen_w);
f32 T1_screen_height_to_height_noz(const f32 screen_h);

void T1_make_shadowmap_and_attach_to_light(
    const s32 T1_id, const u32 w, const u32 h);

void T1_cam_create_main_view(const u32 new_w, const u32 new_h);
void T1_make_reflection_cam(
    const u32 new_w, const u32 new_h,
    const f32 pos_x, const f32 pos_y, const f32 pos_z,
    const f32 angle_x, const f32 angle_y, const f32 angle_z,
    const f32 reflection_z);

/*
FILE PARSING
*/
void T1_png_get_width_height(
    const u8 * compressed_input, const u64 compressed_input_size,
    u32 * out_width, u32 * out_height,
    u8 * out_good);
void T1_png_decode(
    const u8 * compressed_input, const u64 compressed_input_size,
    const u8 * out_rgba, const u64 out_rgba_cap, const u32 thread_id,
    u8 * good);

/*
MANAGE TEXTURES
*/
void T1_tex_files_prereg_png_res(const c8 * filename, b8 * good);
void T1_tex_files_prereg_dds_res(const c8 * filename, b8 * good);
void T1_tex_files_reg_new_by_splitting_file(
    const c8 * filename, const u32 rows, const u32 cols);
    
u16 T1_tex_array_get_filename_loc(const c8 * for_filename);
u16 T1_tex_array_reg_img(
    const c8 * filename,
    const u32 w, const u32 h,
    const b8 is_render_target, const b8 use_bc1_compression);
void T1_tex_array_update_rgba(
    const s32 array_i, const s32 slice_i,
    const u8 * rgba, const u32 rgba_size);

/*
MANAGE 3-D MODELS (.OBJ FILES)

Each "mesh" you register will return a mesh_id (i32)

When creating a T1 object later, you can set the mesh_id
in the T1MakeRequest to this value to make an object with
that mesh.
*/
s32 T1_objmodel_new_mesh_id_from_resources(
    const c8 * filename,
    const c8 * mtl_filename,
    const u8 flip_uv_u,
    const u8 flip_uv_v,
    u8 * success,
    c8 * error_message);

/*
TEXT LABELS

Set fields in T1_text_props before requesting a label to choose
the font properties etc. for your next label
*/
extern T1TextFontSettings * T1_text_props;

void T1_text_request_label_offset_around(
    const s32 with_T1_id, const c8 * text,
    const f32 mid_screen_x, const f32 mid_screen_y, const f32 z,
    const f32 max_width);
void T1_text_request_label_leftx_toplinemidy(
    const s32 with_T1_id, const c8 * text,
    const f32 screen_left, const f32 topline_mid_screen_y, const f32 z,
    const f32 max_width);
void T1_text_request_label_renderable(
    const s32 with_T1_id,
    const c8 * text,
    const f32 left_x_pixelspace,
    const f32 top_y_pixelspace,
    const f32 z,
    const f32 tab_width,
    const f32 max_width);
void T1_text_request_label_around_x_at_top_y(
    const s32 with_T1_id, const c8 * text_to_draw,
    const f32 screen_mid_x, const f32 screen_top_y, const f32 z,
    const f32 max_width);
void T1_text_request_label_around(
    const s32 with_T1_id, const c8 * text_to_draw,
    const f32 screen_mid_x, const f32 screen_mid_y, const f32 z,
    const f32 max_width);

/*
Creating T1 objects
each object has a "T1_id", set it and store it to interact
with it later. If you give multiple objects the same T1_id, you
can move and delete them together as a group

You can use a T1MakeRequest to request objects

Example:
T1MakeRequest req;
T1_makerequest_construct(&req);
req.xyz[2] = 2.0f; // put our object at z position 2.0f
req.wh[1] = 0.5f; // give our object a height of 0.5f
req.rgba[3] = 1.0f; // make our object fully transparent
...
T1_texquad_make(&req); // make an object, in this case a texquad

there are other functions to make other stuff
*/
typedef struct {
    f32 xyz[3];
    f32 wh[2];
    f32 rgba[4];
    s32 T1_id;
    s32 touch_id;
    u16 tex;
} T1MakeRequest;
void T1_makerequest_construct(T1MakeRequest * to_construct);

/*
UI WIDGETS
*/
void T1_ui_widget_delete(const s32 T1_id);

/*
3D models
*/
f32 T1_get_x_mul_for_width(const s32 for_mesh_id, const f32 for_width);
f32 T1_get_y_mul_for_height(const s32 for_mesh_id, const f32 for_height);
f32 T1_get_z_mul_for_depth(const s32 for_mesh_id, const f32 for_depth);

/*
TexQuads (textured 2D quads)
*/
void T1_texquad_make(T1MakeRequest * request); // returns T1_id
void T1_texquad_delete(const s32 T1_id);
void T1_texquad_delete_all(void);

/*
z-Lights (3D lights)
*/
T1zLight * T1_zlight_next(void);
void T1_zlight_commit(T1zLight * to_request);
void T1_zlight_delete(const s32 T1_id);

/*
INPUTS FROM MOUSE, KEYBOARD, GAMEPAD

Short taps and long taps will disappear when you "consume"
them, but there may have been multiple short taps in a single
frame, so check if you care about that 

Short taps and long taps get cleared every frame, but a key
being down does not. If you don't consume a tap on the frame
when it ends, you will lose it
*/
b8 T1_io_key_is_down(T1IOKey key);
b8 T1_io_key_consume_tap_began_frame(T1IOKey key);
b8 T1_io_key_consume_short_tap_this_frame(T1IOKey key);
b8 T1_io_key_consume_long_tap_this_frame(T1IOKey key);
f32 T1_io_get_mouse_x_this_frame(void);
f32 T1_io_get_mouse_y_this_frame(void);
s32 T1_io_get_mouse_touch_id_this_frame(void);

/*
OPERATING SYSTEM

The "res dir" (resources directory) is a special directory
that you're guaranteed to be allowed to read from, and
where your assets are presumably stored

The "writables" dir is a special directory where your app
is allowed to write, so you can store files with data about
your user's progress or preferences, etc.
*/
void T1_os_get_res_dir(c8 * recip, const u32 recip_cap);
void T1_os_get_filenames_in(
    const c8 * directory, c8 filenames[2000][500]);
u64 T1_os_get_resource_size(const c8 * res_name);
void T1_os_read_resource_file(
    const c8 * filename,
    c8 * recip, const u64 recip_cap,
    u8 * good);
void T1_os_get_app_dir(c8 * recip, const u32 recip_size);
void T1_os_get_writables_dir(c8 * recip, const u32 recip_size);
void T1_os_gpu_push_tex_slice_and_free_rgba(
    const s32 tex_array_i,
    const s32 tex_slice_i);
void T1_os_toggle_fullscreen(void);
u64 T1_os_get_current_time_us(void);
void T1_os_open_dir_in_file_explorer_window_if_possible(
    const c8 * folderpath);

#endif // T1_H
