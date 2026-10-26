#ifndef T1_TYPES_GPUCPU_H
#define T1_TYPES_GPUCPU_H

#include "T1_stdint.h"
#include "T1_macro_settings.h"
#include "T1_types_public_gpucpu.h"

/*
Where shaders find what T1 binds for every pass:
- texture(0) to texture(T1_TEXARRAYS_CAP - 1): every texture array, so a
  shader can sample any T1Tex (see T1_TEX_ARRAY_I and T1_TEX_SLICE_I)
- buffer(T1_READ_TEXS_BUFFER_I): the pass's "read_tex[0]" and up, a
  constant array of T1_READ_TEXS_CAP uint32_t T1Texs
- texture(T1_WRITE_TEX_I): a compute pass's "write_tex", one thread per
  pixel of it (uint2 [[thread_position_in_grid]])
- buffer(T1_FRAME_CONSTS_I): a T1GPUFrameConsts
- buffer(T1_BUFS_I + k): the pass's "buf[k]", a T1Buf the shader declares
  as whatever type it holds, e.g. device MyParticle * [[buffer(T1_BUFS_I)]]
Draw passes bind these for both their vertex and fragment functions.
*/
#define T1_FRAME_CONSTS_I 7
#define T1_READ_TEXS_BUFFER_I 9
#define T1_READ_TEXS_CAP 8
#define T1_BUFS_I 16
#define T1_PASS_BUFS_CAP 8
#define T1_WRITE_TEX_I 30

// What T1 tells shaders about the frame, see T1_FRAME_CONSTS_I
typedef struct
{
    uint32_t timestamp; // microseconds
    uint32_t cam_rv_i;
    uint32_t lights_size;
    uint32_t padding;
} T1GPUFrameConsts;

#pragma pack(push, 1)
typedef struct {
    s32 locked_vertex_i; // index into GPULockedVertex buffer
    s32 polygon_i;       // index into GPUPolygonCollection buffer
} T1GPUVertexIndices;

/*
A triangle vertex of a loaded mesh, uploaded once. Objects are moved through
their zsprite's values, not these.
*/
#define PARENT_MATERIAL_BASE 4294967295
typedef struct {
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
} T1GPULockedVertex;

typedef struct {
    // v = view, p = projection
    f32    v_4x4[16];
    f32    p_4x4[16];
    f32    normv_3x3[9]; // 16 + 16 + 9 = 41
    f32    cull_below_z;
    f32    cull_above_z;
    // a light's "shadow_cam"
    s32    write_to_shadow_maps;
    u32    visible_layers; // draws the objects whose layers share a bit
    s32    padding[3];
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

typedef struct {
    f32 xyz[3];
    f32 angle_xyz[3];
    f32 diffuse;
    f32 specular;
    f32 reach;
    f32 rgb[3];
    s32 shadow_cam_i; // -1 for no shadows
    u32 shadow_tex; // the T1Tex its shadow cam's depth is packed into
    u32 layers; // lights the render views whose visible_layers share a bit
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

typedef struct {
    float bonus_rgb[3];
    float base_mat_uv_offsets[2];
    float alpha;
    float no_light;
    float no_cam;
    float outline_alpha;
    float shadow_strength;
    float vertex_noise_z;
    float padding[1];
} T1GPUzSpritef32;

typedef struct {
    uint32_t touch_id;
    uint32_t mix_rv_and_mix_tex;
    // drawn by the render views whose visible_layers share a bit with it
    uint32_t layers;
    uint32_t padding[1];
} T1GPUzSpriteu32;

typedef struct {
    float ambient_rgb[3];
    float diffuse_rgb[3];
    float specular_rgb[3];
    float uv_scroll[2];
    float specular_exponent;
    float refraction;
    float alpha;
    float illum;
    float padding[1];
} T1GPUMatf32;

typedef struct {
    uint32_t normalmap_tex_and_tex;
    uint32_t padding[3];
} T1GPUMatu32;

typedef struct {
    T1GPUzSpritef32 f32s;
    T1GPUzSpriteu32 u32s;
    T1GPUMatf32     base_mat_f32; // start f32 here
    T1GPUMatu32     base_mat_u32;
} T1GPUzSprite;

typedef struct {
    T1GPUzSprite polygons[T1_ZSPRITES_CAP];
    uint32_t size;
} T1GPUzSpriteList;

typedef struct {
    uint32_t reserved_and_tex;
    uint32_t touch_id;
    // drawn by the render views whose visible_layers share a bit with it
    uint32_t layers;
    uint32_t padding[1];
} T1GPUTexQuadu32;

typedef struct {
    float xyz[3];
    float offset_xy[2];
    float wh[2];
    float rgba[4];
    float mul_wh[2]; // scales wh when drawing, 1.0f by default
    float padding[3];
} T1GPUTexQuadf32;

typedef struct {
    T1GPUTexQuadf32 f32s;
    T1GPUTexQuadu32 u32s;
} T1GPUTexQuad;

#endif // T1_TYPES_GPUCPU_H
