#include "T1_material.h"

LockedMaterialCollection * all_mesh_materials = NULL;

void T1_material_init(
    void * (* arg_malloc_function)(size_t size))
{
    all_mesh_materials = arg_malloc_function(sizeof(LockedMaterialCollection));
    all_mesh_materials->size = 0;
}
