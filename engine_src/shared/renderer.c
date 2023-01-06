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
    
    if (zpolygons_to_render_size == 0 && texquads_to_render_size == 0) {
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
                    zpolygons_to_render[zp_i].triangles[tri_i].normals[m].x;
                next_gpu_workload[*next_workload_size].normal_y =
                    zpolygons_to_render[zp_i].triangles[tri_i].normals[m].y;
                next_gpu_workload[*next_workload_size].normal_z =
                    zpolygons_to_render[zp_i].triangles[tri_i].normals[m].z;
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
    
    float left_uv_coord   = 0.0f;
    float right_uv_coord  = 1.0f;
    float bottom_uv_coord = 1.0f;
    float top_uv_coord    = 0.0f;
    
    for (uint32_t tq_i = 0; tq_i < texquads_to_render_size; tq_i++) {
        float offset_left =
            texquads_to_render[tq_i].left_x +
                texquads_to_render[tq_i].x_offset;
        float offset_right =
            offset_left + texquads_to_render[tq_i].width;
        float offset_mid_x = (offset_left + offset_right) / 2;
        
        float offset_top =
            texquads_to_render[tq_i].top_y +
                texquads_to_render[tq_i].y_offset;
        float offset_bottom = offset_top - texquads_to_render[tq_i].height;
        float offset_mid_y = (offset_top + offset_bottom) / 2;
        
        // top left vertex
        next_gpu_workload[*next_workload_size].x = offset_left - offset_mid_x;
        next_gpu_workload[*next_workload_size].y = offset_top - offset_mid_y;
        next_gpu_workload[*next_workload_size].z = 0.0f;
        next_gpu_workload[*next_workload_size].parent_x = offset_mid_x;
        next_gpu_workload[*next_workload_size].parent_y = offset_mid_y;
        next_gpu_workload[*next_workload_size].parent_z = texquads_to_render[tq_i].z;
        next_gpu_workload[*next_workload_size].normal_x = 0.0f;
        next_gpu_workload[*next_workload_size].normal_y = 0.0f;
        next_gpu_workload[*next_workload_size].normal_z = 1.0f;
        next_gpu_workload[*next_workload_size].x_angle = 0.0f;
        next_gpu_workload[*next_workload_size].y_angle = 0.0f;
        next_gpu_workload[*next_workload_size].z_angle =
            texquads_to_render[tq_i].z_angle;
        next_gpu_workload[*next_workload_size].texture_i =
            texquads_to_render[tq_i].texture_i;
        next_gpu_workload[*next_workload_size].texturearray_i =
            texquads_to_render[tq_i].texturearray_i;
        next_gpu_workload[*next_workload_size].uv[0] = left_uv_coord;
        next_gpu_workload[*next_workload_size].uv[1] = top_uv_coord;
        next_gpu_workload[*next_workload_size].RGBA[0] =
            texquads_to_render[tq_i].RGBA[0];
        next_gpu_workload[*next_workload_size].RGBA[1] =
            texquads_to_render[tq_i].RGBA[1];
        next_gpu_workload[*next_workload_size].RGBA[2] =
            texquads_to_render[tq_i].RGBA[2];
        next_gpu_workload[*next_workload_size].RGBA[3] =
            texquads_to_render[tq_i].RGBA[3];
        next_gpu_workload[*next_workload_size].ignore_lighting =
            texquads_to_render[tq_i].ignore_lighting;
        next_gpu_workload[*next_workload_size].scale_factor =
            texquads_to_render[tq_i].scale_factor;
        next_gpu_workload[*next_workload_size].ignore_camera =
            texquads_to_render[tq_i].ignore_camera;
        // top right vertex
        next_gpu_workload[*next_workload_size + 1].x = offset_right - offset_mid_x;
        next_gpu_workload[*next_workload_size + 1].y = offset_top - offset_mid_y;
        next_gpu_workload[*next_workload_size + 1].z = 0.0f;
        next_gpu_workload[*next_workload_size + 1].parent_x = offset_mid_x;
        next_gpu_workload[*next_workload_size + 1].parent_y = offset_mid_y;
        next_gpu_workload[*next_workload_size + 1].parent_z = texquads_to_render[tq_i].z;
        next_gpu_workload[*next_workload_size + 1].normal_x = 0.0f;
        next_gpu_workload[*next_workload_size + 1].normal_y = 0.0f;
        next_gpu_workload[*next_workload_size + 1].normal_z = 1.0f;
        next_gpu_workload[*next_workload_size + 1].x_angle = 0.0f;
        next_gpu_workload[*next_workload_size + 1].y_angle = 0.0f;
        next_gpu_workload[*next_workload_size + 1].z_angle =
            texquads_to_render[tq_i].z_angle;
        next_gpu_workload[*next_workload_size + 1].texture_i =
            texquads_to_render[tq_i].texture_i;
        next_gpu_workload[*next_workload_size + 1].texturearray_i =
            texquads_to_render[tq_i].texturearray_i;
        next_gpu_workload[*next_workload_size + 1].uv[0] = right_uv_coord;
        next_gpu_workload[*next_workload_size + 1].uv[1] = top_uv_coord;
        next_gpu_workload[*next_workload_size + 1].RGBA[0] =
            texquads_to_render[tq_i].RGBA[0];
        next_gpu_workload[*next_workload_size + 1].RGBA[1] =
            texquads_to_render[tq_i].RGBA[1];
        next_gpu_workload[*next_workload_size + 1].RGBA[2] =
            texquads_to_render[tq_i].RGBA[2];
        next_gpu_workload[*next_workload_size + 1].RGBA[3] =
            texquads_to_render[tq_i].RGBA[3];
        next_gpu_workload[*next_workload_size + 1].ignore_lighting =
            texquads_to_render[tq_i].ignore_lighting;
        next_gpu_workload[*next_workload_size + 1].scale_factor =
            texquads_to_render[tq_i].scale_factor;
        next_gpu_workload[*next_workload_size + 1].ignore_camera =
            texquads_to_render[tq_i].ignore_camera;
        // bottom left vertex
        next_gpu_workload[*next_workload_size + 2].x = offset_left - offset_mid_x;
        next_gpu_workload[*next_workload_size + 2].y = offset_bottom - offset_mid_y;
        next_gpu_workload[*next_workload_size + 2].z = 0.0f;
        next_gpu_workload[*next_workload_size + 2].parent_x = offset_mid_x;
        next_gpu_workload[*next_workload_size + 2].parent_y = offset_mid_y;
        next_gpu_workload[*next_workload_size + 2].parent_z = texquads_to_render[tq_i].z;
        next_gpu_workload[*next_workload_size + 2].normal_x = 0.0f;
        next_gpu_workload[*next_workload_size + 2].normal_y = 0.0f;
        next_gpu_workload[*next_workload_size + 2].normal_z = 1.0f;
        next_gpu_workload[*next_workload_size + 2].x_angle = 0.0f;
        next_gpu_workload[*next_workload_size + 2].y_angle = 0.0f;
        next_gpu_workload[*next_workload_size + 2].z_angle =
            texquads_to_render[tq_i].z_angle;
        next_gpu_workload[*next_workload_size + 2].texture_i =
            texquads_to_render[tq_i].texture_i;
        next_gpu_workload[*next_workload_size + 2].texturearray_i =
            texquads_to_render[tq_i].texturearray_i;
        next_gpu_workload[*next_workload_size + 2].uv[0] = left_uv_coord;
        next_gpu_workload[*next_workload_size + 2].uv[1] = bottom_uv_coord;
        next_gpu_workload[*next_workload_size + 2].RGBA[0] =
            texquads_to_render[tq_i].RGBA[0];
        next_gpu_workload[*next_workload_size + 2].RGBA[1] =
            texquads_to_render[tq_i].RGBA[1];
        next_gpu_workload[*next_workload_size + 2].RGBA[2] =
            texquads_to_render[tq_i].RGBA[2];
        next_gpu_workload[*next_workload_size + 2].RGBA[3] =
            texquads_to_render[tq_i].RGBA[3];
        next_gpu_workload[*next_workload_size + 2].ignore_lighting =
            texquads_to_render[tq_i].ignore_lighting;
        next_gpu_workload[*next_workload_size + 2].scale_factor =
            texquads_to_render[tq_i].scale_factor;
        next_gpu_workload[*next_workload_size + 2].ignore_camera =
            texquads_to_render[tq_i].ignore_camera;
        // top right vertex
        next_gpu_workload[*next_workload_size + 3].x = offset_right - offset_mid_x;
        next_gpu_workload[*next_workload_size + 3].y = offset_top - offset_mid_y;
        next_gpu_workload[*next_workload_size + 3].z = 0.0f;
        next_gpu_workload[*next_workload_size + 3].parent_x = offset_mid_x;
        next_gpu_workload[*next_workload_size + 3].parent_y = offset_mid_y;
        next_gpu_workload[*next_workload_size + 3].parent_z = texquads_to_render[tq_i].z;
        next_gpu_workload[*next_workload_size + 3].normal_x = 0.0f;
        next_gpu_workload[*next_workload_size + 3].normal_y = 0.0f;
        next_gpu_workload[*next_workload_size + 3].normal_z = 1.0f;
        next_gpu_workload[*next_workload_size + 3].x_angle = 0.0f;
        next_gpu_workload[*next_workload_size + 3].y_angle = 0.0f;
        next_gpu_workload[*next_workload_size + 3].z_angle =
            texquads_to_render[tq_i].z_angle;
        next_gpu_workload[*next_workload_size + 3].texture_i =
            texquads_to_render[tq_i].texture_i;
        next_gpu_workload[*next_workload_size + 3].texturearray_i =
            texquads_to_render[tq_i].texturearray_i;
        next_gpu_workload[*next_workload_size + 3].uv[0] = right_uv_coord;
        next_gpu_workload[*next_workload_size + 3].uv[1] = top_uv_coord;
        next_gpu_workload[*next_workload_size + 3].RGBA[0] =
            texquads_to_render[tq_i].RGBA[0];
        next_gpu_workload[*next_workload_size + 3].RGBA[1] =
            texquads_to_render[tq_i].RGBA[1];
        next_gpu_workload[*next_workload_size + 3].RGBA[2] =
            texquads_to_render[tq_i].RGBA[2];
        next_gpu_workload[*next_workload_size + 3].RGBA[3] =
            texquads_to_render[tq_i].RGBA[3];
        next_gpu_workload[*next_workload_size + 3].ignore_lighting =
            texquads_to_render[tq_i].ignore_lighting;
        next_gpu_workload[*next_workload_size + 3].scale_factor =
            texquads_to_render[tq_i].scale_factor;
        next_gpu_workload[*next_workload_size + 3].ignore_camera =
            texquads_to_render[tq_i].ignore_camera;
        // bottom right vertex
        next_gpu_workload[*next_workload_size + 4].x = offset_right - offset_mid_x;
        next_gpu_workload[*next_workload_size + 4].y = offset_bottom - offset_mid_y;
        next_gpu_workload[*next_workload_size + 4].z = 0.0f;
        next_gpu_workload[*next_workload_size + 4].parent_x = offset_mid_x;
        next_gpu_workload[*next_workload_size + 4].parent_y = offset_mid_y;
        next_gpu_workload[*next_workload_size + 4].parent_z = texquads_to_render[tq_i].z;
        next_gpu_workload[*next_workload_size + 4].normal_x = 0.0f;
        next_gpu_workload[*next_workload_size + 4].normal_y = 0.0f;
        next_gpu_workload[*next_workload_size + 4].normal_z = 1.0f;
        next_gpu_workload[*next_workload_size + 4].x_angle = 0.0f;
        next_gpu_workload[*next_workload_size + 4].y_angle = 0.0f;
        next_gpu_workload[*next_workload_size + 4].z_angle =
            texquads_to_render[tq_i].z_angle;
        next_gpu_workload[*next_workload_size + 4].texture_i =
            texquads_to_render[tq_i].texture_i;
        next_gpu_workload[*next_workload_size + 4].texturearray_i =
            texquads_to_render[tq_i].texturearray_i;
        next_gpu_workload[*next_workload_size + 4].uv[0] = right_uv_coord;
        next_gpu_workload[*next_workload_size + 4].uv[1] = bottom_uv_coord;
        next_gpu_workload[*next_workload_size + 4].RGBA[0] =
            texquads_to_render[tq_i].RGBA[0];
        next_gpu_workload[*next_workload_size + 4].RGBA[1] =
            texquads_to_render[tq_i].RGBA[1];
        next_gpu_workload[*next_workload_size + 4].RGBA[2] =
            texquads_to_render[tq_i].RGBA[2];
        next_gpu_workload[*next_workload_size + 4].RGBA[3] =
            texquads_to_render[tq_i].RGBA[3];
        next_gpu_workload[*next_workload_size + 4].ignore_lighting =
            texquads_to_render[tq_i].ignore_lighting;
        next_gpu_workload[*next_workload_size + 4].scale_factor =
            texquads_to_render[tq_i].scale_factor;
        next_gpu_workload[*next_workload_size + 4].ignore_camera =
            texquads_to_render[tq_i].ignore_camera;
        // bottom left vertex
        next_gpu_workload[*next_workload_size + 5].x = offset_left - offset_mid_x;
        next_gpu_workload[*next_workload_size + 5].y = offset_bottom - offset_mid_y;
        next_gpu_workload[*next_workload_size + 5].z = 0.0f;
        next_gpu_workload[*next_workload_size + 5].parent_x = offset_mid_x;
        next_gpu_workload[*next_workload_size + 5].parent_y = offset_mid_y;
        next_gpu_workload[*next_workload_size + 5].parent_z = texquads_to_render[tq_i].z;
        next_gpu_workload[*next_workload_size + 5].normal_x = 0.0f;
        next_gpu_workload[*next_workload_size + 5].normal_y = 0.0f;
        next_gpu_workload[*next_workload_size + 5].normal_z = 1.0f;
        next_gpu_workload[*next_workload_size + 5].x_angle = 0.0f;
        next_gpu_workload[*next_workload_size + 5].y_angle = 0.0f;
        next_gpu_workload[*next_workload_size + 5].z_angle =
            texquads_to_render[tq_i].z_angle;
        next_gpu_workload[*next_workload_size + 5].texture_i =
            texquads_to_render[tq_i].texture_i;
        next_gpu_workload[*next_workload_size + 5].texturearray_i =
            texquads_to_render[tq_i].texturearray_i;
        next_gpu_workload[*next_workload_size + 5].uv[0] = left_uv_coord;
        next_gpu_workload[*next_workload_size + 5].uv[1] = bottom_uv_coord;
        next_gpu_workload[*next_workload_size + 5].RGBA[0] =
            texquads_to_render[tq_i].RGBA[0];
        next_gpu_workload[*next_workload_size + 5].RGBA[1] =
            texquads_to_render[tq_i].RGBA[1];
        next_gpu_workload[*next_workload_size + 5].RGBA[2] =
            texquads_to_render[tq_i].RGBA[2];
        next_gpu_workload[*next_workload_size + 5].RGBA[3] =
            texquads_to_render[tq_i].RGBA[3];
        next_gpu_workload[*next_workload_size + 5].ignore_lighting =
            texquads_to_render[tq_i].ignore_lighting;
        next_gpu_workload[*next_workload_size + 5].scale_factor =
            texquads_to_render[tq_i].scale_factor;
        next_gpu_workload[*next_workload_size + 5].ignore_camera =
            texquads_to_render[tq_i].ignore_camera;
        
        *next_workload_size += 6;
    }
}
