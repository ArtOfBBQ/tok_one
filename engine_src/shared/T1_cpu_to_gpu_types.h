#ifndef CPU_TO_GPU_TYPES_H
#define CPU_TO_GPU_TYPES_H

#include "T1_cpu_gpu_shared_types.h"
#include "T1_std.h"

// This is a bunch of pointers because apple's metal (both ios and macosx)
// requires shared data to be aligned to page size :(
typedef struct
{
    T1GPUVertexIndices *  verts;
    T1GPUzSpriteList *    zsprite_list;
    T1GPULight *          lights;
    T1GPURenderView *     render_views;
    T1GPUPostProcConsts * postproc_consts;
    T1GPUFlatQuad *       flat_bb_quads;
    
    uint32_t              verts_size;
    uint32_t              render_views_size;
    uint32_t              flat_bb_quads_size;
} T1GPUFrame;

typedef struct
{
    T1GPUFrame triple_buffers[FRAMES_CAP];
    T1GPULockedVertex * locked_vertices;
    T1GPUConstMat * const_mats;
    uint32_t locked_vertices_size;
    uint32_t const_mats_size;
    uint32_t const_mats_alloc_size;
    uint32_t vertices_alloc_size;
    uint32_t flat_quads_alloc_size;
    uint32_t locked_vertices_alloc_size;
    uint32_t polygons_alloc_size;
    uint32_t polygon_mats_alloc_size;
    uint32_t lights_alloc_size;
    uint32_t render_views_alloc_size;
    uint32_t postprocessing_constants_alloc_size;
    uint32_t frame_i;
} T1GPUSharedDataCollection;

extern T1GPUSharedDataCollection * gpu_shared_data_collection;

#endif // CPU_TO_GPU_TYPES_H
