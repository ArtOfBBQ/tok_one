#include "terminal.h"

bool32_t terminal_active = false;

#define SINGLE_LINE_MAX 1024
static char * current_command = NULL;

#define TERMINAL_HISTORY_MAX 500000
#define TERMINAL_WHITESPACE   15.0f

#define TERM_FONT_SIZE        30.0f
#define TERM_Z                0.11f
static char * terminal_history = NULL;
static uint32_t terminal_history_size = 0;

static float term_font_color[4];
static float term_background_color[4];

static int32_t terminal_back_object_id = -1;
static int32_t terminal_labels_object_id = INT32_MAX - 1;

static bool32_t requesting_label_update = false;

static void destroy_terminal_objects(void) {
    for (uint32_t i = 0; i < zpolygons_to_render_size; i++) {
        if (zpolygons_to_render[i].object_id ==
            terminal_back_object_id)
        {
            for (uint32_t tri_i = 0; tri_i < zpolygons_to_render[i].triangles_size; tri_i++) {
                zpolygons_to_render[i].visible = terminal_active;
            }
        }
    }
    
    delete_zpolygon_object(terminal_labels_object_id);
}

static void update_terminal_history_size() {
    terminal_history_size = 0;
    while (terminal_history[terminal_history_size] != '\0') {
        terminal_history_size++;
    }
}

void terminal_init(void) {
    current_command = (char *)malloc_from_unmanaged(
        SINGLE_LINE_MAX);
    
    strcpy_capped(
        current_command,
        SINGLE_LINE_MAX,
        "");
    
    terminal_history = (char *)malloc_from_unmanaged(
        TERMINAL_HISTORY_MAX);
    
    strcpy_capped(
        terminal_history,
        TERMINAL_HISTORY_MAX,
        "TOK ONE embedded debugging terminal v1.0\n");
    
    update_terminal_history_size();
    
    term_font_color[0] = 1.0f;
    term_font_color[1] = 1.0f;
    term_font_color[2] = 1.0f;
    term_font_color[3] = 1.0f;
    
    term_background_color[0] = 0.0f;
    term_background_color[1] = 0.0f;
    term_background_color[2] = 1.0f;
    term_background_color[3] = 1.0f;
}

