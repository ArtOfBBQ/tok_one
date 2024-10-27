#include "profiler.h"

#ifdef PROFILER_ACTIVE

#ifndef __x86_64__
static uint64_t __rdtsc(void)
{
    return 1;
}
#endif

#ifdef NDEBUG
#define SLOW_FRAME_CYCLES 600000000
#else
#define SLOW_FRAME_CYCLES 6000000000
#endif

#define GUI_TOP_MESSAGE_MAX 128
static uint64_t acceptable_frame_clock_cycles = 1;

static char gui_top_message[GUI_TOP_MESSAGE_MAX];

static bool32_t profiler_paused = false;

int32_t profiler_object_id = INT32_MAX;
int32_t profiler_touchable_id = -1;

int32_t frame_selection_touchable_ids[2];

int32_t gui_selected_frames[2];

#define PROFILER_Z 0.13f

#define MAX_DESCRIPTION_SIZE 256
#define MAX_CHILDREN_PER_NODE 12
typedef struct ProfiledNode {
    char description[MAX_DESCRIPTION_SIZE];
    uint64_t elapsed_min;
    uint64_t elapsed_max;
    uint64_t elapsed_mostrecent;
    uint64_t elapsed_total;
    uint64_t last_start;
    uint32_t elapsed_count;
    uint16_t children[MAX_CHILDREN_PER_NODE];
    uint8_t children_size;
    uint8_t parents_size;
} ProfiledNode;

#define PROFILE_TREE_MAX 1000

#define FUNCTION_STACK_MAX 10000
static uint32_t * function_stack = NULL;
static uint16_t function_stack_size = 0;

static uint32_t * gui_function_stack = NULL;
static uint16_t gui_function_stack_size = 0;

#define FRAMES_MAX 10
typedef struct Frame {
    ProfiledNode profiles[PROFILE_TREE_MAX];
    uint64_t started_at;
    uint64_t elapsed;
    uint8_t hit[PROFILE_TREE_MAX];
    uint16_t profiles_size;
    /* uint8_t simd_padding[8]; */
} Frame;

static Frame * frames = NULL;
static uint16_t frame_i = 0;

void profiler_init(
    const uint64_t clock_frequency,
    void * (* profiler_malloc_function)(size_t))
{
    acceptable_frame_clock_cycles = clock_frequency / 60;
    
    log_assert(sizeof(Frame) % 32 == 0); // simd padding, 32 bytes for AVX
    
    frames = profiler_malloc_function(
        sizeof(Frame) * FRAMES_MAX);
    memset_char(frames, 0, sizeof(Frame) * FRAMES_MAX);
    
    function_stack = profiler_malloc_function(
        sizeof(uint32_t) * FUNCTION_STACK_MAX);
    memset_char(function_stack, 0, sizeof(uint32_t) * FUNCTION_STACK_MAX);
    
    gui_function_stack = profiler_malloc_function(
        sizeof(uint32_t) * FUNCTION_STACK_MAX);
    memset_char(gui_function_stack, 0, sizeof(uint32_t) * FUNCTION_STACK_MAX);
    
    profiler_object_id = next_nonui_object_id();
    profiler_touchable_id = next_nonui_touchable_id();
    
    gui_selected_frames[0] = 0;
    gui_selected_frames[1] = 1;
    
    strcpy_capped(
        gui_top_message,
        GUI_TOP_MESSAGE_MAX,
        "Profiler intialized...");
    
    frame_selection_touchable_ids[0] = next_nonui_touchable_id();
    frame_selection_touchable_ids[1] = next_nonui_touchable_id();
}

