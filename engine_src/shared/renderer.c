#include "renderer.h"

static uint32_t renderer_initialized = false;

void init_renderer() {
    renderer_initialized = true;
    
    camera.x       = 0.0f;
    camera.y       = 0.0f;
    camera.z       = 0.0f;
    camera.x_angle = 0.0f;
    camera.y_angle = 0.0f;
    camera.z_angle = 0.0f;
}

inline static void zpolygons_to_triangles(
    GPUVertex * next_gpu_workload,
    uint32_t * next_workload_size)
{    
    for (
        uint32_t zp_i = 0;
        zp_i < zpolygons_to_render_size;
        zp_i++)
    {
        if (
            zpolygons_to_render[zp_i].deleted ||
            !zpolygons_to_render[zp_i].visible ||
            zpolygons_to_render[zp_i].mesh_id < 0 ||
            (uint32_t)zpolygons_to_render[zp_i].mesh_id >=
                all_mesh_summaries_size)
        {
            continue;
        }
        
        int32_t mesh_id = zpolygons_to_render[zp_i].mesh_id;
        log_assert(mesh_id >= 0);
        log_assert(mesh_id < (int32_t)all_mesh_summaries_size);
        
        int32_t tail_i =
            all_mesh_summaries[mesh_id].triangles_head_i +
                all_mesh_summaries[mesh_id].triangles_size;
        
        for (
            int32_t tri_i = all_mesh_summaries[mesh_id].triangles_head_i;
            tri_i < tail_i;
            tri_i++)
        {
            int32_t material_i = all_mesh_triangles[tri_i].parent_material_i;
            
            next_gpu_workload[*next_workload_size + 0].parent_x =
                zpolygons_to_render[zp_i].x;
            next_gpu_workload[*next_workload_size + 0].parent_y =
                zpolygons_to_render[zp_i].y;
            next_gpu_workload[*next_workload_size + 0].parent_z =
                zpolygons_to_render[zp_i].z;
            next_gpu_workload[*next_workload_size + 0].x_angle =
                zpolygons_to_render[zp_i].x_angle;
            next_gpu_workload[*next_workload_size + 0].y_angle =
                zpolygons_to_render[zp_i].y_angle;
            next_gpu_workload[*next_workload_size + 0].z_angle =
                zpolygons_to_render[zp_i].z_angle;
            next_gpu_workload[*next_workload_size + 0].normal_x =
                all_mesh_triangles[tri_i].normal.x;
            next_gpu_workload[*next_workload_size + 0].normal_y =
                all_mesh_triangles[tri_i].normal.y;
            next_gpu_workload[*next_workload_size + 0].normal_z =
                all_mesh_triangles[tri_i].normal.z;
            next_gpu_workload[*next_workload_size + 0].touchable_id =
                zpolygons_to_render[zp_i].touchable_id;
            next_gpu_workload[*next_workload_size + 0].texture_i =
                zpolygons_to_render[zp_i].
                    triangle_materials[material_i].texture_i;
            next_gpu_workload[*next_workload_size + 0].texturearray_i =
                zpolygons_to_render[zp_i].
                    triangle_materials[material_i].texturearray_i;
            next_gpu_workload[*next_workload_size + 0].ignore_lighting =
                zpolygons_to_render[zp_i].ignore_lighting;
            next_gpu_workload[*next_workload_size + 0].scale_factor =
                zpolygons_to_render[zp_i].scale_factor;
            next_gpu_workload[*next_workload_size + 0].ignore_camera =
                zpolygons_to_render[zp_i].ignore_camera;
            next_gpu_workload[*next_workload_size + 0].RGBA[0] =
                zpolygons_to_render[zp_i].
                    triangle_materials[material_i].color[0] +
                        zpolygons_to_render[zp_i].rgb_bonus[0];
            next_gpu_workload[*next_workload_size + 0].RGBA[1] =
                zpolygons_to_render[zp_i].
                    triangle_materials[material_i].color[1] +
                        zpolygons_to_render[zp_i].rgb_bonus[1];
            next_gpu_workload[*next_workload_size + 0].RGBA[2] =
                zpolygons_to_render[zp_i].
                    triangle_materials[material_i].color[2] +
                        zpolygons_to_render[zp_i].rgb_bonus[2];
            next_gpu_workload[*next_workload_size + 0].RGBA[3] =
                zpolygons_to_render[zp_i].
                    triangle_materials[material_i].color[3];
            next_gpu_workload[*next_workload_size + 0].x =
                    (all_mesh_triangles[tri_i].vertices[0].x *
                        zpolygons_to_render[zp_i].x_multiplier) +
                    zpolygons_to_render[zp_i].x_offset;
            next_gpu_workload[*next_workload_size + 0].y =
                (all_mesh_triangles[tri_i].vertices[0].y *
                    zpolygons_to_render[zp_i].y_multiplier) +
                zpolygons_to_render[zp_i].y_offset;
            next_gpu_workload[*next_workload_size + 0].z =
                (all_mesh_triangles[tri_i].vertices[0].z *
                    zpolygons_to_render[zp_i].z_multiplier);
            next_gpu_workload[*next_workload_size + 0].uv[0] =
                all_mesh_triangles[tri_i].
                    vertices[0].uv[0];
            next_gpu_workload[*next_workload_size + 0].uv[1] =
                all_mesh_triangles[tri_i].
                    vertices[0].uv[1];
            
            next_gpu_workload[*next_workload_size + 1].parent_x =
                zpolygons_to_render[zp_i].x;
            next_gpu_workload[*next_workload_size + 1].parent_y =
                zpolygons_to_render[zp_i].y;
            next_gpu_workload[*next_workload_size + 1].parent_z =
                zpolygons_to_render[zp_i].z;
            next_gpu_workload[*next_workload_size + 1].x_angle =
                zpolygons_to_render[zp_i].x_angle;
            next_gpu_workload[*next_workload_size + 1].y_angle =
                zpolygons_to_render[zp_i].y_angle;
            next_gpu_workload[*next_workload_size + 1].z_angle =
                zpolygons_to_render[zp_i].z_angle;
            next_gpu_workload[*next_workload_size + 1].normal_x =
                all_mesh_triangles[tri_i].normal.x;
            next_gpu_workload[*next_workload_size + 1].normal_y =
                all_mesh_triangles[tri_i].normal.y;
            next_gpu_workload[*next_workload_size + 1].normal_z =
                all_mesh_triangles[tri_i].normal.z;
            next_gpu_workload[*next_workload_size + 1].touchable_id =
                zpolygons_to_render[zp_i].touchable_id;
            next_gpu_workload[*next_workload_size + 1].texture_i =
                zpolygons_to_render[zp_i].
                    triangle_materials[material_i].texture_i;
            next_gpu_workload[*next_workload_size + 1].texturearray_i =
                zpolygons_to_render[zp_i].
                    triangle_materials[material_i].texturearray_i;
            next_gpu_workload[*next_workload_size + 1].ignore_lighting =
                zpolygons_to_render[zp_i].ignore_lighting;
            next_gpu_workload[*next_workload_size + 1].scale_factor =
                zpolygons_to_render[zp_i].scale_factor;
            next_gpu_workload[*next_workload_size + 1].ignore_camera =
                zpolygons_to_render[zp_i].ignore_camera;
            next_gpu_workload[*next_workload_size + 1].RGBA[0] =
                zpolygons_to_render[zp_i].
                    triangle_materials[material_i].color[0] +
                        zpolygons_to_render[zp_i].rgb_bonus[0];
            next_gpu_workload[*next_workload_size + 1].RGBA[1] =
                zpolygons_to_render[zp_i].
                    triangle_materials[material_i].color[1] +
                        zpolygons_to_render[zp_i].rgb_bonus[1];
            next_gpu_workload[*next_workload_size + 1].RGBA[2] =
                zpolygons_to_render[zp_i].
                    triangle_materials[material_i].color[2] +
                        zpolygons_to_render[zp_i].rgb_bonus[2];
            next_gpu_workload[*next_workload_size + 1].RGBA[3] =
                zpolygons_to_render[zp_i].
                    triangle_materials[material_i].color[3];
            next_gpu_workload[*next_workload_size + 1].x =
                    (all_mesh_triangles[tri_i].vertices[1].x *
                        zpolygons_to_render[zp_i].x_multiplier) +
                    zpolygons_to_render[zp_i].x_offset;
            next_gpu_workload[*next_workload_size + 1].y =
                (all_mesh_triangles[tri_i].vertices[1].y *
                    zpolygons_to_render[zp_i].y_multiplier) +
                zpolygons_to_render[zp_i].y_offset;
            next_gpu_workload[*next_workload_size + 1].z =
                (all_mesh_triangles[tri_i].vertices[1].z *
                    zpolygons_to_render[zp_i].z_multiplier);
            next_gpu_workload[*next_workload_size + 1].uv[0] =
                all_mesh_triangles[tri_i].
                    vertices[1].uv[0];
            next_gpu_workload[*next_workload_size + 1].uv[1] =
                all_mesh_triangles[tri_i].
                    vertices[1].uv[1];
            
            next_gpu_workload[*next_workload_size + 2].parent_x =
                zpolygons_to_render[zp_i].x;
            next_gpu_workload[*next_workload_size + 2].parent_y =
                zpolygons_to_render[zp_i].y;
            next_gpu_workload[*next_workload_size + 2].parent_z =
                zpolygons_to_render[zp_i].z;
            next_gpu_workload[*next_workload_size + 2].x_angle =
                zpolygons_to_render[zp_i].x_angle;
            next_gpu_workload[*next_workload_size + 2].y_angle =
                zpolygons_to_render[zp_i].y_angle;
            next_gpu_workload[*next_workload_size + 2].z_angle =
                zpolygons_to_render[zp_i].z_angle;
            next_gpu_workload[*next_workload_size + 2].normal_x =
                all_mesh_triangles[tri_i].normal.x;
            next_gpu_workload[*next_workload_size + 2].normal_y =
                all_mesh_triangles[tri_i].normal.y;
            next_gpu_workload[*next_workload_size + 2].normal_z =
                all_mesh_triangles[tri_i].normal.z;
            next_gpu_workload[*next_workload_size + 2].touchable_id =
                zpolygons_to_render[zp_i].touchable_id;
            next_gpu_workload[*next_workload_size + 2].texture_i =
                zpolygons_to_render[zp_i].
                    triangle_materials[material_i].texture_i;
            next_gpu_workload[*next_workload_size + 2].texturearray_i =
                zpolygons_to_render[zp_i].
                    triangle_materials[material_i].texturearray_i;
            next_gpu_workload[*next_workload_size + 2].ignore_lighting =
                zpolygons_to_render[zp_i].ignore_lighting;
            next_gpu_workload[*next_workload_size + 2].scale_factor =
                zpolygons_to_render[zp_i].scale_factor;
            next_gpu_workload[*next_workload_size + 2].ignore_camera =
                zpolygons_to_render[zp_i].ignore_camera;
            next_gpu_workload[*next_workload_size + 2].RGBA[0] =
                zpolygons_to_render[zp_i].
                    triangle_materials[material_i].color[0] +
                        zpolygons_to_render[zp_i].rgb_bonus[0];
            next_gpu_workload[*next_workload_size + 2].RGBA[1] =
                zpolygons_to_render[zp_i].
                    triangle_materials[material_i].color[1] +
                        zpolygons_to_render[zp_i].rgb_bonus[1];
            next_gpu_workload[*next_workload_size + 2].RGBA[2] =
                zpolygons_to_render[zp_i].
                    triangle_materials[material_i].color[2] +
                        zpolygons_to_render[zp_i].rgb_bonus[2];
            next_gpu_workload[*next_workload_size + 2].RGBA[3] =
                zpolygons_to_render[zp_i].
                    triangle_materials[material_i].color[3];
            next_gpu_workload[*next_workload_size + 2].x =
                    (all_mesh_triangles[tri_i].vertices[2].x *
                        zpolygons_to_render[zp_i].x_multiplier) +
                    zpolygons_to_render[zp_i].x_offset;
            next_gpu_workload[*next_workload_size + 2].y =
                (all_mesh_triangles[tri_i].vertices[2].y *
                    zpolygons_to_render[zp_i].y_multiplier) +
                zpolygons_to_render[zp_i].y_offset;
            next_gpu_workload[*next_workload_size + 2].z =
                (all_mesh_triangles[tri_i].vertices[2].z *
                    zpolygons_to_render[zp_i].z_multiplier);
            next_gpu_workload[*next_workload_size + 2].uv[0] =
                all_mesh_triangles[tri_i].
                    vertices[2].uv[0];
            next_gpu_workload[*next_workload_size + 2].uv[1] =
                all_mesh_triangles[tri_i].
                    vertices[2].uv[1];
            
            *next_workload_size += 3;
            log_assert(*next_workload_size <= MAX_VERTICES_PER_BUFFER);
        }
        log_assert(*next_workload_size <= MAX_VERTICES_PER_BUFFER);
        
        // draw touchable hitboxes (the yellow lines) in visual debug mode
        if (
            window_globals->visual_debug_mode &&
            zpolygons_to_render[zp_i].touchable_id >= 0)
        {
            float hitbox_left   =
                -(zpolygons_to_render[zp_i].hitbox_width / 2) +
                zpolygons_to_render[zp_i].x_offset;
            float hitbox_right  =
                (zpolygons_to_render[zp_i].hitbox_width / 2) +
                zpolygons_to_render[zp_i].x_offset;
            float hitbox_top    =
                (zpolygons_to_render[zp_i].hitbox_height / 2) +
                zpolygons_to_render[zp_i].y_offset;
            float hitbox_bottom =
                -(zpolygons_to_render[zp_i].hitbox_height / 2) +
                zpolygons_to_render[zp_i].y_offset;
            float hitbox_front  = -(zpolygons_to_render[zp_i].hitbox_depth / 2);
            float hitbox_back   = (zpolygons_to_render[zp_i].hitbox_depth / 2);

            // triangle 1:
            // top left front to top right front
            next_gpu_workload[*next_workload_size].x = hitbox_left;
            next_gpu_workload[*next_workload_size].y = hitbox_top;
            next_gpu_workload[*next_workload_size].z = hitbox_front;
            // top right front
            next_gpu_workload[*next_workload_size + 1].x = hitbox_right;
            next_gpu_workload[*next_workload_size + 1].y = hitbox_top;
            next_gpu_workload[*next_workload_size + 1].z = hitbox_front;
            // top right front
            next_gpu_workload[*next_workload_size + 2].x = hitbox_right;
            next_gpu_workload[*next_workload_size + 2].y = hitbox_top;
            next_gpu_workload[*next_workload_size + 2].z = hitbox_front + 0.01f;
            // triangle 2:
            // bottom left front to bottom right front
            next_gpu_workload[*next_workload_size + 3].x = hitbox_left;
            next_gpu_workload[*next_workload_size + 3].y = hitbox_bottom;
            next_gpu_workload[*next_workload_size + 3].z = hitbox_front;
            next_gpu_workload[*next_workload_size + 4].x = hitbox_right;
            next_gpu_workload[*next_workload_size + 4].y = hitbox_bottom;
            next_gpu_workload[*next_workload_size + 4].z = hitbox_front;
            next_gpu_workload[*next_workload_size + 5].x = hitbox_right;
            next_gpu_workload[*next_workload_size + 5].y = hitbox_bottom;
            next_gpu_workload[*next_workload_size + 5].z = hitbox_front + 0.01f;
            // triangle 3:
            // top left back to top right back
            next_gpu_workload[*next_workload_size + 6].x = hitbox_left;
            next_gpu_workload[*next_workload_size + 6].y = hitbox_top;
            next_gpu_workload[*next_workload_size + 6].z = hitbox_back;
            next_gpu_workload[*next_workload_size + 7].x = hitbox_right;
            next_gpu_workload[*next_workload_size + 7].y = hitbox_top;
            next_gpu_workload[*next_workload_size + 7].z = hitbox_back;
            next_gpu_workload[*next_workload_size + 8].x = hitbox_right;
            next_gpu_workload[*next_workload_size + 8].y = hitbox_top;
            next_gpu_workload[*next_workload_size + 8].z = hitbox_back + 0.01f;
            // triangle 4:
            next_gpu_workload[*next_workload_size + 9].x = hitbox_left;
            next_gpu_workload[*next_workload_size + 9].y = hitbox_bottom;
            next_gpu_workload[*next_workload_size + 9].z = hitbox_back;
            next_gpu_workload[*next_workload_size + 10].x = hitbox_right;
            next_gpu_workload[*next_workload_size + 10].y = hitbox_bottom;
            next_gpu_workload[*next_workload_size + 10].z = hitbox_back;
            next_gpu_workload[*next_workload_size + 11].x = hitbox_right;
            next_gpu_workload[*next_workload_size + 11].y = hitbox_bottom;
            next_gpu_workload[*next_workload_size + 11].z = hitbox_back + 0.01f;
            // triangle 5:
            // top left front to bottom left front
            next_gpu_workload[*next_workload_size + 12].x = hitbox_left;
            next_gpu_workload[*next_workload_size + 12].y = hitbox_top;
            next_gpu_workload[*next_workload_size + 12].z = hitbox_front;
            next_gpu_workload[*next_workload_size + 13].x = hitbox_left;
            next_gpu_workload[*next_workload_size + 13].y = hitbox_bottom;
            next_gpu_workload[*next_workload_size + 13].z = hitbox_front;
            next_gpu_workload[*next_workload_size + 14].x = hitbox_left + 0.01f;
            next_gpu_workload[*next_workload_size + 14].y = hitbox_bottom;
            next_gpu_workload[*next_workload_size + 14].z = hitbox_front;
            // triangle 6:
            // top right front to bottom right front
            next_gpu_workload[*next_workload_size + 15].x = hitbox_right;
            next_gpu_workload[*next_workload_size + 15].y = hitbox_top;
            next_gpu_workload[*next_workload_size + 15].z = hitbox_front;
            next_gpu_workload[*next_workload_size + 16].x = hitbox_right;
            next_gpu_workload[*next_workload_size + 16].y = hitbox_bottom;
            next_gpu_workload[*next_workload_size + 16].z = hitbox_front;
            next_gpu_workload[*next_workload_size + 17].x = hitbox_right + 0.01f;
            next_gpu_workload[*next_workload_size + 17].y = hitbox_bottom;
            next_gpu_workload[*next_workload_size + 17].z = hitbox_front;
            // triangle 7:
            // top left back to bottom left back
            next_gpu_workload[*next_workload_size + 18].x = hitbox_left;
            next_gpu_workload[*next_workload_size + 18].y = hitbox_top;
            next_gpu_workload[*next_workload_size + 18].z = hitbox_back;
            next_gpu_workload[*next_workload_size + 19].x = hitbox_left;
            next_gpu_workload[*next_workload_size + 19].y = hitbox_bottom;
            next_gpu_workload[*next_workload_size + 19].z = hitbox_back;
            next_gpu_workload[*next_workload_size + 20].x = hitbox_left + 0.01f;
            next_gpu_workload[*next_workload_size + 20].y = hitbox_bottom;
            next_gpu_workload[*next_workload_size + 20].z = hitbox_back;
            // triangle 8:
            // top right back to bottom right back
            next_gpu_workload[*next_workload_size + 21].x = hitbox_right;
            next_gpu_workload[*next_workload_size + 21].y = hitbox_top;
            next_gpu_workload[*next_workload_size + 21].z = hitbox_back;
            next_gpu_workload[*next_workload_size + 22].x = hitbox_right;
            next_gpu_workload[*next_workload_size + 22].y = hitbox_bottom;
            next_gpu_workload[*next_workload_size + 22].z = hitbox_back;
            next_gpu_workload[*next_workload_size + 23].x = hitbox_right + 0.01f;
            next_gpu_workload[*next_workload_size + 23].y = hitbox_bottom;
            next_gpu_workload[*next_workload_size + 23].z = hitbox_back;

            for (uint32_t m = 0; m < 24; m++) {
                uint32_t next_wl_i = *next_workload_size + m;
                next_gpu_workload[next_wl_i].parent_x =
                    zpolygons_to_render[zp_i].x;
                next_gpu_workload[next_wl_i].parent_y =
                    zpolygons_to_render[zp_i].y;
                next_gpu_workload[next_wl_i].parent_z =
                    zpolygons_to_render[zp_i].z;
                next_gpu_workload[next_wl_i].texturearray_i = -1;
                next_gpu_workload[next_wl_i].texture_i = -1;
                next_gpu_workload[next_wl_i].RGBA[0] = 0.8f;
                next_gpu_workload[next_wl_i].RGBA[1] = 0.8f +
                    ((zpolygons_to_render[zp_i].touchable_id ==
                        window_globals->visual_debug_highlight_touchable_id) *
                            0.2f);
                next_gpu_workload[next_wl_i].RGBA[2] = 0.4f +
                    ((zpolygons_to_render[zp_i].touchable_id ==
                        window_globals->visual_debug_highlight_touchable_id) *
                            0.4f);
                next_gpu_workload[next_wl_i].RGBA[3] = 1.0f;
                next_gpu_workload[next_wl_i].ignore_lighting =
                    true;
                next_gpu_workload[next_wl_i].scale_factor = 1.0f;
                next_gpu_workload[next_wl_i].ignore_camera =
                    zpolygons_to_render[zp_i].ignore_camera;
                next_gpu_workload[next_wl_i].touchable_id = -1;
                next_gpu_workload[next_wl_i].x_angle =
                    zpolygons_to_render[zp_i].x_angle;
                next_gpu_workload[next_wl_i].y_angle =
                    zpolygons_to_render[zp_i].y_angle;
                next_gpu_workload[next_wl_i].z_angle =
                    zpolygons_to_render[zp_i].z_angle;
            }

            *next_workload_size += 24;
            log_assert(*next_workload_size - 1 < MAX_VERTICES_PER_BUFFER);
        }
    }
}

