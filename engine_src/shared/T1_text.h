#ifndef T1_TEXT_H
#define T1_TEXT_H

#include "T1_types_public.h"
#include "T1_texquad.h"

#ifdef __cplusplus
extern "C" {
#endif

extern T1TextFontSettings * T1_text_props;

void T1_text_init(
    void * (* arg_text_malloc_func)(u64 size),
    const char * raw_fontmetrics_file_contents,
    const u64 raw_fontmetrics_file_size);

void T1_text_request_label_offset_around(
    u32 with_id,
    const char * text_to_draw,
    f32 mid_x_pixelspace,
    f32 mid_y_pixelspace,
    f32 z,
    f32 max_width);

void T1_text_request_label_around_x_at_top_y(
    u32 with_object_id,
    const char * text_to_draw,
    f32 mid_x_pixelspace,
    f32 top_y_pixelspace,
    f32 z,
    f32 max_width);

void T1_text_request_label_around(
    u32 with_object_id,
    const char * text_to_draw,
    f32 mid_x_pixelspace,
    f32 mid_y_pixelspace,
    f32 z,
    f32 max_width);

void T1_text_request_label_renderable(
    u32 with_T1_id,
    const char * text_to_draw,
    f32 left_pixelspace,
    f32 top_y_pixelspace,
    f32 z,
    f32 tab_width,
    f32 max_width);

void T1_text_request_label_leftx_toplinemidy(
    u32 with_object_id,
    const char * text_to_draw,
    f32 left_pixelspace,
    f32 topline_mid_y_pixelspace,
    f32 z,
    f32 max_width);

void T1_text_request_debug_text(const char * text);
void T1_text_request_fps(u64 elapsed_us);

void T1_text_request_top_touch_id(
    u32 top_touchable_id);

#ifdef __cplusplus
}
#endif

#endif // T1_TEXT_H
