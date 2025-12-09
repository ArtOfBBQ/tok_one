#ifndef T1_FRAMEANIMS_H
#define T1_FRAMEANIMS_H

#include "T1_std.h"
#include "T1_mem.h"
#include "T1_zsprite.h"

// Client use
#define T1_frame_anims_gpu_mod_to_touch_id(zsprite_id, gpu_prop_name, val_to_memcpy_32bit) T1_frame_anims_gpu_mod_to_touch_id_by_offset(zsprite_id, offsetof(T1GPUzSprite, gpu_prop_name), val_to_memcpy_32bit)
void T1_frame_anims_gpu_mod_to_touch_id_by_offset(
    const int32_t zsprite_id,
    const uint32_t gpu_prop_offset,
    const float val_to_memcpy_32bit);

// Internal use (call from renderer)
void T1_frame_anims_init(void);
void T1_frame_anims_new_frame_starts(void);
void T1_frame_anims_apply_all(
    T1GPUFrame * frame_data);

#endif // T1_FRAMEANIMS_H

