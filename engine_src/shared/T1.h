#ifndef T1_H
#define T1_H

#include <stddef.h>
#include <stdint.h>

#include "T1_types_public.h"

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
DEBUG MODE
*/
#if T1_LOG_ASSERTS_ACTIVE == T1_ACTIVE
void T1_assert(uint8_t condition);
void T1_log_warn_if_false(uint8_t condition, const char * msg);
void T1_log_dump_and_crash(const char * crash_message);
#elif T1_LOG_ASSERTS_ACTIVE == T1_INACTIVE
#else
#define T1_assert(x)
#define T1_log_dump_and_crash(x)
#error
#endif

#if T1_LOG_PRINTF == T1_ACTIVE
void T1_log_append(const char *);
void T1_log_append_c8(char c8val);
void T1_log_append_f32(float f32val);
void T1_log_append_s32(int32_t s32val);
void T1_log_append_u32(uint32_t u32val);
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
PROFILER
*/
void T1_profiler_start(const char * func_name);
void T1_profiler_end(const char * func_name);

/*
T1_std
standard or commonly needed functions
*/
#define  T1_std_abs(a) ((((a) > 0)*(a))+(((a) < 0)*-(a)))
#define  T1_std_fabs(a) ((((a) > 0.0f)*(a))+(((a) < 0.0f)*-(a)))
int32_t  T1_std_mini(int32_t x, int32_t y);
int32_t  T1_std_maxi(int32_t x, int32_t y);
float    T1_std_minf(float x, float y);
float    T1_std_maxf(float x, float y);
void *   T1_std_memset(void * input, int32_t value, uint64_t size_bytes);
void     T1_std_memset_i16(void * input, int16_t value, uint32_t size_bytes);
void *   T1_std_memcpy(void * dest, const void * src, uint64_t n_bytes);
void     T1_std_strcpy_cap(char * recipient, uint32_t cap, const char * origin);
void     T1_std_strcat_cap(char * recip, uint32_t cap, const char * to_append);
void     T1_std_strcat_u32_cap(char * recip, uint32_t recip_size, uint32_t to_append);
void     T1_std_strcat_s32_cap(char * recip, uint32_t cap, int32_t to_append);
void     T1_std_strcat_f32_cap(char * recipient, uint32_t cap, float to_append);
void     T1_std_strcat_c8_cap(char * recipient, char to_append);
uint64_t T1_std_strlen(const char * nullterm_str);
uint8_t  T1_std_string_starts_with(const char * to_check, const char * start);
uint8_t  T1_std_are_equal_strings(const char * str1, const char * str2);
void     T1_std_strsub(char * in, const char * to_match, const char * repl);
void     T1_std_s32_to_string(int32_t input, char * recip);
void     T1_std_u32_to_string(uint32_t input, char * recipient);
uint32_t T1_std_string_to_u32_validate(const char * input, uint8_t * good);
int32_t  T1_std_string_to_s32(const char * input);
int32_t  T1_std_string_to_s32_validate(const char * input, uint8_t * good);
float    T1_std_string_to_f32_validate(const char * input, uint8_t * good);


/*
T1_mem - allocate memory
*/
void * T1_mem_malloc_unmanaged(size_t size);
void * T1_mem_malloc_managed(size_t size);
void T1_mem_free_managed(void * to_free);

/*
User settings
*/
uint32_t T1_settings_get_render_width(void);
uint32_t T1_settings_get_render_height(void);

/*
T1_id
*/
#define T1_ID_FPS_COUNTER 0
#define T1_ID_DEBUG_TEXT 1
#define T1_ID_MAX 8000
#define T1_ID_FIRST_NONUI 1011
#define T1_ID_LAST_UI_TOUCH 1000
#define T1_ID_FIRST_NONUI_TOUCH 1001
uint32_t  T1_id_next_ui_element_id(void);
uint32_t  T1_id_next_nonui_id(void);
uint32_t  T1_id_next_ui_element_touch_id(void);
void T1_id_clear_ui_element_touch_ids(void);
uint32_t  T1_id_next_nonui_touch_id(void);

