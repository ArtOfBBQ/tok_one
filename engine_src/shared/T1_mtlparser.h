#ifndef MTLPARSER_H
#define MTLPARSER_H

#include <stdint.h>
#include <stddef.h>

#include "T1_stdint.h"

typedef struct ParsedMaterial {
    char name[T1_MATERIAL_NAME_CAP];
    char ambient_map[T1_MATERIAL_NAME_CAP];
    char diffuse_map[T1_MATERIAL_NAME_CAP];
    char specular_map[T1_MATERIAL_NAME_CAP];
    char specular_exponent_map[T1_MATERIAL_NAME_CAP];
    char bump_or_normal_map[T1_MATERIAL_NAME_CAP];
    f32 bump_map_intensity;
    f32 specular_exponent;
    f32 refraction;
    f32 alpha;
    f32 illum;
    f32 roughness; // Pr in mtl files
    f32 metallic; // Pm in mtl files
    f32 sheen; // Ps in mtl files
    f32 clearcoat; // Pc in mtl files
    f32 clearcoat_roughness; // Pcr in mtl files
    f32 anisotropy; // aniso in mtl files
    f32 anisotropy_rotation; // anisor in mtl files
    f32 ambient_rgb[3];
    f32 diffuse_rgb[3];
    f32 specular_rgb[3];
    f32 emissive_rgb[3];
    f32 T1_uv_scroll[2];
    u8 use_base_mtl_flag;
} ParsedMaterial;

void mtlparser_init(
    void * (* memset_func)(void *, int, u64),
    void * (* malloc_func)(u64),
    u64 (* strlcat)(char *, const char *, u64));

const char * mtlparser_get_last_error_msg(void);

void mtlparser_parse(
    ParsedMaterial * recipient,
    u32 * recipient_size,
    const u32 recipient_cap,
    const char * input,
    u8 * good);

#endif // MTLPARSER_H

