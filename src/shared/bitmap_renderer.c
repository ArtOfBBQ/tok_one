#include "bitmap_renderer.h"

TexQuad * texquads_to_render = NULL;
uint32_t texquads_to_render_size = 0;

static void triangle_apply_lighting(
    Vertex out_input[3],
    const zLightSource * zlight_source)
{
    log_assert(zlight_source != NULL);
    
    if (
        zlight_source == NULL
        || zlight_source->deleted
        || zlight_source->reach == 0)
    {
        return;
    }
    
    // add lighting to the 3 vertices
    for (
        uint32_t m = 0;
        m < 3;
        m++)
    {
        float distance =
            sqrtf(
                (
                    (zlight_source->x - out_input[m].x) *
                    (zlight_source->x - out_input[m].x)
                ) +
                (
                    (zlight_source->y - out_input[m].y) *
                    (zlight_source->y - out_input[m].y)
                )
            );
        
        if (distance < 5.0f) { distance = 5.0f; }
        
        float distance_mod =
            (zlight_source->reach - distance) / zlight_source->reach;
        distance_mod *= (distance <= zlight_source->reach);
        
        for (uint32_t l = 0; l < 3; l++) {
            out_input[m].lighting[l] +=
                zlight_source->RGBA[l] * zlight_source->diffuse
                    * distance_mod;
            out_input[m].lighting[l] +=
                zlight_source->RGBA[l] * zlight_source->ambient;
        }
    }
}

// returns false if none found
bool32_t touchable_id_to_texquad_object_id(
    const int32_t touchable_id,
    int32_t * object_id_out)
{
    if (touchable_id < 0) {
        return false;
    }
    
    assert(texquads_to_render_size <= TEXQUADS_TO_RENDER_ARRAYSIZE);
    
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
    
    return false;
}

static bool32_t already_requesting = false;
void request_texquad_renderable(TexQuad * to_add) {
    log_assert(!already_requesting);
    already_requesting = true;
    log_assert(to_add->visible);
    log_assert(to_add->deleted == 0);
    log_assert(to_add->width_pixels > 0);
    log_assert(to_add->height_pixels > 0);
    if (to_add->texturearray_i < 0) { log_assert(to_add->texture_i < 0); }
    if (to_add->texture_i < 0) { log_assert(to_add->texturearray_i < 0); }
    
    if (to_add->texturearray_i >= 0) {
        register_high_priority_if_unloaded(
           to_add->texturearray_i,
           to_add->texture_i);
    }
    
    for (
        uint32_t i = 0;
        i < texquads_to_render_size;
        i++)
    {
        if (texquads_to_render[i].deleted)
        {
            texquads_to_render[i] = *to_add;
            already_requesting = false;
            return;
        }
    }
    
    log_assert(texquads_to_render_size + 1 < TEXQUADS_TO_RENDER_ARRAYSIZE);
    texquads_to_render[texquads_to_render_size] = *to_add;
    texquads_to_render_size += 1;
    
    already_requesting = false;
}

