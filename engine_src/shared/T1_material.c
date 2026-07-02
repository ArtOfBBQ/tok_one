#include "T1_material.h"

#include "T1_std.h"
#include "T1_log.h"
#include "T1_tex.h"

LockedMaterialCollection * all_mesh_materials = NULL;

void T1_material_init(
    void * (* arg_malloc_function)(u64 size))
{
    all_mesh_materials = arg_malloc_function(sizeof(LockedMaterialCollection));
    all_mesh_materials->size = 0;
}

void T1_material_construct(
    T1GPUConstMatf32 * to_construct_f32,
    T1GPUConstMats32 * to_construct_s32)
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
    // this code is annoying & dangerous because
    // it's doing bitwise ops with <4 byte primtives
    // (integer promotion) and mixing signed/unsigned
    // you should review it when normal mapping comes
    // back online
    T1_log_assert(0);
    to_construct_s32->normalmap_tex_and_tex = 0;
    T1Tex tex_none = T1_TEX_NONE;
    u32 tex_none_u32 = tex_none;
    s32 tex_none_s32;
    T1_std_memcpy(&tex_none_s32, &tex_none_u32, sizeof(s32));
     
    to_construct_s32->normalmap_tex_and_tex |=
        ((tex_none_s32 << 16) & tex_none_s32);
    #elif T1_NORMAL_MAPPING_ACTIVE == T1_INACTIVE
    to_construct_s32->normalmap_tex_and_tex =
        (T1_TEX_NONE << 16);
    to_construct_s32->normalmap_tex_and_tex_u32 |=
        T1_TEX_NONE;
    T1Tex check_tex = (to_construct_s32->normalmap_tex_and_tex_u32 & 0x0000FFFF); 
    s32 check_arr_i = T1_tex_to_array_i(check_tex);
    T1_log_assert(check_arr_i == -1);
    s32 check_slice_i = T1_tex_to_slice_i(check_tex);
    T1_log_assert(check_slice_i == -1);
    #else
    #error
    #endif
}

#if T1_LOG_ASSERTS_ACTIVE == T1_ACTIVE
static u32 T1_material_fetch_locked_material_i(
    const char * material_name)
{
    for (u32 i = 0; i < all_mesh_materials->size; i++) {
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
#elif T1_LOG_ASSERTS_ACTIVE == T1_INACTIVE
#else
#error
#endif

u32 T1_material_preappend_locked_material_i(
    const char * obj_resource_name,
    const char * material_name)
{
    #if T1_LOG_ASSERTS_ACTIVE == T1_ACTIVE
    u32 existing_i = T1_material_fetch_locked_material_i(material_name);
    if (!T1_std_are_equal_strings(material_name, "default")) {
        T1_log_assert(existing_i == UINT32_MAX); // doesn't exist
    }
    #elif T1_LOG_ASSERTS_ACTIVE == T1_INACTIVE
    #else
    #error
    #endif
    
    T1_std_strcpy_cap(
        all_mesh_materials->material_names[all_mesh_materials->size],
        T1_MATERIAL_NAME_CAP,
        obj_resource_name);
    T1_std_strcat_cap(
        all_mesh_materials->material_names[all_mesh_materials->size],
        T1_MATERIAL_NAME_CAP,
        "_");
    T1_std_strcat_cap(
        all_mesh_materials->material_names[all_mesh_materials->size],
        T1_MATERIAL_NAME_CAP,
        material_name);
    
    T1_log_assert(all_mesh_materials->size < T1_ALL_LOCKED_MATERIALS_SIZE);
    
    all_mesh_materials->size += 1;
    return all_mesh_materials->size - 1;
}

void T1_material_fetch_ptrs(
    T1GPUConstMatf32 ** recip_f32,
    T1GPUConstMats32 ** recip_s32,
    const u32 material_i)
{
    T1_log_assert(material_i < all_mesh_materials->size);
    
    *recip_f32 = all_mesh_materials->gpu_f32 +
        material_i;
    *recip_s32 = all_mesh_materials->gpu_s32 +
        material_i;
}
