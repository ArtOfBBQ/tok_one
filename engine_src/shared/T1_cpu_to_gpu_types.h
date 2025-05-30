#ifndef CPU_TO_GPU_TYPES_H
#define CPU_TO_GPU_TYPES_H

#include "T1_cpu_gpu_shared_types.h"
#include "T1_common.h"

// This is a bunch of pointers because apple's metal (both ios and macosx)
// requires shared data to be aligned to page size :(
typedef struct GPUDataForSingleFrame
{
    GPUVertex *                                  vertices;
    GPUSpriteCollection *              polygon_collection;
    GPULight *                                     lights;
    GPUCamera *                                    camera;
    #if RAW_SHADER_ACTIVE
    GPURawVertex *                          line_vertices;
    GPURawVertex *                         point_vertices;
    #endif
    GPUPostProcessingConstants * postprocessing_constants;
    uint32_t                                vertices_size;
    uint32_t                                  lights_size;
    uint32_t                           first_alphablend_i;
    uint32_t                          point_vertices_size;
    uint32_t                           line_vertices_size;
} GPUDataForSingleFrame;

typedef struct GPUSharedDataCollection
{
    GPUDataForSingleFrame triple_buffers[MAX_RENDERING_FRAME_BUFFERS];
    GPUProjectionConstants * locked_pjc;
    GPULockedVertex * locked_vertices;
    GPULockedMaterial * locked_materials;
    uint32_t locked_vertices_size;
    uint32_t locked_materials_size;
    uint32_t locked_materials_allocation_size;
    uint32_t vertices_allocation_size;
    uint32_t locked_vertices_allocation_size;
    uint32_t polygons_allocation_size;
    uint32_t polygon_materials_allocation_size;
    uint32_t lights_allocation_size;
    uint32_t camera_allocation_size;
    uint32_t projection_constants_allocation_size;
    uint32_t line_vertices_allocation_size;
    uint32_t point_vertices_allocation_size;
    uint32_t postprocessing_constants_allocation_size;
    uint32_t frame_i;
} GPUSharedDataCollection;

extern GPUSharedDataCollection * gpu_shared_data_collection;

#endif // CPU_TO_GPU_TYPES_H
