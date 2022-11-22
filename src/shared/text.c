#include "text.h"

#pragma pack(push, 1)
typedef struct FontMetrics {
    float ascent;
    float descent;
    int32_t line_gap;
    float font_size;
    float scale_factor;
    
    float max_glyph_height;
    float max_glyph_width;
    // lineGap is the spacing between one row's descent and the next row's
    // ascent... so you should advance the vertical position by
    // "ascent - descent + lineGap"
    int32_t codepoints_in_font;
} FontMetrics;

typedef struct FontCodepoint {
    char character;
    int32_t x0;
    int32_t x1;
    int32_t y0;
    int32_t y1;
    int glyph_width;
    int glyph_height;
    int32_t advance_width;
    int32_t left_side_bearing;
} FontCodepoint;
#pragma pack(pop)

int32_t font_texturearray_i = 0;
float font_height = 30.0;
float font_color[4] = {1.0f, 1.0f, 1.0f, 1.0f};
bool32_t font_ignore_lighting = true;

FontMetrics * global_font_metrics = NULL;
FontCodepoint * codepoint_metrics = NULL;
uint32_t codepoint_metrics_size = 0;

void init_font(
    const char * raw_fontmetrics_file_contents,
    const uint64_t raw_fontmetrics_file_size)
{
    char * buffer_at = (char *)raw_fontmetrics_file_contents;
    log_assert(sizeof(FontMetrics) < raw_fontmetrics_file_size);
    
    global_font_metrics = (FontMetrics *)buffer_at;
    buffer_at += sizeof(FontMetrics);
    uint64_t filesize_remaining =
        raw_fontmetrics_file_size - sizeof(FontMetrics);
    
    codepoint_metrics_size = (uint32_t)global_font_metrics->codepoints_in_font;
    log_assert(filesize_remaining % sizeof(FontCodepoint) == 0);
    log_assert(
        filesize_remaining ==
            codepoint_metrics_size * sizeof(FontCodepoint));
    codepoint_metrics =
        (FontCodepoint *)buffer_at;
}

static float get_advance_width(const char input) {
    if (input == ' ') {
        return font_height * 0.5f;
    }
    
    if (input == '\0' || input == '\n') { return 0.0f; }
    
    uint32_t i = (uint32_t)(input - '!');
    // log_assert(codepoint_metrics[i].character == input);
    
    if (i >= codepoint_metrics_size) {
        i = (uint32_t)('m' - '!');
        log_assert(i < codepoint_metrics_size);
    }
    
    return
        (codepoint_metrics[i].advance_width *
            global_font_metrics->scale_factor *
                font_height) /
                    global_font_metrics->font_size;
}

static float get_left_side_bearing(const char input) {
    if (input == ' ' || input == '\0' || input == '\n') { return 0.0f; }
    
    uint32_t i = (uint32_t)(input - '!');
    // log_assert(codepoint_metrics[i].character == input);
    
    if (i >= codepoint_metrics_size) {
        i = (uint32_t)('m' - '!');
        log_assert(i < codepoint_metrics_size);
    }
    
    return
        (codepoint_metrics[i].left_side_bearing *
            global_font_metrics->scale_factor *
                font_height) /
                    global_font_metrics->font_size;
}

static float get_y_offset(const char input) {
    if (input == ' ' || input == '\0' || input == '\n') { return 0.0f; }
    
    uint32_t i = (uint32_t)(input - '!');
    if (i >= codepoint_metrics_size) { return 0.0f; }
    
    log_assert(codepoint_metrics[i].character == input);
    
    if (i >= codepoint_metrics_size) {
        i = (uint32_t)('m' - '!');
        log_assert(i < codepoint_metrics_size);
    }
    
    return
        ((-codepoint_metrics[i].y1 *
            global_font_metrics->scale_factor *
                font_height) /
                    global_font_metrics->font_size) +
        (font_height * 0.75f);
}

