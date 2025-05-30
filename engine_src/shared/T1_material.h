#ifndef T1_MATERIAL_H
#define T1_MATERIAL_H

#include <stdint.h>

#include "T1_cpu_gpu_shared_types.h"

typedef struct LockedMaterialCollection {
    GPULockedMaterial gpu_data[ALL_LOCKED_VERTICES_SIZE];
    uint32_t size;
} LockedMaterialCollection;

extern LockedMaterialCollection * all_mesh_materials;

void T1_material_init(
    void * (* arg_malloc_function)(size_t size));

#endif // T1_MATERIAL_H