/*
RANDOM NUMBERS
*/
#define T1_RAND_SEQUENCE_SIZE 999
int32_t  T1_rand(void);
int32_t  T1_rand_at_i(uint64_t index);
void T1_rand_shuf_array(void * array, uint32_t array_sz, uint32_t elem_sz);

/*
CAMERA MANIPULATION
*/
void  T1_cam_set_us_to_dest(int32_t cam_i, uint64_t us);
float   T1_cam_get_angle_xyz(int32_t cam_i, int32_t i);
void  T1_cam_set_dest_xyz(int32_t cam_i, int32_t i, float newval);
void  T1_cam_add_dest_xyz(int32_t cam_i, int32_t i, float newval);
void  T1_cam_set_dest_angle_xyz(int32_t cam_i, int32_t i, float newval);
void  T1_cam_add_dest_angle_xyz(int32_t cam_i, int32_t i, float val);
void  T1_cam_set_min_xyz(int32_t cam_i, int32_t i, float val);
void  T1_cam_set_max_xyz(int32_t cam_i, int32_t i, float val);
void  T1_cam_set_angle_xyz_min(int32_t cam_i, int32_t i, float val);
void  T1_cam_set_angle_xyz_max(int32_t cam_i, int32_t i, float val);
// to unclamp, clamp to T1_id = -1
void  T1_cam_set_clamped_to_T1_id(int32_t cam_i, uint32_t T1_id);
void  T1_cam_set_movement_enabled(int32_t cam_i, uint8_t newval);
void  T1_cam_delete(int32_t cam_i);
void  T1_cam_delete_all(void);
T1Tex T1_cam_get_write_tex(int32_t cam_i);
void  T1_cam_reset(int32_t at_i);

// To convert from our screenspace system to 'world x' that is used for
// the position of zpolygons
float T1_screen_x_to_x(float screen_x, float given_z);
float T1_screen_y_to_y(float screen_y, float at_z);
float T1_x_to_screen_x(float screen_x, float at_z);
float T1_y_to_screen_y(float y, float given_z);
float T1_screen_x_to_x_noz(float screen_x);
float T1_screen_y_to_y_noz(float screen_y);
float T1_x_to_screen_x_noz(float y);
float T1_y_to_screen_y_noz(float y);
float T1_screen_height_to_height(float screen_h, float at_z);
float T1_screen_width_to_width(float screen_w, float at_z);
float T1_screen_width_to_width_noz(float screen_w);
float T1_screen_height_to_height_noz(float screen_h);

void T1_make_shadowmap_and_attach_to_light(
    uint32_t T1_id, uint32_t w, uint32_t h);

void T1_cam_create_main_view(uint32_t new_w, uint32_t new_h);
void T1_make_reflection_cam(
    uint32_t new_w, uint32_t new_h, float reflection_z);

/*
FILE PARSING
*/
uint8_t * T1_png_malloc_managed_from_resource(
    const char * resource_name,
    uint32_t * out_width,
    uint32_t * out_height,
    uint8_t * out_good);

/*
MANAGE TEXTURES
*/
int16_t  T1_tex_to_array_i(T1Tex in); // (x == T1_TEX_NONE ? -1 : x >> 11)
int16_t  T1_tex_to_slice_i(T1Tex in); // (x == T1_TEX_NONE ? -1 : x & 0x07FF)
void T1_tex_files_prereg_png_res(const char * filename, uint8_t * good);
void T1_tex_files_prereg_dds_res(const char * filename, uint8_t * good);
void T1_tex_files_reg_new_by_splitting_file(
    const char * filename,
    uint32_t rows, uint32_t cols,
    uint8_t free_rgba);

uint16_t T1_tex_array_get_filename_loc(const char * for_filename);
uint16_t T1_tex_array_reg_img(
    const char * filename,
    uint32_t w, uint32_t h,
    uint8_t is_render_target, uint8_t use_bc1_compression);
