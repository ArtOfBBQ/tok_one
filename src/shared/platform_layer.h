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
#include "../shared_apple/gpu.h"
extern MetalKitViewDelegate * apple_gpu_delegate;
#endif

#include <stdlib.h>
#include <stdio.h>

#include "common.h"
#include "decodedimage.h"

typedef struct FileBuffer {
    uint32_t size;
    char * contents;
} FileBuffer;

// Read a file (without path, only filename)
// and return its contents as a buffer of bytes
FileBuffer * platform_read_file(char * filename);

void platform_update_gpu_texture(
    int32_t texturearray_i,
    int32_t texture_i,
    DecodedImage * with_img);

#endif

