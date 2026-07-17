#ifndef T1_RENDER_VIEW_H
#define T1_RENDER_VIEW_H

#include <stdint.h>

#include "T1_types_gpucpu.h"

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

#define T1_RENDER_PASSES_MAX 6
typedef struct {
    T1GPUProjectConsts project;
    T1RenderPass passes[T1_RENDER_PASSES_MAX];
    u64 us_to_destination;
    f32 xyz[3];
    f32 angle_xyz[3];
    f32 dest_xyz[3];
    f32 dest_angle_xyz[3];
    f32 min_xyz[3];
    f32 max_xyz[3];
    f32 min_angle_xyz[3];
    f32 max_angle_xyz[3];
    #if T1_REFLECTION_ACTIVE == T1_ACTIVE
    f32 refl_cam_around_plane_z;
    #elif T1_REFLECTION_ACTIVE == T1_INACTIVE
    #else
    #error
    #endif
    T1RenderViewWriteType write_type;
    s32 clamped_to_T1_id;
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
    u8  movement_enabled;
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

void T1_render_view_validate(void);

f32 T1_render_view_x_to_screen_x(
    const f32 x,
    const f32 given_z);
f32 T1_render_view_x_to_screen_x_noz(
    const f32 x);

// To convert from our screenspace system to 'world x' that is used for
// the position of zpolygons
f32 T1_render_view_screen_x_to_x(
    const f32 screenspace_x,
    const f32 given_z);
f32 T1_render_view_y_to_screen_y(
    const f32 y,
    const f32 given_z);

f32 T1_render_view_y_to_screen_y_noz(
    const f32 y);

// To convert from our screenspace system to 'world y' that is used for
// the position of zpolygons
f32 T1_render_view_screen_y_to_y(
    const f32 screenspace_y,
    const f32 given_z);

f32 T1_render_view_screen_x_to_x_noz(
    const f32 screenspace_width);
f32 T1_render_view_screen_y_to_y_noz(
    const f32 screenspace_height);
f32 T1_render_view_screen_width_to_width_noz(
    const f32 screenspace_width);
f32 T1_render_view_screen_height_to_height_noz(
    const f32 screenspace_height);

f32 T1_render_view_screen_height_to_height(
    const f32 screenspace_height, const f32 given_z);
f32 T1_render_view_screen_width_to_width(
    const f32 screenspace_width, const f32 given_z);

#endif // T1_RENDER_VIEW_H