void T1_tex_array_update_rgba(
    int32_t array_i, int32_t slice_i,
    const uint8_t * rgba, uint32_t rgba_size);
uint8_t T1_tex_array_tex_exists_and_is_not_deleted(T1Tex tex);
const uint8_t * T1_tex_array_get_const_rgba(
    int32_t array_i,
    int32_t slice_i);
uint32_t T1_tex_array_get_img_height(int32_t array_i);
uint32_t T1_tex_array_get_img_width(int32_t array_i);

/*
MANAGE 3-D MODELS (.OBJ FILES)

Each "mesh" you register will return a mesh_id (i32)
*/
int32_t T1_objmodel_new_mesh_id_from_resources(
    const char * filename,
    const char * mtl_filename,
    uint8_t flip_uv_u,
    uint8_t flip_uv_v,
    uint8_t * success,
    char * error_message);
int32_t T1_objmodel_resource_name_to_mesh_id(
    const char * obj_filename);
float T1_objmodel_get_x_multiplier_for_width(
    int32_t mesh_id,
    float screenspace_width,
    float given_z);
float T1_objmodel_get_y_multiplier_for_height(
    int32_t mesh_id,
    float screenspace_height,
    float given_z);

/*
TEXT LABELS

Set fields in T1_text_props before requesting a label to choose
the font properties etc. for your next label
*/
extern T1TextFontSettings * T1_text_props;

void T1_text_draw_label(
    uint8_t * on_rgba, uint32_t rgba_w, uint32_t rgba_h,
    const char * text_to_draw,
    float left_x_pixelspace, float top_y_pixelspace,
    float tab_width, float max_width);
void T1_text_request_label_offset_around(
    uint32_t with_T1_id, const char * text,
    float mid_screen_x, float mid_screen_y, float z,
    float max_width);
void T1_text_request_label_leftx_toplinemidy(
    uint32_t with_T1_id, const char * text,
    float screen_left, float topline_mid_screen_y, float z,
    float max_width);
void T1_text_request_label_renderable(
    uint32_t with_T1_id,
    const char * text,
    float left_x_pixelspace,
    float top_y_pixelspace,
    float z,
    float tab_width,
    float max_width);
void T1_text_request_label_around_x_at_top_y(
    uint32_t with_T1_id, const char * text_to_draw,
    float screen_mid_x, float screen_top_y, float z,
    float max_width);
void T1_text_request_label_around(
    uint32_t with_T1_id, const char * text_to_draw,
    float screen_mid_x, float screen_mid_y, float z,
    float max_width);

/*
UI WIDGETS

For now this just has sliders

The *requester* setter functions are basically a list of
arguments, but they persist over repeated calls to
T1_ui_widget_request_slider(). So you can set your style
once and keep using it if you want 
*/
void T1_ui_widget_requester_set_pin_rgba(uint8_t rgba_i, float val);
void T1_ui_widget_requester_set_screenspace_height(uint32_t height);
void T1_ui_widget_requester_set_screenspace_width(uint32_t width);
void T1_ui_widget_requester_set_screenspace_pin_height(uint32_t height);
void T1_ui_widget_requester_set_screenspace_pin_width(uint32_t width);
void T1_ui_widget_requester_set_sfx_filename(char * sfx_fn);
void T1_ui_widget_requester_set_font_height(uint32_t to_val);
void T1_ui_widget_requester_set_screen_x(int32_t x);
void T1_ui_widget_requester_set_screen_y(int32_t y);
void T1_ui_widget_requester_set_z(float z);
void T1_ui_widget_requester_set_custom_minmax_f32(uint8_t active, float min, float max);
void T1_ui_widget_requester_set_linked_type(T1MetaType type);
void T1_ui_widget_request_slider(
    uint32_t background_T1_id, uint32_t label_T1_id, uint32_t pin_T1_id,
    void * linked_value_ptr);
