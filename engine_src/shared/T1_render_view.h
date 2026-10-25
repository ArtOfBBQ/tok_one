#ifndef T1_RENDER_VIEW_H
#define T1_RENDER_VIEW_H

#include <stdint.h>

#include "T1_types_gpucpu.h"
#include "T1_property.h"

typedef struct {
    f32 aspect_ratio;
    f32 znear;
    f32 zfar;
    f32 field_of_view_rad;
    f32 field_of_view_modifier;
    f32 x_multiplier;
    f32 y_multiplier;
} T1GPUProjectConsts;

typedef enum : u8 {
    T1RENDERPASS_BELOWBOUNDS = 0,
    T1RENDERPASS_DEPTH_PREPASS = 1,
    T1RENDERPASS_OUTLINES = 2,
    T1RENDERPASS_DIAMOND_ALPHA = 3,
    T1RENDERPASS_ALPHA_BLEND = 4,
    T1RENDERPASS_BLOOM = 5,
    T1RENDERPASS_BILLBOARDS = 6,
    T1RENDERPASS_FLAT_TEXQUADS = 7,
    T1RENDERPASS_ABOVEBOUNDS = 8,
} T1RenderPassType;

// TODO: discover what architecture we need/want
typedef struct {
    // Stays the same over render view lifetime
    T1RenderPassType type;
    
    // Set every frame
    s32 vert_i;
    s32 verts_size;
} T1RenderPass;

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
} T1CamF32s;

#define T1_RENDER_PASSES_MAX 6
typedef struct {
    T1GPUProjectConsts project;
    T1RenderPass passes[T1_RENDER_PASSES_MAX];
    T1CamF32s f32s;
    u32 us_to_destination;
    u32 T1_id;
    #if T1_REFLECTION_ACTIVE == T1_ACTIVE
    f32 refl_cam_around_plane_z;
    #elif T1_REFLECTION_ACTIVE == T1_INACTIVE
    #else
    #error
    #endif
    T1RenderViewWriteType write_type;
    u32 clamped_to_T1_id;
    u32 movement_enabled;
    u32 visible_layers; // copied to the gpu every frame
    u32 width;
    u32 height;
    T1Tex write_tex;
    u8  passes_size;
    #if T1_REFLECTION_ACTIVE == T1_ACTIVE
    u8  reflect_around_plane_z;
    #elif T1_REFLECTION_ACTIVE == T1_INACTIVE
    #else
    #error
    #endif
    u8  deleted;
} T1CPURenderView;

typedef struct {
    T1GPURenderView gpu[T1_RENDER_VIEW_CAP];
    T1CPURenderView cpu[T1_RENDER_VIEW_CAP];
    u32 size;
} T1RenderViewCollection;

extern T1RenderViewCollection * T1_render_views;
extern T1CPURenderView * T1_cam; // convenience

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
// also frees the render view's write texture
void T1_render_view_delete_with_tex(const s32 rv_i);
void T1_render_view_delete_T1_id(const u32 T1_id);
u32  T1_render_view_set_property(
    const u32 T1_id,
    const T1Property * property,
    const u32 bits);
void T1_render_view_rename_T1_id(const u32 from_T1_id, const u32 to_T1_id);
void T1_render_view_apply_endpoint_anim(
    const u32 T1_id,
    const f32 t_applied,
    const f32 t_now,
    const f32 * end_vals_f32);
void T1_render_view_apply_delta_anim(
    const u32 T1_id,
    const f32 t_applied,
    const f32 t_now,
    const f32 * delta_vals_f32);

void T1_render_view_validate(void);

// To convert from our screenspace system to 'world x' that is used for
// the position of zpolygons
f32 T1_render_view_screen_x_to_x(
    const f32 screenspace_x,
    const f32 given_z);

// To convert from our screenspace system to 'world y' that is used for
// the position of zpolygons
f32 T1_render_view_screen_y_to_y(
    const f32 screenspace_y,
    const f32 given_z);

f32 T1_render_view_screen_height_to_height(
    const f32 screenspace_height, const f32 given_z);
f32 T1_render_view_screen_width_to_width(
    const f32 screenspace_width, const f32 given_z);

#endif // T1_RENDER_VIEW_H
