#include "bitmap_renderer.h"

DecodedImage bitmap;

void bitmap_renderer_init() {
    printf("bitmap_renderer_init()...\n");
    bitmap.width = BITMAP_PIXELS_WIDTH;
    bitmap.height = BITMAP_PIXELS_WIDTH;
    bitmap.rgba_values_size = bitmap.height * bitmap.width * 4;
    bitmap.rgba_values = malloc(bitmap.rgba_values_size);
    
    for (
        uint32_t i = 0;
        i < bitmap.rgba_values_size;
        i += 4)
    {
        bitmap.rgba_values[i] = i % 50;
        bitmap.rgba_values[i+1] = 25;
        bitmap.rgba_values[i+2] = (i + 125) % 75;
        bitmap.rgba_values[i+3] = 255;
    }
}

void minimap_clear() {
    for (
        uint32_t i = 0;
        i < bitmap.rgba_values_size;
        i += 4)
    {
        if
        (
            i % (bitmap.width * 4) == 0
            ||
            (i % (bitmap.width * 4)) == ((bitmap.width * 4) - 4)
            ||
            i / (bitmap.width * 4) == 0
            ||
            (i / (bitmap.width * 4) == (bitmap.height - 1))
        )
        {
            bitmap.rgba_values[i] = 220;
            bitmap.rgba_values[i+1] = 220;
            bitmap.rgba_values[i+2] = 220;
            bitmap.rgba_values[i+3] = 255;
        } else {
            bitmap.rgba_values[i] = 30;
            bitmap.rgba_values[i+1] = 30;
            bitmap.rgba_values[i+2] = 30;
            bitmap.rgba_values[i+3] = 255;
        }
    }
}

uint32_t img_xy_to_pixel(
    const uint32_t x,
    const uint32_t y,
    DecodedImage * img)
{
    uint32_t return_value = 0;
    
    assert(x > 0);
    assert(x < img->width);
    assert(y > 0);
    assert(y < img->height);
    
    return_value += ((x - 1) * 4);
    return_value += ((y - 1) * 4 * img->width);
    
    assert(return_value >= 0);
    assert(return_value < img->rgba_values_size);
    
    return return_value;
}

void minimap_add_object(
    const float x,
    const float y,
    const uint8_t red,
    const uint8_t green,
    const uint8_t blue)
{
    printf("minimap_add_object()\n");
    uint32_t cam_reach = BITMAP_PIXELS_WIDTH / 2;
    
    int32_t i_x = (uint32_t)(x + cam_reach);
    int32_t i_y = (uint32_t)(y + cam_reach);
    
    if (i_x < 0 || i_y < 0) {
        return;
    }
    
    uint32_t ui_x = (uint32_t)i_x;
    uint32_t ui_y = (uint32_t)i_y;
    
    /*
    printf(
        "converted floats [%f,%f] to ints [%i,%i] then uints [%u,%u]\n",
        x,
        y,
        i_x,
        i_y,
        ui_x,
        ui_y);
    */

    if (ui_x < 1 || ui_x >= bitmap.width) { return; }
    if (ui_y < 1 || ui_y >= bitmap.height) { return; }
    
    uint32_t location = img_xy_to_pixel(
        /* x: */ ui_x,
        /* y: */ ui_y,
        /* img: */ &bitmap);
    
    bitmap.rgba_values[location] = 255;
    bitmap.rgba_values[location + 1] = red;
    bitmap.rgba_values[location + 2] = green;
    bitmap.rgba_values[location + 3] = blue;
    printf("end of minimap_add_object\n");
}

void minimap_add_zpolygon(
    zPolygon * to_add)
{
    if (to_add == NULL) { return; }
    printf("adding zpolygon at [%f,%f]\n",
        to_add->x,
        to_add->z);
    
    minimap_add_object(
        to_add->x,
        to_add->z,
        255,
        255,
        255);
}

void minimap_add_triangle(
    zTriangle * to_add)
{
    if (to_add == NULL) { return; }
    
    float avg_x =
        (to_add->vertices[0].x +
        to_add->vertices[1].x +
        to_add->vertices[2].x) / 3.0f;
    float avg_z =
        (to_add->vertices[0].z +
        to_add->vertices[1].z +
        to_add->vertices[2].z) / 3.0f;
    
    minimap_add_object(
        avg_x,
        avg_z,
        255,
        255,
        255);
}

void minimap_add_camera(
    zCamera * to_add)
{
    minimap_add_object(
        to_add->x,
        to_add->z,
        255,
        0,
        255);
}

