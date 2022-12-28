#ifndef VERTEX_BUFFER_TYPES_H
#define VERTEX_BUFFER_TYPES_H

#define MAX_VERTICES_PER_BUFFER 110000 
#define MAX_LIGHTS_PER_BUFFER 100

#include "vertex_types.h"
#include "common.h"

typedef struct BufferedVertexCollection
{
    Vertex * vertices;
    uint32_t vertices_size;
    LightCollection * light_collection;
} BufferedVertexCollection;

typedef struct VertexBuffer
{
    uint32_t frame_i;
    BufferedVertexCollection vertex_buffers[3];
} VertexBuffer;

#endif
