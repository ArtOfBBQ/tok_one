#include "bitmap_renderer.h"

void render_bitmaps(
    Vertex * next_gpu_workload,
    uint32_t * next_gpu_workload_size)
{
    if (
        next_gpu_workload == NULL
        || next_gpu_workload_size == NULL)
    {
        printf("ERROR: platform layer didnt pass recipients\n");
        return;
    }
    
    if (texquads_to_render_size == 0) {
        return;
    }
    
    for (
        uint32_t i = 0;
        i < texquads_to_render_size;
        i++)
    {
        assert(i < TEXQUADS_TO_RENDER_ARRAYSIZE);
        if (!texquads_to_render[i].visible) { continue; }
        
        Vertex topleft[3];
        Vertex bottomright[3];
        
        // top left vertex
        topleft[0].x = texquads_to_render[i].left;
        topleft[0].y = texquads_to_render[i].top;
        topleft[0].z = 1.0f;
        topleft[0].w = 1.0f;
        topleft[0].texturearray_i =
            texquads_to_render[i].texturearray_i;
        topleft[0].texture_i =
            texquads_to_render[i].texture_i;
        topleft[0].uv[0] = 0.0f;
        topleft[0].uv[1] = 0.0f;
        for (uint32_t j = 0; j < 4; j++) {
            topleft[0].RGBA[j] = texquads_to_render[i].RGBA[j];
            topleft[0].lighting[j] = 1.0f;
        }
        // top right vertex
        topleft[1].x =
            texquads_to_render[i].left +
            texquads_to_render[i].width;
        topleft[1].y = texquads_to_render[i].top;
        topleft[1].z = 1.0f;
        topleft[1].w = 1.0f;
        topleft[1].texturearray_i =
            texquads_to_render[i].texturearray_i;
        topleft[1].texture_i =
            texquads_to_render[i].texture_i;
        topleft[1].uv[0] = 1.0f;
        topleft[1].uv[1] = 0.0f;
        for (uint32_t j = 0; j < 4; j++) {
            topleft[1].RGBA[j] = texquads_to_render[i].RGBA[j];
            topleft[1].lighting[j] = 1.0f;
        }
        // bottom left vertex
        topleft[2].x = texquads_to_render[i].left;
        topleft[2].y =
            texquads_to_render[i].top -
            texquads_to_render[i].height;
        topleft[2].z = 1.0f;
        topleft[2].w = 1.0f;
        topleft[2].texturearray_i =
            texquads_to_render[i].texturearray_i;
        topleft[2].texture_i =
            texquads_to_render[i].texture_i;
        topleft[2].uv[0] = 0.0f;
        topleft[2].uv[1] = 1.0f;
        for (uint32_t j = 0; j < 4; j++) {
            topleft[2].RGBA[j] = texquads_to_render[i].RGBA[j];
            topleft[2].lighting[j] = 1.0f;
        }
        
        // top right vertex
        bottomright[0].x =
            texquads_to_render[i].left +
            texquads_to_render[i].width;
        bottomright[0].y = texquads_to_render[i].top;
        bottomright[0].z = 1.0f;
        bottomright[0].w = 1.0f;
        bottomright[0].texturearray_i =
            texquads_to_render[i].texturearray_i;
        bottomright[0].texture_i =
            texquads_to_render[i].texture_i;
        bottomright[0].uv[0] = 1.0f;
        bottomright[0].uv[1] = 0.0f;
        for (uint32_t j = 0; j < 4; j++) {
            bottomright[0].RGBA[j] = texquads_to_render[i].RGBA[j];
            bottomright[0].lighting[j] = 1.0f;
        }
        
        // bottom left vertex
        bottomright[1].x = texquads_to_render[i].left;
        bottomright[1].y = texquads_to_render[i].top -
            texquads_to_render[i].height;
        bottomright[1].z = 1.0f;
        bottomright[1].w = 1.0f;
        bottomright[1].texturearray_i =
            texquads_to_render[i].texturearray_i;
        bottomright[1].texture_i =
            texquads_to_render[i].texture_i;
        bottomright[1].uv[0] = 0.0f;
        bottomright[1].uv[1] = 1.0f;
        for (uint32_t j = 0; j < 4; j++) {
            bottomright[1].RGBA[j] = texquads_to_render[i].RGBA[j];
            bottomright[1].lighting[j] = 1.0f;
        }
        
        // bottom right vertex
        bottomright[2].x =
            texquads_to_render[i].left +
            texquads_to_render[i].width;
        bottomright[2].y = texquads_to_render[i].top -
            texquads_to_render[i].height;
        bottomright[2].z = 1.0f;
        bottomright[2].w = 1.0f;
        bottomright[2].texturearray_i =
            texquads_to_render[i].texturearray_i;
        bottomright[2].texture_i =
            texquads_to_render[i].texture_i;
        bottomright[2].uv[0] = 1.0f;
        bottomright[2].uv[1] = 1.0f;
        for (uint32_t j = 0; j < 4; j++) {
            bottomright[2].RGBA[j] = texquads_to_render[i].RGBA[j];
            bottomright[2].lighting[j] = 1.0f;
        }
        
        draw_triangle(
            /* vertices_recipient: */
                next_gpu_workload,
            /* vertex_count_recipient: */
                next_gpu_workload_size,
            /* input: */
                topleft);
        
        draw_triangle(
            /* vertices_recipient: */
                next_gpu_workload,
            /* vertex_count_recipient: */
                next_gpu_workload_size,
            /* input: */
                bottomright);
    }
}

