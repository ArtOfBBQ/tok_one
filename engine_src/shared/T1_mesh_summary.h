#ifndef T1_MESH_H
#define T1_MESH_H

#include <stdint.h>

#include "T1_cpu_to_gpu_types.h"

// Basic quads and cubes are predefined, they can be used without registering
// an .obj file.
#define T1_BASIC_QUAD_MESH_ID 0
#define T1_BASIC_CUBE_MESH_ID 1

#define OBJ_STRING_SIZE 128
typedef struct MeshSummary {
    char resource_name[OBJ_STRING_SIZE]; // resource filename
    int32_t mesh_id;
    int32_t vertices_head_i;
    int32_t vertices_size;
    float base_width;
    float base_height;
    float base_depth;
    int32_t shattered_vertices_head_i; // -1 if no shattered version
    int32_t shattered_vertices_size; // 0 if no shattered version
    uint32_t locked_material_head_i;
    uint32_t locked_material_base_offset; // UINT32_MAX = no base mat
    uint32_t materials_size;
} MeshSummary;

extern MeshSummary * T1_mesh_summary_list;
extern uint32_t T1_mesh_summary_list_size;

typedef struct LockedVertexWithMaterialCollection {
    T1GPULockedVertex gpu_data[ALL_LOCKED_VERTICES_SIZE];
    uint32_t size;
} LockedVertexWithMaterialCollection;

extern LockedVertexWithMaterialCollection * T1_mesh_summary_all_vertices;

#endif // T1_MESH_H
