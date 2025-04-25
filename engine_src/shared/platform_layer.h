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

#ifndef PLATFORM_LAYER_H
#define PLATFORM_LAYER_H

#ifdef _WIN32 
#include <windows.h>
#include <Knownfolders.h>
#include <Libloaderapi.h>
#include <shlwapi.h>
#endif

#ifdef PLATFORM_IOS
#import <UIKit/UIKit.h>
#endif

#define MUTEXES_SIZE 100

#ifdef SHARED_APPLE_PLATFORM
#import <Foundation/Foundation.h>
#include <pthread.h>
#include <sys/time.h>
#include <sys/sysctl.h> // for sysctl to get clock frequency
#endif

#ifdef LINUX_PLATFORM
#include <pthread.h>
#include <sys/time.h>
#include <sys/mman.h>
#include <sys/stat.h> // stat function to check if dir exists
#include <sys/errno.h>
#include <fcntl.h> // contains flags like O_RDONLY for open()
#include <dirent.h> // to list files in a dir
#endif

#ifdef __ARM_NEON
#include "arm_neon.h"
#elif defined(__AVX__)
#include "immintrin.h"
#endif

#ifdef __cplusplus
extern "C" {
#endif

// #include "audio.h"
#include "clientlogic_macro_settings.h"
#include "common.h"
#include "logger.h"
#include "decodedimage.h"
#include "window_size.h"

void platform_layer_init(
    void ** unmanaged_memory_store,
    const uint32_t aligned_to);

typedef struct FileBuffer {
    uint64_t size_without_terminator;
    char * contents;
    bool32_t good;
} FileBuffer;

void platform_close_application(void);

void * platform_malloc_unaligned_block(const uint64_t size);

uint32_t platform_get_directory_separator_size(void);
void platform_get_directory_separator(char * recipient);

void resource_filename_to_pathfile(
    const char * filename,
    char * recipient,
    const uint32_t recipient_capacity);
void writable_filename_to_pathfile(
    const char * filename,
    char * recipient,
    const uint32_t recipient_capacity);

void platform_open_folder_in_window_if_possible(
    const char * folderpath);

// get current working directory
void platform_get_application_path(
    char * recipient,
    const uint32_t recipient_size);
void platform_get_resources_path(
    char * recipient,
    const uint32_t recipient_size);
void platform_get_cwd(
    char * recipient,
    const uint32_t recipient_size);

// a root directory where we're allowed to write
void platform_get_writables_path(
    char * recipient,
    const uint32_t recipient_size);

bool32_t platform_resource_exists(const char * resource_name);
bool32_t platform_file_exists(const char * filepath);
void platform_delete_file(const char * filepath);
void platform_delete_writable(const char * writable_filename);

void
platform_write_file(
    const char * filepath_destination,
    const char * output,
    const uint32_t output_size,
    bool32_t * good);

void
platform_write_file_to_writables(
    const char * filepath_inside_writables,
    const char * output,
    const uint32_t output_size,
    bool32_t * good);

void platform_copy_file(
    const char * filepath_source,
    const char * filepath_destination);
void platform_mkdir_if_not_exist(
    const char * dirname);

void platform_get_filenames_in(
    const char * directory,
    char filenames[2000][500]);

/*
Get a file's size. Returns 0 if no such file

A 'resource' is a file that's available in the typical folder for our platform
, so you can pass "warrior.png" or whatever, the filename only without a path

A 'filepath' is a full explicit path to and including the filename
*/
uint64_t platform_get_resource_size(const char * filename);
uint64_t platform_get_filesize(const char * filepath);

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
void platform_read_resource_file(
    const char * filename,
    FileBuffer * out_preallocatedbuffer);

void platform_read_file(
    const char * filepath,
    FileBuffer * out_preallocatedbuffer);

void platform_gpu_get_device_name(
    char * recipient,
    const uint32_t recipient_cap);

/*
Run a task in the background I only use this to pass clientlogic.c's
client_logic_threadmain() passing the threadmain_id to it you have to
implement client_logic_threadmain() to do what you want it to do when it gets
that id
*/
void platform_start_thread(
    void (*function_to_run)(int32_t),
    int32_t argument);

uint64_t platform_get_current_time_microsecs(void);

uint64_t platform_get_clock_frequency(void);

float platform_x_to_x(const float x);
float platform_y_to_y(const float y);

void platform_enter_fullscreen(void);
void platform_toggle_fullscreen(void);
void platform_gpu_update_viewport(void);

void platform_gpu_copy_locked_vertices(void);

int32_t platform_gpu_get_touchable_id_at_screen_pos(
    const float screen_x,
    const float screen_y);

void platform_gpu_init_texture_array(
    const int32_t texture_array_i,
    const uint32_t num_images,
    const uint32_t single_image_width,
    const uint32_t single_image_height,
    const bool32_t use_bc1_compression);

void platform_gpu_push_texture_slice_and_free_rgba_values(
    const int32_t texture_array_i,
    const int32_t texture_i,
    const uint32_t parent_texture_array_images_size,
    const uint32_t image_width,
    const uint32_t image_height,
    uint8_t * rgba_values_to_use_and_free);

void platform_gpu_push_bc1_texture_slice(
    const int32_t texture_array_i,
    const int32_t texture_i,
    const uint32_t parent_texture_array_images_size,
    const uint32_t image_width,
    const uint32_t image_height,
    const uint8_t * bc1_values);

void platform_update_mouse_location(void);

// This is used to communicate failure after failure to initialize GPU
// acceleration, so assume no Metal/OpenGL/Vulkan/etc. available
void platform_request_messagebox(const char * message);

/*
creates a mutex and return the ID of said mutex for you to store
*/
uint32_t platform_init_mutex_and_return_id(void);

/* returns true if mutex succesfully locked */
bool32_t platform_mutex_trylock(const uint32_t mutex_id);

void platform_assert_mutex_locked(const uint32_t mutex_id);

void platform_mutex_lock(const uint32_t mutex_id);

void platform_mutex_unlock(const uint32_t mutex_id);

#ifdef __cplusplus
}
#endif

#endif // PLATFORM_LAYER_H 

