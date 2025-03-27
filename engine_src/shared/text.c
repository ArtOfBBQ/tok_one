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

FontMetrics * global_font_metrics = NULL;
FontCodepoint * codepoint_metrics = NULL;
uint32_t codepoint_metrics_size = 0;

typedef struct PrefetchedLine {
    float width;
    int32_t start_i;
    int32_t end_i;
} PrefetchedLine;

FontSettings * font_settings = NULL;

void text_init(
    void * (* arg_text_malloc_func)(size_t size),
    const char * raw_fontmetrics_file_contents,
    const uint64_t raw_fontmetrics_file_size)
{
    #ifdef LOGGER_IGNORE_ASSERTS
    (void)raw_fontmetrics_file_size;
    #endif
    
    if (!font_settings) {
        font_settings = arg_text_malloc_func(sizeof(FontSettings));
        font_settings->font_texturearray_i = 0;
        font_settings->font_touchable_id = -1;
        font_settings->font_height = 30.0;
        font_settings->font_color[0] = 1.0f;
        font_settings->font_color[1] = 1.0f;
        font_settings->font_color[2] = 1.0f;
        font_settings->font_color[3] = 1.0f;
        font_settings->rgb_cap[0] = 1.5f;
        font_settings->rgb_cap[1] = 1.5f;
        font_settings->rgb_cap[2] = 1.5f;
        font_settings->font_ignore_lighting = true;
        font_settings->remove_shadow = true;
    }
    
    char * buffer_at = (char *)raw_fontmetrics_file_contents;
    log_assert(sizeof(FontMetrics) < raw_fontmetrics_file_size);
    
    global_font_metrics = (FontMetrics *)buffer_at;
    buffer_at += sizeof(FontMetrics);
    codepoint_metrics_size = (uint32_t)global_font_metrics->codepoints_in_font;
    
    #ifndef LOGGER_IGNORE_ASSERTS
    uint64_t filesize_remaining =
        raw_fontmetrics_file_size - sizeof(FontMetrics);
    log_assert(filesize_remaining % sizeof(FontCodepoint) == 0);
    log_assert(
        filesize_remaining ==
            codepoint_metrics_size * sizeof(FontCodepoint));
    #endif
    
    codepoint_metrics =
        (FontCodepoint *)buffer_at;
}

static float get_newline_advance(void) {
    return font_settings->font_height * 1.1f;
}

static float get_advance_width(const char input) {
    if (input == ' ') {
        return font_settings->font_height * 0.5f;
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
                font_settings->font_height) /
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
                font_settings->font_height) /
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
                font_settings->font_height) /
                    global_font_metrics->font_size) +
        (font_settings->font_height * 0.75f);
}

