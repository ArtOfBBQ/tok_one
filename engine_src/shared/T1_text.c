#include "T1_types_public.h"

#include "T1_text.h"
#include "T1_log.h"
#include "T1_tex.h"
#include "T1_id.h"
#include "T1_render_view.h"
#include "T1_os.h"
#include "T1_tex_array.h"

#pragma pack(push, 1)
typedef struct {
    f32 ascent;
    f32 descent;
    s32 line_gap;
    f32 font_size;
    f32 scale_factor;
    
    f32 max_glyph_height;
    f32 max_glyph_width;
    // lineGap is the spacing between one row's descent and the next row's
    // ascent... so you should advance the vertical position by
    // "ascent - descent + lineGap"
    s32 codepoints_in_font;
} FontMetrics;

typedef struct {
    char character;
    s32 x0;
    s32 x1;
    s32 y0;
    s32 y1;
    s32 glyph_width;
    s32 glyph_height;
    s32 advance_width;
    s32 left_side_bearing;
} FontCodepoint;
#pragma pack(pop)

FontMetrics * global_font_metrics = NULL;
FontCodepoint * codepoint_metrics = NULL;
u32 codepoint_metrics_size = 0;

typedef struct {
    f32 width;
    s32 start_i;
    s32 end_i;
} PrefetchedLine;

#define MAX_LINES 100

T1TextFontSettings * T1_text_props = NULL;

void T1_text_init(
    void * (* arg_text_malloc_func)(size_t size),
    const char * raw_fontmetrics_file_contents,
    u64 raw_fontmetrics_file_size)
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
            &T1_text_props->f32s,
            &T1_text_props->u32s);
        
        T1_text_props->font_height = 30.0f;
        T1_text_props->tab_width = 4.0f;
        
        T1_text_props->u32s.reserved_and_tex =
            0x00000000 | T1_TEX_NONE;
        T1_text_props->f32s.rgba[0] = 1.0f;
        T1_text_props->f32s.rgba[1] = 1.0f;
        T1_text_props->f32s.rgba[2] = 1.0f;
        T1_text_props->f32s.rgba[3] = 1.0f;
    }
    
    char * buffer_at = (char *)raw_fontmetrics_file_contents;
    T1_log_assert(sizeof(FontMetrics) < raw_fontmetrics_file_size);
    
    global_font_metrics = (FontMetrics *)buffer_at;
    buffer_at += sizeof(FontMetrics);
    codepoint_metrics_size = (u32)global_font_metrics->codepoints_in_font;
    
    #if T1_LOG_ASSERTS_ACTIVE == T1_ACTIVE
    #if 0
    u64 filesize_remaining =
        raw_fontmetrics_file_size - sizeof(FontMetrics);
    #endif
    #elif T1_LOG_ASSERTS_ACTIVE == T1_INACTIVE
    #else
    #error
    #endif
    
    codepoint_metrics = (FontCodepoint *)buffer_at;
}

static f32 get_newline_advance(void) {
    return T1_text_props->font_height * 1.1f;
}

static f32 get_advance_width(char input) {
    if (input == ' ') {
        return T1_text_props->font_height * 0.5f;
    }
    
    if (input == '\0' || input == '\n') { return 0.0f; }
    
    u32 i = (u32)(input - '!');
    // T1_log_assert(codepoint_metrics[i].character == input);
    
    if (i >= codepoint_metrics_size) {
        i = (u32)('m' - '!');
        T1_log_assert(i < codepoint_metrics_size);
    }
    
    return
        (codepoint_metrics[i].advance_width *
            global_font_metrics->scale_factor *
                T1_text_props->font_height) /
                    global_font_metrics->font_size;
}

static f32 get_left_side_bearing(char input) {
    if (input == ' ' || input == '\0' || input == '\n') { return 0.0f; }
    
    u32 i = (u32)(input - '!');
    // T1_log_assert(codepoint_metrics[i].character == input);
    
    if (i >= codepoint_metrics_size) {
        i = (u32)('m' - '!');
        T1_log_assert(i < codepoint_metrics_size);
    }
    
    return
        (codepoint_metrics[i].left_side_bearing *
            global_font_metrics->scale_factor *
                T1_text_props->font_height) /
                    global_font_metrics->font_size;
}

