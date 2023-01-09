#ifndef TEXT_H
#define TEXT_H

#include "platform_layer.h"
#include "zpolygon.h"

#ifdef __cplusplus
extern "C" {
#endif

extern int32_t font_texturearray_i;
extern float font_height;
extern float font_color[4];
extern bool32_t font_ignore_lighting;

void init_font(
    const char * raw_fontmetrics_file_contents,
    const uint64_t raw_fontmetrics_file_size);

void request_label_around(
    const int32_t with_object_id,
    const char * text_to_draw,
    const float mid_x_pixelspace,
    const float mid_y_pixelspace,
    const float z,
    const float max_width,
    const bool32_t ignore_camera);

void request_label_renderable(
    const int32_t with_object_id,
    const char * text_to_draw,
    const float left_pixelspace,
    const float top_pixelspace,
    const float z,
    const float max_width,
    const bool32_t ignore_camera);

void request_fps_counter(uint64_t microseconds_elapsed);

#ifdef __cplusplus
}
#endif

#endif
