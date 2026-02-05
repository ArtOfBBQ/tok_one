#ifndef T1_MATERIAL_H
#define T1_MATERIAL_H

#include <stdint.h>

#include "T1_std.h"
#include "T1_logger.h"
#include "T1_cpu_gpu_shared_types.h"

typedef struct LockedMaterialCollection {
    T1GPUConstMatf32 gpu_f32[T1_ALL_LOCKED_MATERIALS_SIZE];
    T1GPUConstMati32 gpu_i32[T1_ALL_LOCKED_MATERIALS_SIZE];
    char material_names[T1_ALL_LOCKED_MATERIALS_SIZE][T1_MATERIAL_NAME_CAP];
    uint32_t size;
} LockedMaterialCollection;

extern LockedMaterialCollection * all_mesh_materials;

void T1_material_init(
    void * (* arg_malloc_function)(size_t size));

void T1_material_construct(
    T1GPUConstMatf32 * to_construct_f32,
    T1GPUConstMati32 * to_construct_i32);

uint32_t T1_material_preappend_locked_material_i(
    const char * obj_resource_name,
    const char * material_name);

void T1_material_fetch_ptrs(
    T1GPUConstMatf32 ** recip_f32,
    T1GPUConstMati32 ** recip_i32,
    const uint32_t material_i);

#endif // T1_MATERIAL_H
