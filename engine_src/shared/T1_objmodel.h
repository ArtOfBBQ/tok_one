#ifndef OBJMODEL_H
#define OBJMODEL_H

#include "T1_stdint.h"

// ******************************************************
// **                 Client functions                 **
// ******************************************************

// This functions returns a mesh_id
// Use it in clientlogic_early_startup()
s32 T1_objmodel_new_mesh_id_from_resources(
    const char * filename,
    const char * mtl_filename,
    const u8 flip_uv_u,
    const u8 flip_uv_v,
    u8 * success,
    char * error_message);

s32 T1_objmodel_resource_name_to_mesh_id(
    const char * obj_filename);
f32 T1_objmodel_get_x_multiplier_for_width(
    const s32 mesh_id,
    const f32 screenspace_width,
    const f32 given_z);
f32 T1_objmodel_get_y_multiplier_for_height(
    const s32 mesh_id,
    const f32 screenspace_height,
    const f32 given_z);

/*
**********************************************************************
**                    Internal functions (ignore)                   **
**********************************************************************
*/
void T1_objmodel_init(void);

s32 T1_objmodel_new_mesh_id_from_obj_mtl_text(
    const char * original_obj_filename,
    const char * obj_text,
    const char * mtl_text);

void T1_objmodel_center_mesh_offsets(const s32 mesh_id);

void T1_objmodel_flip_mesh_uvs(const s32 mesh_id);
void T1_objmodel_flip_mesh_uvs_u(const s32 mesh_id);
void T1_objmodel_flip_mesh_uvs_v(const s32 mesh_id);

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
    const s32 mesh_id,
    const u32 triangles_mulfiplier);

#endif // OBJMODEL_H
