#ifndef T1_CPU_GPU_SHARED_TYPES_H
#define T1_CPU_GPU_SHARED_TYPES_H

#include "T1_public_types.h"
#include "client_macro_settings.h"

#define T1_CAM_DEPTH_FRAGARG_I 30
#define T1_SHADOW_MAPS_1ST_FRAGARG_I 31

/*
This isn't a real texture array!
it's used as the write_array_i of a render view that
doesn't have an RGBA output but does output a depth
texture, such as a shadow map
*/
#define T1_DEPTH_TEXTUREARRAYS_I 31 

#define T1_DOWNSAMPLES_SIZE 5
#define T1_DOWNSAMPLES_CUTOFF 4

#pragma pack(push, 1)
typedef struct {
    s32 locked_vertex_i; // index into GPULockedVertex buffer
    s32 polygon_i;       // index into GPUPolygonCollection buffer
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
    f32 xyz            [3];
    f32 norm_xyz[3];
    f32        uv[2];
    #if T1_OUTLINES_ACTIVE == T1_ACTIVE
    f32 face_normal_xyz[3];
    #elif T1_OUTLINES_ACTIVE == T1_INACTIVE
    #else
    #error
    #endif
    #if T1_NORMAL_MAPPING_ACTIVE == T1_ACTIVE
    f32 tan_xyz[3];
    f32 bitan_xyz[3];
    #elif T1_NORMAL_MAPPING_ACTIVE == T1_INACTIVE
    #else
    #error
    #endif
    u32 locked_materials_head_i;
    u32 parent_material_i;
} __attribute__((aligned(16))) T1GPULockedVertex;

typedef enum : u32 {
    T1RENDERVIEW_WRITE_BELOWBOUNDS = 0,
    T1RENDERVIEW_WRITE_RENDER_TARGET = 1,
    T1RENDERVIEW_WRITE_DEPTH = 2,
    T1RENDERVIEW_WRITE_RGBA = 3,
    T1RENDERVIEW_WRITE_ABOVEBOUNDS = 4,
} T1RenderViewWriteType;

typedef struct {
    // v = view, p = projection
    f32    v_4x4[16];
    f32    p_4x4[16];
    f32    normv_3x3[9];
    f32    cull_below_z;
    f32    cull_above_z;
    s32      read_from_shadow_maps;
    s32      write_to_shadow_maps;
} T1GPURenderView;

typedef struct {
    f32 ambient_rgb[3];
    f32 diffuse_rgb[3];
    f32 specular_rgb[3];
    f32 uv_scroll[2];
    f32 specular_exponent;
    f32 refraction;
    f32 alpha;
    f32 illum;
} T1GPUConstMatf32;

typedef struct {
    s32 normalmap_tex_and_tex;
} T1GPUConstMats32;

typedef struct {
    T1GPUConstMatf32 base_mat_f32; // start f32 here
    f32              bonus_rgb[3];
    f32              base_mat_uv_offsets[2];
    f32              alpha;
    f32              no_lighting;
    f32              no_camera;
    f32              outline_alpha;
    f32              shadow_strength;
    f32              f32_padding[7];
} T1GPUzSpritef32;

typedef struct {
    T1GPUConstMats32 base_mat_s32; // start of s32
    s32 touch_id;
    s32 mix_rv_and_mix_tex;
} __attribute__((aligned(16))) T1GPUzSprites32;

typedef struct {
    T1GPUzSpritef32 f32s;
    T1GPUzSprites32 s32;
} __attribute__((aligned(16))) T1GPUzSprite;

typedef struct {
    f32 m_4x4[16];
    f32 norm_3x3[9];
} T1GPUzSpriteMatrices;

typedef struct {
    T1GPUzSprite polygons[T1_ZSPRITES_CAP];
    u32 size;
} T1GPUzSpriteList;

typedef struct {
    f32 xyz[3];
    f32 size;
    f32 rgba[4];
} T1GPUFlatQuad;

typedef struct {
    f32 xyz[3];
    f32 angle_xyz[3];
    f32 diffuse;
    f32 specular;
    f32 reach;
    f32 rgb[3];
    s32   shadow_map_depth_tex_i;
    s32   shadow_map_render_view_i;
} T1GPULight;

typedef struct
{
    f32 texture_width;
    f32 texture_height;
    f32 brightness_threshold;
    f32 brightness_reduction;
} T1GPUDownsamplingConstants;

typedef struct
{
    f32 position[2];
    f32 texcoord[2];
} T1PostProcessingVertex;
#pragma pack(pop)

#endif // T1_CPU_GPU_SHARED_TYPES_H
