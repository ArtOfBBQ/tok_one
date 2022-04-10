#ifndef TEXT_H
#define TEXT_H

#include "bitmap_renderer.h"

void request_label_renderable(
    uint32_t font_texturearray_i,
    float font_height,
    char * text_to_draw,
    uint32_t text_to_draw_size,
    float left,
    float top,
    float max_width,
    float max_height);

#endif

