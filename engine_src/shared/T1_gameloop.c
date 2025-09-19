#include "T1_gameloop.h"

bool32_t gameloop_active = false;
bool32_t loading_textures = false;

static uint64_t gameloop_previous_time = 0;
static uint64_t gameloop_frame_no = 0;
static int32_t  loading_text_sprite_id = -1;

#if T1_TERMINAL_ACTIVE == T1_ACTIVE
static void update_terminal(void) {
    if (keypress_map[TOK_KEY_ENTER] && !keypress_map[TOK_KEY_CONTROL]) {
        keypress_map[TOK_KEY_ENTER] = false;
        terminal_commit_or_activate();
    }
    
    if (terminal_active) {
        for (uint32_t i = 0; i < KEYPRESS_MAP_SIZE; i++) {
            if (keypress_map[i]) {
                terminal_sendchar(i);
                keypress_map[i] = false;
            }
        }
    }
    
    terminal_render();
}
#elif T1_TERMINAL_ACTIVE == T1_INACTIVE
#else
#error "T1_TERMINAL_ACTIVE undefined"
#endif

void gameloop_init(void) {
}

static void show_dead_simple_text(
    T1GPUFrame * frame_data,
    const char * text_message,
    const uint64_t elapsed)
{
    T1_uielement_delete_all();
    #if T1_PARTICLES_ACTIVE == T1_ACTIVE
    T1_particle_effects_size = 0;
    T1_particle_lineparticle_effects_size = 0;
    #elif T1_PARTICLES_ACTIVE == T1_INACTIVE
    // Pass
    #else
    #error "T1_PARTICLES_ACTIVE undefined"
    #endif
    zlights_to_apply_size = 0;
    T1_zsprites_to_render->size = 0;
    
    #if T1_FOG_ACTIVE == T1_ACTIVE
    engine_globals->postproc_consts.fog_factor = 0.0f;
    #elif T1_FOG_ACTIVE == T1_INACTIVE
    // Pass
    #else
    #error "T1_FOG_ACTIVE undefined"
    #endif
    
    camera.xyz[0] = 0.0f;
    camera.xyz[1] = 0.0f;
    camera.xyz[2] = 0.0f;
    camera.xyz_angle[0] = 0.0f;
    camera.xyz_angle[1] = 0.0f;
    camera.xyz_angle[2] = 0.0f;
    camera.xyz_cosangle[0] = 0.0f;
    camera.xyz_cosangle[1] = 0.0f;
    camera.xyz_cosangle[2] = 0.0f;
    camera.xyz_sinangle[0] = 0.0f;
    camera.xyz_sinangle[1] = 0.0f;
    camera.xyz_sinangle[2] = 0.0f;
    
    font_settings->font_height          = 20.0f;
    font_settings->mat.ambient_rgb[0]   = 1.0f;
    font_settings->mat.ambient_rgb[1]   = 1.0f;
    font_settings->mat.ambient_rgb[2]   = 1.0f;
    font_settings->mat.diffuse_rgb[0]   = 1.0f;
    font_settings->mat.diffuse_rgb[1]   = 1.0f;
    font_settings->mat.diffuse_rgb[2]   = 1.0f;
    font_settings->mat.alpha            = 1.0f;
    font_settings->ignore_camera        = false; // we set camera to 0,0,0
    font_settings->ignore_lighting      = true;
    
    assert(font_settings->mat.texturearray_i == 0);
    text_request_label_renderable(
        /* const uint32_t with_object_id: */
            0,
        /* const char * text_to_draw: */
            text_message,
        /* const float left_pixelspace: */
            30,
        /* const float top_pixelspace: */
            engine_globals->window_height - 30,
        /* const float z: */
            1.0f,
        /* const float max_width: */
            engine_globals->window_width - 30);
    
    renderer_hardware_render(
            frame_data,
        /* uint64_t elapsed_us: */
            elapsed);
}

