#ifndef T1_H
#define T1_H

#include <stdint.h>
#include <stddef.h>

#include "T1_macro_settings.h"
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
#define T1_assert(x)
#define T1_log_dump_and_crash(x)
#else
#error
#endif

/*
APPLICATION STARTUP
Each step reports problems through fatal_error and does nothing if it's
already set, so you can call them in a row and check once at the end:

T1_init(&fatal_error);          // memory, subsystems: no window or GPU yet
T1_gpu_init(&fatal_error);      // GPU device, T1's own textures
T1_gpu_try_shader_source(...);  // at least once successfully
T1_window_open(&fatal_error);   // any time after T1_gpu_init()
... preregister your textures and meshes ...
T1_assets_load_all(&fatal_error);
*/
uint8_t T1_running(void);

void T1_init(
    char ** const fatal_error);
void T1_gpu_init(
    char ** const fatal_error);

/*
Compiles shader_source (with T1's GPU struct headers
prepended) and builds all render pipelines from it.
If anything fails, *good is set to 0, a description is
copied to error_recip (optional, may be NULL, else
needs a cap of at least 512), and any
shaders loaded by a previous successful call stay in
place, so you can retry with a different source.
Needs T1_gpu_init(). You must succeed at least once
before T1_assets_load_all(), else that's fatal.
*/
void T1_gpu_try_shader_source(
    const char * shader_source,
    uint8_t * good,
    char * error_recip,
    const uint32_t error_recip_cap,
    char ** const fatal_error);

// Opens the (one) window your frames are presented in
void T1_window_open(
    char ** const fatal_error);
void T1_window_toggle_fullscreen(void);

// Decodes and uploads all preregistered textures, meshes and materials.
// Blocks until done.
void T1_assets_load_all(
    char ** const fatal_error);

/*
MAIN LOOP
T1 doesn't run a loop of its own, your app does, e.g.:

while (!T1_window_close_requested()) {
    T1_window_poll_events();
    if (!T1_window_visible()) { continue; }
    T1_frame_begin();
    if (T1_frame_resized()) { rebuild what depends on the window size }
    if (T1_frame_should_update()) { update(T1_frame_elapsed_us()); }
    T1_frame_render();
    T1_window_present();
}
T1_shutdown();

T1_window_poll_events() must be called from the main thread. Presenting
blocks at the display's refresh rate; when nothing was presented,
T1_window_poll_events() waits briefly for input instead, so the loop never
spins.
*/
void     T1_window_poll_events(void);
// The window's close button, the OS's quit command, the terminal's QUIT
uint8_t  T1_window_close_requested(void);
// False while the window is minimized or fully covered
uint8_t  T1_window_visible(void);
// Copies the frame T1_frame_render() made to the window
void     T1_window_present(void);

// Waits until the GPU is done with the next frame's buffers and runs T1's
// work that comes before your update (animations, the terminal, ...)
void     T1_frame_begin(void);
// The window settled on a new size this frame
uint8_t  T1_frame_resized(void);
// False e.g. while T1 shows a crash message or a resize is settling
uint8_t  T1_frame_should_update(void);
uint64_t T1_frame_elapsed_us(void);
// Runs T1's work that comes after your update and renders the frame
// (without showing it, that's T1_window_present())
void     T1_frame_render(void);

// Saves the engine's state (window position etc.) and writes the log
void     T1_shutdown(void);

/*
TERMINAL
Commands the T1 terminal doesn't know are passed to your handler, which
writes its reply to response.
*/
void T1_term_set_command_handler(
    void (* handler)(char * command, char * response, uint32_t response_cap));

/*
PROFILER
*/
void T1_profiler_start(const char * func_name);
void T1_profiler_end(const char * func_name);

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
#define T1_ID_MAX 8000
#define T1_ID_FIRST_NONUI_TOUCH 1001
uint32_t  T1_id_next_ui_element_id(void);
uint32_t  T1_id_next_nonui_id(void);

