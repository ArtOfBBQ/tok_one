#include "bitmap_renderer.h"

void bitmap_renderer_init() {
    minimap.width = BITMAP_PIXELS_WIDTH;
    minimap.height = BITMAP_PIXELS_WIDTH;
    minimap.rgba_values_size =
        minimap.height * minimap.width * 4;
    minimap.rgba_values = malloc(minimap.rgba_values_size);
    
    minimap2.width = BITMAP_PIXELS_WIDTH;
    minimap2.height = BITMAP_PIXELS_WIDTH;
    minimap2.rgba_values_size =
        minimap2.height * minimap2.width * 4;
    minimap2.rgba_values =
        malloc(minimap2.rgba_values_size);
    
    for (
        uint32_t i = 0;
        i < minimap.rgba_values_size;
        i += 4)
    {
        minimap.rgba_values[i+0] = i % 50;
        minimap.rgba_values[i+1] = 25;
        minimap.rgba_values[i+2] = (i + 125) % 75;
        minimap.rgba_values[i+3] = 255;
        
        minimap2.rgba_values[i+0] = i % 50;
        minimap2.rgba_values[i+1] = 25;
        minimap2.rgba_values[i+2] = (i + 125) % 75;
        minimap2.rgba_values[i+3] = 255;
    }
}

