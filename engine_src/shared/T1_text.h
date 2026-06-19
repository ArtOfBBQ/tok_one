#ifndef T1_TEXT_H
#define T1_TEXT_H

#include "T1_public_types.h"
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
    const s32 with_id,
    const char * text_to_draw,
    const f32 mid_x_pixelspace,
    const f32 mid_y_pixelspace,
    const f32 z,
    const f32 max_width);

void T1_text_request_label_around_x_at_top_y(
    const s32 with_object_id,
    const char * text_to_draw,
    const f32 mid_x_pixelspace,
    const f32 top_y_pixelspace,
    const f32 z,
    const f32 max_width);

void T1_text_request_label_around(
    const s32 with_object_id,
    const char * text_to_draw,
    const f32 mid_x_pixelspace,
    const f32 mid_y_pixelspace,
    const f32 z,
    const f32 max_width);

void T1_text_request_label_renderable(
    const s32 with_object_id,
    const char * text_to_draw,
    const f32 left_pixelspace,
    const f32 top_y_pixelspace,
    const f32 z,
    const f32 tab_width,
    const f32 max_width);

void T1_text_request_label_leftx_toplinemidy(
    const s32 with_object_id,
    const char * text_to_draw,
    const f32 left_pixelspace,
    const f32 topline_mid_y_pixelspace,
    const f32 z,
    const f32 max_width);

void T1_text_request_debug_text(const char * text);
void T1_text_request_fps(u64 elapsed_us);

void T1_text_request_top_touch_id(
    s32 top_touchable_id);

#ifdef __cplusplus
}
#endif

#endif // T1_TEXT_H
