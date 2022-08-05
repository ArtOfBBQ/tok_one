#include "gameloop.h"

static uint64_t previous_time = 0;
static uint64_t frame_no = 0;

void shared_gameloop_update(
    Vertex * vertices_for_gpu,
    uint32_t * vertices_for_gpu_size)
{
    frame_no++;
    uint64_t time = platform_get_current_time_microsecs();
    uint64_t elapsed = time - previous_time;
    if (previous_time < 1) {
        previous_time = time;
        return;
    }
    previous_time = time;
    
    if (elapsed > 175000) {
        log_append("Error: elapsed time was: ");
        log_append_uint(elapsed);
        log_append(" microseconds at frame: ");
        log_append_uint(frame_no);
        log_append("\n");
        log_assert(elapsed < 175000);
    }
    
    if (
        time - last_resize_request_at < 1500000
        && !request_post_resize_clearscreen)
    {
        if (time - last_resize_request_at < 300000) {
            return;
        }
        
        client_logic_window_resize(
            window_height,
            window_width);
        last_resize_request_at = 999999999;
    }
    
    touchable_triangles_size = 0;
    
    resolve_animation_effects(elapsed);
    clean_deleted_lights();
    clean_deleted_texquads();
    
    // translate all lights
    translate_lights();
    
    if (!application_running) {
        texquads_to_render_size = 0;
        zpolygons_to_render_size = 0;
        camera = {0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f};
        
        font_height = 28.0f;
        font_ignore_lighting = true;
        font_color[0] = 0.8f;
        font_color[1] = 0.8f;
        font_color[2] = 1.0f;
        font_color[3] = 1.0f;
        
        request_label_renderable(
            /* const uint32_t with_object_id: */
                0,
            /* const char * text_to_draw: */
                assert_failed_message,
            /* const float left_pixelspace: */
                10,
            /* const float top_pixelspace: */
                window_height - 20,
            /* const float z: */
                0.5f,
            /* const float max_width: */
                window_width - 20,
            /* const bool32_t ignore_camera: */
                true);
    } else {
        client_logic_update(elapsed);
        
        software_render(
            /* next_gpu_workload: */
                vertices_for_gpu,
            /* next_gpu_workload_size: */
                vertices_for_gpu_size,
            /* zlights_transformed: */
                zlights_transformed,
            /* elapsed_microseconds: */
                elapsed);
    }
    
    draw_texquads_to_render(
        /* next_gpu_workload: */
            vertices_for_gpu,
        /* next_gpu_workload_size: */
            vertices_for_gpu_size,
        /* zlights_transformed: */
            zlights_transformed);
}

