#include "gameloop.h"

static uint64_t previous_time = 0;
static uint64_t frame_no = 0;

static int32_t closest_touchable_from_screen_ray(
    const float screen_x,
    const float screen_y,
    zVertex * collision_point)
{
    uint32_t nonzero_camera_angles = 0;
    if (camera.x_angle != 0.0f) { nonzero_camera_angles += 1; }
    if (camera.y_angle != 0.0f) { nonzero_camera_angles += 1; }
    if (camera.z_angle != 0.0f) { nonzero_camera_angles += 1; }
    
    // TODO: study raytracing math and find out why our solution doesn't
    // TODO: support 2 or more camera rotations  
    log_assert(nonzero_camera_angles < 2);
    
    float clicked_viewport_x =
        -1.0f + ((screen_x / window_width) * 2.0f);
    float clicked_viewport_y =
        -1.0f + ((screen_y / window_height) * 2.0f);
    
    float z_multiplier = 0.00001f;
    zVertex ray_origin;
    ray_origin.x =
        (clicked_viewport_x / projection_constants.x_multiplier) *
            z_multiplier;
    ray_origin.y =
        ((clicked_viewport_y) / projection_constants.field_of_view_modifier) *
            z_multiplier;
    ray_origin.z = z_multiplier;
    
    zVertex ray_origin_rotated = ray_origin;
    ray_origin_rotated = x_rotate_zvertex(
        &ray_origin_rotated, camera.x_angle);
    ray_origin_rotated = y_rotate_zvertex(
        &ray_origin_rotated, camera.y_angle);
    ray_origin_rotated = z_rotate_zvertex(
        &ray_origin_rotated, camera.z_angle);
    
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
    float distant_z = 10.0f;
    zVertex distant_point;
    distant_point.x =
        (clicked_viewport_x / projection_constants.x_multiplier) *
            distant_z;
    distant_point.y =
        (clicked_viewport_y / projection_constants.field_of_view_modifier) *
            distant_z;
    distant_point.z = distant_z;
    
    
    zVertex distant_point_rotated = distant_point;
    distant_point_rotated = x_rotate_zvertex(&distant_point_rotated, camera.x_angle);
    distant_point_rotated = y_rotate_zvertex(&distant_point_rotated, camera.y_angle);
    distant_point_rotated = z_rotate_zvertex(&distant_point_rotated, camera.z_angle);
    
    visual_debug_ray_origin_direction[0] =
        ray_origin.x;
    visual_debug_ray_origin_direction[1] =
        ray_origin.y;
    visual_debug_ray_origin_direction[2] =
        ray_origin.z;
    visual_debug_ray_origin_direction[3] =
        ray_origin.x + 0.03f;
    visual_debug_ray_origin_direction[4] =
        ray_origin.y + 0.03f;
    visual_debug_ray_origin_direction[5] =
        ray_origin.z + 0.03f;
    visual_debug_ray_origin_direction[6] =
        ray_origin.x + (distant_point_rotated.x * 1.0f);
    visual_debug_ray_origin_direction[7] =
        ray_origin.y + (distant_point_rotated.y * 1.0f);
    visual_debug_ray_origin_direction[8] =
        ray_origin.z + (distant_point_rotated.z * 1.0f);
    
    normalize_zvertex(&distant_point);
    normalize_zvertex(&distant_point_rotated);
    
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
        zPolygon offset_polygon = zpolygons_to_render[zp_i];
        float camera_offset_x = camera.x *
            !zpolygons_to_render[zp_i].ignore_camera;
        float camera_offset_y = camera.y *
            !zpolygons_to_render[zp_i].ignore_camera;
        float camera_offset_z = camera.z *
            !zpolygons_to_render[zp_i].ignore_camera;
        
        offset_polygon.x -= camera_offset_x;
        offset_polygon.y -= camera_offset_y;
        offset_polygon.z -= camera_offset_z;
        hit = ray_intersects_zpolygon_hitbox(
            offset_polygon.ignore_camera ?
                &ray_origin : &ray_origin_rotated,
            offset_polygon.ignore_camera ?
                &distant_point : &distant_point_rotated,
            &offset_polygon,
            &current_collision_point);
        
        zVertex offset_collision_point = current_collision_point;
        offset_collision_point.x += camera_offset_x;
        offset_collision_point.y += camera_offset_y;
        offset_collision_point.z += camera_offset_z;
        
        float dist_to_hit = get_distance(offset_collision_point, ray_origin);
        if (hit && dist_to_hit < smallest_dist) {
            smallest_dist = dist_to_hit;
            return_value = offset_polygon.touchable_id;
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
        
        for (uint32_t i = 0; i < 8; i++) {
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
            
            break; // max 1 ray per frame
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
