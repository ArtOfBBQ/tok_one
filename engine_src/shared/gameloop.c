#include "gameloop.h"

bool32_t gameloop_active = false;

static uint64_t gameloop_previous_time = 0;
static uint64_t gameloop_frame_no = 0;
static uint32_t gameloop_mutex_id = UINT32_MAX;

static int32_t closest_touchable_from_screen_ray(
    const float screen_x,
    const float screen_y,
    float * collision_point,
    const bool8_t update_clickray)
{
    float clicked_viewport_x =
        -1.0f + ((screen_x / window_globals->window_width) * 2.0f);
    float clicked_viewport_y =
        -1.0f + ((screen_y / window_globals->window_height) * 2.0f);
    
    float close_z = 0.00001f;
    float ray_origin[3];
    ray_origin[0] =
        (clicked_viewport_x /
            window_globals->projection_constants.x_multiplier) *
                close_z;
    ray_origin[1] =
        ((clicked_viewport_y) /
            window_globals->projection_constants.field_of_view_modifier) *
                close_z;
    ray_origin[2] = close_z;
    
    z_rotate_zvertex_f3(
        ray_origin,
        camera.xyz_angle[2]);
    y_rotate_zvertex_f3(
        ray_origin,
        camera.xyz_angle[1]);
    x_rotate_zvertex_f3(
        ray_origin,
        camera.xyz_angle[0]);
    
    ray_origin[0] += camera.xyz[0];
    ray_origin[1] += camera.xyz[1];
    ray_origin[2] += camera.xyz[2];
    
    // we need a point that's distant, but yet also ends up at the same
    // screen position as ray_origin
    //
    // given:
    // projected_x = x * pjc->x_multiplier / z
    //     and we want projected_x to be ray_origin.x:
    //     ray_origin.x = (x * pjc->x_multiplier) / z
    //     ray_origin.x * z = x * pjc->x_multi
    // (ray_origin.x * z / pjc->x_multi) = x
    float distant_z = 50.0f;
    float distant_point[3];
    distant_point[0] =
        (clicked_viewport_x /
            window_globals->projection_constants.x_multiplier) *
                distant_z;
    distant_point[1] =
        (clicked_viewport_y /
            window_globals->projection_constants.field_of_view_modifier) *
                distant_z;
    distant_point[2] = distant_z;
    
    z_rotate_zvertex_f3(
        distant_point,
        camera.xyz_angle[2]);
    y_rotate_zvertex_f3(
        distant_point,
        camera.xyz_angle[1]);
    x_rotate_zvertex_f3(
        distant_point,
        camera.xyz_angle[0]);
    
    distant_point[0] += camera.xyz[0];
    distant_point[1] += camera.xyz[1];
    distant_point[2] += camera.xyz[2];
    
    float direction_to_distant[3];
    direction_to_distant[0] =
        distant_point[0] - ray_origin[0];
    direction_to_distant[1] =
        distant_point[1] - ray_origin[1];
    direction_to_distant[2] =
        distant_point[2] - ray_origin[2];
    normalize_zvertex_f3(direction_to_distant);
    
    if (update_clickray) {
        common_memcpy(
            window_globals->last_clickray_origin,
            ray_origin,
            sizeof(float)*3);
        common_memcpy(
            window_globals->last_clickray_direction,
            direction_to_distant,
            sizeof(float)*3);
    }
    
    int32_t return_value = -1;
    float smallest_t_along_ray = COL_FLT_MAX;
    uint32_t smallest_zpolygon_i = MAX_POLYGONS_PER_BUFFER-1;
    
    for (
        uint32_t zp_i = 0;
        zp_i < zpolygons_to_render->size;
        zp_i++)
    {
        if (
            zpolygons_to_render->cpu_data[zp_i].deleted ||
            zpolygons_to_render->cpu_data[zp_i].touchable_id < 0)
        {
            continue;
        }
        
        float current_collision_point[3];
        uint32_t current_tri_vert_i = UINT32_MAX;
        
        #ifdef PROFILER_ACTIVE
        profiler_start("ray_intersects_zpolygon()");
        #endif
        float t_along_ray_to_zpolygon = ray_intersects_zpolygon(
            /* origin: */
                ray_origin,
            /* direction: */
                direction_to_distant,
            /* const zPolygonCPU * cpu_data: */
                &zpolygons_to_render->cpu_data[zp_i],
            /* const GPUPolygon * gpu_data: */
                &zpolygons_to_render->gpu_data[zp_i],
            /* float * recipient_hit_point: */
                current_collision_point,
            /* int32_t * recipient_triangle_vert_i: */
                &current_tri_vert_i);
        #ifdef PROFILER_ACTIVE
        profiler_end("ray_intersects_zpolygon()");
        #endif
        
        if (t_along_ray_to_zpolygon < smallest_t_along_ray &&
            t_along_ray_to_zpolygon < (COL_FLT_MAX / 2))
        {
            smallest_t_along_ray = t_along_ray_to_zpolygon;
            smallest_zpolygon_i = zp_i;
            return_value = zpolygons_to_render->cpu_data[zp_i].touchable_id;
            common_memcpy(
                collision_point,
                current_collision_point,
                sizeof(float) * 3);
        }
    }
    
    if (update_clickray && return_value < FLOAT32_MAX / 2) {
        common_memcpy(
            window_globals->last_clickray_collision,
            collision_point,
            sizeof(float) * 3);
    }
    
    return return_value;
}

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
    gameloop_mutex_id = platform_init_mutex_and_return_id();
}