void T1_ui_widget_delete(uint32_t T1_id);

/*
3D models
*/
float T1_get_x_mul_for_width(int32_t for_mesh_id, float for_width);
float T1_get_y_mul_for_height(int32_t for_mesh_id, float for_height);
float T1_get_z_mul_for_depth(int32_t for_mesh_id, float for_depth);

/*
TexQuads (textured 2D quads)
*/
void T1_texquad_fetch_next(T1TexQuadRequest * stack_recip);
void T1_texquad_commit(T1TexQuadRequest * to_commit);
void T1_texquad_delete(uint32_t T1_id);
void T1_texquad_delete_all(void);

/*
zSprites (a 3D object with a mesh, T1_id, pos/angle etc.)
*/
void T1_zsprite_delete_all(void);
void T1_zsprite_fetch_next_noconstruct(
    T1zSpriteRequest * stack_recipient);
void T1_zsprite_construct_with_mesh_id(
    T1zSpriteRequest * to_construct,
    int32_t mesh_id);
void T1_zsprite_construct_quad_around(
    float mid_x,
    float mid_y,
    float z,
    float width,
    float height,
    T1zSpriteRequest * stack_recipient);
void T1_zsprite_commit(T1zSpriteRequest * to_commit);
void T1_zsprite_delete(uint32_t with_T1_id);
#if T1_OCCLUSION_ACTIVE == T1_ACTIVE
void T1_zsprite_set_occlusion(
    s32 T1_id,
    s32 new_visible_stat,
    u64 wait_before_invis_us);
#elif T1_OCCLUSION_ACTIVE == T1_INACTIVE
#define T1_zsprite_set_occlusion(a, b, c)
#else
#error
#endif
//#if 0
//void T1_zsprite_construct_quad(
//    float left_x, float bottom_y, float z,
//    float width, float height,
//    T1zSpriteRequest * stack_recipient);
//#endif

/*
z-Lights (3D lights)
*/
T1zLight * T1_zlight_next(void);
void T1_zlight_commit(T1zLight * to_request);
void T1_zlight_delete(uint32_t T1_id);
void T1_zlight_delete_all(void);

/*
T1Anim (animations affecting various T1 objects)
*/
T1Anim * T1_anim_request_next(
    uint8_t endpoints_not_deltas,
    uint8_t zs_gpu_f32s,
    uint8_t zs_cpu_f32s,
    uint8_t zs_gpu_u32s,
    uint8_t tq_gpu_f32s,
    uint8_t tq_gpu_u32s,
    uint8_t zl_gpu_f32s);
void T1_anim_commit(
    T1Anim * c
    #if T1_LOG_ASSERTS_ACTIVE == T1_ACTIVE
    ,const char * original_func_name
    #elif T1_LOG_ASSERTS_ACTIVE == T1_INACTIVE
    #else
    #error
    #endif
    );
void T1_anim_commit_and_instarun(
    T1Anim * to_commit
    #if T1_LOG_ASSERTS_ACTIVE == T1_ACTIVE
    ,const char * original_func_name
    #elif T1_LOG_ASSERTS_ACTIVE == T1_INACTIVE
    #else
    #error
    #endif
    );
void T1_anim_fade_destroy_all(
    uint64_t pause_first,
    uint64_t duration_us);
void T1_anim_bump(
    uint32_t T1_id,
    uint32_t wait);
void T1_anim_dud_dance(
    uint32_t T1_id,
    float magnitude);
void T1_anim_set_ignore_camera_but_retain_screenspace_pos(
    uint32_t T1_id,
    float new_ignore_camera);
void T1_anim_shatter_and_destroy(
    uint32_t T1_id,
    uint64_t duration_us);
void T1_anim_evaporate_and_destroy(
    uint32_t T1_id,
    uint64_t duration_us);
void T1_anim_fade_and_destroy(
    uint32_t T1_id,
    uint64_t pause_first,
    uint64_t duration_us);
