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
#define NULL 0
#define true 1
#define false 0

/*
GLOBAL SETTINGS
TODO: remove this from the public API
*/
extern T1Globals * T1_global;

/*
DEBUG MODE
*/
#if T1_LOG_ASSERTS_ACTIVE == T1_ACTIVE
void T1_assert(b8 condition);
void T1_log_warn(u8 condition);
void T1_log_dump_and_crash(const c8 * crash_message);
#elif T1_LOG_ASSERTS_ACTIVE == T1_INACTIVE
#else
#define T1_assert(x)
#define T1_log_dump_and_crash(x)
#error
#endif

#if T1_LOG_PRINTF == T1_ACTIVE
void T1_log_append(const c8 *);
void T1_log_append_c8(c8 c8val);
void T1_log_append_f32(f32 f32val);
void T1_log_append_s32(s32 s32val);
void T1_log_append_u32(u32 u32val);
#elif T1_LOG_PRINTF == T1_INACTIVE
#define T1_log_append(string)
#define T1_log_append_c8(num)
#define T1_log_append_f32(num)
#define T1_log_append_s32(num)
#define T1_log_append_u32(num)
#else
#error
#endif

/*
T1_std
standard or commonly needed functions
*/
#define T1_std_abs(a) ((((a) > 0)*(a))+(((a) < 0)*-(a)))
#define T1_std_fabs(a) ((((a) > 0.0f)*(a))+(((a) < 0.0f)*-(a)))
s32    T1_std_mini(s32 x, s32 y);
s32    T1_std_maxi(s32 x, s32 y);
f32    T1_std_minf(f32 x, f32 y);
f32    T1_std_maxf(f32 x, f32 y);
void * T1_std_memset(void * input, s32 value, u64 size_bytes);
void   T1_std_memset_i16(void * input, s16 value, u32 size_bytes);
void * T1_std_memcpy(void * dest, const void * src, u64 n_bytes);
void   T1_std_strcat_cap(c8 * recip, u32 cap, const c8 * to_append);
void   T1_std_strcat_u32_cap(c8 * recip, u32 recip_size, u32 to_append);
void   T1_std_strcat_s32_cap(c8 * recip, u32 cap, s32 to_append);
u64    T1_std_strlen(const c8 * nullterm_str);
b8     T1_std_string_starts_with(const c8 * to_check, const c8 * start);
b8     T1_std_are_equal_strings(const c8 * str1, const c8 * str2);
void   T1_std_strsub(c8 * in, const c8 * to_match, const c8 * repl);
void   T1_std_s32_to_string(s32 input, c8 * recip);
void   T1_std_u32_to_string(u32 input, c8 * recipient);
u32    T1_std_string_to_u32_validate(const c8 * input, b8 * good);
s32    T1_std_string_to_int32_validate(const c8 * input, u8 * good);

#if T1_STD_ASSERTS_ACTIVE == T1_ACTIVE
#define T1_std_strcpy_cap(recip, recipsize, to_append) T1_std_internal_strcpy_cap(recip, recipsize, to_append);
void
T1_std_internal_strcpy_cap(
    c8 * recipient,
    const u32 recipient_cap,
    const c8 * origin);
#elif T1_STD_ASSERTS_ACTIVE == T1_INACTIVE
#define T1_std_strcpy_cap(recip, recipsize, to_append) T1_std_internal_strcpy_cap(recip, to_append);
void
T1_std_internal_strcpy_cap(
    c8 * recipient,
    const c8 * origin);
#else
#error
#endif

/*
T1_mem - allocate memory
*/
void * T1_mem_malloc_unmanaged(u64 size);
void * T1_mem_malloc_managed(u64 size);
void T1_mem_free_managed(void * to_free);

/*
User settings
*/
u32 T1_settings_get_render_width(void);
u32 T1_settings_get_render_height(void);

/*
T1_id
*/
#define T1_ID_FPS_COUNTER 0
#define T1_ID_DEBUG_TEXT 1
#define T1_ID_MAX 8000
#define T1_ID_FIRST_NONUI 1011
#define T1_ID_LAST_UI_TOUCH 1000
#define T1_ID_FIRST_NONUI_TOUCH 1001
s32  T1_id_next_ui_element_id(void);
s32  T1_id_next_nonui_id(void);
s32  T1_id_next_ui_element_touch_id(void);
void T1_id_clear_ui_element_touch_ids(void);
s32  T1_id_next_nonui_touch_id(void);

/*
RANDOM NUMBERS
*/
#define T1_RAND_SEQUENCE_SIZE 999
s32 T1_rand(void);
s32 T1_rand_at_i(u64 index);
void T1_rand_shuf_array(void * array, u32 array_sz, u32 elem_sz);

