#include "text.h"

#pragma pack(push, 1)
typedef struct FontCodepointOffset {
    char character;
    int advance_width;
    int left_side_bearing;
    int y_offset;
} FontCodepointOffset;

typedef struct FontMetrics {
    // a global multiplier for all font metrics
    float scale_factor;
    // the font size that was used when generating the bitmaps
    float font_size_at_generation;
    // lineGap is the spacing between one row's descent and the next row's
    // ascent... so you should advance the vertical position by
    // "ascent - descent + lineGap"
    int linegap;
    int codepoints_in_font;
} FontMetrics;
#pragma pack(pop)

int32_t font_texturearray_i = 0;
float font_height = 40.0;
float font_color[4] = {1.0f, 1.0f, 1.0f, 1.0f};

uint32_t font_codepoint_offsets_size = 0;
FontMetrics * font_metrics = NULL;
FontCodepointOffset * font_codepoint_offsets = NULL;

void init_font(
    const char * raw_fontmetrics_file_contents,
    const uint64_t raw_fontmetrics_file_size)
{
    char * buffer_at = (char *)raw_fontmetrics_file_contents;
    log_assert(sizeof(FontMetrics) < raw_fontmetrics_file_size);
    font_metrics = (FontMetrics *)buffer_at;
    buffer_at += sizeof(FontMetrics);
    uint64_t filesize_remaining =
        raw_fontmetrics_file_size - sizeof(FontMetrics);
    
    font_codepoint_offsets_size = (uint32_t)font_metrics->codepoints_in_font;
    log_assert(
        filesize_remaining % sizeof(FontCodepointOffset) == 0);
    log_assert(
        filesize_remaining ==
            font_codepoint_offsets_size * sizeof(FontCodepointOffset));
    font_codepoint_offsets =
        (FontCodepointOffset *)buffer_at;
}

static float get_advance_width(
    const char input)
{
    if (input == ' ') {
        return font_height * 0.5f;
    }
    
    if (input == '\0' || input == '\n') { return 0.0f; }
    
    uint32_t i = (uint32_t)(input - '!');
    log_assert(font_codepoint_offsets[i].character == input);
    
    return
        (font_codepoint_offsets[i].advance_width *
            font_metrics->scale_factor *
                font_height) / 40.0f;
}

static float get_left_side_bearing(const char input)
{
    if (input == ' ' || input == '\0' || input == '\n') { return 0.0f; }
    
    uint32_t i = (uint32_t)(input - '!');
    log_assert(font_codepoint_offsets[i].character == input);
    
    return
        (font_codepoint_offsets[i].left_side_bearing *
            font_metrics->scale_factor *
                font_height) / 40.0f;
}

static float get_y_offset(const char input)
{
    if (input == ' ' || input == '\0' || input == '\n') { return 0.0f; }
    
    uint32_t i = (uint32_t)(input - '!');
    log_assert(font_codepoint_offsets[i].character == input);
    
    return ((float)font_codepoint_offsets[i].y_offset
        * 0.5f
        * font_height)
            / 40.0f;
}

static uint32_t find_next_linebreak(
    const char * input,
    const uint32_t input_size,
    const uint32_t after_i)
{
    log_assert(input_size > 1);
    log_assert(after_i < input_size - 1);
    
    uint32_t i = after_i + 1;
    
    log_assert(after_i < input_size);
    
    for (; i < input_size; i++) {
        if (input[i] == '\n')
        {
            return i;
        }
    }
    
    return i;
}

static float get_next_word_width(const char * text) {
    float return_value = 0.0f;
    
    uint32_t i = 0;
    
    while(text[i] == ' ') {
        return_value += get_advance_width(text[i]);
        i++;
    }
    
    while (text[i] != ' ' && text[i] != '\0' && text[i] != '\n') {
        return_value += get_advance_width(text[i]);
        i++;
    }
    
    return return_value;
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
    log_assert(max_width > 0.0f);
    
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
        
        // calculate width of current line
        line_width = 0.0f;
        uint32_t i = line_start_i;
        while (true) {
            line_width += get_advance_width(text_to_draw[i]);
            
            log_assert(line_width <= max_width);
            
            i++;
            if (text_to_draw[i] == '\0') {
                break;
            }
            
            if (text_to_draw[i] == ' ') {
                log_assert(get_next_word_width(text_to_draw + i) > 0.0f);
                
                if (
                    line_width
                    + get_next_word_width(text_to_draw + i) > max_width)
                {
                    line_end_i = i;
                    break;
                }
            }
        }
        
        cur_left = mid_x_pixelspace - (0.5f * line_width);
        for (
            uint32_t i = line_start_i;
            i <= line_end_i;
            i++)
        {
            if (text_to_draw[i] == ' ') {
                cur_left += (font_height / 2);
                continue;
            }
            
            TexQuad letter;
            construct_texquad(&letter);
            letter.ignore_lighting = true;
            letter.ignore_camera = ignore_camera;
            letter.object_id = with_id;
            letter.texturearray_i = font_texturearray_i;
            letter.texture_i = text_to_draw[i] - '!';
            for (
                uint32_t rgba_i = 0;
                rgba_i < 4;
                rgba_i++)
            {
                letter.RGBA[rgba_i] = font_color[rgba_i];
            }
            
            letter.left_pixels = cur_left + get_left_side_bearing(text_to_draw[i]);
            letter.top_pixels = cur_top - get_y_offset(text_to_draw[i]);
            letter.height_pixels = font_height;
            letter.width_pixels = font_height;
            letter.z = z;
            
            request_texquad_renderable(&letter);
            cur_left += get_advance_width(text_to_draw[i]);
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
    log_assert(text_to_draw[0] != '\0');
    float cur_left = left_pixelspace;
    float cur_top = top_pixelspace;
    
    uint32_t i = 0;
    // ignore leading ' '
    while (text_to_draw[i] == ' ') {
        i++;
    }
    
    while (text_to_draw[i] != '\0')
    {
        if (text_to_draw[i] == ' ')
        {
            cur_left += (font_height / 2);
            i++;

            // TODO: check if next word fits inside max_width
            if (
                ((cur_left +
                    get_next_word_width(
                        /* const char * text: */ text_to_draw + i) -
                            left_pixelspace) > max_width))
            {
                cur_left = left_pixelspace;
                cur_top -= font_height;
            }
            continue;
        }
        
        if (text_to_draw[i] == '\n')
        {
            cur_left = left_pixelspace;
            cur_top -= font_height;
            i++;
            continue;
        }
        
        TexQuad letter;
        construct_texquad(&letter);
        letter.object_id = with_id;
        letter.texturearray_i = font_texturearray_i;
        letter.texture_i = (int32_t)(text_to_draw[i] - '!');
        
        for (
            uint32_t rgba_i = 0;
            rgba_i < 4;
            rgba_i++)
        {
            letter.RGBA[rgba_i] = font_color[rgba_i];
        }
        
        letter.left_pixels = cur_left + get_left_side_bearing(text_to_draw[i]);
        letter.top_pixels = cur_top - get_y_offset(text_to_draw[i]);
        letter.height_pixels = font_height;
        letter.width_pixels = font_height;
        letter.ignore_lighting = true;
        letter.ignore_camera = ignore_camera;
        letter.z = z;
        
        request_texquad_renderable(&letter);
        
        cur_left += get_advance_width(text_to_draw[i]);
        
        i++;
    }
}
