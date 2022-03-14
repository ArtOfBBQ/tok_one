#ifndef VERTEX_TYPES_H
#define VERTEX_TYPES_H

#define TEXTURES_SIZE 1

#pragma pack(push, 0)
typedef struct Vertex {
    float x;
    float y;
    float uv[2];
    float RGBA[4];
    float lighting;    // multiply by this lighting after
                       // color/texture
    int32_t texture_i; // -1 for no texture
}  Vertex;
#pragma pack(pop)

#endif

