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
APPLICATION STARTUP
Each step reports problems through fatal_error and does nothing if it's
already set, so you can call them in a row and check once at the end:

T1_init(&fatal_error);          // memory, subsystems, audio: no GPU
T1_gpu_init(&fatal_error);      // GPU device, T1's own textures
T1_gpu_try_shader_source(...);  // at least once successfully
T1_window_open(&fatal_error);

The GPU steps are optional. Without them, you draw your own pixels and
show them with T1_window_blit_rgba(), see examples/ for the smallest app.
If you use the GPU, call T1_gpu_init() before you create textures or
cameras. Textures (T1_tex_new) are uploaded as you create them, meshes and
materials when the next frame is rendered, so you can register either at
any time.

T1 never shows errors in the window. Problems at startup come back through
fatal_error; when T1 catches a bug (a failed check), it prints why and
crashes right away, so your debugger stops there.
*/
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
before T1_window_open(), else that's fatal.
*/
void T1_gpu_try_shader_source(
    const char * shader_source,
    uint8_t * good,
    char * error_recip,
    const uint32_t error_recip_cap,
    char ** const fatal_error);

// Opens the (one) window your frames are presented in. With the GPU, your
// first frame after this reports T1_frame_resized(), so you can build your
// scene there.
void T1_window_open(
    char ** const fatal_error);
void T1_window_toggle_fullscreen(void);

/*
MAIN LOOP
T1 doesn't run a loop of its own, your app does. Without the GPU:

while (!T1_window_close_requested()) {
    T1_window_poll_events();
    draw_your_pixels(rgba);
    T1_window_blit_rgba(rgba, width, height);
}
T1_shutdown();

With the GPU:

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
blocks at the display's refresh rate; when nothing was presented (also
after a blit), T1_window_poll_events() handles input until the next frame
is due, see T1_window_set_fps_cap(), so the loop doesn't spin.
*/
void     T1_window_poll_events(void);
// The window's close button, the OS's quit command, the terminal's QUIT
uint8_t  T1_window_close_requested(void);
// False while the window is minimized or fully covered
uint8_t  T1_window_visible(void);
// Copies the frame T1_frame_render() made to the window
void     T1_window_present(void);
// Shows width * height * 4 bytes of rgba (row by row, top row first, alpha
// ignored) as large as it fits in the window, keeping its aspect ratio, with
// black bands on the sides or on the top and bottom. Copies the pixels, so
// you can draw the next frame into the same buffer right away. Only for
// windows opened without T1_gpu_init().
void     T1_window_blit_rgba(
    const uint8_t * rgba,
    uint32_t width,
    uint32_t height);
// Caps loops that T1_window_present() doesn't pace (e.g. blitting ones) at
// fps frames per second, 60 by default. 0 removes the cap: the loop then
// runs as fast as your frames allow, using a whole CPU core.
void     T1_window_set_fps_cap(uint32_t fps);

// Waits until the GPU is done with the next frame's buffers and runs T1's
// work that comes before your update (animations, the terminal, ...)
void     T1_frame_begin(void);
// The window settled on a new size this frame. T1 keeps all your objects,
// move, rebuild or delete what depends on the size.
uint8_t  T1_frame_resized(void);
// False e.g. while T1 shows a crash message or a resize is settling
uint8_t  T1_frame_should_update(void);
uint64_t T1_frame_elapsed_us(void);
// Runs T1's work that comes after your update and renders the frame
// (without showing it, that's T1_window_present())
void     T1_frame_render(void);

// Saves the engine's state (window position etc.)
void     T1_shutdown(void);

/*
TERMINAL
Commands the T1 terminal doesn't know are passed to your handler, which
writes its reply to response.
*/
void T1_term_set_command_handler(
    void (* handler)(char * command, char * response, uint32_t response_cap));
#if T1_TERM_ACTIVE == T1_ACTIVE
void T1_term_manually_deactivate(void);
#elif T1_TERM_ACTIVE == T1_INACTIVE
#else
#error
#endif

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
uint32_t  T1_id_next_nonui_id(void);

/*
CAMERAS

Cameras are T1 objects like zsprites (see below): they have a T1_id, you set
and animate them by property name, merge them into groups and T1_delete()
them. Their properties: "xyz[0]" and "angle_xyz[0]" (to [2]) are where the
camera is now. It eases towards "dest_xyz[0]" and "dest_angle_xyz[0]" over
"us_to_destination" (u32) microseconds, kept within "min_xyz[0]"/"max_xyz[0]"
and "min_angle_xyz[0]"/"max_angle_xyz[0]". "clamped_to_T1_id" (u32) makes
it follow a zsprite, "movement_enabled" (u32) lets it move at all.
*/
// The camera whose image is shown in the window
uint32_t T1_cam_create_main_view(
    uint32_t new_w, uint32_t new_h, char ** const fatal_error);
