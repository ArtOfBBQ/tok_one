#ifndef TEXT_H
#define TEXT_H

#include "bitmap_renderer.h"

extern uint32_t font_texturearray_i;
extern float font_height;

void request_label_around(
    uint32_t with_object_id,
    char * text_to_draw,
    float text_color[4],
    uint32_t text_to_draw_size,
    float mid_x_pixelspace,
    float mid_y_pixelspace,
    float z,
    float max_width);

void request_label_renderable(
    uint32_t with_object_id,
    char * text_to_draw,
    float text_color[4],
    uint32_t text_to_draw_size,
    float left_pixelspace,
    float top_pixelspace,
    float z,
    float max_width);

#endif

