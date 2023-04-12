#ifndef OBJMODEL_H
#define OBJMODEL_H

#include "common.h"
#include "logger.h"
#include "triangle.h"

#ifdef __cplusplus
extern "C" {
#endif

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
void parse_obj(
    char * rawdata,
    uint64_t rawdata_size,
    const bool32_t flip_winding,
    zTriangle * recipient,
    uint32_t * recipient_size);
typedef struct ExpectedObjMaterials {
    char material_name[16];
    int32_t texturearray_i;
    int32_t texture_i;
    float rgba[4];
} ExpectedObjMaterials;
void parse_obj_expecting_materials(
    char * rawdata,
    uint64_t rawdata_size,
    ExpectedObjMaterials * expected_materials,
    const uint32_t expected_materials_size,
    const bool32_t flip_winding,
    zTriangle * recipient,
    uint32_t * recipient_size);

extern zTriangle * all_meshes;
extern uint32_t all_meshes_size;

int32_t new_mesh_head_id_from_resource(
    const char * filename);

#ifdef __cplusplus
}
#endif

#endif // OBJMODEL_H
