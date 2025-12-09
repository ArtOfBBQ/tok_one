#include "T1_frame_anims.h"

typedef enum : uint8_t {
    T1FRAMEANIMFILTER_ZSPRITE_ID,
    T1FRAMEANIMFILTER_TOUCH_ID,
} T1FrameAnimFilter;

typedef struct {
    int32_t zsprite_id;
    int32_t touch_id;
    T1CPUzSpriteSimdStats cpu_stats;
    T1GPUzSprite gpu_stats;
    T1FrameAnimFilter filter;
} T1FrameAnim;

#define T1_FRAME_ANIMS_CAP 100
static T1FrameAnim * T1_frame_anims = NULL;
static uint32_t T1_frame_anims_size = 0;

void T1_frame_anims_init(void) {
    T1_frame_anims =
        T1_mem_malloc_from_unmanaged(
            sizeof(T1FrameAnim) *
                T1_FRAME_ANIMS_CAP);
}

void T1_frame_anims_new_frame_starts(void) {
    T1_frame_anims_size = 0;
    
    T1_std_memset(
        T1_frame_anims,
        0,
        sizeof(T1FrameAnim) *
            T1_FRAME_ANIMS_CAP);
}

void T1_frame_anims_apply_all(
    T1GPUFrame * frame_data)
{
    for (
        uint32_t mod_i = 0;
        mod_i < T1_frame_anims_size;
        mod_i++)
    {
        for (
            uint32_t zp_i = 0;
            zp_i < frame_data->zsprite_list->size;
            zp_i++)
        {
            uint32_t hit = false;
            switch (T1_frame_anims[mod_i].filter) {
                case T1FRAMEANIMFILTER_TOUCH_ID:
                    hit = frame_data->zsprite_list->
                        polygons[zp_i].touch_id ==
                            T1_frame_anims[mod_i].touch_id;
                break;
                case T1FRAMEANIMFILTER_ZSPRITE_ID:
                    hit = T1_zsprites_to_render->cpu_data[zp_i].zsprite_id ==
                            T1_frame_anims[mod_i].zsprite_id;
                break;
                default:
                    log_assert(0);
            }
            
            if (hit) {
                // TODO: add every property, not 1 test
                frame_data->zsprite_list->
                    polygons[zp_i].outline_alpha +=
                            2.5f;
            }
        }
    }
}

void T1_frame_anims_gpu_mod_to_touch_id_by_offset(
    const int32_t touch_id,
    const uint32_t gpu_prop_offset,
    const float val_to_memcpy)
{
    log_assert(touch_id >= 0);
    
    T1_frame_anims[T1_frame_anims_size].touch_id =
        touch_id;
    T1_frame_anims[T1_frame_anims_size].filter =
        T1FRAMEANIMFILTER_TOUCH_ID;
    T1_std_memcpy(
        (char *)
            &T1_frame_anims[T1_frame_anims_size].
                gpu_stats + gpu_prop_offset,
        &val_to_memcpy,
        4);
    
    T1_frame_anims_size += 1;
}
