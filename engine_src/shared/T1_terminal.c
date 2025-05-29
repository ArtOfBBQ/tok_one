#include "T1_terminal.h"

#if TERMINAL_ACTIVE

bool32_t terminal_active = false;

static void (* terminal_enter_fullscreen_fnc)(void) = NULL;

#define SINGLE_LINE_MAX 1024
static char * current_command = NULL;

#define TERMINAL_HISTORY_MAX 500000
#define TERMINAL_WHITESPACE    7.0f

#define TERM_FONT_SIZE        14.0f
#define TERM_Z                0.111f
#define TERM_LABELS_Z         0.110f
static char * terminal_history = NULL;
static uint32_t terminal_history_size = 0;

static float term_font_color[4];
static float term_font_rgb_cap[3];
static float term_background_color[4];

static int32_t terminal_back_object_id = -1;
static int32_t terminal_labels_object_id = INT32_MAX - 1;

static bool32_t requesting_label_update = false;

static void describe_zpolygon(
    char * append_to,
    uint32_t cap,
    uint32_t zp_i)
{
    #ifdef COMMON_IGNORE_ASSERTS
    (void)cap;
    #endif
    
    common_strcat_capped(append_to, cap, "\n***Zpolygon: ");
    common_strcat_uint_capped(append_to, cap, zp_i);
}

void destroy_terminal_objects(void) {
    if (terminal_back_object_id >= 0) {
        zsprite_delete(terminal_back_object_id);
    }
    
    if (terminal_labels_object_id >= 0) {
        zsprite_delete(terminal_labels_object_id);
    }
}

static void update_terminal_history_size(void) {
    terminal_history_size = 0;
    while (terminal_history[terminal_history_size] != '\0') {
        terminal_history_size++;
    }
}

void terminal_init(
    void (* terminal_enter_fullscreen_fncptr)(void))
{
    terminal_enter_fullscreen_fnc = terminal_enter_fullscreen_fncptr;
    
    current_command = (char *)malloc_from_unmanaged(
        SINGLE_LINE_MAX);
    current_command[0] = '\0';
    
    terminal_history = (char *)malloc_from_unmanaged(
        TERMINAL_HISTORY_MAX);
    common_strcpy_capped(
        terminal_history,
        TERMINAL_HISTORY_MAX,
        "TOK ONE embedded debugging terminal v1.0\n");
    
    update_terminal_history_size();
    
    term_font_color[0] = 10.0f;
    term_font_color[1] = 10.0f;
    term_font_color[2] = 10.0f;
    term_font_color[3] = 1.0f;
    
    term_font_rgb_cap[0] = 1.0f;
    term_font_rgb_cap[1] = 1.0f;
    term_font_rgb_cap[2] = 1.0f;
    
    term_background_color[0] = 0.0f;
    term_background_color[1] = 0.0f;
    term_background_color[2] = 1.0f;
    term_background_color[3] = 0.5f;
}

