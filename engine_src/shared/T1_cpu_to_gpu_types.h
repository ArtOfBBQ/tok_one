#ifndef CPU_TO_GPU_TYPES_H
#define CPU_TO_GPU_TYPES_H

#include "T1_cpu_gpu_shared_types.h"
#include "T1_common.h"

// This is a bunch of pointers because apple's metal (both ios and macosx)
// requires shared data to be aligned to page size :(
typedef struct
{
    GPUVertexIndices *                              verts;
    GPUzSpriteList *                         zsprite_list;
    GPULight *                                     lights;
    GPUCamera *                                    camera;
    #if RAW_SHADER_ACTIVE
    GPURawVertex *                          line_vertices;
    GPURawVertex *                         point_vertices;
    #endif
    GPUPostProcConsts                   * postproc_consts;
    uint32_t                                   verts_size;
    uint32_t                                  lights_size;
    uint32_t                           first_alphablend_i;
    uint32_t                          point_vertices_size;
    uint32_t                           line_vertices_size;
} GPUFrame;

typedef struct GPUSharedDataCollection
{
    GPUFrame triple_buffers[MAX_RENDERING_FRAME_BUFFERS];
    GPUProjectConsts * locked_pjc;
    GPULockedVertex * locked_vertices;
    GPUConstMat * const_mats;
    uint32_t locked_vertices_size;
    uint32_t const_mats_size;
    uint32_t const_mats_allocation_size;
    uint32_t vertices_allocation_size;
    uint32_t locked_vertices_allocation_size;
    uint32_t polygons_allocation_size;
    uint32_t polygon_mats_allocation_size;
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
