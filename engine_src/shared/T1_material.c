#include "T1_material.h"

LockedMaterialCollection * all_mesh_materials = NULL;

void T1_material_init(
    void * (* arg_malloc_function)(size_t size))
{
    all_mesh_materials = arg_malloc_function(sizeof(LockedMaterialCollection));
    all_mesh_materials->size = 0;
}

#ifndef LOGGER_IGNORE_ASSERTS
static uint32_t T1_material_fetch_locked_material_i(
    const char * material_name)
{
    for (uint32_t i = 0; i < all_mesh_materials->size; i++) {
        if (
            common_are_equal_strings(
                material_name,
                all_mesh_materials->material_names[i]))
        {
            return i;
        }
    }
    
    return UINT32_MAX;
}
#endif

uint32_t T1_material_preappend_locked_material_i(
    const char * material_name)
{
    #ifndef LOGGER_IGNORE_ASSERTS
    uint32_t existing_i = T1_material_fetch_locked_material_i(material_name);
    if (!common_are_equal_strings(material_name, "default")) {
        log_assert(existing_i == UINT32_MAX); // doesn't exist
    }
    #endif
    
    common_strcpy_capped(
        all_mesh_materials->material_names[all_mesh_materials->size],
        MATERIAL_NAME_CAP,
        material_name);
    
    all_mesh_materials->size += 1;
    return all_mesh_materials->size - 1;
}

GPULockedMaterial * T1_material_fetch_ptr(
    const uint32_t material_i)
{
    log_assert(material_i < all_mesh_materials->size);
    
    return all_mesh_materials->gpu_data + material_i;
}