void hardware_render(
    GPUVertex * next_gpu_workload,
    uint32_t * next_workload_size,
    GPULightCollection * lights_for_gpu,
    uint64_t elapsed_nanoseconds)
{
    (void)elapsed_nanoseconds;
    
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
    
    if (window_globals->visual_debug_mode) {
        /*
        draw the ray that's used for finding touchables when clicking as a
        triangle
        */
        for (uint32_t m = 0; m < 3; m++) {
            uint32_t next_wl_i = *next_workload_size + m;
            
            next_gpu_workload[next_wl_i].x =
                window_globals->visual_debug_ray_origin_direction[(m*3) + 0];
            next_gpu_workload[next_wl_i].y =
                window_globals->visual_debug_ray_origin_direction[(m*3) + 1];
            next_gpu_workload[next_wl_i].z =
                window_globals->visual_debug_ray_origin_direction[(m*3) + 2];
            next_gpu_workload[next_wl_i].parent_x = 0.0f;
            next_gpu_workload[next_wl_i].parent_y = 0.0f;
            next_gpu_workload[next_wl_i].parent_z = 0.0f;
            next_gpu_workload[next_wl_i].texturearray_i = -1;
            next_gpu_workload[next_wl_i].texture_i = -1;
            next_gpu_workload[next_wl_i].RGBA[0] = 1.0f;
            next_gpu_workload[next_wl_i].RGBA[1] = 1.0f;
            next_gpu_workload[next_wl_i].RGBA[2] = 1.0f;
            next_gpu_workload[next_wl_i].RGBA[3] = 0.75f;
            next_gpu_workload[next_wl_i].ignore_lighting = true;
            next_gpu_workload[next_wl_i].scale_factor = 1.0f;
            next_gpu_workload[next_wl_i].ignore_camera = true;
            next_gpu_workload[next_wl_i].touchable_id = -1;
            next_gpu_workload[next_wl_i].x_angle = 0.0f;
            next_gpu_workload[next_wl_i].y_angle = 0.0f;
            next_gpu_workload[next_wl_i].z_angle = 0.0f;
        }
        *next_workload_size += 3;
    }
    
    log_assert(zpolygons_to_render_size < ZPOLYGONS_TO_RENDER_ARRAYSIZE);
    
    zpolygons_to_triangles(
        next_gpu_workload,
        next_workload_size);
    
    if (application_running) {
        add_particle_effects_to_workload(
            next_gpu_workload,
            next_workload_size,
            lights_for_gpu,
            elapsed_nanoseconds);
        add_shatter_effects_to_workload(
            next_gpu_workload,
            next_workload_size,
            lights_for_gpu,
            elapsed_nanoseconds);
    }
}