void terminal_redraw_backgrounds(void) {
    
    terminal_back_object_id = INT32_MAX;
    
    float current_input_height = (TERM_FONT_SIZE * 2);
    float command_history_height =
        engine_globals->window_height -
        current_input_height -
        (TERMINAL_WHITESPACE * 3);
    
    zSpriteRequest current_command_input;
    zsprite_request_next(&current_command_input);
    zsprite_construct_quad_around(
        /* const float mid_x: */
            engineglobals_screenspace_x_to_x(
                engine_globals->window_width / 2,
                TERM_Z),
        /* const float mid_y: */
            engineglobals_screenspace_y_to_y(
                (TERMINAL_WHITESPACE * 1.5f) +
                (TERM_FONT_SIZE / 2),
                TERM_Z),
        /* const float z: */
            TERM_Z,
        /* const float width: */
            engineglobals_screenspace_width_to_width(
                engine_globals->window_width -
                    (TERMINAL_WHITESPACE * 2),
                TERM_Z),
        /* const float height: */
            engineglobals_screenspace_height_to_height(
                current_input_height,
                TERM_Z),
        /* zPolygon * recipien: */
            &current_command_input);
    
    current_command_input.gpu_materials[0].ambient_rgb[0] =
        term_background_color[0];
    current_command_input.gpu_materials[0].ambient_rgb[1] =
        term_background_color[1];
    current_command_input.gpu_materials[0].ambient_rgb[2] =
        term_background_color[2];
    current_command_input.gpu_materials[0].alpha =
        term_background_color[3];
    current_command_input.gpu_data->ignore_camera = true;
    current_command_input.gpu_data->ignore_lighting = true;
    current_command_input.cpu_data->alpha_blending_enabled = true;
    current_command_input.cpu_data->visible = terminal_active;
    current_command_input.cpu_data->zsprite_id = terminal_back_object_id;
    current_command_input.gpu_data->touchable_id = -1;
    zsprite_commit(&current_command_input);
    
    // The console history area
    zsprite_request_next(&current_command_input);
    zsprite_construct(&current_command_input);
    zsprite_construct_quad_around(
       /* const float mid_x: */
           engineglobals_screenspace_x_to_x(
               engine_globals->window_width / 2,
               TERM_Z),
       /* const float mid_y: */
           engineglobals_screenspace_y_to_y(
               (command_history_height / 2) +
                   current_input_height +
                   (TERMINAL_WHITESPACE * 2),
               TERM_Z),
       /* const float z: */
           TERM_Z,
       /* const float width: */
           engineglobals_screenspace_width_to_width(
                engine_globals->window_width -
                    (TERMINAL_WHITESPACE * 2),
                TERM_Z),
       /* const float height: */
           engineglobals_screenspace_height_to_height(
               command_history_height,
               TERM_Z),
       /* zPolygon * recipien: */
           &current_command_input);
    
    current_command_input.gpu_materials[0].ambient_rgb[0] =
        term_background_color[0];
    current_command_input.gpu_materials[0].ambient_rgb[1] =
        term_background_color[1];
    current_command_input.gpu_materials[0].ambient_rgb[2] =
        term_background_color[2];
    current_command_input.gpu_materials[0].alpha =
        term_background_color[3];
    current_command_input.cpu_data->visible = terminal_active;
    current_command_input.cpu_data->alpha_blending_enabled = true;
    current_command_input.gpu_data->ignore_camera = true;
    current_command_input.gpu_data->ignore_lighting = true;
    current_command_input.cpu_data->zsprite_id = INT32_MAX;
    current_command_input.gpu_data->touchable_id = -1;
    
    zsprite_commit(&current_command_input);
}

