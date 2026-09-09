#ifndef T1_TYPES_PUBLIC_GPUCPU_H
#define T1_TYPES_PUBLIC_GPUCPU_H

#define T1_TEX_NONE 32109

typedef union {
    struct {
        uint32_t reserved_and_tex;
        uint32_t touch_id;
    };
    uint8_t size_with_padding[16];
} T1GPUTexQuadu32;

typedef union {
    struct {
        float xyz[3];
        float offset_xy[2];
        float wh[2];
        float rgba[4];
    };
    uint8_t size_with_padding[64];
} __attribute__((aligned(16))) T1GPUTexQuadf32;

typedef union {
    struct {
        float xyz[3];
        float offset_xyz[3];
        float mul_xyz[3];
        float angle_xyz[3];
        float bloom_on;
        float alpha_on;
    };
    uint8_t size_with_padding[64];
} T1CPUzSpritef32;

typedef union {
    struct {
        float bonus_rgb[3];
        float base_mat_uv_offsets[2];
        float alpha;
        float no_light;
        float no_cam;
        float outline_alpha;
        float shadow_strength;
    };
    uint8_t size_with_padding[48];
} T1GPUzSpritef32;

typedef union {
    struct {
        uint32_t touch_id;
        uint32_t mix_rv_and_mix_tex;
    };
    uint8_t size_with_padding[16];
} T1GPUzSpriteu32;

typedef union {
    struct {
        float ambient_rgb[3];
        float diffuse_rgb[3];
        float specular_rgb[3];
        float uv_scroll[2];
        float specular_exponent;
        float refraction;
        float alpha;
        float illum;
    };
    uint8_t size_with_padding[64];
} T1GPUMatf32;

typedef union {
    struct {
        uint32_t normalmap_tex_and_tex;
    };
    uint8_t size_with_padding[16];
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

typedef struct
{
    uint32_t timestamp;
    uint32_t cam_rv_i;
    uint32_t lights_size;
    int32_t perlin_texturearray_i;
    int32_t perlin_texture_i;
    float rgb_add[3];
    #if T1_FOG_ACTIVE == T1_ACTIVE
    float fog_color[3];
    float fog_factor;
    #elif T1_FOG_ACTIVE == T1_INACTIVE
    #else
    #error
    #endif
    float nonblur_pct;
    float blur_pct;
    float color_quantization;
    #if T1_SHADOWS_ACTIVE == T1_ACTIVE
    float in_shadow_mults[3];
    #elif T1_SHADOWS_ACTIVE == T1_INACTIVE
    #else
    #error
    #endif
    float padding[6];
} T1GPUPostProcConsts;

typedef struct {
    T1GPUTexQuadf32 f32s;
    T1GPUTexQuadu32 u32s;
} T1GPUTexQuad;

typedef struct {
    float xyz[3];
    float xyz_angle[3];
    float xyz_offset[3];
    float RGBA[4];
    float reach; // light's reach
    float diffuse;     // how much diffuse light does this radiate?
    float specular;
} T1zLightf32;

typedef struct {
    T1zLightf32 f32s;
    // you can make a group of lights and/or texquads by
    // giving them the same positive object_id, then make
    // ScheduledAnimations that affect the entire group
    // set to -1 to not be a party of any group
    uint32_t T1_id;
    uint32_t deleted;
    uint32_t committed;
    int32_t shadow_map_depth_texture_i;
    int32_t shadow_map_render_view_i;
    float simd_padding[3];
} T1zLight;

#endif // T1_TYPES_PUBLIC_GPUCPU_H