/*
CAMERA MANIPULATION
*/
void  T1_cam_set_us_to_dest(s32 cam_i, u64 us);
void  T1_cam_set_dest_xyz(s32 cam_i, s32 i, f32 newval);
void  T1_cam_add_dest_xyz(s32 cam_i, s32 i, f32 newval);
void  T1_cam_set_dest_angle_xyz(s32 cam_i, s32 i, f32 newval);
void  T1_cam_add_dest_angle_xyz(s32 cam_i, s32 i, f32 val);
void  T1_cam_set_min_xyz(s32 cam_i, s32 i, f32 val);
void  T1_cam_set_max_xyz(s32 cam_i, s32 i, f32 val);
// to unclamp, clamp to T1_id = -1
void  T1_cam_set_clamped_to_T1_id(s32 cam_i, s32 T1_id);
void  T1_cam_set_movement_enabled(s32 cam_i, u8 newval);
void  T1_cam_delete(s32 cam_i);
void  T1_cam_delete_all(void);
T1Tex T1_cam_get_write_tex(s32 cam_i);
void  T1_cam_reset(s32 at_i);

// To convert from our screenspace system to 'world x' that is used for
// the position of zpolygons
f32 T1_screen_x_to_x(f32 screen_x, f32 given_z);
f32 T1_screen_y_to_y(f32 screen_y, f32 at_z);
f32 T1_x_to_screen_x(f32 screen_x, f32 at_z);
f32 T1_y_to_screen_y(f32 y, f32 given_z);
f32 T1_screen_x_to_x_noz(f32 screen_x);
f32 T1_screen_y_to_y_noz(f32 screen_y);
f32 T1_x_to_screen_x_noz(f32 y);
f32 T1_y_to_screen_y_noz(f32 y);
f32 T1_screen_height_to_height(f32 screen_h, f32 at_z);
f32 T1_screen_width_to_width(f32 screen_w, f32 at_z);
f32 T1_screen_width_to_width_noz(f32 screen_w);
f32 T1_screen_height_to_height_noz(f32 screen_h);

void T1_make_shadowmap_and_attach_to_light(
    s32 T1_id, u32 w, u32 h);

void T1_cam_create_main_view(u32 new_w, u32 new_h);
void T1_make_reflection_cam(
    u32 new_w, u32 new_h,
    f32 pos_x, f32 pos_y, f32 pos_z,
    f32 angle_x, f32 angle_y, f32 angle_z,
    f32 reflection_z);

/*
FILE PARSING
*/
void T1_png_get_width_height(
    const u8 * compressed_input, u64 compressed_input_size,
    u32 * out_width, u32 * out_height,
    u8 * out_good);
void T1_png_decode(
    const u8 * compressed_input, u64 compressed_input_size,
    u8 * out_rgba, u64 out_rgba_cap, u32 thread_id,
    u8 * good);

/*
MANAGE TEXTURES
*/
#define T1_tex_to_array_i(x) (x == T1_TEX_NONE ? -1 : x >> 11)
#define T1_tex_to_slice_i(x) (x == T1_TEX_NONE ? -1 : x & 0x07FF)
void T1_tex_files_prereg_png_res(const c8 * filename, b8 * good);
void T1_tex_files_prereg_dds_res(const c8 * filename, b8 * good);
void T1_tex_files_reg_new_by_splitting_file(
    const c8 * filename, u32 rows, u32 cols);

u16 T1_tex_array_get_filename_loc(const c8 * for_filename);
u16 T1_tex_array_reg_img(
    const c8 * filename,
    u32 w, u32 h,
    b8 is_render_target, b8 use_bc1_compression);
void T1_tex_array_update_rgba(
    s32 array_i, s32 slice_i,
    const u8 * rgba, u32 rgba_size);
b8 T1_tex_array_tex_exists_and_is_not_deleted(T1Tex tex);
u32 T1_tex_array_get_img_height(s32 array_i);
u32 T1_tex_array_get_img_width(s32 array_i);

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
    u8 flip_uv_u,
    u8 flip_uv_v,
    u8 * success,
    c8 * error_message);
s32 T1_objmodel_resource_name_to_mesh_id(
    const c8 * obj_filename);
f32 T1_objmodel_get_x_multiplier_for_width(
    s32 mesh_id,
    f32 screenspace_width,
    f32 given_z);
f32 T1_objmodel_get_y_multiplier_for_height(
    s32 mesh_id,
    f32 screenspace_height,
    f32 given_z);

/*
TEXT LABELS

Set fields in T1_text_props before requesting a label to choose
the font properties etc. for your next label
*/
extern T1TextFontSettings * T1_text_props;

void T1_text_request_label_offset_around(
    s32 with_T1_id, const c8 * text,
    f32 mid_screen_x, f32 mid_screen_y, f32 z,
    f32 max_width);
