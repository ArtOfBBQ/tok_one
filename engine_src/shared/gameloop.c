#include "gameloop.h"

bool32_t gameloop_active = false;

static uint64_t gameloop_previous_time = 0;
static uint64_t gameloop_frame_no = 0;


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

void gameloop_init(void) {
}

void gameloop_update_before_render_pass(
    GPUDataForSingleFrame * frame_data)
{
    if (!gameloop_active) {
        return;
    }
    
    window_globals->next_transformed_imputed_normal_i = 0;
    
    #ifdef PROFILER_ACTIVE
    profiler_start("gameloop_update()");
    #endif
    
    log_assert(frame_data->light_collection != NULL);
    log_assert(frame_data->camera != NULL);
    
    uint64_t time = platform_get_current_time_microsecs();
    if (gameloop_previous_time < 1) {
        gameloop_previous_time = time;
        
        #ifdef PROFILER_ACTIVE
        profiler_end("gameloop_update()");
        #endif
        return;
    }
    
    uint64_t elapsed = time - gameloop_previous_time;
    gameloop_previous_time = time;
    
    log_assert(frame_data->light_collection != NULL);
    common_memcpy(frame_data->camera, &camera, sizeof(GPUCamera));
    log_assert(frame_data->light_collection != NULL);
    
    frame_data->vertices_size            = 0;
    frame_data->polygon_collection->size = 0;
    frame_data->first_alphablend_i       = 0;
    frame_data->line_vertices_size       = 0;
    frame_data->point_vertices_size      = 0;
    
    if (!application_running) {
        delete_all_ui_elements();
        particle_effects_size = 0;
        lineparticle_effects_size = 0;
        zlights_to_apply_size = 0;
        zpolygons_to_render->size = 0;
        
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
        
        if (crashed_top_of_screen_msg[0] == '\0') {
            common_strcpy_capped(crashed_top_of_screen_msg,
            256,
            "Failed assert, and also failed to retrieve an error message");
        }
        
        font_settings->font_height          = 14.0f;
        font_settings->font_color[0]        = 1.0f;
        font_settings->font_color[1]        = 1.0f;
        font_settings->font_color[2]        = 1.0f;
        font_settings->font_color[3]        = 1.0f;
        font_settings->ignore_camera        = false; // we set camera to 0,0,0
        font_settings->font_ignore_lighting = true;
        
        PointRequest point_request;
        fetch_next_point(&point_request);
        point_request.cpu_data->object_id = -1;
        point_request.gpu_vertex->color   = 1.0f;
        point_request.gpu_vertex->xyz[0]  = 0.0f;
        point_request.gpu_vertex->xyz[1]  = 0.0f;
        point_request.gpu_vertex->xyz[2]  = 0.8f;
        commit_point(&point_request);
        
        assert(font_settings->font_texturearray_i == 0);
        text_request_label_renderable(
            /* const uint32_t with_object_id: */
                0,
            /* const char * text_to_draw: */
                crashed_top_of_screen_msg,
            /* const float left_pixelspace: */
                30,
            /* const float top_pixelspace: */
                window_globals->window_height - 30,
            /* const float z: */
                1.0f,
            /* const float max_width: */
                window_globals->window_width - 30);
        
        renderer_hardware_render(
                frame_data,
            /* uint64_t elapsed_microseconds: */
                elapsed);
        return;
    }
    
    gameloop_frame_no++;
    
    if (time - window_globals->last_resize_request_at < 2000000)
    {
        if (time - window_globals->last_resize_request_at < 350000)
        {
            // possibly a request we already handled, or not the final
            // request, wait...
            // we break, not return, because we do want to render an
            // empty screen
            log_append("w82RZ - ");
            
            #ifdef PROFILER_ACTIVE
            profiler_end("gameloop_update()");
            #endif
            return;
        } else {
            
            window_globals->last_resize_request_at = 0;
            log_append("\nOK, resize window\n");
            
            windowsize_init();
            
            platform_gpu_update_viewport();
            
            terminal_redraw_backgrounds();
            
            client_logic_window_resize(
                (uint32_t)window_globals->window_height,
                (uint32_t)window_globals->window_width);
       }
    } else if (application_running) {
        
        camera.xyz_cosangle[0] = cosf(camera.xyz_angle[0]);
        camera.xyz_cosangle[1] = cosf(camera.xyz_angle[1]);
        camera.xyz_cosangle[2] = cosf(camera.xyz_angle[2]);
        camera.xyz_sinangle[0] = sinf(camera.xyz_angle[0]);
        camera.xyz_sinangle[1] = sinf(camera.xyz_angle[1]);
        camera.xyz_sinangle[2] = sinf(camera.xyz_angle[2]);
        
        clean_deleted_lights();
        
        copy_lights(frame_data->light_collection);
        
        renderer_hardware_render(
                frame_data,
            /* uint64_t elapsed_microseconds: */
                elapsed);
        
        uint32_t overflow_vertices = frame_data->vertices_size % 3;
        frame_data->vertices_size -= overflow_vertices;
        
        platform_update_mouse_location();
        
        // always copy
        user_interactions[INTR_PREVIOUS_MOUSE_MOVE] =
            user_interactions[INTR_PREVIOUS_MOUSE_OR_TOUCH_MOVE];
        user_interactions[INTR_PREVIOUS_TOUCH_MOVE] =
            user_interactions[INTR_PREVIOUS_MOUSE_OR_TOUCH_MOVE];
        
        update_terminal();
        
        scheduled_animations_resolve(elapsed);
        
        ui_elements_handle_touches(elapsed);
        
        client_logic_update(elapsed);
        
        texture_array_gpu_try_push();
    }
    
    if (window_globals->draw_fps) {
        text_request_fps_counter(
            /* uint64_t microseconds_elapsed: */
                elapsed);
    } else if (window_globals->draw_top_touchable_id) {
        text_request_top_touchable_id(
            user_interactions[INTR_PREVIOUS_MOUSE_OR_TOUCH_MOVE].
                touchable_id_top);
    }
    
    #ifdef PROFILER_ACTIVE
    profiler_end("gameloop_update()");
    #endif
    
    #ifdef PROFILER_ACTIVE
    profiler_draw_labels();
    #endif
}
