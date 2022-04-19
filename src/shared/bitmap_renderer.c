#include "bitmap_renderer.h"

TexQuad texquads_to_render[TEXQUADS_TO_RENDER_ARRAYSIZE];
uint32_t texquads_to_render_size = 0;

// returns false if none found
bool32_t touchable_id_to_texquad_object_id(
    const int32_t touchable_id,
    uint32_t * object_id_out)
{
    printf(
        "touchable_id_to_texquad_object_id 4 touchable_id: %i\n",
        touchable_id);
    
    if (touchable_id < 0) {
        return false;
    }
    
    for (
        uint32_t i = 0;
        i < texquads_to_render_size;
        i++)
    {
        if (
            texquads_to_render[i].touchable_id ==
                touchable_id)
        {
            *object_id_out = texquads_to_render[i].object_id;
            return true;
        }
    }
    
    printf("no such touchable_id\n");
    return false;
}

void request_texquad_renderable(
    TexQuad * to_add)
{
    if (to_add->texturearray_i != 0) {
        printf(
            "request_texquad_renderable at [%f,%f] with width %f height %f visible %u deleted %u RGBA [%f,%f,%f,%f]\n",
            to_add->left_pixels,
            to_add->top_pixels,
            to_add->width_pixels,
            to_add->height_pixels,
            to_add->visible,
            to_add->deleted,
            to_add->RGBA[0],
            to_add->RGBA[1],
            to_add->RGBA[2],
            to_add->RGBA[3]);
    }
    assert(to_add->visible);
    assert(to_add->deleted == 0);
    
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
    
    float z_value = 0.3f + (to_add->z * 0.001f);
    
    float left = to_add->left_pixels;
    float right = to_add->left_pixels + to_add->width_pixels;
    float mid_x = (left + right) * 0.5;
    float top = to_add->top_pixels;
    float bottom = to_add->top_pixels - to_add->height_pixels;
    float mid_y = (top + bottom) * 0.5;
    
    // top left vertex
    topleft[0].x = left;
    topleft[0].y = top;
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
    topleft[1].x = right;
    topleft[1].y = top;
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
    topleft[2].x = left;
    topleft[2].y = bottom;
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
    bottomright[0].x = right;
    bottomright[0].y = top;
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
    bottomright[1].x = left;
    bottomright[1].y = bottom;
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
    bottomright[2].x = right;
    bottomright[2].y = bottom;
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
        /* input: */
            topleft,
        /* around_x : */
            (left + right) * 0.5f,
        /* around_y : */
            (top + bottom) * 0.5,
        /* by_angle: */
            to_add->z_angle,
        /* recipient: */
            topleft_rotated);
    
    z_rotate_triangle(
        /* input: */
            bottomright,
        /* around_x : */
            (left + right) * 0.5f,
        /* around_y : */
            (top + bottom) * 0.5,
        /* by_angle: */
            to_add->z_angle,
        /* recipient: */
            bottomright_rotated);
    
    for (uint32_t i = 0; i < 3; i++) {
        topleft_rotated[i].x /= (window_width * 0.5f);
        topleft_rotated[i].x -= 1.0f;
        topleft_rotated[i].y /= (window_height * 0.5f);
        topleft_rotated[i].y -= 1.0f;
        bottomright_rotated[i].x /= (window_width * 0.5f);
        bottomright_rotated[i].x -= 1.0f;
        bottomright_rotated[i].y /= (window_height * 0.5f);
        bottomright_rotated[i].y -= 1.0f;
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
    
    if (to_add->touchable_id >= 0) {
        register_touchable_triangle(
            /* const int32_t touchable_id: */
                to_add->touchable_id,
            /* Vertex triangle_area[3]: */
                topleft_rotated);
        
        register_touchable_triangle(
            /* const int32_t touchable_id: */
                to_add->touchable_id,
            /* Vertex triangle_area[3]: */
                bottomright_rotated);
    }
}

int sorter_cmpr_texquad_lowest_z(
    const void * a,
    const void * b)
{
    return ((TexQuad *)a)->z < ((TexQuad *)b)->z ? -1 : 1;
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
    
    TexQuad sorted_texquads[texquads_to_render_size];
    uint32_t sorted_texquads_size = 0;
    for (uint32_t i = 0; i < texquads_to_render_size; i++) {
        if (
            texquads_to_render[i].visible
            && !texquads_to_render[i].deleted)
        {
            sorted_texquads[sorted_texquads_size++] =
                texquads_to_render[i];
        }
    }
    
    qsort(
        sorted_texquads,
        sorted_texquads_size,
        sizeof(TexQuad),
        &sorter_cmpr_texquad_lowest_z);
    
    for (
        uint32_t i = 0;
        i < sorted_texquads_size;
        i++)
    {
        assert(i < TEXQUADS_TO_RENDER_ARRAYSIZE);
        
        add_quad_to_gpu_workload(
            &sorted_texquads[i],
            next_gpu_workload,
            next_gpu_workload_size);
    }
}
