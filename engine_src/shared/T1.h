#ifndef T1_H
#define T1_H

/*
Search for documentation by section:
  APPLICATION STARTUP, FATAL ERRORS, MAIN LOOP, INPUT,
  OBJECTS AND PROPERTIES, ANIMATIONS, CAMERAS, TEXT LABELS, TEXTURES,
  RENDERING, BUFFERS, 3-D MODELS, FILES, OPERATING SYSTEM, AUDIO

The macro settings at the top are yours to edit.
*/

#include <stdint.h>
#include <stddef.h>

#include "T1_macro_settings.h"
#include "T1_types_public.h"

/*
APPLICATION STARTUP
Each step reports problems through fatal_error and does nothing if it's
already set, so you can call them in a row and check once at the end:

T1_init(malloc, malloc, free, &fatal_error); // no GPU yet
T1_gpu_init(&fatal_error);     // GPU device, T1's own textures
T1_gpu_try_shader_source(...); // at least once successfully
T1_window_open(&fatal_error);

The GPU steps are optional. Without them, you draw your own pixels and
show them with T1_window_blit_rgba(), see examples/ for the smallest app,
and T1 doesn't allocate anything for rendering. If you use the GPU, call
T1_gpu_init() before you use anything that renders: textures, buffers,
cameras, zsprites, texquads, lights, text, passes or animations.
Textures (T1_tex_new) are uploaded as you create them, meshes and materials
when the next frame is rendered, so you can register either at any time.

T1 never shows errors in the window. Problems at startup come back through
fatal_error; when T1 catches a bug (a failed check, see T1_ASSERTS_ACTIVE),
it crashes right away, so your debugger stops there.
*/
/*
T1 allocates all its memory through your functions, they only need to align
like malloc(). alloc_forever's memory is never freed, so it can be a bump
allocator in a block of yours: then you know exactly what T1 took and the
rest stays yours. alloc_temp's memory is given back with free_temp.
*/
void T1_init(
    void * (* alloc_forever)(size_t size),
    void * (* alloc_temp)(size_t size),
    void   (* free_temp)(void * ptr),
    char ** const fatal_error);
void T1_gpu_init(
    char ** const fatal_error);

/*
Compiles shader_source, with T1's macro settings and GPU struct headers put
in front of it, and builds your pipelines from it. If anything fails, *good
is 0, error_recip (optional, may be NULL, else needs a cap of at least 512)
gets a description, and the shaders of an earlier successful call stay, so
you can retry with a different source. Needs T1_gpu_init(). It must succeed
once before T1_window_open(), else that's fatal.
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
FATAL ERRORS
Functions that can fail fatally take char ** const fatal_error:
- declare char * fatal_error = NULL and pass &fatal_error, never NULL
- they do nothing if *fatal_error is already set, so you can call several
  in a row and check once afterwards
- the first error stays, later ones don't overwrite it; T1 owns the string
- after a fatal error, T1 is unusable
*/

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
// The window's close button or the OS's quit command
uint8_t  T1_window_close_requested(void);
// False while the window is minimized or fully covered
uint8_t  T1_window_visible(void);
// The window's size in points, the unit touches and the mouse use
uint32_t T1_window_get_width(void);
uint32_t T1_window_get_height(void);
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
// work that comes before your update (animations, ...)
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
INPUT (KEYBOARD, MOUSE, GAMEPAD)
Keyboard keys, mouse buttons and gamepad buttons are all T1IOKeys. A key
being down lasts, but taps are cleared every frame: consume them on the
frame they end or lose them (and there may have been several in a frame).
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
OBJECTS AND PROPERTIES
These are all for GPU rendering
Objects by T1_id (zsprites, texquads, zlights and cameras), with properties
set by name (e.g. "xyz[0]", "alpha"). Every new object gets a unique T1_id
and is visible from the next frame on. Merging makes objects share a T1_id,
so they react to the same animations (e.g. move together) the same concept
has the same name on every kind of object to facilitate that.

