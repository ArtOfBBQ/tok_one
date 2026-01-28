#include "T1_gameloop.h"

bool32_t T1_gameloop_active = false;
bool32_t T1_loading_textures = false;

static uint64_t gameloop_previous_time = 0;
static uint64_t gameloop_frame_no = 0;
static int32_t  loading_text_sprite_id = -1;

#if T1_TERMINAL_ACTIVE == T1_ACTIVE
static void update_terminal(void) {
    if (T1_io_keymap[T1_IO_KEY_ENTER] && !T1_io_keymap[T1_IO_KEY_CONTROL]) {
        T1_io_keymap[T1_IO_KEY_ENTER] = false;
        terminal_commit_or_activate();
    }
    
    if (terminal_active) {
        for (uint32_t i = 0; i < T1_IO_KEYMAP_CAP; i++) {
            if (T1_io_keymap[i]) {
                terminal_sendchar(i);
                T1_io_keymap[i] = false;
            }
        }
    }
    
    terminal_render();
}
#elif T1_TERMINAL_ACTIVE == T1_INACTIVE
#else
#error "T1_TERMINAL_ACTIVE undefined"
#endif

void T1_gameloop_init(void) {
}

static void show_dead_simple_text(
    T1GPUFrame * frame_data,
    const char * text_message,
    const uint64_t elapsed)
{
    T1_uielement_delete_all();
    #if T1_PARTICLES_ACTIVE == T1_ACTIVE
    T1_particle_effects_delete_all();
    #elif T1_PARTICLES_ACTIVE == T1_INACTIVE
    // Pass
    #else
    #error "T1_PARTICLES_ACTIVE undefined"
    #endif
    zlights_to_apply_size = 0;
    T1_zsprite_delete_all();
    
    #if T1_FOG_ACTIVE == T1_ACTIVE
    T1_global->postproc_consts.fog_factor = 0.0f;
    #elif T1_FOG_ACTIVE == T1_INACTIVE
    // Pass
    #else
    #error "T1_FOG_ACTIVE undefined"
    #endif
    
    T1_camera->xyz[0] = 0.0f;
    T1_camera->xyz[1] = 0.0f;
    T1_camera->xyz[2] = 0.0f;
    T1_camera->xyz_angle[0] = 0.0f;
    T1_camera->xyz_angle[1] = 0.0f;
    T1_camera->xyz_angle[2] = 0.0f;
    
    font_settings->font_height = 20.0f;
    font_settings->matf32.ambient_rgb[0] = 1.0f;
    font_settings->matf32.ambient_rgb[1] = 1.0f;
    font_settings->matf32.ambient_rgb[2] = 1.0f;
    font_settings->matf32.diffuse_rgb[0] = 1.0f;
    font_settings->matf32.diffuse_rgb[1] = 1.0f;
    font_settings->matf32.diffuse_rgb[2] = 1.0f;
    font_settings->matf32.alpha = 1.0f;
    font_settings->ignore_camera = false;
    font_settings->ignore_lighting = true;
    
    assert(font_settings->mati32.texturearray_i == 0);
    text_request_label_renderable(
        /* const uint32_t with_object_id: */
            0,
        /* const char * text_to_draw: */
            text_message,
        /* const float left_pixelspace: */
            30,
        /* const float mid_y_pixelspace: */
            T1_global->window_height - 30,
        /* const float z: */
            1.0f,
        /* const float max_width: */
            T1_global->window_width - 30);
    
    T1_global->draw_fps = 0;
    T1_global->draw_triangles = true;
    T1_global->draw_axes = 0;
    T1_global->draw_fps = 0;
    T1_global->show_profiler = false;
    T1_global->postproc_consts.
        perlin_texturearray_i = 0;
    T1_global->postproc_consts.
        perlin_texture_i = 0;
    T1_global->postproc_consts.
        color_quantization = 1;
    T1_global->postproc_consts.
        lights_size = 0;
    
    T1_renderer_hardware_render(
            frame_data,
        /* uint64_t elapsed_us: */
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
    T1_global->elapsed = (uint64_t)(
        (double)T1_global->elapsed *
        (double)T1_global->timedelta_mult);
    
    gameloop_previous_time = T1_global->this_frame_timestamp_us;
    
    frame_data->postproc_consts->lights_size = 0;
    frame_data->verts_size = 0;
    frame_data->flat_bb_quads_size = 0;
    frame_data->flat_tex_quads_size = 0;
    frame_data->zsprite_list->size = 0;
    
    if (
        !T1_gameloop_active && T1_loading_textures)
    {
        if (loading_text_sprite_id < 0) {
            loading_text_sprite_id =
                T1_zspriteid_next_ui_element_id();
        }
        
        float pct_progress =
            (float)T1_global->startup_bytes_loaded /
            (float)T1_global->startup_bytes_to_load;
        pct_progress *= 100.0f;
        
        char loading_text[256];
        T1_std_strcpy_cap(
            loading_text,
            256,
            "Loading textures - ");
        T1_std_strcat_float_cap(
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
    
    log_assert(frame_data->lights != NULL);
    log_assert(frame_data->render_views != NULL);
    
    T1_global->this_frame_timestamp_us =
        T1_platform_get_current_time_us();
    
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
    
    if (!T1_app_running) {
        if (crashed_top_of_screen_msg[0] == '\0') {
            T1_std_strcpy_cap(
                crashed_top_of_screen_msg,
                256,
                "Failed assert, and also failed to retrieve an error message");
        }
        
        show_dead_simple_text(
            frame_data,
            crashed_top_of_screen_msg,
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
                T1_WINDOW_RESIZE_TIMEOUT)
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
            log_append("w82RZ - ");
            
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
            log_append("\nOK, resize window\n");
            
            log_assert(
                (int32_t)T1_render_views->size <=
                    T1_RENDER_VIEW_CAP);
            for (
                int32_t rv_i = 0;
                rv_i < (int32_t)T1_render_views->size;
                rv_i++)
            {
                if (
                    !T1_render_views->cpu[rv_i].
                        deleted &&
                    T1_render_views->cpu[rv_i].
                        write_array_i >= 0)
                {
                    T1_texture_array_delete_slice(
                        T1_render_views->cpu[rv_i].
                            write_array_i,
                        T1_render_views->cpu[rv_i].
                            write_slice_i);
                }
                
                T1_render_view_delete(rv_i);
            }
            log_assert(T1_render_views->size == 0);
            
            // TODO:
            // use an internal render size
            // determined by settings, not window
            int32_t rv_i =
                T1_texture_array_create_new_render_view(
                    (uint32_t)T1_global->
                        window_width,
                    (uint32_t)T1_global->
                        window_height);
            log_assert(rv_i == 0);
            log_assert(
                !T1_render_views->cpu[0].deleted);
            log_assert(
                T1_render_views->cpu[0].write_type ==
                    T1RENDERVIEW_WRITE_RENDER_TARGET);
            log_assert(
                !T1_texture_arrays[T1_render_views->cpu[rv_i].write_array_i].images[T1_render_views->cpu[rv_i].write_slice_i].deleted);
            
            T1_platform_gpu_update_window_viewport();
            T1_platform_gpu_update_internal_render_viewport(rv_i);
            
            #if T1_TERMINAL_ACTIVE == T1_ACTIVE
            terminal_redraw_backgrounds();
            #elif T1_TERMINAL_ACTIVE == T1_INACTIVE
            // Pass
            #else
            #error "T1_TERMINAL_ACTIVE undefined"
            #endif
            
            T1_clientlogic_window_resize(
                (uint32_t)T1_global->window_width,
                (uint32_t)T1_global->window_height);
       }
    } else if (
        T1_app_running &&
        T1_global->clientlogic_early_startup_finished)
    {
        #if T1_FRAME_ANIM_ACTIVE == T1_ACTIVE
        T1_frame_anim_new_frame_starts();
        #elif T1_FRAME_ANIM_ACTIVE == T1_INACTIVE
        #else
        #error
        #endif
        
        #if T1_LOGGER_ASSERTS_ACTIVE == T1_ACTIVE
        T1_render_view_validate();
        #elif T1_LOGGER_ASSERTS_ACTIVE == T1_INACTIVE
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
        T1_io_events[T1_IO_LAST_MOUSE_MOVE] =
            T1_io_events[T1_IO_LAST_MOUSE_OR_TOUCH_MOVE];
        T1_io_events[T1_IO_LAST_TOUCH_MOVE] =
            T1_io_events[T1_IO_LAST_MOUSE_OR_TOUCH_MOVE];
        
        T1_uielement_handle_touches(
            T1_global->elapsed);
        
        #if T1_TERMINAL_ACTIVE == T1_ACTIVE
        update_terminal();
        #elif T1_TERMINAL_ACTIVE == T1_INACTIVE
        // Pass
        #else
        #error "T1_TERMINAL_ACTIVE undefined"
        #endif
        
        #if T1_PROFILER_ACTIVE == T1_ACTIVE
        T1_profiler_start(
            "T1_clientlogic_update()");
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
            &T1_global->postproc_consts.
                lights_size);
        
        T1_render_view_update_positions();
        
        #if T1_PROFILER_ACTIVE == T1_ACTIVE
        T1_profiler_start(
            "T1_renderer_hardware_render()");
        #elif T1_PROFILER_ACTIVE == T1_INACTIVE
        #else
        #error "T1_PROFILER_ACTIVE undefined"
        #endif
        T1_renderer_hardware_render(
            frame_data,
            T1_global->elapsed);
        #if T1_PROFILER_ACTIVE == T1_ACTIVE
        T1_profiler_end(
            "T1_renderer_hardware_render()");
        #elif T1_PROFILER_ACTIVE == T1_INACTIVE
        #else
        #error "T1_PROFILER_ACTIVE undefined"
        #endif
        
        uint32_t overflow_vertices = frame_data->verts_size % 3;
        frame_data->verts_size -= overflow_vertices;
    }
    
    if (T1_global->draw_fps) {
        text_request_fps_counter(
            /* uint64_t elapsed_us: */
                T1_global->elapsed);
    } else if (T1_global->draw_top_touchable_id) {
        text_request_top_touchable_id(
            T1_io_events[T1_IO_LAST_MOUSE_OR_TOUCH_MOVE].
                touch_id_top);
    }
    
    frame_data->postproc_consts->timestamp =
        (uint32_t)T1_global->this_frame_timestamp_us;
    
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
    
    if (T1_app_running) {
        T1_clientlogic_update_after_render_pass();
    }
    
    T1_io_events[T1_IO_LAST_GPU_DATA].touch_id_top =
        T1_platform_gpu_get_touch_id_at_screen_pos(
            /* const int screen_x: */
                T1_io_events[T1_IO_LAST_GPU_DATA].
                    screen_x,
            /* const int screen_y: */
                T1_io_events
                    [T1_IO_LAST_GPU_DATA].
                        screen_y);
    
    T1_io_events[T1_IO_LAST_GPU_DATA].
        touch_id_pierce =
            T1_io_events[T1_IO_LAST_GPU_DATA].
                touch_id_top;
    
    if (T1_global->upcoming_fullscreen_request) {
        T1_global->upcoming_fullscreen_request = false;
        T1_platform_enter_fullscreen();
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
