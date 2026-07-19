#ifndef T1_TYPES_GPUCPU_H
#define T1_TYPES_GPUCPU_H

#include "T1_stdint.h"
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
typedef union {
    struct {
        f32 xyz[3];
        f32 norm_xyz[3];
        f32 uv[2];
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
    };
    u8 target_size_with_padding[40];
} T1GPULockedVertex;

typedef enum : u32 {
    T1RENDERVIEW_WRITE_BELOWBOUNDS = 0,
    T1RENDERVIEW_WRITE_RENDER_TARGET = 1,
    T1RENDERVIEW_WRITE_DEPTH = 2,
    T1RENDERVIEW_WRITE_RGBA = 3,
    T1RENDERVIEW_WRITE_ABOVEBOUNDS = 4,
} T1RenderViewWriteType;

typedef union {
    struct {
        // v = view, p = projection
        f32    v_4x4[16];
        f32    p_4x4[16];
        f32    normv_3x3[9]; // 16 + 16 + 9 = 41
        f32    cull_below_z;
        f32    cull_above_z;
        s32    read_from_shadow_maps;
        s32    write_to_shadow_maps;
    };
    u8 size_with_padding[192];
} T1GPURenderView;

typedef struct {
    f32 m_4x4[16];
    f32 norm_3x3[9];
} T1GPUzSpriteMatrices;

typedef struct {
    f32 xyz[3];
    f32 size;
    f32 rgba[4];
} T1GPUFlatQuad;

typedef union {
    struct {
        f32 xyz[3];
        f32 angle_xyz[3];
        f32 diffuse;
        f32 specular;
        f32 reach;
        f32 rgb[3];
        s32 shadow_map_depth_tex_i;
        s32 shadow_map_render_view_i;
    };
    u8 target_size_with_padding[56];
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

#endif // T1_TYPES_GPUCPU_H
