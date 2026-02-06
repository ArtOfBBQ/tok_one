#include "T1_text.h"

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

T1TextFontSettings * T1_text_props = NULL;

void T1_text_init(
    void * (* arg_text_malloc_func)(size_t size),
    const char * raw_fontmetrics_file_contents,
    const uint64_t raw_fontmetrics_file_size)
{
    #if T1_LOG_ASSERTS_ACTIVE == T1_ACTIVE
    (void)raw_fontmetrics_file_size;
    #elif T1_LOG_ASSERTS_ACTIVE == T1_INACTIVE
    #else
    #error
    #endif
    
    if (!T1_text_props) {
        T1_text_props = arg_text_malloc_func(sizeof(T1TextFontSettings));
        T1_texquad_construct(
            &T1_text_props->f32,
            &T1_text_props->i32);
        
        T1_text_props->font_height = 30.0f;
        
        T1_text_props->i32.tex_array_i = 0;
        T1_text_props->f32.rgba[0] = 1.0f;
        T1_text_props->f32.rgba[1] = 1.0f;
        T1_text_props->f32.rgba[2] = 1.0f;
        T1_text_props->f32.rgba[3] = 1.0f;
    }
    
    char * buffer_at = (char *)raw_fontmetrics_file_contents;
    T1_log_assert(sizeof(FontMetrics) < raw_fontmetrics_file_size);
    
    global_font_metrics = (FontMetrics *)buffer_at;
    buffer_at += sizeof(FontMetrics);
    codepoint_metrics_size = (uint32_t)global_font_metrics->codepoints_in_font;
    
    #if T1_LOG_ASSERTS_ACTIVE == T1_ACTIVE
    uint64_t filesize_remaining =
        raw_fontmetrics_file_size - sizeof(FontMetrics);
    T1_log_assert(filesize_remaining % sizeof(FontCodepoint) == 0);
    T1_log_assert(
        filesize_remaining ==
            codepoint_metrics_size * sizeof(FontCodepoint));
    #elif T1_LOG_ASSERTS_ACTIVE == T1_INACTIVE
    #else
    #error
    #endif
    
    codepoint_metrics =
        (FontCodepoint *)buffer_at;
}

static float get_newline_advance(void) {
    return T1_text_props->font_height * 1.1f;
}

static float get_advance_width(const char input) {
    if (input == ' ') {
        return T1_text_props->font_height * 0.5f;
    }
    
    if (input == '\0' || input == '\n') { return 0.0f; }
    
    uint32_t i = (uint32_t)(input - '!');
    // T1_log_assert(codepoint_metrics[i].character == input);
    
    if (i >= codepoint_metrics_size) {
        i = (uint32_t)('m' - '!');
        T1_log_assert(i < codepoint_metrics_size);
    }
    
    return
        (codepoint_metrics[i].advance_width *
            global_font_metrics->scale_factor *
                T1_text_props->font_height) /
                    global_font_metrics->font_size;
}

static float get_left_side_bearing(const char input) {
    if (input == ' ' || input == '\0' || input == '\n') { return 0.0f; }
    
    uint32_t i = (uint32_t)(input - '!');
    // T1_log_assert(codepoint_metrics[i].character == input);
    
    if (i >= codepoint_metrics_size) {
        i = (uint32_t)('m' - '!');
        T1_log_assert(i < codepoint_metrics_size);
    }
    
    return
        (codepoint_metrics[i].left_side_bearing *
            global_font_metrics->scale_factor *
                T1_text_props->font_height) /
                    global_font_metrics->font_size;
}

