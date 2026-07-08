#ifndef ZSPRITE_H
#define ZSPRITE_H

#define T1_ZSPRITE_ID_HIT_EVERYTHING INT32_MAX

#include <math.h>
#include <inttypes.h>

#include "T1_cpu_to_gpu.h"
#include "T1_public_types.h"

#define T1_ZSPRITEANIM_NO_EFFECT 0xFFFF

typedef struct  {
    f32 xyz[3];
    f32 offset_xyz[3];
    f32 mul_xyz[3];
    f32 angle_xyz[3];
    f32 bloom_on;
    f32 alpha_on;
    f32 padding[2];
} T1CPUzSpriteSimdStats;

typedef struct {
    T1CPUzSpriteSimdStats simd;
    
    u64 next_occlusion_in_us;
    s32 mesh_id; // data in all_mesh_summaries[mesh_id]
    s32 T1_id;
    
    b8 committed;
    b8 deleted;
    b8 visible;
} T1CPUzSprite;

typedef struct {
    T1GPUzSprite * gpu_data;
    T1CPUzSprite * cpu_data;
    u32       gpu_data_size;
} T1zSpriteRequest;

void
T1_zsprite_init(void);

void
T1_zsprite_defragment(void);

void
T1_zsprite_construct_with_mesh_id(
    T1zSpriteRequest * to_construct,
    const s32 mesh_id);

void
T1_zsprite_construct(T1zSpriteRequest * to_construct);

// Allocate a PolygonRequest on the stack, then call this

void
T1_zsprite_fetch_next_noconstruct(
    T1zSpriteRequest * stack_recipient);

void
T1_zsprite_commit(
    T1zSpriteRequest * to_commit);

void
T1_zsprite_get_pos_xyz(
    const s32 T1_id,
    f32 * recip_x,
    f32 * recip_y,
    f32 * recip_z);

void
T1_zsprite_delete(
    const s32 with_T1_id);

void
T1_zsprite_delete_all(void);

void
T1_zsprite_scale_multipliers_to_width(
    T1CPUzSprite * cpu_data,
    T1GPUzSprite * gpu_data,
    const f32 new_width);

void
T1_zsprite_scale_multipliers_to_height(
    T1CPUzSprite * cpu_data,
    T1GPUzSprite * gpu_data,
    const f32 new_height);

void
T1_zsprite_construct_quad_around(
    const f32 mid_x,
    const f32 mid_y,
    const f32 z,
    const f32 width,
    const f32 height,
    T1zSpriteRequest * stack_recipient);

void
T1_zsprite_construct_quad(
    const f32 left_x,
    const f32 bottom_y,
    const f32 z,
    const f32 width,
    const f32 height,
    T1zSpriteRequest * stack_recipient);

void
T1_zsprite_construct_cube_around(
    const f32 mid_x,
    const f32 mid_y,
    const f32 z,
    const f32 width,
    const f32 height,
    const f32 depth,
    T1zSpriteRequest * stack_recipient);

void
T1_zsprite_apply_endpoint_anim(
    const s32 T1_id,
    const s32 touch_id,
    const f32 t_applied,
    const f32 t_now,
    const f32 * goal_gpu_vals_f32,
    const s32 * goal_gpu_vals_s32,
    const f32 * goal_cpu_vals);

void
T1_zsprite_anim_apply_effects_at_t(
    const f32 t_applied,
    const f32 t_now,
    const f32 * anim_gpu_vals,
    const s32 * anim_gpu_s32s,
    const f32 * anim_cpu_vals,
    T1GPUzSprite * recip_gpu,
    T1CPUzSpriteSimdStats * recip_cpu);

void
T1_zsprite_apply_anim_effects_to_id(
    const s32 T1_id,
    const s32 touch_id,
    const f32 t_applied,
    const f32 t_now,
    const f32 * anim_gpu_vals_f32,
    const s32 * anim_gpu_vals_s32,
    const f32 * anim_cpu_vals);

#if T1_OCCLUSION_ACTIVE == T1_ACTIVE
void
T1_zsprite_set_occlusion(
    const s32 T1_id,
    const s32 new_visible_stat,
    const u64 wait_before_invis_us);
#elif T1_OCCLUSION_ACTIVE == T1_INACTIVE
#define T1_zsprite_set_occlusion(a, b, c)
#else
#error
#endif

void
T1_zsprite_handle_timed_occlusion(void);

// TODO: encapsulate collection instead of externing
typedef struct {
    T1GPUzSprite gpu[T1_ZSPRITES_CAP];
    T1GPUConstMatf32 gpu_mats_f32[T1_ALL_LOCKED_MATERIALS_SIZE];
    T1GPUConstMats32 gpu_mats_s32[T1_ALL_LOCKED_MATERIALS_SIZE];
    T1CPUzSprite cpu[T1_ZSPRITES_CAP];
    u32 size;
} T1zSpriteCollection;

void
T1_zsprite_anim_set_ignore_camera_but_retain_screenspace_pos(
    const s32 T1_id,
    const f32 new_ignore_camera);

void
T1_zsprite_copy_to_frame_data(
    T1GPUzSprite * recip,
    IdPair * recip_ids,
    u32 * recip_size);

void
T1_zsprite_add_alphablending_zpolygons_to_workload(
    T1GPUFrame * frame_data);

void
T1_zsprite_add_bloom_zpolygons_to_workload(
    T1GPUFrame * frame_data);

void
T1_zsprite_add_opaque_zpolygons_to_workload(
    T1GPUFrame * frame_data);

void
T1_zsprite_construct_model_and_normal_matrices(
    T1GPUFrame * frame_data);
    
void
T1_zsprite_copy_data_for_shatter_effect(
    const s32 T1_id,
    T1GPUzSprite * gpu_recip,
    T1CPUzSprite * cpu_recip);

#endif // ZSPRITE_H