// A camera mirroring the main one around the plane at z = reflection_z
uint32_t T1_cam_make_reflection_cam(
    uint32_t new_w, uint32_t new_h, float reflection_z);
void     T1_cam_reset(uint32_t T1_id);
// T1_TEX_NONE for T1_ID_NONE
T1Tex    T1_cam_get_write_tex(uint32_t T1_id);
void     T1_cam_delete_all(void);

// To convert from our screenspace system to 'world x' that is used for
// the position of zpolygons
float T1_screen_x_to_x(float screen_x, float given_z);
float T1_screen_y_to_y(float screen_y, float at_z);
float T1_screen_height_to_height(float screen_h, float at_z);
float T1_screen_width_to_width(float screen_w, float at_z);

void T1_make_shadowmap_and_attach_to_light(
    uint32_t T1_id, uint32_t w, uint32_t h);

/*
TEXTURES
T1_tex_new() uploads an image: width * height * 4 bytes of rgba, row by row.
The T1Tex can then be used as a "texture" property. You can delete it or
overwrite its pixels (same size) later.
*/
T1Tex    T1_tex_new(
    const uint8_t * rgba,
    uint32_t width,
    uint32_t height,
    char ** const fatal_error);
void     T1_tex_update(T1Tex tex, const uint8_t * rgba);
void     T1_tex_delete(T1Tex tex);
uint32_t T1_tex_get_width(T1Tex tex);
uint32_t T1_tex_get_height(T1Tex tex);

/*
MANAGE 3-D MODELS (.OBJ FILES)

Each "mesh" you register will return a mesh_id (i32)
*/
/*
Materials name their textures (map_Kd etc.), resolve_tex is asked for the
T1Tex of each such filename while a mesh is loaded
*/
void T1_objmodel_set_tex_resolver(
    T1Tex (* resolve_tex)(const char * filename));
int32_t T1_objmodel_new_mesh_id_from_resources(
    const char * filename,
    const char * mtl_filename,
    uint8_t flip_uv_u,
    uint8_t flip_uv_v,
    char ** const fatal_error);
int32_t T1_objmodel_resource_name_to_mesh_id(
    const char * obj_filename);
float T1_get_x_mul_for_width(int32_t for_mesh_id, float for_width);
float T1_get_y_mul_for_height(int32_t for_mesh_id, float for_height);
float T1_get_z_mul_for_depth(int32_t for_mesh_id, float for_depth);


/*
TEXT LABELS

Set text properties before requesting a label to choose the font height,
color etc. for your next labels: "font_height", "rgba[0]" to "rgba[3]",
"tab_width" (in widths of the letter 'D', 4 by default) (f32s), "touch_id",
"highlight_i", "highlight_size" and "opaque_back_active" (u32s).
Tabs only work with T1_TEXT_ALIGN_LEFT_TOP. Settings stay until you change
them, also for later labels.
*/
void T1_text_set_f32(
    const char * property,
    float value);
void T1_text_set_u32(
    const char * property,
    uint32_t value);

/*
A label of texquads with T1_id, wrapped to max_width. align says what
screen_x and screen_y are, e.g. T1_TEXT_ALIGN_CENTER puts the middle of
the text there.
*/
void T1_text_request_label(
    uint32_t T1_id,
    const char * text,
    T1TextAlign align,
    float screen_x,
    float screen_y,
    float z,
    float max_width);

// Draws text centered on (mid_x, mid_y) into an rgba image
void T1_text_draw_label_around(
    uint8_t * on_rgba, uint32_t rgba_w, uint32_t rgba_h,
    const char * text_to_draw,
    float mid_x_pixelspace,
    float mid_y_pixelspace,
    float max_width);

/*
POST-PROCESSING

Settings applied to the whole rendered image, by name: "rgb_add[0]" to
"rgb_add[2]", and if active "fog_color[0]" to "fog_color[2]", "fog_factor"
and "in_shadow_mults[0]" to "in_shadow_mults[2]"
*/
void  T1_postproc_set_f32(const char * property, float value);
float T1_postproc_get_f32(const char * property);

/*
Objects by T1_id (zsprites, texquads, zlights and cameras), with properties
set by name (e.g. "xyz[0]", "alpha"). Every new object gets a unique T1_id
and is visible from the next frame on, no commit needed. Merging makes
objects share a T1_id, so they move, fade and get deleted together: the
same concept has the same name on every kind of object.

"layers" (zsprites, texquads, zlights and text labels, default 1) and
"visible_layers" (cameras, default every bit) are bitmasks whose meaning is
up to you: a camera draws the objects and uses the lights whose layers share
a bit with its visible_layers. layers 0 hides an object from every camera.

zsprites' "mix_cam" takes a camera's T1_id: mix in that camera's image, use
its T1_cam_get_write_tex() as "mix_texture". T1_ID_NONE undoes it.
*/
uint32_t T1_zsprite_new(int32_t mesh_id);
uint32_t T1_zsprite_new_quad(
    float mid_x,
    float mid_y,
    float z,
    float width,
    float height);
