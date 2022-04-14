#include "bitmap_renderer.h"

TexQuad texquads_to_render[TEXQUADS_TO_RENDER_ARRAYSIZE];
uint32_t texquads_to_render_size = 0;

void request_texquad_renderable(TexQuad * to_add)
{
    assert(to_add->visible);
    assert(to_add->deleted == 0);
    assert(to_add->RGBA[3] > 0.05f);
    
    for (
        uint32_t i = 0;
        i < texquads_to_render_size;
        i++)
    {
        if (texquads_to_render[i].deleted)
        {
            texquads_to_render[i] = *to_add;
            return;
        }
    }
    
    texquads_to_render[texquads_to_render_size] = *to_add;
    texquads_to_render_size += 1;
}

void move_texquad_object(
    uint32_t with_object_id,
    float delta_x,
    float delta_y)
{
    for (
        uint32_t i = 0;
        i < texquads_to_render_size;
        i++)
    {
        if (texquads_to_render[i].object_id == with_object_id)
        {
            texquads_to_render[i].left_pixels += delta_x;
            texquads_to_render[i].top_pixels += delta_y;
        }
    }
}

void z_rotate_triangle(
    Vertex input[3],
    float around_x,
    float around_y,
    float by_angle,
    Vertex recipient[3])
{
    for (uint32_t i = 0; i < 3; i++) {
        recipient[i] = input[i];
    }
    
    for (uint32_t i = 0; i < 3; i++) {
        
        float cur_x = input[i].x - around_x;
        float cur_y = input[i].y - around_y;
        
        float new_x =
            (cur_x * cosf(by_angle)) - 
            (cur_y * sinf(by_angle));
        float new_y =
            (cur_y * cosf(by_angle)) +
            (cur_x * sinf(by_angle));
        
        recipient[i].x = new_x + around_x;
        recipient[i].y = new_y + around_y;
    }
}

void delete_texquad_object(uint32_t with_object_id)
{
    for (
        int32_t i = texquads_to_render_size - 1;
        i >= 0;
        i--)
    {
        if (texquads_to_render[i].object_id == with_object_id)
        {
            texquads_to_render[i].visible = false;
            texquads_to_render[i].deleted = true;
            
            if (i == texquads_to_render_size - 1) {
                texquads_to_render_size -= 1;
            }
        }
    }
}

