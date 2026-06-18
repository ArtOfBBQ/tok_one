#ifndef T1_GLOBAL_H
#define T1_GLOBAL_H

#include <math.h>

#include "T1_std.h"
#include "T1_cpu_gpu_shared_types.h"

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

#define T1_GLOBAL_WINDOW_RESIZE_TIMEOUT 2500000
#define T1_GLOBAL_CLEARDEPTH 1.0f

extern T1Globals * T1_global;

void
T1_global_init(void);

void
T1_global_update_window_pos(
    float left,
    float bottom);

void
T1_global_update_window_size(
    float width,
    float height,
    uint64_t at_timestamp_us);

float
T1_global_get_x_mul_for_width(
    const int32_t for_mesh_id,
    const float for_width);

float
T1_global_get_y_mul_for_height(
    const int32_t for_mesh_id,
    const float for_height);

float
T1_global_get_z_mul_for_depth(
    const int32_t for_mesh_id,
    const float for_depth);

#endif // T1_GLOBAL_H
