/*
These functions are not defined in the 'shared' folder
they must be provided by the platform layer in a
platform_layer.c file

For example, 'platform_read_file' is currently defined
in /shared_windows_macos/platform_layer.c for windows
and mac os X, but it's defined elsewhere for iOS

you also need to
#DEFINE INT64TFORMATSTR
and
#DEFINE UINT64TFORMATSTR
because uint64_t and int64_t demand different formatting
strings for printf depending on the platform
I like to do this with a compiler command, e.g.
clang++ -D LONGLONGINT64 on the mac os platform
*/

#ifndef PLATFORM_LAYER_H
#define PLATFORM_LAYER_H

#ifdef LONGLONGINT64
#define FUINT64 "%llu"
#define FINT64 "%lli"
#endif

#ifdef LONGINT64
#define FUINT64 "%lu"
#define FINT64 "%li"
#endif

#ifdef PLATFORM_NS_FILEMANAGER
#import <Foundation/Foundation.h>
#endif

#ifdef SHARED_APPLE_PLATFORM
#include <mach/mach_time.h>
#endif

#include "common.h"
#include "logger.h"
#include "debigulator/src/decodedimage.h"

typedef struct FileBuffer {
    uint64_t size;
    char * contents;
    bool32_t good;
} FileBuffer;

uint32_t platform_get_directory_separator_size();
void platform_get_directory_separator(
    char * recipient);
void resource_filename_to_pathfile(
    const char * filename,
    char * recipient,
    const uint32_t recipient_capacity);

// get current working directory
char * platform_get_application_path(void);
char * platform_get_resources_path(void);
char * platform_get_cwd(void);

bool32_t platform_file_exists(
    const char * filepath);
void platform_delete_file(
    const char * filepath);
void platform_write_file(
    const char * filepath_destination,
    const char * output,
    const uint32_t output_size);
void platform_copy_file(
    const char * filepath_source,
    const char * filepath_destination);
void platform_mkdir_if_not_exist(
    const char * dirname);
void platform_get_filenames_in(
    const char * directory,
    char ** filenames,
    const uint32_t recipient_capacity,
    uint32_t * recipient_size);

/*
Get a file's size. Returns 0 if no such file
*/
uint64_t platform_get_resource_size(
    const char * filename);
uint64_t platform_get_filesize(
    const char * filepath);

/*
Read a file (without path, only filename)
and fill its contents into a buffer of bytes

out_allocatedbuffer should have memory allocated
already and size set to its maximum capacity

the file contents will stop copying when the filebuffer
is full, so you can set the filbuffer's size to a small amount
to quickly read the first (x-1) bytes of a large file
it's x-1 and not x because a 0 will be appended at the end
to make windows happy

If there's an error reading the file, the buffer's
'good' field will be set to 0, else to 1
*/
void platform_read_resource_file(
    const char * filename,
    FileBuffer * out_preallocatedbuffer);
void platform_read_file(
    const char * filepath,
    FileBuffer * out_preallocatedbuffer);

// Run a task in the background
// I only use this to pass clientlogic.c's
// client_logic_threadmain() passing the threadmain_id to it
// you have to implement client_logic_threadmain() to do
// what you want it to do when it gets that id
void platform_start_thread(
    void (*function_to_run)(int32_t),
    int32_t argument);

uint64_t __attribute__((no_instrument_function))
platform_get_current_time_microsecs(void);

#endif
