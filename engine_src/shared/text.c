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

typedef struct PrefetchedLine {
    float width;
    int32_t start_i;
    int32_t end_i;
} PrefetchedLine;

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

void request_label_offset_around(
    const int32_t with_id,
    const char * text_to_draw,
    const float mid_x_pixelspace,
    const float mid_y_pixelspace,
    const float pixelspace_extra_x_offset,
    const float pixelspace_extra_y_offset,
    const float z,
    const float max_width,
    const bool32_t ignore_camera)
{
    log_assert(max_width > 0.0f);
    
    float original_height = font_height;
    font_height *= z;
    
    uint32_t max_lines = 100;
    PrefetchedLine lines[max_lines];
    float widest_line_width = 0.0f;
    uint32_t lines_size = 1;
    
    for (uint32_t i = 0; i < 100; i++) {
        lines[i].width = 0.0f;
        lines[i].start_i = -1;
        lines[i].end_i = -1;
    }
    
    // *********************************************************
    // prefetch line widths and which character they start/end
    // *********************************************************
    uint32_t cur_line_i = 0;
    int32_t i = 0;
    lines[cur_line_i].start_i = 0;
    while (text_to_draw[i] != '\0') {
        lines[cur_line_i].width += get_advance_width(text_to_draw[i]);
        i++;
        
        if (text_to_draw[i] == '\n') {
            lines[cur_line_i].end_i = i;
            lines_size += 1;
            cur_line_i += 1;
            lines[cur_line_i].start_i = i + 1;
            log_assert(cur_line_i < max_lines);
            continue;
        }
        
        if (text_to_draw[i] == '\0') {
            break;
        }
        
        if (text_to_draw[i] == ' ') {
            if (
                lines[cur_line_i].width > 0.0f &&
                lines[cur_line_i].width
                    + get_next_word_width(text_to_draw + i) > max_width)
            {
                log_assert(lines[cur_line_i].width > 0.0f);
                log_assert(lines[cur_line_i].start_i < i);
                lines[cur_line_i].end_i = i;
                lines_size += 1;
                cur_line_i += 1;
                lines[cur_line_i].start_i = i + 1;
                log_assert(cur_line_i < max_lines);
                continue;
            }
        }
    }
    
    log_assert(text_to_draw[i] == '\0');
    lines[cur_line_i].end_i = i;
    if (lines[cur_line_i].end_i == lines[cur_line_i].start_i) {
        lines_size -= 1;
    }
    
    for (uint32_t line_i = 0; line_i < lines_size; line_i++) {
        if (lines[line_i].width > widest_line_width) {
            widest_line_width = lines[line_i].width;
        }
    }
    log_assert(widest_line_width > 0);
    
    zPolygon letter;
    
    float cur_y_offset_pixelspace =
        (font_height * 0.42f) +
        ((lines_size - 1) * font_height * 0.5f);
    
    for (uint32_t line_i = 0; line_i < lines_size; line_i++) {
        float cur_x_offset_pixelspace =
            (font_height * 0.5f) -
            (lines[line_i].width * 0.5f);
        
        for (
            int32_t j = lines[line_i].start_i;
            j <= lines[line_i].end_i;
            j++)
        {
            if (text_to_draw[j] == '\n') {
                continue;
            }
            
            if (text_to_draw[j] == '\0') {
                break;
            }
            
            construct_quad_around(
                /* const float left_x: */
                    screenspace_x_to_x(
                        mid_x_pixelspace,
                        z),
                /* const float bottom_y */
                    screenspace_y_to_y(
                        mid_y_pixelspace,
                        z),
                /* const float z: */
                    z,
                /* const float width: */
                    screenspace_width_to_width(font_height, z),
                /* const float height: */
                    screenspace_height_to_height(font_height, z),
                /* recipient: */
                    &letter);
            
            letter.ignore_lighting = font_ignore_lighting;
            letter.ignore_camera = ignore_camera;
            letter.object_id = with_id;
            
            if ((text_to_draw[j] - '!') < 0) {
                cur_x_offset_pixelspace += get_advance_width(text_to_draw[j]);
                continue;
            }
            
            for (
                uint32_t rgba_i = 0;
                rgba_i < 4;
                rgba_i++)
            {
                letter.triangle_materials[0].color[rgba_i] = font_color[rgba_i];
            }
            
            letter.x_offset = screenspace_width_to_width(
                (cur_x_offset_pixelspace +
                    get_left_side_bearing(text_to_draw[j]) +
                    pixelspace_extra_x_offset),
                z);
            letter.y_offset = screenspace_height_to_height(
                (cur_y_offset_pixelspace -
                    get_y_offset(text_to_draw[j]) -
                    (font_height * 0.5f) +
                    pixelspace_extra_y_offset),
                z);
            
            letter.triangle_materials[0].texturearray_i = font_texturearray_i;
            letter.triangle_materials[0].texture_i      = text_to_draw[j] - '!';
            
            cur_x_offset_pixelspace += get_advance_width(text_to_draw[j]);
            request_zpolygon_to_render(&letter);
        }
        cur_y_offset_pixelspace -= font_height;
    }
    
    font_height = original_height;
}

