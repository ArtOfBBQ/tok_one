#ifndef T1_TYPES_PUBLIC_GPUCPU_H
#define T1_TYPES_PUBLIC_GPUCPU_H

#ifdef __METAL_VERSION__
#else
#include <stdint.h>
#endif

#define T1_TEX_NONE 32109

typedef struct {
    uint32_t reserved_and_tex;
    uint32_t touch_id;
    uint32_t padding[2];
} T1GPUTexQuadu32;

typedef struct {
    float xyz[3];
    float offset_xy[2];
    float wh[2];
    float rgba[4];
    float padding[5];
} T1GPUTexQuadf32;

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

#endif // T1_TYPES_PUBLIC_GPUCPU_H
