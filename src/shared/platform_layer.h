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
extern NSFileManager * file_manager;
#endif

#ifdef SHARED_APPLE_PLATFORM
#include <mach/mach_time.h>
#endif

#include <stdlib.h>
#include <stdio.h>
#include <string.h> // for fopen() error
#include <errno.h>  // for fopen() error

#include "common.h"
#include "clientlogic.h"
#include "debigulator/src/decodedimage.h"

typedef struct FileBuffer {
    uint64_t size;
    char * contents;
} FileBuffer;

// get current working directory
char * platform_get_application_path(void);
char * platform_get_resources_path(void);
char * platform_get_cwd(void);

/*
Get a file's size. Returns -1 if no such file
*/
int64_t platform_get_filesize(const char * filename);

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
*/
void platform_read_file(
    const char * filename,
    FileBuffer * out_preallocatedbuffer);

// Run a task in the background
// This will trigger clientlogic.c's client_logic_threadmain()
// passing the threadmain_id to it
// you have to implement client_logic_threadmain() to do
// what you want it to do when it gets that id
void platform_start_thread(int32_t threadmain_id);

// TODO: is platform_start_timer still needed?
// void platform_start_timer(void);
uint64_t platform_get_current_time_microsecs(void);

#endif

