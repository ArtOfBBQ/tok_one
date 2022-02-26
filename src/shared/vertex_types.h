#ifndef VERTEX_TYPES_H
#define VERTEX_TYPES_H

typedef struct ColoredVertex {
    float x;
    float y;
    float RGBA[4];
} ColoredVertex;

typedef struct TexturedVertex {
    float x;
    float y;
    float texture_coordinates[2];
} TexturedVertex;

#endif

