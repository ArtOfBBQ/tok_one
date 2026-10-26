#ifndef T1_ANIM_H
#define T1_ANIM_H

#if T1_ANIM_ACTIVE == T1_ACTIVE

#include "T1_std.h"
#include "T1_simd.h"
#include "T1_types_public.h"
#include "T1_property.h"


void T1_anim_init(
    u32 (* fptr_init_mutex_and_return_id)(void),
    void (* fptr_mutex_lock)(u32),
    void (* fptr_mutex_unlock)(u32));

void T1_anim_resolve(void);

void T1_anim_delete_all(void);

void T1_anim_delete_all_anims_targeting(u32 T1_id);

/*
Animations by property name, e.g.:
u32 anim = T1_anim_new(my_T1_id, T1_ANIM_MODE_TO);
T1_anim_set_f32(anim, "xyz[0]", 0.5f);
T1_anim_set_duration_us(anim, 250000);
T1_anim_commit(anim);

The returned handle is only valid until it's committed. f32 properties
interpolate, u32 properties jump to their value at the end (and only work
with T1_ANIM_MODE_TO).
*/
u32 T1_anim_new(u32 T1_id, u8 mode);
void T1_anim_set_f32(u32 anim, const c8 * property, f32 value);
void T1_anim_set_u32(u32 anim, const c8 * property, u32 value);
void T1_anim_set_duration_us(u32 anim, u64 duration_us);
void T1_anim_set_pause_us(u32 anim, u64 pause_us);
// easing maps the anim's t (0 to 1) to how far along its values are, NULL
// (the default) is linear
void T1_anim_set_easing(u32 anim, f32 (* easing)(f32 t));
// 0 = repeat forever
void T1_anim_set_runs(u32 anim, u32 runs);
void T1_anim_set_delete_target_on_finish(u32 anim, u8 delete_target);
// On commit, deletes the TO anims on the same T1_id that animate any of
// this anim's properties
void T1_anim_set_cancel_conflicting(u32 anim, u8 cancel_conflicting);
// A duration of 1 microsecond and no pause applies the anim immediately
void T1_anim_commit(u32 anim);

void T1_anim_rename_target_T1_id(u32 from_T1_id, u32 to_T1_id);

void T1_anim_set_ignore_camera_but_retain_screenspace_pos(
    u32 T1_id,
    f32 new_ignore_camera,
    u32 cam_T1_id);
#elif T1_ANIM_ACTIVE == T1_INACTIVE
#else
#error
#endif

#endif // T1_ANIM_H