static uint32_t find_next_linebreak(
    const char * input,
    const uint32_t after_i)
{
    if (input[after_i] == '\0') { return after_i + 1; }
    
    uint32_t i = after_i + 1;
    
    while (input[i] != '\0') {
        if (input[i] == '\n') {
            return i;
        }
        i++;
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
    const int32_t with_id,
    const char * text_to_draw,
    const float mid_x_pixelspace,
    const float top_y_pixelspace,
    const float z,
    const float max_width,
    const bool32_t ignore_camera)
{
    log_assert(max_width > 0.0f);
    log_assert(with_id >= 0);
    
    uint32_t max_lines = 100;
    float line_widths[max_lines];
    float widest_line_width = 0.0f;
    uint32_t line_count = 1;
    
    float cur_left = 0.0f;
    float cur_top = top_y_pixelspace;
    
    // ****************************************
    // set widest_line_width and line_count
    // ****************************************
    uint32_t cur_line_i = 0;
    uint32_t i = 0;
    while (text_to_draw[i] != '\0') {
        line_widths[cur_line_i] += get_advance_width(text_to_draw[i]);            
        i++;
        
        if (text_to_draw[i] == '\n') {
            line_count += 1;
            cur_line_i += 1;
            log_assert(cur_line_i < max_lines);
            continue;
        }
        
        if (text_to_draw[i] == '\0') {
            break;
        }
        
        if (text_to_draw[i] == ' ') {
            if (
                line_widths[cur_line_i]
                    + get_next_word_width(text_to_draw + i) > max_width)
            {
                line_count += 1;
                cur_line_i += 1;
                continue;
            }
        }
    }
    
    printf("text to draw: %s\n", text_to_draw);
    for (uint32_t line_i = 0; line_i < line_count; line_i++) {
        printf("line %u has width: %f\n", line_i, line_widths[line_i]);
        if (line_widths[line_i] > widest_line_width) {
            log_assert(line_widths[line_i] > 0);
            widest_line_width = line_widths[line_i];
        }
    }
    printf("widest line width: %f\n", widest_line_width);
    log_assert(widest_line_width > 0);
    
    i = 0;
    cur_line_i = 0;
    float label_left = mid_x_pixelspace - (widest_line_width / 2);
    float cur_x_offset = (widest_line_width - line_widths[cur_line_i]) / 2;
    float cur_y_offset = 0;
    while (text_to_draw[i] != '\0') {
        
        if (text_to_draw[i] == '\n') {
            cur_line_i += 1;
            cur_y_offset -= font_height;
            cur_x_offset = (widest_line_width - line_widths[cur_line_i]) / 2;
            i++;
            continue;
        }
        
        if (text_to_draw[i] == ' ') {
            if (
                cur_x_offset +
                    get_next_word_width(text_to_draw + i + 1) > max_width)
            {
                cur_line_i += 1;
                cur_y_offset -= font_height;
                cur_x_offset = (widest_line_width - line_widths[cur_line_i]) / 2;
                i++;
                continue;
            }
        }
        
        TexQuad letter;
        construct_texquad(&letter);
        letter.ignore_lighting = font_ignore_lighting;
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
        
        letter.left_pixels = label_left;
        letter.x_offset = cur_x_offset + get_left_side_bearing(text_to_draw[i]);
        letter.top_pixels = top_y_pixelspace;
        letter.y_offset = cur_y_offset - get_y_offset(text_to_draw[i]);
        
        letter.height_pixels = font_height;
        letter.width_pixels = letter.height_pixels;
        letter.z = z;
        
        request_texquad_renderable(&letter);
        cur_x_offset += get_advance_width(text_to_draw[i]);
        i++;
    }
}

void request_label_renderable(
    const int32_t with_id,
    const char * text_to_draw,
    const float left_pixelspace,
    const float top_pixelspace,
    const float z,
    const float max_width,
    const bool32_t ignore_camera)
{
    log_assert(text_to_draw[0] != '\0');
    float cur_x_offset = left_pixelspace;
    float cur_y_offset = 0;
    
    uint32_t i = 0;
    // ignore leading ' '
    while (text_to_draw[i] == ' ') {
        i++;
    }
    
    while (text_to_draw[i] != '\0') {
        
        if (text_to_draw[i] == ' ') {
            cur_x_offset += font_height / 2;
            i++;
            
            // TODO: check if next word fits inside max_width
            float next_word_width = get_next_word_width(
                /* const char * text: */ text_to_draw + i);
            
            if (
                (left_pixelspace +
                    cur_x_offset +
                    next_word_width -
                    left_pixelspace) > max_width
                && (next_word_width < max_width))
            {
                cur_x_offset = 0;
                cur_y_offset -= font_height;
            }
            continue;
        }
        
        if (text_to_draw[i] == '\n') {
            cur_x_offset = 0;
            cur_y_offset -= font_height;
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
        letter.left_pixels = left_pixelspace;
        letter.top_pixels = top_pixelspace;
        letter.x_offset =
            cur_x_offset + get_left_side_bearing(text_to_draw[i]);
        letter.y_offset = cur_y_offset - get_y_offset(text_to_draw[i]);
        letter.height_pixels = font_height * 0.8f;
        letter.width_pixels = letter.height_pixels;
        letter.ignore_lighting = font_ignore_lighting;
        letter.ignore_camera = ignore_camera;
        letter.z = z;
        
        request_texquad_renderable(&letter);
        
        cur_x_offset += get_advance_width(text_to_draw[i]);
        if (left_pixelspace + cur_x_offset + get_advance_width('w') >= max_width)
        {
            cur_x_offset = 0;
            cur_y_offset  -= font_height;
        }
        
        i++;
    }
}

void request_fps_counter(uint64_t microseconds_elapsed) {
    
    // TODO: our timer is weirdly broken on iOS. Fix it!
    char fps_string[8] = "fps: xx";
    int32_t label_object_id = 0;
    uint64_t fps = 1000000 / microseconds_elapsed;
    /*
    float elapsed_mod =
        (float)((double)microseconds_elapsed / (double)16666);
    */
    if (fps < 100) {
        fps_string[5] = '0' + ((fps / 10) % 10);
        fps_string[6] = '0' + (fps % 10);
    } else {
        fps_string[5] = '9' + ((fps / 10) % 10);
        fps_string[6] = '9' + (fps % 10);
    }
    delete_texquad_object(label_object_id);
    
    font_height = 12.0f;
    font_ignore_lighting = true;
    request_label_renderable(
        /* with_id               : */ label_object_id,
        /* char * text_to_draw   : */ fps_string,
        /* float left_pixelspace : */ 20.0f,
        /* float top_pixelspace  : */ 60.0f,
        /* z                     : */ 5.0f,
        /* float max_width       : */ window_width,
        /* bool32_t ignore_camera: */ true);
}
