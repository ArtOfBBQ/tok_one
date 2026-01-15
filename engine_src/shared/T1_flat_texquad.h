#ifndef T1_TEX_QUAD_H
#define T1_TEX_QUAD_H

#include "T1_std.h"
#include "T1_mem.h"
#include "T1_cpu_gpu_shared_types.h"


typedef struct {
    int32_t zsprite_id;
    int32_t touch_id;
    bool8_t committed;
    bool8_t visible;
    bool8_t deleted;
} T1CPUTexQuad;

typedef struct {
    T1CPUTexQuad * cpu;
    T1GPUTexQuad * gpu;
} T1FlatTexQuadRequest;

void T1_flat_texquad_init(void);

void T1_flat_texquad_delete_all(void);

void T1_flat_texquad_fetch_next(
    T1FlatTexQuadRequest * request);

void T1_flat_texquad_commit(
    T1FlatTexQuadRequest * request);

void T1_flat_texquad_copy_to_frame_data(
    T1GPUTexQuad * recip_frame_data,
    uint32_t * recip_frame_data_size);

#endif // T1_TEX_QUAD_H
