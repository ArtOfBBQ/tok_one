#ifndef T1_TYPES_CPU_TO_GPU_H
#define T1_TYPES_CPU_TO_GPU_H

#include <stdint.h>

#include "T1_types_gpucpu.h"
#include "T1_types_public_gpucpu.h"

typedef struct {
    u32 T1_id;
    u32 touch_id;
} IdPair;

/*
This is a bunch of pointers because apple's metal
(both ios and macosx) requires shared data to be aligned to page size :(
*/
typedef struct
{
    T1GPUVertexIndices *  verts;
    T1GPUzSpriteList *    zsprite_list;
    T1GPULight *          lights;
    T1GPURenderView *     render_views;
    T1GPUPostProcConsts * postproc_consts;
    T1GPUFlatQuad *       flat_bb_quads;
    T1GPUTexQuad *        flat_tex_quads;
    T1GPUzSpriteMatrices matrices[T1_ZSPRITES_CAP];
    IdPair                id_pairs[T1_ZSPRITES_CAP];
    u32              verts_size;
    u32              render_views_size;
    u32              flat_bb_quads_size;
    u32              flat_tex_quads_size;
} T1GPUFrame;

typedef struct
{
    T1GPUFrame triple_buffers[T1_FRAMES_CAP];
    T1GPULockedVertex * locked_vertices;
    T1GPUMatf32 * const_mats_f32;
    T1GPUMatu32 * const_mats_s32;
    u32 locked_vertices_size;
    u32 const_mats_size;
    u32 const_matsf32_alloc_size;
    u32 const_matss32_alloc_size;
    u32 vertices_alloc_size;
    u32 flat_quads_alloc_size;
    u32 flat_texquads_alloc_size;
    u32 locked_vertices_alloc_size;
    u32 polygons_alloc_size;
    u32 matrices_alloc_size;
    u32 polygon_mats_alloc_size;
    u32 lights_alloc_size;
    u32 render_views_alloc_size;
    u32 postprocessing_constants_alloc_size;
    u32 frame_i;
} T1CPUToGPUData;

extern T1CPUToGPUData * T1_cpu_to_gpu_data;

#endif // T1_TYPES_CPU_TO_GPU_H
