#include "text.h"

int32_t font_texturearray_i = 0;
float font_height = 40.0f;
float font_color[4] = {1.0f, 1.0f, 1.0f, 1.0f};

static float get_char_width(
    const char * input)
{
    return font_height;
}

static uint32_t find_next_linebreak(
    const char * input,
    const uint32_t input_size,
    const uint32_t after_i)
{
    assert(input_size > 1);
    assert(after_i < input_size - 1);
    
    uint32_t i = after_i + 1;
    
    assert(after_i < input_size);
    
    for (; i < input_size; i++) {
        if (input[i] == '\n')
        {
            return i;
        }
    }
    
    return i;
}

void request_label_around(
    const uint32_t with_id,
    const char * text_to_draw,
    const uint32_t text_to_draw_size,
    const float mid_x_pixelspace,
    const float top_y_pixelspace,
    const float z,
    const float max_width,
    const bool32_t ignore_camera)
{
    assert(max_width > 0.0f);
    
    uint32_t line_start_i = 0;
    uint32_t line_end_i = 0;
    float line_width = 0.0f;
    
    float cur_left = 0.0f;
    float cur_top = top_y_pixelspace;
    
    while (line_start_i < (text_to_draw_size - 2))
    {
        line_end_i = find_next_linebreak(
            /* input      : */ text_to_draw,
            /* input_size : */ text_to_draw_size,
            /* after_i    : */ line_start_i);
        
        if (line_end_i <= line_start_i)
        {
            break;
        }
        
        // enforce maximum line width 
        line_width = 0.0f;
        uint32_t previous_space_i = line_start_i;
        for (uint32_t i = line_start_i; i <= line_end_i; i++) {
            if (text_to_draw[i] == ' ') {
                previous_space_i = i;
            }
            
            line_width += get_char_width(&text_to_draw[i]);
            if (line_width > max_width) {
                line_end_i = previous_space_i == line_start_i ?
                    i - 1 : previous_space_i;
            }
        }
        
        cur_left = mid_x_pixelspace - (0.5f * line_width);
        for (
            uint32_t i = line_start_i;
            i <= line_end_i;
            i++)
        {
            TexQuad letter;
            construct_texquad(&letter);
            letter.ignore_lighting = true;
            letter.ignore_camera = ignore_camera;
            letter.object_id = with_id;
            letter.texturearray_i = font_texturearray_i;
            letter.texture_i = text_to_draw[i] - '0';
            for (
                uint32_t rgba_i = 0;
                rgba_i < 4;
                rgba_i++)
            {
                letter.RGBA[rgba_i] = font_color[rgba_i];
            }
            
            letter.left_pixels = cur_left;
            letter.top_pixels = cur_top;
            letter.height_pixels = font_height;
            letter.width_pixels = font_height;
            letter.z = z;
            
            request_texquad_renderable(&letter);
            cur_left += get_char_width(&text_to_draw[i]);
        }
        
        cur_top -= font_height;
        line_start_i = line_end_i;
    }
}

void request_label_renderable(
    const uint32_t with_id,
    const char * text_to_draw,
    const float left_pixelspace,
    const float top_pixelspace,
    const float z,
    const float max_width,
    const bool32_t ignore_camera)
{
    float cur_left = left_pixelspace;
    float cur_top = top_pixelspace;
    
    uint32_t i = 0;
    while (text_to_draw[i] != '\0')
    {
        if (text_to_draw[i] == ' ')
        {
            cur_left += font_height;
            i++;
            continue;
        }
        
        if (text_to_draw[i] == '\n')
        {
            cur_left = left_pixelspace;
            cur_top -= font_height;
            i++;
            continue;
        }

        if ((cur_left - left_pixelspace) > max_width) {
            cur_left = left_pixelspace;
            cur_top -= font_height;
        }
        
        TexQuad letter;
        construct_texquad(&letter);
        letter.object_id = with_id;
        letter.texturearray_i = font_texturearray_i;
        letter.texture_i = text_to_draw[i] - '0';
        for (
            uint32_t rgba_i = 0;
            rgba_i < 4;
            rgba_i++)
        {
            letter.RGBA[rgba_i] = font_color[rgba_i];
        }
        
        letter.left_pixels = cur_left;
        letter.top_pixels = cur_top;
        letter.height_pixels = font_height;
        letter.width_pixels = font_height;
        letter.ignore_lighting = true;
        letter.ignore_camera = ignore_camera;
        letter.z = z;
        
        request_texquad_renderable(&letter);
        
        cur_left += get_char_width(&text_to_draw[i]);
        
        i++;
    }
}

