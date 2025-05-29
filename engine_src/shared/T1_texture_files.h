#ifndef TEXTURE_FILES_H
#define TEXTURE_FILES_H

#include "T1_memorystore.h"
#include "T1_decodedimage.h"
#include "T1_texture_array.h"
#include "T1_platform_layer.h"

DecodedImage * malloc_img_from_filename(
    const char * filename);

void texture_array_register_new_by_splitting_file(
    const char * filename,
    const uint32_t rows,
    const uint32_t columns);

void texture_files_preregister_png_resource(
    const char * filename,
    uint32_t * good);

void texture_files_decode_png_resource(
    const char * filename,
    uint32_t * good);

#endif // TEXTURE_FILES_H
