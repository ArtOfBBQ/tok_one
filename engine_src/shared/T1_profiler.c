#include "T1_profiler.h"

#include "T1_std.h"
#include "T1_log.h"
#include "T1_id.h"
#include "T1_io.h"

#include "T1_settings.h"
#include "T1_global.h"
#include "T1_zsprite.h"
#include "T1_text.h"


#if T1_PROFILER_ACTIVE == T1_ACTIVE

#ifndef __x86_64__
static u64 __rdtsc(void)
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
static u64 acceptable_frame_clock_cycles = 1;

static char gui_top_message[GUI_TOP_MESSAGE_MAX];

static b8 profiler_paused = false;

s32 profiler_object_id = INT32_MAX;
s32 profiler_touch_id = -1;

s32 frame_selection_touch_ids[2];

s32 gui_selected_frames[2];

#define PROFILER_Z 0.13f

#define MAX_DESCRIPTION_SIZE 256
#define MAX_CHILDREN_PER_NODE 12
typedef struct ProfiledNode {
    char description[MAX_DESCRIPTION_SIZE];
    u64 elapsed_min;
    u64 elapsed_max;
    u64 elapsed_mostrecent;
    u64 elapsed_total;
    u64 last_start;
    u32 elapsed_count;
    u16 children[MAX_CHILDREN_PER_NODE];
    u8 children_size;
    u8 parents_size;
} ProfiledNode;

#define PROFILE_TREE_MAX 15000

#define FUNCTION_STACK_MAX 10000
static u32 * function_stack = NULL;
static u16 function_stack_size = 0;

static u32 * gui_function_stack = NULL;
static u16 gui_function_stack_size = 0;

#define FRAMES_MAX 10
typedef struct Frame {
    ProfiledNode profiles[PROFILE_TREE_MAX];
    u64 started_at;
    u64 elapsed;
    u8 hit[PROFILE_TREE_MAX];
    u16 profiles_size;
    u8 simd_padding[16];
} Frame;

static Frame * frames = NULL;
static u16 frame_i = 0;

