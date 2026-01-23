#ifndef ZSPRITE_H
#define ZSPRITE_H

#include <math.h>
#include <inttypes.h>

#include "T1_cpu_gpu_shared_types.h"
#include "T1_simd.h"
#include "T1_std.h"
#include "T1_logger.h"
#include "T1_global.h"
#include "T1_material.h"
#include "T1_mesh.h"
#include "T1_zlight.h"

#define T1_ZSPRITEANIM_NO_EFFECT 0xFFFF

typedef struct  {
    float xyz[3];
    float offset_xyz[3];
    float mul_xyz[3];
    float angle_xyz[3];
    float scale_factor;
    float padding[3];
} T1CPUzSpriteSimdStats;

typedef struct {
    T1CPUzSpriteSimdStats simd_stats;
    
    uint64_t next_occlusion_in_us;
    int32_t mesh_id; // data in all_mesh_summaries[mesh_id]
    int32_t zsprite_id;
    
    bool8_t bloom_on;
    bool8_t alpha_blending_on;
    bool8_t committed;
    bool8_t deleted;
    bool8_t visible;
} T1CPUzSprite;

typedef struct {
    T1GPUzSprite * gpu_data;
    T1CPUzSprite * cpu_data;
    uint32_t gpu_data_size;
} T1zSpriteRequest;

void T1_zsprite_init(void);

void T1_zsprite_construct_with_mesh_id(
    T1zSpriteRequest * to_construct,
    const int32_t mesh_id);
void T1_zsprite_construct(T1zSpriteRequest * to_construct);

// Allocate a PolygonRequest on the stack, then call this
void T1_zsprite_fetch_next(
    T1zSpriteRequest * stack_recipient);
void T1_zsprite_commit(
    T1zSpriteRequest * to_commit);

void T1_zsprite_delete(
    const int32_t with_zsprite_id);
void T1_zsprite_delete_all(void);

float T1_zsprite_get_z_multiplier_for_depth(
    T1CPUzSprite * for_poly,
    const float for_depth);
float T1_zsprite_get_y_multiplier_for_height(
    T1CPUzSprite * for_poly,
    const float for_height);
float T1_zsprite_get_x_multiplier_for_width(
    T1CPUzSprite * for_poly,
    const float for_width);

void T1_zsprite_scale_multipliers_to_width(
    T1CPUzSprite * cpu_data,
    T1GPUzSprite * gpu_data,
    const float new_width);
void T1_zsprite_scale_multipliers_to_height(
    T1CPUzSprite * cpu_data,
    T1GPUzSprite * gpu_data,
    const float new_height);

void T1_zsprite_construct_quad_around(
    const float mid_x,
    const float mid_y,
    const float z,
    const float width,
    const float height,
    T1zSpriteRequest * stack_recipient);

void zsprite_construct_quad(
    const float left_x,
    const float bottom_y,
    const float z,
    const float width,
    const float height,
    T1zSpriteRequest * stack_recipient);

void zsprite_construct_cube_around(
    const float mid_x,
    const float mid_y,
    const float z,
    const float width,
    const float height,
    const float depth,
    T1zSpriteRequest * stack_recipient);

void T1_zsprite_apply_endpoint_anim(
    const int32_t zsprite_id,
    const int32_t touch_id,
    const float t_applied,
    const float t_now,
    const float * goal_gpu_vals,
    const float * goal_cpu_vals);

void T1_zsprite_anim_apply_effects_at_t(
    const float t_applied,
    const float t_now,
    const float * anim_gpu_vals,
    const float * anim_cpu_vals,
    T1GPUzSprite * recip_gpu,
    T1CPUzSpriteSimdStats * recip_cpu);

void T1_zsprite_apply_anim_effects_to_id(
    const int32_t zsprite_id,
    const int32_t touch_id,
    const float t_applied,
    const float t_now,
    const float * anim_gpu_vals,
    const float * anim_cpu_vals);

void T1_zsprite_set_occlusion(
    const int32_t zsprite_id,
    const int32_t new_visible_stat,
    const uint64_t wait_before_invis_us);

void T1_zsprite_handle_timed_occlusion(void);

// TODO: encapsulate collection instead of externing
typedef struct {
    T1GPUzSprite gpu_data[MAX_ZSPRITES_PER_BUFFER];
    T1GPUConstMatf32 gpu_mats_f32[ALL_LOCKED_MATERIALS_SIZE];
    T1GPUConstMati32 gpu_mats_i32[ALL_LOCKED_MATERIALS_SIZE];
    T1CPUzSprite cpu_data[MAX_ZSPRITES_PER_BUFFER];
    uint32_t size;
} T1zSpriteCollection;

void T1_zsprite_anim_set_ignore_camera_but_retain_screenspace_pos(
    const int32_t zsprite_id,
    const float new_ignore_camera);

void T1_zsprite_copy_to_frame_data(
    T1GPUzSprite * recip,
    IdPair * recip_ids,
    uint32_t * recip_size);

void T1_add_alphablending_zpolygons_to_workload(
    T1GPUFrame * frame_data);

void T1_zsprite_add_bloom_zpolygons_to_workload(
    T1GPUFrame * frame_data);

void
T1_add_opaque_zpolygons_to_workload(
    T1GPUFrame * frame_data);

void
T1_zsprite_construct_model_and_normal_matrices(
    T1GPUFrame * frame_data);

extern T1zSpriteCollection * T1_zsprite_list;

#endif // ZSPRITE_H
