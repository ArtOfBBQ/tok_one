#ifndef T1_TEX_FILES_H
#define T1_TEX_FILES_H

#include "T1_mem.h"
#include "T1_img.h"
#include "T1_tex_array.h"
#include "T1_platform_layer.h"


/*
For "client" use in clientlogic.c or similar
*/
void
T1_tex_files_reg_new_by_splitting_file(
    const char * filename,
    const uint32_t rows,
    const uint32_t columns);

void
T1_tex_files_load_font_images(
    bool32_t * success,
    char * error_message);

void
T1_tex_files_reg_new_by_splitting_file_error_handling(
    const char * filename,
    const uint32_t rows,
    const uint32_t columns,
    bool32_t * success,
    char * error_message);

#if T1_TEXTURES_ACTIVE == T1_ACTIVE
void
T1_tex_files_runtime_reg_png_from_writables(
    const char * filename,
    uint32_t * good);
#elif T1_TEXTURES_ACTIVE == T1_INACTIVE
#else
#error
#endif

void T1_tex_files_prereg_png_res(
    const char * filename,
    uint32_t * good);

void T1_tex_files_prereg_dds_res(
    const char * filename,
    uint32_t * good);

/*
Internal engine use
*/
void T1_tex_files_decode_all_prereg(
    const uint32_t thread_id,
    const uint32_t using_num_threads);

#endif // T1_TEX_FILES_H
