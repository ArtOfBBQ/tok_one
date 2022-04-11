#include "text.h"

uint32_t font_texturearray_i = 0;
float font_height = 0.03f;

void request_label_renderable(
    uint32_t with_id,
    char * text_to_draw,
    uint32_t text_to_draw_size,
    float left,
    float top,
    float max_width)
{
    float cur_left = left;
    float cur_top = top;
    
    for (
        uint32_t i = 0;
        i < text_to_draw_size;
        i++)
    {
        if (text_to_draw[i] == ' ')
        {
            cur_left += font_height;
            continue;
        }
        
        TexQuad letter;
        letter.object_id = with_id;
        letter.texturearray_i = font_texturearray_i;
        letter.texture_i = text_to_draw[i] - '0';
        for (
            uint32_t rgba_i = 0;
            rgba_i < 4;
            rgba_i++)
        {
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