void profiler_new_frame(void) {
    if (profiler_paused && !window_globals->pause_profiler) {
        frames[frame_i].started_at = 0;
    }
    
    profiler_paused = window_globals->pause_profiler;
    
    if (profiler_paused) {
        return;
    } else {
        strcpy_capped(gui_top_message, GUI_TOP_MESSAGE_MAX, "Running...");
    }
    
    frames[frame_i].elapsed = (uint64_t)__rdtsc() - frames[frame_i].started_at;
    if (
        frames[frame_i].elapsed > SLOW_FRAME_CYCLES &&
        frames[frame_i].started_at != 0)
    {
        strcpy_capped(gui_top_message, GUI_TOP_MESSAGE_MAX, "PAUSED - frame ");
        strcat_uint_capped(gui_top_message, GUI_TOP_MESSAGE_MAX, frame_i);
        strcat_capped(gui_top_message, GUI_TOP_MESSAGE_MAX, " took ");
        strcat_uint_capped(
            gui_top_message,
            GUI_TOP_MESSAGE_MAX,
            (uint32_t)frames[frame_i].elapsed);
        strcat_capped(gui_top_message, GUI_TOP_MESSAGE_MAX, " cycles.");
        window_globals->pause_profiler = true;
        return;
    }
    
    frame_i += 1;
    frame_i %= FRAMES_MAX;
    
    memset_char(&frames[frame_i], 0, sizeof(Frame));
    frames[frame_i].started_at = __rdtsc();
}

void profiler_start(const char * function_name)
{
    if (profiler_paused) {
        return;
    }
    
    ProfiledNode * new_node = NULL;
    
    if (function_stack_size > 0) {
        uint32_t parent_prof_i = function_stack[function_stack_size - 1];
        
        log_assert(frames[frame_i].profiles[parent_prof_i].
            children_size < MAX_CHILDREN_PER_NODE);
        
        for (
            int32_t i = 0;
            i < frames[frame_i].profiles[parent_prof_i].children[i];
            i++)
        {
            if (
                strings_are_equal(
                    /* char *string_1: */
                        frames[frame_i].profiles[
                            frames[frame_i].profiles[parent_prof_i].
                                children[i]].description,
                    /* char *string_2: */
                        function_name))
            {
                new_node = &frames[frame_i].profiles[
                    frames[frame_i].profiles[parent_prof_i].children[i]];
                function_stack[function_stack_size] =
                    frames[frame_i].profiles[parent_prof_i].children[i];
                log_assert(new_node != NULL);
                log_assert(new_node->parents_size > 0);
                break;
            }
        }
        
        if (new_node == NULL) {
            function_stack[function_stack_size] =
                frames[frame_i].profiles_size;
            
            new_node = &frames[frame_i].profiles[frames[frame_i].profiles_size];
            log_assert(new_node != NULL);
            frames[frame_i].profiles[parent_prof_i].children[frames[frame_i].
                profiles[parent_prof_i].children_size] =
                    frames[frame_i].profiles_size;
            new_node->parents_size =
                frames[frame_i].profiles[parent_prof_i].parents_size + 1;
                new_node->children_size = 0;
            new_node->elapsed_count = 0;
            new_node->elapsed_max = 0;
            new_node->elapsed_min = UINT64_MAX;
            new_node->elapsed_total = 0;
            new_node->elapsed_mostrecent = 0;
            frames[frame_i].profiles[parent_prof_i].children_size += 1;
        }
    } else {
        new_node = &frames[frame_i].profiles[0];
    }
    
    log_assert(new_node != NULL);
    
    function_stack_size += 1;
    
    new_node->last_start = __rdtsc();
    strcpy_capped(
        new_node->description,
        MAX_DESCRIPTION_SIZE,
        function_name);
    frames[frame_i].profiles_size += 1;
}

void profiler_end(const char * function_name)
{
    if (profiler_paused) {
        return;
    }
    
    log_assert(function_stack_size > 0);
    uint32_t prof_i = function_stack[function_stack_size - 1];
    function_stack_size -= 1;
    
    log_assert(
        are_equal_strings(frames[frame_i].profiles[prof_i].description,
        function_name));
    
    uint64_t elapsed = __rdtsc() - frames[frame_i].profiles[prof_i].last_start;
    frames[frame_i].profiles[prof_i].elapsed_count += 1;
    frames[frame_i].profiles[prof_i].elapsed_total += elapsed;
    frames[frame_i].profiles[prof_i].elapsed_mostrecent = elapsed;
    if (elapsed > frames[frame_i].profiles[prof_i].elapsed_max) {
        frames[frame_i].profiles[prof_i].elapsed_max = elapsed;
    }
    if (elapsed < frames[frame_i].profiles[prof_i].elapsed_min) {
        frames[frame_i].profiles[prof_i].elapsed_min = elapsed;
    }
}