void T1_anim_fade_to(
    uint32_t T1_id,
    uint64_t duration_us,
    float target_alpha);
void T1_anim_delete_all_anims_targeting(uint32_t T1_id);
void T1_anim_delete_all(void);

/*
INPUTS FROM MOUSE, KEYBOARD, GAMEPAD

Short taps and long taps will disappear when you "consume"
them, but there may have been multiple short taps in a single
frame, so check if you care about that 

Short taps and long taps get cleared every frame, but a key
being down does not. If you don't consume a tap on the frame
when it ends, you will lose it
*/
int32_t   T1_io_create_scene_and_return_id(void);
void      T1_io_scene_stack_push(int32_t T1_scene_id);
void      T1_io_scene_stack_pop(void);
int32_t   T1_io_scene_stack_get_active_scene_id(void);
uint8_t   T1_io_key_is_down(T1IOKey key, int32_t T1_scene_id);
uint8_t   T1_io_key_consume_tap_began_frame(T1IOKey key, int32_t T1_scene_id);
uint8_t   T1_io_key_consume_short_tap_this_frame(T1IOKey key, int32_t T1_scene_id);
uint8_t   T1_io_key_consume_long_tap_this_frame(T1IOKey key, int32_t T1_scene_id);
uint32_t  T1_io_get_mouse_touch_id_this_frame(void);
float     T1_io_get_pos_x_this_frame(T1IOKey key); 
float     T1_io_get_pos_y_this_frame(T1IOKey key);
int32_t   T1_io_create_scene_and_return_id(void);
uint8_t   T1_io_consume_mouse_drag(float * delta_x, float * delta_y, int32_t T1_scene_id);

/*
TOKENIZER
*/
void T1_token_reset(uint8_t * good);
#define T1_TOKEN_FLAG_IGNORE_CASE 1
#define T1_TOKEN_FLAG_SCIENTIFIC_OK 2
#define T1_TOKEN_FLAG_LEAD_DOT_OK 4
#define T1_TOKEN_FLAG_PRECISE 8
#define T1_TOKEN_FLAG_CONSUME_STOP_PATTERN 32
void T1_token_set_reg_bitflags(uint8_t bitflags);
void T1_token_clear_start_pattern(void);
void T1_token_set_reg_start_pattern(
    const char * start_pattern);
void T1_token_clear_stop_patterns(void);
void T1_token_set_store_mode(T1TokenStoreMode mode);
void T1_token_set_reg_middle_cap(uint32_t middle_cap);
void T1_token_set_reg_stop_pattern(
    const char * stop_pattern,
    uint32_t pattern_index);
void T1_token_set_string_literal(uint32_t enum_val, uint8_t * good);
void T1_token_register(uint32_t enum_value, uint8_t * good);
void T1_token_run(const char * input, uint8_t * good);
uint32_t T1_token_get_token_count(void);
uint32_t T1_token_get_enum_value(uint16_t token_i);
void T1_token_overwrite_enum_val(uint16_t token_i, uint32_t new_enum_val);
char * T1_token_get_string_value(uint16_t token_i);
uint32_t T1_token_get_string_value_size(uint16_t token_i);
uint32_t T1_token_get_line_num(uint16_t token_i);
uint8_t T1_token_is_number(int32_t at_i);
uint8_t T1_token_fits_f64(int32_t at_i);
uint8_t T1_token_fits_f32(int32_t at_i);
uint8_t T1_token_fits_s64(int32_t at_i);
uint8_t T1_token_fits_s32(int32_t at_i);
uint8_t T1_token_fits_s16(int32_t at_i);
uint8_t T1_token_fits_s8(int32_t at_i);
uint8_t T1_token_fits_u64(int32_t at_i);
uint8_t T1_token_fits_u32(int32_t at_i);
uint8_t T1_token_fits_u16(int32_t at_i);
uint8_t T1_token_fits_u8(int32_t at_i);
uint64_t T1_token_as_number_unsigned(int32_t at_i);
int64_t T1_token_as_number_signed(int32_t at_i);
double T1_token_as_number_floating(int32_t at_i);