void terminal_redraw_backgrounds(void) {
    
    terminal_back_object_id = INT32_MAX;
    
    zPolygon current;
    // The current input area
    construct_quad(
        /* const float left_x: */
            screenspace_x_to_x(
                TERMINAL_WHITESPACE,
                TERM_Z),
        /* const float bottom_y: */
            screenspace_y_to_y(
                TERMINAL_WHITESPACE,
                TERM_Z),
        /* const float z: */
            TERM_Z,
        /* const float width: */
            screenspace_width_to_width(
                window_globals->window_width -
                    (TERMINAL_WHITESPACE * 2),
                TERM_Z),
        /* const float height: */
            screenspace_height_to_height(
                font_height + TERMINAL_WHITESPACE,
                TERM_Z),
        /* zPolygon * recipien: */
            &current);
    
    // TODO: color should be independent of mesh
    //    all_meshes[current.mesh_head_i + 0].color[0] = term_background_color[0];
    //    current.triangles[0].color[1] = term_background_color[1];
    //    current.triangles[0].color[2] = term_background_color[2];
    //    current.triangles[0].color[3] = term_background_color[3];
    //    current.triangles[1].color[0] = term_background_color[0];
    //    current.triangles[1].color[1] = term_background_color[1];
    //    current.triangles[1].color[2] = term_background_color[2];
    //    current.triangles[1].color[3] = term_background_color[3];
    current.ignore_camera = true;
    current.ignore_lighting = true;
    current.visible = terminal_active;
    current.object_id = terminal_back_object_id;
    request_zpolygon_to_render(&current);
    
    
    // The console history area
    construct_zpolygon(&current);
    construct_quad(
       /* const float left_x: */
           screenspace_x_to_x(
               TERMINAL_WHITESPACE,
               TERM_Z),
       /* const float bottom_y: */
           screenspace_y_to_y(
               font_height +
                   (TERMINAL_WHITESPACE * 3),
               TERM_Z),
       /* const float z: */
           TERM_Z,
       /* const float width: */
           screenspace_width_to_width(
                window_globals->window_width -
                    (TERMINAL_WHITESPACE * 2),
                TERM_Z),
       /* const float height: */
           screenspace_height_to_height(
               window_globals->window_height -
                   font_height -
                   (TERMINAL_WHITESPACE * 4),
               TERM_Z),
       /* zPolygon * recipien: */
           &current);
    
    // TODO: color should be independent of mesh
    //    current.triangles[0].color[0] = term_background_color[0];
    //    current.triangles[0].color[1] = term_background_color[1];
    //    current.triangles[0].color[2] = term_background_color[2];
    //    current.triangles[0].color[3] = term_background_color[3];
    //    current.triangles[1].color[0] = term_background_color[0];
    //    current.triangles[1].color[1] = term_background_color[1];
    //    current.triangles[1].color[2] = term_background_color[2];
    //    current.triangles[1].color[3] = term_background_color[3];
    current.visible = terminal_active;
    current.ignore_camera = true;
    current.ignore_lighting = true;
    current.object_id = INT32_MAX;
    
    request_zpolygon_to_render(&current);
    
    requesting_label_update = true;
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
        delete_zpolygon_object(
            /* const int32_t with_object_id: */
                terminal_labels_object_id);
        
        float previous_font_height = font_height;
        font_height = TERM_FONT_SIZE;
        
        // draw the terminal's history as a label
        float history_label_top =
            window_globals->window_height -
                (TERMINAL_WHITESPACE * 2.0f) -
                (TERMINAL_WHITESPACE / 4.0f);
        float history_label_height =
            window_globals->window_height -
                font_height -
                (TERMINAL_WHITESPACE * 4.5f);
        
        uint32_t max_lines_in_history =
            (uint32_t)(history_label_height / (font_height * 1.0f));
        
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
        
        font_color[0] = term_font_color[0];
        font_color[1] = term_font_color[1];
        font_color[2] = term_font_color[2];
        font_color[3] = term_font_color[3];
        
        request_label_renderable(
            /* const int32_t with_object_id: */
                terminal_labels_object_id,
            /* const char * text_to_draw: */
                terminal_history + char_offset,
            /* const float left_pixelspace: */
                TERMINAL_WHITESPACE * 2.5f,
            /* const float top_pixelspace: */
                history_label_top,
            /* const float z: */
                TERM_Z - 0.01f,
            /* const float max_width: */
                window_globals->window_width -
                    (TERMINAL_WHITESPACE * 2),
            /* const bool32_t ignore_camera: */
                true);
        
        if (current_command[0] == '\0') {
            font_height = previous_font_height;
            requesting_label_update = false;
            return;
        }
        
        // the terminal's current input as a label
        request_label_renderable(
            /* const int32_t with_object_id: */
                terminal_labels_object_id,
            /* const char * text_to_draw: */
                current_command,
            /* const float left_pixelspace: */
                TERMINAL_WHITESPACE * 2.5f,
            /* const float top_pixelspace: */
                font_height +
                    (TERMINAL_WHITESPACE / 4),
            /* const float z: */
                TERM_Z - 0.01f,
            /* const float max_width: */
                window_globals->window_width -
                    (TERMINAL_WHITESPACE * 2),
            /* const bool32_t ignore_camera: */
                true);
        
        font_height = previous_font_height;
        
        requesting_label_update = false;
    }
}

