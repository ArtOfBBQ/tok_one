#include "gameloop.h"

static uint64_t previous_time = 0;
static uint64_t frame_no = 0;

void shared_gameloop_update(
    Vertex * vertices_for_gpu,
    uint32_t * vertices_for_gpu_size,
    LightCollection * lights_for_gpu)
{
    frame_no++;
    uint64_t time = platform_get_current_time_microsecs();
    uint64_t elapsed = time - previous_time;
    if (previous_time < 1) {
        previous_time = time;
        return;
    }
    previous_time = time;
    
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
    
    touchable_triangles_size = 0;
    
    resolve_animation_effects(elapsed);
    clean_deleted_lights();
    clean_deleted_texquads();
    
    translate_lights(
        lights_for_gpu);
    assert(lights_for_gpu->lights_size > 0);
    
    if (!application_running) {
        texquads_to_render_size = 0;
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
        client_logic_update(elapsed);
    }
    
    log_assert(*vertices_for_gpu_size < 1);
    software_render(
        /* next_gpu_workload: */
            vertices_for_gpu,
        /* next_gpu_workload_size: */
            vertices_for_gpu_size,
        /* elapsed_microseconds: */
            elapsed);
    
    uint32_t overflow_vertices = *vertices_for_gpu_size % 3;
    *vertices_for_gpu_size -= overflow_vertices;
    
    for (
        uint32_t triangle_start_i = 0;
        triangle_start_i < *vertices_for_gpu_size;
        triangle_start_i += 3)
    {
        register_touchable_triangle(vertices_for_gpu + triangle_start_i);
    }
}
