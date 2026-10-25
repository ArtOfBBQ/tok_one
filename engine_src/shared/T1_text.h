#ifndef T1_TEXT_H
#define T1_TEXT_H

#include "T1_types_public.h"
#include "T1_texquad.h"

/*
The settings for the next label, the client sets them through
T1_text_set_f32() and T1_text_set_u32()
*/
typedef struct {
    T1GPUTexQuadf32 f32s;
    T1GPUTexQuadu32 u32s;
    float           font_height;
    uint32_t        highlight_i;
    uint32_t        highlight_size;
    uint32_t        opaque_back_active;
    float           tab_width; // in widths of the letter 'D'
} T1TextFontSettings;

extern T1TextFontSettings * T1_text_props;

void T1_text_init(
    void * (* arg_text_malloc_func)(size_t size),
    const char * raw_fontmetrics_file_contents,
    const u64 raw_fontmetrics_file_size);

void T1_text_request_label(
    u32 with_T1_id,
    const char * text_to_draw,
    T1TextAlign align,
    f32 screen_x,
    f32 screen_y,
    f32 z,
    f32 max_width);

/*
T1_TEXT_ALIGN_LEFT_TOP with any tab width, for T1's own labels (terminal,
fps counter)
*/
void T1_text_request_label_left_top(
    u32 with_T1_id,
    const char * text_to_draw,
    f32 left_pixelspace,
    f32 top_y_pixelspace,
    f32 z,
    f32 tab_width,
    f32 max_width);

void T1_text_draw_label_around(
    u8 * on_rgba, u32 rgba_w, u32 rgba_h,
    const char * text_to_draw,
    f32 mid_x_pixelspace,
    f32 mid_y_pixelspace,
    f32 max_width);

void T1_text_request_debug_text(const char * text);
void T1_text_request_fps(u64 elapsed_us);

void T1_text_request_top_touch_id(
    u32 top_touchable_id);

#endif // T1_TEXT_H