/*
CAMERA MANIPULATION
*/
void  T1_cam_set_us_to_dest(int32_t cam_i, uint64_t us);
float T1_cam_get_angle_xyz(int32_t cam_i, int32_t i);
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
void  T1_cam_delete_all(void);
T1Tex T1_cam_get_write_tex(int32_t cam_i);
void  T1_cam_reset(int32_t at_i);

// To convert from our screenspace system to 'world x' that is used for
// the position of zpolygons
float T1_screen_x_to_x(float screen_x, float given_z);
float T1_screen_y_to_y(float screen_y, float at_z);
float T1_screen_x_to_x_noz(float screen_x);
float T1_screen_y_to_y_noz(float screen_y);
float T1_screen_height_to_height(float screen_h, float at_z);
float T1_screen_width_to_width(float screen_w, float at_z);
float T1_screen_width_to_width_noz(float screen_w);
float T1_screen_height_to_height_noz(float screen_h);

void T1_make_shadowmap_and_attach_to_light(
    uint32_t T1_id, uint32_t w, uint32_t h);

void T1_cam_create_main_view(
    uint32_t new_w, uint32_t new_h, char ** const fatal_error);
void T1_make_reflection_cam(
    uint32_t new_w, uint32_t new_h, float reflection_z);

/*
FILE PARSING
*/
uint8_t * T1_png_malloc_managed_from_resource(
    const char * resource_name,
    uint32_t * out_width,
    uint32_t * out_height,
    char ** const fatal_error);

/*
MANAGE TEXTURES
*/
int16_t  T1_tex_to_array_i(T1Tex in); // (x == T1_TEX_NONE ? -1 : x >> 11)
int16_t  T1_tex_to_slice_i(T1Tex in); // (x == T1_TEX_NONE ? -1 : x & 0x07FF)
void T1_tex_files_prereg_png_res(const char * filename, uint8_t never_delete, char ** const fatal_error);
void T1_tex_files_prereg_dds_res(const char * filename, char ** const fatal_error);
void T1_tex_files_reg_new_by_splitting_file(
    const char * filename,
    uint32_t rows, uint32_t cols,
    uint8_t free_rgba,
    char ** const fatal_error);

uint16_t T1_tex_array_get_filename_loc(const char * for_filename);
uint16_t T1_tex_array_prereg_img(
    const char * filename,
    uint32_t w, uint32_t h,
    uint8_t is_render_target,
    uint8_t use_bc1_compression,
    uint8_t never_delete,
    char ** const fatal_error);
void T1_tex_array_update_rgba(
    int32_t array_i, int32_t slice_i,
    const uint8_t * rgba, uint32_t rgba_size);
uint8_t T1_tex_array_tex_exists_and_is_not_deleted(T1Tex tex);
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
    char ** const fatal_error);
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

void T1_text_draw_label_around(
    uint8_t * on_rgba, uint32_t rgba_w, uint32_t rgba_h,
    const char * text_to_draw,
    float mid_x_pixelspace,
    float mid_y_pixelspace,
    float max_width);
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
void T1_ui_widget_requester_set_linked_type(T1DataType type);
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
    T1Anim * c);
void T1_anim_commit_and_instarun(
    T1Anim * to_commit);
void T1_anim_fade_destroy_all(
    uint64_t pause_first,
    uint64_t duration_us);
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
FATAL ERRORS

I consistently use char ** const fatal_error as a 
'first error encountered' tracker, a pattern I found and
like in the NextStep code on Apple's objective-c libs.

- My functions taking a 'fatal error' will crash if you
  pass NULL, it's not optional. You must declare a string
  of your own and pass an address to it
- My functions taking a 'fatal error' exit immediately
  if *fatal_error is 0
- That means you must initialize them to 0 (NULL)! Else
  all T1 functions will just return
- My functions refuse to overwrite an existing 'fatal
  error', they only prepend info the string. So you
  always have access to the first thing that went wrong,
  even if you only check after many functions
- Fatal errors are always fatal, the entire library will
  become unusable when one fires. Non-fatal failures will
  be signaled by other variables
- Fatal errors are owned by the library, never free them

