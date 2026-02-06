#ifndef T1_TEXT_H
#define T1_TEXT_H

#include "T1_platform_layer.h"
#include "T1_texquad.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct FontSettings {
    T1GPUTexQuadf32 f32;
    T1GPUTexQuadi32 i32;
    float font_height;
} T1TextFontSettings;

extern T1TextFontSettings * T1_text_props;

void T1_text_init(
    void * (* arg_text_malloc_func)(size_t size),
    const char * raw_fontmetrics_file_contents,
    const uint64_t raw_fontmetrics_file_size);

void T1_text_request_label_offset_around(
    const int32_t with_id,
    const char * text_to_draw,
    const float mid_x_pixelspace,
    const float mid_y_pixelspace,
    const float z,
    const float max_width);

void T1_text_request_label_around_x_at_top_y(
    const int32_t with_object_id,
    const char * text_to_draw,
    const float mid_x_pixelspace,
    const float top_y_pixelspace,
    const float z,
    const float max_width);

void T1_text_request_label_around(
    const int32_t with_object_id,
    const char * text_to_draw,
    const float mid_x_pixelspace,
    const float mid_y_pixelspace,
    const float z,
    const float max_width);

void T1_text_request_label_renderable(
    const int32_t with_object_id,
    const char * text_to_draw,
    const float left_pixelspace,
    const float mid_y_pixelspace,
    const float z,
    const float max_width);

void T1_text_request_debug_text(const char * text);
void T1_text_request_fps(uint64_t elapsed_us);

void T1_text_request_top_touch_id(
    int32_t top_touchable_id);

#ifdef __cplusplus
}
#endif

#endif // T1_TEXT_H
