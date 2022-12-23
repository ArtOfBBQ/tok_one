#include "bitmap_renderer.h"

TexQuad * texquads_to_render = NULL;
uint32_t texquads_to_render_size = 0;

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
    log_assert(to_add->width > 0);
    log_assert(to_add->height > 0);
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

