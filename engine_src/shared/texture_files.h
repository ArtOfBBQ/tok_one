#ifndef TEXTURE_FILES_H
#define TEXTURE_FILES_H

#include "texture_array.h"

#include "platform_layer.h"

void texture_files_preregister_png(
    const char * filename,
    uint32_t * good);

void texture_files_push_png(
    const char * filename,
    uint32_t * good);

#endif // TEXTURE_FILES_H