void gameloop_update_before_render_pass(
    T1GPUFrame * frame_data)
{
    engine_globals->elapsed = engine_globals->this_frame_timestamp_us -
        gameloop_previous_time;
    
    // TODO: set the frame timestamp to an adjusted value also
    engine_globals->elapsed = (uint64_t)(
        (double)engine_globals->elapsed *
        (double)engine_globals->timedelta_mult);
    
    gameloop_previous_time = engine_globals->this_frame_timestamp_us;
    
    T1_std_memcpy(frame_data->camera, &camera, sizeof(T1GPUCamera));
    
    frame_data->verts_size            = 0;
    frame_data->zsprite_list->size = 0;
    frame_data->first_alphablend_i       = 0;
    frame_data->line_vertices_size       = 0;
    frame_data->point_vertices_size      = 0;
    
    if (!gameloop_active && loading_textures) {
        if (loading_text_sprite_id < 0) {
            loading_text_sprite_id = next_ui_element_object_id();
        }
        
        float pct_progress =
            (float)engine_globals->startup_bytes_loaded /
            (float)engine_globals->startup_bytes_to_load;
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
        
        return;
    }
    
    if (!gameloop_active) {
        return;
    }
    
    #if T1_PROFILER_ACTIVE == T1_ACTIVE
    profiler_start("gameloop_update()");
    #elif T1_PROFILER_ACTIVE == T1_INACTIVE
    #else
    #error "T1_PROFILER_ACTIVE undefined"
    #endif
    
    log_assert(frame_data->lights != NULL);
    log_assert(frame_data->camera != NULL);
    
    engine_globals->this_frame_timestamp_us =
        T1_platform_get_current_time_us();
    
    if (gameloop_previous_time < 1) {
        gameloop_previous_time = engine_globals->this_frame_timestamp_us;
        
        #if T1_PROFILER_ACTIVE == T1_ACTIVE
        profiler_end("gameloop_update()");
        #elif T1_PROFILER_ACTIVE == T1_INACTIVE
        // Pass
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
            engine_globals->elapsed);
        return;
    }
    
    gameloop_frame_no++;
    
    if (
        engine_globals->this_frame_timestamp_us -
            engine_globals->last_resize_request_us < 2000000)
    {
        if (
            engine_globals->this_frame_timestamp_us -
                engine_globals->last_resize_request_us < 350000)
        {
            // possibly a request we already handled, or not the final
            // request, wait...
            // we break, not return, because we do want to render an
            // empty screen
            log_append("w82RZ - ");
            
            #if T1_PROFILER_ACTIVE == T1_ACTIVE
            profiler_end("gameloop_update()");
            #elif T1_PROFILER_ACTIVE == T1_INACTIVE
            // Pass
            #else
            #error "T1_PROFILER_ACTIVE undefined"
            #endif
            return;
        } else {
            
            engine_globals->last_resize_request_us = 0;
            log_append("\nOK, resize window\n");
            
            engineglobals_init();
            
            T1_platform_gpu_update_viewport();
            
            #if T1_TERMINAL_ACTIVE == T1_ACTIVE
            terminal_redraw_backgrounds();
            #elif T1_TERMINAL_ACTIVE == T1_INACTIVE
            // Pass
            #else
            #error "T1_TERMINAL_ACTIVE undefined"
            #endif
            
            T1_clientlogic_window_resize(
                (uint32_t)engine_globals->window_height,
                (uint32_t)engine_globals->window_width);
       }
    } else if (T1_app_running) {
        
        #if T1_SCHEDULED_ANIMS_ACTIVE == T1_ACTIVE
        T1_scheduled_animations_resolve();
        #elif T1_SCHEDULED_ANIMS_ACTIVE == T1_INACTIVE
        #else
        #error "T1_SCHEDULED_ANIMS_ACTIVE undefined"
        #endif
        
        T1_platform_update_mouse_location();
        
        // handle timed occlusions
        for (uint32_t zs_i = 0; zs_i < T1_zsprites_to_render->size; zs_i++) {
            if (
                T1_zsprites_to_render->cpu_data[zs_i].next_occlusion_in_us >
                    engine_globals->elapsed)
            {
                T1_zsprites_to_render->cpu_data[zs_i].next_occlusion_in_us -=
                    engine_globals->elapsed;
            } else if (
                T1_zsprites_to_render->cpu_data[zs_i].next_occlusion_in_us > 0)
            {
                T1_zsprites_to_render->cpu_data[zs_i].next_occlusion_in_us = 0;
                T1_zsprites_to_render->cpu_data[zs_i].visible = 0;
            }
        }
        
        // always copy
        user_interactions[INTR_PREVIOUS_MOUSE_MOVE] =
            user_interactions[INTR_PREVIOUS_MOUSE_OR_TOUCH_MOVE];
        user_interactions[INTR_PREVIOUS_TOUCH_MOVE] =
            user_interactions[INTR_PREVIOUS_MOUSE_OR_TOUCH_MOVE];
        
        T1_uielement_handle_touches(engine_globals->elapsed);
        
        #if T1_TERMINAL_ACTIVE == T1_ACTIVE
        update_terminal();
        #elif T1_TERMINAL_ACTIVE == T1_INACTIVE
        // Pass
        #else
        #error "T1_TERMINAL_ACTIVE undefined"
        #endif
        
        T1_clientlogic_update(engine_globals->elapsed);
        
        camera.xyz_cosangle[0] = cosf(camera.xyz_angle[0]);
        camera.xyz_cosangle[1] = cosf(camera.xyz_angle[1]);
        camera.xyz_cosangle[2] = cosf(camera.xyz_angle[2]);
        camera.xyz_sinangle[0] = sinf(camera.xyz_angle[0]);
        camera.xyz_sinangle[1] = sinf(camera.xyz_angle[1]);
        camera.xyz_sinangle[2] = sinf(camera.xyz_angle[2]);
        
        clean_deleted_lights();
        
        copy_lights(
            frame_data->lights,
            &engine_globals->postproc_consts.lights_size,
            &engine_globals->postproc_consts.shadowcaster_i);
        
        renderer_hardware_render(
                frame_data,
            /* uint64_t elapsed_us: */
                engine_globals->elapsed);
        
        uint32_t overflow_vertices = frame_data->verts_size % 3;
        frame_data->verts_size -= overflow_vertices;
    }
    
    if (engine_globals->draw_fps) {
        text_request_fps_counter(
            /* uint64_t elapsed_us: */
                engine_globals->elapsed);
    } else if (engine_globals->draw_top_touchable_id) {
        text_request_top_touchable_id(
            user_interactions[INTR_PREVIOUS_MOUSE_OR_TOUCH_MOVE].
                touchable_id_top);
    }
    
    frame_data->postproc_consts->timestamp =
        (uint32_t)engine_globals->this_frame_timestamp_us;
    frame_data->postproc_consts->shadowcaster_i =
        shadowcaster_light_i;
    
    #if T1_PROFILER_ACTIVE == T1_ACTIVE
    profiler_end("gameloop_update()");
    profiler_draw_labels();
    #elif T1_PROFILER_ACTIVE == T1_INACTIVE
    #else
    #error "T1_PROFILER_ACTIVE undefined"
    #endif
}

void gameloop_update_after_render_pass(void) {
    if (T1_app_running) {
        T1_clientlogic_update_after_render_pass();
    }
    
    user_interactions
        [INTR_LAST_GPU_DATA].touchable_id_top =
        T1_platform_gpu_get_touchable_id_at_screen_pos(
            /* const int screen_x: */
                user_interactions[INTR_LAST_GPU_DATA].
                    screen_x,
            /* const int screen_y: */
                user_interactions
                    [INTR_LAST_GPU_DATA].
                        screen_y);
    user_interactions[INTR_LAST_GPU_DATA].touchable_id_pierce =
        user_interactions[INTR_LAST_GPU_DATA].touchable_id_top;
    
    if (engine_globals->upcoming_fullscreen_request) {
        engine_globals->upcoming_fullscreen_request = false;
        T1_platform_enter_fullscreen();
    }
}