void T1_text_request_label_leftx_toplinemidy(
    s32 with_T1_id, const c8 * text,
    f32 screen_left, f32 topline_mid_screen_y, f32 z,
    f32 max_width);
void T1_text_request_label_renderable(
    s32 with_T1_id,
    const c8 * text,
    f32 left_x_pixelspace,
    f32 top_y_pixelspace,
    f32 z,
    f32 tab_width,
    f32 max_width);
void T1_text_request_label_around_x_at_top_y(
    s32 with_T1_id, const c8 * text_to_draw,
    f32 screen_mid_x, f32 screen_top_y, f32 z,
    f32 max_width);
void T1_text_request_label_around(
    s32 with_T1_id, const c8 * text_to_draw,
    f32 screen_mid_x, f32 screen_mid_y, f32 z,
    f32 max_width);

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

For now this just has sliders

The *requester* setter functions are basically a list of
arguments, but they persist over repeated calls to
T1_ui_widget_request_slider(). So you can set your style
once and keep using it if you want 
*/
void T1_ui_widget_requester_set_pin_rgba(u8 rgba_i, f32 val);
void T1_ui_widget_requester_set_screenspace_height(u32 height);
void T1_ui_widget_requester_set_screenspace_width(u32 width);
void T1_ui_widget_requester_set_screenspace_pin_height(u32 height);
void T1_ui_widget_requester_set_screenspace_pin_width(u32 width);
void T1_ui_widget_requester_set_sfx_filename(c8 * sfx_fn);
void T1_ui_widget_requester_set_font_height(u32 to_val);
void T1_ui_widget_requester_set_screen_x(s32 x);
void T1_ui_widget_requester_set_screen_y(s32 y);
void T1_ui_widget_requester_set_z(f32 z);
void T1_ui_widget_requester_set_custom_minmax_f32(b8 active, f32 min, f32 max);
void T1_ui_widget_requester_set_linked_type(T1MetaType type);
void T1_ui_widget_request_slider(
    s32 background_T1_id, s32 label_T1_id, s32 pin_T1_id,
    void * linked_value_ptr);
void T1_ui_widget_delete(s32 T1_id);

/*
3D models
*/
f32 T1_get_x_mul_for_width(s32 for_mesh_id, f32 for_width);
f32 T1_get_y_mul_for_height(s32 for_mesh_id, f32 for_height);
f32 T1_get_z_mul_for_depth(s32 for_mesh_id, f32 for_depth);

/*
TexQuads (textured 2D quads)
*/
void T1_texquad_make(T1MakeRequest * request); // returns T1_id
void T1_texquad_delete(s32 T1_id);
void T1_texquad_delete_all(void);

/*
z-Lights (3D lights)
*/
T1zLight * T1_zlight_next(void);
void T1_zlight_commit(T1zLight * to_request);
void T1_zlight_delete(s32 T1_id);

/*
INPUTS FROM MOUSE, KEYBOARD, GAMEPAD

Short taps and long taps will disappear when you "consume"
them, but there may have been multiple short taps in a single
frame, so check if you care about that 

Short taps and long taps get cleared every frame, but a key
being down does not. If you don't consume a tap on the frame
when it ends, you will lose it
*/
s32  T1_io_create_scene_and_return_id(void);
void T1_io_scene_stack_push(const s32 scene_id);
void T1_io_scene_stack_pop(void);
b8   T1_io_key_is_down(T1IOKey key, s32 scene_id);
b8   T1_io_key_consume_tap_began_frame(T1IOKey key, s32 scene_id);
b8   T1_io_key_consume_short_tap_this_frame(T1IOKey key, s32 scene_id);
b8   T1_io_key_consume_long_tap_this_frame(T1IOKey key, s32 scene_id);
f32  T1_io_get_mouse_x_this_frame(void);
f32  T1_io_get_mouse_y_this_frame(void);
s32  T1_io_get_mouse_touch_id_this_frame(void);
s32  T1_io_create_scene_and_return_id(void);
b8   T1_io_consume_mouse_changed(const s32 scene_id);
b8   T1_io_consume_mouse_drag(f32 * delta_x, f32 * delta_y, const s32 scene_id);

/*
TOKENIZER
*/
void T1_token_reset(u8 * good);
#define T1_TOKEN_FLAG_IGNORE_CASE 1
#define T1_TOKEN_FLAG_SCIENTIFIC_OK 2
#define T1_TOKEN_FLAG_LEAD_DOT_OK 4
#define T1_TOKEN_FLAG_PRECISE 8
#define T1_TOKEN_FLAG_CONSUME_STOP_PATTERN 32
void T1_token_set_reg_bitflags(u8 bitflags);
void T1_token_clear_start_pattern(void);
void T1_token_set_reg_start_pattern(
    const c8 * start_pattern);
