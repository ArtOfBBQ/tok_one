#include "T1_gameloop.h"

#include "T1_term.h"
#include "T1_log.h"
#include "T1_settings.h"
#include "T1_global.h"
#include "T1_io.h"
#include "T1_id.h"
#include "T1_texquad.h"
#include "T1_platform_layer.h"
#include "T1_tex_array.h"
#include "T1_text.h"
#include "T1_ui_widget.h"
#include "T1_render.h"
#include "T1_zlight.h"
#include "T1_zsprite_anim.h"
#include "T1_texquad_anim.h"
#include "T1_clientlogic.h"
#include "T1_profiler.h"
#include "T1_particle.h"
#include "T1_frame_anim.h"
#include "T1_render_view.h"

u8 T1_gameloop_active = false;
u8 T1_gameloop_loading_texs = false;

static u64 gameloop_previous_time = 0;
static u64 gameloop_frame_no = 0;
static s32  loading_text_sprite_id = -1;

#if T1_TERM_ACTIVE == T1_ACTIVE
static void update_terminal(void) {
    if (
        T1_io->keymap[T1_IO_KEY_ENTER] &&
        !T1_io->keymap[T1_IO_KEY_CONTROL])
    {
        T1_io->keymap[T1_IO_KEY_ENTER] = false;
        T1_term_commit_or_activate();
    }
    
    if (T1_term_active) {
        for (u32 i = 0; i < T1_IO_KEYMAP_CAP; i++) {
            if (i == T1_IO_KEY_SHIFT) { continue; }
            if (T1_io->keymap[i]) {
                if (T1_io->keymap[T1_IO_KEY_SHIFT]) {
                    T1_term_sendchar('#');
                } else {
                    T1_term_sendchar(i);
                }
                
                T1_io->keymap[i] = false;
            }
        }
    }
    
    T1_term_render();
}
#elif T1_TERM_ACTIVE == T1_INACTIVE
#else
#error "T1_TERM_ACTIVE undefined"
#endif

void T1_gameloop_init(void) {
}

static void show_dead_simple_text(
    T1GPUFrame * frame_data,
    const char * text_message,
    const u64 elapsed)
{
    T1_ui_widget_delete_all();
    #if T1_PARTICLES_ACTIVE == T1_ACTIVE
    T1_particle_effects_delete_all();
    #elif T1_PARTICLES_ACTIVE == T1_INACTIVE
    // Pass
    #else
    #error "T1_PARTICLES_ACTIVE undefined"
    #endif
    T1_zlights_size = 0;
    T1_zsprite_delete_all();
    
    #if T1_FOG_ACTIVE == T1_ACTIVE
    T1_global->postproc_consts.fog_factor = 0.0f;
    #elif T1_FOG_ACTIVE == T1_INACTIVE
    // Pass
    #else
    #error "T1_FOG_ACTIVE undefined"
    #endif
    
    T1_render_view_reset(0);
    
    T1_text_props->font_height = 20.0f;
    T1_text_props->f32s.rgba[0] = 1.0f;
    T1_text_props->f32s.rgba[1] = 1.0f;
    T1_text_props->f32s.rgba[2] = 1.0f;
    T1_text_props->f32s.rgba[3] = 1.0f;
    
    T1_log_assert((T1_text_props->s32s.reserved_and_tex & 0x0000FFFF) == T1_TEX_NONE);
    
    T1_text_request_label_renderable(
        /* const u32 with_object_id: */
            0,
        /* const char * text_to_draw: */
            text_message,
        /* const f32 left_pixelspace: */
            30,
        /* const f32 mid_y_pixelspace: */
            T1_settings_get_render_height() - 30,
        /* const f32 z: */
            1.0f,
        /* const f32 tab_width: */
            4.0f,
        /* const f32 max_width: */
            T1_settings_get_render_width() - 30);
    
    T1_global->draw_fps = 0;
    T1_global->draw_triangles = true;
    T1_global->draw_axes = 0;
    T1_global->draw_fps = 0;
    T1_global->show_profiler = false;
    T1_global->postproc_consts.perlin_texturearray_i = 0;
    T1_global->postproc_consts.perlin_texture_i = 0;
    T1_global->postproc_consts.color_quantization = 1;
    T1_global->postproc_consts.lights_size = 0;
    
    T1_render_update(
            frame_data,
        /* u64 elapsed_us: */
            elapsed);
}

