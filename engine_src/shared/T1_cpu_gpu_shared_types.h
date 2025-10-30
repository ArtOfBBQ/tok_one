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
typedef struct {
    int locked_vertex_i; // index into GPULockedVertex buffer
    int polygon_i;       // index into GPUPolygonCollection buffer
} T1GPUVertexIndices;

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
typedef struct {
    float        xyz          [3];        // 12 bytes
    float        normal_xyz   [3];        // 12 bytes
    float        tangent_xyz  [3];        // 12 bytes
    float        bitangent_xyz[3];        // 12 bytes
    float        uv           [2];        //  8 bytes
    unsigned int locked_materials_head_i; //  4 bytes
    unsigned int parent_material_i;       //  4 bytes
    float        padding[2];              // 12 bytes
} __attribute__((aligned(32))) T1GPULockedVertex;

typedef struct {
    float xyz[3];           // 12 bytes
    float xyz_angle[3];     // 12 bytes
    float xyz_cosangle[3];  // 12 bytes
    float xyz_sinangle[3];  // 12 bytes
    float padding[2];       //  8 bytes
} T1GPUCamera;

typedef struct {
    float ambient_rgb[3];
    float diffuse_rgb[3];
    float specular_rgb[3];
    float rgb_cap[3];
    float uv_scroll[2];
    int   texturearray_i;
    int   texture_i;
    #if T1_NORMAL_MAPPING_ACTIVE
    int   normalmap_texturearray_i;
    int   normalmap_texture_i;
    #endif
    float specular_exponent;
    float refraction;
    float alpha;
    float illum;
} T1GPUConstMat;

typedef struct {
    T1GPUConstMat base_mat;
    float         model_4x4[16];
    float         bonus_rgb[3];
    float         base_mat_uv_offsets[2];
    float         alpha;
    float         ignore_lighting;
    float         ignore_camera;
    unsigned int  remove_shadow;
    int           touchable_id;
} __attribute__((aligned(32))) T1GPUzSprite;

typedef struct {
    T1GPUzSprite polygons[MAX_ZSPRITES_PER_BUFFER];
    unsigned int size;
} T1GPUzSpriteList;

typedef struct {
    float xyz[3];
    float angle_xyz[3];
    float diffuse;
    float specular;
    float reach;
    float rgb[3];
} T1GPULight;

typedef struct {
    float znear;
    float zfar;
    float field_of_view_rad;
    float field_of_view_modifier;
    float x_multiplier;
    float y_multiplier;
} T1GPUProjectConsts;

typedef struct
{
    float texture_width;
    float texture_height;
    float brightness_threshold;
    float brightness_reduction;
} T1GPUDownsamplingConstants;

typedef struct
{
    float rgb_add[3];
    #if T1_FOG_ACTIVE == T1_ACTIVE
    float fog_color[3];
    #elif T1_FOG_ACTIVE == T1_INACTIVE
    #else
    #error
    #endif
    float screen_width;
    float screen_height;
    float nonblur_pct;
    float blur_pct;
    float color_quantization;
    #if T1_FOG_ACTIVE == T1_ACTIVE
    float fog_factor;
    #elif T1_FOG_ACTIVE == T1_INACTIVE
    #else
    #error
    #endif
    #if T1_SHADOWS_ACTIVE == T1_ACTIVE
    float in_shadow_multipliers[3];
    #elif T1_SHADOWS_ACTIVE == T1_INACTIVE
    #else
    #error
    #endif
    unsigned int timestamp;
    unsigned int lights_size;
    unsigned int shadowcaster_i;
    int perlin_texturearray_i;
    int perlin_texture_i;
    float padding[7];
} T1GPUPostProcConsts;

typedef struct {
    float xyz[3];
    float color;
} T1GPURawVertex;

typedef struct
{
    float position[2];
    float texcoord[2];
} T1PostProcessingVertex;

#pragma pack(pop)

#endif
