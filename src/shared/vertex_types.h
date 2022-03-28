#ifndef VERTEX_TYPES_H
#define VERTEX_TYPES_H

#define TEXTUREARRAYS_SIZE 4

#pragma pack(push, 1)
typedef struct Vertex {
    float x;
    float y;
    float z;
    float uv[2];
    float RGBA[4];
    float lighting;    // multiply by this lighting after
                       // color/texture
    int32_t texturearray_i; // -1 for no texture
    int32_t texture_i; // -1 for no texture
}  Vertex;
#pragma pack(pop)

#endif