void terminal_sendchar(uint32_t to_send) {
    
    if (
        to_send == TOK_KEY_ESCAPE)
    {
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
        are_equal_strings(command, "RESET CAMERA") ||
        are_equal_strings(command, "CENTER CAMERA"))
    {
        camera.x = 0.0f;
        camera.y = 0.0f;
        camera.z = 0.0f;
        camera.x_angle = 0.0f;
        camera.y_angle = 0.0f;
        camera.z_angle = 0.0f;
        strcpy_capped(
            response,
            SINGLE_LINE_MAX,
            "Reset camera position and angles to {0,0,0}");
        return true;
    }
    
    if (are_equal_strings(command, "ZLIGHTS")) {
        strcpy_capped(
            response,
            SINGLE_LINE_MAX,
            "# of zLights: ");
        strcat_uint_capped(
            response,
            SINGLE_LINE_MAX,
            zlights_to_apply_size);
        strcat_capped(
            response,
            SINGLE_LINE_MAX,
            " of ");
        strcat_uint_capped(
            response,
            SINGLE_LINE_MAX,
            ZLIGHTS_TO_APPLY_ARRAYSIZE);
        return true;
    }
    
    if (
        are_equal_strings(command, "WINDOW"))
    {
        strcpy_capped(
            response,
            SINGLE_LINE_MAX,
            "window height: ");
        strcat_uint_capped(
            response,
            SINGLE_LINE_MAX,
            (uint32_t)window_globals->window_height);
        strcat_capped(
            response,
            SINGLE_LINE_MAX,
            ", width: ");
        strcat_uint_capped(
            response,
            SINGLE_LINE_MAX,
            (uint32_t)window_globals->window_width);
        return true;
    }
    
    if (
        are_equal_strings(command, "VISUAL DEBUG") ||
        are_equal_strings(command, "DEBUG") ||
        are_equal_strings(command, "DEBUG LINES") ||
        are_equal_strings(command, "SHOW LINES") ||
        are_equal_strings(command, "OUTLINES") ||
        are_equal_strings(command, "SHOW OUTLINES"))
    {
        window_globals->visual_debug_mode = !window_globals->visual_debug_mode;
        if (!window_globals->visual_debug_mode) {
            delete_zpolygon_object(0);
            strcpy_capped(
                response,
                SINGLE_LINE_MAX,
                "Exiting visual debug mode");
        } else {
            strcpy_capped(
                response,
                SINGLE_LINE_MAX,
                "Activating visual debug mode");
        }
        
        return true;
    }
    
    if (are_equal_strings(command, "quit") ||
        are_equal_strings(command, "Quit") ||
        are_equal_strings(command, "QUIT") ||
        are_equal_strings(command, "exit") ||
        are_equal_strings(command, "Exit") ||
        are_equal_strings(command, "EXIT"))
    {
        platform_close_application();
        return true;
    }
    
    if (are_equal_strings(command, "CRASH")) {
        strcpy_capped(
            response,
            SINGLE_LINE_MAX,
            "Forcing the app to crash...");
        log_dump_and_crash("Terminal-induced crash");
        return true;
    }
    
    return false;
}

void terminal_commit_or_activate(void) {
    
    requesting_label_update = true;
    
    if (
        terminal_active &&
        current_command[0] != '\0')
    {
        strcat_capped(terminal_history, TERMINAL_HISTORY_MAX, current_command);
        strcat_capped(terminal_history, TERMINAL_HISTORY_MAX, "\n");
        char client_response[SINGLE_LINE_MAX];
        client_response[0] = '\0';
        
        if (
            evaluate_terminal_command(
                current_command,
                client_response))
        {
            strcat_capped(
                terminal_history,
                TERMINAL_HISTORY_MAX,
                client_response);
            strcat_capped(
                terminal_history,
                TERMINAL_HISTORY_MAX,
                "\n");
            current_command[0] = '\0';
            update_terminal_history_size();
            return;
        } else {
            client_logic_evaluate_terminal_command(
                current_command,
                client_response,
                SINGLE_LINE_MAX);
            strcat_capped(
                terminal_history,
                TERMINAL_HISTORY_MAX,
                client_response);
            strcat_capped(
                terminal_history,
                TERMINAL_HISTORY_MAX,
                "\n");
            current_command[0] = '\0';
            update_terminal_history_size();
            return;
        }
    }
    
    terminal_active = !terminal_active;
    
    destroy_terminal_objects();    
}
