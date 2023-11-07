#ifndef CPU_GPU_SHARED_TYPES_H
#define CPU_GPU_SHARED_TYPES_H

#define TEXTUREARRAYS_SIZE 31
#define MAX_FILES_IN_SINGLE_TEXARRAY 200

#include "clientlogic_macro_settings.h"

#pragma pack(push, 1)
typedef struct GPUVertex {
    float x;
    float y;
    float z;
    /* Currently same for "entire" triangle */
    float normal_x;
    float normal_y;
    float normal_z;
    /* Below: same for entire material: */
    float uv  [2];
    float RGBA[4];
    int   texturearray_i; // -1 for no texture
    int   texture_i;      // -1 for no texture
    int   polygon_i;      // index into GPUPolygonCollection buffer
} GPUVertex;

/*
A 'set and forget' description of a triangle vertex, intended to stay static
for a long time or even the entire duration of the app.

When our app loads, we parse .obj files to get the triangle vertices in our 3D
models up-front, and keep these inplace. Ideally, we send them to the GPU once
and never update them, which will easily work for the small apps/games I'm
making.
 
To manipulate the location, direction, size etc. of our objects, we manipulate
the parents that contain these (see zpolygons_to_render in zpolygon.h),
not this.
*/
typedef struct GPULockedVertex {
    float xyz       [3];
    float normal_xyz[3];
    float uv        [2];
    int material_i;
} GPULockedVertex;

typedef struct GPUCamera {
    float x;
    float y;
    float z;
    float x_angle;
    float y_angle;
    float z_angle;
} GPUCamera;

typedef struct GPUPolygonCollection {
    float xyz            [MAX_POLYGONS_PER_BUFFER][3];
    float xyz_angle      [MAX_POLYGONS_PER_BUFFER][3];
    float xyz_multiplier [MAX_POLYGONS_PER_BUFFER][3];
    float xy_offset      [MAX_POLYGONS_PER_BUFFER][2];
    float bonus_rgb      [MAX_POLYGONS_PER_BUFFER][3];
    float scale_factor   [MAX_POLYGONS_PER_BUFFER];
    float ignore_lighting[MAX_POLYGONS_PER_BUFFER];
    float ignore_camera  [MAX_POLYGONS_PER_BUFFER];
    unsigned int size;
} GPUPolygonCollection;

typedef struct GPULightCollection {
    float light_x[MAX_LIGHTS_PER_BUFFER];
    float light_y[MAX_LIGHTS_PER_BUFFER];
    float light_z[MAX_LIGHTS_PER_BUFFER];
    float ambient[MAX_LIGHTS_PER_BUFFER];
    float diffuse[MAX_LIGHTS_PER_BUFFER];
    float reach  [MAX_LIGHTS_PER_BUFFER];
    float red    [MAX_LIGHTS_PER_BUFFER];
    float green  [MAX_LIGHTS_PER_BUFFER];
    float blue   [MAX_LIGHTS_PER_BUFFER];
    unsigned int lights_size;
} GPULightCollection;

typedef struct GPUProjectionConstants {
    float near;
    float far;
    float q;
    float field_of_view_rad;
    float field_of_view_modifier;
    float x_multiplier;
    float y_multiplier;
} GPUProjectionConstants;
#pragma pack(pop)

#endif

