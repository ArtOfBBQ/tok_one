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

#include "T1_std.h"
#include "T1_public_types.h"

#ifdef __cplusplus
extern "C" {
#endif

void T1_os_init(
    void ** unmanaged_memory_store,
    const uint32_t aligned_to);

void
T1_os_close_app(void);

void *
T1_os_malloc_unaligned_block(
    const uint64_t size);

uint32_t
T1_os_get_dir_separator_size(void);

void
T1_os_get_dir_separator(char * recipient);

void
T1_os_res_filename_to_pathfile(
    const char * filename,
    char * recipient,
    const uint32_t recipient_capacity);

void
T1_os_writable_filename_to_pathfile(
    const char * filename,
    char * recipient,
    const uint32_t recipient_capacity);

void
T1_os_open_dir_in_window_if_possible(
    const char * folderpath);

// get current working directory
void
T1_os_get_app_dir(
    char * recipient,
    const uint32_t recipient_size);

void
T1_os_get_res_dir(
    char * recip,
    const uint32_t recip_cap);

void
T1_os_get_cwd(
    char * recipient,
    const uint32_t recipient_size);

// a root directory where we're allowed to write
void
T1_os_get_writables_dir(
    char * recipient,
    const uint32_t recipient_size);

uint8_t
T1_os_res_exists(const char * resource_name);

uint8_t
T1_os_file_exists(const char * filepath);

void
T1_os_del_file(const char * filepath);

void
T1_os_del_writable(const char * writable_filename);

void
T1_os_write_file(
    const char * filepath_destination,
    const char * output,
    const uint32_t output_size,
    uint8_t * good);

void
T1_os_write_file_to_writables(
    const char * filepath_inside_writables,
    const char * output,
    const uint32_t output_size,
    uint8_t * good);

void
T1_os_write_rgba_to_writables(
    const char * local_filename,
    uint8_t * rgba,
    const uint32_t rgba_size,
    const uint32_t width,
    const uint32_t height,
    uint8_t * good);

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
uint64_t
T1_os_get_resource_size(
    const char * filename);

uint64_t
T1_os_get_writable_size(
    const char * filename);

uint64_t
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
    const uint64_t recip_cap,
    uint8_t * good);

void
T1_os_read_file(
    const char * filepath,
    char * recip,
    uint32_t * recip_size,
    const uint64_t recip_cap,
    uint8_t * good);

void
T1_os_read_file_from_writables(
    const char * filepath_inside_writables,
    char * recipient,
    const uint32_t recipient_size,
    uint8_t * good);

void
T1_os_gpu_get_device_name(
    char * recipient,
    const uint32_t recipient_cap);

void T1_os_gpu_update_capacity_if_needed(
    const int32_t tex_array_i);

uint32_t
T1_os_get_cpu_logical_core_count(void);

/*
Run a task in the background I only use this to pass clientlogic.c's
client_logic_threadmain() passing the threadmain_id to it you have to
implement client_logic_threadmain() to do what you want it to do when it gets
that id
*/
void
T1_os_start_thread(
    void (*function_to_run)(int32_t),
    int32_t argument);

uint64_t
T1_os_get_current_time_us(void);

uint64_t
T1_os_get_clock_frequency(void);

float
T1_os_x_to_x(const float x);

float
T1_os_y_to_y(const float y);

void
T1_os_enter_fullscreen(void);

void
T1_os_toggle_fullscreen(void);

void
T1_os_gpu_update_internal_render_viewport(
    const int32_t at_i);

void
T1_os_gpu_update_window_viewport(void);

void
T1_os_gpu_copy_locked_vertices(void);

void
T1_os_gpu_copy_locked_materials(void);

int32_t
T1_os_gpu_get_touch_id_at_screen_pos(
    const float screen_x,
    const float screen_y);

#if T1_MIPMAPS_ACTIVE == T1_ACTIVE
void
T1_platform_gpu_generate_mipmaps_for_texture_array(
    const int32_t texture_array_i);
#elif T1_MIPMAPS_ACTIVE == T1_INACTIVE
#else
#error
#endif

void
T1_os_gpu_push_tex_slice_and_free_rgba(
    const int32_t texture_array_i,
    const int32_t texture_i);

#if T1_TEXTURES_ACTIVE == T1_ACTIVE
void T1_os_gpu_fetch_rgba_at(
    const int32_t texture_array_i,
    const int32_t texture_i,
    uint8_t * rgba_recipient,
    uint32_t * recipient_size,
    uint32_t * recipient_width,
    uint32_t * recipient_height,
    const uint32_t recipient_cap,
    uint32_t * good);

void T1_platform_gpu_delete_texture_array(
    const int32_t array_i);

void T1_platform_gpu_delete_depth_tex(
    const int32_t slice_i);
#elif T1_TEXTURES_ACTIVE == T1_INACTIVE
#else
#error
#endif

int32_t
T1_platform_gpu_make_depth_tex(
    const uint32_t width,
    const uint32_t height);

void T1_platform_update_mouse_location(void);

// This is used to communicate failure after failure to initialize GPU
// acceleration, so assume no Metal/OpenGL/Vulkan/etc. available
void T1_platform_request_messagebox(const char * message);

/*
creates a mutex and return the ID of said mutex for you to store
*/
uint32_t T1_platform_init_mutex_and_return_id(void);

/* returns true if mutex succesfully locked */
uint8_t T1_platform_mutex_trylock(const uint32_t mutex_id);

void T1_platform_assert_mutex_locked(const uint32_t mutex_id);

void T1_platform_mutex_lock(const uint32_t mutex_id);

void T1_platform_mutex_unlock(const uint32_t mutex_id);

void T1_platform_layer_start_window_resize(
    const uint64_t timestamp);

#ifdef __cplusplus
}
#endif

#endif // T1_PLATFORM_LAYER_H
