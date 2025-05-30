#ifndef T1_MATERIAL_H
#define T1_MATERIAL_H

#include <stdio.h>

typedef struct LockedMaterialCollection {
    GPULockedMaterial gpu_data[ALL_LOCKED_VERTICES_SIZE];
    uint32_t size;
} LockedMaterialCollection;

extern LockedMaterialCollection * all_mesh_materials;

void printstuff(void);

#endif // T1_MATERIAL_H
