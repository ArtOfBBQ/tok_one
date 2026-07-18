#ifndef T1_FRAMEANIM_H
#define T1_FRAMEANIM_H

#if 0
#include <stddef.h>

#include "T1_types_cpu_to_gpu.h"

#if T1_FRAME_ANIM_ACTIVE == T1_ACTIVE

// Client use
#define T1_frame_anim_gpu_mod_to_touch_id(touch_id, gpu_prop_name, val_f32) T1_frame_anim_gpu_mod_to_touch_id_by_offset(touch_id, offsetof(T1GPUzSprite, gpu_prop_name), val_f32)
void T1_frame_anim_gpu_mod_to_touch_id_by_offset(
    const s32 touch_id,
    const u32 gpu_prop_offset,
    const f32 val_f32);

#define T1_frame_anim_gpu_mod_to_T1_id(T1_id, gpu_prop_name, val_f32) T1_frame_anim_gpu_mod_to_T1_id_by_offset(T1_id, offsetof(T1GPUzSprite, gpu_prop_name), val_f32)
void T1_frame_anim_gpu_mod_to_T1_id_by_offset(
    const s32 T1_id,
    const u32 gpu_prop_offset,
    const f32 val_f32);

// Internal use (call from renderer)
void T1_frame_anim_init(void);

void T1_frame_anim_new_frame_starts(void);

void T1_frame_anim_apply_all(T1GPUFrame * frame_data);

#elif T1_FRAME_ANIM_ACTIVE == T1_INACTIVE
#else
#error
#endif

#endif // T1_FRAMEANIM_H

#endif
