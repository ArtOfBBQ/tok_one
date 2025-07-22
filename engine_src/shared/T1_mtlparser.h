#ifndef MTLPARSER_H
#define MTLPARSER_H

#include <stdio.h> // TODO: delete me
#include <assert.h> // TODO: delete me

#include "T1_common.h"
#include "T1_tokenizer.h"

typedef struct ParsedMaterial {
    char name[MATERIAL_NAME_CAP];
    char ambient_map[MATERIAL_NAME_CAP];
    char diffuse_map[MATERIAL_NAME_CAP];
    char specular_map[MATERIAL_NAME_CAP];
    char specular_exponent_map[MATERIAL_NAME_CAP];
    char bump_or_normal_map[MATERIAL_NAME_CAP];
    float bump_map_intensity;
    float specular_exponent;
    float refraction;
    float alpha;
    float illum;
    float roughness; // Pr in mtl files
    float metallic; // Pm in mtl files
    float sheen; // Ps in mtl files
    float clearcoat; // Pc in mtl files
    float clearcoat_roughness; // Pcr in mtl files
    float anisotropy; // aniso in mtl files
    float anisotropy_rotation; // anisor in mtl files
    float ambient_rgb[3];
    float diffuse_rgb[3];
    float specular_rgb[3];
    float emissive_rgb[3];
    float T1_uv_scroll[2];
    uint8_t use_base_mtl_flag;
} ParsedMaterial;

void mtlparser_init(
    void * (* memset_func)(void *, int, size_t),
    void * (* malloc_func)(size_t),
    size_t (* strlcat)(char *, const char *, size_t));

const char * mtlparser_get_last_error_msg(void);

void mtlparser_parse(
    ParsedMaterial * recipient,
    uint32_t * recipient_size,
    const uint32_t recipient_cap,
    const char * input,
    uint32_t * good);

#endif // MTLPARSER_H

