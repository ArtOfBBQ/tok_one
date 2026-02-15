#include "T1_term.h"

#if T1_TERM_ACTIVE == T1_ACTIVE

#define T1_TERM_HIST_CAP 500000
#define T1_TERM_SINGLE_LINE_MAX 1024
typedef struct {
    void (* to_fullscreen_fncptr)(void);
    uint32_t history_size;
    float font_color[4];
    float font_rgb_cap[3];
    float background_color[4];
    int32_t back_object_id;
    int32_t labels_object_id; // INT32_MAX - 1;
    char cur_command[T1_TERM_SINGLE_LINE_MAX];
    char history[T1_TERM_HIST_CAP];
} T1TermState;

bool8_t T1_term_active = false;
static T1TermState * T1_trms = NULL;

#define T1_TERM_WHITESPACE    7.0f

#define T1_TERM_FONT_SIZE        18.0f
#define T1_TERM_Z                0.111f
#define T1_TERM_LABELS_Z         0.109f

#define T1_TERM_INPUT_BOX_HEIGHT (T1_TERM_FONT_SIZE + (T1_TERM_WHITESPACE * 4))
#define T1_TERM_INPUT_BOX_MID_Y ((T1_TERM_INPUT_BOX_HEIGHT * 0.5f) + T1_TERM_WHITESPACE)


static bool32_t requesting_label_update = false;

static void T1_term_describe_zpolygon(
    char * append_to,
    uint32_t cap,
    uint32_t zp_i)
{
    #if T1_STD_ASSERTS_ACTIVE == T1_ACTIVE
    (void)cap;
    #elif T1_STD_ASSERTS_ACTIVE == T1_INACTIVE
    #else
    #error
    #endif
    
    T1_std_strcat_cap(append_to, cap, "\n***Zpolygon: ");
    T1_std_strcat_uint_cap(append_to, cap, zp_i);
}

void T1_term_destroy_all(void) {
    if (T1_trms->back_object_id >= 0) {
        T1_texquad_delete(
            T1_trms->back_object_id);
    }
    
    if (T1_trms->labels_object_id >= 0) {
        T1_texquad_delete(
            T1_trms->labels_object_id);
    }
}

static void T1_term_update_history_size(void) {
    T1_trms->history_size = 0;
    while (
        T1_trms->history[T1_trms->history_size] != '\0')
    {
        T1_trms->history_size++;
    }
}

void T1_term_init(
    void (* enter_fullscreen_fncptr)(void))
{
    T1_trms = T1_mem_malloc_unmanaged(
        sizeof(T1TermState));
    T1_std_memset(T1_trms, 0, sizeof(T1TermState));
    
    T1_trms->to_fullscreen_fncptr = enter_fullscreen_fncptr;
    
    T1_std_strcpy_cap(
        T1_trms->history,
        T1_TERM_HIST_CAP,
        "T1 debugging terminal\n");
    
    T1_term_update_history_size();
    
    T1_trms->font_color[0] = 10.0f;
    T1_trms->font_color[1] = 10.0f;
    T1_trms->font_color[2] = 10.0f;
    T1_trms->font_color[3] = 1.0f;
    
    T1_trms->font_rgb_cap[0] = 1.0f;
    T1_trms->font_rgb_cap[1] = 1.0f;
    T1_trms->font_rgb_cap[2] = 1.0f;
    
    T1_trms->background_color[0] = 0.0f;
    T1_trms->background_color[1] = 0.0f;
    T1_trms->background_color[2] = 1.0f;
    T1_trms->background_color[3] = 0.5f;
}

