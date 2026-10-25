#include "T1_tex_files.h"

#include "T1_embedded_data.h"
#include "T1_mem.h"
#include "T1_log.h"
#include "T1_img.h"
#include "T1_tex_array.h"

/*
T1's font is white, so concat.c embeds only its alpha channel: u32 width,
u32 height, then (count, alpha) byte pairs covering the image row by row
*/
static void malloc_font_img_from_alpha_rle(
    T1Img * recip,
    char ** const fatal_error)
{
    if (*fatal_error != 0) { return; }
    
    const u8 * rle = T1_embedded_data_font_alpha_rle;
    const u32 rle_size = T1_embedded_data_font_alpha_rle_size;
    
    if (rle_size < 8) {
        *fatal_error = "Embedded font is missing";
        return;
    }
    
    T1_std_memset(recip, 0, sizeof(T1Img));
    T1_std_memcpy(&recip->width, rle, 4);
    T1_std_memcpy(&recip->height, rle + 4, 4);
    recip->pixel_count = recip->width * recip->height;
    recip->rgba_values_size = recip->pixel_count * 4;
    T1_mem_malloc_managed_page_aligned(
        /* void *base_pointer_for_freeing: */
            (void *)&recip->rgba_values_freeable,
        /* void *aligned_subptr: */
            (void *)&recip->rgba_values_page_aligned,
        /* const u64 subptr_size: */
            recip->rgba_values_size);
    if (recip->rgba_values_page_aligned == NULL) {
        *fatal_error = "Malloc fail";
        return;
    }
    
    u8 * rgba = recip->rgba_values_page_aligned;
    u32 px = 0;
    for (u32 i = 8; i + 1 < rle_size; i += 2) {
        u8 count = rle[i];
        u8 alpha = rle[i + 1];
        if (px + count > recip->pixel_count) { break; }
        for (u8 c = 0; c < count; c++) {
            rgba[(px * 4) + 0] = 255;
            rgba[(px * 4) + 1] = 255;
            rgba[(px * 4) + 2] = 255;
            rgba[(px * 4) + 3] = alpha;
            px++;
        }
    }
    
    if (px != recip->pixel_count) {
        *fatal_error = "Embedded font is corrupt";
        return;
    }
}

void T1_tex_files_load_font_images(
    char ** const fatal_error)
{
    if (*fatal_error != 0) { return; }
    
    if (T1_tex_arrays_size != 0) {
        *fatal_error = "Font error - other textures already existed";
        return;
    }
    
    T1Img font_img;
    malloc_font_img_from_alpha_rle(&font_img, fatal_error);
    if (*fatal_error != 0) { return; }
    
    // the glyphs stay forever, the whole image is only needed to split it
    T1_tex_array_reg_new_by_splitting_img(
        /* T1Img * new_image: */ &font_img,
        /* u32 rows: */ 10,
        /* u32 columns: */ 10,
        /* b8 never_delete: */ 1,
        fatal_error);
    T1_mem_free_managed(font_img.rgba_values_freeable);
    if (*fatal_error != 0) { return; }
    T1_tex_arrays[0].request_init = 0;
}
