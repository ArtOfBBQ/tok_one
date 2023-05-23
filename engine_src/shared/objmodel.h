#ifndef OBJMODEL_H
#define OBJMODEL_H

#include "common.h"
#include "logger.h"
#include "triangle.h"

#include "memorystore.h"

#ifdef __cplusplus
extern "C" {
#endif

#define OBJ_STRING_SIZE 128
typedef struct MeshSummary {
    char resource_name[OBJ_STRING_SIZE]; // the resource filename (without path)
    int32_t mesh_id;
    int32_t triangles_head_i;
    int32_t triangles_size;
    float base_width;
    float base_height;
    float base_depth;
    int32_t shattered_triangles_head_i; // -1 if no shattered version
    int32_t shattered_triangles_size; // 0 if no shattered version
    char material_names[MAX_MATERIALS_SIZE][OBJ_STRING_SIZE];
    uint32_t materials_size;
} MeshSummary;

extern MeshSummary * all_mesh_summaries;
extern uint32_t all_mesh_summaries_size;

extern zTriangle * all_mesh_triangles;
extern uint32_t all_mesh_triangles_size;

void init_all_meshes(void);

/*
The base version of parse_obj will just ignore materials and set all texture_i
's to -1, all texturearray_i's to -1, and all colors to 1.0f.

If you need more specific behavior, pass an array of ExpectedObjMaterials to
set textures or colors

For example, you could set material_name to 'marble' and set texturearray_i 5
and texture_i 1 if you had labeled some faces of your object with the material
'marble' in Blender and your marble texture was stored at 5,1.
You could set the texturearray_i and texture_i to -1 and use a color for
another material name, etc.
*/
//typedef struct ExpectedObjMaterials {
//    char material_name[16];
//    int32_t texturearray_i;
//    int32_t texture_i;
//    float rgba[4];
//} ExpectedObjMaterials;

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