void minimaps_clear() {
    for (
        uint32_t i = 0;
        i < minimap.rgba_values_size;
        i += 4)
    {
        if
        (
            i % (minimap.width * 4) == 0
            ||
            (i % (minimap.width * 4)) == ((minimap.width * 4) - 4)
            ||
            i / (minimap.width * 4) == 0
            ||
            (i / (minimap.width * 4) == (minimap.height - 1))
        )
        {
            minimap.rgba_values[i] = 220;
            minimap.rgba_values[i+1] = 220;
            minimap.rgba_values[i+2] = 220;
            minimap.rgba_values[i+3] = 255;
            
            minimap2.rgba_values[i] = 220;
            minimap2.rgba_values[i+1] = 220;
            minimap2.rgba_values[i+2] = 220;
            minimap2.rgba_values[i+3] = 255;
        } else {
            minimap.rgba_values[i] = 30;
            minimap.rgba_values[i+1] = 30;
            minimap.rgba_values[i+2] = 30;
            minimap.rgba_values[i+3] = 255;
            
            minimap2.rgba_values[i] = 30;
            minimap2.rgba_values[i+1] = 30;
            minimap2.rgba_values[i+2] = 30;
            minimap2.rgba_values[i+3] = 255;
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

void decodedimg_add_pixel(
    DecodedImage * to_modify,
    const float x,
    const float y,
    const uint8_t red,
    const uint8_t green,
    const uint8_t blue)
{
    uint32_t cam_reach = BITMAP_PIXELS_WIDTH / 2;
    
    int32_t i_x = (uint32_t)(x + cam_reach);
    int32_t i_y = (uint32_t)(y + cam_reach);
    
    if (i_x < 0 || i_y < 0) {
        return;
    }
    
    uint32_t ui_x = (uint32_t)i_x;
    uint32_t ui_y = (uint32_t)i_y;
    
    if (ui_x < 1 || ui_x >= to_modify->width) { return; }
    if (ui_y < 1 || ui_y >= to_modify->height) { return; }
    
    uint32_t location = img_xy_to_pixel(
        /* x: */ ui_x,
        /* y: */ ui_y,
        /* img: */ to_modify);
    
    to_modify->rgba_values[location] = 255;
    to_modify->rgba_values[location + 1] = red;
    to_modify->rgba_values[location + 2] = green;
    to_modify->rgba_values[location + 3] = blue;
}

void decodedimg_add_triangle(
    DecodedImage * to_modify,
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
    
    decodedimg_add_pixel(
        to_modify,
        avg_x,
        avg_z,
        255,
        255,
        255);
}

void decodedimg_add_zpolygon(
    DecodedImage * to_modify,
    zPolygon * to_add)
{
    if (to_add == NULL) { return; }
    
    decodedimg_add_pixel(
        to_modify,
        to_add->x,
        to_add->z,
        255,
        255,
        255);
}

void decodedimg_add_camera(
    DecodedImage * to_modify,
    zCamera * to_add)
{
    decodedimg_add_pixel(
        to_modify,
        to_add->x,
        to_add->z,
        255,
        0,
        255);
    
    // draw field of view somehow
    //
    //       cam looking striaght down (Y angle is 0)
    //        *
    //       **
    //      * *
    //     P***   P2
    //
    // angle_right is just cam's y angle + (3.14 / 4)
    // we know the angle and the distance (the hypotenuse)
    // we want to know the opposite and the adjacent
    // we can get adjacent with CAH and opposite with SOH
    //
    // if C = A/H then A = C * H
    // if S = O/H then O = S * H
    for (
        float hypotenuse = 5.0f;
        hypotenuse < 41.0f;
        hypotenuse += 5.0f)
    {
        float angle_right =
            to_add->y_angle + (
                projection_constants.field_of_view_rad / 2);
        float angle_left =
            to_add->y_angle + (
                projection_constants.field_of_view_rad / 2);
        
        float adjacent = cosf(angle_right) * hypotenuse;
        float opposite = sinf(angle_right) * hypotenuse;
        
        decodedimg_add_pixel(
            to_modify,
            to_add->x + opposite,
            to_add->z + adjacent,
            20,
            20,
            250);
        
        angle_right =
            to_add->y_angle - (
                projection_constants.field_of_view_rad / 2);
        angle_left =
            to_add->y_angle - (
                projection_constants.field_of_view_rad / 2);
        
        adjacent = cosf(angle_right) * hypotenuse;
        opposite = sinf(angle_right) * hypotenuse;
        decodedimg_add_pixel(
            to_modify,
            to_add->x + opposite,
            to_add->z + adjacent,
            20,
            20,
            250);
    }
}

/* Draw 2 minimaps and add them to the gpu's workload */
void minimaps_blit(
    Vertex * next_gpu_workload,
    uint32_t * next_gpu_workload_size)
{
    if (minimaps_visible == false) {
        return;
    }
    
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
    topleft[0].z = 1.0f;
    topleft[0].w = 1.0f;
    topleft[0].texturearray_i = 2;
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
    topleft[1].z = 1.0f;
    topleft[1].w = 1.0f;
    topleft[1].texturearray_i = 2;
    topleft[1].texture_i = 0;
    topleft[1].uv[0] = 1.0f;
    topleft[1].uv[1] = 0.0f;
    topleft[1].lighting = 0.5f;
    // bottom left vertex
    topleft[2].x = 1.0f - minimap_width - minimap_offset;
    topleft[2].y = -1.0f + minimap_offset;
    topleft[2].z = 1.0f;
    topleft[2].w = 1.0f;
    topleft[2].texturearray_i = 2;
    topleft[2].texture_i = 0;
    topleft[2].uv[0] = 0.0f;
    topleft[2].uv[1] = 1.0f;
    topleft[2].lighting = 0.5f;
    
    // top right vertex
    bottomright[0].x = 1.0f - minimap_offset;
    bottomright[0].y = -1.0f + minimap_height + minimap_offset;
    bottomright[0].z = 1.0f;
    bottomright[0].w = 1.0f;
    bottomright[0].texturearray_i = 2;
    bottomright[0].texture_i = 0;
    bottomright[0].uv[0] = 1.0f;
    bottomright[0].uv[1] = 0.0f;
    bottomright[0].lighting = 0.5f;
    // bottom left vertex
    bottomright[1].x = 1.0f - minimap_width - minimap_offset;
    bottomright[1].y = -1.0f + minimap_offset;
    bottomright[1].z = 1.0f;
    bottomright[1].w = 1.0f;
    bottomright[1].texturearray_i = 2;
    bottomright[1].texture_i = 0;
    bottomright[1].uv[0] = 0.0f;
    bottomright[1].uv[1] = 1.0f;
    bottomright[1].lighting = 0.5f;
    // bottom right vertex
    bottomright[2].x = 1.0f - minimap_offset;
    bottomright[2].y = -1.0f + minimap_offset;
    bottomright[2].z = 1.0f;
    bottomright[2].w = 1.0f;
    bottomright[2].texturearray_i = 2;
    bottomright[2].texture_i = 0;
    bottomright[2].uv[0] = 1.0f;
    bottomright[2].uv[1] = 1.0f;
    bottomright[2].lighting = 0.5f;

    Vertex minimap2_topleft[3];
    Vertex minimap2_bottomright[3];
    for (
        uint32_t i = 0;
        i < 3;
        i++)
    {
        minimap2_topleft[i].lighting = 0.5f;
        minimap2_bottomright[i].lighting = 0.5f;
        minimap2_topleft[i].x = topleft[i].x;
        minimap2_topleft[i].y = topleft[i].y;
        minimap2_topleft[i].z = topleft[i].z;
        minimap2_topleft[i].w = topleft[i].w;
        minimap2_bottomright[i].x = bottomright[i].x;
        minimap2_bottomright[i].y = bottomright[i].y;
        minimap2_bottomright[i].z = bottomright[i].z;
        minimap2_bottomright[i].w = bottomright[i].w;
        
        minimap2_topleft[i].uv[0] = topleft[i].uv[0];
        minimap2_topleft[i].uv[1] = topleft[i].uv[1];
        minimap2_bottomright[i].uv[0] = bottomright[i].uv[0];
        minimap2_bottomright[i].uv[1] = bottomright[i].uv[1];
        
        minimap2_topleft[i].texturearray_i =
            3;
        minimap2_bottomright[i].texturearray_i =
            3;
        minimap2_topleft[i].texture_i = 0;
        minimap2_bottomright[i].texture_i = 0;
        minimap2_topleft[i].y +=
            minimap_offset + minimap_height;
        minimap2_bottomright[i].y +=
            minimap_offset + minimap_height;
    }
    
    platform_update_gpu_texture(
        /* texturearray_i: */ 2,
        /* texture_i: */ 0,
        /* with_img: */ &minimap);
    platform_update_gpu_texture(
        /* texturearray_i: */ 3,
        /* texture_i: */ 0,
        /* with_img: */ &minimap2);
    
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
    
    draw_triangle(
        /* vertices_recipient: */
            next_gpu_workload,
        /* vertex_count_recipient: */
            next_gpu_workload_size,
        /* input: */
            minimap2_topleft);
    
    draw_triangle(
        /* vertices_recipient: */
            next_gpu_workload,
        /* vertex_count_recipient: */
            next_gpu_workload_size,
        /* input: */
            minimap2_bottomright);
}