static f32 get_y_offset(const char input) {
    if (input == ' ' || input == '\0' || input == '\n') { return 0.0f; }
    
    u32 i = (u32)(input - '!');
    if (i >= codepoint_metrics_size) { return 0.0f; }
    
    T1_log_assert(codepoint_metrics[i].character == input);
    
    if (i >= codepoint_metrics_size) {
        i = (u32)('m' - '!');
        T1_log_assert(i < codepoint_metrics_size);
    }
    
    return
        ((-codepoint_metrics[i].y1 *
            global_font_metrics->scale_factor *
                T1_text_props->font_height) /
                    global_font_metrics->font_size) +
        (T1_text_props->font_height * 0.75f);
}

static f32 get_next_word_width(
    const char * text)
{
    f32 return_value = 0.0f;
    
    u32 i = 0;
    
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
    f32 max_width,
    PrefetchedLine * recipient,
    u32 * recipient_size)
{
    T1_log_assert(text_to_draw != NULL);
    T1_log_assert(text_to_draw[0] != '\0');
    
    f32 widest_line_width = 0.0f;
    *recipient_size = 1;
    
    for (u32 i = 0; i < 100; i++) {
        recipient[i].width = 0.0f;
        recipient[i].start_i = -1;
        recipient[i].end_i = -1;
    }
    
    // *********************************************************
    // prefetch line widths and which character they start/end
    // *********************************************************
    u32 cur_line_i = 0;
    s32 i = 0;
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
    
    for (u32 line_i = 0; line_i < *recipient_size; line_i++) {
        if (recipient[line_i].width > widest_line_width) {
            widest_line_width = recipient[line_i].width;
        }
    }
    T1_log_assert(widest_line_width > 0.0f);
}

/*
Commits a letter of a label, after applying the label settings that work
per letter: the highlight (if char_i is in it) and the opaque back
*/
static void T1_text_commit_letter(
    T1TexQuadRequest * letter,
    u32 char_i,
    f32 z)
{
    if (
        char_i >= T1_text_props->highlight_i &&
        char_i < T1_text_props->highlight_i + T1_text_props->highlight_size)
    {
        letter->gpu->f32s.rgba[0] += 0.2f;
        letter->gpu->f32s.rgba[1] += 0.2f;
        letter->gpu->f32s.rgba[2] += 0.2f;
        letter->gpu->f32s.wh[0] *= 1.12f;
        letter->gpu->f32s.wh[1] *= 1.12f;
    }
    
    if (T1_text_props->opaque_back_active) {
        T1TexQuadRequest back;
        T1_texquad_fetch_next(&back);
        *back.cpu = *letter->cpu;
        *back.gpu = *letter->gpu;
        back.gpu->u32s.reserved_and_tex = T1_TEX_NONE;
        back.gpu->f32s.rgba[0] = 0.15f;
        back.gpu->f32s.rgba[1] = 0.15f;
        back.gpu->f32s.rgba[2] = 0.60f;
        back.gpu->f32s.rgba[3] = 1.00f;
        
        // One step further back. Positions and sizes are world units
        // at the quad's depth, so scale them to stay on the same pixels.
        f32 back_mult = (z + 0.01f) / z;
        back.gpu->f32s.xyz[0] *= back_mult;
        back.gpu->f32s.xyz[1] *= back_mult;
        back.gpu->f32s.wh[0] *= back_mult;
        back.gpu->f32s.wh[1] *= back_mult;
        back.cpu->offset_xyz[0] *= back_mult;
        back.cpu->offset_xyz[1] *= back_mult;
        back.gpu->f32s.xyz[2] += 0.01f;
        T1_texquad_commit(&back);
    }
    
    T1_texquad_commit(letter);
}