static float get_y_offset(const char input) {
    if (input == ' ' || input == '\0' || input == '\n') { return 0.0f; }
    
    uint32_t i = (uint32_t)(input - '!');
    if (i >= codepoint_metrics_size) { return 0.0f; }
    
    T1_log_assert(codepoint_metrics[i].character == input);
    
    if (i >= codepoint_metrics_size) {
        i = (uint32_t)('m' - '!');
        T1_log_assert(i < codepoint_metrics_size);
    }
    
    return
        ((-codepoint_metrics[i].y1 *
            global_font_metrics->scale_factor *
                T1_text_props->font_height) /
                    global_font_metrics->font_size) +
        (T1_text_props->font_height * 0.75f);
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
    T1_log_assert(text_to_draw != NULL);
    T1_log_assert(text_to_draw[0] != '\0');
    
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
                T1_log_assert(recipient[cur_line_i].width > 0.0f);
                T1_log_assert(recipient[cur_line_i].start_i < i);
                recipient[cur_line_i].end_i = i;
                *recipient_size += 1;
                cur_line_i += 1;
                recipient[cur_line_i].start_i = i + 1;
                continue;
            }
        }
    }
    
    T1_log_assert(text_to_draw[i] == '\0');
    recipient[cur_line_i].end_i = i;
    if (recipient[cur_line_i].end_i == recipient[cur_line_i].start_i) {
        *recipient_size -= 1;
    }
    
    for (uint32_t line_i = 0; line_i < *recipient_size; line_i++) {
        if (recipient[line_i].width > widest_line_width) {
            widest_line_width = recipient[line_i].width;
        }
    }
    T1_log_assert(widest_line_width > 0.0f);
}

void T1_text_request_label_offset_around(
    const int32_t with_id,
    const char * text_to_draw,
    const float mid_x_pixelspace,
    const float mid_y_pixelspace,
    const float z,
    const float max_width)
{
    T1_log_assert(max_width > 0.0f);
    T1_log_assert(T1_text_props->font_height > 0);
    T1_log_assert(T1_text_props->f32.rgba[3] > -0.02f);
    T1_log_assert(T1_text_props->f32.rgba[3] < 1.05f);
    T1_log_assert(T1_text_props->font_height > 0.05f);
    T1_log_assert(text_to_draw != NULL);
    T1_log_assert(text_to_draw[0] != '\0');
    
    #define MAX_LINES 100
    PrefetchedLine lines[MAX_LINES];
    uint32_t lines_size;
    prefetch_label_lines(
        text_to_draw,
        max_width,
        lines,
        &lines_size);
    T1_log_assert(lines_size < MAX_LINES);
    
    T1FlatTexQuadRequest letter;
    
    float cur_y_offset_pixelspace =
        (T1_text_props->font_height * 0.42f) +
        ((lines_size - 1) *
            T1_text_props->font_height * 0.5f);
    
    for (
        uint32_t line_i = 0;
        line_i < lines_size;
        line_i++)
    {
        float cur_x_offset_pixelspace =
            (T1_text_props->font_height * 0.5f) -
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
            
            T1_texquad_fetch_next(&letter);
            
            letter.gpu->i32 = T1_text_props->i32;
            letter.gpu->f32 = T1_text_props->f32;
            
            letter.gpu->f32.xyz[0] =
                T1_render_view_screen_x_to_x_noz(
                    mid_x_pixelspace);
            letter.gpu->f32.xyz[1] =
                T1_render_view_screen_y_to_y_noz(
                    mid_y_pixelspace);
            letter.gpu->f32.xyz[2] = z;
            
            letter.gpu->f32.size_xy[0] =
                T1_render_view_screen_width_to_width_noz(
                    T1_text_props->font_height);
            letter.gpu->f32.size_xy[1] =
                T1_render_view_screen_height_to_height_noz(
                    T1_text_props->font_height);
            
            letter.cpu->zsprite_id = with_id;
            
            if ((text_to_draw[j] - '!') < 0) {
                cur_x_offset_pixelspace +=
                    get_advance_width(text_to_draw[j]);
                continue;
            }
            
            letter.cpu->offset_xyz[0] =
                T1_render_view_screen_width_to_width_noz(
                    (cur_x_offset_pixelspace +
                        get_left_side_bearing(text_to_draw[j])));
            letter.cpu->offset_xyz[1] =
                T1_render_view_screen_height_to_height_noz(
                    (cur_y_offset_pixelspace -
                        get_y_offset(text_to_draw[j]) -
                        (T1_text_props->font_height * 0.5f)));
            
            letter.gpu->i32.tex_array_i = 0;
            letter.gpu->i32.tex_slice_i =
                (int32_t)text_to_draw[j] - '!';
            
            cur_x_offset_pixelspace +=
                get_advance_width(text_to_draw[j]);
            T1_texquad_commit(&letter);
        }
        
        cur_y_offset_pixelspace -=
            get_newline_advance();
    }
}

