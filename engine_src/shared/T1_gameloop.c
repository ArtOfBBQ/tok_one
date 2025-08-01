#include "T1_gameloop.h"

bool32_t gameloop_active = false;
bool32_t loading_textures = false;

static uint64_t gameloop_previous_time = 0;
static uint64_t gameloop_frame_no = 0;
static int32_t  loading_text_sprite_id = -1;

#if TERMINAL_ACTIVE
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
#endif

void gameloop_init(void) {
}

static void show_dead_simple_text(
    GPUDataForSingleFrame * frame_data,
    const char * text_message,
    const uint64_t elapsed)
{
    delete_all_ui_elements();
    #if PARTICLES_ACTIVE
    particle_effects_size = 0;
    lineparticle_effects_size = 0;
    #endif
    zlights_to_apply_size = 0;
    zsprites_to_render->size = 0;
    
    #if FOG_ACTIVE
    engine_globals->postprocessing_constants.fog_factor = 0.0f;
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
    GPUDataForSingleFrame * frame_data)
{
    uint64_t elapsed = engine_globals->this_frame_timestamp_us -
    gameloop_previous_time;
    gameloop_previous_time = engine_globals->this_frame_timestamp_us;
    
    common_memcpy(frame_data->camera, &camera, sizeof(GPUCamera));
    
    frame_data->vertices_size            = 0;
    frame_data->polygon_collection->size = 0;
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
        common_strcpy_capped(loading_text, 256, "Loading textures - ");
        common_strcat_float_capped(
            loading_text,
            256,
            pct_progress);
        common_strcat_capped(
            loading_text,
            256,
            "%");
        show_dead_simple_text(frame_data, loading_text, 1);
        
        return;
    }
    
    if (!gameloop_active) {
        return;
    }
    
    #if PROFILER_ACTIVE
    profiler_start("gameloop_update()");
    #endif
    
    log_assert(frame_data->lights != NULL);
    log_assert(frame_data->camera != NULL);
    
    engine_globals->this_frame_timestamp_us =
        platform_get_current_time_us();
    
    if (gameloop_previous_time < 1) {
        gameloop_previous_time = engine_globals->this_frame_timestamp_us;
        
        #if PROFILER_ACTIVE
        profiler_end("gameloop_update()");
        #endif
        return;
    }
    
    if (!application_running) {
        if (crashed_top_of_screen_msg[0] == '\0') {
            common_strcpy_capped(
                crashed_top_of_screen_msg,
                256,
                "Failed assert, and also failed to retrieve an error message");
        }
        
        show_dead_simple_text(
            frame_data,
            crashed_top_of_screen_msg,
            elapsed);
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
            
            #if PROFILER_ACTIVE
            profiler_end("gameloop_update()");
            #endif
            return;
        } else {
            
            engine_globals->last_resize_request_us = 0;
            log_append("\nOK, resize window\n");
            
            engineglobals_init();
            
            platform_gpu_update_viewport();
            
            #if TERMINAL_ACTIVE
            terminal_redraw_backgrounds();
            #endif
            
            client_logic_window_resize(
                (uint32_t)engine_globals->window_height,
                (uint32_t)engine_globals->window_width);
       }
    } else if (application_running) {
        
        #if SCHEDULED_ANIMS_ACTIVE
        scheduled_animations_resolve();
        #endif
        
        platform_update_mouse_location();
        
        // handle timed occlusions
        for (uint32_t zs_i = 0; zs_i < zsprites_to_render->size; zs_i++) {
            if (
                zsprites_to_render->cpu_data[zs_i].next_occlusion_in_us >
                    elapsed)
            {
                zsprites_to_render->cpu_data[zs_i].next_occlusion_in_us -=
                    elapsed;
            } else if (
                zsprites_to_render->cpu_data[zs_i].next_occlusion_in_us > 0)
            {
                zsprites_to_render->cpu_data[zs_i].next_occlusion_in_us = 0;
                zsprites_to_render->cpu_data[zs_i].visible = 0;
            }
        }
        
        // always copy
        user_interactions[INTR_PREVIOUS_MOUSE_MOVE] =
            user_interactions[INTR_PREVIOUS_MOUSE_OR_TOUCH_MOVE];
        user_interactions[INTR_PREVIOUS_TOUCH_MOVE] =
            user_interactions[INTR_PREVIOUS_MOUSE_OR_TOUCH_MOVE];
        
        ui_elements_handle_touches(elapsed);
        
        #if TERMINAL_ACTIVE
        update_terminal();
        #endif
        
        client_logic_update(elapsed);
        
        camera.xyz_cosangle[0] = cosf(camera.xyz_angle[0]);
        camera.xyz_cosangle[1] = cosf(camera.xyz_angle[1]);
        camera.xyz_cosangle[2] = cosf(camera.xyz_angle[2]);
        camera.xyz_sinangle[0] = sinf(camera.xyz_angle[0]);
        camera.xyz_sinangle[1] = sinf(camera.xyz_angle[1]);
        camera.xyz_sinangle[2] = sinf(camera.xyz_angle[2]);
        
        clean_deleted_lights();
        
        copy_lights(
            frame_data->lights,
            &engine_globals->postprocessing_constants.lights_size,
            &engine_globals->postprocessing_constants.shadowcaster_i);
        
        renderer_hardware_render(
                frame_data,
            /* uint64_t elapsed_us: */
                elapsed);
        
        uint32_t overflow_vertices = frame_data->vertices_size % 3;
        frame_data->vertices_size -= overflow_vertices;
    }
    
    if (engine_globals->draw_fps) {
        text_request_fps_counter(
            /* uint64_t elapsed_us: */
                elapsed);
    } else if (engine_globals->draw_top_touchable_id) {
        text_request_top_touchable_id(
            user_interactions[INTR_PREVIOUS_MOUSE_OR_TOUCH_MOVE].
                touchable_id_top);
    }
    
    frame_data->postprocessing_constants->timestamp =
        (uint32_t)engine_globals->this_frame_timestamp_us;
    frame_data->postprocessing_constants->shadowcaster_i =
        shadowcaster_light_i;
    
    #if PROFILER_ACTIVE
    profiler_end("gameloop_update()");
    #endif
    
    #if PROFILER_ACTIVE
    profiler_draw_labels();
    #endif
}

void gameloop_update_after_render_pass(void) {
    if (application_running) {
        client_logic_update_after_render_pass();
    }
    
    user_interactions[INTR_LAST_GPU_DATA].touchable_id_top =
        platform_gpu_get_touchable_id_at_screen_pos(
            /* const int screen_x: */
                user_interactions[INTR_LAST_GPU_DATA].
                    screen_x,
            /* const int screen_y: */
                user_interactions[INTR_LAST_GPU_DATA].
                    screen_y);
    user_interactions[INTR_LAST_GPU_DATA].touchable_id_pierce =
        user_interactions[INTR_LAST_GPU_DATA].touchable_id_top;
}
