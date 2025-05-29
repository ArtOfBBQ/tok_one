#ifndef TEXTURE_FILES_H
#define TEXTURE_FILES_H

#include "T1_memorystore.h"
#include "T1_texture_array.h"
#include "T1_platform_layer.h"

void texture_files_preregister_png_resource(
    const char * filename,
    uint32_t * good);

void texture_files_decode_png_resource(
    const char * filename,
    uint32_t * good);

#endif // TEXTURE_FILES_H