void T1_term_redraw_backgrounds(void) {
    T1_trms->back_object_id = INT32_MAX;
    
    float command_history_height =
        T1_render_views->cpu[0].height -
        T1_TERM_INPUT_BOX_HEIGHT -
        (T1_TERM_WHITESPACE * 3);
    
    float width =
        (float)T1_render_views->cpu[0].
            width - (T1_TERM_WHITESPACE
                * 2.0f);
    
    T1FlatTexQuadRequest input_req;
    T1_texquad_fetch_next(&input_req);
    input_req.cpu->zsprite_id =
        T1_trms->back_object_id;
    input_req.gpu->f32.xyz[0] = 0.0f;
    input_req.gpu->f32.xyz[1] =
        T1_render_view_screen_y_to_y_noz(
            T1_TERM_INPUT_BOX_MID_Y);
    input_req.gpu->f32.xyz[2] = T1_TERM_Z;
    
    input_req.gpu->f32.size_xy[0] =
        T1_render_view_screen_width_to_width_noz(width);
    input_req.gpu->f32.size_xy[1] =
        T1_render_view_screen_height_to_height_noz(
            T1_TERM_INPUT_BOX_HEIGHT);
    input_req.gpu->i32.tex_array_i = -1;
    input_req.gpu->i32.tex_slice_i = -1;
    input_req.gpu->f32.rgba[0] =
        T1_trms->background_color[0];
    input_req.gpu->f32.rgba[1] =
        T1_trms->background_color[1];
    input_req.gpu->f32.rgba[2] =
        T1_trms->background_color[2];
    input_req.gpu->f32.rgba[3] =
        T1_trms->background_color[3];
    T1_texquad_commit(&input_req);
    
    // The console history area
    T1FlatTexQuadRequest history_req;
    T1_texquad_fetch_next(&history_req);
    history_req.gpu->f32.xyz[0] = 0.0f;
    history_req.gpu->f32.xyz[1] =
        T1_render_view_screen_y_to_y_noz(
           (command_history_height / 2) +
               T1_TERM_INPUT_BOX_HEIGHT +
               (T1_TERM_WHITESPACE * 2.0f));
    history_req.gpu->f32.xyz[2] = T1_TERM_Z;
    history_req.gpu->f32.size_xy[0] =
        T1_render_view_screen_width_to_width_noz(
                T1_render_views->cpu[0].width -
                    (T1_TERM_WHITESPACE * 2));
    history_req.gpu->f32.size_xy[1] =
        T1_render_view_screen_height_to_height_noz(
            command_history_height);
    history_req.cpu->zsprite_id = INT32_MAX;
    history_req.gpu->i32.touch_id = -1;
    history_req.gpu->f32.rgba[0] =
        T1_trms->background_color[0];
    history_req.gpu->f32.rgba[1] =
        T1_trms->background_color[1];
    history_req.gpu->f32.rgba[2] =
        T1_trms->background_color[2];
    history_req.gpu->f32.rgba[3] =
        T1_trms->background_color[3];
    T1_texquad_commit(&history_req);
}

void T1_term_render(void) {
    if (!T1_term_active) {
        return;
    }
    
    T1_texquad_delete(T1_trms->back_object_id);
    
    T1_term_redraw_backgrounds();
    
    T1_texquad_delete(T1_trms->labels_object_id);
    
    float previous_font_height =
        T1_text_props->font_height;
    T1_text_props->font_height =
        T1_TERM_FONT_SIZE;
    
    // draw the terminal's history as a label
    float history_label_top =
        T1_render_views->cpu[0].height -
            (T1_TERM_WHITESPACE * 2);
    float history_label_height =
        T1_render_views->cpu[0].height -
            T1_TERM_FONT_SIZE -
            (T1_TERM_WHITESPACE * 4.5f);
    
    uint32_t max_lines_in_history =
        (uint32_t)(history_label_height / (T1_TERM_FONT_SIZE * 1.0f));
    
    uint32_t char_offset = T1_trms->history_size;
    uint32_t lines_taken = 0;
    uint32_t chars_in_current_line = 0;
    
    while (
        lines_taken <= max_lines_in_history &&
        char_offset > 0)
    {
        char_offset--;
        
        if (T1_trms->history[char_offset] == '\n') {
            chars_in_current_line = 0;
            lines_taken += 1;
        } else if (chars_in_current_line >= T1_TERM_SINGLE_LINE_MAX) {
            chars_in_current_line = 0;
            lines_taken += 1;
        } else {
            chars_in_current_line += 1;
        }
    }
    if (T1_trms->history[char_offset] == '\n') {
        char_offset += 1;
    }
    
    T1_log_append("terminal history: ");
    T1_log_append(T1_trms->history + char_offset);
    T1_log_append_char('\n');
    
    T1_text_props->f32.rgba[0] =
        T1_trms->font_color[0];
    T1_text_props->f32.rgba[1] =
        T1_trms->font_color[1];
    T1_text_props->f32.rgba[2] =
        T1_trms->font_color[2];
    T1_text_props->f32.rgba[3] =
        T1_trms->font_color[3];
    T1_text_props->i32.touch_id = -1;
    
    T1_text_request_label_renderable(
        /* const int32_t with_object_id: */
            T1_trms->labels_object_id,
        /* const char * text_to_draw: */
            T1_trms->history + char_offset,
        /* const float left_pixelspace: */
            T1_TERM_WHITESPACE * 2 +
                (T1_TERM_FONT_SIZE * 0.5f),
        /* const float mid_y_pixelspace: */
            history_label_top -
                (T1_TERM_FONT_SIZE * 0.5f),
        /* const float z: */
            T1_TERM_LABELS_Z,
        /* const float max_width: */
            T1_render_views->cpu[0].width -
                (T1_TERM_WHITESPACE * 2));
    
    if (T1_trms->cur_command[0] == '\0') {
        T1_text_props->font_height =
            previous_font_height;
        requesting_label_update = false;
        return;
    }
    
    T1_text_props->i32.touch_id = -1;
    // the terminal's current input as a label
    T1_text_request_label_renderable(
        /* with_object_id: */
            T1_trms->labels_object_id,
        /* const char * text_to_draw: */
            T1_trms->cur_command,
        /* const float left_pixelspace: */
            T1_TERM_WHITESPACE * 2 +
                (T1_TERM_FONT_SIZE * 0.5f),
        /* const float mid_y_pixelspace: */
            T1_TERM_INPUT_BOX_MID_Y,
        /* const float z: */
            T1_TERM_LABELS_Z,
        /* const float max_width: */
            T1_render_views->cpu[0].width -
                (T1_TERM_WHITESPACE * 2));
    
    T1_text_props->font_height =
        previous_font_height;
    
    requesting_label_update = false;
}

