#include "T1_profiler.h"

#if T1_PROFILER_ACTIVE == T1_ACTIVE

#ifndef __x86_64__
static uint64_t __rdtsc(void)
{
    return __builtin_readcyclecounter();
}
#endif

#ifdef NDEBUG
#define SLOW_FRAME_CYCLES 120000000
#else
#define SLOW_FRAME_CYCLES 150000000
#endif

#define GUI_TOP_MESSAGE_MAX 128
static uint64_t acceptable_frame_clock_cycles = 1;

static char gui_top_message[GUI_TOP_MESSAGE_MAX];

static bool32_t profiler_paused = false;

int32_t profiler_object_id = INT32_MAX;
int32_t profiler_touch_id = -1;

int32_t frame_selection_touch_ids[2];

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

#define PROFILE_TREE_MAX 15000

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
    uint8_t simd_padding[16];
} Frame;

static Frame * frames = NULL;
static uint16_t frame_i = 0;

void T1_profiler_init(
    const uint64_t clock_frequency,
    void * (* profiler_malloc_function)(size_t))
{
    acceptable_frame_clock_cycles = clock_frequency / 60;
    
    T1_log_assert(sizeof(Frame) % 32 == 0); // simd padding, 32 bytes for AVX
    
    frames = profiler_malloc_function(
        sizeof(Frame) * FRAMES_MAX);
    T1_log_assert(frames != NULL);
    T1_std_memset(frames, 0, sizeof(Frame) * FRAMES_MAX);
    
    function_stack = profiler_malloc_function(
        sizeof(uint32_t) * FUNCTION_STACK_MAX);
    T1_log_assert(function_stack != NULL);
    T1_std_memset(function_stack, 0, sizeof(uint32_t) * FUNCTION_STACK_MAX);
    
    gui_function_stack = profiler_malloc_function(
        sizeof(uint32_t) * FUNCTION_STACK_MAX);
    T1_std_memset(gui_function_stack, 0, sizeof(uint32_t) * FUNCTION_STACK_MAX);
    
    profiler_object_id = T1_zspriteid_next_nonui_id();
    profiler_touch_id = T1_zspriteid_next_nonui_touch_id();
    
    gui_selected_frames[0] = 0;
    gui_selected_frames[1] = 1;
    
    T1_std_strcpy_cap(
        gui_top_message,
        GUI_TOP_MESSAGE_MAX,
        "Profiler intialized...");
    
    frame_selection_touch_ids[0] = T1_zspriteid_next_nonui_touch_id();
    frame_selection_touch_ids[1] = T1_zspriteid_next_nonui_touch_id();
}

void T1_profiler_new_frame(void) {
    if (profiler_paused && !T1_global->pause_profiler) {
        frames[frame_i].started_at = 0;
    }
    
    profiler_paused = T1_global->pause_profiler;
    
    if (profiler_paused) {
        return;
    } else {
        T1_std_strcpy_cap(gui_top_message, GUI_TOP_MESSAGE_MAX, "Running...");
        function_stack_size = 0;
        gui_function_stack_size = 0;
    }
    
    frames[frame_i].elapsed = (uint64_t)__rdtsc() - frames[frame_i].started_at;
    if (frames[frame_i].started_at == 0) {
        frames[frame_i].elapsed = SLOW_FRAME_CYCLES - 1;
    } else if (
        frames[frame_i].elapsed > SLOW_FRAME_CYCLES)
    {
        T1_std_strcpy_cap(
            gui_top_message,
            GUI_TOP_MESSAGE_MAX,
            "PAUSED - frame ");
        T1_std_strcat_uint_cap(
            gui_top_message,
            GUI_TOP_MESSAGE_MAX,
            frame_i);
        T1_std_strcat_cap(
            gui_top_message,
            GUI_TOP_MESSAGE_MAX,
            " took ");
        T1_std_strcat_uint_cap(
            gui_top_message,
            GUI_TOP_MESSAGE_MAX,
            (uint32_t)frames[frame_i].elapsed);
        T1_std_strcat_cap(gui_top_message, GUI_TOP_MESSAGE_MAX, " cycles.");
        T1_global->pause_profiler = true;
        return;
    }
    
    frame_i += 1;
    frame_i %= FRAMES_MAX;
    
    T1_std_memset(&frames[frame_i], 0, sizeof(Frame));
    frames[frame_i].started_at = __rdtsc();
}

