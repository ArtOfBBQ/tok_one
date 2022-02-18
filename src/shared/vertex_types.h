#ifndef VERTEX_TYPES_H
#define VERTEX_TYPES_H

typedef struct ColoredVertex {
    float x;
    float y;
    float RGBA[4];
} ColoredVertex;

typedef struct BufferedVertexCollection
{
    ColoredVertex * vertices;
    uint32_t size;
} BufferedVertexCollection;

typedef struct VertexBuffer
{
    uint32_t frame_i;
    BufferedVertexCollection vertex_buffers[3];
} VertexBuffer;

#endif

