#ifndef CPU_GPU_SHARED_TYPES_H
#define CPU_GPU_SHARED_TYPES_H

#include "clientlogic_macro_settings.h"

#define CAMERADEPTH_TEXTUREARRAY_I 30
#define SHADOWMAP_TEXTUREARRAY_I 31

#ifndef TEXTUREARRAYS_SIZE
#define TEXTUREARRAYS_SIZE 29
#endif

#define DOWNSAMPLES_SIZE 5
#define DOWNSAMPLES_CUTOFF 4

#ifndef MAX_FILES_IN_SINGLE_TEXARRAY
#define MAX_FILES_IN_SINGLE_TEXARRAY 200
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
#define PARENT_MATERIAL_BASE 4294967295
typedef struct GPULockedVertex {
    float        xyz          [3];        // 12 bytes
    float        normal_xyz   [3];        // 12 bytes
    float        tangent_xyz  [3];        // 12 bytes
    float        bitangent_xyz[3];        // 12 bytes
    float        uv           [2];        //  8 bytes
    unsigned int locked_materials_head_i; //  4 bytes
    unsigned int parent_material_i;       //  4 bytes
    float        padding[2];              // 12 bytes
} __attribute__((aligned(32))) GPULockedVertex;

typedef struct GPUCamera {
    float xyz[3];           // 12 bytes
    float xyz_angle[3];     // 12 bytes
    float xyz_cosangle[3];  // 12 bytes
    float xyz_sinangle[3];  // 12 bytes
    float padding[2];       //  8 bytes
} GPUCamera;

typedef struct GPULockedMaterial {
    float ambient_rgb[3];
    float diffuse_rgb[3];
    float specular_rgb[3];
    float rgb_cap[3];
    int   texturearray_i;
    int   texture_i;
    #if NORMAL_MAPPING_ACTIVE
    int   normalmap_texturearray_i;
    int   normalmap_texture_i;
    #endif
    float specular_exponent;
    float refraction;
    float alpha;
    float illum;
} GPULockedMaterial;

typedef struct GPUzSprite {
    float             xyz[3];
    float             xyz_angle[3];
    float             bonus_rgb[3];
    float             xyz_multiplier[3]; // determines width/height/depth
    float             xyz_offset[3];
    float             base_mat_uv_offsets[2];
    float             scale_factor;
    float             alpha;
    float             ignore_lighting;
    float             ignore_camera;
    unsigned int      remove_shadow;
    int               touchable_id;
    GPULockedMaterial base_material;
} __attribute__((aligned(32))) GPUzSprite; // 26 floats (? SIMD runs)

typedef struct GPUSpriteCollection {
    GPUzSprite   polygons[MAX_ZSPRITES_PER_BUFFER];
    unsigned int size;
} GPUSpriteCollection;

typedef struct GPULight {
    float xyz[3];
    float angle_xyz[3];
    float ambient;
    float diffuse;
    float specular;
    float reach;
    float rgb[3];
} GPULight;

//typedef struct GPULightCollection {
//    float        light_x              [MAX_LIGHTS_PER_BUFFER];
//    float        light_y              [MAX_LIGHTS_PER_BUFFER];
//    float        light_z              [MAX_LIGHTS_PER_BUFFER];
//    float        angle_x              [MAX_LIGHTS_PER_BUFFER];
//    float        angle_y              [MAX_LIGHTS_PER_BUFFER];
//    float        angle_z              [MAX_LIGHTS_PER_BUFFER];
//    float        ambient              [MAX_LIGHTS_PER_BUFFER];
//    float        diffuse              [MAX_LIGHTS_PER_BUFFER];
//    float        specular             [MAX_LIGHTS_PER_BUFFER];
//    float        reach                [MAX_LIGHTS_PER_BUFFER];
//    float        red                  [MAX_LIGHTS_PER_BUFFER];
//    float        green                [MAX_LIGHTS_PER_BUFFER];
//    float        blue                 [MAX_LIGHTS_PER_BUFFER];
//} GPULightCollection;

typedef struct GPUProjectionConstants {
    float znear;
    float zfar;
    float field_of_view_rad;
    float field_of_view_modifier;
    float x_multiplier;
    float y_multiplier;
} GPUProjectionConstants;

typedef struct GPUDownsamplingConstants
{
    float texture_width;
    float texture_height;
    float brightness_threshold;
    float brightness_reduction;
} GPUDownsamplingConstants;

typedef struct GPUPostProcessingConstants
{
    float rgb_add[3];
    #if FOG_ACTIVE
    float fog_color[3];
    #endif
    float screen_width;
    float screen_height;
    float nonblur_pct;
    float blur_pct;
    float color_quantization;
    #if FOG_ACTIVE
    float fog_factor;
    #endif
    #if SHADOWS_ACTIVE
    float in_shadow_multiplier;
    #endif
    unsigned int timestamp;
    unsigned int lights_size;
    unsigned int shadowcaster_i;
    int perlin_texturearray_i;
    int perlin_texture_i;
    float padding[7];
} GPUPostProcessingConstants;

typedef struct GPURawVertex {
    float xyz[3];
    float color;
} GPURawVertex;

typedef struct PostProcessingVertex
{
    float position[2];
    float texcoord[2];
} PostProcessingVertex;

#pragma pack(pop)

#endif