static float get_next_word_width(
    const char * text)
{
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

static void prefetch_label_lines(
    const char * text_to_draw,
    const float max_width,
    PrefetchedLine * recipient,
    uint32_t * recipient_size)
{
    float widest_line_width = 0.0f;
    *recipient_size = 1;
    
    for (uint32_t i = 0; i < 100; i++) {
        recipient[i].width = 0.0f;
        recipient[i].start_i = -1;
        recipient[i].end_i = -1;
    }
    
    // *********************************************************
    // prefetch line widths and which character they start/end
    // *********************************************************
    uint32_t cur_line_i = 0;
    int32_t i = 0;
    recipient[cur_line_i].start_i = 0;
    while (text_to_draw[i] != '\0')
    {
        recipient[cur_line_i].width += get_advance_width(text_to_draw[i]);
        i++;
        
        if (
            text_to_draw[i] == '\n' ||
            (
                recipient[cur_line_i].width > 0.0f &&
                (recipient[cur_line_i].width +
                    get_advance_width(text_to_draw[i])) >= max_width))
        {
            recipient[cur_line_i].end_i = i;
            *recipient_size += 1;
            cur_line_i += 1;
            recipient[cur_line_i].start_i = i + 1;
            continue;
        }
        
        if (text_to_draw[i] == '\0') {
            break;
        }
        
        if (text_to_draw[i] == ' ') {
            if (
                recipient[cur_line_i].width > 0.0f &&
                recipient[cur_line_i].width
                    + get_next_word_width(text_to_draw + i) > max_width)
            {
                log_assert(recipient[cur_line_i].width > 0.0f);
                log_assert(recipient[cur_line_i].start_i < i);
                recipient[cur_line_i].end_i = i;
                *recipient_size += 1;
                cur_line_i += 1;
                recipient[cur_line_i].start_i = i + 1;
                continue;
            }
        }
    }
    
    log_assert(text_to_draw[i] == '\0');
    recipient[cur_line_i].end_i = i;
    if (recipient[cur_line_i].end_i == recipient[cur_line_i].start_i) {
        *recipient_size -= 1;
    }
    
    for (uint32_t line_i = 0; line_i < *recipient_size; line_i++) {
        if (recipient[line_i].width > widest_line_width) {
            widest_line_width = recipient[line_i].width;
        }
    }
    log_assert(widest_line_width > 0.0f);
}

void text_request_label_offset_around(
    const int32_t with_id,
    const char * text_to_draw,
    const float mid_x_pixelspace,
    const float mid_y_pixelspace,
    const float z,
    const float max_width)
{
    log_assert(max_width > 0.0f);
    log_assert(font_settings->font_height > 0);
    log_assert(font_settings->font_color[3] > -0.02f);
    log_assert(font_settings->font_color[3] < 1.05f);
    
    #define MAX_LINES 100
    PrefetchedLine lines[MAX_LINES];
    uint32_t lines_size;
    prefetch_label_lines(
        text_to_draw,
        max_width,
        lines,
        &lines_size);
    log_assert(lines_size < MAX_LINES);
    
    PolygonRequest letter;
    
    float cur_y_offset_pixelspace =
        (font_settings->font_height * 0.42f) +
        ((lines_size - 1) * font_settings->font_height * 0.5f);
    
    for (uint32_t line_i = 0; line_i < lines_size; line_i++) {
        float cur_x_offset_pixelspace =
            (font_settings->font_height * 0.5f) -
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
            
            request_next_zpolygon(&letter);
            construct_quad_around(
                /* const float left_x: */
                    windowsize_screenspace_x_to_x(
                        mid_x_pixelspace,
                        z),
                /* const float bottom_y */
                    windowsize_screenspace_y_to_y(
                        mid_y_pixelspace,
                        z),
                /* const float z: */
                    z,
                /* const float width: */
                    windowsize_screenspace_width_to_width(
                        font_settings->font_height,
                        z),
                /* const float height: */
                    windowsize_screenspace_height_to_height(
                        font_settings->font_height,
                        z),
                /* recipient: */
                    &letter);
            
            letter.gpu_data->ignore_lighting = font_settings->font_ignore_lighting;
            letter.gpu_data->ignore_camera = font_settings->ignore_camera;
            letter.gpu_data->scale_factor = font_settings->scale_factor;
            letter.cpu_data->sprite_id = with_id;
            letter.gpu_data->touchable_id = font_settings->font_touchable_id;
            letter.cpu_data->alpha_blending_enabled =
                font_settings->alphablending;
            
            if ((text_to_draw[j] - '!') < 0) {
                cur_x_offset_pixelspace +=
                    get_advance_width(text_to_draw[j]);
                continue;
            }
            
            letter.gpu_data->xyz_offset[0] =
                windowsize_screenspace_width_to_width(
                    (cur_x_offset_pixelspace +
                        font_settings->extra_x_offset +
                            get_left_side_bearing(text_to_draw[j])),
                    z);
            letter.gpu_data->xyz_offset[1] =
                windowsize_screenspace_height_to_height(
                    (cur_y_offset_pixelspace -
                        get_y_offset(text_to_draw[j]) -
                        (font_settings->font_height * 0.5f)) +
                            font_settings->extra_y_offset,
                    z);
            
            letter.gpu_data->remove_shadow = font_settings->remove_shadow;
            
            letter.gpu_materials[0].texturearray_i =
                font_settings->font_texturearray_i;
            letter.gpu_materials[0].texture_i      =
                text_to_draw[j] - '!';
            letter.gpu_materials[0].rgb_cap[0] =
                font_settings->rgb_cap[0];
            letter.gpu_materials[0].rgb_cap[1] =
                font_settings->rgb_cap[1];
            letter.gpu_materials[0].rgb_cap[2] =
                font_settings->rgb_cap[2];
            
            for (
                uint32_t rgba_i = 0;
                rgba_i < 4;
                rgba_i++)
            {
                letter.gpu_materials[0].rgba[rgba_i] =
                    font_settings->font_color[rgba_i];
            }
            
            cur_x_offset_pixelspace +=
                get_advance_width(text_to_draw[j]);
            commit_zpolygon_to_render(&letter);
        }
        cur_y_offset_pixelspace -= get_newline_advance();
    }
    
    font_settings->extra_x_offset = 0;
    font_settings->extra_y_offset = 0;
}