void terminal_render(void) {
    if (!terminal_active) {
        return;
    }
    
    if (terminal_back_object_id < 0) {
        terminal_redraw_backgrounds();
    }
    
    if (requesting_label_update) {
        log_append("redraw terminal label...\n");
        zsprite_delete(
            /* const int32_t with_object_id: */
                terminal_labels_object_id);
        
        float previous_font_height = font_settings->font_height;
        font_settings->font_height = TERM_FONT_SIZE;
        
        // draw the terminal's history as a label
        float history_label_top =
            engine_globals->window_height -
                (TERMINAL_WHITESPACE * 2);
        float history_label_height =
            engine_globals->window_height -
                TERM_FONT_SIZE -
                (TERMINAL_WHITESPACE * 4.5f);
        
        uint32_t max_lines_in_history =
            (uint32_t)(history_label_height / (TERM_FONT_SIZE * 1.0f));
        
        uint32_t char_offset = terminal_history_size;
        uint32_t lines_taken = 0;
        uint32_t chars_in_current_line = 0;
        
        while (
            lines_taken <= max_lines_in_history &&
            char_offset > 0)
        {
            char_offset--;
            
            if (terminal_history[char_offset] == '\n') {
                chars_in_current_line = 0;
                lines_taken += 1;
            } else if (chars_in_current_line >= SINGLE_LINE_MAX) {
                chars_in_current_line = 0;
                lines_taken += 1;
            } else {
                chars_in_current_line += 1;
            }
        }
        if (terminal_history[char_offset] == '\n') {
            char_offset += 1;
        }
        
        log_append("terminal history: ");
        log_append(terminal_history + char_offset);
        log_append_char('\n');
        
        font_settings->font_color[0] = term_font_color[0];
        font_settings->font_color[1] = term_font_color[1];
        font_settings->font_color[2] = term_font_color[2];
        font_settings->font_color[3] = term_font_color[3];
        font_settings->rgb_cap[0] = term_font_rgb_cap[0];
        font_settings->rgb_cap[1] = term_font_rgb_cap[1];
        font_settings->rgb_cap[2] = term_font_rgb_cap[2];
        font_settings->ignore_camera = true;
        font_settings->font_ignore_lighting = 1.0f;
        font_settings->font_touchable_id = -1;
        
        text_request_label_renderable(
            /* const int32_t with_object_id: */
                terminal_labels_object_id,
            /* const char * text_to_draw: */
                terminal_history + char_offset,
            /* const float left_pixelspace: */
                TERMINAL_WHITESPACE * 2.0f,
            /* const float top_pixelspace: */
                history_label_top,
            /* const float z: */
                TERM_LABELS_Z,
            /* const float max_width: */
                engine_globals->window_width -
                    (TERMINAL_WHITESPACE * 2));
        
        if (current_command[0] == '\0') {
            font_settings->font_height = previous_font_height;
            requesting_label_update = false;
            return;
        }
        
        font_settings->ignore_camera = true;
        font_settings->font_touchable_id = -1;
        // the terminal's current input as a label
        text_request_label_renderable(
            /* with_object_id: */
                terminal_labels_object_id,
            /* const char * text_to_draw: */
                current_command,
            /* const float left_pixelspace: */
                TERMINAL_WHITESPACE * 2,
            /* const float top_pixelspace: */
                (TERMINAL_WHITESPACE * 2) + (TERM_FONT_SIZE * 0.7f),
            /* const float z: */
                TERM_LABELS_Z,
            /* const float max_width: */
                engine_globals->window_width - (TERMINAL_WHITESPACE * 2));
        
        font_settings->font_height = previous_font_height;
        
        requesting_label_update = false;
    }
}

void terminal_sendchar(uint32_t to_send) {
    
    if (to_send == TOK_KEY_ESCAPE) {
        // ESC key
        current_command[0] = '\0';
        terminal_active = false;
        destroy_terminal_objects();
        requesting_label_update = true;
        return;
    }
    
    uint32_t last_i = 0;
    while (
        current_command[last_i] != '\0')
    {
        last_i++;
    }
    
    if (
        to_send == TOK_KEY_SPACEBAR)
    {
        current_command[last_i] = ' ';
        last_i++;
        current_command[last_i] = '\0';
        return;
    }
    
    if (
        to_send == TOK_KEY_BACKSPACE &&
        last_i > 0)
    {
        current_command[last_i - 1] = '\0';
        requesting_label_update = true;
    }
    
    if (
        to_send >= '!' &&
        to_send <= '}' &&
        last_i < SINGLE_LINE_MAX)
    {
        current_command[last_i] = (char)to_send;
        current_command[last_i + 1] = '\0';
        requesting_label_update = true;
    }
}

