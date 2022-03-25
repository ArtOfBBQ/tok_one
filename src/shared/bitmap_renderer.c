#include "bitmap_renderer.h"

DecodedImage bitmap;

void bitmap_renderer_init() {
    bitmap.width = 100;
    bitmap.height = 100;
    bitmap.rgba_values_size = bitmap.height * bitmap.width * 4;
    bitmap.rgba_values = malloc(bitmap.rgba_values_size);
    
    for (uint32_t i = 0; i < bitmap.rgba_values_size; i += 4) {
        bitmap.rgba_values[i] = 125;
        bitmap.rgba_values[i+1] = 125;
        bitmap.rgba_values[i+2] = 125;
        bitmap.rgba_values[i+3] = 255;
    }
}

void bitmap_clear() {
    for (uint32_t i = 0; i < bitmap.rgba_values_size; i += 4) {
        bitmap.rgba_values[i] = 255;
        bitmap.rgba_values[i+1] = 0;
        bitmap.rgba_values[i+2] = 125;
        bitmap.rgba_values[i+3] = 255;
    }
}

/* Draw bitmap(s) of pixels and add them to the gpu's workload */
void bitmap_render(
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
    
    bitmap_clear();

    Vertex topleft[3];
    Vertex bottomright[3];
    
    // top left vertex
    topleft[0].x = 0.70;
    topleft[0].y = -0.70;
    topleft[0].z = 0.0f;
    topleft[0].texturearray_i = BITMAP_TEXTUREARRAY_I;
    topleft[0].texture_i = 0;
    topleft[0].RGBA[0] = 0.0f;
    topleft[0].RGBA[1] = 0.2f;
    topleft[0].RGBA[2] = 1.0f;
    topleft[0].RGBA[3] = 0.0f;
    topleft[0].lighting = 0.5f;
    // top right vertex
    topleft[1].x = 0.95f;
    topleft[1].y = -0.70;
    topleft[1].z = 0.0f;
    topleft[1].texturearray_i = BITMAP_TEXTUREARRAY_I;
    topleft[1].texture_i = 0;
    topleft[1].RGBA[0] = 0.5f;
    topleft[1].RGBA[1] = 0.5f;
    topleft[1].RGBA[2] = 0.2f;
    topleft[1].RGBA[3] = 1.0f;
    topleft[1].lighting = 0.5f;
    // bottom left vertex
    topleft[2].x = 0.70;
    topleft[2].y = -0.95f;
    topleft[2].z = 0.0f;
    topleft[2].texturearray_i = BITMAP_TEXTUREARRAY_I;
    topleft[2].texture_i = 0;
    topleft[2].RGBA[0] = 1.0f;
    topleft[2].RGBA[1] = 0.2f;
    topleft[2].RGBA[2] = 0.2f;
    topleft[2].RGBA[3] = 1.0f;
    topleft[2].lighting = 0.5f;
    
    // top right vertex
    bottomright[0].x = 0.95f;
    bottomright[0].y = -0.70;
    bottomright[0].z = 0.0f;
    bottomright[0].texturearray_i = BITMAP_TEXTUREARRAY_I;
    bottomright[0].texture_i = 0;
    bottomright[0].RGBA[0] = 0.5f;
    bottomright[0].RGBA[1] = 0.5f;
    bottomright[0].RGBA[2] = 0.2f;
    bottomright[0].RGBA[3] = 1.0f;
    bottomright[0].lighting = 0.5f;
    // bottom left vertex
    bottomright[1].x = 0.70;
    bottomright[1].y = -0.95f;
    bottomright[1].z = 0.0f;
    bottomright[1].texturearray_i = BITMAP_TEXTUREARRAY_I;
    bottomright[1].texture_i = 0;
    bottomright[1].RGBA[0] = 1.0f;
    bottomright[1].RGBA[1] = 0.2f;
    bottomright[1].RGBA[2] = 0.2f;
    bottomright[1].RGBA[3] = 1.0f;
    bottomright[1].lighting = 0.5f;
    // bottom right vertex
    bottomright[2].x = 0.95;
    bottomright[2].y = -0.95;
    bottomright[2].z = 0.0f;
    bottomright[2].texturearray_i = BITMAP_TEXTUREARRAY_I;
    bottomright[2].texture_i = 0;
    bottomright[2].RGBA[0] = 1.0f;
    bottomright[2].RGBA[1] = 0.2f;
    bottomright[2].RGBA[2] = 0.2f;
    bottomright[2].RGBA[3] = 1.0f;
    bottomright[2].lighting = 0.5f;
    
    platform_update_gpu_texture(
        /* texturearray_i: */ BITMAP_TEXTUREARRAY_I,
        /* texture_i: */ 0,
        /* with_img: */ &bitmap);
    
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

