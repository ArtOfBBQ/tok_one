#ifndef T1_MESH_H
#define T1_MESH_H

#include <stdint.h>

#include "T1_cpu_to_gpu.h"

#define T1_OBJ_STRING_SIZE 128
typedef struct {
    char resource_name[T1_OBJ_STRING_SIZE]; // resource filename
    s32 mesh_id;
    s32 vertices_head_i;
    s32 vertices_size;
    f32 base_width;
    f32 base_height;
    f32 base_depth;
    s32 shattered_vertices_head_i; // -1 if no shattered version
    s32 shattered_vertices_size; // 0 if no shattered version
    u32 locked_material_head_i;
    u32 locked_material_base_offset; // UINT32_MAX = no base mat
    u32 materials_size;
} T1MeshSummary;

extern T1MeshSummary * T1_mesh_summary_list;
extern u32 T1_mesh_summary_list_size;

typedef struct {
    T1GPULockedVertex gpu_data[T1_LOCKED_VERTEX_CAP];
    u32 size;
} T1LockedVertexWithMaterialCollection;

extern T1LockedVertexWithMaterialCollection * T1_mesh_summary_all_vertices;

#endif // T1_MESH_H