void T1_token_clear_stop_patterns(void);
void T1_token_set_store_mode(T1TokenStoreMode mode);
void T1_token_set_reg_middle_cap(u32 middle_cap);
void T1_token_set_reg_stop_pattern(
    const c8 * stop_pattern,
    u32 pattern_index);
void T1_token_set_string_literal(u32 enum_value, u8 * good);
void T1_token_register(u32 enum_value, u8 * good);
void T1_token_run(const c8 * input, u8 * good);
u32 T1_token_get_token_count(void);
u32 T1_token_get_enum_value(u16 token_i);
c8 * T1_token_get_string_value(u16 token_i);
u32 T1_token_get_string_value_size(u16 token_i);
u32 T1_token_get_line_num(u16 token_i);
b8 T1_token_is_number(s32 at_i);
b8 T1_token_fits_f64(s32 at_i);
b8 T1_token_fits_f32(s32 at_i);
b8 T1_token_fits_s64(s32 at_i);
b8 T1_token_fits_s32(s32 at_i);
b8 T1_token_fits_s16(s32 at_i);
b8 T1_token_fits_s8(s32 at_i);
b8 T1_token_fits_u64(s32 at_i);
b8 T1_token_fits_u32(s32 at_i);
b8 T1_token_fits_u16(s32 at_i);
b8 T1_token_fits_u8(s32 at_i);
u64 T1_token_as_number_unsigned(s32 at_i);
s64 T1_token_as_number_signed(s32 at_i);
f64 T1_token_as_number_floating(s32 at_i);

/*
META TYPES
*/
void T1_meta_get_offset_and_type(
    const c8 * struct_name,
    const c8 * field_name,
    s32 * out_offset,
    T1MetaType * out_data_type);
void T1_meta_write_to_known_field_uint(
    const c8 * target_parent_type,
    const c8 * target_field_name,
    u64 value_to_write_uint,
    void * target_parent_ptr,
    u8 * good);
void T1_meta_write_to_known_field_str(
    const c8 * target_parent_type,
    const c8 * target_field_name,
    const c8 * value_to_write_str,
    void * target_parent_ptr,
    b8 * good);

/*
OPERATING SYSTEM

The "res dir" (resources directory) is a special directory
that you're guaranteed to be allowed to read from, and
where your assets are presumably stored

The "writables" dir is a special directory where your app
is allowed to write, so you can store files with data about
your user's progress or preferences, etc.
*/
u64 T1_os_get_current_time_us(void);
void T1_os_toggle_fullscreen(void);
void T1_os_start_thread(void (*function_to_run)(s32), s32 argument);
u32 T1_os_init_mutex_and_return_id(void);
u8 T1_os_mutex_trylock(u32 mutex_id);
void T1_os_assert_mutex_locked(const u32 mutex_id);
void T1_os_mutex_lock(u32 mutex_id);
void T1_os_mutex_unlock(u32 mutex_id);
void T1_os_get_res_dir(c8 * recip, u32 recip_cap);
void T1_os_get_filenames_in(
    const c8 * directory, c8 filenames[2000][500]);
u8 T1_os_res_exists(const c8 * resource_name);
u64 T1_os_get_resource_size(const c8 * res_name);
void T1_os_read_resource_file(
    const c8 * filename,
    c8 * recip, u64 recip_cap,
    u8 * good);
void T1_os_get_app_dir(c8 * recip, u32 recip_size);
void T1_os_get_writables_dir(c8 * recip, u32 recip_size);
void T1_os_gpu_push_tex_slice_and_free_rgba(
    s32 tex_array_i,
    s32 tex_slice_i);
void T1_os_open_dir_in_file_explorer_window_if_possible(
    const c8 * folderpath);
u64
T1_os_get_current_time_us(void);

/*
AUDIO
*/
extern T1AudioSettingsFullyPublic * T1_audio_state;
void T1_wav_parse(
    s16 * recipient, u32 * recipient_size,
    u32 recip_cap, u8 * raw_file,
    u32 data_size, b8 * good);
void T1_audio_register_samples_to_permasound(
    s32 permasound_id,
    s16 * samples,
    s32 samples_size);
s32 T1_audio_get_permasound_id_or_register_new(
    const c8 * for_res_name);
void T1_audio_add_permasound_to_global_buffer(
    s32 permasound_id,
    f32 volume_f32);
void T1_audio_add_permasound_to_global_buffer_at_offset(
    s32 permasound_id,
    u64 play_cursor_offset,
    f32 volume_mult);
void T1_audio_copy_offset_permasound_to_global_buffer_at_offset(
    s32 permasound_id,
    u64 permasound_offset,
    u64 play_cursor_offset,
    u32 samples_to_copy_size,
    b8 is_music);
u64 T1_audio_get_play_cursor(void);
void T1_audio_clear_global_buffer(void);

#endif // T1_H
