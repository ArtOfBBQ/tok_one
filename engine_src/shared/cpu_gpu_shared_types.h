#ifndef CPU_GPU_SHARED_TYPES_H
#define CPU_GPU_SHARED_TYPES_H

#define TEXTUREARRAYS_SIZE 31
#define MAX_FILES_IN_SINGLE_TEXARRAY 200

#define MAX_POLYGONS_PER_BUFFER  10000
#define MAX_LIGHTS_PER_BUFFER       75
#define MAX_MATERIALS_SIZE          15

#pragma pack(push, 1)
typedef struct GPUVertex {
    int locked_vertex_i; // index into GPULockedVertex buffer
    int polygon_i;       // index into GPUPolygonCollection buffer
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
    float        xyz       [3];
    float        normal_xyz[3];
    float        uv        [2];
    unsigned int parent_material_i;
} GPULockedVertex;

typedef struct GPUCamera {
    float x;
    float y;
    float z;
    float x_angle;
    float y_angle;
    float z_angle;
} GPUCamera;

typedef struct GPUPolygon {
    float        xyz[3];
    float        xyz_angle[3];
    float        bonus_rgb[3];
    float        xyz_multiplier[3];
    float        xyz_offset[3];
    float        scale_factor;
    unsigned int ignore_lighting;
    unsigned int ignore_camera;
} GPUPolygon;

typedef struct GPUPolygonCollection {
    GPUPolygon   polygons[MAX_POLYGONS_PER_BUFFER];
    unsigned int size;
} GPUPolygonCollection;

typedef struct GPUPolygonMaterial {
    float rgba[4];
    int   texturearray_i;
    int   texture_i;
} GPUPolygonMaterial;

typedef struct GPULightCollection {
    float        light_x[MAX_LIGHTS_PER_BUFFER];
    float        light_y[MAX_LIGHTS_PER_BUFFER];
    float        light_z[MAX_LIGHTS_PER_BUFFER];
    float        ambient[MAX_LIGHTS_PER_BUFFER];
    float        diffuse[MAX_LIGHTS_PER_BUFFER];
    float        reach  [MAX_LIGHTS_PER_BUFFER];
    float        red    [MAX_LIGHTS_PER_BUFFER];
    float        green  [MAX_LIGHTS_PER_BUFFER];
    float        blue   [MAX_LIGHTS_PER_BUFFER];
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
