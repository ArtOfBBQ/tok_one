#ifndef TEXT_H
#define TEXT_H

#include "platform_layer.h"
#include "zpolygon.h"

#ifdef __cplusplus
extern "C" {
#endif

extern int32_t font_texturearray_i;
extern int32_t font_touchable_id;
extern float font_height;
extern float font_color[4];
extern bool32_t font_ignore_lighting;

void init_font(
    const char * raw_fontmetrics_file_contents,
    const uint64_t raw_fontmetrics_file_size);

/*
This function includes offsets, which is only useful if you
are setting up your label as part of a group of objects and
you're intending to move the object (that includes this label)
to a new location later with a ScheduledAnimation. It will then
retain the offsets. If you don't care about this just use the
function below which keeps all offsets at 0
*/
void text_request_label_offset_around(
    const int32_t with_id,
    const char * text_to_draw,
    const float mid_x_pixelspace,
    const float mid_y_pixelspace,
    const float z,
    const float max_width,
    const bool32_t ignore_camera);

void text_request_label_around_x_at_top_y(
    const int32_t with_object_id,
    const char * text_to_draw,
    const float mid_x_pixelspace,
    const float top_y_pixelspace,
    const float z,
    const float max_width,
    const bool32_t ignore_camera);

void text_request_label_around(
    const int32_t with_object_id,
    const char * text_to_draw,
    const float mid_x_pixelspace,
    const float mid_y_pixelspace,
    const float z,
    const float max_width,
    const bool32_t ignore_camera);

void text_request_label_renderable(
    const int32_t with_object_id,
    const char * text_to_draw,
    const float left_pixelspace,
    const float top_pixelspace,
    const float z,
    const float max_width,
    const bool32_t ignore_camera);

void text_request_fps_counter(uint64_t microseconds_elapsed);

#ifdef __cplusplus
}
#endif

#endif