/* Draw bitmap(s) of pixels and add them to the gpu's workload */
void bitmap_blit(
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
    
    Vertex topleft[3];
    Vertex bottomright[3];
    
    // we'll make it 1/5th the width of screen,
    // and the height as high as the width
    float minimap_width = 0.4;
    float minimap_height =
        minimap_width * (window_width / window_height);
    float minimap_offset = 0.05f;
    
    // top left vertex
    topleft[0].x = 1 - minimap_width - minimap_offset;
    topleft[0].y = -1.0 + minimap_height + minimap_offset;
    topleft[0].z = 0.0f;
    topleft[0].texturearray_i = BITMAP_TEXTUREARRAY_I;
    topleft[0].texture_i = 0;
    topleft[0].RGBA[0] = 0.0f;
    topleft[0].RGBA[1] = 0.2f;
    topleft[0].RGBA[2] = 1.0f;
    topleft[0].RGBA[3] = 0.0f;
    topleft[0].uv[0] = 0.0f;
    topleft[0].uv[1] = 0.0f;
    topleft[0].lighting = 0.5f;
    // top right vertex
    topleft[1].x = 1.0f - minimap_offset;
    topleft[1].y = -1.0f + minimap_height + minimap_offset;
    topleft[1].z = 0.0f;
    topleft[1].texturearray_i = BITMAP_TEXTUREARRAY_I;
    topleft[1].texture_i = 0;
    topleft[1].RGBA[0] = 0.5f;
    topleft[1].RGBA[1] = 0.5f;
    topleft[1].RGBA[2] = 0.2f;
    topleft[1].RGBA[3] = 1.0f;
    topleft[1].uv[0] = 1.0f;
    topleft[1].uv[1] = 0.0f;
    topleft[1].lighting = 0.5f;
    // bottom left vertex
    topleft[2].x = 1.0f - minimap_width - minimap_offset;
    topleft[2].y = -1.0f + minimap_offset;
    topleft[2].z = 0.0f;
    topleft[2].texturearray_i = BITMAP_TEXTUREARRAY_I;
    topleft[2].texture_i = 0;
    topleft[2].RGBA[0] = 1.0f;
    topleft[2].RGBA[1] = 0.2f;
    topleft[2].RGBA[2] = 0.2f;
    topleft[2].RGBA[3] = 1.0f;
    topleft[2].uv[0] = 0.0f;
    topleft[2].uv[1] = 1.0f;
    topleft[2].lighting = 0.5f;
    
    // top right vertex
    bottomright[0].x = 1.0f - minimap_offset;
    bottomright[0].y = -1.0f + minimap_height + minimap_offset;
    bottomright[0].z = 0.0f;
    bottomright[0].texturearray_i = BITMAP_TEXTUREARRAY_I;
    bottomright[0].texture_i = 0;
    bottomright[0].RGBA[0] = 1.0f;
    bottomright[0].RGBA[1] = 0.0f;
    bottomright[0].RGBA[2] = 0.0f;
    bottomright[0].RGBA[3] = 1.0f;
    bottomright[0].uv[0] = 1.0f;
    bottomright[0].uv[1] = 0.0f;
    bottomright[0].lighting = 0.5f;
    // bottom left vertex
    bottomright[1].x = 1.0f - minimap_width - minimap_offset;
    bottomright[1].y = -1.0f + minimap_offset;
    bottomright[1].z = 0.0f;
    bottomright[1].texturearray_i = BITMAP_TEXTUREARRAY_I;
    bottomright[1].texture_i = 0;
    bottomright[1].RGBA[0] = 1.0f;
    bottomright[1].RGBA[1] = 0.0f;
    bottomright[1].RGBA[2] = 0.0f;
    bottomright[1].RGBA[3] = 1.0f;
    bottomright[1].uv[0] = 0.0f;
    bottomright[1].uv[1] = 1.0f;
    bottomright[1].lighting = 0.5f;
    // bottom right vertex
    bottomright[2].x = 1.0f - minimap_offset;
    bottomright[2].y = -1.0f + minimap_offset;
    bottomright[2].z = 0.0f;
    bottomright[2].texturearray_i = BITMAP_TEXTUREARRAY_I;
    bottomright[2].texture_i = 0;
    bottomright[2].RGBA[0] = 1.0f;
    bottomright[2].RGBA[1] = 0.0f;
    bottomright[2].RGBA[2] = 0.0f;
    bottomright[2].RGBA[3] = 1.0f;
    bottomright[2].uv[0] = 1.0f;
    bottomright[2].uv[1] = 1.0f;
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

