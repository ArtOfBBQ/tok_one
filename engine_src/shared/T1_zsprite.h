#ifndef ZSPRITE_H
#define ZSPRITE_H

#include <math.h>
#include <inttypes.h>

#include "T1_simd.h"
#include "T1_logger.h"
#include "T1_std.h"
#include "T1_collision.h"
#include "T1_platform_layer.h"
#include "T1_cpu_gpu_shared_types.h"
#include "T1_lightsource.h"
#include "T1_engine_globals.h"
#include "T1_texture_array.h"
#include "T1_material.h"
#include "T1_objmodel.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct CPUzSprite {
    uint64_t next_occlusion_in_us;
    int32_t mesh_id; // data in all_mesh_summaries[mesh_id]
    
    int32_t zsprite_id;
    bool8_t alpha_blending_enabled;
    bool8_t committed;
    bool8_t deleted;
    bool8_t visible;
} CPUzSprite;

typedef struct zSpriteCollection {
    GPUzSprite gpu_data[MAX_ZSPRITES_PER_BUFFER];
    GPUConstMat gpu_mats[ALL_LOCKED_MATERIALS_SIZE];
    CPUzSprite cpu_data[MAX_ZSPRITES_PER_BUFFER];
    uint32_t size;
} zSpriteCollection;

extern zSpriteCollection * zsprites_to_render;

typedef struct zSpriteRequest {
    GPUzSprite * gpu_data;
    CPUzSprite * cpu_data;
    uint32_t gpu_data_size;
} zSpriteRequest;

void zsprite_construct_with_mesh_id(
    zSpriteRequest * to_construct,
    const int32_t mesh_id);
void zsprite_construct(zSpriteRequest * to_construct);
// Allocate a PolygonRequest on the stack, then call this
void zsprite_request_next(
    zSpriteRequest * stack_recipient);
void zsprite_commit(zSpriteRequest * to_commit);


/*
Make a PolygonRequest (on the stack or whatever) and call this with an
object_id.

If a zPolygon exists with that object_id, the pointers in your LineRequest
will be set so you can edit its properties. (So you can move your objects etc.)

returns false if no such object_id, else true
*/
bool32_t zsprite_fetch_by_zsprite_id(
    zSpriteRequest * stack_recipient,
    const int32_t zsprite_id);

void zsprite_delete(const int32_t with_zsprite_id);

float zsprite_dot_of_vertices_f3(
    const float a[3],
    const float b[3]);

float zsprite_get_z_multiplier_for_depth(
    CPUzSprite * for_poly,
    const float for_depth);
float zsprite_get_y_multiplier_for_height(
    CPUzSprite * for_poly,
    const float for_height);
float zsprite_get_x_multiplier_for_width(
    CPUzSprite * for_poly,
    const float for_width);

void zsprite_scale_multipliers_to_width(
    CPUzSprite * cpu_data,
    GPUzSprite * gpu_data,
    const float new_width);
void zsprite_scale_multipliers_to_height(
    CPUzSprite * cpu_data,
    GPUzSprite * gpu_data,
    const float new_height);

float zsprite_get_distance_f3(
    const float p1[3],
    const float p2[3]);

void zsprite_construct_quad_around(
    const float mid_x,
    const float mid_y,
    const float z,
    const float width,
    const float height,
    zSpriteRequest * stack_recipient);

void zsprite_construct_quad(
    const float left_x,
    const float bottom_y,
    const float z,
    const float width,
    const float height,
    zSpriteRequest * stack_recipient);

void zsprite_construct_cube_around(
    const float mid_x,
    const float mid_y,
    const float z,
    const float width,
    const float height,
    const float depth,
    zSpriteRequest * stack_recipient);

#ifdef __cplusplus
}
#endif

#endif // ZSPRITE_H
