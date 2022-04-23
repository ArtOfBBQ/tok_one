#include "texquad_type.h"

void construct_texquad(TexQuad * to_construct)
{
    to_construct->object_id = 0;
    to_construct->touchable_id = -1;
    to_construct->texturearray_i = -1;
    to_construct->texture_i = -1;
    for (uint32_t i = 0; i < 4; i++) {
        to_construct->RGBA[i] = 1.0f;
    }
    to_construct->left_pixels = 0.0f;
    to_construct->top_pixels = 0.0f;
    to_construct->height_pixels = 75.0f;
    to_construct->width_pixels = 75.0f;
    to_construct->scale_factor = 1.0f;
    to_construct->z = 0.5f;
    to_construct->z_angle = 0.0f;
    to_construct->ignore_camera = false;
    to_construct->visible = true;
    to_construct->deleted = false;
}