/*
META TYPES (registration)
*/
#define T1_meta_struct(struct_name, good) T1_meta_reg_struct(#struct_name, sizeof(struct_name), good)
void T1_meta_reg_struct(
    const char * struct_name,
    const uint32_t size_bytes,
    uint8_t * good);
#define T1_meta_field(parent_type_name, field_T1_type, field_name, good) T1_meta_reg_field(#field_name, offsetof(parent_type_name, field_name), field_T1_type, NULL, 1, 1, 1, 0, good)
#define T1_meta_enum_field(parent_type_name, enum_name, field_T1_type, field_name, good) T1_meta_reg_field(#field_name, offsetof(parent_type_name, field_name), field_T1_type, #enum_name, 1, 1, 1, 1, good)
#define T1_meta_enum_array(parent_type_name, field_enum_name, field_T1_type, field_name, array_size, good) T1_meta_reg_field(#field_name, offsetof(parent_type_name, field_name), field_T1_type, #field_enum_name, array_size, 1, 1, 1, good)
#define T1_meta_struct_field(parent_type_name, field_type_or_NULL, field_name, good) T1_meta_reg_field(#field_name, offsetof(parent_type_name, field_name), T1_TYPE_STRUCT, #field_type_or_NULL, 1, 1, 1, 0, good)
#define T1_meta_array(parent_type_name, field_T1_type, field_name, array_size, good) T1_meta_reg_field(#field_name, offsetof(parent_type_name, field_name), field_T1_type, NULL, array_size, 1, 1, 0, good)
#define T1_meta_struct_array(parent_type_name, field_type_or_NULL, field_name, array_size, good) T1_meta_reg_field(#field_name, offsetof(parent_type_name, field_name), T1_TYPE_STRUCT, #field_type_or_NULL, array_size, 1, 1, 0, good)
#define T1_meta_multi_array(parent_type_name, field_T1_type, field_struct_type_or_NULL, field_name, array_size_1, array_size_2, array_size_3, good) T1_meta_reg_field(#field_name, offsetof(parent_type_name, field_name), field_T1_type, #field_struct_type_or_NULL, array_size_1, array_size_2, array_size_3, 0, good)
void T1_meta_reg_field(
    const char * field_name,
    uint32_t field_offset,
    T1MetaType field_type,
    const char * field_struct_type_name_or_null,
    uint16_t field_array_size_1,
    uint16_t field_array_size_2,
    uint16_t field_array_size_3,
    uint8_t is_enum,
    uint8_t * good);
#define T1_meta_enum(enum_type_name, T1_data_type, good) T1_meta_reg_enum(#enum_type_name, T1_data_type, sizeof(enum_type_name), good)
void T1_meta_reg_enum(
    const char * enum_type_name,
    const T1MetaType T1_type,
    const uint32_t type_size_check,
    uint8_t * good);
#define T1_meta_enum_value(enum_type_name, enum_value, good) T1_meta_reg_enum_value(#enum_type_name, #enum_value, enum_value, good)
void T1_meta_reg_enum_value(
    const char * enum_type_name,
    const char * value_name,
    int64_t value,
    uint8_t * good);
void
T1_meta_reg_u4_subname_for_last_field(
    const char * subname,
    const char * enum_name_if_any,
    uint8_t is_right_nibble,
    uint8_t * good);

/*
META TYPES (querying)
*/
void T1_meta_get_offset_and_type(
    const char * struct_name,
    const char * field_name,
    int32_t * out_offset,
    T1MetaType * out_data_type);
void T1_meta_write_to_known_field_uint(
    const char * target_parent_type,
    const char * target_field_name,
    uint64_t value_to_write_uint,
    void * target_parent_ptr,
    uint8_t * good);
