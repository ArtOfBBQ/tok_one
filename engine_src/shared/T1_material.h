#ifndef T1_MATERIAL_H
#define T1_MATERIAL_H

#include <stddef.h>
#include <stdint.h>

#include "T1_cpu_gpu_shared_types.h"

typedef struct LockedMaterialCollection {
    T1GPUConstMatf32 gpu_f32[T1_ALL_LOCKED_MATERIALS_SIZE];
    T1GPUConstMats32 gpu_s32[T1_ALL_LOCKED_MATERIALS_SIZE];
    char material_names[T1_ALL_LOCKED_MATERIALS_SIZE][T1_MATERIAL_NAME_CAP];
    u32 size;
} LockedMaterialCollection;

extern LockedMaterialCollection * all_mesh_materials;

void T1_material_init(
    void * (* arg_malloc_function)(u64 size));

void T1_material_construct(
    T1GPUConstMatf32 * to_construct_f32,
    T1GPUConstMats32 * to_construct_s32);

u32 T1_material_preappend_locked_material_i(
    const char * obj_resource_name,
    const char * material_name);

void T1_material_fetch_ptrs(
    T1GPUConstMatf32 ** recip_f32,
    T1GPUConstMats32 ** recip_s32,
    const u32 material_i);

#endif // T1_MATERIAL_H
