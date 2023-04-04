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
    const float pixelspace_x_offset_for_each_character,
    const float pixelspace_y_offset_for_each_character,
    const float z,
    const float max_width,
    const bool32_t ignore_camera)
{
    log_assert(max_width > 0.0f);
    log_assert(with_id >= 0);
    
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
    
    zPolygon label;
    zPolygon letter;
    construct_zpolygon(&label);
    label.ignore_lighting = font_ignore_lighting;
    label.ignore_camera = ignore_camera;
    label.object_id = with_id;
    // float label_left = mid_x_pixelspace - (widest_line_width / 2);
    label.x = screenspace_x_to_x(
        mid_x_pixelspace,
        z);
    label.y = screenspace_y_to_y(
        mid_y_pixelspace,
        z);
    label.z = z;
    label.triangles_size = 0;
    
    float cur_y_offset_pixelspace =
        (((lines_size - 1) * font_height) / 2) +
        pixelspace_y_offset_for_each_character;
    for (uint32_t line_i = 0; line_i < lines_size; line_i++) {
        float cur_x_offset_pixelspace =
            pixelspace_x_offset_for_each_character +
                (-1.0f * ((lines[line_i].width) / 2) +
                (font_height / 2));
        
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
            
            construct_quad(
                /* const float left_x: */
                    0,
                /* const float top_y: */
                     0,
                /* const float z: */
                    label.z,
                /* const float width: */
                    screenspace_width_to_width(font_height, label.z),
                /* const float height: */
                    screenspace_height_to_height(font_height, label.z),
                /* recipient: */
                    &letter);
            letter.z = 0.0f;
            
            if ((text_to_draw[j] - '!') < 0) {
                cur_x_offset_pixelspace += get_advance_width(text_to_draw[j]);
                continue;
            }
            for (
                uint32_t rgba_i = 0;
                rgba_i < 4;
                rgba_i++)
            {
                letter.triangles[0].color[rgba_i] = font_color[rgba_i];
                letter.triangles[1].color[rgba_i] = font_color[rgba_i];
            }
            
            float letter_x_offset = screenspace_width_to_width(
                (cur_x_offset_pixelspace + get_left_side_bearing(text_to_draw[j])),
                label.z);
            float letter_y_offset = screenspace_height_to_height(
                (cur_y_offset_pixelspace - get_y_offset(text_to_draw[j])),
                label.z);
            letter.z = 0;
            
            for (uint32_t m = 0; m < 3; m++) {
                letter.triangles[0].vertices[m].x += letter_x_offset;
                letter.triangles[0].vertices[m].y += letter_y_offset;
                letter.triangles[1].vertices[m].x += letter_x_offset;
                letter.triangles[1].vertices[m].y += letter_y_offset;
            }
            
            letter.triangles[0].texturearray_i = font_texturearray_i;
            letter.triangles[0].texture_i      = text_to_draw[j] - '!';
            letter.triangles[1].texturearray_i = font_texturearray_i;
            letter.triangles[1].texture_i      = text_to_draw[j] - '!';
            
            log_assert(label.triangles_size + 1 < POLYGON_TRIANGLES_SIZE);
            label.triangles[label.triangles_size    ] = letter.triangles[0];
            label.triangles[label.triangles_size + 1] = letter.triangles[1];
            label.triangles_size += 2;
            
            cur_x_offset_pixelspace += get_advance_width(text_to_draw[j]);
        }
        cur_y_offset_pixelspace -= font_height;
    }
    
    if (label.triangles_size > 0) {
        request_zpolygon_to_render(&label);
    } else {
        log_append("Warning: no triangles in label made of text: ");
        log_append(text_to_draw);
        log_append_char('\n');
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
    
    zPolygon label_to_render;
    construct_zpolygon(&label_to_render);
    zPolygon letter;
    label_to_render.object_id = with_id;
    label_to_render.x = screenspace_x_to_x(left_pixelspace, z);
    label_to_render.y = screenspace_y_to_y(top_pixelspace, z);
    label_to_render.ignore_lighting = font_ignore_lighting;
    label_to_render.ignore_camera = ignore_camera;
    label_to_render.z = z;
    
    float letter_width = screenspace_width_to_width(
        font_height * 0.8f, z);
    float letter_height = screenspace_height_to_height(
        font_height * 0.8f, z);
    
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
        
        construct_quad(
            /* const float left_x: */
                0,
            /* const float top_y: */
                0,
            /* const flota z: */
                0.01f,
            /* const float width: */
                letter_width,
            /* const float height: */
                letter_height,
            /* recipient: */
                &letter);
        letter.triangles[0].texturearray_i = font_texturearray_i;
        letter.triangles[1].texturearray_i = font_texturearray_i;
        letter.triangles[0].texture_i = (int32_t)(text_to_draw[i] - '!');
        letter.triangles[1].texture_i = (int32_t)(text_to_draw[i] - '!');
        
        for (
            uint32_t rgba_i = 0;
            rgba_i < 4;
            rgba_i++)
        {
            letter.triangles[0].color[rgba_i] = font_color[rgba_i];
            letter.triangles[1].color[rgba_i] = font_color[rgba_i];
        }
        
        float letter_x_offset =
            screenspace_width_to_width(
                cur_x_offset + get_left_side_bearing(text_to_draw[i]), z);
        float letter_y_offset =
            screenspace_height_to_height(
                cur_y_offset - get_y_offset(text_to_draw[i]), z);
        
        for (uint32_t m = 0; m < 3; m++) {
            letter.triangles[0].vertices[m].x += letter_x_offset;
            letter.triangles[0].vertices[m].y += letter_y_offset;
            letter.triangles[1].vertices[m].x += letter_x_offset;
            letter.triangles[1].vertices[m].y += letter_y_offset;
        }
        
        log_assert(label_to_render.triangles_size + 1 < POLYGON_TRIANGLES_SIZE);
        if (!application_running) { return; }
        
        label_to_render.triangles[label_to_render.triangles_size] =
            letter.triangles[0];
        label_to_render.triangles[label_to_render.triangles_size + 1] =
            letter.triangles[1];
        label_to_render.triangles_size += 2;
        
        cur_x_offset += get_advance_width(text_to_draw[i]);
        if (left_pixelspace + cur_x_offset + get_advance_width('w') >= max_width)
        {
            cur_x_offset   = 0;
            cur_y_offset  -= font_height;
        }
        
        i++;
    }
    
    request_zpolygon_to_render(&label_to_render);
}

void request_fps_counter(uint64_t microseconds_elapsed) {
    #ifdef __ARM_NEON
    char fps_string[12] = "NEONfps: xx";
    #elif defined(__AVX__)
    char fps_string[12] = "AVX fps: xx";
    #else
    char fps_string[12] = "std fps: xx";
    #endif
    
    int32_t label_object_id = 0;
    uint64_t fps = 1000000 / microseconds_elapsed;
    
    if (fps < 100) {
        fps_string[ 9] = '0' + ((fps / 10) % 10);
        fps_string[10] = '0' + (fps % 10);
    } else {
        fps_string[ 9] = '9' + ((fps / 10) % 10);
        fps_string[10] = '9' + (fps % 10);
    }
    delete_zpolygon_object(label_object_id);
    
    font_height = 16.0f;
    font_ignore_lighting = true;
    request_label_renderable(
        /* with_id               : */ label_object_id,
        /* char * text_to_draw   : */ fps_string,
        /* float left_pixelspace : */ 20.0f,
        /* float top_pixelspace  : */ 60.0f,
        /* z                     : */ 1.0f,
        /* float max_width       : */ window_globals->window_width,
        /* bool32_t ignore_camera: */ true);
}
