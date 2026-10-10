#ifndef T1_TEX_FILES_H
#define T1_TEX_FILES_H

#include <stdint.h>

#include "T1_types_public.h"
#include "T1_std.h"

/*
For "client" use in clientlogic.c or similar
*/
void T1_tex_files_reg_new_by_splitting_file(
    const c8 * filename,
    u32 rows,
    u32 columns,
    b8 free_rgba,
    char ** const fatal_error);

void T1_tex_files_load_font_images(
    char ** const fatal_error);

void T1_tex_files_prereg_and_decode_png_res(
    const char * resource_name,
    char ** const fatal_error);

void T1_tex_files_reg_new_by_splitting_file_error_handling(
    const c8 * filename,
    u32 rows, u32 columns,
    b8 free_rgba,
    char ** const fatal_error);

#if T1_TEXTURES_ACTIVE == T1_ACTIVE
void T1_tex_files_runtime_reg_png_from_writables(
    const c8 * filename,
    char ** const fatal_error);
#elif T1_TEXTURES_ACTIVE == T1_INACTIVE
#else
#error
#endif

void T1_tex_files_prereg_png_res(
    const c8 * filename,
    char ** const fatal_error);

void T1_tex_files_prereg_dds_res(
    const c8 * filename,
    char ** const fatal_error);

/*
Internal engine use
*/
void T1_tex_files_decode_all_prereg(
    u32 thread_id,
    u32 using_num_threads,
    char ** const fatal_error);

#endif // T1_TEX_FILES_H