void text_request_label_around_x_at_top_y(
    const int32_t with_object_id,
    const char * text_to_draw,
    const float mid_x_pixelspace,
    const float top_y_pixelspace,
    const float z,
    const float max_width)
{
    #define MAX_LINES 100
    PrefetchedLine lines[MAX_LINES];
    uint32_t lines_size;
    prefetch_label_lines(text_to_draw, max_width, lines, &lines_size);
    log_assert(lines_size < MAX_LINES);
    
    text_request_label_offset_around(
        /* const int32_t with_id: */
            with_object_id,
        /* const char * text_to_draw: */
            text_to_draw,
        /* const float mid_x_pixelspace: */
            mid_x_pixelspace,
        /* const float mid_y_pixelspace: */
            top_y_pixelspace -
                ((((lines_size) * (get_newline_advance()))) / 2),
        /* const float z: */
            z,
        /* const float max_width: */
            max_width);
}

void text_request_label_around(
    const int32_t with_id,
    const char * text_to_draw,
    const float mid_x_pixelspace,
    const float mid_y_pixelspace,
    const float z,
    const float max_width)
{
    text_request_label_offset_around(
        /* const int32_t with_id: */
            with_id,
        /* const char * text_to_draw: */
            text_to_draw,
        /* const float mid_x_pixelspace: */
            mid_x_pixelspace,
        /* const float mid_y_pixelspace: */
            mid_y_pixelspace,
        /* const float z: */
            z,
        /* const float max_width: */
            max_width);
}

void text_request_label_renderable(
    const int32_t with_id,
    const char * text_to_draw,
    const float left_pixelspace,
    const float top_pixelspace,
    const float z,
    const float max_width)
{
    log_assert(text_to_draw[0] != '\0');
    float cur_x_offset = 0;
    float cur_y_offset = 0;
    
    uint32_t i = 0;
    // ignore leading ' '
    while (text_to_draw[i] == ' ') {
        i++;
    }
    
    PolygonRequest letter;
    
    float letter_width = windowsize_screenspace_width_to_width(
        font_settings->font_height, z);
    float letter_height = windowsize_screenspace_height_to_height(
        font_settings->font_height, z);
    
    while (text_to_draw[i] != '\0') {
        if (text_to_draw[i] == ' ') {
            cur_x_offset += font_settings->font_height / 2;
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
                cur_y_offset -= get_newline_advance();
            }
            
            continue;
        }
        
        if (text_to_draw[i] == '\n') {
            cur_x_offset = 0;
            cur_y_offset -= get_newline_advance();
            i++;
            continue;
        }
        
        request_next_zpolygon(&letter);
        construct_quad(
            /* const float left_x: */
                windowsize_screenspace_x_to_x(left_pixelspace, z),
            /* const float bottom_y: */
                windowsize_screenspace_y_to_y(
                    top_pixelspace - font_settings->font_height, z),
            /* const flota z: */
                z,
            /* const float width: */
                letter_width,
            /* const float height: */
                letter_height,
            /* recipient: */
                &letter);
        
        letter.cpu_data->sprite_id = with_id;
        letter.gpu_data->touchable_id = font_settings->font_touchable_id;
        letter.cpu_data->alpha_blending_enabled =
            font_settings->alphablending;
        letter.gpu_data->ignore_lighting = font_settings->font_ignore_lighting;
        letter.gpu_data->ignore_camera = font_settings->ignore_camera;
        letter.gpu_data->remove_shadow = font_settings->remove_shadow;
        
        letter.gpu_materials[0].texturearray_i =
            font_settings->font_texturearray_i;
        letter.gpu_materials[0].texture_i =
            (int32_t)(text_to_draw[i] - '!');
        
        if (
            letter.gpu_materials[0].texture_i < 0 ||
            letter.gpu_materials[0].texture_i > 100)
        {
            continue;
        }
        
        for (
            uint32_t rgba_i = 0;
            rgba_i < 4;
            rgba_i++)
        {
            letter.gpu_materials[0].rgba[rgba_i] =
                font_settings->font_color[rgba_i];
        }
        letter.gpu_materials[0].rgb_cap[0] =
            font_settings->rgb_cap[0];
        letter.gpu_materials[0].rgb_cap[1] =
            font_settings->rgb_cap[1];
        letter.gpu_materials[0].rgb_cap[2] =
            font_settings->rgb_cap[2];
        
        
        letter.gpu_data->xyz_offset[0] =
            windowsize_screenspace_width_to_width(
                cur_x_offset + get_left_side_bearing(
                    text_to_draw[i]), z);
        letter.gpu_data->xyz_offset[1] =
            windowsize_screenspace_height_to_height(
                cur_y_offset - get_y_offset(
                    text_to_draw[i]), z);
        
        cur_x_offset += get_advance_width(text_to_draw[i]);
        if (cur_x_offset + get_advance_width('w') >= max_width)
        {
            cur_x_offset   = 0;
            cur_y_offset  -= get_newline_advance();
        }
        
        
        i++;
        commit_zpolygon_to_render(&letter);
    }
}

