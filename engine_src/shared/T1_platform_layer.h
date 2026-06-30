/*
These functions are not defined in the 'shared' folder they must be provided
by the platform layer in a platform_layer.c file

For example, 'platform_read_file' is currently defined in
/shared_windows_macos/platform_layer.c for windows and mac os X, but it's
defined elsewhere for iOS

Therefore, the implementations of the function signatures in this header are
actually scattered across multiple source files, since the combination of
source files will be different on every platform

The implementation folders are organized like this:
1. src/shared_apple -> code that work on iOS and MacOS but not elsewhere
2. src/macos -> code that works on MacOs esclusively
3. src/linux -> code that works on Linux exclusively
4. src/ios -> code that works on iOS
5. src/shared_windows_macos -> code that works on windows and macos, but not elsewhere
etc.

Finally, there is 'common_platform_layer.c' which contains functions that
are called only by other platform layer functions, but have identical code
on each platform
*/

#ifndef T1_PLATFORM_LAYER_H
#define T1_PLATFORM_LAYER_H

#include "T1_stdint.h"

#ifdef __cplusplus
extern "C" {
#endif

void T1_os_init(
    void ** unmanaged_memory_store,
    const u32 aligned_to);

void
T1_os_close_app(void);

void *
T1_os_malloc_unaligned_block(
    const u64 size);

u32
T1_os_get_dir_separator_size(void);

void
T1_os_get_dir_separator(char * recipient);

void
T1_os_res_filename_to_pathfile(
    const char * filename,
    char * recipient,
    const u32 recipient_capacity);

void
T1_os_writable_filename_to_pathfile(
    const char * filename,
    char * recipient,
    const u32 recipient_capacity);

void
T1_os_open_dir_in_file_explorer_window_if_possible(
    const char * folderpath);

// get current working directory
void
T1_os_get_app_dir(
    char * recipient,
    const u32 recipient_size);

void
T1_os_get_res_dir(
    char * recip,
    const u32 recip_cap);

void
T1_os_get_cwd(
    char * recipient,
    const u32 recipient_size);

// a root directory where we're allowed to write
void
T1_os_get_writables_dir(
    char * recipient,
    const u32 recipient_size);

u8
T1_os_res_exists(const char * resource_name);

u8
T1_os_file_exists(const char * filepath);

void
T1_os_del_file(const char * filepath);

void
T1_os_del_writable(const char * writable_filename);

void
T1_os_write_file(
    const char * filepath_destination,
    const char * output,
    const u32 output_size,
    u8 * good);

void
T1_os_write_file_to_writables(
    const char * filepath_inside_writables,
    const char * output,
    const u32 output_size,
    u8 * good);

void
T1_os_write_rgba_to_writables(
    const char * local_filename,
    u8 * rgba,
    const u32 rgba_size,
    const u32 width,
    const u32 height,
    u8 * good);

void
T1_os_copy_file(
    const char * filepath_source,
    const char * filepath_destination);

void
T1_os_mkdir_if_not_exist(
    const char * dirname);

void
T1_os_get_filenames_in(
    const char * directory,
    char filenames[2000][500]);

/*
Get a file's size. Returns 0 if no such file

A 'resource' is a file that's available in the typical folder for our platform
, so you can pass "warrior.png" or whatever, the filename only without a path

A 'filepath' is a full explicit path to and including the filename
*/
u64
T1_os_get_resource_size(
    const char * filename);

u64
T1_os_get_writable_size(
    const char * filename);

u64
T1_os_get_filesize(
    const char * filepath);

#if T1_AUDIO_ACTIVE == T1_ACTIVE
void
T1_platform_audio_start_loop(void);
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
void
T1_os_read_resource_file(
    const char * filename,
    char * recip,
    const u64 recip_cap,
    u8 * good);

void
T1_os_read_file(
    const char * filepath,
    char * recip,
    u32 * recip_size,
    const u64 recip_cap,
    u8 * good);

void
T1_os_read_file_from_writables(
    const char * filepath_inside_writables,
    char * recipient,
    const u32 recipient_size,
    u8 * good);

void
T1_os_gpu_get_device_name(
    char * recipient,
    const u32 recipient_cap);

void T1_os_gpu_update_capacity_if_needed(
    const s32 tex_array_i);

u32
T1_os_get_cpu_logical_core_count(void);

/*
Run a task in the background I only use this to pass clientlogic.c's
client_logic_threadmain() passing the threadmain_id to it you have to
implement client_logic_threadmain() to do what you want it to do when it gets
that id
*/
void
T1_os_start_thread(
    void (*function_to_run)(s32),
    s32 argument);

u64
T1_os_get_current_time_us(void);

u64
T1_os_get_clock_frequency(void);

f32
T1_os_x_to_x(const f32 x);

f32
T1_os_y_to_y(const f32 y);

void
T1_os_enter_fullscreen(void);

void
T1_os_toggle_fullscreen(void);

void
T1_os_gpu_update_internal_render_viewport(
    const s32 at_i);

void
T1_os_gpu_update_window_viewport(void);

void
T1_os_gpu_copy_locked_vertices(void);

void
T1_os_gpu_copy_locked_materials(void);

s32
T1_os_gpu_get_touch_id_at_screen_pos(
    const f32 screen_x,
    const f32 screen_y);

#if T1_MIPMAPS_ACTIVE == T1_ACTIVE
void
T1_os_gpu_generate_mipmaps_for_texture_array(
    const s32 texture_array_i);
#elif T1_MIPMAPS_ACTIVE == T1_INACTIVE
#else
#error
#endif

void
T1_os_gpu_push_tex_slice_and_free_rgba(
    const s32 texture_array_i,
    const s32 texture_i);

#if T1_TEXTURES_ACTIVE == T1_ACTIVE
void T1_os_gpu_fetch_rgba_at(
    const s32 texture_array_i,
    const s32 texture_i,
    u8 * rgba_recipient,
    u32 * recipient_size,
    u32 * recipient_width,
    u32 * recipient_height,
    const u32 recipient_cap,
    u32 * good);

void T1_os_gpu_delete_texture_array(
    const s32 array_i);

void T1_os_gpu_delete_depth_tex(
    const s32 slice_i);
#elif T1_TEXTURES_ACTIVE == T1_INACTIVE
#else
#error
#endif

s16 T1_os_gpu_make_depth_tex(
    const u32 width,
    const u32 height);

void T1_platform_update_mouse_location(void);

// This is used to communicate failure after failure to initialize GPU
// acceleration, so assume no Metal/OpenGL/Vulkan/etc. available
void T1_platform_request_messagebox(const char * message);

/*
creates a mutex and return the ID of said mutex for you to store
*/
u32 T1_os_init_mutex_and_return_id(void);

/* returns true if mutex succesfully locked */
u8 T1_os_mutex_trylock(const u32 mutex_id);

void T1_os_assert_mutex_locked(const u32 mutex_id);

void T1_os_mutex_lock(const u32 mutex_id);

void T1_os_mutex_unlock(const u32 mutex_id);

void T1_os_layer_start_window_resize(
    const u64 timestamp);

#ifdef __cplusplus
}
#endif

#endif // T1_PLATFORM_LAYER_H
