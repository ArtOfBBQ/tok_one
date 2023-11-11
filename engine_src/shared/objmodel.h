#ifndef OBJMODEL_H
#define OBJMODEL_H

#include "common.h"
#include "logger.h"
#include "triangle.h"
#include "objparser.h"

#include "memorystore.h"

#ifdef __cplusplus
extern "C" {
#endif

#define OBJ_STRING_SIZE 128
typedef struct MeshSummary {
    char resource_name[OBJ_STRING_SIZE]; // the resource filename (without path)
    int32_t mesh_id;
    int32_t vertices_head_i;
    int32_t vertices_size;
    float base_width;
    float base_height;
    float base_depth;
    int32_t shattered_vertices_head_i; // -1 if no shattered version
    int32_t shattered_vertices_size; // 0 if no shattered version
    char material_names[MAX_MATERIALS_PER_POLYGON][OBJ_STRING_SIZE];
    uint32_t materials_size;
} MeshSummary;

typedef struct GPULockedVertexWithMaterial {
    GPULockedVertex gpu_data;
    uint32_t parent_material_i;
} GPULockedVertexWithMaterial;

extern MeshSummary * all_mesh_summaries;
extern uint32_t all_mesh_summaries_size;

extern GPULockedVertexWithMaterial * all_mesh_vertices;
extern uint32_t all_mesh_vertices_size;

void init_all_meshes(void);

int32_t new_mesh_id_from_resource(
    const char * filename);

void center_mesh_offsets(
    const int32_t mesh_id);

/*
Creates a version of the mesh with (normally needless) extra triangles

This can be useful if you have a very large flat area that needs to catch
lights in between its vertices, or if you plan to 'shatter' or explode the
mesh into many pieces.

After running this function, the new triangles can be found in:
all_mesh_summaries[your_mesh_id].shattered_triangles_head_i;
all_mesh_summaries[your_mesh_id].shattered_triangles_size;
*/
void create_shattered_version_of_mesh(
    const int32_t mesh_id,
    const uint32_t triangles_mulfiplier);

#ifdef __cplusplus
}
#endif

#endif // OBJMODEL_H
