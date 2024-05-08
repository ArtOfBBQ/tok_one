#ifndef CPU_TO_GPU_TYPES_H
#define CPU_TO_GPU_TYPES_H

#include "cpu_gpu_shared_types.h"
#include "common.h"
#include "clientlogic_macro_settings.h"

// This is a bunch of pointers because apple's metal (both ios and macosx)
// requires shared data to be aligned to page size :(
typedef struct GPUDataForSingleFrame
{
    GPUVertex *                            vertices;
    GPUPolygonCollection *       polygon_collection;
    GPUPolygonMaterial *          polygon_materials;
    GPUCamera *                              camera;
    GPULightCollection *           light_collection;
    uint32_t                          vertices_size;
    uint32_t                           first_line_i;
    uint32_t                     first_alphablend_i;
} GPUDataForSingleFrame;

typedef struct GPUSharedDataCollection
{
    GPUDataForSingleFrame triple_buffers[3];
    GPUProjectionConstants * locked_pjc;
    GPULockedVertex * locked_vertices;
    uint32_t locked_vertices_size;
    uint32_t vertices_allocation_size;
    uint32_t locked_vertices_allocation_size;
    uint32_t polygons_allocation_size;
    uint32_t polygon_materials_allocation_size;
    uint32_t lights_allocation_size;
    uint32_t camera_allocation_size;
    uint32_t projection_constants_allocation_size;
    uint32_t frame_i;
} GPUSharedDataCollection;

extern GPUSharedDataCollection gpu_shared_data_collection;

#endif // CPU_TO_GPU_TYPES_H
