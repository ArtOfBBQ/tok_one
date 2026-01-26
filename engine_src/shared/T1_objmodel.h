#ifndef OBJMODEL_H
#define OBJMODEL_H

#include <math.h>

#include "T1_cpu_gpu_shared_types.h"

#include "T1_std.h"
#include "T1_mem.h"
#include "T1_tex.h"
#include "T1_global.h"
#include "T1_logger.h"

#include "T1_triangle.h"
#include "T1_texture_array.h"

#include "T1_objparser.h"
#include "T1_mtlparser.h"
// #include "T1_platform_layer.h"
#include "T1_material.h"
#include "T1_mesh_summary.h"

// **********************************************************************
// **                    Client functions                              **
// **********************************************************************

// This functions returns a mesh_id
// Use it in clientlogic_early_startup()
int32_t T1_objmodel_new_mesh_id_from_resources(
    const char * filename,
    const char * mtl_filename,
    const bool32_t flip_uv_u,
    const bool32_t flip_uv_v,
    bool32_t * success,
    char * error_message);

int32_t T1_objmodel_resource_name_to_mesh_id(
    const char * obj_filename);
float T1_objmodel_get_x_multiplier_for_width(
    const int32_t mesh_id,
    const float screenspace_width,
    const float given_z);
float T1_objmodel_get_y_multiplier_for_height(
    const int32_t mesh_id,
    const float screenspace_height,
    const float given_z);

/*
**********************************************************************
**                    Internal functions (ignore)                   **
**********************************************************************
*/
#define MATERIAL_NAMES_MAX 20

void T1_objmodel_init(void);

int32_t T1_objmodel_new_mesh_id_from_obj_mtl_text(
    const char * original_obj_filename,
    const char * obj_text,
    const char * mtl_text);

void T1_objmodel_center_mesh_offsets(const int32_t mesh_id);

void T1_objmodel_flip_mesh_uvs(const int32_t mesh_id);
void T1_objmodel_flip_mesh_uvs_u(const int32_t mesh_id);
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

#endif // OBJMODEL_H
