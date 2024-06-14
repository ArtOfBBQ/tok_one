#include "gameloop.h"

static uint64_t gameloop_previous_time = 0;
static uint64_t gameloop_frame_no = 0;
static uint32_t gameloop_mutex_id = UINT32_MAX;

static int32_t closest_touchable_from_screen_ray(
    const float screen_x,
    const float screen_y,
    zVertex * collision_point)
{
    #ifndef LOGGER_IGNORE_ASSERTS
    uint32_t nonzero_camera_angles = 0;
    if (camera.xyz_angle[0] != 0.0f) { nonzero_camera_angles += 1; }
    if (camera.xyz_angle[1] != 0.0f) { nonzero_camera_angles += 1; }
    if (camera.xyz_angle[2] != 0.0f) { nonzero_camera_angles += 1; }
    
    // TODO: study raytracing math and find out why our solution doesn't
    // TODO: support 2 or more camera rotations  
    log_assert(nonzero_camera_angles < 2);
    #endif
    
    float clicked_viewport_x =
        -1.0f + ((screen_x / window_globals->window_width) * 2.0f);
    float clicked_viewport_y =
        -1.0f + ((screen_y / window_globals->window_height) * 2.0f);
    
    float z_multiplier = 0.00001f;
    zVertex ray_origin;
    ray_origin.x =
        (clicked_viewport_x /
            window_globals->projection_constants.x_multiplier) *
                z_multiplier;
    ray_origin.y =
        ((clicked_viewport_y) /
            window_globals->projection_constants.field_of_view_modifier) *
                z_multiplier;
    ray_origin.z = z_multiplier;
    
    zVertex ray_origin_rotated = ray_origin;
    ray_origin_rotated = x_rotate_zvertex(
        &ray_origin_rotated, camera.xyz_angle[0]);
    ray_origin_rotated = y_rotate_zvertex(
        &ray_origin_rotated, camera.xyz_angle[1]);
    ray_origin_rotated = z_rotate_zvertex(
        &ray_origin_rotated, camera.xyz_angle[2]);
    
    // we need a point that's distant, but yet also ends up at the same
    // screen position as ray_origin
    //
    // This is also the distant point we draw to for visual debug
    //
    // given:
    // projected_x = x * pjc->x_multiplier / z
    //     and we want projected_x to be ray_origin.x:
    // ray_origin.x = (x * pjc->x_multiplier) / z
    // ray_origin.x * z = x * pjc->x_multi
    // (ray_origin.x * z / pjc->x_multi) = x
    float distant_z = 10.0f;
    zVertex distant_point;
    distant_point.x =
        (clicked_viewport_x /
            window_globals->projection_constants.x_multiplier) *
                distant_z;
    distant_point.y =
        (clicked_viewport_y /
            window_globals->projection_constants.field_of_view_modifier) *
                distant_z;
    distant_point.z = distant_z;
    
    
    zVertex distant_point_rotated = distant_point;
    distant_point_rotated =
        x_rotate_zvertex(
            &distant_point_rotated,
            camera.xyz_angle[0]);
    distant_point_rotated =
        y_rotate_zvertex(
            &distant_point_rotated,
            camera.xyz_angle[1]);
    distant_point_rotated =
        z_rotate_zvertex(
            &distant_point_rotated,
            camera.xyz_angle[2]);
    
    window_globals->visual_debug_ray_origin_direction[0] =
        ray_origin.x;
    window_globals->visual_debug_ray_origin_direction[1] =
        ray_origin.y;
    window_globals->visual_debug_ray_origin_direction[2] =
        ray_origin.z;
    window_globals->visual_debug_ray_origin_direction[3] =
        ray_origin.x + 0.03f;
    window_globals->visual_debug_ray_origin_direction[4] =
        ray_origin.y + 0.03f;
    window_globals->visual_debug_ray_origin_direction[5] =
        ray_origin.z + 0.03f;
    window_globals->visual_debug_ray_origin_direction[6] =
        ray_origin.x + (distant_point_rotated.x * 1.0f);
    window_globals->visual_debug_ray_origin_direction[7] =
        ray_origin.y + (distant_point_rotated.y * 1.0f);
    window_globals->visual_debug_ray_origin_direction[8] =
        ray_origin.z + (distant_point_rotated.z * 1.0f);
    
    normalize_zvertex(&distant_point);
    normalize_zvertex(&distant_point_rotated);
    
    int32_t return_value = -1;
    float smallest_dist = FLOAT32_MAX;
    
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
        
        zVertex current_collision_point;
        bool32_t hit = false;
        zPolygonCPU offset_polygon_cpu = zpolygons_to_render->cpu_data[zp_i];
        GPUPolygon offset_polygon_gpu = zpolygons_to_render->gpu_data[zp_i];
        
        float camera_offset_x = camera.xyz[0] *
            (1.0f - zpolygons_to_render->gpu_data[zp_i].ignore_camera);
        float camera_offset_y = camera.xyz[1] *
            (1.0f - zpolygons_to_render->gpu_data[zp_i].ignore_camera);
        float camera_offset_z = camera.xyz[2] *
            (1.0f - zpolygons_to_render->gpu_data[zp_i].ignore_camera);
        
        offset_polygon_gpu.xyz[0] -= camera_offset_x;
        offset_polygon_gpu.xyz[1] -= camera_offset_y;
        offset_polygon_gpu.xyz[2] -= camera_offset_z;
        offset_polygon_gpu.xyz[0] += offset_polygon_gpu.xyz_offset[0];
        offset_polygon_gpu.xyz[1] += offset_polygon_gpu.xyz_offset[1];
        offset_polygon_gpu.xyz[2] += offset_polygon_gpu.xyz_offset[2];
        
        hit = ray_intersects_zpolygon_hitbox(
            offset_polygon_gpu.ignore_camera > 0.5f ?
                &ray_origin : &ray_origin_rotated,
            offset_polygon_gpu.ignore_camera > 0.5f ?
                &distant_point : &distant_point_rotated,
            &offset_polygon_cpu,
            &offset_polygon_gpu,
            &current_collision_point);
        
        zVertex offset_collision_point = current_collision_point;
        offset_collision_point.x += camera_offset_x;
        offset_collision_point.y += camera_offset_y;
        offset_collision_point.z += camera_offset_z;
        
        float dist_to_hit = get_distance(offset_collision_point, ray_origin);
        if (hit && dist_to_hit < smallest_dist) {
            smallest_dist = dist_to_hit;
            return_value = offset_polygon_cpu.touchable_id;
            *collision_point = current_collision_point;            
        }
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

void shared_gameloop_init(void) {
    gameloop_mutex_id = platform_init_mutex_and_return_id();
}

void shared_gameloop_update(
    GPUDataForSingleFrame * frame_data)
{
    // platform_mutex_lock(gameloop_mutex_id);
    
    uint64_t time = platform_get_current_time_microsecs();
    if (gameloop_previous_time < 1) {
        gameloop_previous_time = time;
        // platform_mutex_unlock(gameloop_mutex_id);
        return;
    }
    uint64_t elapsed = time - gameloop_previous_time;
    
    if (!application_running) {
        delete_all_ui_elements();
        zpolygons_to_render->size = 0;
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
            strcpy_capped(crashed_top_of_screen_msg,
            256,
            "Failed assert, and also failed to retrieve an error message");
        }
        
        request_label_renderable(
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
    
    window_globals->visual_debug_collision_size =
        ((time % 500000) / 50000) * 0.001f;
    
    gameloop_previous_time = time;
    
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
            return;
        } else {
            
            window_globals->last_resize_request_at = 0;
            log_append("\nOK, resize window\n");
            
            init_projection_constants();
            
            platform_gpu_update_viewport();
            
            terminal_redraw_backgrounds();
            
            client_logic_window_resize(
                (uint32_t)window_globals->window_height,
                (uint32_t)window_globals->window_width);
       }
    } else {
        init_or_push_one_gpu_texture_array_if_needed();
    }
    
    if (application_running == true) {
        
        update_terminal();
        
        if (debugmouseptr_id >= 0) {
            ScheduledAnimation * move_mouseptr =
                next_scheduled_animation(true);
            move_mouseptr->affected_object_id = debugmouseptr_id;
            move_mouseptr->gpu_polygon_vals.xyz[0] =
                screenspace_x_to_x(
                    user_interactions[INTR_PREVIOUS_MOUSE_MOVE].screen_x,
                    1.0f);
            move_mouseptr->gpu_polygon_vals.xyz[1] =
                screenspace_y_to_y(
                    user_interactions[INTR_PREVIOUS_MOUSE_MOVE].screen_y,
                    1.0f);
            move_mouseptr->gpu_polygon_vals.xyz[2] = 1.0f;
            move_mouseptr->duration_microseconds = 1;
            commit_scheduled_animation(move_mouseptr);
        }
        
        resolve_animation_effects(elapsed);
        clean_deleted_lights();
        
        copy_lights(frame_data->light_collection);
        
        for (uint32_t i = 0; i < 8; i++) {
            if (
                user_interactions[i].handled ||
                user_interactions[i].checked_touchables)
            {
                continue;
            }
            
            zVertex collision_point;
            user_interactions[i].touchable_id =
                closest_touchable_from_screen_ray(
                    /* screen_x: */
                        user_interactions[i].screen_x,
                    /* screen_y: */
                        user_interactions[i].screen_y,
                    /* collision point: */
                        &collision_point);
            
            window_globals->visual_debug_last_clicked_touchable_id =
                user_interactions[i].touchable_id;
            
            user_interactions[i].checked_touchables = true;
            
            for (uint32_t j = i + 1; j < USER_INTERACTIONS_SIZE; j++) {
                if (
                    user_interactions[j].checked_touchables ||
                    user_interactions[j].handled)
                {
                    continue;
                }
                
                if (
                    user_interactions[i].screen_x ==
                        user_interactions[j].screen_x &&
                    user_interactions[i].screen_y ==
                        user_interactions[j].screen_y)
                {
                    user_interactions[j].touchable_id =
                        user_interactions[i].touchable_id;
                    user_interactions[j].checked_touchables = true;
                }
            }
            
            break; // max 1 ray per frame
        }
        
        ui_elements_handle_touches(elapsed);
        
        client_logic_update(elapsed);        
    }
    
    frame_data->camera->xyz[0] = camera.xyz[0];
    frame_data->camera->xyz[1] = camera.xyz[1];
    frame_data->camera->xyz[2] = camera.xyz[2];
    frame_data->camera->xyz_angle[0] = camera.xyz_angle[0];
    frame_data->camera->xyz_angle[1] = camera.xyz_angle[1];
    frame_data->camera->xyz_angle[2] = camera.xyz_angle[2];
    
    frame_data->vertices_size = 0;
    frame_data->polygon_collection->size = 0;
    frame_data->first_alphablend_i = 0;
    frame_data->first_line_i = 0;
    hardware_render(
            frame_data,
        /* uint64_t elapsed_microseconds: */
            elapsed);
    
    uint32_t overflow_vertices = frame_data->vertices_size % 3;
    frame_data->vertices_size -= overflow_vertices;
    
    // platform_mutex_unlock(gameloop_mutex_id);
}

