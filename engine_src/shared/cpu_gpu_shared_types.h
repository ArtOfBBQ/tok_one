#ifndef CPU_GPU_SHARED_TYPES_H
#define CPU_GPU_SHARED_TYPES_H

#ifndef TEXTUREARRAYS_SIZE
#define TEXTUREARRAYS_SIZE 31
#endif

#ifndef MAX_FILES_IN_SINGLE_TEXARRAY
#define MAX_FILES_IN_SINGLE_TEXARRAY 200
#endif

#ifndef MAX_POLYGONS_PER_BUFFER
#define MAX_POLYGONS_PER_BUFFER  40000
#endif

#ifndef MAX_LIGHTS_PER_BUFFER
#define MAX_LIGHTS_PER_BUFFER       75
#endif

#ifndef MAX_MATERIALS_PER_POLYGON
#define MAX_MATERIALS_PER_POLYGON   15
#endif

#ifndef MAX_VERTICES_PER_BUFFER
#define MAX_VERTICES_PER_BUFFER 3000000
#endif

#ifndef ALL_LOCKED_VERTICES_SIZE
#define ALL_LOCKED_VERTICES_SIZE 1500000
#endif

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
    float        xyz       [3];     // 12 bytes
    float        normal_xyz[3];     // 12 bytes
    float        uv        [2];     //  8 bytes
    unsigned int parent_material_i; // 4 bytes
    float        padding[3];        // 12 bytes
} GPULockedVertex;

typedef struct GPUCamera {
    float xyz[3];           // 12 bytes
    float xyz_angle[3];     // 12 bytes
    float padding[2];       //  8 bytes
} GPUCamera;

typedef struct GPUPolygon {
    float        xyz[3];
    float        xyz_angle[3];
    float        bonus_rgb[3];
    float        xyz_multiplier[3]; // determines width/height/depth
    float        xyz_offset[3];
    float        scale_factor;
    float        ignore_lighting;
    float        ignore_camera;
    float        simd_padding[6];
} GPUPolygon; // 24 floats (3 SIMD runs)

typedef struct GPUPolygonCollection {
    GPUPolygon   polygons[MAX_POLYGONS_PER_BUFFER];
    unsigned int size;
} GPUPolygonCollection;

typedef struct GPUPolygonMaterial {
    float rgba[4];          // 16 bytes
    int   texturearray_i;   //  4 bytes
    int   texture_i;        //  4 bytes
    float simd_padding[2];  //  8 bytes
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
    float znear;
    float zfar;
    float q;
    float field_of_view_rad;
    float field_of_view_modifier;
    float x_multiplier;
    float y_multiplier;
    float padding;
} GPUProjectionConstants;
#pragma pack(pop)

#endif