void T1_term_sendchar(uint32_t to_send) {
    
    if (to_send == T1_IO_KEY_ESCAPE) {
        // ESC key
        T1_trms->cur_command[0] = '\0';
        T1_term_active = false;
        T1_term_destroy_all();
        requesting_label_update = true;
        return;
    }
    
    uint32_t last_i = 0;
    while (
        T1_trms->cur_command[last_i] != '\0')
    {
        last_i++;
    }
    
    if (to_send == T1_IO_KEY_SPACEBAR)
    {
        T1_trms->cur_command[last_i] = ' ';
        last_i++;
        T1_trms->cur_command[last_i] = '\0';
        return;
    }
    
    if (
        to_send == T1_IO_KEY_BACKSPACE &&
        last_i > 0)
    {
        T1_trms->cur_command[last_i - 1] = '\0';
        requesting_label_update = true;
    }
    
    if (
        to_send >= '!' &&
        to_send <= '}' &&
        last_i < T1_TERM_SINGLE_LINE_MAX)
    {
        T1_trms->cur_command[last_i] = (char)to_send;
        T1_trms->cur_command[last_i + 1] = '\0';
        requesting_label_update = true;
    }
}

static bool32_t T1_term_evaluate(
    char * command,
    char * response)
{
    if (
        T1_std_are_equal_strings(
            command, "PROFILE") ||
        T1_std_are_equal_strings(
            command, "PROFILER") ||
        T1_std_are_equal_strings(
            command, "PROFILE TREE"))
    {
        #if T1_PROFILER_ACTIVE == T1_ACTIVE
        T1_global->show_profiler =
            !T1_global->show_profiler;
        
        if (T1_global->show_profiler) {
            T1_std_strcpy_cap(
                response,
                T1_TERM_SINGLE_LINE_MAX,
                "Showing profiler results...");
        } else {
            T1_std_strcpy_cap(
                response,
                T1_TERM_SINGLE_LINE_MAX,
                "Stopped showing profiler results...");
        }
        
        #elif T1_PROFILER_ACTIVE == T1_INACTIVE
        T1_std_strcpy_cap(
            response,
            T1_TERM_SINGLE_LINE_MAX,
            "PROFILER_ACTIVE was undefined at compile time, no profiler data "
            "is available.");
        #else
        #error
        #endif
        return true;
    }
    
    if (
        T1_std_string_starts_with(
            command,
            "UNBLOCK RENDER VIEW UPDATES"))
    {
        T1_std_strcpy_cap(
            response,
            T1_TERM_SINGLE_LINE_MAX,
            "Enabling render view position updates...");
        T1_global->block_render_view_pos_updates = false;
        
        return true;
    }
    
    if (
        T1_std_string_starts_with(
            command,
            "BLOCK RENDER VIEW UPDATES"))
    {
        T1_std_strcpy_cap(
            response,
            T1_TERM_SINGLE_LINE_MAX,
            "Blocking render view position updates...");
        T1_global->block_render_view_pos_updates = true;
        
        return true;
    }
    
    if (
        T1_std_string_starts_with(
            command, "TO RENDER VIEW "))
    {
        uint32_t rv_good = 0;
        int32_t jump_rv = T1_std_string_to_int32_validate(
            command + 15,
            &rv_good);
        
        if (!rv_good ||
            jump_rv < 1 ||
            jump_rv >= (int32_t)T1_render_views->size) {
            T1_std_strcpy_cap(
                response,
                T1_TERM_SINGLE_LINE_MAX,
                "Couldn't parse to render view index: ");
            T1_std_strcat_cap(
                response,
                T1_TERM_SINGLE_LINE_MAX,
                command + 15);
            return true;
        }
        
        T1_std_strcpy_cap(
            response,
            T1_TERM_SINGLE_LINE_MAX,
            "Jumping to render view: ");
        T1_std_strcat_int_cap(
            response,
            T1_TERM_SINGLE_LINE_MAX,
            jump_rv);
        T1_camera->xyz[0] = T1_render_views->
            cpu[jump_rv].xyz[0];
        T1_camera->xyz[1] = T1_render_views->
            cpu[jump_rv].xyz[1];
        T1_camera->xyz[2] = T1_render_views->
            cpu[jump_rv].xyz[2];
        
        T1_camera->xyz_angle[0] = T1_render_views->
            cpu[jump_rv].xyz_angle[0];
        T1_camera->xyz_angle[1] = T1_render_views->
            cpu[jump_rv].xyz_angle[1];
        T1_camera->xyz_angle[2] = T1_render_views->
            cpu[jump_rv].xyz_angle[2];
        
        return true;
    }
    
    if (
        T1_std_are_equal_strings(command, "PAUSE PROFILER"))
    {
        if (!T1_global->pause_profiler) {
            T1_global->pause_profiler = true;
            T1_std_strcpy_cap(
                response,
                T1_TERM_SINGLE_LINE_MAX,
                "Paused profiler...");
        } else {
            T1_std_strcpy_cap(
                response,
                T1_TERM_SINGLE_LINE_MAX,
                "Profiler was already paused...");
        }
        return true;
    }
    
    if (
        T1_std_are_equal_strings(command, "RESUME PROFILER") ||
        T1_std_are_equal_strings(command, "RUN PROFILER") ||
        T1_std_are_equal_strings(command, "UNPAUSE PROFILER"))
    {
        if (T1_global->pause_profiler) {
            T1_global->pause_profiler = false;
            T1_std_strcpy_cap(
                response,
                T1_TERM_SINGLE_LINE_MAX,
                "Resuming profiler...");
        } else {
            T1_std_strcpy_cap(
                response,
                T1_TERM_SINGLE_LINE_MAX,
                "Profiler was already running...");
        }
        return true;
    }
    
    if (
        T1_std_are_equal_strings(command, "RESET CAMERA") ||
        T1_std_are_equal_strings(command, "CENTER CAMERA"))
    {
        T1_camera->xyz[0] = 0.0f;
        T1_camera->xyz[1] = 0.0f;
        T1_camera->xyz[2] = 0.0f;
        T1_camera->xyz_angle[0] = 0.0f;
        T1_camera->xyz_angle[1] = 0.0f;
        T1_camera->xyz_angle[2] = 0.0f;
        T1_std_strcpy_cap(
            response,
            T1_TERM_SINGLE_LINE_MAX,
            "Reset camera position and angles to {0,0,0}");
        return true;
    }
    
    if (
        T1_std_are_equal_strings(command, "CAMERA POS") ||
        T1_std_are_equal_strings(command, "CAMERA POSITION") ||
        T1_std_are_equal_strings(command, "CAMERA INFO") ||
        T1_std_are_equal_strings(command, "INFO CAMERA") ||
        T1_std_are_equal_strings(command, "CAMERA"))
    {
        T1_std_strcpy_cap(
            response,
            T1_TERM_SINGLE_LINE_MAX,
            "Camera is at: [");
        T1_std_strcat_float_cap(
            response,
            T1_TERM_SINGLE_LINE_MAX,
            T1_camera->xyz[0]);
        T1_std_strcat_cap(
            response,
            T1_TERM_SINGLE_LINE_MAX,
            ", ");
        T1_std_strcat_float_cap(
            response,
            T1_TERM_SINGLE_LINE_MAX,
            T1_camera->xyz[1]);
        T1_std_strcat_cap(
            response,
            T1_TERM_SINGLE_LINE_MAX,
            ", ");
        T1_std_strcat_float_cap(
            response,
            T1_TERM_SINGLE_LINE_MAX,
            T1_camera->xyz[2]);
        T1_std_strcat_cap(
            response,
            T1_TERM_SINGLE_LINE_MAX,
            "], xyz_angles: [");
        T1_std_strcat_float_cap(
            response,
            T1_TERM_SINGLE_LINE_MAX,
            T1_camera->xyz_angle[0]);
        T1_std_strcat_cap(
            response,
            T1_TERM_SINGLE_LINE_MAX,
            ", ");
        T1_std_strcat_float_cap(
            response,
            T1_TERM_SINGLE_LINE_MAX,
            T1_camera->xyz_angle[1]);
        T1_std_strcat_cap(
            response,
            T1_TERM_SINGLE_LINE_MAX,
            ", ");
        T1_std_strcat_float_cap(
            response,
            T1_TERM_SINGLE_LINE_MAX,
            T1_camera->xyz_angle[2]);
        T1_std_strcat_cap(
            response,
            T1_TERM_SINGLE_LINE_MAX,
            "].");
        return true;
    }
    
    if (
        T1_std_are_equal_strings(command, "AXIS") ||
        T1_std_are_equal_strings(command, "AXES") ||
        T1_std_are_equal_strings(command, "DRAW AXIS") ||
        T1_std_are_equal_strings(command, "DRAW AXES"))
    {
        #if RAW_SHADER_ACTIVE
        engine_globals->draw_axes = !engine_globals->draw_axes;
        
        if (engine_globals->draw_axes) {
            T1_std_strcpy_cap(
                response,
                SINGLE_LINE_MAX,
                "Drawing axes...");
        } else {
            T1_std_strcpy_cap(
                response,
                SINGLE_LINE_MAX,
                "Stopped drawing axes...");
        }
        #else
        T1_std_strcpy_cap(
            response,
            T1_TERM_SINGLE_LINE_MAX,
            "Can't draw axes because raw shader (points & lines) is disabled");
        #endif
        return true;
    }
    
    if (
        T1_std_are_equal_strings(
            command, "MOUSE") ||
        T1_std_are_equal_strings(
            command, "DRAW MOUSE"))
    {
        T1_global->draw_mouseptr = !T1_global->draw_mouseptr;
        
        if (T1_global->draw_mouseptr) {
            T1_std_strcpy_cap(
                response,
                T1_TERM_SINGLE_LINE_MAX,
                "Drawing the mouse pointer...");
        } else {
            T1_std_strcpy_cap(
                response,
                T1_TERM_SINGLE_LINE_MAX,
                "Stopped drawing the mouse pointer...");
        }
        return true;
    }
    
    
    if (
        T1_std_are_equal_strings(command, "DUMP SOUND") ||
        T1_std_are_equal_strings(command, "DUMP SOUND BUFFER"))
    {
        #if T1_AUDIO_ACTIVE == T1_ACTIVE
        unsigned char * recipient =
            T1_mem_malloc_managed(T1_audio_state->global_buffer_size_bytes + 100);
        uint32_t recipient_size = 0;
        
        T1_wav_samples_to_wav(
            /* unsigned char * recipient: */
                recipient,
            /* uint32_t * recipient_size: */
                &recipient_size,
            /* const uint32_t recipient_cap: */
                T1_audio_state->global_buffer_size_bytes + 100,
            /* int16_t * samples: */
                T1_audio_state->samples_buffer,
            /* const uint32_t samples_size: */
                T1_audio_state->global_samples_size);
        
        T1_std_strcpy_cap(
            response,
            T1_TERM_SINGLE_LINE_MAX,
            "Dumping global sound buffer to disk...");
        
        uint32_t good = 0;
        T1_platform_write_file_to_writables(
            /* const char * filepath_inside_writables: */
                "global_sound_buffer.wav",
            /* const char * output: */
                (char *)recipient,
            /* const uint32_t output_size: */
                recipient_size,
            /* uint32_t * good: */
                &good);
        
        if (good) {
            T1_std_strcpy_cap(
                response,
                T1_TERM_SINGLE_LINE_MAX,
                "\nSuccess!");
        } else {
            T1_std_strcpy_cap(
                response,
                T1_TERM_SINGLE_LINE_MAX,
                "\nFailed to write to disk :(");
        }
        #elif T1_AUDIO_ACTIVE == T1_INACTIVE
        T1_std_strcpy_cap(
            response,
            T1_TERM_SINGLE_LINE_MAX,
            "Audio code is disabled! set AUDIO_CODE_ACTIVE to 1 in "
            "clientlogic_macro_settings.h to enable it.");
        #else
        #error
        #endif
        return true;
    }
    
    
    if (
        T1_std_are_equal_strings(command, "BLUR BUFFER"))
    {
        T1_std_strcpy_cap(
            response,
            T1_TERM_SINGLE_LINE_MAX,
            "Inspect the blur buffer... use 'STANDARD BUFFERS' to undo");
        T1_global->postproc_consts.nonblur_pct = 0.02f;
        T1_global->postproc_consts.blur_pct = 1.0f;
        return true;
    }
    
    if (
        T1_std_are_equal_strings(command, "NONBLUR BUFFER"))
    {
        T1_std_strcpy_cap(
            response,
            T1_TERM_SINGLE_LINE_MAX,
            "Inspect the non-blur buffer... use 'STANDARD BUFFERS' to undo");
        T1_global->postproc_consts.nonblur_pct = 1.0f;
        T1_global->postproc_consts.blur_pct = 0.0f;
        return true;
    }
    
    if (
        T1_std_are_equal_strings(command, "STANDARD BUFFER") ||
        T1_std_are_equal_strings(command, "STANDARD BUFFERS"))
    {
        T1_std_strcpy_cap(
            response,
            T1_TERM_SINGLE_LINE_MAX,
            "Reverted to standard blur+non-blur composite...");
        T1_global->postproc_consts.nonblur_pct = 1.0f;
        T1_global->postproc_consts.blur_pct = 1.0f;
        return true;
    }
    
    if (
        T1_std_are_equal_strings(command, "WINDOW"))
    {
        T1_std_strcpy_cap(
            response,
            T1_TERM_SINGLE_LINE_MAX,
            "window height: ");
        T1_std_strcat_uint_cap(
            response,
            T1_TERM_SINGLE_LINE_MAX,
            (uint32_t)T1_render_views->cpu[0].height);
        T1_std_strcat_cap(
            response,
            T1_TERM_SINGLE_LINE_MAX,
            ", width: ");
        T1_std_strcat_uint_cap(
            response,
            T1_TERM_SINGLE_LINE_MAX,
            (uint32_t)T1_render_views->cpu[0].width);
        return true;
    }
    
    if (
        command[0] == 'Z' &&
        command[1] == 'P' &&
        command[2] == 'O' &&
        command[3] == 'L' &&
        command[4] == 'Y' &&
        command[5] == 'G' &&
        command[6] == 'O' &&
        command[7] == 'N' &&
        command[8] == ' ' &&
        command[9] >= '0' && command[9] <= '9')
    {
        uint32_t zp_i = T1_std_string_to_uint32(command + 9);
        
        response[0] = '\0';
        T1_term_describe_zpolygon(
            response,
            T1_TERM_SINGLE_LINE_MAX,
            zp_i);
        return true;
    }
    
    if (
        T1_std_are_equal_strings(command, "BLOCK MOUSE") ||
        T1_std_are_equal_strings(command, "STOP MOUSE") ||
        T1_std_are_equal_strings(command, "TOGGLE MOUSE"))
    {
        T1_global->block_mouse = !T1_global->block_mouse;
        
        T1_std_strcpy_cap(
            response,
            T1_TERM_SINGLE_LINE_MAX,
            "Toggled mouse block");
        return true;
    }
    
    if (
        T1_std_are_equal_strings(command, "DRAW TOP TOUCHABLE") ||
        T1_std_are_equal_strings(command, "DRAW TOP") ||
        T1_std_are_equal_strings(command, "SHOW TOP"))
    {
        T1_global->draw_top_touchable_id =
            !T1_global->draw_top_touchable_id;
        
        if (T1_global->draw_top_touchable_id) {
            T1_std_strcpy_cap(
                response,
                T1_TERM_SINGLE_LINE_MAX,
                "Drawing the top touchable_id...");
        } else {
            T1_texquad_delete(
                T1_ZSPRITEID_FPS_COUNTER);
            
            T1_std_strcpy_cap(
                response,
                T1_TERM_SINGLE_LINE_MAX,
                "Stopped drawing the top touchable_id...");
        }
        return true;
    }
    
    if (
        T1_std_are_equal_strings(command, "DRAW FPS") ||
        T1_std_are_equal_strings(command, "FPS") ||
        T1_std_are_equal_strings(command, "SHOW FPS"))
    {
        T1_global->draw_fps = !T1_global->draw_fps;
        
        if (T1_global->draw_fps) {
            T1_std_strcpy_cap(
                response,
                T1_TERM_SINGLE_LINE_MAX,
                "Drawing the fps counter...");
        } else {
            T1_texquad_delete(
                T1_ZSPRITEID_FPS_COUNTER);
            T1_std_strcpy_cap(
                response,
                T1_TERM_SINGLE_LINE_MAX,
                "Stopped drawing the fps counter...");
        }
        return true;
    }
    
    if (
        T1_std_are_equal_strings(command, "DRAW NORMALS") ||
        T1_std_are_equal_strings(command, "DRAW IMPUTED NORMALS") ||
        T1_std_are_equal_strings(command, "IMPUTED NORMALS") ||
        T1_std_are_equal_strings(command, "GUESS NORMALS") ||
        T1_std_are_equal_strings(command, "DEDUCE NORMALS"))
    {
        T1_global->draw_imputed_normals =
            !T1_global->draw_imputed_normals;
        
        if (T1_global->draw_imputed_normals) {
            T1_std_strcpy_cap(
                response,
                T1_TERM_SINGLE_LINE_MAX,
                "Drawing the 'imputed normals' "
                "for for each triangle...");
        } else {
            T1_std_strcpy_cap(
                response,
                T1_TERM_SINGLE_LINE_MAX,
                "Stopped drawing the 'imputed normals' for the last touch...");
        }
        return true;
    }
    
    if (
        T1_std_string_starts_with(
            command, "DUMP TEXTUREARRAY "))
    {
        if (command[18] < '0' || command[18] > '9') {
            T1_std_strcpy_cap(
                response,
                T1_TERM_SINGLE_LINE_MAX,
                "Can't dump texture array, TEXTUREARRAYS_SIZE was: ");
            T1_std_strcat_uint_cap(
                response,
                T1_TERM_SINGLE_LINE_MAX,
                T1_TEXTUREARRAYS_CAP);
            T1_std_strcat_cap(
                response,
                T1_TERM_SINGLE_LINE_MAX,
                ", but you didn't pass an index.");
        } else {
            int32_t texture_array_i = T1_std_string_to_int32(command + 18);
            
            if (texture_array_i >= T1_TEXTUREARRAYS_CAP) {
                T1_std_strcpy_cap(
                    response,
                    T1_TERM_SINGLE_LINE_MAX,
                    "Can't dump texture array, TEXTUREARRAYS_SIZE was: ");
                T1_std_strcat_uint_cap(
                    response,
                    T1_TERM_SINGLE_LINE_MAX,
                    T1_TEXTUREARRAYS_CAP);
                T1_std_strcat_cap(
                    response,
                    T1_TERM_SINGLE_LINE_MAX,
                    ", so please pass a number below that.");
                return true;
            }
            
            T1_std_strcpy_cap(
                response,
                T1_TERM_SINGLE_LINE_MAX,
                "Attempting to dump texture array: ");
            T1_std_strcat_int_cap(
                response,
                T1_TERM_SINGLE_LINE_MAX,
                texture_array_i);
            T1_std_strcat_cap(
                response,
                T1_TERM_SINGLE_LINE_MAX,
                " to disk...\n");
            
            uint32_t success = 0;
            T1_tex_array_debug_dump_to_writables(
                /* const int32_t texture_array_i: */
                    texture_array_i,
                /* uint32_t * success: */
                    &success);
            
            T1_std_strcat_cap(
                response,
                T1_TERM_SINGLE_LINE_MAX,
                success ?
                    "Succesfully dumped texturearray\n" :
                    "Failed to dump texturearray\n");
        }
        
        return true;
    }
    
    if (
        T1_std_are_equal_strings(command, "WRITABLES") ||
        T1_std_are_equal_strings(command, "OPEN WRITABLES"))
    {
        char writables_path[512];
        T1_platform_get_writables_dir(writables_path, 512);
        T1_platform_open_dir_in_window_if_possible(writables_path);
        
        return true;
    }
    
    if (
        T1_std_are_equal_strings(command, "DRAW TRIANGLES") ||
        T1_std_are_equal_strings(command, "TRIANGLES"))
    {
        T1_global->draw_triangles = !T1_global->draw_triangles;
        
        if (T1_global->draw_triangles) {
            T1_std_strcpy_cap(
                response,
                T1_TERM_SINGLE_LINE_MAX,
                "Drawing triangles...");
        } else {
            T1_std_strcpy_cap(
                response,
                T1_TERM_SINGLE_LINE_MAX,
                "Stopped drawing triangles...");
        }
        return true;
    }
    
    if (T1_std_are_equal_strings(command, "FS") ||
        T1_std_are_equal_strings(command, "FULLSCREEN"))
    {
        T1_std_strcpy_cap(
            response,
            T1_TERM_SINGLE_LINE_MAX,
            "Entering full screen...");
        T1_trms->to_fullscreen_fncptr();
        return true;
    }
    
    if (T1_std_are_equal_strings(command, "quit") ||
        T1_std_are_equal_strings(command, "Quit") ||
        T1_std_are_equal_strings(command, "QUIT") ||
        T1_std_are_equal_strings(command, "exit") ||
        T1_std_are_equal_strings(command, "Exit") ||
        T1_std_are_equal_strings(command, "EXIT"))
    {
        T1_platform_close_app();
        return true;
    }
    
    #if T1_LOGGER_ASSERTS_ACTIVE
    if (T1_std_are_equal_strings(command, "CRASH")) {
        T1_std_strcpy_cap(
            response,
            T1_TERM_SINGLE_LINE_MAX,
            "Forcing the app to crash...");
        log_dump_and_crash("Terminal-induced crash");
        return true;
    }
    #endif
    
    if (
        T1_std_are_equal_strings(command, "INFO LIGHTS") ||
        T1_std_are_equal_strings(command, "LIGHTS") ||
        T1_std_are_equal_strings(command, "INFO LIGHT") ||
        T1_std_are_equal_strings(command, "LIGHT") ||
        T1_std_are_equal_strings(command, "LIGHTING"))
    {
        T1_std_strcpy_cap(
            response,
            T1_TERM_SINGLE_LINE_MAX,
            "There are ");
        T1_std_strcat_uint_cap(
            response,
            T1_TERM_SINGLE_LINE_MAX,
            T1_zlights_size);
        T1_std_strcat_cap(
            response,
            T1_TERM_SINGLE_LINE_MAX,
            " lights in this scene.\n");
        return true;
    }
    
    if (
        T1_std_are_equal_strings(command, "FASTER") ||
        T1_std_are_equal_strings(command, "SLOWER"))
    {
        if (command[0] == 'F') {
            T1_global->timedelta_mult += 0.15f;
        } else {
            T1_global->timedelta_mult -= 0.15f;
        }
        
        if (T1_global->timedelta_mult < 0.05f) {
            T1_global->timedelta_mult = 0.01f;
        }
        
        T1_std_strcpy_cap(
            response,
            T1_TERM_SINGLE_LINE_MAX,
            "Multiplying all delta time by: ");
        T1_std_strcat_float_cap(
            response,
            T1_TERM_SINGLE_LINE_MAX,
            T1_global->timedelta_mult);
        return true;
    }
    
    return false;
}

