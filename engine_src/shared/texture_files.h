#ifndef TEXTURE_FILES_H
#define TEXTURE_FILES_H

#include "memorystore.h"
#include "texture_array.h"
#include "platform_layer.h"

void texture_files_preregister_png_resource(
    const char * filename,
    uint32_t * good);

void texture_files_decode_png_resource(
    const char * filename,
    uint32_t * good);

#endif // TEXTURE_FILES_H
