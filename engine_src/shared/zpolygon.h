#ifndef ZPOLYGON_H
#define ZPOLYGON_H

#include <math.h>
#include <inttypes.h>

#include "simd.h"
#include "logger.h"
#include "common.h"
#include "platform_layer.h"
#include "vertex_types.h"
#include "lightsource.h"
#include "window_size.h"
#include "texture_array.h"

#ifdef __cplusplus
extern "C" {
#endif

// projection constants
typedef struct ProjectionConstants {
    float near;
    float far;
    float field_of_view_rad;
    float field_of_view_modifier;
} ProjectionConstants;

extern ProjectionConstants projection_constants;

void init_projection_constants(void);

void zcamera_move_forward(
    zCamera * to_move,
    const float distance);

typedef struct zTriangle {
    zVertex vertices[3];
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

#define POLYGON_TRIANGLES_SIZE 700
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

void ztriangles_apply_lighting(
    float * vertices_x,
    float * vertices_y,
    float * vertices_z,
    float * lighting_multipliers,
    const uint32_t vertices_size,
    Vertex * recipients,
    const uint32_t recipients_size,
    zLightSource * zlight_source);

/*
void 
ztriangle_apply_lighting(
    Vertex recipient[3],
    zTriangle * input,
    zLightSource * zlight_source);
*/

void ztriangles_to_2d_inplace(
    float * vertices_x,
    float * vertices_y,
    float * vertices_z,
    const uint32_t vertices_size);

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

void normalize_zvertex(
    zVertex * to_normalize);

float get_distance(
    const zVertex p1,
    const zVertex p2);

float distance_to_ztriangle(
    const zVertex p1,
    const zTriangle p2);

void get_visibility_ratings(
    const zVertex observer,
    const float * vertices_x,
    const float * vertices_y,
    const float * vertices_z,
    const uint32_t vertices_size,
    float * out_visibility_ratings);
/*
float get_visibility_rating(
    const zVertex observer,
    const zTriangle * observed);
*/

float dot_of_vertices(
    const zVertex vertex_1,
    const zVertex vertex_2);

// float screen_y_to_3d_y(const float screen_y);
// float screen_x_to_3d_x(const float screen_x);

#ifdef __cplusplus
}
#endif

#endif // ZPOLYGON_H
