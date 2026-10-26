#ifndef T1_RENDER_VIEW_H
#define T1_RENDER_VIEW_H

#include <stdint.h>

#include "T1_types_gpucpu.h"
#include "T1_property.h"

#define T1_CAM_FOV_DEGREES 75.0f // vertical

typedef struct {
    f32 aspect_ratio;
    f32 znear;
    f32 zfar;
    f32 field_of_view_rad;
    f32 field_of_view_modifier;
    f32 x_multiplier;
    f32 y_multiplier;
} T1GPUProjectConsts;

/*
A camera's values that can be animated. It eases from xyz/angle_xyz towards
dest_xyz/dest_angle_xyz (see us_to_destination), kept within the min/max.
*/
typedef struct {
    f32 xyz[3];
    f32 angle_xyz[3];
    f32 dest_xyz[3];
    f32 dest_angle_xyz[3];
    f32 min_xyz[3];
    f32 max_xyz[3];
    f32 min_angle_xyz[3];
    f32 max_angle_xyz[3];
    f32 reflect_plane_z;
} T1CamF32s;

typedef struct {
    T1GPUProjectConsts project;
    T1CamF32s f32s;
    u32 us_to_destination;
    u32 T1_id;
    // the camera this one mirrors around f32s.reflect_plane_z, T1_ID_NONE
    // for none
    u32 reflect_cam;
    u32 clamped_to_T1_id;
    u32 movement_enabled;
    u32 visible_layers; // copied to the gpu every frame
    u32 width;
    u32 height;
    u8  deleted;
} T1CPURenderView;

typedef struct {
    T1GPURenderView gpu[T1_RENDER_VIEW_CAP];
    T1CPURenderView cpu[T1_RENDER_VIEW_CAP];
    u32 size;
} T1RenderViewCollection;

extern T1RenderViewCollection * T1_render_views;

void T1_render_view_init(void);

void T1_render_view_reset(s32 i);

s32 T1_render_view_fetch_next(
    const u32 width,
    const u32 height);

void T1_render_view_update_positions(
    const u64 elapsed);

void T1_render_view_delete(const s32 rv_i);

/*
Cameras are T1 objects: every render view has a unique T1_id, set and
animated by property name like zsprites (see T1_property.h)
*/
// the index of the render view with this T1_id, or -1
s32  T1_render_view_T1_id_to_i(const u32 T1_id);
void T1_render_view_delete_T1_id(const u32 T1_id);
u32  T1_render_view_set_property(
    const u32 T1_id,
    const T1Property * property,
    const u32 bits);
void T1_render_view_rename_T1_id(const u32 from_T1_id, const u32 to_T1_id);
// applies the step to the store of the camera with T1_id
void T1_render_view_apply_anim_step(
    const u32 T1_id,
    const u8 store,
    const T1AnimStep * step);

void T1_render_view_validate(void);

/*
To convert from pixels on a viewport_wh[0] * viewport_wh[1] screen to the
world units at given_z that zsprites are positioned in
*/
f32 T1_render_view_screen_x_to_x(
    const f32 viewport_wh[2], const f32 screenspace_x, const f32 given_z);
f32 T1_render_view_screen_y_to_y(
    const f32 viewport_wh[2], const f32 screenspace_y, const f32 given_z);
f32 T1_render_view_screen_height_to_height(
    const f32 viewport_wh[2], const f32 screenspace_height, const f32 given_z);
f32 T1_render_view_screen_width_to_width(
    const f32 viewport_wh[2], const f32 screenspace_width, const f32 given_z);
// a camera's width and height (1, 1 if there's no such camera)
u8 T1_render_view_get_wh(const u32 T1_id, f32 viewport_wh[2]);

#endif // T1_RENDER_VIEW_H