#define FPS_FRAMES_MAX 10
uint64_t ms_last_n_frames[FPS_FRAMES_MAX];
void text_request_fps_counter(
    uint64_t microseconds_elapsed)
{
    #ifdef __ARM_NEON
    char fps_string[14] = "NEONfps:     ";
    #elif defined(__AVX__)
    char fps_string[14] = "AVX fps:     ";
    #else
    char fps_string[14] = "std fps:     ";
    #endif
    
    for (int32_t i = FPS_FRAMES_MAX-1; i >= 1; i--) {
        ms_last_n_frames[i] = ms_last_n_frames[i-1];
    }
    ms_last_n_frames[0] = microseconds_elapsed;
    
    uint64_t ms_last_n_frames_total = 0;
    for (uint32_t i = 0; i < FPS_FRAMES_MAX; i++) {
        ms_last_n_frames_total += ms_last_n_frames[i];
    }
    
    uint64_t fps = (1000000 * FPS_FRAMES_MAX) / (ms_last_n_frames_total);
    
    if (fps < 100) {
        fps_string[11] = '0' + ((fps / 10) % 10);
        fps_string[12] = '0' + (fps % 10);
    } else if (fps < 1000) {
        fps_string[10] = '0' + ((fps / 100) % 10);
        fps_string[11] = '0' + ((fps / 10) % 10);
        fps_string[12] = '0' + (fps % 10);
    } else if (fps < 10000) {
        fps_string[ 9] = '0' + ((fps / 1000) % 10);
        fps_string[10] = '0' + ((fps / 100) % 10);
        fps_string[11] = '0' + ((fps / 10) % 10);
        fps_string[12] = '0' + (fps % 10);
    } else {
        fps_string[ 9] = '9';
        fps_string[10] = '9';
        fps_string[11] = '9';
        fps_string[12] = '9';
    }
    
    delete_zpolygon_object(FPS_COUNTER_OBJECT_ID);
    
    font_settings->font_height = 16.0f;
    font_settings->font_color[0] = 1.0f;
    font_settings->font_color[1] = 1.0f;
    font_settings->font_color[2] = 1.0f;
    font_settings->font_color[3] = 1.0f;
    font_settings->font_ignore_lighting = true;
    font_settings->ignore_camera = true;
    font_settings->font_touchable_id = -1;
    text_request_label_renderable(
        /* with_id               : */
            FPS_COUNTER_OBJECT_ID,
        /* char * text_to_draw   : */
            fps_string,
        /* float left_pixelspace : */
            20.0f,
        /* float top_pixelspace  : */
            30.0f,
        /* z                     : */
            0.05f,
        /* float max_width       : */
            window_globals->window_width);
}

void text_request_top_touchable_id(
    int32_t top_touchable_id)
{
    delete_zpolygon_object(FPS_COUNTER_OBJECT_ID);
    
    char fps_string[512];
    common_strcpy_capped(fps_string, 512, "Top touchable id: ");
    common_strcat_int_capped(fps_string, 512, top_touchable_id);
    
    common_strcat_capped(fps_string, 512, ", camera xyz: [");
    common_strcat_float_capped(fps_string, 512, camera.xyz[0]);
    common_strcat_float_capped(fps_string, 512, camera.xyz[1]);
    common_strcat_float_capped(fps_string, 512, camera.xyz[2]);
    common_strcat_capped(fps_string, 512, "]");
    
    font_settings->font_height = 16.0f;
    font_settings->font_color[0] = 1.0f;
    font_settings->font_color[1] = 1.0f;
    font_settings->font_color[2] = 1.0f;
    font_settings->font_color[3] = 1.0f;
    font_settings->font_ignore_lighting = true;
    font_settings->ignore_camera = true;
    font_settings->font_touchable_id = -1;
    text_request_label_renderable(
        /* with_id               : */
            FPS_COUNTER_OBJECT_ID,
        /* char * text_to_draw   : */
            fps_string,
        /* float left_pixelspace : */
            20.0f,
        /* float top_pixelspace  : */
            30.0f,
        /* z                     : */
            0.05f,
        /* float max_width       : */
            window_globals->window_width);
}