void
T1_text_request_label_around_x_at_top_y(
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
    T1_log_assert(lines_size < MAX_LINES);
    
    T1_text_request_label_offset_around(
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

void T1_text_request_label_around(
    const int32_t with_id,
    const char * text_to_draw,
    const float mid_x_pixelspace,
    const float mid_y_pixelspace,
    const float z,
    const float max_width)
{
    T1_log_assert(text_to_draw != NULL);
    T1_log_assert(text_to_draw[0] != '\0');
    
    T1_text_request_label_offset_around(
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

void T1_text_request_label_renderable(
    const int32_t with_id,
    const char * text_to_draw,
    const float left_pixelspace,
    const float mid_y_pixelspace,
    const float z,
    const float max_width)
{
    T1_log_assert(z >= 0.0f);
    T1_log_assert(z <= 1.0f);
    
    T1_log_assert(text_to_draw[0] != '\0');
    float cur_x_offset = 0;
    float cur_y_offset = 0;
    
    uint32_t i = 0;
    // ignore leading ' '
    while (text_to_draw[i] == ' ') {
        i++;
    }
    
    T1FlatTexQuadRequest letter;
    
    float letter_width = T1_render_view_screen_width_to_width_noz(
        T1_text_props->font_height);
    float letter_height = T1_render_view_screen_height_to_height_noz(
        T1_text_props->font_height);
    
    while (text_to_draw[i] != '\0') {
        if (text_to_draw[i] == ' ') {
            cur_x_offset +=
                T1_text_props->font_height / 2;
            i++;
            
            float next_word_width = get_next_word_width(
                /* const char * text: */
                    text_to_draw + i);
            
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
        
        T1_texquad_fetch_next(&letter);
        
        letter.gpu->i32 = T1_text_props->i32;
        letter.gpu->f32 = T1_text_props->f32;
        
        letter.gpu->f32.xyz[0] =
            T1_render_view_screen_x_to_x_noz(
                left_pixelspace);
        letter.gpu->f32.xyz[1] =
            T1_render_view_screen_y_to_y_noz(
                mid_y_pixelspace);
        letter.gpu->f32.xyz[2] = z;
        
        letter.gpu->f32.size_xy[0] = letter_width;
        letter.gpu->f32.size_xy[1] = letter_height;
        
        letter.cpu->zsprite_id = with_id;
        
        letter.gpu->i32.tex_slice_i = (int32_t)(text_to_draw[i] - '!');
        
        if (
            letter.gpu->i32.tex_slice_i < 0 ||
            letter.gpu->i32.tex_slice_i > 100)
        {
            continue;
        }
        
        letter.gpu->i32.touch_id =
            T1_text_props->i32.touch_id;
        
        letter.cpu->offset_xyz[0] =
            T1_render_view_screen_width_to_width_noz(
                cur_x_offset + get_left_side_bearing(
                    text_to_draw[i]));
        float y_offset =
            T1_render_view_screen_height_to_height_noz(
                cur_y_offset - get_y_offset(
                    text_to_draw[i]));
        letter.cpu->offset_xyz[1] = y_offset;
        
        cur_x_offset += get_advance_width(
            text_to_draw[i]);
        
        if (
            cur_x_offset + get_advance_width('w') >=
                max_width)
        {
            cur_x_offset = 0;
            cur_y_offset -=
                get_newline_advance();
        }
        
        i++;
        T1_texquad_commit(&letter);
    }
}

void T1_text_request_debug_text(const char * text)
{
    T1_texquad_delete(
        T1_ZSPRITEID_DEBUG_TEXT);
    
    T1_text_props->font_height = 16.0f;
    T1_text_props->f32.rgba[0] = 1.0f;
    T1_text_props->f32.rgba[1] = 1.0f;
    T1_text_props->f32.rgba[2] = 1.0f;
    T1_text_props->f32.rgba[3] = 1.0f;
    T1_text_props->i32.touch_id = -1;
    T1_text_request_label_renderable(
        /* with_id               : */
            T1_ZSPRITEID_DEBUG_TEXT,
        /* char * text_to_draw   : */
            text,
        /* float left_pixelspace : */
            20.0f,
        /* float mid_y_pixelspace  : */
            40.0f + T1_text_props->font_height,
        /* z                     : */
            0.05f,
        /* float max_width       : */
            T1_render_views->cpu[0].width);
}

#define FPS_FRAMES_MAX 10
uint64_t ms_last_n_frames[FPS_FRAMES_MAX];
void T1_text_request_fps(
    uint64_t elapsed_us)
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
    ms_last_n_frames[0] = elapsed_us;
    
    uint64_t ms_last_n_frames_total = 0;
    for (uint32_t i = 0; i < FPS_FRAMES_MAX; i++) {
        ms_last_n_frames_total += ms_last_n_frames[i];
    }
    
    uint64_t fps = (1000000 * FPS_FRAMES_MAX) /
        (ms_last_n_frames_total);
    
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
    
    T1_texquad_delete(T1_ZSPRITEID_FPS_COUNTER);
    
    T1_text_props->font_height = 16.0f;
    T1_text_props->f32.rgba[0] = 1.0f;
    T1_text_props->f32.rgba[1] = 1.0f;
    T1_text_props->f32.rgba[2] = 1.0f;
    T1_text_props->f32.rgba[3] = 1.0f;
    T1_text_props->i32.touch_id = -1;
    
    T1_text_request_label_renderable(
        /* with_id               : */
            T1_ZSPRITEID_FPS_COUNTER,
        /* char * text_to_draw   : */
            fps_string,
        /* float left_pixelspace : */
            20.0f,
        /* float mid_y_pixelspace  : */
            30.0f,
        /* z                     : */
            T1_render_views->cpu[0].project.znear + 0.0001f,
        /* float max_width       : */
            T1_render_views->cpu[0].width);
}

void T1_text_request_top_touch_id(
    int32_t top_touchable_id)
{
    T1_texquad_delete(T1_ZSPRITEID_FPS_COUNTER);
    
    char fps_string[512];
    T1_std_strcpy_cap(fps_string, 512, "Top touchable id: ");
    T1_std_strcat_int_cap(fps_string, 512, top_touchable_id);
    
    T1_std_strcat_cap(fps_string, 512, ", camera xyz: [");
    T1_std_strcat_float_cap(fps_string, 512, T1_camera->xyz[0]);
    T1_std_strcat_float_cap(fps_string, 512, T1_camera->xyz[1]);
    T1_std_strcat_float_cap(fps_string, 512, T1_camera->xyz[2]);
    T1_std_strcat_cap(fps_string, 512, "]");
    
    T1_text_props->font_height = 16.0f;
    T1_text_props->f32.rgba[0] = 1.0f;
    T1_text_props->f32.rgba[1] = 1.0f;
    T1_text_props->f32.rgba[2] = 1.0f;
    T1_text_props->f32.rgba[3] = 1.0f;
    T1_text_props->i32.touch_id = -1;
    
    T1_text_request_label_renderable(
        /* with_id               : */
            T1_ZSPRITEID_FPS_COUNTER,
        /* char * text_to_draw   : */
            fps_string,
        /* float left_pixelspace : */
            20.0f,
        /* float mid_y_pixelspace : */
            30.0f,
        /* z                     : */
            T1_render_views->cpu[0].project.znear + 0.0001f,
        /* float max_width       : */
            T1_render_views->cpu[0].width);
}