These 2 functions are helpers, both allocating memory
for a new error string. You don't need to use them if
you don't like this error system, it's optional

T1_fatal_error_new(my_message) is equivalent to this
*fatal_error = malloc(512);
*fatal_error[0] = '\0';
strcat(*fatal_error, my_message);

T1_fatal_error_prepend_if_bad() is for when you want
to add additional text to the start of an error if there,
is one, usually because of a subprocess that may have
gone wrong. For example, if you have a filename, pass
data to a parser, and the parser sets the fatal error,
it probably didn't have the filename where the error
happened. You can then prepend that information so it
gets passed up. It just copies the data to a new string,
leaking the old one.

***
Example code
***
char * fatal_error = 0;
do_dangerous_thing_1(&fatal_error);
do_dangerous_thing_2(&fatal_error);
    -> inside this:
    do_another_dangerous_thing(fatal_error);
do_dangerous_thing_3(&fatal_error);

if (*fatal_error != 0) {
    T1_fatal_error_prepend_if_bad(
        fatal_error,
        "While handling foo: ");
    return; // early exit
}

do_extra_things(); // happy path assured
***
End of example code
***
*/
void T1_fatal_error_new(
    char ** const fatal_error,
    const char * with_string);

void T1_fatal_error_prepend_if_bad(
    char ** const fatal_error,
    const char * to_append);

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
void T1_os_start_thread(void *(*function_to_run)(void *), void * arg_on_heap);
uint32_t T1_os_init_mutex_and_return_id(void);
uint8_t T1_os_mutex_trylock(uint32_t mutex_id);
void T1_os_mutex_lock(uint32_t mutex_id);
void T1_os_mutex_unlock(uint32_t mutex_id);
void T1_os_get_res_dir(char * recip, uint32_t recip_cap);

void T1_os_get_filenames_in(
    const char * directory,
    char * filenames,
    uint32_t filenames_cap);

uint8_t T1_os_writable_exists(const char * resource_name);
uint8_t T1_os_res_exists(const char * resource_name);
uint8_t T1_os_file_exists(const char * filepath);
void T1_os_del_file(const char * filepath);
void T1_os_copy_file(
    const char * filepath_src, const char * filepath_dest);
uint64_t T1_os_get_writable_size(const char * filename);
uint64_t T1_os_get_resource_size(const char * res_name);
void T1_os_read_resource_file(
    const char * filename,
    char * recip, uint64_t recip_cap,
    char ** const fatal_error);
void T1_os_read_writable_file(
    const char * filename,
    char * recip, uint64_t recip_cap,
    char ** const fatal_error);
void T1_os_writable_filename_to_pathfile(
    const char * filen, char * recip, uint32_t recip_cap);
void T1_os_res_filename_to_pathfile(
    const char * filen, char * recip, uint32_t recip_cap);
void T1_os_get_writables_dir(char * recip, uint32_t recip_size);
void T1_os_write_file_to_writables(
    const char * filepath_in_writables, const char * out,
    uint32_t output_size, uint8_t * good);
void T1_os_gpu_push_tex_slice(
    int32_t tex_array_i,
    int32_t tex_slice_i);
void T1_os_open_dir_in_file_explorer_window_if_possible(
    const char * folderpath);
uint64_t T1_os_get_current_time_us(void);

/*
TERMINAL
*/
#if T1_TERM_ACTIVE == T1_ACTIVE
void T1_term_manually_deactivate(void);
#elif T1_TERM_ACTIVE == T1_INACTIVE
#else
#error
#endif

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
    const char * for_res_name,
    char ** const fatal_error);
void T1_audio_add_permasound_to_global_buffer(
    int32_t permasound_id,
    float volume_f32);
void T1_audio_copy_offset_permasound_to_global_buffer_at_offset(
    int32_t  permasound_id,
    uint64_t permasound_offset,
    uint64_t play_cursor_offset,
    uint32_t samples_to_copy_size,
    uint8_t  is_music);
uint64_t T1_audio_get_play_cursor(void);
void T1_audio_clear_global_buffer(void);

#endif // T1_H