static bool32_t evaluate_terminal_command(
    char * command,
    char * response)
{
    if (
        common_are_equal_strings(command, "PROFILE") ||
        common_are_equal_strings(command, "PROFILER") ||
        common_are_equal_strings(command, "PROFILE TREE"))
    {
        #if PROFILER_ACTIVE
        engine_globals->show_profiler = !engine_globals->show_profiler;
        
        if (engine_globals->show_profiler) {
            common_strcpy_capped(
                response,
                SINGLE_LINE_MAX,
                "Showing profiler results...");
        } else {
            common_strcpy_capped(
                response,
                SINGLE_LINE_MAX,
                "Stopped showing profiler results...");
        }
        #else
        common_strcpy_capped(
            response,
            SINGLE_LINE_MAX,
            "PROFILER_ACTIVE was undefined at compile time, no profiler data "
            "is available.");
        #endif
        return true;
    }
    
    if (
        common_are_equal_strings(command, "PAUSE PROFILER"))
    {
        if (!engine_globals->pause_profiler) {
            engine_globals->pause_profiler = true;
            common_strcpy_capped(
                response,
                SINGLE_LINE_MAX,
                "Paused profiler...");
        } else {
            common_strcpy_capped(
                response,
                SINGLE_LINE_MAX,
                "Profiler was already paused...");
        }
        return true;
    }
    
    if (
        common_are_equal_strings(command, "RESUME PROFILER") ||
        common_are_equal_strings(command, "RUN PROFILER") ||
        common_are_equal_strings(command, "UNPAUSE PROFILER"))
    {
        if (engine_globals->pause_profiler) {
            engine_globals->pause_profiler = false;
            common_strcpy_capped(
                response,
                SINGLE_LINE_MAX,
                "Resuming profiler...");
        } else {
            common_strcpy_capped(
                response,
                SINGLE_LINE_MAX,
                "Profiler was already running...");
        }
        return true;
    }
    
    if (
        common_are_equal_strings(command, "RESET CAMERA") ||
        common_are_equal_strings(command, "CENTER CAMERA"))
    {
        camera.xyz[0] = 0.0f;
        camera.xyz[1] = 0.0f;
        camera.xyz[2] = 0.0f;
        camera.xyz_angle[0] = 0.0f;
        camera.xyz_angle[1] = 0.0f;
        camera.xyz_angle[2] = 0.0f;
        common_strcpy_capped(
            response,
            SINGLE_LINE_MAX,
            "Reset camera position and angles to {0,0,0}");
        return true;
    }
    
    if (
        common_are_equal_strings(command, "CAMERA POS") ||
        common_are_equal_strings(command, "CAMERA POSITION") ||
        common_are_equal_strings(command, "CAMERA INFO") ||
        common_are_equal_strings(command, "INFO CAMERA"))
    {
        common_strcpy_capped(
            response,
            SINGLE_LINE_MAX,
            "Camera is at: [");
        common_strcat_float_capped(
            response,
            SINGLE_LINE_MAX,
            camera.xyz[0]);
        common_strcat_capped(
            response,
            SINGLE_LINE_MAX,
            ", ");
        common_strcat_float_capped(
            response,
            SINGLE_LINE_MAX,
            camera.xyz[1]);
        common_strcat_capped(
            response,
            SINGLE_LINE_MAX,
            ", ");
        common_strcat_float_capped(
            response,
            SINGLE_LINE_MAX,
            camera.xyz[2]);
        common_strcat_capped(
            response,
            SINGLE_LINE_MAX,
            "], xyz_angles: [");
        common_strcat_float_capped(
            response,
            SINGLE_LINE_MAX,
            camera.xyz_angle[0]);
        common_strcat_capped(
            response,
            SINGLE_LINE_MAX,
            ", ");
        common_strcat_float_capped(
            response,
            SINGLE_LINE_MAX,
            camera.xyz_angle[1]);
        common_strcat_capped(
            response,
            SINGLE_LINE_MAX,
            ", ");
        common_strcat_float_capped(
            response,
            SINGLE_LINE_MAX,
            camera.xyz_angle[2]);
        common_strcat_capped(
            response,
            SINGLE_LINE_MAX,
            "].");
        return true;
    }
    
    if (
        common_are_equal_strings(command, "AXIS") ||
        common_are_equal_strings(command, "AXES") ||
        common_are_equal_strings(command, "DRAW AXIS") ||
        common_are_equal_strings(command, "DRAW AXES"))
    {
        #if RAW_SHADER_ACTIVE
        engine_globals->draw_axes = !engine_globals->draw_axes;
        
        if (engine_globals->draw_axes) {
            common_strcpy_capped(
                response,
                SINGLE_LINE_MAX,
                "Drawing axes...");
        } else {
            common_strcpy_capped(
                response,
                SINGLE_LINE_MAX,
                "Stopped drawing axes...");
        }
        #else
        common_strcpy_capped(
            response,
            SINGLE_LINE_MAX,
            "Can't draw axes because raw shader (points & lines) is disabled");
        #endif
        return true;
    }
    
    if (
        common_are_equal_strings(command, "MOUSE") ||
        common_are_equal_strings(command, "DRAW MOUSE"))
    {
        engine_globals->draw_mouseptr = !engine_globals->draw_mouseptr;
        
        if (engine_globals->draw_mouseptr) {
            common_strcpy_capped(
                response,
                SINGLE_LINE_MAX,
                "Drawing the mouse pointer...");
        } else {
            common_strcpy_capped(
                response,
                SINGLE_LINE_MAX,
                "Stopped drawing the mouse pointer...");
        }
        return true;
    }
    
    
    if (
        common_are_equal_strings(command, "DUMP SOUND") ||
        common_are_equal_strings(command, "DUMP SOUND BUFFER"))
    {
        #if AUDIO_ACTIVE
        unsigned char * recipient =
            malloc_from_managed(sound_settings->global_buffer_size_bytes + 100);
        uint32_t recipient_size = 0;
        
        wav_samples_to_wav(
            /* unsigned char * recipient: */
                recipient,
            /* uint32_t * recipient_size: */
                &recipient_size,
            /* const uint32_t recipient_cap: */
                sound_settings->global_buffer_size_bytes + 100,
            /* int16_t * samples: */
                sound_settings->samples_buffer,
            /* const uint32_t samples_size: */
                sound_settings->global_samples_size);
        
        common_strcpy_capped(
            response,
            SINGLE_LINE_MAX,
            "Dumping global sound buffer to disk...");
        
        uint32_t good = 0;
        platform_write_file_to_writables(
            /* const char * filepath_inside_writables: */
                "global_sound_buffer.wav",
            /* const char * output: */
                (char *)recipient,
            /* const uint32_t output_size: */
                recipient_size,
            /* uint32_t * good: */
                &good);
        
        if (good) {
            common_strcpy_capped(
                response,
                SINGLE_LINE_MAX,
                "\nSuccess!");
        } else {
            common_strcpy_capped(
                response,
                SINGLE_LINE_MAX,
                "\nFailed to write to disk :(");
        }
        #else
        common_strcpy_capped(
            response,
            SINGLE_LINE_MAX,
            "Audio code is disabled! set AUDIO_CODE_ACTIVE to 1 in "
            "clientlogic_macro_settings.h to enable it.");
        #endif
        return true;
    }
    
    
    if (
        common_are_equal_strings(command, "BLUR BUFFER"))
    {
        common_strcpy_capped(
            response,
            SINGLE_LINE_MAX,
            "Inspect the blur buffer... use 'STANDARD BUFFERS' to undo");
        engine_globals->postprocessing_constants.nonblur_pct = 0.02f;
        engine_globals->postprocessing_constants.blur_pct = 1.0f;
        return true;
    }
    
    if (
        common_are_equal_strings(command, "NONBLUR BUFFER"))
    {
        common_strcpy_capped(
            response,
            SINGLE_LINE_MAX,
            "Inspect the non-blur buffer... use 'STANDARD BUFFERS' to undo");
        engine_globals->postprocessing_constants.nonblur_pct = 1.0f;
        engine_globals->postprocessing_constants.blur_pct = 0.0f;
        return true;
    }
    
    if (
        common_are_equal_strings(command, "STANDARD BUFFER") ||
        common_are_equal_strings(command, "STANDARD BUFFERS"))
    {
        common_strcpy_capped(
            response,
            SINGLE_LINE_MAX,
            "Reverted to standard blur+non-blur composite...");
        engine_globals->postprocessing_constants.nonblur_pct = 1.0f;
        engine_globals->postprocessing_constants.blur_pct = 1.0f;
        return true;
    }
    
    if (
        common_are_equal_strings(command, "WINDOW"))
    {
        common_strcpy_capped(
            response,
            SINGLE_LINE_MAX,
            "window height: ");
        common_strcat_uint_capped(
            response,
            SINGLE_LINE_MAX,
            (uint32_t)engine_globals->window_height);
        common_strcat_capped(
            response,
            SINGLE_LINE_MAX,
            ", width: ");
        common_strcat_uint_capped(
            response,
            SINGLE_LINE_MAX,
            (uint32_t)engine_globals->window_width);
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
        uint32_t zp_i = common_string_to_uint32(command + 9);
        
        response[0] = '\0';
        describe_zpolygon(response, SINGLE_LINE_MAX, zp_i);
        return true;
    }
    
    if (
        common_are_equal_strings(command, "BLOCK MOUSE") ||
        common_are_equal_strings(command, "STOP MOUSE") ||
        common_are_equal_strings(command, "TOGGLE MOUSE"))
    {
        engine_globals->block_mouse = !engine_globals->block_mouse;
        
        common_strcpy_capped(
            response,
            SINGLE_LINE_MAX,
            "Toggled mouse block");
        return true;
    }
    
    if (
        common_are_equal_strings(command, "SET SHADOWCASTER 0"))
    {
        
        shadowcaster_light_i = 0;
        return true;
    }
    
    if (
        common_are_equal_strings(command, "SHADOWCASTER OFF"))
    {
        
        shadowcaster_light_i = UINT32_MAX;
        return true;
    }
    
    if (common_are_equal_strings(command, "CAMERA TO SHADOWCASTER")) {
        for (uint32_t i = 0; i < zlights_to_apply_size; i++) {
            if (
                shadowcaster_light_i >= 0 &&
                shadowcaster_light_i < zlights_to_apply_size)
            {
                common_strcpy_capped(
                    response,
                    SINGLE_LINE_MAX,
                    "Setting camera to match shadowcaster (light ");
                common_strcat_uint_capped(
                    response,
                    SINGLE_LINE_MAX,
                    shadowcaster_light_i);
                common_strcat_capped(
                    response,
                    SINGLE_LINE_MAX,
                    ")");
                
                camera.xyz[0] = zlights_to_apply[shadowcaster_light_i].xyz[0];
                camera.xyz[1] = zlights_to_apply[shadowcaster_light_i].xyz[1];
                camera.xyz[2] = zlights_to_apply[shadowcaster_light_i].xyz[2];
                
                camera.xyz_angle[0] =
                    zlights_to_apply[shadowcaster_light_i].xyz_angle[0];
                camera.xyz_angle[1] =
                    zlights_to_apply[shadowcaster_light_i].xyz_angle[1];
                camera.xyz_angle[2] =
                    zlights_to_apply[shadowcaster_light_i].xyz_angle[2];
            } else {
                common_strcpy_capped(
                    response,
                    SINGLE_LINE_MAX,
                    "The shadowcaster light is not set...");
            }
        }
        
        return true;
    }
    
    if (
        common_are_equal_strings(command, "DRAW TOP TOUCHABLE") ||
        common_are_equal_strings(command, "DRAW TOP") ||
        common_are_equal_strings(command, "SHOW TOP"))
    {
        engine_globals->draw_top_touchable_id =
            !engine_globals->draw_top_touchable_id;
        
        if (engine_globals->draw_top_touchable_id) {
            common_strcpy_capped(
                response,
                SINGLE_LINE_MAX,
                "Drawing the top touchable_id...");
        } else {
            zsprite_delete(FPS_COUNTER_OBJECT_ID);
            common_strcpy_capped(
                response,
                SINGLE_LINE_MAX,
                "Stopped drawing the top touchable_id...");
        }
        return true;
    }
    
    if (
        common_are_equal_strings(command, "DRAW FPS") ||
        common_are_equal_strings(command, "FPS") ||
        common_are_equal_strings(command, "SHOW FPS"))
    {
        engine_globals->draw_fps = !engine_globals->draw_fps;
        
        if (engine_globals->draw_fps) {
            common_strcpy_capped(
                response,
                SINGLE_LINE_MAX,
                "Drawing the fps counter...");
        } else {
            zsprite_delete(FPS_COUNTER_OBJECT_ID);
            common_strcpy_capped(
                response,
                SINGLE_LINE_MAX,
                "Stopped drawing the fps counter...");
        }
        return true;
    }
    
    if (
        common_are_equal_strings(command, "DRAW NORMALS") ||
        common_are_equal_strings(command, "DRAW IMPUTED NORMALS") ||
        common_are_equal_strings(command, "IMPUTED NORMALS") ||
        common_are_equal_strings(command, "GUESS NORMALS") ||
        common_are_equal_strings(command, "DEDUCE NORMALS"))
   {
       engine_globals->draw_imputed_normals =
           !engine_globals->draw_imputed_normals;
        
       if (engine_globals->draw_imputed_normals) {
            common_strcpy_capped(
                response,
                SINGLE_LINE_MAX,
                "Drawing the 'imputed normals' for for each triangle...");
        } else {
            common_strcpy_capped(
                response,
                SINGLE_LINE_MAX,
                "Stopped drawing the 'imputed normals' for the last touch...");
        }
        return true;
   }
    
    
    if (
        common_are_equal_strings(command, "DRAW CLICKRAY") ||
        common_are_equal_strings(command, "CLICKRAY") ||
        common_are_equal_strings(command, "DRAW CLICKRAYS") ||
        common_are_equal_strings(command, "CLICKRAYS"))
    {
        engine_globals->draw_clickray = !engine_globals->draw_clickray;
        
        if (engine_globals->draw_clickray) {
            common_strcpy_capped(
                response,
                SINGLE_LINE_MAX,
                "Drawing the 'click ray' for the last touch...");
        } else {
            common_strcpy_capped(
                response,
                SINGLE_LINE_MAX,
                "Stopped drawing the 'click ray' for the last touch...");
        }
        return true;
    }
    
    if (
        common_are_equal_strings(command, "DRAW TRIANGLES") ||
        common_are_equal_strings(command, "TRIANGLES"))
    {
        engine_globals->draw_triangles = !engine_globals->draw_triangles;
        
        if (engine_globals->draw_triangles) {
            common_strcpy_capped(
                response,
                SINGLE_LINE_MAX,
                "Drawing triangles...");
        } else {
            common_strcpy_capped(
                response,
                SINGLE_LINE_MAX,
                "Stopped drawing triangles...");
        }
        return true;
    }
    
    if (common_are_equal_strings(command, "FS") ||
        common_are_equal_strings(command, "FULLSCREEN"))
    {
        common_strcpy_capped(
            response,
            SINGLE_LINE_MAX,
            "Entering full screen...");
        terminal_enter_fullscreen_fnc();
        return true;
    }
    
    if (common_are_equal_strings(command, "quit") ||
        common_are_equal_strings(command, "Quit") ||
        common_are_equal_strings(command, "QUIT") ||
        common_are_equal_strings(command, "exit") ||
        common_are_equal_strings(command, "Exit") ||
        common_are_equal_strings(command, "EXIT"))
    {
        platform_close_application();
        return true;
    }
    
    #ifndef LOGGER_IGNORE_ASSERTS
    if (common_are_equal_strings(command, "CRASH")) {
        common_strcpy_capped(
            response,
            SINGLE_LINE_MAX,
            "Forcing the app to crash...");
        log_dump_and_crash("Terminal-induced crash");
        return true;
    }
    #endif
    
    return false;
}

