#ifndef T1_TEXQUAD_H
#define T1_TEXQUAD_H

#include "T1.h"
#include "T1_std.h"
#include "T1_types_public.h"

void T1_texquad_construct(
    T1GPUTexQuadf32 * f32,
    T1GPUTexQuads32 * s32);

void T1_texquad_init(void);

void T1_texquad_defragment(void);

void T1_texquad_delete(const s32 T1_id);

void T1_texquad_get_avg_xyz(
    f32 * recip_xyz,
    const s32 T1_id,
    b8 * found);

void T1_texquad_delete_all(void);

void T1_texquad_fetch_next(
    T1TexQuadRequest * request);

void T1_texquad_commit(
    T1TexQuadRequest * request);

void T1_texquad_apply_endpoint_anim(
    s32 T1_id, s32 touch_id,
    f32 t_applied, f32 t_now,
    const f32 * goal_gpu_f32s,
    const s32 * goal_gpu_s32s);

void T1_texquad_apply_anim_effects_at_t(
    f32 t_applied, f32 t_now,
    const f32 * anim_gpu_f32s,
    const s32 * anim_gpu_s32s,
    const f32 * anim_cpu_f32s,
    T1GPUTexQuadf32 * recip_f32s,
    T1GPUTexQuads32 * recip_s32s);

void T1_texquad_apply_anim_effects_to_id(
    const s32 T1_id,
    const s32 touch_id,
    const f32 t_applied,
    const f32 t_now,
    const f32 * anim_gpu_vals_f32,
    const s32 * anim_gpu_vals_s32);

void T1_texquad_draw_test(
    const f32 width,
    const f32 height);

void T1_texquad_copy_to_frame_data(
    T1GPUTexQuad * recip_frame_data,
    u32 * recip_frame_data_size);

#endif // T1_TEXQUAD_H
