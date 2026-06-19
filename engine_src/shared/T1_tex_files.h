#ifndef T1_TEX_FILES_H
#define T1_TEX_FILES_H

#include <stdint.h>

#include "T1_public_types.h"
#include "T1_std.h"

/*
For "client" use in clientlogic.c or similar
*/
void
T1_tex_files_reg_new_by_splitting_file(
    const char * filename,
    const u32 rows,
    const u32 columns);

void
T1_tex_files_load_font_images(
    u8 * success,
    char * error_message);

void
T1_tex_files_reg_new_by_splitting_file_error_handling(
    const char * filename,
    const u32 rows,
    const u32 columns,
    u8 * success,
    char * error_message);

#if T1_TEXTURES_ACTIVE == T1_ACTIVE
void
T1_tex_files_runtime_reg_png_from_writables(
    const char * filename,
    u8 * good);
#elif T1_TEXTURES_ACTIVE == T1_INACTIVE
#else
#error
#endif

void T1_tex_files_prereg_png_res(
    const char * filename,
    b8 * good);

void T1_tex_files_prereg_dds_res(
    const char * filename,
    b8 * good);

/*
Internal engine use
*/
void T1_tex_files_decode_all_prereg(
    const u32 thread_id,
    const u32 using_num_threads);

#endif // T1_TEX_FILES_H
