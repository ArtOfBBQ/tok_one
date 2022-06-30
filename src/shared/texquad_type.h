#ifndef TEXQUAD_TYPE_H
#define TEXQUAD_TYPE_H

#include "common.h"

typedef struct TexQuad {
    // You can assign an object_id to -1 (not part of any group) or give
    // multiple texquads, zlights, and zvertexes the same object_id to make
    // them behave as a group. You can schedule animations that affect all
    // members in a group the same way. For example, a animation to move all
    // texquads and zlights with object_id of 2 500 pixels to the left over the
    // next 0.2 seconds.
    int32_t object_id; 
    
    // -1 if you don't care when this is clicked or touched
    int32_t touchable_id;
    
    // the index of the texturearray,
    // aka texture atlas, to
    // texture-map to this quad
    // use '-1' for 'no texture'
    int32_t texturearray_i; 
    
    /*
    the index of the texture inside the texturearray to texture-map to this
    quad. If the texture atlas is just 1 big image, use 0 use '-1' for
    'no texture'
    */
    int32_t texture_i;
    
    // the color of this quad
    // when combined with a texture, the texture
    // will be mixed with this color
    float RGBA[4];
    
    /*
    use 'adjust_texquad_left_pixels' and 'adjust_texquad_top_pixels' so that
    the attached zlights move along
    */
    float left_pixels;
    float top_pixels; // y = window_height for top of screen
                      // y = 0 for right below the screen
    
    float height_pixels;
    float width_pixels;
    float scale_factor_x;
    float scale_factor_y;
    float z;
    float z_angle;
    bool32_t ignore_camera;
    bool32_t ignore_lighting;
    bool32_t visible; // skip rendering me but keep me in memory
    bool32_t deleted; // overwrite me if true
} TexQuad;

void construct_texquad(TexQuad * to_construct);

#endif
