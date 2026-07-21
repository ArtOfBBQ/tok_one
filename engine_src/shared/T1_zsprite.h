#ifndef ZSPRITE_H
#define ZSPRITE_H

#include <math.h>
#include <inttypes.h>

#include "T1_types_cpu_to_gpu.h"
#include "T1_types_public.h"

typedef struct {
    T1CPUzSpritef32 zs_cpu_f32s;
    
    u64 next_occlusion_in_us;
    s32 mesh_id; // data in all_mesh_summaries[mesh_id]
    u32 T1_id;
    
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

void T1_zsprite_construct_with_mesh_id(
    T1zSpriteRequest * to_construct,
    s32 mesh_id);

void T1_zsprite_construct(T1zSpriteRequest * to_construct);

// Allocate a PolygonRequest on the stack, then call this

void T1_zsprite_fetch_next_noconstruct(
    T1zSpriteRequest * stack_recipient);

void T1_zsprite_commit(T1zSpriteRequest * to_commit);

void
T1_zsprite_get_pos_xyz(
    u32 T1_id,
    f32 * recip_x,
    f32 * recip_y,
    f32 * recip_z);

void T1_zsprite_delete(u32 with_T1_id);

void T1_zsprite_delete_all(void);

void
T1_zsprite_scale_multipliers_to_width(
    T1CPUzSprite * cpu_data,
    T1GPUzSprite * gpu_data,
    f32 new_width);

void
T1_zsprite_scale_multipliers_to_height(
    T1CPUzSprite * cpu_data,
    T1GPUzSprite * gpu_data,
    const f32 new_height);

void
T1_zsprite_construct_quad_around(
    f32 mid_x, f32 mid_y, f32 z,
    f32 width, f32 height,
    T1zSpriteRequest * stack_recipient);

void T1_zsprite_construct_quad(
    f32 left_x,
    f32 bottom_y,
    f32 z,
    f32 width,
    f32 height,
    T1zSpriteRequest * stack_recipient);

void
T1_zsprite_construct_cube_around(
    f32 mid_x, f32 mid_y, f32 z,
    f32 width, f32 height, f32 depth,
    T1zSpriteRequest * stack_recipient);

void T1_zsprite_apply_endpoint_anim(
    u32 T1_id, u32 touch_id, f32 t_applied, f32 t_now,
    const f32 * goal_gpu_vals_f32,
    const u32 * goal_gpu_vals_u32,
    const f32 * goal_cpu_vals);

void T1_zsprite_anim_apply_effects_at_t(
    f32 t_applied, f32 t_now,
    const f32 * anim_gpu_vals,
    const u32 * anim_gpu_u32s,
    const f32 * anim_cpu_f32s,
    T1GPUzSpritef32 * recip_gpu_f32s,
    T1GPUzSpriteu32 * recip_gpu_u32s,
    T1CPUzSpritef32 * recip_cpu_f32s);

void T1_zsprite_apply_anim_effects_to_id(
    u32 T1_id, u32 touch_id,
    f32 t_applied, f32 t_now,
    const f32 * anim_gpu_f32s,
    const u32 * anim_gpu_u32s,
    const f32 * anim_cpu_f32s);

#if T1_OCCLUSION_ACTIVE == T1_ACTIVE
void T1_zsprite_set_occlusion(
    s32 T1_id,
    s32 new_visible_stat,
    u64 wait_before_invis_us);
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
    T1GPUMatf32 gpu_mats_f32[T1_ALL_LOCKED_MATERIALS_SIZE];
    T1GPUMatu32 gpu_mats_s32[T1_ALL_LOCKED_MATERIALS_SIZE];
    T1CPUzSprite cpu[T1_ZSPRITES_CAP];
    u32 size;
} T1zSpriteCollection;

void
T1_anim_set_ignore_camera_but_retain_screenspace_pos(
    u32 T1_id, f32 new_ignore_camera);

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
    u32 T1_id,
    T1GPUzSprite * gpu_recip,
    T1CPUzSprite * cpu_recip);

#endif // ZSPRITE_H