void T1_profiler_start(const char * function_name)
{
    if (
        profiler_paused ||
        frames == NULL ||
        function_stack == NULL)
    {
        return;
    }
    
    ProfiledNode * new_node = NULL;
    
    if (function_stack_size > 0) {
        uint32_t parent_prof_i = function_stack[function_stack_size - 1];
        
        T1_log_assert(frames[frame_i].profiles[parent_prof_i].
            children_size < MAX_CHILDREN_PER_NODE);
        
        for (
            int32_t i = 0;
            i < frames[frame_i].profiles[parent_prof_i].children[i];
            i++)
        {
            if (
                T1_std_are_equal_strings(
                    /* char *string_1: */
                        frames[frame_i].profiles[
                            frames[frame_i].profiles[parent_prof_i].
                                children[i]].description,
                    /* char *string_2: */
                        function_name))
            {
                uint16_t new_i = frames[frame_i].profiles[parent_prof_i].
                    children[i];
                new_node = &frames[frame_i].profiles[new_i];
                T1_log_assert(new_node->elapsed_count > 0);
                function_stack[function_stack_size] = new_i;
                T1_log_assert(new_node != NULL);
                T1_log_assert(new_node->parents_size > 0);
                T1_log_assert(new_node->elapsed_count > 0);
                break;
            }
        }
        
        if (new_node == NULL) {
            uint16_t new_i = frames[frame_i].profiles_size;
            function_stack[function_stack_size] = new_i;
            
            new_node = &frames[frame_i].profiles[new_i];
            T1_log_assert(new_node != NULL);
            T1_log_assert(frames[frame_i].profiles[parent_prof_i].children_size <
                MAX_CHILDREN_PER_NODE);
            frames[frame_i].profiles[parent_prof_i].children[frames[frame_i].
                profiles[parent_prof_i].children_size] = new_i;
            frames[frame_i].profiles[parent_prof_i].children_size += 1;
            T1_log_assert(frames[frame_i].profiles[parent_prof_i].children_size <
                MAX_CHILDREN_PER_NODE);
            
            new_node->parents_size = frames[frame_i].profiles[parent_prof_i].
                parents_size + 1;
            new_node->children_size = 0;
            new_node->elapsed_count = 0;
            new_node->elapsed_max = 0;
            new_node->elapsed_min = UINT64_MAX;
            new_node->elapsed_total = 0;
            new_node->elapsed_mostrecent = 0;
            
            T1_std_strcpy_cap(
                new_node->description,
                MAX_DESCRIPTION_SIZE,
                function_name);
            
            T1_log_assert(frames[frame_i].profiles_size < PROFILE_TREE_MAX);
            frames[frame_i].profiles_size += 1;
        }
    } else {
        uint16_t new_i = frames[frame_i].profiles_size;
        function_stack[function_stack_size] = new_i;
        new_node = &frames[frame_i].profiles[new_i];
        T1_std_strcpy_cap(
            new_node->description,
            MAX_DESCRIPTION_SIZE,
            function_name);
        T1_log_assert(new_node->parents_size == 0);
        T1_log_assert(new_node->children_size == 0);
        T1_log_assert(new_node->elapsed_total == 0);
        T1_log_assert(new_node->elapsed_count == 0);
        T1_log_assert(frames[frame_i].profiles_size < PROFILE_TREE_MAX);
        frames[frame_i].profiles_size += 1;
    }
    
    T1_log_assert(new_node != NULL);
    
    function_stack_size += 1;
    
    new_node->last_start = __rdtsc();
}

