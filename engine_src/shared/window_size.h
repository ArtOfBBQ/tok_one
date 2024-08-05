#ifndef WINDOW_SIZE_H
#define WINDOW_SIZE_H

/*
Window sizes are a source of confusion because there are so many coordinate
systems.

The coordinate system for window positions is generally referred to as
'screen space' coordinates. However, even screen space coordinates are different
depending on the operating system you're using. Here are some things we should
keep separated:
- Mac Os X screenspace coordinates
- Windows screenspace coordinates
- Linux screenspace coordinates
- OpenGL coordinate systems (before and after projections & transformations)
- Metal coordinate systems (before and after projections & transformations)

Our approach will be to make our own screenspace coordinate system here and
force the platform layers to adjust to it.
TOP LEFT (x,y is 0, height)                    TOP RIGHT (x, y is width, height)



                              [SCREEN]


BOTTOM LEFT (x,y is 0, 0)                       BOTTOM RIGHT (x, y is width, 0)

This is the same screenspace coordinate system as used on Mac OS, so the Mac
platform layer doesn't need to do anything and can just record values directly
*/

#define CLEARDEPTH 1.0f

#include "common.h"
#include "cpu_gpu_shared_types.h"

typedef struct WindowGlobals {
    bool32_t draw_clickray;
    bool32_t draw_triangles;
    bool32_t draw_hitboxes;
    bool32_t draw_vertices;
    bool32_t draw_axes;
    bool32_t draw_fps;
    bool32_t fullscreen;
    
    float last_clickray_origin[3];
    float last_clickray_direction[3];
    float last_clickray_collision[3];
    
    float window_height;
    float window_width;
    float window_left;
    float window_bottom;
    float aspect_ratio;
    
    uint64_t last_resize_request_at;
    GPUProjectionConstants projection_constants;
} WindowGlobals;

extern WindowGlobals * window_globals;

// To convert from our screenspace system to 'world x' that is used for
// the position of zpolygons
float screenspace_x_to_x(const float screenspace_x, const float given_z);

// To convert from our screenspace system to 'world y' that is used for
// the position of zpolygons
float screenspace_y_to_y(const float screenspace_y, const float given_z);

float screenspace_height_to_height(
    const float screenspace_height, const float given_z);
float screenspace_width_to_width(
    const float screenspace_width, const float given_z);

void init_projection_constants(void);

void update_window_position(
    float left,
    float bottom);
void update_window_size(
    float width,
    float height,
    uint64_t at_timestamp_microseconds);

#endif