void T1_meta_write_to_known_field_str(
    const char * target_parent_type,
    const char * target_field_name,
    const char * value_to_write_str,
    void * target_parent_ptr,
    uint8_t * good);

/*
OPERATING SYSTEM

The "res dir" (resources directory) is a special directory
that you're guaranteed to be allowed to read from, and
where your assets are presumably stored

The "writables" dir is a special directory where your app
is allowed to write, so you can store files with data about
your user's progress or preferences, etc.
*/
uint64_t T1_os_get_current_time_us(void);
void T1_os_toggle_fullscreen(void);
void T1_os_start_thread(void (*func_to_run)(int32_t), int32_t argument);
uint32_t T1_os_init_mutex_and_return_id(void);
uint8_t T1_os_mutex_trylock(uint32_t mutex_id);
void T1_os_assert_mutex_locked(uint32_t mutex_id);
void T1_os_mutex_lock(uint32_t mutex_id);
void T1_os_mutex_unlock(uint32_t mutex_id);
void T1_os_get_res_dir(char * recip, uint32_t recip_cap);
void T1_os_get_filenames_in(
    const char * directory, char filenames[2000][500]);
uint8_t T1_os_res_exists(const char * resource_name);
uint8_t T1_os_file_exists(const char * filepath);
void T1_os_del_file(const char * filepath);
void T1_os_copy_file(
    const char * filepath_src, const char * filepath_dest);
uint64_t T1_os_get_filesize(const char * filepath);
uint64_t T1_os_get_resource_size(const char * res_name);
void T1_os_read_file(const char * filepath,
    char * recip, uint32_t * recip_size, uint64_t recip_cap, uint8_t * good);
void T1_os_read_resource_file(
    const char * filen, char * recip, uint64_t recip_cap, uint8_t * good);
void T1_os_get_dir_separator(char * recip);
uint32_t T1_os_get_dir_separator_size(void);
void T1_os_writable_filename_to_pathfile(
    const char * filen, char * recip, uint32_t recip_cap);
void T1_os_res_filename_to_pathfile(
    const char * filen, char * recip, uint32_t recip_cap);
void T1_os_get_app_dir(char * recip, uint32_t recip_size);
void T1_os_get_writables_dir(char * recip, uint32_t recip_size);
void T1_os_write_file_to_writables(
    const char * filepath_in_writables, const char * out,
    uint32_t output_size, uint8_t * good);
void T1_os_gpu_push_tex_slice(
    int32_t tex_array_i,
    int32_t tex_slice_i,
    uint8_t free_rgba);
void T1_os_open_dir_in_file_explorer_window_if_possible(
    const char * folderpath);
uint64_t T1_os_get_current_time_us(void);

/*
TERMINAL
*/
void T1_term_manually_deactivate(void);

/*
AUDIO
*/
extern T1AudioSettingsFullyPublic * T1_audio_state;
void T1_wav_parse(
    int16_t * recipient, uint32_t * recipient_size,
    uint32_t recip_cap, uint8_t * raw_file,
    uint32_t data_size, uint8_t * good);
void T1_audio_register_samples_to_permasound(
    int32_t permasound_id,
    int16_t * samples,
    int32_t samples_size);
int32_t T1_audio_get_permasound_id_or_register_new(
    const char * for_res_name);
void T1_audio_add_permasound_to_global_buffer(
    int32_t permasound_id,
    float volume_f32);
void T1_audio_add_permasound_to_global_buffer_at_offset(
    int32_t permasound_id,
    uint64_t play_cursor_offset,
    float volume_mult);
void T1_audio_copy_offset_permasound_to_global_buffer_at_offset(
    int32_t  permasound_id,
    uint64_t permasound_offset,
    uint64_t play_cursor_offset,
    uint32_t samples_to_copy_size,
    uint8_t  is_music);
uint64_t T1_audio_get_play_cursor(void);
void T1_audio_clear_global_buffer(void);

#endif // T1_H
