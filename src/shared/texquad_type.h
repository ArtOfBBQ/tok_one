#ifndef TEXQUAD_TYPE_H
#define TEXQUAD_TYPE_H

typedef struct TexQuad {
    // the object_id this texquad is
    // associated with. You can send a
    // request to delete all texquads with
    // this object_id, or fade them all out
    // etc.
    uint32_t object_id; 
    
    // the index of the texturearray,
    // aka texture atlas, to
    // texture-map to this quad
    // use '-1' for 'no texture'
    int32_t texturearray_i; 

    // the index of the texture inside the
    // texturearray to texture-map to this
    // quad. If the texture atlas is just 1
    // big image, use 0
    // use '-1' for 'no texture'
    int32_t texture_i; 
    
    // the color of this quad
    // when combined with a texture, the texture
    // will be mixed with this color
    float RGBA[4];
    
    float left_pixels;
    float top_pixels;
    float height_pixels;
    float width_pixels;
    float z;
    float z_angle;
    bool32_t visible; // skip rendering me but keep me in memory
    bool32_t deleted; // overwrite me if true
} TexQuad;

#endif

