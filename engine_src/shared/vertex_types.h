#ifndef VERTEX_TYPES_H
#define VERTEX_TYPES_H

#define TEXTUREARRAYS_SIZE 31

#pragma pack(push, 1)
typedef struct Vertex {
    float x;
    float y;
    float z;
    float w;
    float uv[2];
    float RGBA[4];
    float lighting[4];    // multiply by this lighting after
                          // color/texture
    int texturearray_i; // -1 for no texture
    int texture_i;      // -1 for no texture
    int touchable_id;
}  Vertex;
#pragma pack(pop)

#endif
