/*
These functions are not defined in the 'shared' folder but
they must be implemented by .c files specific to
your platform
*/

#ifndef T1_OS_H
#define T1_OS_H

#include "T1_stdint.h"
#include "T1_macro_settings.h"

/*
The platform T1 is built for, as the compiler says: one of these is
T1_ACTIVE, the rest T1_INACTIVE. Its layer is in engine_src/ (shared_apple/
and macos/, windows/), shared/ code reaches it only through this header
*/
#if defined(_WIN32)
#define T1_PLATFORM_APPLE T1_INACTIVE
#define T1_PLATFORM_WINDOWS T1_ACTIVE
#elif defined(__APPLE__)
#define T1_PLATFORM_APPLE T1_ACTIVE
#define T1_PLATFORM_WINDOWS T1_INACTIVE
#else
#error "T1 has no platform layer for this OS"
#endif

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

#if T1_PLATFORM_APPLE == T1_ACTIVE
void T1_apple_os_init(void);
#ifdef TARGET_OS_OSX
void T1_os_macos_init(void);
#endif
#elif T1_PLATFORM_WINDOWS == T1_ACTIVE
// loads the DLLs the rest of the platform layer calls, after T1_os_init()
void T1_os_windows_init(char ** const fatal_error);
#else
#error
#endif

// Needs T1_mem_init()
void T1_os_init(void);

// The OS's memory page size, before T1_os_init()
u32 T1_os_get_page_size(void);

f32 T1_os_get_screen_backing_scale_factor(void);

void T1_os_close_app(void);


void T1_os_get_dir_separator(c8 * recipient);

void T1_os_res_filename_to_pathfile(
    const c8 * filen,
    c8 * recip,
    u32 recip_cap);

void T1_os_writable_filename_to_pathfile(
    const c8 * filename,
    c8 * recipient,
    u32 recipient_capacity);


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

// Sizes in bytes, 0 if there's no such file. A resource or writable is a
// filename in that dir, a filepath is a full path.
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
Reads up to recip_cap - 1 bytes of the file into recip and puts a 0 after
them, *recip_size gets how many were read
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
/*
A thread that ends gives its T1 thread id back, for the next thread to get:
T1 thread ids index per-thread state (the objc autorelease pools), so
without this only so many threads could ever start
*/
void T1_os_forget_raw_thread_id(u64 raw_in);
u64 T1_os_raw_thread_id_to_T1_thread_id(
    u64 raw_in,
    char ** const fatal_error);
/*
creates a mutex and return the ID of said mutex for you to store. Mutexes
are the platform layer's (pthreads, Windows' SRW locks), not recursive
*/
#define T1_MUTEXES_CAP 50
u32 T1_os_init_mutex_and_return_id(void);

/* returns true if mutex succesfully locked */
u8 T1_os_mutex_trylock(u32 mutex_id);


void T1_os_mutex_lock(u32 mutex_id);

void T1_os_mutex_unlock(u32 mutex_id);


u64 T1_os_get_current_time_us(void);

u64 T1_os_get_clock_frequency(void);

f32 T1_os_x_to_x(f32 x);

f32 T1_os_y_to_y(f32 y);

void T1_os_enter_fullscreen(void);

void T1_os_toggle_fullscreen(void);




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

void T1_os_layer_start_window_resize(u64 timestamp);

// Creates the GPU device and T1's GPU resources, no window needed
void T1_os_gpu_init(char ** const fatal_error);
// T1_os_gpu_init() succeeded
u8 T1_os_gpu_active(void);
// see T1_gpu_lost() in T1.h
u8 T1_os_gpu_lost(void);

// Opens the main window, presenting with the device from T1_os_gpu_init()
void T1_os_window_open(char ** const fatal_error);
void T1_os_destroy_main_window_if_possible(void);

// see T1_gpu_try_shader_source() in T1.h
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

// GPU memory of size zeroed bytes for T1Buf buf_i
void T1_os_gpu_buf_new(u32 buf_i, u64 size, u8 * good);
// Copies data now, the GPU applies it before the next rendered frame
void T1_os_gpu_buf_write(
    u32 buf_i,
    u64 offset,
    const void * data,
    u64 size);
void T1_os_gpu_buf_delete(u32 buf_i);
// A read the next rendered frame encodes after its passes, its index or
// T1_BUF_READ_NONE (see T1_buf_read_start())
u32 T1_os_gpu_buf_read_start(u32 buf_i, u64 offset, u64 size);
// A read of w * h rgba pixels of a texture, from (x, y)
u32 T1_os_gpu_tex_read_start(u32 tex, u32 x, u32 y, u32 w, u32 h);
// 1 once the read's frame is done and its bytes are copied to recip
u8 T1_os_gpu_buf_read_finish(u32 read_i, void * recip);

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
// Shows the T1Tex tex as large as it fits in the window, keeping its aspect
// ratio, with black bands around it. Returns false if nothing was
// presented.
u8 T1_os_window_present(const u32 tex);

// Waits until fewer than T1_FRAMES_CAP frames are in flight. False if the
// GPU isn't ready to render.
u8 T1_os_gpu_begin_frame(void);
// Encodes the passes. Returns false if nothing was rendered.
u8 T1_os_gpu_render_frame(void);

#endif // T1_OS_H
