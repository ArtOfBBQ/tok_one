#ifndef ZPOLYGON_H
#define ZPOLYGON_H

#include <math.h>
#include <inttypes.h>

#include "clientlogic_macro_settings.h"

#include "simd.h"
#include "logger.h"
#include "common.h"
#include "platform_layer.h"
#include "cpu_gpu_shared_types.h"
#include "lightsource.h"
#include "window_size.h"
#include "texture_array.h"
#include "objmodel.h"

#define FPS_COUNTER_OBJECT_ID 0

#ifdef __cplusplus
extern "C" {
#endif

void zcamera_move_forward(
    GPUCamera * to_move,
    const float distance);

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

typedef struct zPolygonCPU {
    int32_t mesh_id; // data in all_mesh_summaries[mesh_id]
    VertexMaterial vertex_materials[MAX_MATERIALS_SIZE];
    uint32_t vertex_materials_size;
    
    int32_t object_id;
    int32_t touchable_id;
    float hitbox_width;
    float hitbox_height;
    float hitbox_depth;
    
    bool32_t deleted;
    bool32_t visible;
} zPolygonCPU;

typedef struct zPolygonCollection {
    GPUPolygon gpu_data[MAX_POLYGONS_PER_BUFFER];
    zPolygonCPU cpu_data[MAX_POLYGONS_PER_BUFFER];
    uint32_t size;
} zPolygonCollection;
void construct_zpolygon(GPUPolygon * a, zPolygonCPU * b);
void request_zpolygon_to_render(GPUPolygon * a, zPolygonCPU * b);

// A buffer of zPolygon objects that should be rendered
// in your application
// index 0 to zpolygons_to_render_size will be rendered,
// the rest of the array will be ignored
extern zPolygonCollection * zpolygons_to_render;

void delete_zpolygon_object(const int32_t with_object_id);

zTriangle
x_rotate_ztriangle(
    const zTriangle * input,
    const float angle);
zTriangle
y_rotate_ztriangle(
    const zTriangle * input,
    const float angle);
zTriangle 
z_rotate_ztriangle(
    const zTriangle * input,
    const float angle);

zTriangle translate_ztriangle(
    const zTriangle * input,
    const float by_x,
    const float by_y,
    const float by_z);

float dot_of_zvertices(
    const zVertex * a,
    const zVertex * b);

zVertex get_ztriangle_normal(
    const zTriangle * input);

float get_y_multiplier_for_height(
    zPolygonCPU * for_poly,
    const float for_height);
float get_x_multiplier_for_width(
    zPolygonCPU * for_poly,
    const float for_width);

void scale_zpolygon_multipliers_to_height(
    zPolygonCPU * cpu_data,
    GPUPolygon * gpu_data,
    const float new_height);

//void center_zpolygon_offsets(
//    zPolygon * to_center);

//float zpolygon_get_width(
//    const zPolygon * to_inspect);

float get_avg_z(
    const zTriangle * of_triangle);

int sorter_cmpr_lowest_z(
    const void * a,
    const void * b);

void z_sort(
    zTriangle * triangles,
    const uint32_t triangles_size);

float get_distance(
    const zVertex p1,
    const zVertex p2);

float distance_to_ztriangle(
    const zVertex p1,
    const zTriangle p2);

bool32_t ray_intersects_zpolygon_hitbox(
    const zVertex * ray_origin,
    const zVertex * ray_direction,
    const zPolygonCPU * cpu_data,
    const GPUPolygon  * gpu_data,
    zVertex * recipient_hit_point);

void construct_quad_around(
    const float mid_x,
    const float mid_y,
    const float z,
    const float width,
    const float height,
    GPUPolygon * gpu_data,
    zPolygonCPU * cpu_data);

void construct_quad(
    const float left_x,
    const float bottom_y,
    const float z,
    const float width,
    const float height,
    GPUPolygon * gpu_data,
    zPolygonCPU * cpu_data);

void construct_cube_around(
    const float mid_x,
    const float mid_y,
    const float z,
    const float width,
    const float height,
    const float depth,
    GPUPolygon * gpu_data,
    zPolygonCPU * cpu_data);

#ifdef __cplusplus
}
#endif

#endif // ZPOLYGON_H
