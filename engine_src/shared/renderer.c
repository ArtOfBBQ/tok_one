#include "renderer.h"

static uint32_t renderer_initialized = false;

void init_renderer() {
    renderer_initialized = true;
    
    camera.x = 0.0f;
    camera.y = 0.0f;
    camera.z = 0.0f;
    camera.x_angle = 0.0f;
    camera.y_angle = 0.0f;
    camera.z_angle = 0.0f;
}

void hardware_render(
    GPU_Vertex * next_gpu_workload,
    uint32_t * next_workload_size,
    uint64_t elapsed_nanoseconds)
{
    if (renderer_initialized != true) {
        log_append("renderer not initialized, aborting...\n");
        return;
    }
    
    if (
        next_gpu_workload == NULL ||
        next_workload_size == NULL)
    {
        log_append("ERROR: platform layer didnt pass recipients\n");
        return;
    }
    
    if (zpolygons_to_render_size == 0) {
        return;
    }
    
    log_assert(zpolygons_to_render_size < ZPOLYGONS_TO_RENDER_ARRAYSIZE);
    
    for (uint32_t zp_i = 0; zp_i < zpolygons_to_render_size; zp_i++) {
        for (uint32_t tri_i = 0; tri_i < zpolygons_to_render[zp_i].triangles_size; tri_i++) {
            for (uint32_t m = 0; m < 3; m++) {
                next_gpu_workload[*next_workload_size].x =
                    zpolygons_to_render[zp_i].triangles[tri_i].vertices[m].x;
                next_gpu_workload[*next_workload_size].y =
                    zpolygons_to_render[zp_i].triangles[tri_i].vertices[m].y;
                next_gpu_workload[*next_workload_size].z =
                    zpolygons_to_render[zp_i].triangles[tri_i].vertices[m].z;
                next_gpu_workload[*next_workload_size].parent_x =
                    zpolygons_to_render[zp_i].x;
                next_gpu_workload[*next_workload_size].parent_y =
                    zpolygons_to_render[zp_i].y;
                next_gpu_workload[*next_workload_size].parent_z =
                    zpolygons_to_render[zp_i].z;
                next_gpu_workload[*next_workload_size].x_angle =
                    zpolygons_to_render[zp_i].x_angle;
                next_gpu_workload[*next_workload_size].y_angle =
                    zpolygons_to_render[zp_i].y_angle;
                next_gpu_workload[*next_workload_size].z_angle =
                    zpolygons_to_render[zp_i].z_angle;
                next_gpu_workload[*next_workload_size].normal_x =
                    zpolygons_to_render[zp_i].triangles[tri_i].normal.x;
                next_gpu_workload[*next_workload_size].normal_y =
                    zpolygons_to_render[zp_i].triangles[tri_i].normal.y;
                next_gpu_workload[*next_workload_size].normal_z =
                    zpolygons_to_render[zp_i].triangles[tri_i].normal.z;
                next_gpu_workload[*next_workload_size].RGBA[0] =
                    zpolygons_to_render[zp_i].triangles[tri_i].color[0];
                next_gpu_workload[*next_workload_size].RGBA[1] =
                    zpolygons_to_render[zp_i].triangles[tri_i].color[1];
                next_gpu_workload[*next_workload_size].RGBA[2] =
                    zpolygons_to_render[zp_i].triangles[tri_i].color[2];
                next_gpu_workload[*next_workload_size].RGBA[3] =
                    zpolygons_to_render[zp_i].triangles[tri_i].color[3];
                next_gpu_workload[*next_workload_size].touchable_id =
                    zpolygons_to_render[zp_i].touchable_id;
                next_gpu_workload[*next_workload_size].texture_i =
                    zpolygons_to_render[zp_i].triangles[tri_i].texture_i;
                next_gpu_workload[*next_workload_size].texturearray_i =
                    zpolygons_to_render[zp_i].triangles[tri_i].texturearray_i;
                next_gpu_workload[*next_workload_size].uv[0] =
                    zpolygons_to_render[zp_i].triangles[tri_i].vertices[m].uv[0];
                next_gpu_workload[*next_workload_size].uv[1] =
                    zpolygons_to_render[zp_i].triangles[tri_i].vertices[m].uv[1];
                next_gpu_workload[*next_workload_size].ignore_lighting =
                    zpolygons_to_render[zp_i].ignore_lighting;
                next_gpu_workload[*next_workload_size].scale_factor =
                    zpolygons_to_render[zp_i].scale_factor;
                next_gpu_workload[*next_workload_size].ignore_camera =
                    zpolygons_to_render[zp_i].ignore_camera;
                
                *next_workload_size += 1;
                assert(*next_workload_size - 1 < MAX_VERTICES_PER_BUFFER);
            }
        }
    }    
}
