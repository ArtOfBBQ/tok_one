#ifndef ZPOLYGON_H
#define ZPOLYGON_H

#include <math.h>
#include <inttypes.h>

#include "simd.h"
#include "logger.h"
#include "common.h"
#include "platform_layer.h"
#include "cpu_gpu_shared_types.h"
#include "lightsource.h"
#include "window_size.h"
#include "texture_array.h"

#ifdef __cplusplus
extern "C" {
#endif

void zcamera_move_forward(
    zCamera * to_move,
    const float distance);

typedef struct zTriangle {
    zVertex vertices[3];
    zVertex normal;
    float color[4];         // RGBA, ignored if textured
    int32_t texturearray_i; /*
                            the index in the global var
                            'texture_arrays' of the texturearray
                             -1 for "untextured, use color
                            instead"
                            */
    int32_t texture_i;     // index in texturearray
    uint32_t visible;
} zTriangle;

#define POLYGON_TRIANGLES_SIZE 8400
typedef struct zPolygon {
    int32_t object_id;
    int32_t touchable_id;
    zTriangle triangles[POLYGON_TRIANGLES_SIZE];
    uint32_t triangles_size;
    float x;
    float y;
    float z;
    float x_angle;
    float y_angle;
    float z_angle;
    float scale_factor;
    bool32_t ignore_camera;
    bool32_t ignore_lighting;
    bool32_t deleted;
} zPolygon;
void construct_zpolygon(zPolygon * to_construct);
void request_zpolygon_to_render(zPolygon * to_add);

// A buffer of zPolygon objects that should be rendered
// in your application
// index 0 to zpolygons_to_render_size will be rendered,
// the rest of the array will be ignored
#define ZPOLYGONS_TO_RENDER_ARRAYSIZE 1000
extern zPolygon zpolygons_to_render[ZPOLYGONS_TO_RENDER_ARRAYSIZE];
extern uint32_t zpolygons_to_render_size;

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
zVertex crossproduct_of_zvertices(
    const zVertex * a,
    const zVertex * b);

zVertex get_ztriangle_normal(
    const zTriangle * input);

/*
The base version of parse_obj will just ignore materials and set all texture_i
's to -1, all texturearray_i's to -1, and all colors to 1.0f.

If you need more specific behavior, pass an array of ExpectedObjMaterials to
set textures or colors

For example, you could set material_name to 'marble' and set texturearray_i 5
and texture_i 1 if you had labeled some faces of your object with the material
'marble' in Blender and your marble texture was stored at 5,1.
You could set the texturearray_i and texture_i to -1 and use a color for
another material name, etc.
*/
zPolygon parse_obj(
    char * rawdata,
    uint64_t rawdata_size,
    const bool32_t flip_winding);
typedef struct ExpectedObjMaterials {
    char material_name[16];
    int32_t texturearray_i;
    int32_t texture_i;
    float rgba[4];
} ExpectedObjMaterials;
zPolygon parse_obj_expecting_materials(
    char * rawdata,
    uint64_t rawdata_size,
    ExpectedObjMaterials * expected_materials,
    const uint32_t expected_materials_size,
    const bool32_t flip_winding);

void zpolygon_scale_to_width_given_z(
    zPolygon * to_scale,
    const float new_width,
    const float when_observed_at_z);

void scale_zpolygon(
    zPolygon * to_scale,
    const float new_height);

void center_zpolygon_offsets(
    zPolygon * to_center);

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

bool32_t ray_intersects_triangle(
    const zVertex * ray_origin,
    const zVertex * ray_direction,
    const zTriangle * triangle,
    zVertex * recipient_hit_point);

bool32_t ray_intersects_zpolygon(
    const zVertex * ray_origin,
    const zVertex * ray_direction,
    const zPolygon * mesh,
    zVertex * recipient_hit_point);

zPolygon construct_quad(
    const float left_x,
    const float top_y,
    const float width,
    const float height);

#ifdef __cplusplus
}
#endif

#endif // ZPOLYGON_H