static void T1_text_request_label_centered(
    u32 with_T1_id,
    const char * text_to_draw,
    f32 mid_x_pixelspace,
    f32 mid_y_pixelspace,
    f32 z,
    f32 max_width)
{
    T1_log_assert(max_width > 0.0f);
    T1_log_assert(T1_text_props->font_height > 0);
    T1_log_assert(T1_text_props->f32s.rgba[3] > -0.02f);
    T1_log_assert(T1_text_props->f32s.rgba[3] < 1.05f);
    T1_log_assert(T1_text_props->font_height > 0.05f);
    T1_log_assert(text_to_draw != NULL);
    T1_log_assert(text_to_draw[0] != '\0');
    
    PrefetchedLine lines[MAX_LINES];
    u32 lines_size;
    prefetch_label_lines(
        text_to_draw,
        max_width,
        lines,
        &lines_size);
    T1_log_assert(lines_size < MAX_LINES);
    
    T1TexQuadRequest letter;
    
    f32 cur_y_offset_pixelspace =
        (T1_text_props->font_height * 0.42f) +
        ((lines_size - 1) *
            T1_text_props->font_height * 0.5f);
    
    for (
        u32 line_i = 0;
        line_i < lines_size;
        line_i++)
    {
        f32 cur_x_offset_pixelspace =
            (T1_text_props->font_height * 0.5f) -
            (lines[line_i].width * 0.5f);
        
        for (
            s32 j = lines[line_i].start_i;
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
            
            letter.gpu->u32s = T1_text_props->u32s;
            letter.gpu->f32s = T1_text_props->f32s;
            
            letter.gpu->f32s.xyz[0] =
                T1_render_view_screen_x_to_x(
                    mid_x_pixelspace,
                    z);
            letter.gpu->f32s.xyz[1] =
                T1_render_view_screen_y_to_y(
                    mid_y_pixelspace,
                    z);
            letter.gpu->f32s.xyz[2] = z;
            
            letter.gpu->f32s.wh[0] =
                T1_render_view_screen_width_to_width(
                    T1_text_props->font_height,
                    z);
            letter.gpu->f32s.wh[1] =
                T1_render_view_screen_height_to_height(
                    T1_text_props->font_height,
                    z);
            
            letter.cpu->T1_id = with_T1_id;
            
            if ((text_to_draw[j] - '!') < 0) {
                cur_x_offset_pixelspace +=
                    get_advance_width(text_to_draw[j]);
                continue;
            }
            
            letter.cpu->offset_xyz[0] =
                T1_render_view_screen_width_to_width(
                    (cur_x_offset_pixelspace +
                        get_left_side_bearing(text_to_draw[j])),
                    z);
            letter.cpu->offset_xyz[1] =
                T1_render_view_screen_height_to_height(
                    (cur_y_offset_pixelspace -
                        get_y_offset(text_to_draw[j]) -
                        (T1_text_props->font_height * 0.5f)),
                    z);
            
            T1Tex tex;
            T1_tex_set_array_i(&tex, 0);
            T1_tex_set_slice_i(
                &tex, (s16)text_to_draw[j] - '!');
            letter.gpu->u32s.reserved_and_tex = 0x00000000 | tex;
            
            cur_x_offset_pixelspace +=
                get_advance_width(text_to_draw[j]);
            T1_text_commit_letter(&letter, (u32)j, z);
        }
        
        cur_y_offset_pixelspace -=
            get_newline_advance();
    }
}