void add_quad_to_gpu_workload(
    TexQuad * to_add,
    Vertex * next_gpu_workload,
    uint32_t * next_gpu_workload_size)
{
    Vertex topleft[3];
    Vertex bottomright[3];
    
    float z_value = 1.0f;
    
    // top left vertex
    topleft[0].x = to_add->left_pixels;
    topleft[0].y = to_add->top_pixels;
    topleft[0].z = z_value;
    topleft[0].w = 1.0f;
    topleft[0].texturearray_i =
        to_add->texturearray_i;
    topleft[0].texture_i =
        to_add->texture_i;
    topleft[0].uv[0] = 0.0f;
    topleft[0].uv[1] = 0.0f;
    for (uint32_t j = 0; j < 4; j++) {
        topleft[0].RGBA[j] = to_add->RGBA[j];
        topleft[0].lighting[j] = 1.0f;
    }
    // top right vertex
    topleft[1].x =
        to_add->left_pixels +
        to_add->width_pixels;
    topleft[1].y = to_add->top_pixels;
    topleft[1].z = z_value;
    topleft[1].w = 1.0f;
    topleft[1].texturearray_i =
        to_add->texturearray_i;
    topleft[1].texture_i =
        to_add->texture_i;
    topleft[1].uv[0] = 1.0f;
    topleft[1].uv[1] = 0.0f;
    for (uint32_t j = 0; j < 4; j++) {
        topleft[1].RGBA[j] = to_add->RGBA[j];
        topleft[1].lighting[j] = 1.0f;
    }
    // bottom left vertex
    topleft[2].x = to_add->left_pixels;
    topleft[2].y =
        to_add->top_pixels -
        to_add->height_pixels;
    topleft[2].z = z_value;
    topleft[2].w = 1.0f;
    topleft[2].texturearray_i =
        to_add->texturearray_i;
    topleft[2].texture_i =
        to_add->texture_i;
    topleft[2].uv[0] = 0.0f;
    topleft[2].uv[1] = 1.0f;
    for (uint32_t j = 0; j < 4; j++) {
        topleft[2].RGBA[j] = to_add->RGBA[j];
        topleft[2].lighting[j] = 1.0f;
    }
    
    // top right vertex
    bottomright[0].x =
        to_add->left_pixels +
        to_add->width_pixels;
    bottomright[0].y = to_add->top_pixels;
    bottomright[0].z = z_value;
    bottomright[0].w = 1.0f;
    bottomright[0].texturearray_i =
        to_add->texturearray_i;
    bottomright[0].texture_i =
        to_add->texture_i;
    bottomright[0].uv[0] = 1.0f;
    bottomright[0].uv[1] = 0.0f;
    for (uint32_t j = 0; j < 4; j++) {
        bottomright[0].RGBA[j] = to_add->RGBA[j];
        bottomright[0].lighting[j] = 1.0f;
    }
    
    // bottom left vertex
    bottomright[1].x = to_add->left_pixels;
    bottomright[1].y = to_add->top_pixels -
        to_add->height_pixels;
    bottomright[1].z = z_value;
    bottomright[1].w = 1.0f;
    bottomright[1].texturearray_i =
        to_add->texturearray_i;
    bottomright[1].texture_i =
        to_add->texture_i;
    bottomright[1].uv[0] = 0.0f;
    bottomright[1].uv[1] = 1.0f;
    for (uint32_t j = 0; j < 4; j++) {
        bottomright[1].RGBA[j] = to_add->RGBA[j];
        bottomright[1].lighting[j] = 1.0f;
    }
    
    // bottom right vertex
    bottomright[2].x =
        to_add->left_pixels +
        to_add->width_pixels;
    bottomright[2].y = to_add->top_pixels -
        to_add->height_pixels;
    bottomright[2].z = z_value;
    bottomright[2].w = 1.0f;
    bottomright[2].texturearray_i =
        to_add->texturearray_i;
    bottomright[2].texture_i =
        to_add->texture_i;
    bottomright[2].uv[0] = 1.0f;
    bottomright[2].uv[1] = 1.0f;
    for (uint32_t j = 0; j < 4; j++) {
        bottomright[2].RGBA[j] = to_add->RGBA[j];
        bottomright[2].lighting[j] = 1.0f;
    }

    Vertex topleft_rotated[3];
    Vertex bottomright_rotated[3];
    
    z_rotate_triangle(
        /* input: */ topleft,
        /* around_x : */ to_add->left_pixels + (to_add->width_pixels * 0.5f),
        /* around_y : */ to_add->top_pixels - (to_add->height_pixels * 0.5f),
        /* by_angle: */ to_add->z_angle,
        /* recipient: */ topleft_rotated);
    z_rotate_triangle(
        /* input: */ bottomright,
        /* around_x : */ to_add->left_pixels + (to_add->width_pixels * 0.5f),
        /* around_y : */ to_add->top_pixels - (to_add->height_pixels * 0.5f),
        /* by_angle: */ to_add->z_angle,
        /* recipient: */ bottomright_rotated);

    for (uint32_t i = 0; i < 3; i++) {
        topleft_rotated[i].x /= window_width;
        topleft_rotated[i].y /= window_height;
        bottomright_rotated[i].x /= window_width;
        bottomright_rotated[i].y /= window_height;
    }
    
    draw_triangle(
        /* vertices_recipient: */
            next_gpu_workload,
        /* vertex_count_recipient: */
            next_gpu_workload_size,
        /* input: */
            topleft_rotated);
    
    draw_triangle(
        /* vertices_recipient: */
            next_gpu_workload,
        /* vertex_count_recipient: */
            next_gpu_workload_size,
        /* input: */
            bottomright_rotated);
}

void draw_texquads_to_render(
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
        add_quad_to_gpu_workload(
            &texquads_to_render[i],
            next_gpu_workload,
            next_gpu_workload_size);
    }
}

