#ifndef T1_RENDER_VIEW_H
#define T1_RENDER_VIEW_H

#include "T1_std.h"
#include "T1_mem.h"
#include "T1_linalg3d.h"
#include "T1_global.h"
#include "T1_cpu_gpu_shared_types.h"
#include "T1_log.h"

typedef struct {
    float aspect_ratio;
    float znear;
    float zfar;
    float field_of_view_rad;
    float field_of_view_modifier;
    float x_multiplier;
    float y_multiplier;
} T1GPUProjectConsts;

typedef enum : uint8_t {
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
    int32_t vert_i;
    int32_t verts_size;
} T1RenderPass;

#define T1_RENDER_PASSES_MAX 6
typedef struct {
    T1GPUProjectConsts project;
    T1RenderPass passes[T1_RENDER_PASSES_MAX];
    float    xyz[3];           // 12 bytes
    float    xyz_angle[3];     // 12 bytes
    float    refl_cam_around_plane_xyz[3];
    T1RenderViewWriteType write_type;
    int32_t  write_array_i;
    int32_t  write_slice_i;
    uint32_t width;
    uint32_t height;
    uint8_t  passes_size;
    uint8_t  reflect_around_plane;
    uint8_t  deleted;
} T1CPURenderView;

typedef struct {
    T1GPURenderView gpu[T1_RENDER_VIEW_CAP];
    T1CPURenderView cpu[T1_RENDER_VIEW_CAP];
    uint32_t size;
} T1RenderViewCollection;

extern T1RenderViewCollection * T1_render_views;
extern T1CPURenderView * T1_camera; // convenience

void T1_render_view_init(void);

int32_t T1_render_view_fetch_next(
    const uint32_t width,
    const uint32_t height);

void T1_render_view_update_positions(
    void);

void T1_render_view_delete(const int32_t rv_i);

void T1_render_view_validate(void);

float T1_render_view_x_to_screen_x(
    const float x,
    const float given_z);
float T1_render_view_x_to_screen_x_noz(
    const float x);

// To convert from our screenspace system to 'world x' that is used for
// the position of zpolygons
float T1_render_view_screen_x_to_x(const float screenspace_x, const float given_z);

float T1_render_view_y_to_screen_y(
    const float y,
    const float given_z);

float T1_render_view_y_to_screen_y_noz(
    const float y);

// To convert from our screenspace system to 'world y' that is used for
// the position of zpolygons
float T1_render_view_screen_y_to_y(
    const float screenspace_y,
    const float given_z);

float T1_render_view_screen_x_to_x_noz(
    const float screenspace_width);
float T1_render_view_screen_y_to_y_noz(
    const float screenspace_height);
float T1_render_view_screen_width_to_width_noz(
    const float screenspace_width);
float T1_render_view_screen_height_to_height_noz(
    const float screenspace_height);

float T1_render_view_screen_height_to_height(
    const float screenspace_height, const float given_z);
float T1_render_view_screen_width_to_width(
    const float screenspace_width, const float given_z);

#endif // T1_RENDER_VIEW_H
