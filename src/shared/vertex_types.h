#ifndef VERTEX_TYPES_H
#define VERTEX_TYPES_H

#include <simd/simd.h>

typedef struct ColoredVertex
{
    simd_float2 XY;
    simd_float4 RGBA;
} ColoredVertex;

typedef struct BufferedVertex
{
    ColoredVertex * vertices;
    uint32_t size;
} BufferedVertex;

typedef struct VertexBuffer
{
    uint32_t frame_i;
    BufferedVertex vertex_buffers[3];
} VertexBuffer;

#endif