static void z_rotate_triangle(
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

void delete_texquad_object(const int32_t with_object_id)
{
    for (
        int32_t i = (int32_t)texquads_to_render_size - 1;
        i >= 0;
        i--)
    {
        if (texquads_to_render[i].object_id == with_object_id) {
            texquads_to_render[i].visible = false;
            texquads_to_render[i].deleted = true;
        }
    }
}

static void add_quad_to_gpu_workload(
    TexQuad * to_add,
    Vertex * next_gpu_workload,
    uint32_t * next_gpu_workload_size)
{    
    if (to_add->scale_factor < 0.01f)
    {
        log_append(
            "skipping add_quad_to_gpu_workload() because scale factor is"
            " below 0.01\n");
        return;
    }
    
    log_assert(to_add->subquads_per_row < 50);
    log_assert(to_add->width_pixels > 0.0f);
    
    float offset_left = to_add->left_pixels + to_add->x_offset;
    float offset_top = to_add->top_pixels + to_add->y_offset;
    
    float parent_quad_middle_x =
        offset_left + (to_add->width_pixels / 2) - camera.x;
    float parent_quad_middle_y =
        offset_top -
            (to_add->height_pixels / 2) -
                camera.y;
    
    for (
        uint32_t sq_row_i = 0;
        sq_row_i < to_add->subquads_per_row;
        sq_row_i++)
    {
        for (
            uint32_t sq_col_i = 0;
            sq_col_i < to_add->subquads_per_row;
            sq_col_i++)
        {
            Vertex topleft[3];
            Vertex bottomright[3];
            
            float extra_scale = (to_add->scale_factor - 1.0f);
            // float extra_scale_y = (to_add->scale_factor_y - 1.0f);
            if (extra_scale != extra_scale) { return; } // not a number NaN
            // if (extra_scale_y != extra_scale_y) { return; } // not a number NaN
            
            float parent_left = offset_left -
                (extra_scale * 0.5f * to_add->width_pixels);
            float parent_right = offset_left +
                to_add->width_pixels +
                (extra_scale * 0.5f * to_add->width_pixels);
            if (parent_left >= parent_right) {
                parent_right = parent_left + 0.001f;
            }
            float parent_top = offset_top +
                (extra_scale * 0.5f * to_add->height_pixels);
            float parent_bottom = offset_top -
                to_add->height_pixels -
                (extra_scale * 0.5f * to_add->height_pixels);
            if (parent_top <= parent_bottom) {
                parent_top = parent_bottom + 0.01f;
            }
            log_assert(parent_bottom < parent_top);
            
            float left = parent_left +
                ((float)sq_col_i *
                    ((parent_right - parent_left) /
                        (float)to_add->subquads_per_row));
            float right = parent_left +
                ((float)(sq_col_i + 1) *
                    ((parent_right - parent_left) /
                        (float)to_add->subquads_per_row));
            log_assert(left < right);
            float bottom = parent_bottom +
                ((float)sq_row_i *
                    ((parent_top - parent_bottom) /
                        (float)to_add->subquads_per_row));
            float top = parent_bottom +
                ((float)(sq_row_i + 1) *
                    ((parent_top - parent_bottom) /
                        (float)to_add->subquads_per_row));
            log_assert(bottom < top);
            
            float left_uv_coord =
                ((float)sq_col_i / (float)to_add->subquads_per_row);
            float right_uv_coord =
                ((float)sq_col_i + 1.0f) / (float)to_add->subquads_per_row;
            float bottom_uv_coord =
                1.0f - (float)sq_row_i / (float)to_add->subquads_per_row;
            float top_uv_coord =
                1.0f - (float)(sq_row_i + 1.0f) /
                    (float)to_add->subquads_per_row;
            
            if (!to_add->ignore_camera) {
                left -= camera.x;
                right -= camera.x;
                top -= camera.y;
                bottom -= camera.y;
            }
            
            // top left vertex
            topleft[0].x = left;
            topleft[0].y = top;
            topleft[0].z = to_add->z;
            topleft[0].w = 1.0f;
            topleft[0].texturearray_i =
                to_add->texturearray_i;
            topleft[0].texture_i =
                to_add->texture_i;
            topleft[0].uv[0] = left_uv_coord;
            topleft[0].uv[1] = top_uv_coord;
            for (uint32_t j = 0; j < 4; j++) {
                topleft[0].RGBA[j] = to_add->RGBA[j];
                topleft[0].lighting[j] = j == 3 ? 1.0f : 0.0f;
            }
            // top right vertex
            topleft[1].x = right;
            topleft[1].y = top;
            topleft[1].z = to_add->z;
            topleft[1].w = 1.0f;
            topleft[1].texturearray_i =
                to_add->texturearray_i;
            topleft[1].texture_i =
                to_add->texture_i;
            topleft[1].uv[0] = right_uv_coord;
            topleft[1].uv[1] = top_uv_coord;
            for (uint32_t j = 0; j < 4; j++) {
                topleft[1].RGBA[j] = to_add->RGBA[j];
                topleft[1].lighting[j] = j == 3 ? 1.0f : 0.0f;
            }
            // bottom left vertex
            topleft[2].x = left;
            topleft[2].y = bottom;
            topleft[2].z = to_add->z;
            topleft[2].w = 1.0f;
            topleft[2].texturearray_i =
                to_add->texturearray_i;
            topleft[2].texture_i =
                to_add->texture_i;
            topleft[2].uv[0] = left_uv_coord;
            topleft[2].uv[1] = bottom_uv_coord;
            for (uint32_t j = 0; j < 4; j++) {
                topleft[2].RGBA[j] = to_add->RGBA[j];
                topleft[2].lighting[j] = j == 3 ? 1.0f : 0.0f;
            }
            
            // top right vertex
            bottomright[0].x = right;
            bottomright[0].y = top;
            bottomright[0].z = to_add->z;
            bottomright[0].w = 1.0f;
            bottomright[0].texturearray_i =
                to_add->texturearray_i;
            bottomright[0].texture_i =
                to_add->texture_i;
            bottomright[0].uv[0] = right_uv_coord;
            bottomright[0].uv[1] = top_uv_coord;
            for (uint32_t j = 0; j < 4; j++) {
                bottomright[0].RGBA[j] = to_add->RGBA[j];
                bottomright[0].lighting[j] = j == 3 ? 1.0f : 0.0f;
            }
            
            // bottom left vertex
            bottomright[1].x = left;
            bottomright[1].y = bottom;
            bottomright[1].z = to_add->z;
            bottomright[1].w = 1.0f;
            bottomright[1].texturearray_i =
                to_add->texturearray_i;
            bottomright[1].texture_i =
                to_add->texture_i;
            bottomright[1].uv[0] = left_uv_coord;
            bottomright[1].uv[1] = bottom_uv_coord;
            for (uint32_t j = 0; j < 4; j++) {
                bottomright[1].RGBA[j] = to_add->RGBA[j];
                bottomright[1].lighting[j] = j == 3 ? 1.0f : 0.0f;
            }
            
            // bottom right vertex
            bottomright[2].x = right;
            bottomright[2].y = bottom;
            bottomright[2].z = to_add->z;
            bottomright[2].w = 1.0f;
            bottomright[2].texturearray_i =
                to_add->texturearray_i;
            bottomright[2].texture_i =
                to_add->texture_i;
            bottomright[2].uv[0] = right_uv_coord;
            bottomright[2].uv[1] = bottom_uv_coord;
            for (uint32_t j = 0; j < 4; j++) {
                bottomright[2].RGBA[j] = to_add->RGBA[j];
                bottomright[2].lighting[j] = j == 3 ? 1.0f : 0.0f;
            }
            
            Vertex topleft_rotated[3];
            Vertex bottomright_rotated[3];
            
            z_rotate_triangle(
                /* input: */
                    topleft,
                /* around_x : */
                    parent_quad_middle_x,
                /* around_y : */
                    parent_quad_middle_y,
                /* by_angle: */
                    to_add->z_angle,
                /* recipient: */
                    topleft_rotated);
            
            z_rotate_triangle(
                /* input: */
                    bottomright,
                /* around_x : */
                    parent_quad_middle_x,
                /* around_y : */
                    parent_quad_middle_y,
                /* by_angle: */
                    to_add->z_angle,
                /* recipient: */
                    bottomright_rotated);
            
            if (!to_add->ignore_lighting) {
                for (uint32_t i = 0; i < zlights_transformed_size; i++) {
                    triangle_apply_lighting(
                        /* Vertex[3] out_input: */
                            topleft_rotated,
                        /* ZlightSource zlight_source: */
                            zlights_transformed + i);
                    
                    triangle_apply_lighting(
                        /* Vertex[3] out_input: */
                            bottomright_rotated,
                        /* ZlightSource zlight_source: */
                            zlights_transformed + i);
                }
            } else {
                for (uint32_t i = 0; i < 3; i++) {
                    topleft_rotated[i].lighting[0] = 1.0f;
                    topleft_rotated[i].lighting[1] = 1.0f;
                    topleft_rotated[i].lighting[2] = 1.0f;
                    topleft_rotated[i].lighting[3] = 1.0f;
                    bottomright_rotated[i].lighting[0] = 1.0f;
                    bottomright_rotated[i].lighting[1] = 1.0f;
                    bottomright_rotated[i].lighting[2] = 1.0f;
                    bottomright_rotated[i].lighting[3] = 1.0f;
                }
            }
            
            for (uint32_t i = 0; i < 3; i++) {
                topleft_rotated[i].x /= (window_width * 0.5f);
                topleft_rotated[i].x -= 1.0f;
                topleft_rotated[i].y /= (window_height * 0.5f);
                topleft_rotated[i].y -= 1.0f;
                // topleft_rotated[i].w = topleft_rotated[i].z;
                bottomright_rotated[i].x /= (window_width * 0.5f);
                bottomright_rotated[i].x -= 1.0f;
                bottomright_rotated[i].y /= (window_height * 0.5f);
                bottomright_rotated[i].y -= 1.0f;
                // bottomright_rotated[i].w = bottomright_rotated[i].z;
            }
            
            draw_triangle(
                /* vertices_recipient: */
                    next_gpu_workload,
                /* vertex_count_recipient: */
                    next_gpu_workload_size,
                /* input: */
                    topleft_rotated,
                /* touchable_id: */
                    to_add->touchable_id);
            
            draw_triangle(
                /* vertices_recipient: */
                    next_gpu_workload,
                /* vertex_count_recipient: */
                    next_gpu_workload_size,
                /* input: */
                    bottomright_rotated,
                /* touchable_id: */
                    to_add->touchable_id);
        }
    }
}

static int cmpr_vertices_lowest_z(const void * a, const void * b)
{
    if ((*(Vertex *)a).z < (*(Vertex *)b).z) {
        return 1;
    }
    
    return -1;
}

// TODO: remove debugging code
uint32_t sorts_this_frame = 0;

void draw_texquads_to_render(
    Vertex * next_gpu_workload,
    uint32_t * next_gpu_workload_size,
    bool32_t must_opaque,
    bool32_t must_have_alpha_channel)
{
    log_assert(!must_opaque || !must_have_alpha_channel);
    
    if (
        next_gpu_workload == NULL
        || next_gpu_workload_size == NULL)
    {
        log_append(
            "ERROR: platform layer didnt pass recipients\n");
        return;
    }
    
    if (texquads_to_render_size < 1) {
        return;
    }
    
    uint32_t original_texquads_to_render_size = texquads_to_render_size;
    uint32_t original_gpu_workload_size = *next_gpu_workload_size;
    
    for (
        uint32_t i = 0;
        i < original_texquads_to_render_size &&
            i < texquads_to_render_size;
        i++)
    {
        if (
            texquads_to_render[i].visible
            && !texquads_to_render[i].deleted
            && texquads_to_render[i].left_pixels -
                (texquads_to_render[i].ignore_camera ? 0 : camera.x)
                    <= window_width
            && texquads_to_render[i].left_pixels +
                texquads_to_render[i].width_pixels -
                    (texquads_to_render[i].ignore_camera ? 0 : camera.x) >= 0
            && texquads_to_render[i].top_pixels -
                texquads_to_render[i].height_pixels - 
                    (texquads_to_render[i].ignore_camera ? 0 : camera.y)
                        <= window_height
            && texquads_to_render[i].top_pixels -
                (texquads_to_render[i].ignore_camera ? 0 : camera.y)
                    >= 0)
        {
            bool32_t has_alpha_channel =
                texquads_to_render[i].RGBA[3] < 1.0f ||
                texture_has_alpha_channel(
                    texquads_to_render[i].texturearray_i,
                    texquads_to_render[i].texture_i);
            
            if (must_opaque && has_alpha_channel) { continue; }
            if (must_have_alpha_channel && !has_alpha_channel) {
                continue;
            }
            
            log_assert(i <= texquads_to_render_size);
            log_assert(i < TEXQUADS_TO_RENDER_ARRAYSIZE);
            add_quad_to_gpu_workload(
                &texquads_to_render[i],
                next_gpu_workload,
                next_gpu_workload_size);
        }
    }
    
    // We're sorting here and only here because the sprites are
    // transparent, and all of the opaque sprites are already in
    // the next_gpu_workload's first chunk. Those can use the GPU's
    // depth buffer, but from here on out we have to use the painter's
    // algorithm. I've heard of more advanced ways to accomplish this
    // on the GPU without any sorting, but I am not ready for that
    if (
        must_have_alpha_channel &&
        *next_gpu_workload_size > original_gpu_workload_size)
    {
        // start sorting from original_texquads_to_render_size
        // until texquads_to_render_size - 1
        sorts_this_frame += 1;
        qsort(
            /* void *base: */
                next_gpu_workload + *next_gpu_workload_size,
            /* size_t nitems: */
                *next_gpu_workload_size -
                    original_gpu_workload_size,
            /* size_t size (in bytes of 1 element): */
                sizeof(Vertex),
            /* int (*compar)(const void *, const void*): */
                cmpr_vertices_lowest_z);
    }
}

void clean_deleted_texquads(void) {
    
    if (texquads_to_render_size < 2) { return; }
    
    uint32_t i = 0;
    uint32_t j = texquads_to_render_size - 1;
    
    if (i == j) { return; }
    
    while (true) {
        // seek the first non-deleted texquad from the right
        while (texquads_to_render[j].deleted && j > i) {
            log_assert(texquads_to_render_size > 0);
            texquads_to_render_size--;
            j--;
        }
        
        // seek the first deleted texquad from the left
        while (!texquads_to_render[i].deleted && i < j) {
            i++;
        }
        
        if (j > i) {
            // now i is deleted and j is live, swap them
            texquads_to_render[i] = texquads_to_render[j];
            texquads_to_render[j].deleted = true;
            j--;
            log_assert(texquads_to_render_size > 0);
            texquads_to_render_size--;
        } else {
            break;
        }
    }
}

