#include "text.h"

uint32_t font_texturearray_i = 0;
float font_height = 40.0f;

void request_label_renderable(
    uint32_t with_id,
    char * text_to_draw,
    float text_color[4],
    uint32_t text_to_draw_size,
    float left_pixelspace,
    float top_pixelspace,
    float z,
    float max_width)
{
    float cur_left = left_pixelspace;
    float cur_top = top_pixelspace;
    
    assert(text_color[3] >= 0.0f);
    assert(text_color[3] <= 1.0f);
    
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
            letter.RGBA[rgba_i] = text_color[rgba_i];
        }
        
        letter.left_pixels = cur_left;
        letter.top_pixels = cur_top;
        letter.height_pixels = font_height;
        letter.width_pixels = font_height;
        letter.z_angle = 0.0f;
        letter.visible = true;
        letter.deleted = false;
        
        request_texquad_renderable(&letter);
        
        cur_left += font_height;
    }
}

