#ifndef T1_ZLIGHT_H
#define T1_ZLIGHT_H

#include <math.h>

#include "T1_types_public.h"
#include "T1_types_gpucpu.h"
#include "T1_std.h"
#include "T1_global.h"
#include "T1_triangle.h"
#include "T1_property.h"

typedef struct {
    float xyz[3];
    float angle_xyz[3];
    float offset_xyz[3];
    float rgba[4];
    float reach; // light's reach
    float diffuse;     // how much diffuse light does this radiate?
    float specular;
} T1zLightf32;

typedef struct {
    // lights the render views whose visible_layers share a bit with it
    uint32_t layers;
    uint32_t shadow_cam; // T1_id
    uint32_t shadow_tex; // T1Tex
} T1zLightu32;

typedef struct {
    T1zLightf32 f32s;
    // you can make a group of lights and/or texquads by
    // giving them the same positive object_id, then make
    // ScheduledAnimations that affect the entire group
    // set to -1 to not be a party of any group
    uint32_t T1_id;
    uint32_t deleted;
    uint32_t committed;
    T1zLightu32 u32s;
    float simd_padding[2];
} T1zLight;


// A buffer of zLightSources to light up your scene(s)
// index 0 to zlights_to_apply_size will be rendered,
// the rest of the array will be ignored
extern T1zLight * T1_zlights;
extern u32 T1_zlights_size;

T1zLight * T1_zlight_next(void);

void T1_zlight_commit(T1zLight * to_request);

u32 T1_zlight_new(void);

// returns how many zlights with this T1_id were written
u32 T1_zlight_set_property(
    u32 T1_id,
    const T1Property * property,
    u32 bits);

void T1_zlight_rename_T1_id(u32 from_T1_id, u32 to_T1_id);

void T1_zlight_clean_all_deleted(void);

void T1_zlight_project_f324_to_2d_inplace(
    f32 * position_x,
    f32 * position_y,
    f32 * position_z);

// just copy the lights without translation, for hardware renderer
void T1_zlight_copy_all(
    T1GPULight * lights,
    u32 * lights_size);

// move each light around the camera (e.g. when the camera moves
// right, we move all lights etc. to the left instead)
// reminder: this is calculated once before 2d and 3d renderer
// and then used in both
void T1_zlight_translate_all(
    T1GPULight * lights,
    u32 * lights_size);

void T1_zlight_delete(u32 with_T1_id);

void T1_zlight_delete_all(void);

void T1_zlight_point_light_to_location(
    f32 * recipient_xyz_angle,
    const f32 * from_pos_xyz,
    const f32 * point_to_xyz);

void T1_zlight_update_all_attached_render_views(void);

// applies the step to the store of every zlight with T1_id
void T1_zlight_apply_anim_step(
    const u32 T1_id,
    const u8 store,
    const T1AnimStep * step);

#endif // T1_ZLIGHT_H
