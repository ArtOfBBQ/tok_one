#ifndef VERTEX_BUFFER_TYPES_H
#define VERTEX_BUFFER_TYPES_H

#define MAX_VERTICES_PER_BUFFER 400000 
#define MAX_LIGHTS_PER_BUFFER 100

#include "cpu_gpu_shared_types.h"
#include "common.h"

// This needs to have pointers because apple's
// metal (both ios and macosx) requires shared data
// to be aligned to page size
typedef struct GPUDataForSingleFrame
{
    GPU_Vertex * vertices;
    uint32_t vertices_size;
    GPU_LightCollection * light_collection;
    GPU_Camera * camera;
    GPU_ProjectionConstants * projection_constants;
} GPUDataForSingleFrame;

typedef struct GPUSharedDataCollection
{
    uint32_t frame_i;
    GPUDataForSingleFrame triple_buffers[3];
} GPUSharedDataCollection;

#endif
