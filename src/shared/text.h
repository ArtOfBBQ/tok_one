#ifndef TEXT_H
#define TEXT_H

#include "bitmap_renderer.h"

extern uint32_t font_texturearray_i;
extern float font_height;

void request_label_around(
    const uint32_t with_object_id,
    const char * text_to_draw,
    const float text_color[4],
    const uint32_t text_to_draw_size,
    const float mid_x_pixelspace,
    const float mid_y_pixelspace,
    const float z,
    const float max_width,
    const bool32_t ignore_camera);

void request_label_renderable(
    const uint32_t with_object_id,
    const char * text_to_draw,
    const float text_color[4],
    const uint32_t text_to_draw_size,
    const float left_pixelspace,
    const float top_pixelspace,
    const float z,
    const float max_width,
    const bool32_t ignore_camera);

#endif

