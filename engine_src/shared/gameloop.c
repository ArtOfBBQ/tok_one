#include "gameloop.h"

static uint64_t previous_time = 0;
static uint64_t frame_no = 0;

static int32_t closest_touchable_from_screen_ray(
    const float screen_x,
    const float screen_y,
    zVertex * collision_point)
{
    float clicked_viewport_x =
        -1.0f + ((screen_x / window_width) * 2.0f);
    float clicked_viewport_y =
        -1.0f + ((screen_y / window_height) * 2.0f);
    
    zVertex ray_origin;
    ray_origin.x =
        (clicked_viewport_x / projection_constants.x_multiplier) *
            projection_constants.near;
    ray_origin.y =
        (clicked_viewport_y / projection_constants.field_of_view_modifier) *
            projection_constants.near;
    ray_origin.z = projection_constants.near;
    
    // we need a point that's distant, but yet also ends up at the same
    // screen position as ray_origin
    //
    // This is also the distant point we draw to for visual debug
    //
    // given:
    // projected_x = x * pjc->x_multiplier / z
    // and we want projected_x to be ray_origin.x:
    // ray_origin.x = (x * pjc->x_multiplier) / z
    // ray_origin.x * z = x * pjc->x_multi
    // (ray_origin.x * z / pjc->x_multi) = x
    float offset_z = camera.z + 1.0f;
    zVertex distant_point;
    distant_point.x =
        (clicked_viewport_x / projection_constants.x_multiplier) *
            offset_z;
    distant_point.y =
        (clicked_viewport_y / projection_constants.field_of_view_modifier) *
            offset_z;
    distant_point.z = offset_z;
    
    zVertex ray_direction = distant_point;
    normalize_zvertex(&ray_direction);
    //    ray_direction = x_rotate_zvertex(&ray_direction, camera.x_angle);
    //    ray_direction = y_rotate_zvertex(&ray_direction, camera.y_angle);
    //    ray_direction = z_rotate_zvertex(&ray_direction, camera.z_angle);
    //    normalize_zvertex(&ray_direction);
    
    int32_t return_value = -1;
    float smallest_dist = FLOAT32_MAX;
    
    for (
        uint32_t zp_i = 0;
        zp_i < zpolygons_to_render_size;
        zp_i++)
    {
        if (
            zpolygons_to_render[zp_i].deleted ||
            zpolygons_to_render[zp_i].touchable_id < 0)
        {
            continue;
        }
        
        zVertex current_collision_point;
        bool32_t hit = false;
        hit = ray_intersects_zpolygon_hitbox(
            &ray_origin,
            &ray_direction,
            &zpolygons_to_render[zp_i],
            &current_collision_point);
        
        float dist_to_hit = get_distance(*collision_point, ray_origin);
        if (hit && dist_to_hit < smallest_dist) {
            smallest_dist = dist_to_hit;
            return_value = zpolygons_to_render[zp_i].touchable_id;
            *collision_point = current_collision_point;            
        }
    }
    
    return return_value;
}

