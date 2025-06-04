#ifndef TEXTURE_FILES_H
#define TEXTURE_FILES_H

#include "T1_memorystore.h"
#include "T1_decodedimage.h"
#include "T1_texture_array.h"
#include "T1_platform_layer.h"

/*
For "client" use in clientlogic.c or similar
*/

DecodedImage * malloc_img_from_filename(
    const char * filename);

void T1_texture_files_register_new_by_splitting_file(
    const char * filename,
    const uint32_t rows,
    const uint32_t columns);

void T1_texture_files_preregister_png_resource(
    const char * filename,
    uint32_t * good);


/*
Internal engine use
*/

void T1_texture_files_push_all_preregistered(void);

void T1_texture_files_decode_png_resource(
    const char * filename,
    uint32_t * good);

#endif // TEXTURE_FILES_H
