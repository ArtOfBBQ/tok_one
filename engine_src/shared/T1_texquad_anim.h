#ifndef T1_TEXQUAD_ANIM_H
#define T1_TEXQUAD_ANIM_H

#if T1_TEXQUAD_ANIM_ACTIVE == T1_ACTIVE

#include "T1_std.h"
#include "T1_mem.h"
#include "T1_cpu_to_gpu_types.h"
#include "T1_global.h"
#include "T1_easing.h"
#include "T1_texquad.h"

void T1_texquad_anim_init(
    uint32_t (* funcptr_init_mutex_and_return_id)(void),
    void (* funcptr_mutex_lock)(const uint32_t),
    void (* funcptr_mutex_unlock)(const uint32_t));

void T1_texquad_anim_resolve(void);

typedef struct {
    T1GPUTexQuad gpu_vals;
    T1CPUTexQuad cpu_vals;
    
    uint64_t duration_us;
    uint64_t pause_us;
    
    uint32_t runs;
    int32_t affect_zsprite_id;
    int32_t affect_touch_id;
    
    T1EasingType easing_type; // u8
    bool8_t gpu_f32_active;
    bool8_t gpu_i32_active;
    bool8_t cpu_active;
    bool8_t del_obj_on_finish;
    bool8_t del_conflict_anims;
} T1TexQuadAnim;

T1TexQuadAnim * T1_texquad_anim_request_next(
    bool8_t endpoints_not_deltas);

void T1_texquad_anim_commit(
    T1TexQuadAnim * to_commit);

void T1_texquad_anim_fade_and_destroy(
    const int32_t  object_id,
    const uint64_t duration_us);

#elif T1_TEXQUAD_ANIM_ACTIVE == T1_INACTIVE
#else
#error
#endif

#endif // T1_TEXQUAD_ANIM_H
