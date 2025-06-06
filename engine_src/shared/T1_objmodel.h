#ifndef OBJMODEL_H
#define OBJMODEL_H

#include <math.h>

#include "T1_cpu_gpu_shared_types.h"

#include "T1_common.h"
#include "T1_logger.h"
#include "T1_triangle.h"
#include "T1_texture_array.h"
#include "T1_objparser.h"
#include "T1_mtlparser.h"
#include "T1_platform_layer.h"
#include "T1_memorystore.h"
#include "T1_material.h"


#ifdef __cplusplus
extern "C" {
#endif

// **********************************************************************
// **                    Client functions                              **
// **********************************************************************

// Basic quads and cubes are predefined, they can be used without registering
// an .obj file.
#define BASIC_QUAD_MESH_ID 0
#define BASIC_CUBE_MESH_ID 1

// This functions returns a mesh_id
// Use it in clientlogic_early_startup()
int32_t T1_objmodel_new_mesh_id_from_resources(
    const char * filename,
    const char * mtl_filename,
    const bool32_t flip_uv_v);


// **********************************************************************
// **                    Internal functions (ignore)                   **
// **********************************************************************

#define MATERIAL_NAMES_MAX 20

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
    uint32_t locked_material_head_i;
    uint32_t materials_size;
} MeshSummary;

typedef struct LockedVertexWithMaterialCollection {
    GPULockedVertex gpu_data[ALL_LOCKED_VERTICES_SIZE];
    uint32_t size;
} LockedVertexWithMaterialCollection;

extern MeshSummary * all_mesh_summaries;
extern uint32_t all_mesh_summaries_size;

extern LockedVertexWithMaterialCollection * all_mesh_vertices;

void T1_objmodel_init(void);

int32_t T1_objmodel_new_mesh_id_from_obj_mtl_text(
    const char * obj_text,
    const char * mtl_text);

void T1_objmodel_center_mesh_offsets(const int32_t mesh_id);

void T1_objmodel_flip_mesh_uvs(const int32_t mesh_id);
void T1_objmodel_flip_mesh_uvs_v(const int32_t mesh_id);

/*
Creates a version of the mesh with (normally needless) extra triangles

This can be useful if you have a very large flat area that needs to catch
lights in between its vertices, or if you plan to 'shatter' or explode the
mesh into many pieces.

After running this function, the new triangles can be found in:
all_mesh_summaries[your_mesh_id].shattered_triangles_head_i;
all_mesh_summaries[your_mesh_id].shattered_triangles_size;
*/
void T1_objmodel_create_shattered_version_of_mesh(
    const int32_t mesh_id,
    const uint32_t triangles_mulfiplier);

#ifdef __cplusplus
}
#endif

#endif // OBJMODEL_H