void profiler_handle_touches(void) {
    if (
        !user_interactions[INTR_PREVIOUS_TOUCH_OR_LEFTCLICK_START].handled)
    {
        if (
            user_interactions[INTR_PREVIOUS_TOUCH_OR_LEFTCLICK_START].
                touchable_id == profiler_touchable_id)
        {
            user_interactions[INTR_PREVIOUS_TOUCH_OR_LEFTCLICK_START].
                handled = true;
            printf("profiler touched\n");
            return;
        }
        
        if (
            user_interactions[INTR_PREVIOUS_TOUCH_OR_LEFTCLICK_START].
                touchable_id == frame_selection_touchable_ids[0])
        {
            user_interactions[INTR_PREVIOUS_TOUCH_OR_LEFTCLICK_START].
                handled = true;
            gui_selected_frames[0] += 1;
            gui_selected_frames[0] %= FRAMES_MAX;
            return;
        }
        if (
            user_interactions[INTR_PREVIOUS_TOUCH_OR_LEFTCLICK_START].
                touchable_id == frame_selection_touchable_ids[1])
        {
            user_interactions[INTR_PREVIOUS_TOUCH_OR_LEFTCLICK_START].
                handled = true;
            gui_selected_frames[1] += 1;
            gui_selected_frames[1] %= FRAMES_MAX;
            return;
        }
    }
}