void T1_gameloop_update_before_render_pass(
    T1GPUFrame * frame_data)
{
    #if T1_PROFILER_ACTIVE == T1_ACTIVE
    T1_profiler_start("T1_gameloop_update_before_render_pass()");
    #elif T1_PROFILER_ACTIVE == T1_INACTIVE
    #else
    #error "T1_PROFILER_ACTIVE undefined"
    #endif
    
    T1_global->elapsed =
        T1_global->this_frame_timestamp_us -
            gameloop_previous_time;
    
    // TODO: set frame timestamp to an adj value
    T1_global->elapsed = (u64)(
        (f64)T1_global->elapsed *
        (f64)T1_global->timedelta_mult);
    
    gameloop_previous_time = T1_global->this_frame_timestamp_us;
    
    frame_data->postproc_consts->lights_size = 0;
    frame_data->verts_size = 0;
    frame_data->flat_bb_quads_size = 0;
    frame_data->flat_tex_quads_size = 0;
    frame_data->zsprite_list->size = 0;
    
    if (
        !T1_gameloop_active && T1_gameloop_loading_texs)
    {
        if (loading_text_sprite_id < 0) {
            loading_text_sprite_id =
                T1_id_next_ui_element_id();
        }
        
        f32 pct_progress =
            (f32)T1_global->startup_bytes_loaded /
            (f32)T1_global->startup_bytes_to_load;
        pct_progress *= 100.0f;
        
        char loading_text[256];
        T1_std_strcpy_cap(
            loading_text,
            256,
            "Loading textures - ");
        T1_std_strcat_f32_cap(
            loading_text,
            256,
            pct_progress);
        T1_std_strcat_cap(
            loading_text,
            256,
            "%");
        show_dead_simple_text(frame_data, loading_text, 1);
        
        #if T1_PROFILER_ACTIVE == T1_ACTIVE
        T1_profiler_end("T1_gameloop_update_before_render_pass()");
        #elif T1_PROFILER_ACTIVE == T1_INACTIVE
        #else
        #error "T1_PROFILER_ACTIVE undefined"
        #endif
        return;
    }
    
    if (!T1_gameloop_active) {
        #if T1_PROFILER_ACTIVE == T1_ACTIVE
        T1_profiler_end(
            "T1_gameloop_update_before_render_pass()");
        #elif T1_PROFILER_ACTIVE == T1_INACTIVE
        #else
        #error "T1_PROFILER_ACTIVE undefined"
        #endif
        return;
    }
    
    T1_log_assert(frame_data->lights != NULL);
    T1_log_assert(frame_data->render_views != NULL);
    
    T1_global->this_frame_timestamp_us =
        T1_os_get_current_time_us();
    
    if (gameloop_previous_time < 1) {
        gameloop_previous_time = T1_global->this_frame_timestamp_us;
        
        #if T1_PROFILER_ACTIVE == T1_ACTIVE
        T1_profiler_end("T1_gameloop_update_before_render_pass()");
        #elif T1_PROFILER_ACTIVE == T1_INACTIVE
        #else
        #error "T1_PROFILER_ACTIVE undefined"
        #endif
        return;
    }
    
    if (!T1_log_app_running) {
        show_dead_simple_text(
            frame_data,
            T1_log_crash_msg,
            T1_global->elapsed);
        
        #if T1_PROFILER_ACTIVE == T1_ACTIVE
        T1_profiler_end("T1_gameloop_update_before_render_pass()");
        #elif T1_PROFILER_ACTIVE == T1_INACTIVE
        #else
        #error "T1_PROFILER_ACTIVE undefined"
        #endif
        return;
    }
    
    gameloop_frame_no++;
    
    if (
        T1_global->this_frame_timestamp_us -
            T1_global->last_resize_request_us <
                T1_GLOBAL_WINDOW_RESIZE_TIMEOUT)
    {
        if (
            T1_global->
                this_frame_timestamp_us -
            T1_global->
                last_resize_request_us < 200000)
        {
            // possibly a request we already handled, or not the final
            // request, wait...
            // we break, not return, because we do want to render an
            // empty screen
            T1_log_append("w82RZ - ");
            
            #if T1_PROFILER_ACTIVE == T1_ACTIVE
            T1_profiler_end(
                "T1_gameloop_update_b4_render_pass()");
            #elif T1_PROFILER_ACTIVE == T1_INACTIVE
            #else
            #error "T1_PROFILER_ACTIVE undefined"
            #endif
            return;
        } else {
            T1_global_init();
            
            T1_global->last_resize_request_us = 0;
            T1_log_append("\nOK, resize window\n");
            
            T1_clientlogic_window_resize();
       }
    } else if (
        T1_log_app_running &&
        T1_global->clientlogic_early_startup_finished)
    {
        #if T1_FRAME_ANIM_ACTIVE == T1_ACTIVE
        T1_frame_anim_new_frame_starts();
        #elif T1_FRAME_ANIM_ACTIVE == T1_INACTIVE
        #else
        #error
        #endif
        
        #if T1_LOG_ASSERTS_ACTIVE == T1_ACTIVE
        T1_render_view_validate();
        #elif T1_LOG_ASSERTS_ACTIVE == T1_INACTIVE
        #else
        #error
        #endif
        
        #if T1_ZSPRITE_ANIM_ACTIVE == T1_ACTIVE
        T1_zsprite_anim_resolve();
        #elif T1_ZSPRITE_ANIM_ACTIVE == T1_INACTIVE
        #else
        #error
        #endif
        
        #if T1_TEXQUAD_ANIM_ACTIVE == T1_ACTIVE
        T1_texquad_anim_resolve();
        #elif T1_TEXQUAD_ANIM_ACTIVE == T1_INACTIVE
        #else
        #error
        #endif
        
        T1_platform_update_mouse_location();
        
        T1_zsprite_handle_timed_occlusion();
        
        // always copy
        T1_io->events[T1_IO_LAST_MOUSE_MOVE] =
            T1_io->events[T1_IO_LAST_MOUSE_OR_TOUCH_MOVE];
        T1_io->events[T1_IO_LAST_TOUCH_MOVE] =
            T1_io->events[T1_IO_LAST_MOUSE_OR_TOUCH_MOVE];
        
        T1_ui_widget_handle_touches(T1_global->elapsed);
        
        #if T1_TERM_ACTIVE == T1_ACTIVE
        update_terminal();
        #elif T1_TERM_ACTIVE == T1_INACTIVE
        // Pass
        #else
        #error "T1_TERM_ACTIVE undefined"
        #endif
        
        #if T1_PROFILER_ACTIVE == T1_ACTIVE
        T1_profiler_start("T1_clientlogic_update()");
        #elif T1_PROFILER_ACTIVE == T1_INACTIVE
        #else
        #error "T1_PROFILER_ACTIVE undefined"
        #endif
        T1_clientlogic_update(T1_global->elapsed);
        #if T1_PROFILER_ACTIVE == T1_ACTIVE
        T1_profiler_end("T1_clientlogic_update()");
        #elif T1_PROFILER_ACTIVE == T1_INACTIVE
        #else
        #error "T1_PROFILER_ACTIVE undefined"
        #endif
        
        T1_zlight_clean_all_deleted();
        
        T1_zlight_copy_all(
            frame_data->lights,
            &T1_global->postproc_consts.lights_size);
        
        T1_render_view_update_positions(T1_global->elapsed);
        
        #if T1_PROFILER_ACTIVE == T1_ACTIVE
        T1_profiler_start(
            "T1_renderer_hardware_render()");
        #elif T1_PROFILER_ACTIVE == T1_INACTIVE
        #else
        #error "T1_PROFILER_ACTIVE undefined"
        #endif
        T1_render_update(
            frame_data,
            T1_global->elapsed);
        #if T1_PROFILER_ACTIVE == T1_ACTIVE
        T1_profiler_end(
            "T1_renderer_hardware_render()");
        #elif T1_PROFILER_ACTIVE == T1_INACTIVE
        #else
        #error "T1_PROFILER_ACTIVE undefined"
        #endif
        
        T1_texquad_defragment();
        T1_zsprite_defragment();
        
        u32 overflow_vertices = frame_data->verts_size % 3;
        frame_data->verts_size -= overflow_vertices;
    }
    
    if (T1_global->draw_fps) {
        T1_text_request_fps(
            /* u64 elapsed_us: */
                T1_global->elapsed);
    } else if (T1_global->draw_top_touchable_id) {
        T1_text_request_top_touch_id(
            T1_io->events[T1_IO_LAST_MOUSE_OR_TOUCH_MOVE].
                touch_id_top);
    }
    
    frame_data->postproc_consts->timestamp =
        (u32)T1_global->this_frame_timestamp_us;
    
    #if T1_PROFILER_ACTIVE == T1_ACTIVE
    T1_profiler_end(
        "T1_gameloop_update_before_render_pass()");
    T1_profiler_draw_labels();
    #elif T1_PROFILER_ACTIVE == T1_INACTIVE
    #else
    #error "T1_PROFILER_ACTIVE undefined"
    #endif
}

