#include "bitmap_renderer.h"

TexQuad texquads_to_render[TEXQUADS_TO_RENDER_ARRAYSIZE];
uint32_t texquads_to_render_size = 0;

void request_texquad_renderable(TexQuad * to_add)
{
    printf("adding new texquad renderable...\n");
    texquads_to_render[texquads_to_render_size] = *to_add;
    texquads_to_render_size += 1;
    printf(
        "now there are %u renderables...\n",
        texquads_to_render_size);
}

void add_quad_to_gpu_workload(
    TexQuad * to_add,
    Vertex * next_gpu_workload,
    uint32_t * next_gpu_workload_size)
{
    Vertex topleft[3];
    Vertex bottomright[3];
    
    // top left vertex
    topleft[0].x = to_add->left;
    topleft[0].y = to_add->top;
    topleft[0].z = 1.0f;
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
        to_add->left +
        to_add->width;
    topleft[1].y = to_add->top;
    topleft[1].z = 1.0f;
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
    topleft[2].x = to_add->left;
    topleft[2].y =
        to_add->top -
        to_add->height;
    topleft[2].z = 1.0f;
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
        to_add->left +
        to_add->width;
    bottomright[0].y = to_add->top;
    bottomright[0].z = 1.0f;
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
    bottomright[1].x = to_add->left;
    bottomright[1].y = to_add->top -
        to_add->height;
    bottomright[1].z = 1.0f;
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
        to_add->left +
        to_add->width;
    bottomright[2].y = to_add->top -
        to_add->height;
    bottomright[2].z = 1.0f;
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

