#include "T1_frame_anim.h"

#include "T1_log.h"
#include "T1_std.h"
#include "T1_mem.h"
#include "T1_zsprite.h"

#if T1_FRAME_ANIM_ACTIVE == T1_ACTIVE

typedef enum : u8 {
    T1FRAMEANIMFILTER_ZSPRITE_ID,
    T1FRAMEANIMFILTER_TOUCH_ID,
} T1FrameAnimFilter;

typedef struct {
    s32 T1_id;
    s32 touch_id;
    T1GPUzSprite gpu_stats;
    T1FrameAnimFilter filter;
} T1FrameAnim;

#define T1_FRAME_ANIMS_CAP 100
static T1FrameAnim * T1_frame_anims = NULL;
static u32 T1_frame_anims_size = 0;

void T1_frame_anim_init(void) {
    T1_frame_anims =
        T1_mem_malloc_unmanaged(
            sizeof(T1FrameAnim) *
                T1_FRAME_ANIMS_CAP);
}

void T1_frame_anim_new_frame_starts(void) {
    T1_frame_anims_size = 0;
    
    T1_std_memset(
        T1_frame_anims,
        0,
        sizeof(T1FrameAnim) *
            T1_FRAME_ANIMS_CAP);
}

void T1_frame_anim_apply_all(
    T1GPUFrame * frame_data)
{
    for (
        u32 mod_i = 0;
        mod_i < T1_frame_anims_size;
        mod_i++)
    {
        f32 * mod_ptr_f32 =
            (f32 *)(
                &T1_frame_anims[mod_i].
                    gpu_stats.f32s);
        s32 * mod_ptr_s32 =
            (s32 *)(
                &T1_frame_anims[mod_i].
                    gpu_stats.s32);
        
        T1_log_assert(frame_data->zsprite_list->size < T1_ZSPRITES_CAP);
        for (
            s32 i = 0;
            i < (s32)frame_data->
                zsprite_list->size;
            i++)
        {
            T1_log_assert(i < T1_ZSPRITES_CAP);
            
            u32 hit = 0;
            
            switch (
                T1_frame_anims[mod_i].filter)
            {
                case T1FRAMEANIMFILTER_TOUCH_ID:
                {
                    if (
                        frame_data->id_pairs[i].T1_id ==
                        T1_frame_anims[mod_i].T1_id)
                    {
                        hit = 1;
                    }
                }
                break;
                case T1FRAMEANIMFILTER_ZSPRITE_ID:
                {
                    if (
                        frame_data->id_pairs[i].T1_id ==
                        T1_frame_anims[mod_i].T1_id)
                    {
                        hit = 1;
                    }
                }
                break;
                default:
                    T1_log_assert(0);
            }
            
            if (hit) {
                T1_zsprite_anim_apply_effects_at_t(
                    /* const f32 t_applied: */
                        0.0f,
                    /* const f32 t_now: */
                        1.0f,
                    /* const f32 * anim_gpu_vals: */
                        mod_ptr_f32,
                    /* const s32 * anim_gpu_s32s: */
                        mod_ptr_s32,
                    /* const f32 * anim_cpu_vals: */
                        NULL,
                    /* T1GPUzSprite * recip_gpu: */
                        frame_data->zsprite_list->polygons + i,
                    /* T1CPUzSpriteSimdStats * recip_cpu: */
                        NULL);
            }
        }
    }
}

void T1_frame_anim_gpu_mod_to_touch_id_by_offset(
    const s32 touch_id,
    const u32 gpu_prop_offset,
    const f32 val_to_memcpy_32bit)
{
    T1_log_assert(touch_id >= 0);
    
    s32 exist_i = -1;
    for (
        s32 mod_i = 0;
        mod_i < (s32)T1_frame_anims_size;
        mod_i++)
    {
        if (
            T1_frame_anims[mod_i].filter ==
                T1FRAMEANIMFILTER_TOUCH_ID &&
            T1_frame_anims[mod_i].touch_id == touch_id)
        {
            exist_i = mod_i;
        }
    }
    
    if (exist_i < 0) {
        exist_i = (s32)T1_frame_anims_size;
        T1_frame_anims_size += 1;
    }
    
    T1_frame_anims[exist_i].touch_id = touch_id;
    T1_frame_anims[exist_i].filter =
        T1FRAMEANIMFILTER_TOUCH_ID;
    T1_std_memcpy(
        (char *)
            &T1_frame_anims[exist_i].
                gpu_stats + gpu_prop_offset,
        &val_to_memcpy_32bit,
        4);
}

void T1_frame_anim_gpu_mod_to_T1_id_by_offset(
    const s32 T1_id,
    const u32 gpu_prop_offset,
    const f32 val_to_memcpy_32bit)
{
    T1_log_assert(T1_id >= 0);
    
    s32 exist_i = -1;
    for (
        s32 mod_i = 0;
        mod_i < (s32)T1_frame_anims_size;
        mod_i++)
    {
        if (
            T1_frame_anims[mod_i].filter ==
                T1FRAMEANIMFILTER_ZSPRITE_ID &&
            T1_frame_anims[mod_i].T1_id ==
                T1_id)
        {
            exist_i = mod_i;
        }
    }
    
    if (exist_i < 0) {
        exist_i = (s32)T1_frame_anims_size;
        T1_frame_anims_size += 1;
    }
    
    T1_frame_anims[exist_i].T1_id =
        T1_id;
    T1_frame_anims[exist_i].filter =
        T1FRAMEANIMFILTER_ZSPRITE_ID;
    T1_std_memcpy(
        (char *)
            &T1_frame_anims[exist_i].
                gpu_stats + gpu_prop_offset,
        &val_to_memcpy_32bit,
        4);
}

#elif T1_FRAME_ANIM_ACTIVE == T1_INACTIVE
#else
#error
#endif