"layers" (zsprites, texquads, zlights and text labels, default 1) and
"visible_layers" (cameras, default every bit) are bitmasks whose meaning is
up to you: a camera draws the objects and uses the lights whose layers share
a bit with its visible_layers. layers 0 hides an object from every camera.

zsprites' "mix_cam" takes a camera's T1_id: mix in the image (that you drew
from) that camera, its T1Tex is "mix_texture". T1_ID_NONE undoes it.

zlights' "shadow_cam" takes a camera's T1_id, which then follows the light;
your shaders pack its depth into the light's "shadow_tex" (a T1Tex).
T1_ID_NONE for no shadows.
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
/*
Gives everything with T1_id_to_add the T1_id T1_id_to_keep. T1_id_to_add is
recycled for later objects, so stop using it.
*/
void T1_merge_T1_ids(
    uint32_t T1_id_to_add,
    uint32_t T1_id_to_keep);
// Deletes every zsprite, texquad, zlight and camera with this T1_id
void T1_delete(uint32_t T1_id);
// Deletes every zsprite, texquad and zlight, cameras and passes stay
void T1_delete_all(void);

// T1_ids
#define T1_ID_FPS_COUNTER 0
#define T1_ID_MAX 8000
#define T1_ID_FIRST_NONUI_TOUCH 1001
uint32_t  T1_id_next_nonui_id(void);

/*
ANIMATIONS
*/
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
CAMERAS

A camera is a viewpoint of a size, which draw passes (see RENDERING) draw
from. Every camera has its own depth buffer at its size, the camera's first
draw pass of the frame clears it.

Cameras are T1 objects like zsprites (see below): they have a T1_id, you set
and animate them by property name, merge them into groups and T1_delete()
them. Their properties: "xyz[0]" and "angle_xyz[0]" (to [2]) are where the
camera is now. It eases towards "dest_xyz[0]" and "dest_angle_xyz[0]" over
"us_to_destination" (u32) microseconds, kept within "min_xyz[0]"/"max_xyz[0]"
and "min_angle_xyz[0]"/"max_angle_xyz[0]". "clamped_to_T1_id" (u32) makes
it follow a zsprite, "movement_enabled" (u32) lets it move at all.
"reflect_cam" (u32, a camera's T1_id) makes it mirror that camera around
the plane at z = "reflect_plane_z", T1_ID_NONE stops that.
*/
uint32_t T1_cam_new(uint32_t width, uint32_t height);
void     T1_cam_reset(uint32_t T1_id);
void     T1_cam_delete_all(void);

// From pixels on cam's screen (its width * height, top left is 0, 0) to
// the world units at z that zsprites are positioned in
float T1_screen_x_to_x(uint32_t cam_T1_id, float screen_x, float at_z);
float T1_screen_y_to_y(uint32_t cam_T1_id, float screen_y, float at_z);
float T1_screen_height_to_height(
    uint32_t cam_T1_id, float screen_h, float at_z);
float T1_screen_width_to_width(
    uint32_t cam_T1_id, float screen_w, float at_z);

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
TEXTURES
T1_tex_new() uploads an image: width * height * 4 bytes of rgba, row by row.
With rgba NULL the texture starts out blank, e.g. for passes to draw into.
The T1Tex can then be used as a "texture" property and by passes. You can
delete it or overwrite its pixels (same size) later. Every texture can be
sampled, drawn into and written by compute passes. Data that isn't a color
(depth, touch ids) is packed into the 4 bytes of a pixel by your shaders.
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
RENDERING

A frame is your list of passes, run in the order you made them. Passes and
pipelines are T1 objects without animations: T1_delete() them, they can't
be merged or animated.

A pipeline is a function of your shaders (by name) plus the state that
can't change afterwards: for drawing a vertex and a fragment function, how
color(0) blends (T1_BLEND_*) and which triangles to skip (T1_CULL_*). *good
is 0 if a function doesn't exist in the loaded shaders (names are checked
when the first shaders load if there are none yet).

