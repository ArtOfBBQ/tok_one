#ifndef T1_MATERIAL_H
#define T1_MATERIAL_H

#include <stdint.h>

#include "T1_common.h"
#include "T1_logger.h"
#include "T1_cpu_gpu_shared_types.h"

typedef struct LockedMaterialCollection {
    GPULockedMaterial gpu_data[ALL_LOCKED_MATERIALS_SIZE];
    char material_names[ALL_LOCKED_MATERIALS_SIZE][MATERIAL_NAME_CAP];
    uint32_t size;
} LockedMaterialCollection;

extern LockedMaterialCollection * all_mesh_materials;

void T1_material_init(
    void * (* arg_malloc_function)(size_t size));

void T1_material_construct(
    GPULockedMaterial * to_construct);

uint32_t T1_material_preappend_locked_material_i(
    const char * material_name);

GPULockedMaterial * T1_material_fetch_ptr(
    const uint32_t locked_material_i);

#endif // T1_MATERIAL_H