void T1_term_commit_or_activate(void) {
    T1_term_destroy_all();
    
    if (
        T1_term_active &&
        T1_trms->cur_command[0] != '\0')
    {
        T1_std_strcat_cap(
            T1_trms->history,
            T1_TERM_HIST_CAP,
            T1_trms->cur_command);
        T1_std_strcat_cap(
            T1_trms->history,
            T1_TERM_HIST_CAP,
            "\n");
        char client_response[T1_TERM_SINGLE_LINE_MAX];
        client_response[0] = '\0';
        
        if (
            T1_term_evaluate(
                T1_trms->cur_command,
                client_response))
        {
            T1_std_strcat_cap(
                T1_trms->history,
                T1_TERM_HIST_CAP,
                client_response);
            T1_std_strcat_cap(
                T1_trms->history,
                T1_TERM_HIST_CAP,
                "\n");
        } else {
            T1_clientlogic_evaluate_terminal_command(
                T1_trms->cur_command,
                client_response,
                T1_TERM_SINGLE_LINE_MAX);
            T1_std_strcat_cap(
                T1_trms->history,
                T1_TERM_HIST_CAP,
                client_response);
            T1_std_strcat_cap(
                T1_trms->history,
                T1_TERM_HIST_CAP,
                "\n");
        }
        
        T1_trms->cur_command[0] = '\0';
        T1_term_update_history_size();
        T1_term_redraw_backgrounds();
        requesting_label_update = true;
        return;
    }
    
    T1_term_active = !T1_term_active;
    
    if (T1_term_active) {
        T1_term_redraw_backgrounds();
        requesting_label_update = true;
    }
}
#elif T1_TERM_ACTIVE == T1_INACTIVE
#else
#error
#endif // T1_TERM_ACTIVE
