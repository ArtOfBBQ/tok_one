/*
These functions must be provided by the platform layer!
*/

#ifndef PLATFORM_LAYER_H
#define PLATFORM_LAYER_H

#include "inttypes.h"

typedef struct FileBuffer {
    uint32_t size;
    char * contents;
} FileBuffer;

// Read a file (without path, only filename)
// and return its contents as a buffer of bytes
FileBuffer * platform_read_file(char * filename);

#endif

