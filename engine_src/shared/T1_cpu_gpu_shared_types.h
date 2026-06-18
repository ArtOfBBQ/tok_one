#ifndef T1_CPU_GPU_SHARED_TYPES_H
#define T1_CPU_GPU_SHARED_TYPES_H

#include "T1_public_types.h"
#include "clientlogic_macro_settings.h"

#define T1_CAM_DEPTH_FRAGARG_I 30
#define T1_SHADOW_MAPS_1ST_FRAGARG_I 31

#define T1_DEPTH_TEXTUREARRAYS_I 54321

#define T1_DOWNSAMPLES_SIZE 5
#define T1_DOWNSAMPLES_CUTOFF 4

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
    float xyz            [3];
    #if T1_OUTLINES_ACTIVE == T1_ACTIVE
    float face_normal_xyz[3];
    #elif T1_OUTLINES_ACTIVE == T1_INACTIVE
    #else
    #error
    #endif
    float norm_xyz[3];
    #if T1_NORMAL_MAPPING_ACTIVE == T1_ACTIVE
    float tan_xyz[3];
    float bitan_xyz[3];
    #elif T1_NORMAL_MAPPING_ACTIVE == T1_INACTIVE
    #else
    #error
    #endif
    float        uv[2];
    unsigned int locked_materials_head_i;
    unsigned int parent_material_i;
} __attribute__((aligned(16))) T1GPULockedVertex;

typedef enum : unsigned int {
    T1RENDERVIEW_WRITE_BELOWBOUNDS = 0,
    T1RENDERVIEW_WRITE_RENDER_TARGET = 1,
    T1RENDERVIEW_WRITE_DEPTH = 2,
    T1RENDERVIEW_WRITE_RGBA = 3,
    T1RENDERVIEW_WRITE_ABOVEBOUNDS = 4,
} T1RenderViewWriteType;

typedef struct {
    // v = view, p = projection
    float    v_4x4[16];
    float    p_4x4[16];
    float    normv_3x3[9];
    float    cull_below_z;
    float    cull_above_z;
    int      read_from_shadow_maps;
    int      write_to_shadow_maps;
} T1GPURenderView;

typedef struct {
    float ambient_rgb[3];
    float diffuse_rgb[3];
    float specular_rgb[3];
    float uv_scroll[2];
    float specular_exponent;
    float refraction;
    float alpha;
    float illum;
} T1GPUConstMatf32;

typedef struct {
    int normalmap_tex_and_tex;
} T1GPUConstMati32;

typedef struct {
    T1GPUConstMatf32 base_mat_f32; // start f32 here
    float            explode;
    float            bonus_rgb[3];
    float            base_mat_uv_offsets[2];
    float            alpha;
    float            ignore_lighting;
    float            ignore_camera;
    float            outline_alpha;
    float            shadow_strength;
    float            f32_padding[6];
} T1GPUzSpritef32;

typedef struct {
    T1GPUConstMati32 base_mat_i32; // start of i32
    int touch_id;
    int mix_rv_and_mix_tex;
} __attribute__((aligned(16))) T1GPUzSpritei32;

typedef struct {
    T1GPUzSpritef32 f32;
    T1GPUzSpritei32 i32;
} __attribute__((aligned(16))) T1GPUzSprite;

typedef struct {
    float m_4x4[16];
    float norm_3x3[9];
} T1GPUzSpriteMatrices;

typedef struct {
    T1GPUzSprite polygons[T1_ZSPRITES_CAP];
    unsigned int size;
} T1GPUzSpriteList;

typedef struct {
    float xyz[3];
    float size;
    float rgba[4];
} T1GPUFlatQuad;

typedef struct {
    float xyz[3];
    float angle_xyz[3];
    float diffuse;
    float specular;
    float reach;
    float rgb[3];
    int   shadow_map_depth_tex_i;
    int   shadow_map_render_view_i;
} T1GPULight;

typedef struct
{
    float texture_width;
    float texture_height;
    float brightness_threshold;
    float brightness_reduction;
} T1GPUDownsamplingConstants;

typedef struct
{
    float position[2];
    float texcoord[2];
} T1PostProcessingVertex;

#pragma pack(pop)

#endif
