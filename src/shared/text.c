#include "text.h"

void request_label_renderable(
    uint32_t font_texturearray_i,
    float font_height,
    char * text_to_draw,
    uint32_t text_to_draw_size,
    float left,
    float top,
    float max_width,
    float max_height)
{
    float cur_left = left;
    float cur_top = top;
    
    for (uint32_t i = 0; i < text_to_draw_size; i++) {
        if (text_to_draw[i] == ' ') {
            cur_left += font_height;
            continue;
        }
        
        TexQuad letter;
        letter.texturearray_i = font_texturearray_i;
        letter.texture_i = text_to_draw[i] - '0';
        printf(
            "requesting letter: %c, got %u\n",
            text_to_draw[i],
            letter.texture_i);
        for (uint32_t rgba_i = 0; rgba_i < 4; rgba_i++) {
            letter.RGBA[rgba_i] = 1.0f;
        }
        letter.left = cur_left;
        letter.top = cur_top;
        letter.height = font_height;
        letter.width = font_height;
        letter.visible = true;
        letter.deleted = false;
        
        request_texquad_renderable(&letter);
        
        cur_left += font_height;
    }
}