void T1_gameloop_update_after_render_pass(void) {
    #if T1_PROFILER_ACTIVE == T1_ACTIVE
    T1_profiler_start("T1_gameloop_update_after_render_pass()");
    #elif T1_PROFILER_ACTIVE == T1_INACTIVE
    #else
    #error "T1_PROFILER_ACTIVE undefined"
    #endif
    
    if (T1_log_app_running) {
        T1_clientlogic_update_after_render_pass();
    }
    
    T1_io->events[T1_IO_LAST_GPU_DATA].touch_id_top =
        T1_os_gpu_get_touch_id_at_screen_pos(
            /* const s32 screen_x: */
                T1_io->events[T1_IO_LAST_GPU_DATA].
                    screen_x,
            /* const s32 screen_y: */
                T1_io->events[T1_IO_LAST_GPU_DATA].
                    screen_y);
    
    T1_io->events[T1_IO_LAST_GPU_DATA].
        touch_id_pierce =
            T1_io->events[T1_IO_LAST_GPU_DATA].
                touch_id_top;
    
    if (T1_global->upcoming_fullscreen_request) {
        T1_global->upcoming_fullscreen_request = false;
        T1_os_enter_fullscreen();
    }
    
    #if T1_PROFILER_ACTIVE == T1_ACTIVE
    T1_profiler_end("T1_gameloop_update_after_render_pass()");
    #elif T1_PROFILER_ACTIVE == T1_INACTIVE
    #else
    #error "T1_PROFILER_ACTIVE undefined"
    #endif
    
    #if T1_PROFILER_ACTIVE == T1_ACTIVE
    T1_profiler_new_frame();
    #elif T1_PROFILER_ACTIVE == T1_INACTIVE
    #else
    #error
    #endif
}