void T1_text_request_label_left_top(
    u32 with_T1_id,
    const char * text_to_draw,
    f32 left_x_pixelspace,
    f32 top_y_pixelspace,
    f32 z,
    f32 tab_width,
    f32 max_width)
{
    T1_log_assert(max_width > 0.005f);
    T1_log_assert(z > 0.0f);
    T1_log_assert(z <= 1.0f);
    
    T1_log_assert(text_to_draw[0] != '\0');
    f32 cur_x_offset = 0;
    
    f32 tab_x_width  = (get_advance_width('D') * tab_width) * 1.01f;
    f32 cur_y_offset = 0;
    
    u32 i = 0;
    // ignore leading ' '
    while (text_to_draw[i] == ' ') {
        i++;
    }
    
    T1TexQuadRequest letter;
    
    f32 letter_width = 
        T1_render_view_screen_width_to_width(
            T1_text_props->font_height,
            z);
    f32 letter_height = 
        T1_render_view_screen_height_to_height(
            T1_text_props->font_height,
            z);
    
    while (text_to_draw[i] != '\0') {
        if (text_to_draw[i] == ' ') {
            cur_x_offset += T1_text_props->font_height / 2;
            i++;
            
            f32 next_word_width = get_next_word_width(
                /* const char * text: */
                    text_to_draw + i);
            
            if (
                (left_x_pixelspace +
                    cur_x_offset +
                    next_word_width -
                    left_x_pixelspace) > max_width
                && (next_word_width < max_width))
            {
                cur_x_offset = 0;
                cur_y_offset -= get_newline_advance();
            }
            
            continue;
        }
        
        if (text_to_draw[i] == '\t') {
            f32 next_tabstop = 0.0f;
            while (next_tabstop <= cur_x_offset)
            {
                next_tabstop += tab_x_width;
            }
            cur_x_offset = next_tabstop;
            i++;
            
            f32 next_word_width = get_next_word_width(
                /* const char * text: */
                    text_to_draw + i);
            
            if (
                (left_x_pixelspace +
                    cur_x_offset +
                    next_word_width -
                    left_x_pixelspace) > max_width
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
        
        T1_log_assert(letter.cpu != NULL);
        T1_log_assert(letter.gpu != NULL);
        
        letter.gpu->u32s = T1_text_props->u32s;
        letter.gpu->f32s = T1_text_props->f32s;
        
        letter.gpu->f32s.xyz[0] =
            T1_render_view_screen_x_to_x(
                left_x_pixelspace + (T1_text_props->font_height / 2),
                z);
        letter.gpu->f32s.xyz[1] =
            T1_render_view_screen_y_to_y(
                top_y_pixelspace,
                z); // (get_newline_advance() / 2)
        letter.gpu->f32s.xyz[2] = z;
        
        letter.gpu->f32s.wh[0] = letter_width;
        letter.gpu->f32s.wh[1] = letter_height;
        
        letter.cpu->T1_id = with_T1_id;
        
        T1Tex tex;
        s16 charval = text_to_draw[i] - '!';
        T1_tex_set_array_i(&tex, 0);
        T1_tex_set_slice_i(&tex, charval);
        
        if (
            T1_tex_to_slice_i(tex) < 0 ||
            T1_tex_to_slice_i(tex) > 100)
        {
            i++;
            continue;
        }
        
        letter.gpu->u32s.reserved_and_tex = tex;
        letter.gpu->u32s.touch_id = T1_text_props->u32s.touch_id;
        
        letter.cpu->offset_xyz[0] = 
            T1_render_view_screen_width_to_width(
                cur_x_offset + get_left_side_bearing(
                    text_to_draw[i]),
                z);
        f32 y_offset =
            T1_render_view_screen_height_to_height(
                cur_y_offset - get_y_offset(text_to_draw[i]),
                z);
        letter.cpu->offset_xyz[1] = y_offset;
        
        cur_x_offset += get_advance_width(text_to_draw[i]);
        
        if (cur_x_offset + get_advance_width('w') >= max_width)
        {
            cur_x_offset = 0;
            cur_y_offset -= get_newline_advance();
        }
        
        T1_text_commit_letter(&letter, i, z);
        i++;
    }
}

void T1_text_request_label(
    u32 with_T1_id,
    const char * text_to_draw,
    T1TextAlign align,
    f32 screen_x,
    f32 screen_y,
    f32 z,
    f32 max_width)
{
    T1_log_assert(text_to_draw != NULL);
    T1_log_assert(text_to_draw[0] != '\0');
    
    switch (align) {
        case T1_TEXT_ALIGN_LEFT_TOP:
            T1_text_request_label_left_top(
                with_T1_id,
                text_to_draw,
                screen_x,
                screen_y,
                z,
                T1_text_props->tab_width,
                max_width);
            break;
        case T1_TEXT_ALIGN_CENTER_TOP: {
            // the middle of all lines is half the extra lines lower
            PrefetchedLine lines[MAX_LINES];
            u32 lines_size;
            prefetch_label_lines(text_to_draw, max_width, lines, &lines_size);
            T1_log_assert(lines_size < MAX_LINES);
            
            T1_text_request_label_centered(
                with_T1_id,
                text_to_draw,
                screen_x,
                screen_y - (((lines_size - 1) * get_newline_advance()) / 2),
                z,
                max_width);
            break;
        }
        case T1_TEXT_ALIGN_CENTER:
            T1_text_request_label_centered(
                with_T1_id,
                text_to_draw,
                screen_x,
                screen_y,
                z,
                max_width);
            break;
        default:
            T1_log_assert(0);
    }
}

typedef struct {
    f32 xy[2];
    f32 wh[2];
    f32 offset_xy[2];
    f32 rgba[4];
    T1Tex tex;
    s16 charval;
} DrawLetter;

static void T1_tex_draw_letter(
    u8 * on_rgba, u32 rgba_w, u32 rgba_h,
    DrawLetter * letter)
{
    T1_tex_set_array_i(&letter->tex, 0);
    T1_tex_set_slice_i(&letter->tex, letter->charval);
    
    const u32 src_w = T1_tex_array_get_img_width(
        T1_tex_to_array_i(letter->tex));
    const u32 src_h = T1_tex_array_get_img_height(
        T1_tex_to_array_i(letter->tex));
    for (u32 x = 0; x < letter->wh[0]; x++)
    {
        for (u32 y = 0; y < letter->wh[1]; y++)
        {
            u32 tgt_x = (u32)(letter->xy[0] + letter->offset_xy[0]) + x;
            u32 tgt_y = (u32)(letter->xy[1] - letter->offset_xy[1]) + y;
            if (tgt_x >= rgba_w || tgt_y >= rgba_h) {
                continue;
            }
            
            const u8 * src_rgba =
                T1_tex_array_get_const_rgba(
                    0, letter->charval);
            T1_log_assert(src_rgba != NULL);
            
            u32 src_x = (u32)(((f32)x / (f32)letter->wh[0]) * src_w);
            u32 src_y = (u32)(((f32)y / (f32)letter->wh[1]) * src_h);
            
            u32 src_pix_i = ((src_y * src_w) + src_x) * 4;
            
            u32 pix_i = ((tgt_y * rgba_w) + tgt_x) * 4;
            
            // TODO: handle alpha
            if (src_rgba[src_pix_i + 3] > 0) {
                on_rgba[pix_i + 0] = 255;
                on_rgba[pix_i + 1] = 255;
                on_rgba[pix_i + 2] = 255;
                on_rgba[pix_i + 3] = 255;
            }
        }
    }
}

void T1_text_draw_label_around(
    u8 * on_rgba, u32 rgba_w, u32 rgba_h,
    const char * text_to_draw,
    f32 mid_x_pixelspace,
    f32 mid_y_pixelspace,
    f32 max_width)
{
    T1_log_assert(max_width > 0.0f);
    T1_log_assert(T1_text_props->font_height > 0);
    T1_log_assert(T1_text_props->f32s.rgba[3] > -0.02f);
    T1_log_assert(T1_text_props->f32s.rgba[3] < 1.05f);
    T1_log_assert(T1_text_props->font_height > 0.05f);
    T1_log_assert(text_to_draw != NULL);
    T1_log_assert(text_to_draw[0] != '\0');
    
    PrefetchedLine lines[MAX_LINES];
    u32 lines_size;
    prefetch_label_lines(
        text_to_draw,
        max_width,
        lines,
        &lines_size);
    T1_log_assert(lines_size < MAX_LINES);
    
    f32 cur_y_offset_pixelspace =
        (T1_text_props->font_height * 0.42f) +
        ((lines_size - 1) *
            T1_text_props->font_height * 0.5f);
    
    for (
        u32 line_i = 0;
        line_i < lines_size;
        line_i++)
    {
        f32 cur_x_offset_pixelspace =
            -(lines[line_i].width * 0.5f);
        
        for (
            s32 j = lines[line_i].start_i;
            j <= lines[line_i].end_i;
            j++)
        {
            if (text_to_draw[j] == '\n') {
                continue;
            }
            
            if (text_to_draw[j] == '\0') {
                break;
            }
            
            if ((text_to_draw[j] - '!') < 0) {
                cur_x_offset_pixelspace +=
                    get_advance_width(text_to_draw[j]);
                continue;
            }
            
            DrawLetter letter;
            T1_std_memset(&letter, 0, sizeof(DrawLetter));
            T1Tex tex;
            letter.charval = text_to_draw[j] - '!';
            T1_tex_set_array_i(&tex, 0);
            T1_tex_set_slice_i(&tex, letter.charval);
            letter.rgba[0] = T1_text_props->f32s.rgba[0];
            letter.rgba[1] = T1_text_props->f32s.rgba[1];
            letter.rgba[2] = T1_text_props->f32s.rgba[2];
            letter.rgba[3] = T1_text_props->f32s.rgba[3];
            
            letter.xy[0] = mid_x_pixelspace;
            letter.xy[1] = mid_y_pixelspace;
            
            letter.wh[0] = T1_text_props->font_height;
            letter.wh[1] = T1_text_props->font_height;
            
            letter.offset_xy[0] =
                (cur_x_offset_pixelspace +
                    get_left_side_bearing(text_to_draw[j]));
            letter.offset_xy[1] =
                (cur_y_offset_pixelspace -
                    get_y_offset(text_to_draw[j]) -
                        (T1_text_props->font_height * 0.5f));
            
            cur_x_offset_pixelspace +=
                get_advance_width(text_to_draw[j]);
            
            T1_tex_draw_letter(
                /* u8 * on_rgba: */
                    on_rgba,
                /* u32 rgba_w: */
                    rgba_w,
                /* u32 rgba_h: */
                    rgba_h,
                /* DrawLetter * letter: */
                    &letter);
        }
        
        cur_y_offset_pixelspace -=
            get_newline_advance();
    }
}

void T1_text_request_debug_text(const char * text)
{
    T1_texquad_delete(T1_ID_DEBUG_TEXT);
    
    T1_text_props->font_height = 16.0f;
    T1_text_props->f32s.rgba[0] = 1.0f;
    T1_text_props->f32s.rgba[1] = 1.0f;
    T1_text_props->f32s.rgba[2] = 1.0f;
    T1_text_props->f32s.rgba[3] = 1.0f;
    T1_text_props->u32s.touch_id = T1_TOUCH_ID_NONE;
    T1_text_request_label_left_top(
        /* with_id               : */
            T1_ID_DEBUG_TEXT,
        /* char * text_to_draw   : */
            text,
        /* f32 left_pixelspace : */
            20.0f,
        /* f32 mid_y_pixelspace  : */
            40.0f + T1_text_props->font_height,
        /* z                     : */
            0.05f,
        /* const f32 tab_width : */
            4.0f,
        /* f32 max_width       : */
            T1_render_views->cpu[0].width);
}

#define FPS_FRAMES_MAX 10
u64 ms_last_n_frames[FPS_FRAMES_MAX];
void T1_text_request_fps(
    u64 elapsed_us)
{
    #ifdef __ARM_NEON
    char fps_string[14] = "NEONfps:     ";
    #elif defined(__AVX__)
    char fps_string[14] = "AVX fps:     ";
    #else
    char fps_string[14] = "std fps:     ";
    #endif
    
    for (s32 i = FPS_FRAMES_MAX-1; i >= 1; i--) {
        ms_last_n_frames[i] = ms_last_n_frames[i-1];
    }
    ms_last_n_frames[0] = elapsed_us;
    
    u64 ms_last_n_frames_total = 0;
    for (u32 i = 0; i < FPS_FRAMES_MAX; i++) {
        ms_last_n_frames_total += ms_last_n_frames[i];
    }
    
    u64 fps = (1000000 * FPS_FRAMES_MAX) /
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
    
    T1_texquad_delete(T1_ID_FPS_COUNTER);
    
    T1_text_props->font_height = 16.0f;
    T1_text_props->f32s.rgba[0] = 1.0f;
    T1_text_props->f32s.rgba[1] = 1.0f;
    T1_text_props->f32s.rgba[2] = 1.0f;
    T1_text_props->f32s.rgba[3] = 1.0f;
    T1_text_props->u32s.touch_id = T1_TOUCH_ID_NONE;
    
    T1_text_request_label_left_top(
        /* with_id               : */
            T1_ID_FPS_COUNTER,
        /* char * text_to_draw   : */
            fps_string,
        /* f32 left_pixelspace : */
            20.0f,
        /* f32 mid_y_pixelspace  : */
            30.0f,
        /* z                     : */
            T1_render_views->cpu[0].project.znear + 0.0001f,
        /* const f32 tab_width : */
            4.0f,
        /* f32 max_width       : */
            T1_render_views->cpu[0].width);
}

void T1_text_request_top_touch_id(
    u32 top_touchable_id)
{
    T1_texquad_delete(T1_ID_FPS_COUNTER);
    
    char fps_string[512];
    T1_std_strcpy_cap(fps_string, 512, "Top touchable id: ");
    T1_std_strcat_u32_cap(fps_string, 512, top_touchable_id);
    
    T1_text_props->font_height = 16.0f;
    T1_text_props->f32s.rgba[0] = 1.0f;
    T1_text_props->f32s.rgba[1] = 1.0f;
    T1_text_props->f32s.rgba[2] = 1.0f;
    T1_text_props->f32s.rgba[3] = 1.0f;
    T1_text_props->u32s.touch_id = T1_TOUCH_ID_NONE;
    
    T1_text_request_label_left_top(
        /* with_id               : */
            T1_ID_FPS_COUNTER,
        /* char * text_to_draw   : */
            fps_string,
        /* f32 left_pixelspace : */
            20.0f,
        /* f32 mid_y_pixelspace : */
            30.0f,
        /* z                     : */
            T1_render_views->cpu[0].project.znear + 0.0001f,
        /* const f32 tab_width : */
            4.0f,
        /* f32 max_width       : */
            T1_render_views->cpu[0].width);
}