void shared_gameloop_update(
    GPU_Vertex * vertices_for_gpu,
    uint32_t * vertices_for_gpu_size,
    GPU_LightCollection * lights_for_gpu,
    GPU_Camera * camera_for_gpu,
    GPU_ProjectionConstants * projection_constants_for_gpu)
{
    uint64_t time = platform_get_current_time_microsecs();
    uint64_t elapsed = time - previous_time;
    if (previous_time < 1) {
        previous_time = time;
        return;
    }
    
    visual_debug_collision_size = ((time % 500000) / 50000) * 0.001f;
    
    *projection_constants_for_gpu = projection_constants;
    
    previous_time = time;
    
    frame_no++;
    
    if (
        time - last_resize_request_at < 1500000 &&
        !request_post_resize_clearscreen)
    {
        if (time - last_resize_request_at < 200000) {
            return;
        }
        
        last_resize_request_at = 999999999;
        client_logic_window_resize(
            (uint32_t)window_height,
            (uint32_t)window_width);
    } else {
        init_or_push_one_gpu_texture_array_if_needed();
    }
    
    resolve_animation_effects(elapsed);
    clean_deleted_lights();
    
    copy_lights(lights_for_gpu);
    
    if (!application_running) {
        zpolygons_to_render_size = 0;
        camera.x = 0.0f;
        camera.y = 0.0f;
        camera.z = 0.0f;
        camera.x_angle = 0.0f;
        camera.y_angle = 0.0f;
        camera.z_angle = 0.0f;
        
        font_height = 28.0f;
        font_ignore_lighting = true;
        font_color[0] = 0.8f;
        font_color[1] = 0.8f;
        font_color[2] = 1.0f;
        font_color[3] = 1.0f;
        
        if (crashed_top_of_screen_msg[0] == '\0') {
            strcpy_capped(crashed_top_of_screen_msg,
            256,
            "The engine failed an assertion, and also failed to retrieve an error message");
        }
        
        request_label_renderable(
            /* const uint32_t with_object_id: */
                0,
            /* const char * text_to_draw: */
                crashed_top_of_screen_msg,
            /* const float left_pixelspace: */
                30,
            /* const float top_pixelspace: */
                window_height - 30,
            /* const float z: */
                0.99f,
            /* const float max_width: */
                window_width - 30,
            /* const bool32_t ignore_camera: */
                true);
    } else {
        Interaction * touchable_seekers[11];
        touchable_seekers[0]  = &previous_touch_start;
        touchable_seekers[1]  = &previous_touch_end;
        touchable_seekers[2]  = &previous_leftclick_start;
        touchable_seekers[3]  = &previous_leftclick_end;
        touchable_seekers[4]  = &previous_touch_or_leftclick_start;
        touchable_seekers[5]  = &previous_touch_or_leftclick_end;
        touchable_seekers[6]  = &previous_rightclick_start;
        touchable_seekers[7]  = &previous_rightclick_end;
        touchable_seekers[8]  = &previous_mouse_move;
        touchable_seekers[9]  = &previous_touch_move;
        touchable_seekers[10] = &previous_mouse_or_touch_move;
        
        for (uint32_t i = 0; i < 11; i++) {
            if (
                touchable_seekers[i]->handled ||
                touchable_seekers[i]->checked_touchables)
            {
                continue;
            }
            
            zVertex collision_point;
            touchable_seekers[i]->touchable_id =
                closest_touchable_from_screen_ray(
                    /* screen_x: */
                        touchable_seekers[i]->screen_x,
                    /* screen_y: */
                        touchable_seekers[i]->screen_y,
                    /* collision point: */
                        &collision_point);
            
            if (
                visual_debug_mode &&
                touchable_seekers[i] == &previous_leftclick_start)
            {
                if (touchable_seekers[i]->touchable_id >= 0) {
                    // TODO: remove debug code
                    int32_t zp_i = 0;
                    for (; zp_i < zpolygons_to_render_size; zp_i++) {
                        if (zpolygons_to_render[zp_i].touchable_id ==
                            touchable_seekers[i]->touchable_id)
                        {
                            break;
                        }
                    }
                    log_assert(zp_i >= 0);
                    log_assert(zpolygons_to_render[zp_i].touchable_id ==
                               touchable_seekers[i]->touchable_id);
                    float dist_x = collision_point.x - zpolygons_to_render[zp_i].x;
                    float dist_y = collision_point.y - zpolygons_to_render[zp_i].y;
                    float dist_z = collision_point.z - zpolygons_to_render[zp_i].z;
                    log_assert(dist_x < 0.2f);
                    log_assert(dist_y < 0.2f);
                    log_assert(dist_z < 0.2f);
                    
                    visual_debug_collision[0] = collision_point.x;
                    visual_debug_collision[1] = collision_point.y;
                    visual_debug_collision[2] = collision_point.z;
                }
                
                float clicked_viewport_x =
                    -1.0f + (
                        (touchable_seekers[i]->screen_x / window_width) *
                            2.0f);
                float clicked_viewport_y =
                    -1.0f + (
                        (touchable_seekers[i]->screen_y / window_height) *
                            2.0f);
                
                visual_debug_ray_origin_direction[0] =
                    (clicked_viewport_x / projection_constants.x_multiplier) *
                        projection_constants.near;
                visual_debug_ray_origin_direction[1] =
                    (clicked_viewport_y / projection_constants.field_of_view_modifier) *
                        projection_constants.near;
                visual_debug_ray_origin_direction[2] =
                    projection_constants.near;
                visual_debug_ray_origin_direction[3] =
                    ((clicked_viewport_x /
                        projection_constants.x_multiplier) *
                            projection_constants.near) + 0.001f;
                visual_debug_ray_origin_direction[4] =
                    ((clicked_viewport_y /
                        projection_constants.field_of_view_modifier) *
                            projection_constants.near) + 0.001f;
                visual_debug_ray_origin_direction[5] =
                    projection_constants.near;
                visual_debug_ray_origin_direction[6] =
                    (clicked_viewport_x /
                        projection_constants.x_multiplier);
                visual_debug_ray_origin_direction[7] =
                    (clicked_viewport_y /
                        projection_constants.field_of_view_modifier);
                visual_debug_ray_origin_direction[8] = 1.0f;
            }
            
            if (i == 8) {
                visual_debug_highlight_touchable_id =
                    touchable_seekers[i]->touchable_id;
            }
            
            touchable_seekers[i]->checked_touchables = true;
            
            for (uint32_t j = i + 1; j < 11; j++) {
                if (
                    touchable_seekers[j]->checked_touchables ||
                    touchable_seekers[j]->handled)
                {
                    continue;
                }
                
                if (
                    touchable_seekers[i]->screen_x ==
                        touchable_seekers[j]->screen_x &&
                    touchable_seekers[i]->screen_y ==
                        touchable_seekers[j]->screen_y)
                {
                    touchable_seekers[j]->touchable_id =
                        touchable_seekers[i]->touchable_id;
                    touchable_seekers[j]->checked_touchables = true;
                }
            }
        }
        
        client_logic_update(elapsed);
    }
    
    camera_for_gpu->x = camera.x;
    camera_for_gpu->y = camera.y;
    camera_for_gpu->z = camera.z;
    camera_for_gpu->x_angle = camera.x_angle;
    camera_for_gpu->y_angle = camera.y_angle;
    camera_for_gpu->z_angle = camera.z_angle;
    
    log_assert(*vertices_for_gpu_size < 1);
    hardware_render(
        /* next_gpu_workload: */
            vertices_for_gpu,
        /* next_gpu_workload_size: */
            vertices_for_gpu_size,
        /* elapsed_microseconds: */
            elapsed);
    
    uint32_t overflow_vertices = *vertices_for_gpu_size % 3;
    *vertices_for_gpu_size -= overflow_vertices;
}
