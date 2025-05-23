#ifndef ZPOLYGON_H
#define ZPOLYGON_H

#include <math.h>
#include <inttypes.h>

#include "simd.h"
#include "logger.h"
#include "common.h"
#include "collision.h"
#include "platform_layer.h"
#include "cpu_gpu_shared_types.h"
#include "lightsource.h"
#include "window_size.h"
#include "texture_array.h"
#include "objmodel.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct VertexMaterial {
    float color[4];
    int32_t texturearray_i; /*
                            the index in the global var
                            'texture_arrays' of the texturearray
                             -1 for "untextured, use color
                            instead"
                            */
    int32_t texture_i;     // index in texturearray
} VertexMaterial;

typedef struct CPUzSprite {
    int32_t mesh_id; // data in all_mesh_summaries[mesh_id]
    
    int32_t  sprite_id;
    bool32_t alpha_blending_enabled;
    bool32_t committed;
    bool32_t deleted;
    bool32_t visible;
} CPUzSprite;

typedef struct zPolygonCollection {
    GPUzSprite gpu_data[MAX_POLYGONS_PER_BUFFER];
    GPUzSpriteMaterial gpu_materials[
        MAX_POLYGONS_PER_BUFFER * MAX_MATERIALS_PER_POLYGON];
    CPUzSprite cpu_data[MAX_POLYGONS_PER_BUFFER];
    uint32_t size;
} zSpriteCollection;

extern zSpriteCollection * zsprites_to_render;

typedef struct PolygonRequest {
    GPUzSprite * gpu_data;
    GPUzSpriteMaterial * gpu_materials;
    CPUzSprite * cpu_data;
    uint32_t gpu_data_size;
    uint32_t materials_size;
} PolygonRequest;

void construct_zpolygon(PolygonRequest * to_construct);
// Allocate a PolygonRequest on the stack, then call this
void request_next_zpolygon(PolygonRequest * stack_recipient);
void commit_zpolygon_to_render(PolygonRequest * to_commit);


/*
Make a PolygonRequest (on the stack or whatever) and call this with an
object_id.

If a zPolygon exists with that object_id, the pointers in your LineRequest
will be set so you can edit its properties. (So you can move your objects etc.)

returns false if no such object_id, else true
*/
bool32_t fetch_zpolygon_by_object_id(
    PolygonRequest * stack_recipient,
    const int32_t object_id);

void delete_zpolygon_object(const int32_t with_object_id);

float dot_of_vertices_f3(
    const float a[3],
    const float b[3]);

float get_y_multiplier_for_height(
    CPUzSprite * for_poly,
    const float for_height);
float get_x_multiplier_for_width(
    CPUzSprite * for_poly,
    const float for_width);

void scale_zpolygon_multipliers_to_width(
    CPUzSprite * cpu_data,
    GPUzSprite * gpu_data,
    const float new_width);
void scale_zpolygon_multipliers_to_height(
    CPUzSprite * cpu_data,
    GPUzSprite * gpu_data,
    const float new_height);

float get_distance_f3(
    const float p1[3],
    const float p2[3]);

void construct_quad_around(
    const float mid_x,
    const float mid_y,
    const float z,
    const float width,
    const float height,
    PolygonRequest * stack_recipient);

void construct_quad(
    const float left_x,
    const float bottom_y,
    const float z,
    const float width,
    const float height,
    PolygonRequest * stack_recipient);

void construct_cube_around(
    const float mid_x,
    const float mid_y,
    const float z,
    const float width,
    const float height,
    const float depth,
    PolygonRequest * stack_recipient);

#ifdef __cplusplus
}
#endif

#endif // ZPOLYGON_H
