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
#include "T1_zsprite.h"
#include "T1_zlight.h"
#include "T1_anim.h"
#include "T1_client.h"
#include "T1_profiler.h"
#include "T1_particle.h"
#include "T1_frame_anim.h"
#include "T1_render_view.h"

u8 T1_gameloop_active = false;
u8 T1_gameloop_loading_texs = false;

static u64 gameloop_previous_time = 0;
static u64 gameloop_frame_no = 0;
static u32 loading_text_T1_id = T1_ID_NONE;

#if T1_TERM_ACTIVE == T1_ACTIVE
static void update_terminal(void) {
    T1_term_update();
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
    
    T1_log_assert((T1_text_props->u32s.reserved_and_tex & 0x0000FFFF) == T1_TEX_NONE);
    
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
    T1GPUFrame * f)
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
    
    f->postproc_consts->lights_size = 0;
    f->verts_size = 0;
    f->flat_bb_quads_size = 0;
    f->flat_tex_quads_size = 0;
    f->zsprite_list->size = 0;
    
    if (
        !T1_gameloop_active && T1_gameloop_loading_texs)
    {
        if (loading_text_T1_id < 0) {
            loading_text_T1_id =
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
        show_dead_simple_text(f, loading_text, 1);
        
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
    
    T1_log_assert(f->lights != NULL);
    T1_log_assert(f->render_views != NULL);
    
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
            f,
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
            T1_global->this_frame_timestamp_us -
            T1_global->last_resize_request_us < 200000)
        {
            // possibly a request we already handled, or not the final
            // request, wait...
            // we break, not return, because we do want to render an
            // empty screen
            T1_log_append("w82RZ - ");
            
            #if T1_PROFILER_ACTIVE == T1_ACTIVE
            T1_profiler_end("T1_gameloop_update_b4_render_pass()");
            #elif T1_PROFILER_ACTIVE == T1_INACTIVE
            #else
            #error "T1_PROFILER_ACTIVE undefined"
            #endif
            return;
        } else {
            T1_global_init();
            
            T1_global->last_resize_request_us = 0;
            T1_log_append("\nOK, resize window\n");
            
            T1_client_window_resize();
       }
    } else if (
        T1_log_app_running &&
        T1_global->clientlogic_early_startup_finished &&
        !T1_gameloop_loading_texs)
    {
        #if T1_LOG_ASSERTS_ACTIVE == T1_ACTIVE
        T1_render_view_validate();
        #elif T1_LOG_ASSERTS_ACTIVE == T1_INACTIVE
        #else
        #error
        #endif
        
        #if T1_ANIM_ACTIVE == T1_ACTIVE
        T1_anim_resolve();
        #elif T1_ANIM_ACTIVE == T1_INACTIVE
        #else
        #error
        #endif
        
        T1_zsprite_handle_timed_occlusion();
        
        #if 0
        T1_ui_widget_handle_touches(T1_global->elapsed);
        #endif
        
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
        T1_client_update(T1_global->elapsed);
        #if T1_PROFILER_ACTIVE == T1_ACTIVE
        T1_profiler_end("T1_clientlogic_update()");
        #elif T1_PROFILER_ACTIVE == T1_INACTIVE
        #else
        #error "T1_PROFILER_ACTIVE undefined"
        #endif
        
        T1_zlight_clean_all_deleted();
        
        T1_zlight_copy_all(
            f->lights,
            &T1_global->postproc_consts.lights_size);
        
        T1_render_view_update_positions(
            T1_global->elapsed);
        
        #if T1_PROFILER_ACTIVE == T1_ACTIVE
        T1_profiler_start(
            "T1_renderer_hardware_render()");
        #elif T1_PROFILER_ACTIVE == T1_INACTIVE
        #else
        #error "T1_PROFILER_ACTIVE undefined"
        #endif
        T1_render_update(f, T1_global->elapsed);
        #if T1_PROFILER_ACTIVE == T1_ACTIVE
        T1_profiler_end(
            "T1_renderer_hardware_render()");
        #elif T1_PROFILER_ACTIVE == T1_INACTIVE
        #else
        #error "T1_PROFILER_ACTIVE undefined"
        #endif
        
        T1_texquad_defragment();
        T1_zsprite_defragment();
        
        u32 overflow_vertices = f->verts_size % 3;
        f->verts_size -= overflow_vertices;
    }
    
    if (T1_global->draw_fps) {
        T1_text_request_fps(T1_global->elapsed);
    } else if (T1_global->draw_touch_id) {
        T1_text_request_top_touch_id(
            T1_io_get_mouse_touch_id_this_frame());
    } else if (T1_global->draw_scene_id) {
        T1_log_assert(0);
    }
    
    f->postproc_consts->timestamp = (u32)T1_global->
        this_frame_timestamp_us;
    
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
        T1_client_update_after_render_pass();
    }
    
    T1_io_update_and_clear_for_next_frame();
    T1_os_poll_gamepad_events();
    
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
