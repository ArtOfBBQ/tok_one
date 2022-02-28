#include "vertex_types.h"

#ifndef VERTEX_BUFFER_TYPES_H
#define VERTEX_BUFFER_TYPES_H

typedef struct BufferedVertexCollection
{
    Vertex * vertices;
    uint32_t vertices_size;
} BufferedVertexCollection;

typedef struct VertexBuffer
{
    uint32_t frame_i;
    BufferedVertexCollection vertex_buffers[3];
} VertexBuffer;

#endif

