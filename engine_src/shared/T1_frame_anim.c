#include "T1_frame_anim.h"

#if T1_FRAME_ANIM_ACTIVE == T1_ACTIVE

typedef enum : uint8_t {
    T1FRAMEANIMFILTER_ZSPRITE_ID,
    T1FRAMEANIMFILTER_TOUCH_ID,
} T1FrameAnimFilter;

typedef struct {
    int32_t zsprite_id;
    int32_t touch_id;
    T1GPUzSprite gpu_stats;
    T1FrameAnimFilter filter;
} T1FrameAnim;

#define T1_FRAME_ANIMS_CAP 100
static T1FrameAnim * T1_frame_anims = NULL;
static uint32_t T1_frame_anims_size = 0;

void T1_frame_anim_init(void) {
    T1_frame_anims =
        T1_mem_malloc_from_unmanaged(
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
        uint32_t mod_i = 0;
        mod_i < T1_frame_anims_size;
        mod_i++)
    {
        float * mod_ptr =
            (float *)(
                &T1_frame_anims[mod_i].
                    gpu_stats);
        
        for (
            int32_t i = 0;
            i < (int32_t)frame_data->
                zsprite_list->size;
            i++)
        {
            uint32_t hit = 0;
            
            switch (
                T1_frame_anims[mod_i].filter)
            {
                case T1FRAMEANIMFILTER_TOUCH_ID:
                {
                    if (
                        frame_data->id_pairs[i].
                            zsprite_id ==
                        T1_frame_anims[mod_i].
                            zsprite_id)
                    {
                        hit = 1;
                    }
                }
                break;
                case T1FRAMEANIMFILTER_ZSPRITE_ID:
                {
                    if (
                        frame_data->id_pairs[i].
                            zsprite_id ==
                        T1_frame_anims[mod_i].
                            zsprite_id)
                    {
                        hit = 1;
                    }
                }
                break;
                default:
                    log_assert(0);
            }
            
            if (hit) {
                T1_zsprite_anim_apply_effects_at_t(
                    /* const float t_applied: */
                        0.0f,
                    /* const float t_now: */
                        1.0f,
                    /* const float * anim_gpu_vals: */
                        mod_ptr,
                    /* const float * anim_cpu_vals: */
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
    const int32_t touch_id,
    const uint32_t gpu_prop_offset,
    const float val_to_memcpy_32bit)
{
    log_assert(touch_id >= 0);
    
    int32_t exist_i = -1;
    for (
        int32_t mod_i = 0;
        mod_i < (int32_t)T1_frame_anims_size;
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
        exist_i = (int32_t)T1_frame_anims_size;
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

void T1_frame_anim_gpu_mod_to_zsprite_id_by_offset(
    const int32_t zsprite_id,
    const uint32_t gpu_prop_offset,
    const float val_to_memcpy_32bit)
{
    log_assert(zsprite_id >= 0);
    
    int32_t exist_i = -1;
    for (
        int32_t mod_i = 0;
        mod_i < (int32_t)T1_frame_anims_size;
        mod_i++)
    {
        if (
            T1_frame_anims[mod_i].filter ==
                T1FRAMEANIMFILTER_ZSPRITE_ID &&
            T1_frame_anims[mod_i].zsprite_id ==
                zsprite_id)
        {
            exist_i = mod_i;
        }
    }
    
    if (exist_i < 0) {
        exist_i = (int32_t)T1_frame_anims_size;
        T1_frame_anims_size += 1;
    }
    
    T1_frame_anims[exist_i].zsprite_id =
        zsprite_id;
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

