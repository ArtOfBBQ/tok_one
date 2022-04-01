/*
These functions are not defined in the 'shared' folder
they must be provided by the platform layer in a .c file

For example, 'platform_read_file' is currently defined
in /shared_windows_macos/platform_read_file.c for windows
and mac os X, but it's defined elsewhere for iOS
*/

#ifndef PLATFORM_LAYER_H
#define PLATFORM_LAYER_H

#ifdef SHARED_APPLE_PLATFORM
#include <mach/mach_time.h>
#endif

#include <stdlib.h>
#include <stdio.h>

#include "common.h"
#include "decodedimage.h"

typedef struct FileBuffer {
    uint64_t size;
    char * contents;
} FileBuffer;

// Read a file (without path, only filename)
// and return its contents as a buffer of bytes
FileBuffer * platform_read_file(
    char * filename);

void platform_start_timer(void);
uint64_t platform_get_current_time_nanosecs(void);
uint64_t platform_end_timer_get_nanosecs(void);

#endif
