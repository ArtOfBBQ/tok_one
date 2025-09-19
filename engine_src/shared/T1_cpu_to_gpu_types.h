#ifndef CPU_TO_GPU_TYPES_H
#define CPU_TO_GPU_TYPES_H

#include "T1_cpu_gpu_shared_types.h"
#include "T1_std.h"

// This is a bunch of pointers because apple's metal (both ios and macosx)
// requires shared data to be aligned to page size :(
typedef struct
{
    T1GPUVertexIndices *                            verts;
    T1GPUzSpriteList *                       zsprite_list;
    T1GPULight *                                   lights;
    T1GPUCamera *                                  camera;
    #if T1_RAW_SHADER_ACTIVE == T1_ACTIVE
    T1GPURawVertex *                        line_vertices;
    T1GPURawVertex *                       point_vertices;
    #elif T1_RAW_SHADER_ACTIVE == T1_INACTIVE
    #else
    #error
    #endif
    T1GPUPostProcConsts                 * postproc_consts;
    uint32_t                                   verts_size;
    uint32_t                                  lights_size;
    uint32_t                           first_alphablend_i;
    uint32_t                          point_vertices_size;
    uint32_t                           line_vertices_size;
} T1GPUFrame;

typedef struct
{
    T1GPUFrame triple_buffers[MAX_RENDERING_FRAME_BUFFERS];
    T1GPUProjectConsts * locked_pjc;
    T1GPULockedVertex * locked_vertices;
    T1GPUConstMat * const_mats;
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
} T1GPUSharedDataCollection;

extern T1GPUSharedDataCollection * gpu_shared_data_collection;

#endif // CPU_TO_GPU_TYPES_H