void profiler_draw_labels(void) {
    delete_zpolygon_object(profiler_object_id);
    
    if (window_globals->show_profiler) {
        PolygonRequest profiler_backdrop;
        request_next_zpolygon(&profiler_backdrop);
        construct_quad_around(
            /* const float mid_x: */
                0.0f,
            /* const float mid_y: */
                0.0f,
            /* const float z: */
                PROFILER_Z,
            /* const float width: */
                window_globals->window_width,
            /* const float height: */
                window_globals->window_height,
            /* PolygonRequest *stack_recipient: */
                &profiler_backdrop);
        profiler_backdrop.cpu_data->object_id = profiler_object_id;
        profiler_backdrop.gpu_data->ignore_camera = 1.0f;
        profiler_backdrop.gpu_data->ignore_lighting = 1.0f;
        profiler_backdrop.materials_size = 1;
        profiler_backdrop.gpu_materials[0].rgba[0] = 0.50f;
        profiler_backdrop.gpu_materials[0].rgba[1] = 0.50f;
        profiler_backdrop.gpu_materials[0].rgba[2] = 0.50f;
        profiler_backdrop.gpu_materials[0].rgba[3] = 0.75f;
        profiler_backdrop.cpu_data->alpha_blending_enabled = true;
        profiler_backdrop.cpu_data->touchable_id = -1;
        commit_zpolygon_to_render(&profiler_backdrop);
        
        font_color[0] = 0.1f;
        font_color[1] = 0.1f;
        font_color[2] = 0.1f;
        font_color[3] = 1.0f;
        font_height = 18.0f;
        
        text_request_label_renderable(
            /* const int32_t with_object_id: */
                profiler_object_id,
            /* const char * text_to_draw: */
                gui_top_message,
            /* const float left_pixelspace: */
                20,
            /* const float top_pixelspace: */
                window_globals->window_height - 20,
            /* const float z: */
                PROFILER_Z - 0.02f,
            /* const float max_width: */
                window_globals->window_width,
            /* const bool32_t ignore_camera: */
                true);
        
        for (int32_t gui_frame_i = 0; gui_frame_i < 2; gui_frame_i++) {
            font_color[0] = 0.1f;
            float gui_frame_left = 20 +
                (gui_frame_i * window_globals->window_width / 2);
            
            int32_t f_i = gui_selected_frames[gui_frame_i];
            
            float cur_top = window_globals->window_height -
                20 -
                (font_height + 2.0f);
            
            char line_text[128];
            strcpy_capped(line_text, 128, "Selected frame: ");
            strcat_int_capped(
                line_text,
                128,
                f_i);
            font_touchable_id = frame_selection_touchable_ids[gui_frame_i];
            text_request_label_renderable(
                /* const int32_t with_object_id: */
                    profiler_object_id,
                /* const char * text_to_draw: */
                    line_text,
                /* const float left_pixelspace: */
                    gui_frame_left,
                /* const float top_pixelspace: */
                    cur_top,
                /* const float z: */
                    PROFILER_Z - 0.02f,
                /* const float max_width: */
                    window_globals->window_width,
                /* const bool32_t ignore_camera: */
                    true);
            
            font_touchable_id = -1;
            
            cur_top -= (font_height + 2.0f);
            strcpy_capped(line_text, 128, "Cycles: ");
            strcat_uint_capped(
                line_text,
                128,
                (uint32_t)frames[f_i].elapsed);
            strcat_capped(line_text, 128, " (");
            float pct_of_acceptable = (float)frames[f_i].elapsed /
                (float)acceptable_frame_clock_cycles;
            strcat_float_capped(
                line_text,
                128,
                pct_of_acceptable);
            strcat_capped(line_text, 128, ")");
            text_request_label_renderable(
                /* const int32_t with_object_id: */
                    profiler_object_id,
                /* const char * text_to_draw: */
                    line_text,
                /* const float left_pixelspace: */
                    gui_frame_left,
                /* const float top_pixelspace: */
                    cur_top,
                /* const float z: */
                    PROFILER_Z - 0.02f,
                /* const float max_width: */
                    window_globals->window_width,
                /* const bool32_t ignore_camera: */
                    true);
            
            if (frames[f_i].profiles_size > 0) {
                gui_function_stack_size = 0;
                gui_function_stack[gui_function_stack_size++] = 0;
            }
            
            while (gui_function_stack_size > 0 && frames[f_i].elapsed > 0) {
                uint32_t func_i = gui_function_stack[
                    gui_function_stack_size - 1];
                gui_function_stack_size -= 1;
                
                float pct_elapsed =
                    (float)frames[f_i].profiles[func_i].elapsed_total /
                        (float)acceptable_frame_clock_cycles;
                font_color[0] = 0.1f + (pct_elapsed * 2);
                if (font_color[0] > 1.0f) { font_color[0] = 1.0f; }
                
                if (pct_elapsed >= 0.05f) {
                    // push all child nodes onto the stack
                    for (
                         uint32_t child_i = 0;
                         child_i < frames[f_i].
                         profiles[func_i].children_size;
                         child_i++)
                    {
                        gui_function_stack[gui_function_stack_size] =
                        frames[f_i].profiles[func_i].children[child_i];
                        log_assert(
                           frames[f_i].profiles[
                               gui_function_stack[gui_function_stack_size]].
                                   parents_size > 0);
                        gui_function_stack_size += 1;
                    }
                }
                
                strcpy_capped(
                    line_text,
                    128,
                    frames[f_i].profiles[func_i].description);
                strcat_capped(
                    line_text,
                    128,
                    " ");
                strcat_uint_capped(
                    line_text,
                    128,
                    (uint32_t)frames[f_i].
                        profiles[func_i].elapsed_total);
                strcat_capped(
                    line_text,
                    128,
                    " (");
                
                log_assert(pct_elapsed >= 0.0f);
                strcat_int_capped(
                    line_text,
                    128,
                    (int)(pct_elapsed * 100.0f));
                strcat_capped(
                    line_text,
                    128,
                    "%)");
                
                cur_top -= (font_height + 2.0f);
                text_request_label_renderable(
                    /* const int32_t with_object_id: */
                        profiler_object_id,
                    /* const char * text_to_draw: */
                        line_text,
                    /* const float left_pixelspace: */
                        gui_frame_left + (
                            frames[f_i].profiles[func_i].parents_size * 30.0f),
                    /* const float top_pixelspace: */
                        cur_top,
                    /* const float z: */
                        PROFILER_Z - 0.02f,
                    /* const float max_width: */
                        window_globals->window_width,
                    /* const bool32_t ignore_camera: */
                        true);
            }
        }
    }
}

#endif