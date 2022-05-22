// functions we must implement
#include "../shared/platform_layer.h" 

// shared functionality we can use
#include "../shared/common.h"
#include "../shared/userinput.h"
#include "../shared/window_size.h"
#include "../shared/vertex_types.h"
#include "../shared/zpolygon.h"
#include "../shared/software_renderer.h"
#include "../shared/bitmap_renderer.h"
#include "../shared/clientlogic.h"

int main(int argc, const char * argv[]) 
{
    window_height = 750;
    window_width = 1200;
    bool32_t gameloop = 5000;
    
    init_projection_constants();
    init_renderer();

    Vertex * vertices_for_gpu = malloc(sizeof(Vertex) * 5000);
    
    while (gameloop > 0) {
        
        printf("gameloop: %u\n", gameloop);
        
        // uint64_t time = platform_get_current_time_microsecs();
        // uint64_t elapsed = time - previous_time;
        uint64_t elapsed = 7500;

        for (uint32_t i = 0; i < texture_arrays_size; i++) {
            if (texture_arrays[i].request_update) {
                // [self updateTextureArray: i];
                break;
            }
        }
        
        uint32_t vertices_for_gpu_size = 0;

        // translate all lights
        zLightSource zlights_transformed[zlights_to_apply_size];
        translate_lights(
            /* originals: */ &zlights_to_apply[0],
            /* out_translated: */ &zlights_transformed[0],
            /* lights_count: */ zlights_to_apply_size);
        
        software_render(
            /* next_gpu_workload: */
                vertices_for_gpu,
            /* next_gpu_workload_size: */
                &vertices_for_gpu_size,
            /* zlights_transformed: */
                zlights_transformed,
            /* elapsed_microseconds: */
                elapsed);
        
        draw_texquads_to_render(
            /* next_gpu_workload: */
                vertices_for_gpu,
            /* next_gpu_workload_size: */
                &vertices_for_gpu_size,
            /* zlights_transformed: */
                zlights_transformed);

        printf(
            "if there was a UI we'd draw %u vertices\n",
            vertices_for_gpu_size);
        
        gameloop--;
    }
    
    printf("end of 5000 gameloops\n");
}