void T1_profiler_end(const char * function_name)
{
    #if T1_LOGGER_ASSERTS_ACTIVE
    (void)function_name;
    #endif
    
    if (
        profiler_paused ||
        frames == NULL ||
        function_stack == NULL)
    {
        return;
    }
    
    T1_log_assert(function_stack_size > 0);
    uint32_t prof_i =
        function_stack[function_stack_size - 1];
    function_stack_size -= 1;
    
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

void T1_profiler_handle_touches(void) {
    if (
        !T1_io_events[T1_IO_LAST_TOUCH_OR_LCLICK_START].handled)
    {
        if (
            T1_io_events[T1_IO_LAST_TOUCH_OR_LCLICK_START].
                touch_id_top == profiler_touch_id)
        {
            T1_io_events[T1_IO_LAST_TOUCH_OR_LCLICK_START].
                handled = true;
            return;
        }
        
        if (
            T1_io_events[T1_IO_LAST_TOUCH_OR_LCLICK_START].
                touch_id_top == frame_selection_touch_ids[0])
        {
            T1_io_events[T1_IO_LAST_TOUCH_OR_LCLICK_START].
                handled = true;
            gui_selected_frames[0] += 1;
            gui_selected_frames[0] %= FRAMES_MAX;
            return;
        }
        if (
            T1_io_events[T1_IO_LAST_TOUCH_OR_LCLICK_START].
                touch_id_top == frame_selection_touch_ids[1])
        {
            T1_io_events[T1_IO_LAST_TOUCH_OR_LCLICK_START].
                handled = true;
            gui_selected_frames[1] += 1;
            gui_selected_frames[1] %= FRAMES_MAX;
            return;
        }
    }
}

void T1_profiler_draw_labels(void) {
    T1_zsprite_delete(profiler_object_id);
    
    if (T1_global->show_profiler) {
        T1zSpriteRequest profiler_backdrop;
        T1_zsprite_fetch_next(&profiler_backdrop);
        T1_zsprite_construct_quad_around(
            /* const float mid_x: */
                0.0f,
            /* const float mid_y: */
                0.0f,
            /* const float z: */
                PROFILER_Z,
            /* const float width: */
                T1_global->window_width,
            /* const float height: */
                T1_global->window_height,
            /* PolygonRequest *stack_recipient: */
                &profiler_backdrop);
        profiler_backdrop.cpu_data->zsprite_id = profiler_object_id;
        profiler_backdrop.gpu_data->
            f32.ignore_camera = 1.0f;
        profiler_backdrop.gpu_data->
            f32.ignore_lighting = 1.0f;
        profiler_backdrop.gpu_data->f32.base_mat_f32.diffuse_rgb[0] = 0.50f;
        profiler_backdrop.gpu_data->f32.base_mat_f32.diffuse_rgb[1] = 0.50f;
        profiler_backdrop.gpu_data->f32.base_mat_f32.diffuse_rgb[2] = 0.50f;
        profiler_backdrop.gpu_data->f32.base_mat_f32.alpha = 0.75f;
        profiler_backdrop.cpu_data->simd_stats.alpha_blending_on = true;
        profiler_backdrop.gpu_data->i32.touch_id = -1;
        T1_zsprite_commit(&profiler_backdrop);
        
        T1_text_props->f32.rgba[0] = 0.1f;
        T1_text_props->f32.rgba[1] = 0.1f;
        T1_text_props->f32.rgba[2] = 0.1f;
        T1_text_props->f32.rgba[3] = 1.0f;
        T1_text_props->font_height = 18.0f;
        
        T1_text_request_label_renderable(
            /* const int32_t with_object_id: */
                profiler_object_id,
            /* const char * text_to_draw: */
                gui_top_message,
            /* const float left_pixelspace: */
                20,
            /* const float mid_y_pixelspace: */
                T1_global->window_height - 20,
            /* const float z: */
                PROFILER_Z - 0.02f,
            /* const float max_width: */
                T1_global->window_width);
        
        for (
            int32_t gui_frame_i = 0;
            gui_frame_i < 2;
            gui_frame_i++)
        {
            T1_text_props->f32.rgba[0] = 0.1f;
            float gui_frame_left = 20 +
                (gui_frame_i * T1_global->window_width / 2);
            
            int32_t f_i = gui_selected_frames[gui_frame_i];
            
            float cur_top = T1_global->window_height -
                20 -
                (T1_text_props->font_height + 2.0f);
            
            char line_text[128];
            T1_std_strcpy_cap(line_text, 128, "Selected frame: ");
            T1_std_strcat_int_cap(
                line_text,
                128,
                f_i);
            T1_text_props->i32.touch_id = frame_selection_touch_ids[
                gui_frame_i];
            
            T1_text_request_label_renderable(
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
                    T1_global->window_width);
            
            T1_text_props->i32.touch_id = -1;
            
            cur_top -= (T1_text_props->font_height + 2.0f);
            T1_std_strcpy_cap(line_text, 128, "Cycles: ");
            T1_std_strcat_uint_cap(
                line_text,
                128,
                (uint32_t)frames[f_i].elapsed);
            T1_std_strcat_cap(line_text, 128, " (");
            float pct_of_acceptable = (float)frames[f_i].elapsed /
                (float)acceptable_frame_clock_cycles;
            T1_std_strcat_float_cap(
                line_text,
                128,
                pct_of_acceptable);
            T1_std_strcat_cap(line_text, 128, ")");
            T1_text_request_label_renderable(
                /* const int32_t with_object_id: */
                    profiler_object_id,
                /* const char * text_to_draw: */
                    line_text,
                /* const float left_pixelspace: */
                    gui_frame_left,
                /* const float mid_y_pixelspace: */
                    cur_top,
                /* const float z: */
                    PROFILER_Z - 0.02f,
                /* const float max_width: */
                    T1_global->window_width);
            
            if (frames[f_i].profiles_size > 0) {
                gui_function_stack[0] = 0;
                gui_function_stack_size = 1;
            } else {
                gui_function_stack_size = 0;
            }
            
            while (gui_function_stack_size > 0 && frames[f_i].elapsed > 0) {
                uint32_t func_i = gui_function_stack[
                    gui_function_stack_size - 1];
                gui_function_stack_size -= 1;
                
                T1_log_assert(frames[f_i].profiles[func_i].elapsed_total > 0);
                
                float pct_elapsed =
                    (float)frames[f_i].profiles[func_i].elapsed_total /
                        (float)acceptable_frame_clock_cycles;
                T1_text_props->f32.rgba[0] = 0.1f + (pct_elapsed * 2);
                if (
                    T1_text_props->f32.rgba[0] > 1.0f)
                {
                    T1_text_props->f32.rgba[0] = 1.0f;
                }
                
                if (pct_elapsed >= 0.0f) {
                    // push all child nodes onto the stack
                    for (
                         uint32_t child_i = 0;
                         child_i < frames[f_i].profiles[func_i].children_size;
                         child_i++)
                    {
                        uint16_t child_prof_i = frames[f_i].profiles[func_i].
                            children[child_i];
                        gui_function_stack[gui_function_stack_size] =
                            child_prof_i;
                        T1_log_assert(
                           frames[f_i].profiles[child_prof_i].parents_size > 0);
                        gui_function_stack_size += 1;
                    }
                }
                
                T1_std_strcpy_cap(
                    line_text,
                    128,
                    frames[f_i].profiles[func_i].description);
                T1_std_strcat_cap(
                    line_text,
                    128,
                    " ");
                T1_std_strcat_uint_cap(
                    line_text,
                    128,
                    (uint32_t)frames[f_i].
                        profiles[func_i].elapsed_total);
                T1_std_strcat_cap(
                    line_text,
                    128,
                    " (");
                
                T1_log_assert(pct_elapsed >= 0.0f);
                T1_std_strcat_int_cap(
                    line_text,
                    128,
                    (int)(pct_elapsed * 100.0f));
                T1_std_strcat_cap(
                    line_text,
                    128,
                    "%)");
                
                cur_top -= (T1_text_props->font_height + 2.0f);
                T1_text_request_label_renderable(
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
                        T1_global->window_width);
            }
        }
    }
}
#elif T1_PROFILER_ACTIVE == T1_INACTIVE
// Pass
#else
#error "T1_PROFILER_ACTIVE undefined!"
#endif // T1_PROFILER_ACTIVE
