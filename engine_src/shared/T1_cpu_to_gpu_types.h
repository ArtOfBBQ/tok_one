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
    T1GPURenderView *        render_views[T1_RENDER_VIEW_CAP];
    T1GPUPostProcConsts * postproc_consts;
    T1GPUFlatQuad *       flat_billboard_quads;
    uint32_t              verts_size;
    uint32_t              render_views_size;
    uint32_t              flat_billboard_quads_size;
    uint32_t              opaq_verts_size;
    uint32_t              first_alpha_i;
    uint32_t              alpha_verts_size;
    uint32_t              first_bloom_i;
    uint32_t              bloom_verts_size;
} T1GPUFrame;

typedef struct
{
    T1GPUFrame triple_buffers[MAX_FRAME_BUFFERS];
    T1GPUProjectConsts * locked_pjc;
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
    uint32_t render_view_alloc_size;
    uint32_t projection_constants_alloc_size;
    uint32_t postprocessing_constants_alloc_size;
    uint32_t frame_i;
} T1GPUSharedDataCollection;

extern T1GPUSharedDataCollection * gpu_shared_data_collection;

#endif // CPU_TO_GPU_TYPES_H