void request_label_around(
    const int32_t with_id,
    const char * text_to_draw,
    const float mid_x_pixelspace,
    const float mid_y_pixelspace,
    const float z,
    const float max_width,
    const bool32_t ignore_camera)
{
    request_label_offset_around(
        /* const int32_t with_id: */
            with_id,
        /* const char * text_to_draw: */
            text_to_draw,
        /* const float mid_x_pixelspace: */
            mid_x_pixelspace,
        /* const float mid_y_pixelspace: */
            mid_y_pixelspace,
        /* const float pixelspace_x_offset_for_each_character: */
            0.0f,
        /* const float pixelspace_y_offset_for_each_character: */
            0.0f,
        /* const float z: */
            z,
        /* const float max_width: */
            max_width,
        /* const bool32_t ignore_camera: */
            ignore_camera);
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
    float cur_x_offset = 0;
    float cur_y_offset = 0;
    
    uint32_t i = 0;
    // ignore leading ' '
    while (text_to_draw[i] == ' ') {
        i++;
    }
    
    zPolygon letter;
    
    float letter_width = screenspace_width_to_width(
        font_height * 0.8f, z);
    float letter_height = screenspace_height_to_height(
        font_height * 0.8f, z);
    
    while (text_to_draw[i] != '\0') {
        if (text_to_draw[i] == ' ') {
            cur_x_offset += font_height / 2;
            i++;
            
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
        
        construct_quad(
            /* const float left_x: */
                screenspace_x_to_x(left_pixelspace, z),
            /* const float bottom_y: */
                screenspace_y_to_y(top_pixelspace - font_height, z),
            /* const flota z: */
                z,
            /* const float width: */
                letter_width,
            /* const float height: */
                letter_height,
            /* recipient: */
                &letter);
        
        letter.object_id = with_id;
        letter.ignore_lighting = font_ignore_lighting;
        letter.ignore_camera = ignore_camera;
        
        letter.triangle_materials[0].texturearray_i = font_texturearray_i;
        letter.triangle_materials[0].texture_i = (int32_t)(text_to_draw[i] - '!');
        
        for (
            uint32_t rgba_i = 0;
            rgba_i < 4;
            rgba_i++)
        {
            letter.triangle_materials[0].color[rgba_i] = font_color[rgba_i];
        }

        letter.x_offset =
            screenspace_width_to_width(
                cur_x_offset + get_left_side_bearing(text_to_draw[i]), z);
        letter.y_offset =
            screenspace_height_to_height(
                cur_y_offset - get_y_offset(text_to_draw[i]), z);
        
        cur_x_offset += get_advance_width(text_to_draw[i]);
        if (left_pixelspace + cur_x_offset + get_advance_width('w') >= max_width)
        {
            cur_x_offset   = 0;
            cur_y_offset  -= font_height;
        }
        
        i++;
        request_zpolygon_to_render(&letter);
    }
}

void request_fps_counter(uint64_t microseconds_elapsed) {
    #ifdef __ARM_NEON
    char fps_string[12] = "NEONfps: xx";
    #elif defined(__AVX__)
    char fps_string[12] = "AVX fps: xx";
    #else
    char fps_string[12] = "std fps: xx";
    #endif
    
    uint64_t fps = 1000000 / microseconds_elapsed;
    
    if (fps < 100) {
        fps_string[ 9] = '0' + ((fps / 10) % 10);
        fps_string[10] = '0' + (fps % 10);
    } else {
        fps_string[ 9] = '9' + ((fps / 10) % 10);
        fps_string[10] = '9' + (fps % 10);
    }
    delete_zpolygon_object(FPS_COUNTER_OBJECT_ID);
    
    font_height = 16.0f;
    font_color[0] = 1.0f;
    font_color[1] = 1.0f;
    font_color[2] = 1.0f;
    font_color[3] = 1.0f;
    font_ignore_lighting = true;
    request_label_renderable(
        /* with_id               : */ FPS_COUNTER_OBJECT_ID,
        /* char * text_to_draw   : */ fps_string,
        /* float left_pixelspace : */ 20.0f,
        /* float top_pixelspace  : */ 30.0f,
        /* z                     : */ 1.0f,
        /* float max_width       : */ window_globals->window_width,
        /* bool32_t ignore_camera: */ true);
}