uint32_t T1_texquad_new(void);
uint32_t T1_zlight_new(void);
void T1_set_f32(
    uint32_t T1_id,
    const char * property,
    float value);
void T1_set_u32(
    uint32_t T1_id,
    const char * property,
    uint32_t value);
void T1_merge_T1_ids(
    uint32_t T1_id_to_add,
    uint32_t T1_id_to_keep);
// Deletes every zsprite, texquad, zlight and camera with this T1_id
void T1_delete(uint32_t T1_id);
// Deletes every zsprite, texquad and zlight, cameras stay
void T1_delete_all(void);

/*
Animations affecting various T1 objects
*/
// Toggles a zsprite's "no_cam" (0 or 1) without moving it on screen
void T1_anim_set_ignore_camera_but_retain_screenspace_pos(
    uint32_t T1_id,
    float new_ignore_camera);
void T1_anim_delete_all_anims_targeting(uint32_t T1_id);
void T1_anim_delete_all(void);

/*
Animations by property name. mode is T1_ANIM_MODE_TO or T1_ANIM_MODE_BY.
The returned handle is only valid until it's committed. f32 properties
interpolate, u32 properties jump to their value at the end (TO mode only).
*/
uint32_t T1_anim_new(uint32_t T1_id, uint8_t mode);
void T1_anim_set_f32(
    uint32_t anim,
    const char * property,
    float value);
void T1_anim_set_u32(
    uint32_t anim,
    const char * property,
    uint32_t value);
void T1_anim_set_duration_us(uint32_t anim, uint64_t duration_us);
void T1_anim_set_pause_us(uint32_t anim, uint64_t pause_us);
// easing maps the anim's t (0 to 1) to how far along its values are, NULL
// (the default) is linear
void T1_anim_set_easing(uint32_t anim, float (* easing)(float t));
void T1_anim_set_runs(uint32_t anim, uint32_t runs); // 0 = forever
void T1_anim_set_delete_target_on_finish(
    uint32_t anim,
    uint8_t delete_target);
// On commit, deletes the TO anims on the same T1_id that animate any of
// this anim's properties
void T1_anim_set_cancel_conflicting(
    uint32_t anim,
    uint8_t cancel_conflicting);
// A duration of 1 microsecond and no pause applies the anim immediately
void T1_anim_commit(uint32_t anim);

/*
INPUTS FROM MOUSE, KEYBOARD, GAMEPAD

Short taps and long taps will disappear when you "consume"
them, but there may have been multiple short taps in a single
frame, so check if you care about that 

Short taps and long taps get cleared every frame, but a key
being down does not. If you don't consume a tap on the frame
when it ends, you will lose it
*/
uint8_t   T1_io_key_is_down(T1IOKey key);
uint8_t   T1_io_key_consume_tap_began_frame(T1IOKey key);
uint8_t   T1_io_key_consume_short_tap_this_frame(T1IOKey key);
uint8_t   T1_io_key_consume_long_tap_this_frame(T1IOKey key);
uint32_t  T1_io_get_mouse_touch_id_this_frame(void);
float     T1_io_get_pos_x_this_frame(T1IOKey key); 
float     T1_io_get_pos_y_this_frame(T1IOKey key);
uint8_t   T1_io_consume_mouse_drag(float * delta_x, float * delta_y);

/*
FATAL ERRORS

I consistently use char ** const fatal_error as a 
'first error encountered' tracker, a pattern I found and
like in the NextStep code on Apple's objective-c libs.

- My functions taking a 'fatal error' will crash if you
  pass NULL, it's not optional. You must declare a string
  of your own and pass an address to it
- My functions taking a 'fatal error' exit immediately
  if *fatal_error is already set (not 0)
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

***
Example code
***
char * fatal_error = 0;
do_dangerous_thing_1(&fatal_error);
do_dangerous_thing_2(&fatal_error);
    -> inside this:
    do_another_dangerous_thing(fatal_error);
do_dangerous_thing_3(&fatal_error);

if (fatal_error != 0) {
    return; // early exit
}

do_extra_things(); // happy path assured
***
End of example code
***
*/

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
void T1_os_open_dir_in_file_explorer_window_if_possible(
    const char * folderpath);

/*
AUDIO
T1 plays raw samples (16-bit stereo at 44.1kHz), decode your sound files
with whatever parser you like
*/
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
