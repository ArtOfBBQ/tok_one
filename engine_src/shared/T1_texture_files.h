#ifndef TEXTURE_FILES_H
#define TEXTURE_FILES_H

#include "T1_memorystore.h"
#include "T1_decodedimage.h"
#include "T1_texture_array.h"
#include "T1_platform_layer.h"


/*
For "client" use in clientlogic.c or similar
*/
void T1_texture_files_register_new_by_splitting_file(
    const char * filename,
    const uint32_t rows,
    const uint32_t columns);

void T1_texture_files_runtime_register_png_from_writables(
    const char * filename,
    uint32_t * good);

void T1_texture_files_preregister_png_resource(
    const char * filename,
    uint32_t * good);

void T1_texture_files_preregister_dds_resource(
    const char * filename,
    uint32_t * good);

/*
Internal engine use
*/
void T1_texture_files_decode_all_preregistered(
    const uint32_t thread_id,
    const uint32_t using_num_threads);

#endif // TEXTURE_FILES_H
