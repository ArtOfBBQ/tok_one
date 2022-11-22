#ifndef TEXQUAD_TYPE_H
#define TEXQUAD_TYPE_H

#include "common.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct TexQuad {
    // You can assign an object_id to -1 (not part of any group) or give
    // multiple texquads, zlights, and zvertexes the same object_id to make
    // them behave as a group. You can schedule animations that affect all
    // members in a group the same way. For example, a animation to move all
    // texquads and zlights with object_id of 2 500 pixels to the left over the
    // next 0.2 seconds.
    int32_t object_id;
    
    // This allows you to control how many triangles are used to draw your quad
    // - Set to 1: draw with 1 quad of 2 triangles (top left and bottom right)
    // - Set to 2: draw 2 subquads wide/high = 2x2=4 quads = 8 triangles
    // - Set to 3: draw 3 subquads wide/high = 3x3=9 quads = 18 triangles 
    uint32_t subquads_per_row;
    
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
    
    float left_pixels;
    float top_pixels; // y = window_height for top of screen
                      // y = 0 for right below the screen
    
    float x_offset;   // an additional x offset unaffected by animations that target x/y 
    float y_offset;   // an additional y offset unaffected by animations that target x/y
    
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

#ifdef __cplusplus
}
#endif

#endif // TEXQUAD_TYPE_H