void T1_profiler_init(
    const u64 clock_frequency,
    void * (* profiler_malloc_function)(u64))
{
    acceptable_frame_clock_cycles = clock_frequency / 60;
    
    T1_log_assert(sizeof(Frame) % 32 == 0); // simd padding, 32 bytes for AVX
    
    frames = profiler_malloc_function(
        sizeof(Frame) * FRAMES_MAX);
    T1_log_assert(frames != NULL);
    T1_std_memset(frames, 0, sizeof(Frame) * FRAMES_MAX);
    
    function_stack = profiler_malloc_function(
        sizeof(u32) * FUNCTION_STACK_MAX);
    T1_log_assert(function_stack != NULL);
    T1_std_memset(function_stack, 0, sizeof(u32) * FUNCTION_STACK_MAX);
    
    gui_function_stack = profiler_malloc_function(
        sizeof(u32) * FUNCTION_STACK_MAX);
    T1_std_memset(gui_function_stack, 0, sizeof(u32) * FUNCTION_STACK_MAX);
    
    profiler_object_id = T1_id_next_nonui_id();
    profiler_touch_id = T1_id_next_nonui_touch_id();
    
    gui_selected_frames[0] = 0;
    gui_selected_frames[1] = 1;
    
    T1_std_strcpy_cap(
        gui_top_message,
        GUI_TOP_MESSAGE_MAX,
        "Profiler intialized...");
    
    frame_selection_touch_ids[0] = T1_id_next_nonui_touch_id();
    frame_selection_touch_ids[1] = T1_id_next_nonui_touch_id();
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
    
    frames[frame_i].elapsed = (u64)__rdtsc() - frames[frame_i].started_at;
    if (frames[frame_i].started_at == 0) {
        frames[frame_i].elapsed = SLOW_FRAME_CYCLES - 1;
    } else if (
        frames[frame_i].elapsed > SLOW_FRAME_CYCLES)
    {
        T1_std_strcpy_cap(
            gui_top_message,
            GUI_TOP_MESSAGE_MAX,
            "PAUSED - frame ");
        T1_std_strcat_u32_cap(
            gui_top_message,
            GUI_TOP_MESSAGE_MAX,
            frame_i);
        T1_std_strcat_cap(
            gui_top_message,
            GUI_TOP_MESSAGE_MAX,
            " took ");
        T1_std_strcat_u32_cap(
            gui_top_message,
            GUI_TOP_MESSAGE_MAX,
            (u32)frames[frame_i].elapsed);
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
        u32 parent_prof_i = function_stack[function_stack_size - 1];
        
        T1_log_assert(frames[frame_i].profiles[parent_prof_i].
            children_size < MAX_CHILDREN_PER_NODE);
        
        for (
            s32 i = 0;
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
                u16 new_i = frames[frame_i].profiles[parent_prof_i].
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
            u16 new_i = frames[frame_i].profiles_size;
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
        u16 new_i = frames[frame_i].profiles_size;
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
    #if T1_LOG_ASSERTS_ACTIVE == T1_ACTIVE
    (void)function_name;
    #elif T!_LOG_ASSERTS_ACTIVE == T1_INACTIVE
    #else
    #error
    #endif
    
    if (
        profiler_paused ||
        frames == NULL ||
        function_stack == NULL)
    {
        return;
    }
    
    T1_log_assert(function_stack_size > 0);
    u32 prof_i =
        function_stack[function_stack_size - 1];
    function_stack_size -= 1;
    
    u64 elapsed = __rdtsc() - frames[frame_i].profiles[prof_i].last_start;
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

b8 T1_profiler_handle_lclick(
    const s32 touch_id)
{
    if (
        touch_id == frame_selection_touch_ids[0])
    {
        gui_selected_frames[0] += 1;
        gui_selected_frames[0] %= FRAMES_MAX;
        return true;
    }
    
    if (
        touch_id == frame_selection_touch_ids[1])
    {
        gui_selected_frames[1] += 1;
        gui_selected_frames[1] %= FRAMES_MAX;
        return true;
    }
    
    return false;
}

void T1_profiler_draw_labels(void) {
    T1_zsprite_delete(profiler_object_id);
    
    if (T1_global->show_profiler) {
        T1zSpriteRequest profiler_backdrop;
        T1_zsprite_fetch_next_noconstruct(&profiler_backdrop);
        T1_zsprite_construct_quad_around(
            /* const f32 mid_x: */
                0.0f,
            /* const f32 mid_y: */
                0.0f,
            /* const f32 z: */
                PROFILER_Z,
            /* const f32 width: */
                T1_settings_get_render_width(),
            /* const f32 height: */
                T1_settings_get_render_height(),
            /* PolygonRequest *stack_recipient: */
                &profiler_backdrop);
        profiler_backdrop.cpu_data->T1_id = profiler_object_id;
        profiler_backdrop.gpu_data->
            f32s.no_cam = 1.0f;
        profiler_backdrop.gpu_data->
            f32s.no_light = 1.0f;
        profiler_backdrop.gpu_data->f32s.base_mat_f32.diffuse_rgb[0] = 0.50f;
        profiler_backdrop.gpu_data->f32s.base_mat_f32.diffuse_rgb[1] = 0.50f;
        profiler_backdrop.gpu_data->f32s.base_mat_f32.diffuse_rgb[2] = 0.50f;
        profiler_backdrop.gpu_data->f32s.base_mat_f32.alpha = 0.75f;
        profiler_backdrop.cpu_data->simd.alpha_blending_on = true;
        profiler_backdrop.gpu_data->s32.touch_id = -1;
        T1_zsprite_commit(&profiler_backdrop);
        
        T1_text_props->f32s.rgba[0] = 0.1f;
        T1_text_props->f32s.rgba[1] = 0.1f;
        T1_text_props->f32s.rgba[2] = 0.1f;
        T1_text_props->f32s.rgba[3] = 1.0f;
        T1_text_props->font_height = 18.0f;
        
        T1_text_request_label_renderable(
            /* const s32 with_object_id: */
                profiler_object_id,
            /* const char * text_to_draw: */
                gui_top_message,
            /* const f32 left_pixelspace: */
                20,
            /* const f32 mid_y_pixelspace: */
                T1_settings_get_render_height() - 20,
            /* const f32 z: */
                PROFILER_Z - 0.02f,
            /* const f32 tab_width: */
                4.0f,
            /* const f32 max_width: */
                T1_settings_get_render_width());
        
        for (
            s32 gui_frame_i = 0;
            gui_frame_i < 2;
            gui_frame_i++)
        {
            T1_text_props->f32s.rgba[0] = 0.1f;
            f32 gui_frame_left = 20 +
                ((u32)gui_frame_i * T1_settings_get_render_width() / 2);
            
            s32 f_i = gui_selected_frames[gui_frame_i];
            
            f32 cur_top = T1_settings_get_render_height() -
                20 -
                (T1_text_props->font_height + 2.0f);
            
            char line_text[128];
            T1_std_strcpy_cap(line_text, 128, "Selected frame: ");
            T1_std_strcat_s32_cap(
                line_text,
                128,
                f_i);
            T1_text_props->s32s.touch_id = frame_selection_touch_ids[
                gui_frame_i];
            
            T1_text_request_label_renderable(
                /* const s32 with_T1_id: */
                    profiler_object_id,
                /* const char * text_to_draw: */
                    line_text,
                /* const f32 left_pixelspace: */
                    gui_frame_left,
                /* const f32 top_y_pixelspace: */
                    cur_top,
                /* const f32 z: */
                    PROFILER_Z - 0.02f,
                /* const f32 tab_width: */
                    4.0f,
                /* const f32 max_width: */
                    T1_settings_get_render_width());
            
            T1_text_props->s32s.touch_id = -1;
            
            cur_top -= (T1_text_props->font_height + 2.0f);
            T1_std_strcpy_cap(line_text, 128, "Cycles: ");
            T1_std_strcat_u32_cap(
                line_text,
                128,
                (u32)frames[f_i].elapsed);
            T1_std_strcat_cap(line_text, 128, " (");
            f32 pct_of_acceptable = (f32)frames[f_i].elapsed /
                (f32)acceptable_frame_clock_cycles;
            T1_std_strcat_f32_cap(
                line_text,
                128,
                pct_of_acceptable);
            T1_std_strcat_cap(line_text, 128, ")");
            T1_text_request_label_renderable(
                /* const s32 with_object_id: */
                    profiler_object_id,
                /* const char * text_to_draw: */
                    line_text,
                /* const f32 left_pixelspace: */
                    gui_frame_left,
                /* const f32 mid_y_pixelspace: */
                    cur_top,
                /* const f32 z: */
                    PROFILER_Z - 0.02f,
                /* const f32 tab_width: */
                    4.0f,
                /* const f32 max_width: */
                    T1_settings_get_render_width());
            
            if (frames[f_i].profiles_size > 0) {
                gui_function_stack[0] = 0;
                gui_function_stack_size = 1;
            } else {
                gui_function_stack_size = 0;
            }
            
            while (gui_function_stack_size > 0 && frames[f_i].elapsed > 0) {
                u32 func_i = gui_function_stack[
                    gui_function_stack_size - 1];
                gui_function_stack_size -= 1;
                
                T1_log_assert(frames[f_i].profiles[func_i].elapsed_total > 0);
                
                f32 pct_elapsed =
                    (f32)frames[f_i].profiles[func_i].elapsed_total /
                        (f32)acceptable_frame_clock_cycles;
                T1_text_props->f32s.rgba[0] = 0.1f + (pct_elapsed * 2);
                if (
                    T1_text_props->f32s.rgba[0] > 1.0f)
                {
                    T1_text_props->f32s.rgba[0] = 1.0f;
                }
                
                if (pct_elapsed >= 0.0f) {
                    // push all child nodes onto the stack
                    for (
                         u32 child_i = 0;
                         child_i < frames[f_i].profiles[func_i].children_size;
                         child_i++)
                    {
                        u16 child_prof_i = frames[f_i].profiles[func_i].
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
                T1_std_strcat_u32_cap(
                    line_text,
                    128,
                    (u32)frames[f_i].
                        profiles[func_i].elapsed_total);
                T1_std_strcat_cap(
                    line_text,
                    128,
                    " (");
                
                T1_log_assert(pct_elapsed >= 0.0f);
                T1_std_strcat_s32_cap(
                    line_text,
                    128,
                    (int)(pct_elapsed * 100.0f));
                T1_std_strcat_cap(
                    line_text,
                    128,
                    "%)");
                
                cur_top -= (T1_text_props->font_height + 2.0f);
                T1_text_request_label_renderable(
                    /* const s32 with_object_id: */
                        profiler_object_id,
                    /* const char * text_to_draw: */
                        line_text,
                    /* const f32 left_pixelspace: */
                        gui_frame_left + (
                            frames[f_i].profiles[func_i].parents_size * 30.0f),
                    /* const f32 top_pixelspace: */
                        cur_top,
                    /* const f32 z: */
                        PROFILER_Z - 0.02f,
                    /* const f32 tab_width: */
                        4.0f,
                    /* const f32 max_width: */
                        T1_settings_get_render_width());
            }
        }
    }
}
#elif T1_PROFILER_ACTIVE == T1_INACTIVE
// Pass
#else
#error "T1_PROFILER_ACTIVE undefined!"
#endif // T1_PROFILER_ACTIVE
