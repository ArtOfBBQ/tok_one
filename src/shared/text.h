#ifndef TEXT_H
#define TEXT_H

#include "bitmap_renderer.h"

extern uint32_t font_texturearray_i;
extern float font_height;

void request_label_renderable(
    uint32_t with_object_id,
    char * text_to_draw,
    uint32_t text_to_draw_size,
    float left,
    float top,
    float max_width);

#endif

