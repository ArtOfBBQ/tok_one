#ifndef T1_TEXQUAD_H
#define T1_TEXQUAD_H

#if T1_LOGGER_ASSERTS_ACTIVE == T1_ACTIVE
#include <math.h> // isnan check
#elif T1_LOGGER_ASSERTS_ACTIVE == T1_INACTIVE
#else
#error
#endif

#include "T1_std.h"
#include "T1_mem.h"
#include "T1_cpu_gpu_shared_types.h"
#include "stdlib.h" // TODO: stop using qsort

#define T1_TEXQUADANIM_NO_EFFECT 0xFFFF
#define T1_TEXQUAD_ID_HIT_EVERYTHING INT32_MAX

typedef struct {
    float offset_xyz[3];
    int32_t zsprite_id;
    bool8_t one_frame_only;
    bool8_t committed;
    bool8_t visible;
    bool8_t deleted;
} T1CPUTexQuad;

typedef struct {
    T1CPUTexQuad * cpu;
    T1GPUTexQuad * gpu;
} T1FlatTexQuadRequest;

void T1_texquad_construct(
    T1GPUTexQuadf32 * f32,
    T1GPUTexQuadi32 * i32);

void T1_texquad_init(void);

void T1_texquad_delete(const int32_t zsprite_id);

void T1_texquad_get_avg_xyz(
    float * recip_xyz,
    const int32_t zsprite_id,
    bool8_t * found);

void T1_texquad_delete_all(void);

void T1_texquad_fetch_next(
    T1FlatTexQuadRequest * request);

void T1_texquad_commit(
    T1FlatTexQuadRequest * request);

void T1_texquad_apply_endpoint_anim(
    const int32_t zsprite_id,
    const int32_t touch_id,
    const float t_applied,
    const float t_now,
    const float * goal_gpu_vals_f32,
    const int32_t * goal_gpu_vals_i32);

void T1_texquad_anim_apply_effects_at_t(
    const float t_applied,
    const float t_now,
    const float * anim_gpu_vals,
    const int32_t * anim_gpu_i32s,
    const float * anim_cpu_vals,
    T1GPUTexQuad * recip_gpu,
    T1CPUTexQuad * recip_cpu);

void T1_texquad_apply_anim_effects_to_id(
    const int32_t zsprite_id,
    const int32_t touch_id,
    const float t_applied,
    const float t_now,
    const float * anim_gpu_vals_f32,
    const int32_t * anim_gpu_vals_i32);

void T1_texquad_draw_test(
    const float width,
    const float height);

void T1_texquad_copy_to_frame_data(
    T1GPUTexQuad * recip_frame_data,
    uint32_t * recip_frame_data_size);

#endif // T1_TEXQUAD_H
