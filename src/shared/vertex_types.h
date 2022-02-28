#ifndef VERTEX_TYPES_H
#define VERTEX_TYPES_H

typedef struct Vertex {
    float x;
    float y;
    float uv[2];
    float RGBA[4];
    int32_t texture_i; // -1 for no texture
}  Vertex;

#endif

