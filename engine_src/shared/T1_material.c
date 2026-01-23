#include "T1_material.h"

LockedMaterialCollection * all_mesh_materials = NULL;

void T1_material_init(
    void * (* arg_malloc_function)(size_t size))
{
    all_mesh_materials = arg_malloc_function(sizeof(LockedMaterialCollection));
    all_mesh_materials->size = 0;
}

void T1_material_construct(
    T1GPUConstMatf32 * to_construct_f32,
    T1GPUConstMati32 * to_construct_i32)
{
    T1_std_memset(
        to_construct_f32,
        0,
        sizeof(T1GPUConstMatf32));
    
    to_construct_f32->alpha = 1.0f;
    to_construct_f32->ambient_rgb[0] = 0.20f;
    to_construct_f32->ambient_rgb[1] = 0.20f;
    to_construct_f32->ambient_rgb[2] = 0.20f;
    to_construct_f32->diffuse_rgb[0] = 0.80f;
    to_construct_f32->diffuse_rgb[1] = 0.80f;
    to_construct_f32->diffuse_rgb[2] = 0.80f;
    to_construct_f32->specular_rgb[0] = 0.50f;
    to_construct_f32->specular_rgb[1] = 0.50f;
    to_construct_f32->specular_rgb[2] = 0.50f;
    to_construct_f32->specular_exponent = 25.0f;
    #if T1_NORMAL_MAPPING_ACTIVE == T1_ACTIVE
    to_construct_i32->normalmap_texture_i = -1;
    to_construct_i32->normalmap_texturearray_i = -1;
    #elif T1_NORMAL_MAPPING_ACTIVE == T1_INACTIVE
    #else
    #error
    #endif
    to_construct_i32->texturearray_i = -1;
    to_construct_i32->texture_i = -1;
}

#if T1_LOGGER_ASSERTS_ACTIVE == T1_ACTIVE
static uint32_t T1_material_fetch_locked_material_i(
    const char * material_name)
{
    for (uint32_t i = 0; i < all_mesh_materials->size; i++) {
        if (
            T1_std_are_equal_strings(
                material_name,
                all_mesh_materials->material_names[i]))
        {
            return i;
        }
    }
    
    return UINT32_MAX;
}
#elif T1_LOGGER_ASSERTS_ACTIVE == T1_INACTIVE
#else
#error
#endif

uint32_t T1_material_preappend_locked_material_i(
    const char * obj_resource_name,
    const char * material_name)
{
    #if T1_LOGGER_ASSERTS_ACTIVE == T1_ACTIVE
    uint32_t existing_i = T1_material_fetch_locked_material_i(material_name);
    if (!T1_std_are_equal_strings(material_name, "default")) {
        log_assert(existing_i == UINT32_MAX); // doesn't exist
    }
    #elif T1_LOGGER_ASSERTS_ACTIVE == T1_INACTIVE
    #else
    #error
    #endif
    
    T1_std_strcpy_cap(
        all_mesh_materials->material_names[all_mesh_materials->size],
        MATERIAL_NAME_CAP,
        obj_resource_name);
    T1_std_strcat_cap(
        all_mesh_materials->material_names[all_mesh_materials->size],
        MATERIAL_NAME_CAP,
        "_");
    T1_std_strcat_cap(
        all_mesh_materials->material_names[all_mesh_materials->size],
        MATERIAL_NAME_CAP,
        material_name);
    
    log_assert(all_mesh_materials->size < ALL_LOCKED_MATERIALS_SIZE);
    
    all_mesh_materials->size += 1;
    return all_mesh_materials->size - 1;
}

void T1_material_fetch_ptrs(
    T1GPUConstMatf32 ** recip_f32,
    T1GPUConstMati32 ** recip_i32,
    const uint32_t material_i)
{
    log_assert(material_i < all_mesh_materials->size);
    
    *recip_f32 = all_mesh_materials->gpu_f32 +
        material_i;
    *recip_i32 = all_mesh_materials->gpu_i32 +
        material_i;
}
