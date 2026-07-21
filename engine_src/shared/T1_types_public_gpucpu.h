#ifndef T1_TYPES_PUBLIC_GPUCPU_H
#define T1_TYPES_PUBLIC_GPUCPU_H

#include "T1_stdint.h"

#define T1_TEX_NONE 32109

typedef union {
    struct {
        s32 reserved_and_tex;
        s32 touch_id;
    };
    u8 size_with_padding[16];
} T1GPUTexQuads32;

typedef union {
    struct {
        f32 xyz[3];
        f32 offset_xy[2];
        f32 wh[2];
        f32 rgba[4];
    };
    u8 size_with_padding[64];
} __attribute__((aligned(16))) T1GPUTexQuadf32;

typedef union {
    struct {
        f32 xyz[3];
        f32 offset_xyz[3];
        f32 mul_xyz[3];
        f32 angle_xyz[3];
        f32 bloom_on;
        f32 alpha_on;
    };
    u8 size_with_padding[64];
} T1CPUzSpritef32;

typedef union {
    struct {
        f32 bonus_rgb[3];
        f32 base_mat_uv_offsets[2];
        f32 alpha;
        f32 no_light;
        f32 no_cam;
        f32 outline_alpha;
        f32 shadow_strength;
    };
    u8 size_with_padding[48];
} T1GPUzSpritef32;

typedef union {
    struct {
        s32 touch_id;
        s32 mix_rv_and_mix_tex;
    };
    u8 size_with_padding[16];
} T1GPUzSprites32;

typedef union {
    struct {
        f32 ambient_rgb[3];
        f32 diffuse_rgb[3];
        f32 specular_rgb[3];
        f32 uv_scroll[2];
        f32 specular_exponent;
        f32 refraction;
        f32 alpha;
        f32 illum;
    };
    u8 size_with_padding[64];
} T1GPUMatf32;

typedef union {
    struct {
        union {
            s32 normalmap_tex_and_tex;
            u32 normalmap_tex_and_tex_u32;
        };
    };
    u8 size_with_padding[16];
} T1GPUMats32;

typedef struct {
    T1GPUzSpritef32 f32s;
    T1GPUzSprites32 s32s;
    T1GPUMatf32     base_mat_f32; // start f32 here
    T1GPUMats32     base_mat_s32;
} T1GPUzSprite;

typedef struct {
    T1GPUzSprite polygons[T1_ZSPRITES_CAP];
    u32 size;
} T1GPUzSpriteList;

typedef struct
{
    u32 timestamp;
    u32 cam_rv_i;
    u32 lights_size;
    s32 perlin_texturearray_i;
    s32 perlin_texture_i;
    f32 rgb_add[3];
    #if T1_FOG_ACTIVE == T1_ACTIVE
    f32 fog_color[3];
    f32 fog_factor;
    #elif T1_FOG_ACTIVE == T1_INACTIVE
    #else
    #error
    #endif
    f32 nonblur_pct;
    f32 blur_pct;
    f32 color_quantization;
    #if T1_SHADOWS_ACTIVE == T1_ACTIVE
    f32 in_shadow_mults[3];
    #elif T1_SHADOWS_ACTIVE == T1_INACTIVE
    #else
    #error
    #endif
    f32 padding[6];
} T1GPUPostProcConsts;

typedef struct {
    T1GPUTexQuadf32 f32s;
    T1GPUTexQuads32 s32s;
} T1GPUTexQuad;

typedef struct {
    // you can make a group of lights and/or texquads by
    // giving them the same positive object_id, then make
    // ScheduledAnimations that affect the entire group
    // set to -1 to not be a party of any group
    union {
        s32 T1_id;
        f32 flt_T1_id;
    };
    union {
        u32 deleted;
        f32 flt_deleted;
    };
    union {
        u32 committed;
        f32 flt_committed;
    };
    f32 xyz[3];
    f32 xyz_angle[3];
    f32 xyz_offset[3];
    f32 RGBA[4];
    f32 reach; // light's reach
    f32 diffuse;     // how much diffuse light does this radiate?
    f32 specular;
    s32 shadow_map_depth_texture_i;
    s32 shadow_map_render_view_i;
    f32 simd_padding[3];
} T1zLight; // 17 f32s = 68 bytes

#endif // T1_TYPES_PUBLIC_GPUCPU_H

