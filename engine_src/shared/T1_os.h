/*
These functions are not defined in the 'shared' folder but
they must be implemented by .c files specific to
your platform
*/

#ifndef T1_OS_H
#define T1_OS_H

#include "T1_stdint.h"
#include "T1_types_cpu_to_gpu.h"

#if T1_ENGINE_SAVEFILE_ACTIVE == T1_ACTIVE
typedef struct {
    f32 window_left;
    f32 window_width;
    f32 window_bottom;
    f32 window_height;
    f32 music_volume;
    f32 sound_volume;
    b8 window_fullscreen;
} T1EngineSaveFile;

extern T1EngineSaveFile * engine_save_file;
#elif T1_ENGINE_SAVEFILE_ACTIVE == T1_INACTIVE
// Pass
#else
#error "T1_ENGINE_SAVEFILE_ACTIVE not set!"
#endif

#ifdef __APPLE__
void T1_apple_os_init(void);
#endif

#ifdef TARGET_OS_OSX
void T1_os_macos_init(void);
#endif

void T1_os_init(
    void ** unmanaged_memory_store,
    u32 aligned_to);

f32 T1_os_get_screen_backing_scale_factor(void);

void T1_os_close_app(void);

void * T1_os_malloc_unaligned_block(
    u64 size);


void T1_os_get_dir_separator(c8 * recipient);

void T1_os_res_filename_to_pathfile(
    const c8 * filen,
    c8 * recip,
    u32 recip_cap);

void T1_os_writable_filename_to_pathfile(
    const c8 * filename,
    c8 * recipient,
    u32 recipient_capacity);

void T1_os_open_dir_in_file_explorer_window_if_possible(
    const c8 * folderpath);

// get current working directory

void T1_os_get_res_dir(
    c8 * recip,
    u32 recip_cap);

void T1_os_get_cwd(
    c8 * recipient,
    u32 recipient_size);

// a root directory where we're allowed to write
void T1_os_get_writables_dir(
    c8 * recipient,
    u32 recipient_size);

u8 T1_os_writable_exists(const c8 * writable_name);
u8 T1_os_res_exists(const c8 * resource_name);

u8 T1_os_file_exists(const c8 * filepath);

void T1_os_del_file(const c8 * filepath);

void T1_os_del_writable(const c8 * writable_filename);

void T1_os_write_file(
    const c8 * filepath_destination,
    const c8 * output,
    u32 output_size,
    u8 * good);

void T1_os_write_file_to_writables(
    const c8 * filepath_inside_writables,
    const c8 * output,
    u32 output_size,
    b8 * good);

void T1_os_write_rgba_to_writables(
    const c8 * local_filename,
    u8 * rgba,
    u32 rgba_size,
    u32 width,
    u32 height,
    u8 * good);

void T1_os_copy_file(
    const c8 * filepath_source,
    const c8 * filepath_destination);

b8 T1_os_mkdir_if_not_exist(
    const c8 * dirname);

void T1_os_get_filenames_in(
    const c8 * directory,
    char * filenames,
    const u32 filenames_cap);

/*
Get a file's size. Returns 0 if no such file

A 'resource' is a file that's available in the typical folder
for our platform, so you can pass "warrior.png" or whatever,
the filename only without a path

A 'filepath' is a full explicit path to and including the filename
*/
u64 T1_os_get_resource_size(
    const c8 * filename);

u64 T1_os_get_writable_size(
    const c8 * filename);

u64 T1_os_get_filesize(
    const c8 * filepath);

#if T1_AUDIO_ACTIVE == T1_ACTIVE
void T1_os_audio_init(u8 * good);
void T1_os_audio_start_loop(void);
#elif T1_AUDIO_ACTIVE == T1_INACTIVE
#else
#error
#endif

/*
Read a file (without path, only filename) and fill its contents into a buffer
of bytes

out_allocatedbuffer should have memory allocated already and size set to its
maximum capacity

the file contents will stop copying when the filebuffer is full, so you can
set the filbuffer's size to a small amount to quickly read the first (x-1)
bytes of a large file it's x-1 and not x because a 0 will be appended at the
end to make windows happy

If there's an error reading the file, the buffer's 'good' field will be set to
0, else to 1
*/
void T1_os_read_file(
    const c8 * filepath,
    c8 * recip,
    u32 * recip_size,
    u64 recip_cap,
    char ** const fatal_error);

void T1_os_read_resource_file(
    const c8 * filename,
    c8 * recip,
    u64 recip_cap,
    char ** const fatal_error);

void T1_os_read_writable_file(
    const c8 * filepath_inside_writables,
    c8 * recip,
    u64 recip_cap,
    char ** const fatal_error);

void T1_os_gpu_get_device_name(
    c8 * recipient,
    u32 recipient_cap);

void T1_os_gpu_update_capacity_if_needed(
    s32 tex_array_i);

u32 T1_os_get_cpu_logical_core_count(void);

#if T1_GAMEPAD_ACTIVE == T1_ACTIVE
void T1_os_poll_gamepad_events(void);
#elif T1_GAMEPAD_ACTIVE == T1_INACTIVE
#else
#error
#endif

