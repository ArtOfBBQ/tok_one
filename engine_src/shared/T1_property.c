#include "T1_property.h"

#include <stddef.h>

#include "T1_std.h"
#include "T1_log.h"
#include "T1_types_public.h"
#include "T1_zsprite.h"
#include "T1_texquad.h"
#include "T1_zlight.h"
#include "T1_anim.h"
#include "T1_text.h"
#include "T1_global.h"
#include "T1_render_view.h"

// the extra layer lets ZS_CPU etc. expand to 2 arguments
#define T1_PROP_F32_(store, type, field) \
    {#field, 0xFFFFFFFF, offsetof(type, field), 0, store, 0}
#define T1_PROP_F32(...) T1_PROP_F32_(__VA_ARGS__)
#define T1_PROP_F32_NAMED_(name, store, type, field) \
    {name, 0xFFFFFFFF, offsetof(type, field), 0, store, 0}
#define T1_PROP_F32_NAMED(...) T1_PROP_F32_NAMED_(__VA_ARGS__)
#define T1_PROP_U32_(name, store, type, field) \
    {name, 0xFFFFFFFF, offsetof(type, field), 0, store, 1}
#define T1_PROP_U32(...) T1_PROP_U32_(__VA_ARGS__)
#define T1_PROP_U32_PACKED_(name, store, type, field, mask, shift) \
    {name, mask, offsetof(type, field), shift, store, 1}
#define T1_PROP_U32_PACKED(...) T1_PROP_U32_PACKED_(__VA_ARGS__)

#define ZS_CPU T1_PROP_STORE_ZS_CPU_F32, T1CPUzSpritef32
#define ZS_GPU T1_PROP_STORE_ZS_GPU_F32, T1GPUzSpritef32
#define ZS_U32 T1_PROP_STORE_ZS_GPU_U32, T1GPUzSpriteu32
#define ZS_MAT T1_PROP_STORE_ZS_MAT_F32, T1GPUMatf32
#define ZS_MTU T1_PROP_STORE_ZS_MAT_U32, T1GPUMatu32
#define TQ_CPU T1_PROP_STORE_TQ_CPU_F32, T1CPUTexQuad
#define TQ_GPU T1_PROP_STORE_TQ_GPU_F32, T1GPUTexQuadf32
#define TQ_U32 T1_PROP_STORE_TQ_GPU_U32, T1GPUTexQuadu32
#define ZL     T1_PROP_STORE_ZL_F32,     T1zLightf32
#define TEXT   T1_PROP_STORE_TEXT,       T1TextFontSettings
#define POST   T1_PROP_STORE_POSTPROC,   T1GPUPostProcConsts
#define CAM    T1_PROP_STORE_CAM_F32,    T1CamF32s
#define CAMU   T1_PROP_STORE_CAM_U32,    T1CPURenderView
#define T1_PROP_U32_CAM_REF_(name, store, type, field, mask, shift, ref) \
    {name, mask, offsetof(type, field), shift, store, 1, ref}
#define T1_PROP_U32_CAM_REF(...) T1_PROP_U32_CAM_REF_(__VA_ARGS__)

static const T1Property T1_properties[] = {
    T1_PROP_F32(ZS_CPU, xyz[0]),
    T1_PROP_F32(ZS_CPU, xyz[1]),
    T1_PROP_F32(ZS_CPU, xyz[2]),
    T1_PROP_F32(ZS_CPU, offset_xyz[0]),
    T1_PROP_F32(ZS_CPU, offset_xyz[1]),
    T1_PROP_F32(ZS_CPU, offset_xyz[2]),
    T1_PROP_F32(ZS_CPU, mul_xyz[0]),
    T1_PROP_F32(ZS_CPU, mul_xyz[1]),
    T1_PROP_F32(ZS_CPU, mul_xyz[2]),
    T1_PROP_F32(ZS_CPU, angle_xyz[0]),
    T1_PROP_F32(ZS_CPU, angle_xyz[1]),
    T1_PROP_F32(ZS_CPU, angle_xyz[2]),
    T1_PROP_F32(ZS_CPU, bloom_on),
    T1_PROP_F32(ZS_CPU, alpha_on),
    T1_PROP_F32(ZS_CPU, visible),

    T1_PROP_F32(ZS_GPU, bonus_rgb[0]),
    T1_PROP_F32(ZS_GPU, bonus_rgb[1]),
    T1_PROP_F32(ZS_GPU, bonus_rgb[2]),
    T1_PROP_F32(ZS_GPU, base_mat_uv_offsets[0]),
    T1_PROP_F32(ZS_GPU, base_mat_uv_offsets[1]),
    T1_PROP_F32(ZS_GPU, alpha),
    T1_PROP_F32(ZS_GPU, no_light),
    T1_PROP_F32(ZS_GPU, no_cam),
    T1_PROP_F32(ZS_GPU, outline_alpha),
    T1_PROP_F32(ZS_GPU, shadow_strength),
    T1_PROP_F32(ZS_GPU, vertex_noise_z),

    T1_PROP_U32("touch_id", ZS_U32, touch_id),
    // the camera (T1_id) that doesn't draw this zsprite, T1_ID_NONE for none
    T1_PROP_U32_CAM_REF(
        "hidden_from_cam", ZS_U32, active_render_view_flags,
        0xFFFFFFFF, 0, T1_PROP_CAM_REF_HIDE_MASK),
    // the camera (T1_id) whose image is mixed in, the shader reads it as an
    // s16 index from the low 16 bits, the texture from the high 16
    T1_PROP_U32_CAM_REF(
        "mix_cam", ZS_U32, mix_rv_and_mix_tex,
        0x0000FFFF, 0, T1_PROP_CAM_REF_INDEX),
    T1_PROP_U32_PACKED(
        "mix_texture", ZS_U32, mix_rv_and_mix_tex, 0xFFFF0000, 16),

    // the zsprite's base material (alpha is left out, use the sprite's)
    T1_PROP_F32(ZS_MAT, ambient_rgb[0]),
    T1_PROP_F32(ZS_MAT, ambient_rgb[1]),
    T1_PROP_F32(ZS_MAT, ambient_rgb[2]),
    T1_PROP_F32(ZS_MAT, diffuse_rgb[0]),
    T1_PROP_F32(ZS_MAT, diffuse_rgb[1]),
    T1_PROP_F32(ZS_MAT, diffuse_rgb[2]),
    T1_PROP_F32(ZS_MAT, specular_rgb[0]),
    T1_PROP_F32(ZS_MAT, specular_rgb[1]),
    T1_PROP_F32(ZS_MAT, specular_rgb[2]),
    T1_PROP_F32(ZS_MAT, specular_exponent),
    // the shader reads tex from the low 16 bits, the normal map from the high
    T1_PROP_U32_PACKED(
        "texture", ZS_MTU, normalmap_tex_and_tex, 0x0000FFFF, 0),

    T1_PROP_F32(TQ_CPU, offset_xyz[0]),
    T1_PROP_F32(TQ_CPU, offset_xyz[1]),
    T1_PROP_F32(TQ_CPU, offset_xyz[2]),
    T1_PROP_F32(TQ_CPU, visible),

    T1_PROP_F32(TQ_GPU, xyz[0]),
    T1_PROP_F32(TQ_GPU, xyz[1]),
    T1_PROP_F32(TQ_GPU, xyz[2]),
    T1_PROP_F32(TQ_GPU, wh[0]),
    T1_PROP_F32(TQ_GPU, wh[1]),
    T1_PROP_F32(TQ_GPU, mul_wh[0]),
    T1_PROP_F32(TQ_GPU, mul_wh[1]),
    T1_PROP_F32(TQ_GPU, rgba[0]),
    T1_PROP_F32(TQ_GPU, rgba[1]),
    T1_PROP_F32(TQ_GPU, rgba[2]),
    T1_PROP_F32(TQ_GPU, rgba[3]),

    // the shader reads tex from the low 16 bits
    T1_PROP_U32_PACKED(
        "texture", TQ_U32, reserved_and_tex, 0x0000FFFF, 0),
    T1_PROP_U32("touch_id", TQ_U32, touch_id),

    T1_PROP_F32(ZL, xyz[0]),
    T1_PROP_F32(ZL, xyz[1]),
    T1_PROP_F32(ZL, xyz[2]),
    T1_PROP_F32(ZL, angle_xyz[0]),
    T1_PROP_F32(ZL, angle_xyz[1]),
    T1_PROP_F32(ZL, angle_xyz[2]),
    T1_PROP_F32(ZL, offset_xyz[0]),
    T1_PROP_F32(ZL, offset_xyz[1]),
    T1_PROP_F32(ZL, offset_xyz[2]),
    T1_PROP_F32(ZL, rgba[0]),
    T1_PROP_F32(ZL, rgba[1]),
    T1_PROP_F32(ZL, rgba[2]),
    T1_PROP_F32(ZL, rgba[3]),
    T1_PROP_F32(ZL, reach),
    T1_PROP_F32(ZL, diffuse),
    T1_PROP_F32(ZL, specular),

    T1_PROP_F32(TEXT, font_height),
    T1_PROP_F32_NAMED("rgba[0]", TEXT, f32s.rgba[0]),
    T1_PROP_F32_NAMED("rgba[1]", TEXT, f32s.rgba[1]),
    T1_PROP_F32_NAMED("rgba[2]", TEXT, f32s.rgba[2]),
    T1_PROP_F32_NAMED("rgba[3]", TEXT, f32s.rgba[3]),
    T1_PROP_U32("touch_id", TEXT, u32s.touch_id),
    // highlights highlight_size letters, starting at letter highlight_i
    T1_PROP_U32("highlight_i", TEXT, highlight_i),
    T1_PROP_U32("highlight_size", TEXT, highlight_size),
    T1_PROP_U32("opaque_back_active", TEXT, opaque_back_active),
    T1_PROP_F32(TEXT, tab_width),

    T1_PROP_F32(POST, rgb_add[0]),
    T1_PROP_F32(POST, rgb_add[1]),
    T1_PROP_F32(POST, rgb_add[2]),
    #if T1_FOG_ACTIVE == T1_ACTIVE
    T1_PROP_F32(POST, fog_color[0]),
    T1_PROP_F32(POST, fog_color[1]),
    T1_PROP_F32(POST, fog_color[2]),
    T1_PROP_F32(POST, fog_factor),
    #elif T1_FOG_ACTIVE == T1_INACTIVE
    #else
    #error
    #endif
    #if T1_SHADOWS_ACTIVE == T1_ACTIVE
    T1_PROP_F32(POST, in_shadow_mults[0]),
    T1_PROP_F32(POST, in_shadow_mults[1]),
    T1_PROP_F32(POST, in_shadow_mults[2]),
    #elif T1_SHADOWS_ACTIVE == T1_INACTIVE
    #else
    #error
    #endif

    // cameras, with the same names as the other objects where they mean the
    // same, so a camera merged into a group moves and turns with it
    T1_PROP_F32(CAM, xyz[0]),
    T1_PROP_F32(CAM, xyz[1]),
    T1_PROP_F32(CAM, xyz[2]),
    T1_PROP_F32(CAM, angle_xyz[0]),
    T1_PROP_F32(CAM, angle_xyz[1]),
    T1_PROP_F32(CAM, angle_xyz[2]),
    T1_PROP_F32(CAM, dest_xyz[0]),
    T1_PROP_F32(CAM, dest_xyz[1]),
    T1_PROP_F32(CAM, dest_xyz[2]),
    T1_PROP_F32(CAM, dest_angle_xyz[0]),
    T1_PROP_F32(CAM, dest_angle_xyz[1]),
    T1_PROP_F32(CAM, dest_angle_xyz[2]),
    T1_PROP_F32(CAM, min_xyz[0]),
    T1_PROP_F32(CAM, min_xyz[1]),
    T1_PROP_F32(CAM, min_xyz[2]),
    T1_PROP_F32(CAM, max_xyz[0]),
    T1_PROP_F32(CAM, max_xyz[1]),
    T1_PROP_F32(CAM, max_xyz[2]),
    T1_PROP_F32(CAM, min_angle_xyz[0]),
    T1_PROP_F32(CAM, min_angle_xyz[1]),
    T1_PROP_F32(CAM, min_angle_xyz[2]),
    T1_PROP_F32(CAM, max_angle_xyz[0]),
    T1_PROP_F32(CAM, max_angle_xyz[1]),
    T1_PROP_F32(CAM, max_angle_xyz[2]),
    T1_PROP_U32("us_to_destination", CAMU, us_to_destination),
    // follows the zsprite with this T1_id, T1_ID_NONE to stop
    T1_PROP_U32("clamped_to_T1_id", CAMU, clamped_to_T1_id),
    T1_PROP_U32("movement_enabled", CAMU, movement_enabled),
};

#define T1_PROPERTIES_SIZE (sizeof(T1_properties) / sizeof(T1Property))

static void T1_property_crash(
    const c8 * problem,
    const c8 * name)
{
    c8 msg[256];
    msg[0] = '\0';
    T1_std_strcat_cap(msg, 256, problem);
    T1_std_strcat_cap(msg, 256, ": \"");
    T1_std_strcat_cap(msg, 256, name);
    T1_std_strcat_cap(msg, 256, "\"");
    T1_log_crash(msg);
}

static b8 T1_property_store_is_settings(const u8 store) {
    return store == T1_PROP_STORE_TEXT || store == T1_PROP_STORE_POSTPROC;
}

u32 T1_property_find(
    const c8 * name,
    const u8 is_u32,
    const u8 settings_store,
    const T1Property ** recip,
    const u32 recip_cap)
{
    u32 found = 0;
    b8 name_exists = 0;

    for (u32 i = 0; i < T1_PROPERTIES_SIZE; i++) {
        if (settings_store == T1_PROP_STORE_OUTOFBOUNDS ?
            T1_property_store_is_settings(T1_properties[i].store) :
            T1_properties[i].store != settings_store)
        {
            continue;
        }

        if (!T1_std_are_equal_strings(T1_properties[i].name, name)) {
            continue;
        }

        name_exists = 1;
        if (T1_properties[i].is_u32 != is_u32) {
            continue;
        }

        T1_log_assert(found < recip_cap);
        if (found < recip_cap) {
            recip[found] = &T1_properties[i];
            found += 1;
        }
    }

    if (!name_exists) {
        T1_property_crash(
            settings_store == T1_PROP_STORE_TEXT ?
                "Unknown T1 text property" :
            settings_store == T1_PROP_STORE_POSTPROC ?
                "Unknown T1 post-processing property" :
                "Unknown T1 property",
            name);
    } else if (found == 0) {
        T1_property_crash(
            is_u32 ?
                "T1 property is not a u32" :
                "T1 property is not an f32",
            name);
    }

    return found;
}

void T1_property_write(
    void * store_struct,
    const T1Property * p,
    const u32 bits)
{
    u32 * target = (u32 *)((u8 *)store_struct + p->offset);
    *target = (*target & ~p->mask) | ((bits << p->shift) & p->mask);
}

u32 T1_property_read(
    const void * store_struct,
    const T1Property * p)
{
    const u32 * source = (const u32 *)((const u8 *)store_struct + p->offset);
    return (*source & p->mask) >> p->shift;
}

b8 T1_property_is_packed(const T1Property * p) {
    return p->mask != 0xFFFFFFFF;
}

b8 T1_property_is_animatable(const T1Property * p) {
    return
        p->store != T1_PROP_STORE_ZS_MAT_F32 &&
        p->store != T1_PROP_STORE_ZS_MAT_U32 &&
        p->store != T1_PROP_STORE_TQ_CPU_F32 &&
        p->store != T1_PROP_STORE_CAM_U32;
}

u32 T1_property_convert_bits(
    const T1Property * p,
    const u32 bits)
{
    if (p->cam_ref == T1_PROP_CAM_REF_NONE) { return bits; }
    
    s32 cam_i = T1_render_view_T1_id_to_i(bits);
    // a T1_id that isn't T1_ID_NONE should be a camera
    T1_log_assert(cam_i >= 0 || bits == T1_ID_NONE);
    
    switch (p->cam_ref) {
        case T1_PROP_CAM_REF_INDEX:
            return (u32)cam_i;
        case T1_PROP_CAM_REF_HIDE_MASK:
            return cam_i < 0 ? 0xFFFFFFFF : ~(1u << cam_i);
        default:
            T1_log_assert(0);
            return bits;
    }
}

static u32 T1_property_set_bits(
    const u32 T1_id,
    const T1Property * p,
    const u32 bits)
{
    switch (p->store) {
        case T1_PROP_STORE_ZS_CPU_F32:
        case T1_PROP_STORE_ZS_GPU_F32:
        case T1_PROP_STORE_ZS_GPU_U32:
        case T1_PROP_STORE_ZS_MAT_F32:
        case T1_PROP_STORE_ZS_MAT_U32:
            return T1_zsprite_set_property(T1_id, p, bits);
        case T1_PROP_STORE_TQ_CPU_F32:
        case T1_PROP_STORE_TQ_GPU_F32:
        case T1_PROP_STORE_TQ_GPU_U32:
            return T1_texquad_set_property(T1_id, p, bits);
        case T1_PROP_STORE_ZL_F32:
            return T1_zlight_set_property(T1_id, p, bits);
        case T1_PROP_STORE_CAM_F32:
        case T1_PROP_STORE_CAM_U32:
            return T1_render_view_set_property(T1_id, p, bits);
        default:
            T1_log_assert(0);
    }

    return 0;
}

static void T1_set_bits(
    const u32 T1_id,
    const c8 * name,
    const u8 is_u32,
    const u32 bits)
{
    T1_log_assert(T1_id != T1_ID_NONE);

    const T1Property * matches[T1_PROPERTY_MATCHES_CAP];
    u32 matches_size = T1_property_find(
        name, is_u32, T1_PROP_STORE_OUTOFBOUNDS,
        matches, T1_PROPERTY_MATCHES_CAP);

    u32 objects_set = 0;
    for (u32 i = 0; i < matches_size; i++) {
        objects_set += T1_property_set_bits(
            T1_id,
            matches[i],
            T1_property_convert_bits(matches[i], bits));
    }

    // no object with this T1_id has this property
    T1_log_assert(objects_set > 0);
}

void T1_set_f32(
    u32 T1_id,
    const c8 * property,
    f32 value)
{
    u32 bits;
    T1_std_memcpy(&bits, &value, 4);
    T1_set_bits(T1_id, property, 0, bits);
}

void T1_set_u32(
    u32 T1_id,
    const c8 * property,
    u32 value)
{
    T1_set_bits(T1_id, property, 1, value);
}

static void T1_text_set_bits(
    const c8 * name,
    const u8 is_u32,
    const u32 bits)
{
    const T1Property * matches[T1_PROPERTY_MATCHES_CAP];
    u32 matches_size = T1_property_find(
        name, is_u32, T1_PROP_STORE_TEXT,
        matches, T1_PROPERTY_MATCHES_CAP);
    T1_log_assert(matches_size == 1);

    T1_property_write(T1_text_props, matches[0], bits);
}

void T1_postproc_set_f32(
    const c8 * property,
    f32 value)
{
    const T1Property * matches[T1_PROPERTY_MATCHES_CAP];
    u32 matches_size = T1_property_find(
        property, 0, T1_PROP_STORE_POSTPROC,
        matches, T1_PROPERTY_MATCHES_CAP);
    T1_log_assert(matches_size == 1);

    u32 bits;
    T1_std_memcpy(&bits, &value, 4);
    T1_property_write(&T1_global->postproc_consts, matches[0], bits);
}

f32 T1_postproc_get_f32(
    const c8 * property)
{
    const T1Property * matches[T1_PROPERTY_MATCHES_CAP];
    u32 matches_size = T1_property_find(
        property, 0, T1_PROP_STORE_POSTPROC,
        matches, T1_PROPERTY_MATCHES_CAP);
    T1_log_assert(matches_size == 1);

    u32 bits = T1_property_read(&T1_global->postproc_consts, matches[0]);
    f32 value;
    T1_std_memcpy(&value, &bits, 4);
    return value;
}

void T1_text_set_f32(
    const c8 * property,
    f32 value)
{
    u32 bits;
    T1_std_memcpy(&bits, &value, 4);
    T1_text_set_bits(property, 0, bits);
}

void T1_text_set_u32(
    const c8 * property,
    u32 value)
{
    T1_text_set_bits(property, 1, value);
}

void T1_merge_T1_ids(
    u32 T1_id_to_add,
    u32 T1_id_to_keep)
{
    T1_log_assert(T1_id_to_add  != T1_ID_NONE);
    T1_log_assert(T1_id_to_keep != T1_ID_NONE);

    if (T1_id_to_add == T1_id_to_keep) {
        return;
    }

    T1_zsprite_rename_T1_id(T1_id_to_add, T1_id_to_keep);
    T1_texquad_rename_T1_id(T1_id_to_add, T1_id_to_keep);
    T1_zlight_rename_T1_id(T1_id_to_add, T1_id_to_keep);
    T1_render_view_rename_T1_id(T1_id_to_add, T1_id_to_keep);

    #if T1_ANIM_ACTIVE == T1_ACTIVE
    T1_anim_rename_target_T1_id(T1_id_to_add, T1_id_to_keep);
    #elif T1_ANIM_ACTIVE == T1_INACTIVE
    #else
    #error
    #endif
}

void T1_delete(u32 T1_id)
{
    T1_log_assert(T1_id != T1_ID_NONE);

    T1_zsprite_delete(T1_id);
    T1_texquad_delete(T1_id);
    T1_zlight_delete(T1_id);
    T1_render_view_delete_T1_id(T1_id);
}

void T1_delete_all(void)
{
    T1_zsprite_delete_all();
    T1_texquad_delete_all();
    T1_zlight_delete_all();
}