Passes:
- A clear pass fills a texture (or T1_TEX_WINDOW) with a color.
- A draw pass draws its sources (T1_DRAW_*) with a pipeline. Properties:
  "cam" (u32, the camera's T1_id: its view and depth buffer, T1_ID_NONE is
  only for T1_DRAW_FULLSCREEN and T1_DRAW_PROCEDURAL) and "color_tex[0]" to
  "color_tex[3]" (u32 T1Texs the fragment function's [[color(k)]] go to,
  the camera's size, "color_tex[0]" may be T1_TEX_WINDOW). A
  T1_DRAW_PROCEDURAL pass draws "vertex_count" (u32) vertices as triangles;
  with a camera, its shaders get the cameras at buffer(3) and the camera's
  index at buffer(4), like zsprite shaders.
- A compute pass runs a compute pipeline once per pixel of its "write_tex"
  (u32 T1Tex, see T1_WRITE_TEX_I), or once per element of a "threads[0]" *
  "threads[1]" * "threads[2]" grid (u32s, 0 counts as 1) if "threads[0]" is
  set, e.g. once per particle in a buffer. Its kernel gets its position in
  the grid as [[thread_position_in_grid]], threads past the end don't run.
Draw and compute passes also have "read_tex[0]" to "read_tex[7]" (u32
T1Texs) for your shaders, see T1_READ_TEXS_BUFFER_I, and "buf[0]" to
"buf[7]" (u32 T1Bufs, see BUFFERS). Every pass's shaders also get a
T1GPUFrameConsts at buffer(T1_FRAME_CONSTS_I). A pass that reads what an
earlier pass drew or wrote waits for it.

Touch ids are read from the texture you give T1_touch_set_tex(), your
shaders pack them into its pixels (T1_TEX_NONE for no touch ids).
*/
#define T1_BLEND_NONE                0
#define T1_BLEND_ADDITIVE            1 // black adds nothing
#define T1_BLEND_PREMULTIPLIED_ALPHA 2 // rgb already multiplied by alpha
#define T1_BLEND_STRAIGHT_ALPHA      3
#define T1_CULL_NONE  0
#define T1_CULL_BACK  1
#define T1_CULL_FRONT 2
uint32_t T1_pipeline_new_draw(
    const char * vertex_fn,
    const char * fragment_fn,
    uint8_t blend,
    uint8_t cull,
    uint8_t * good);
uint32_t T1_pipeline_new_compute(
    const char * kernel_fn,
    uint8_t * good);

#define T1_DRAW_OPAQUE     0x01 // zsprites without alpha_on or bloom_on
#define T1_DRAW_BLENDED    0x02 // zsprites with alpha_on
#define T1_DRAW_BLOOMING   0x04 // zsprites with bloom_on
#define T1_DRAW_TEXQUADS   0x08
#define T1_DRAW_BILLBOARDS 0x10
#define T1_DRAW_FULLSCREEN 0x20 // 6 vertices, a quad covering the targets
// "vertex_count" vertices, T1 binds no geometry: your vertex function
// builds them from its vertex_id and your buffers
#define T1_DRAW_PROCEDURAL 0x40
uint32_t T1_pass_new_clear(
    T1Tex tex,
    float r,
    float g,
    float b,
    float a);
// sources is T1_DRAW_* bits, zsprite sources can be combined
uint32_t T1_pass_new_draw(uint32_t pipeline_T1_id, uint32_t sources);
uint32_t T1_pass_new_compute(uint32_t pipeline_T1_id);

void T1_touch_set_tex(T1Tex tex);

/*
BUFFERS
GPU memory of any size for your shaders, zeroed at first. A pass binds up to
8 of them with its "buf[0]" to "buf[7]" properties, and its shaders read
"buf[k]" at buffer(T1_BUFS_I + k) as whatever you stored there, e.g.
    device MyParticle * particles [[ buffer(T1_BUFS_I) ]]
Compute passes can write to them, and later passes see what they wrote.

T1_buf_write() copies data right away, so it can live on your stack. The
GPU sees your writes in order, from the next rendered frame on, and they
stay until something overwrites them. Keep the C and shader versions of a
struct identical: floats, uint32_ts and arrays of them are safe, while
Metal's float3 takes 16 bytes.
*/
T1Buf T1_buf_new(uint64_t size, char ** const fatal_error);
void  T1_buf_write(
    T1Buf buf,
    uint64_t offset,
    const void * data,
    uint64_t size);
void  T1_buf_delete(T1Buf buf);

/*
Reading back what your shaders wrote. T1_buf_read_start() asks for size bytes
at offset as they are after the next rendered frame's passes, and returns an
id (T1_BUF_READ_NONE if T1_BUF_READS_CAP reads are already waiting). Call
T1_buf_read_finish() with it once per frame or so: it returns 0 while the
GPU isn't done, usually for a frame or two, then copies the bytes to recip
and returns 1, after which the id is free again. Deleting the buffer before
the frame renders gives zeros.
*/
uint32_t T1_buf_read_start(T1Buf buf, uint64_t offset, uint64_t size);
uint8_t  T1_buf_read_finish(uint32_t read, void * recip);

/*
3-D MODELS (.OBJ FILES)

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
FILES
Your app reads its assets from the resources dir, and may write its own
files (saves, settings) in the writables dir. T1_os_res_path() and
T1_os_writable_path() give the full path of a file in either (of the dir
itself for ""), every other function takes full paths.
*/
void     T1_os_res_path(const char * filename, char * recip, uint32_t recip_cap);
void     T1_os_writable_path(
    const char * filename,
    char * recip,
    uint32_t recip_cap);
uint8_t  T1_os_file_exists(const char * path);
// 0 if there's no such file
uint64_t T1_os_file_size(const char * path);
// Fills recip with the file and a 0 after it, so recip_cap must be larger
// than the file
void     T1_os_file_read(
    const char * path,
    char * recip,
    uint64_t recip_cap,
    char ** const fatal_error);
void     T1_os_file_write(
    const char * path,
    const char * data,
    uint32_t size,
    uint8_t * good);
void     T1_os_file_delete(const char * path);
void     T1_os_file_copy(const char * src_path, const char * dst_path);
// The names of the files in a dir, each followed by a 0
void     T1_os_dir_filenames(
    const char * path,
    char * filenames,
    uint32_t filenames_cap);
// Shows the dir in Finder (or the OS's file browser) where possible
void     T1_os_dir_show(const char * path);

/*
OPERATING SYSTEM
*/
uint64_t T1_os_get_current_time_us(void);
void     T1_os_start_thread(void *(*function_to_run)(void *), void * arg_on_heap);
uint32_t T1_os_init_mutex_and_return_id(void);
uint8_t  T1_os_mutex_trylock(uint32_t mutex_id);
void     T1_os_mutex_lock(uint32_t mutex_id);
void     T1_os_mutex_unlock(uint32_t mutex_id);

/*
AUDIO
T1 plays raw samples (16-bit stereo at 44.1kHz), decode your sound files
with whatever parser you like. A sound is known by a name (e.g. its
filename): T1_audio_sound_id() returns its id, the same one every time,
and T1_audio_set_samples() gives it its samples once.
*/
int32_t  T1_audio_sound_id(const char * name, char ** const fatal_error);
void     T1_audio_set_samples(
    int32_t sound_id,
    int16_t * samples,
    int32_t samples_size);
void     T1_audio_play(int32_t sound_id, float volume);
/*
For music: copies samples_size samples of the sound, from sound_offset on,
into what plays play_cursor_offset samples from now
*/
void     T1_audio_stream(
    int32_t  sound_id,
    uint64_t sound_offset,
    uint64_t play_cursor_offset,
    uint32_t samples_size,
    uint8_t  is_music);
// how many samples have played
uint64_t T1_audio_play_cursor(void);
void     T1_audio_stop_all(void);

#endif // T1_H