/*
Runs function_to_run(argument) on a new thread
*/
void T1_os_start_thread(
    void *(*function_to_run)(void *),
    void * argument);
u64 T1_os_get_T1_thread_id(char ** const fatal_error);
u64 T1_os_raw_thread_id_to_T1_thread_id(
    u64 raw_in,
    char ** const fatal_error);

u64 T1_os_get_current_time_us(void);

// Writes message and a newline to the error output (stderr)
void T1_os_print_error(const char * message);

u64 T1_os_get_clock_frequency(void);

f32 T1_os_x_to_x(f32 x);

f32 T1_os_y_to_y(f32 y);

void T1_os_enter_fullscreen(void);

void T1_os_toggle_fullscreen(void);

void T1_os_gpu_update_internal_render_viewport(
    const s32 at_i);

void T1_os_gpu_update_window_viewport(void);

void T1_os_gpu_copy_locked_vertices(void);

void T1_os_gpu_copy_locked_materials(void);

u32 T1_os_gpu_get_touch_id_at_screen_pos(
    f32 screen_x,
    f32 screen_y);

#if T1_MIPMAPS_ACTIVE == T1_ACTIVE
void T1_os_gpu_generate_mipmaps_for_texture_array(
    s32 texture_array_i);
#elif T1_MIPMAPS_ACTIVE == T1_INACTIVE
#else
#error
#endif

void T1_os_gpu_push_tex_slice(
    s32 tex_array_i,
    s32 tex_slice_i);

#if T1_TEXTURES_ACTIVE == T1_ACTIVE
void T1_os_gpu_fetch_rgba_at(
    s32 texture_array_i,
    s32 texture_i,
    u8 * rgba_recipient,
    u32 * recipient_size,
    u32 * recipient_width,
    u32 * recipient_height,
    u32 recipient_cap,
    u32 * good);

void T1_os_gpu_delete_texture_array(
    s32 array_i);

#elif T1_TEXTURES_ACTIVE == T1_INACTIVE
#else
#error
#endif

/*
creates a mutex and return the ID of said mutex for you to store
*/
u32 T1_os_init_mutex_and_return_id(void);

/* returns true if mutex succesfully locked */
u8 T1_os_mutex_trylock(u32 mutex_id);


void T1_os_mutex_lock(u32 mutex_id);

void T1_os_mutex_unlock(u32 mutex_id);

void T1_os_layer_start_window_resize(u64 timestamp);

// Creates the GPU device and T1's GPU resources, no window needed
void T1_os_gpu_init(char ** const fatal_error);
// T1_os_gpu_init() succeeded
u8 T1_os_gpu_active(void);

// Opens the main window, presenting with the device from T1_os_gpu_init()
// if there is one, else showing what T1_os_window_blit_rgba() gets
void T1_os_window_open(char ** const fatal_error);
void T1_os_destroy_main_window_if_possible(void);

/*
Compiles shader_source (with T1's GPU struct headers
prepended) and builds all render pipelines from it.
If anything fails, *good is set to 0, a description is
copied to error_recip (optional, may be NULL, else
needs a cap of at least 512), and any
shaders loaded by a previous successful call stay in
place, so you can retry with a different source.
You must succeed at least once before
T1_window_open(), else that's fatal.
*/
void T1_os_gpu_try_shader_source(
    const char * shader_source,
    u8 * good,
    char * error_recip,
    const u32 error_recip_cap,
    char ** const fatal_error);

u8 T1_os_gpu_shaders_ready(void);

/*
Called for a new entry in T1_pipelines. If shaders are loaded, *good is 0
when its functions don't exist in them, else they're checked on load.
*/
void T1_os_gpu_pipeline_added(s32 pipeline_i, u8 * good);
// releases what the gpu built for the pipeline
void T1_os_gpu_pipeline_deleted(s32 pipeline_i);

// the texture touch ids are read from, T1_TEX_NONE for none
void T1_os_gpu_set_touch_tex(u16 tex);

void T1_os_shutdown(void);

/*
The platform half of the client's main loop (see T1_window_* and T1_frame_*
in T1.h).
*/

// Dispatches pending window/input events. If max_wait_us > 0, first waits up
// to that long for one to arrive. Must be called from the main thread.
void T1_os_window_pump_events(const u64 max_wait_us);
u8 T1_os_window_close_requested(void);
u8 T1_os_window_visible(void);
// Copies the last rendered frame to the window. Returns false if nothing
// was presented.
u8 T1_os_window_present(void);
// Shows width * height rgba pixels (top row first, alpha ignored) as large
// as they fit in the window, keeping their aspect ratio, with black bands
// around them. Only for windows opened without T1_os_gpu_init().
void T1_os_window_blit_rgba(
    const u8 * rgba,
    const u32 width,
    const u32 height);

// Waits until the GPU is done with the next frame's buffers, then returns
// them, or NULL if the GPU isn't ready to render.
T1GPUFrame * T1_os_gpu_begin_frame(void);
// Renders the frame returned by T1_os_gpu_begin_frame() into the final
// texture. Returns false if nothing was rendered.
u8 T1_os_gpu_render_frame(void);

#endif // T1_OS_H
