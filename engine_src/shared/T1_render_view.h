#ifndef T1_RENDER_VIEW_H
#define T1_RENDER_VIEW_H

#include "T1_std.h"
#include "T1_mem.h"
#include "T1_global.h"
#include "T1_cpu_gpu_shared_types.h"
#include "T1_logger.h"

typedef struct {
    float aspect_ratio;
    float znear;
    float zfar;
    float field_of_view_rad;
    float field_of_view_modifier;
    float x_multiplier;
    float y_multiplier;
} T1GPUProjectConsts;

typedef struct {
    T1GPUProjectConsts project;
    float    xyz[3];           // 12 bytes
    float    xyz_angle[3];     // 12 bytes
    int32_t  write_array_i;
    int32_t  write_slice_i;
    uint32_t width;
    uint32_t height;
    T1RenderViewWriteType write_type;
    uint8_t  draw_outlines;
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

void T1_render_view_delete(const int32_t rv_i);

void T1_render_view_validate(void);

float T1_render_view_x_to_screen_x(
    const float x,
    const float given_z);

// To convert from our screenspace system to 'world x' that is used for
// the position of zpolygons
float T1_render_view_screen_x_to_x(const float screenspace_x, const float given_z);

float T1_render_view_y_to_screen_y(
    const float y,
    const float given_z);

// To convert from our screenspace system to 'world y' that is used for
// the position of zpolygons
float T1_render_view_screen_y_to_y(
    const float screenspace_y,
    const float given_z);

float T1_render_view_screen_height_to_height(
    const float screenspace_height, const float given_z);
float T1_render_view_screen_width_to_width(
    const float screenspace_width, const float given_z);

#endif // T1_RENDER_VIEW_H