void terminal_commit_or_activate(void) {
    destroy_terminal_objects();
    
    if (
        terminal_active &&
        current_command[0] != '\0')
    {
        common_strcat_capped(
            terminal_history,
            TERMINAL_HISTORY_MAX,
            current_command);
        common_strcat_capped(
            terminal_history,
            TERMINAL_HISTORY_MAX,
            "\n");
        char client_response[SINGLE_LINE_MAX];
        client_response[0] = '\0';
        
        if (
            evaluate_terminal_command(
                current_command,
                client_response))
        {
            common_strcat_capped(
                terminal_history,
                TERMINAL_HISTORY_MAX,
                client_response);
            common_strcat_capped(
                terminal_history,
                TERMINAL_HISTORY_MAX,
                "\n");
        } else {
            client_logic_evaluate_terminal_command(
                current_command,
                client_response,
                SINGLE_LINE_MAX);
            common_strcat_capped(
                terminal_history,
                TERMINAL_HISTORY_MAX,
                client_response);
            common_strcat_capped(
                terminal_history,
                TERMINAL_HISTORY_MAX,
                "\n");
        }
        
        current_command[0] = '\0';
        update_terminal_history_size();
        terminal_redraw_backgrounds();
        requesting_label_update = true;
        return;
    }
    
    terminal_active = !terminal_active;
    
    if (terminal_active) {
        terminal_redraw_backgrounds();
        requesting_label_update = true;
    }
}

#endif // TERMINAL_ACTIVE