void gameloop_update(
    GPUDataForSingleFrame * frame_data)
{
    if (!gameloop_active) {
        return;
    }
    
    #ifdef PROFILER_ACTIVE
    profiler_start("gameloop_update()");
    #endif
    
    log_assert(frame_data->light_collection != NULL);
    log_assert(frame_data->camera != NULL);
    
    uint64_t time = platform_get_current_time_microsecs();
    if (gameloop_previous_time < 1) {
        gameloop_previous_time = time;
        // platform_mutex_unlock(gameloop_mutex_id);
        
        #ifdef PROFILER_ACTIVE
        profiler_end("gameloop_update()");
        #endif
        return;
    }
    
    uint64_t elapsed = time - gameloop_previous_time;
    gameloop_previous_time = time;
    
    if (!application_running) {
        delete_all_ui_elements();
        particle_effects_size = 0;
        lineparticle_effects_size = 0;
        zlights_to_apply_size = 0;
        
        frame_data->camera->xyz[0] = 0.0f;
        frame_data->camera->xyz[1] = 0.0f;
        frame_data->camera->xyz[2] = 0.0f;
        frame_data->camera->xyz_angle[0] = 0.0f;
        frame_data->camera->xyz_angle[1] = 0.0f;
        frame_data->camera->xyz_angle[2] = 0.0f;
        
        font_height = 28.0f;
        font_ignore_lighting = true;
        font_color[0] = 0.8f;
        font_color[1] = 0.8f;
        font_color[2] = 1.0f;
        font_color[3] = 1.0f;
        
        if (crashed_top_of_screen_msg[0] == '\0') {
            common_strcpy_capped(crashed_top_of_screen_msg,
            256,
            "Failed assert, and also failed to retrieve an error message");
        }
        
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
                window_globals->window_width - 30,
            /* const bool32_t ignore_camera: */
                true);
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
            // platform_mutex_unlock(gameloop_mutex_id);
            
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
        
        platform_update_mouse_location();
        
        update_terminal();
        
        #ifdef PROFILER_ACTIVE
        profiler_start("resolve_animation_effects()");
        #endif
        resolve_animation_effects(elapsed);
        #ifdef PROFILER_ACTIVE
        profiler_end("resolve_animation_effects()");
        #endif
        
        clean_deleted_lights();
        
        copy_lights(frame_data->light_collection);
        
        #ifdef PROFILER_ACTIVE
        profiler_start("check touchables for user_interactions");
        #endif
        for (uint32_t i = 0; i < 9; i++) {
            if (
                user_interactions[i].handled ||
                user_interactions[i].checked_touchables)
            {
                continue;
            }
            
            float collision_point[3];
            user_interactions[i].touchable_id =
                closest_touchable_from_screen_ray(
                    /* screen_x: */
                        user_interactions[i].screen_x,
                    /* screen_y: */
                        user_interactions[i].screen_y,
                    /* collision point: */
                        collision_point,
                    /* update_clickray: */
                        i < 8);
            
            user_interactions[i].checked_touchables = true;
            
            for (uint32_t j = i+1; j<9; j++) {
                if (
                    !user_interactions[j].handled &&
                    !user_interactions[j].checked_touchables &&
                    user_interactions[j].screen_x ==
                        user_interactions[i].screen_x &&
                    user_interactions[j].screen_y ==
                        user_interactions[i].screen_y)
                {
                    user_interactions[j].checked_touchables = true;
                    user_interactions[j].touchable_id =
                        user_interactions[i].touchable_id;
                }
            }
            break;
        }
        #ifdef PROFILER_ACTIVE
        profiler_end("check touchables for user_interactions");
        #endif
        
        ui_elements_handle_touches(elapsed);
        
        #ifdef PROFILER_ACTIVE
        profiler_handle_touches();
        profiler_start("client_logic_update");
        #endif
        client_logic_update(elapsed);
        #ifdef PROFILER_ACTIVE
        profiler_end("client_logic_update");
        #endif
        
        init_or_push_one_gpu_texture_array_if_needed();
    }
    
    log_assert(frame_data->light_collection != NULL);
    common_memcpy(frame_data->camera, &camera, sizeof(GPUCamera));
    log_assert(frame_data->light_collection != NULL);
    
    frame_data->vertices_size = 0;
    frame_data->polygon_collection->size = 0;
    frame_data->first_alphablend_i = 0;
    frame_data->line_vertices_size = 0;
    frame_data->point_vertices_size = 0;
    
    if (window_globals->draw_fps) {
        text_request_fps_counter(
            /* uint64_t microseconds_elapsed: */
                elapsed);
    }
    
    renderer_hardware_render(
            frame_data,
        /* uint64_t elapsed_microseconds: */
            elapsed);
    
    uint32_t overflow_vertices = frame_data->vertices_size % 3;
    frame_data->vertices_size -= overflow_vertices;
    
    #ifdef PROFILER_ACTIVE
    profiler_end("gameloop_update()");
    #endif
    
    #ifdef PROFILER_ACTIVE
    profiler_draw_labels();
    #endif
}
