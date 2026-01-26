#ifndef T1_GLOBAL_H
#define T1_GLOBAL_H

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

#define T1_WINDOW_RESIZE_TIMEOUT 2500000
#define T1_CLEARDEPTH 1.0f

#include <math.h>

#include "T1_std.h"
#include "T1_cpu_gpu_shared_types.h"
#include "T1_logger.h"
#include "T1_mesh_summary.h"

typedef struct {
    T1GPUPostProcConsts postproc_consts;
    
    uint64_t elapsed;
    uint64_t this_frame_timestamp_us;
    uint64_t last_resize_request_us;
    
    uint32_t startup_bytes_to_load;
    uint32_t startup_bytes_loaded;
    
    float last_clickray_origin[3];
    float last_clickray_direction[3];
    float last_clickray_collision[3];
    
    uint32_t pixelation_div;
    
    float timedelta_mult;
    float window_height;
    float window_width;
    float window_left;
    float window_bottom;
    
    bool8_t draw_mouseptr;
    bool8_t draw_imputed_normals;
    bool8_t draw_triangles;
    bool8_t draw_axes;
    bool8_t draw_fps;
    bool8_t draw_top_touchable_id;
    bool8_t show_profiler;
    bool8_t pause_profiler;
    bool8_t block_mouse;
    bool8_t block_render_view_pos_updates;
    bool8_t fullscreen;
    bool8_t clientlogic_early_startup_finished;
    bool8_t upcoming_fullscreen_request;
} T1Globals;

extern T1Globals * T1_global;

void T1_global_register_transformed_imputed_normal_for_debugging(
    const float origin[3],
    const float imputed_normal[3]);

void T1_global_init(void);

void T1_global_update_window_pos(
    float left,
    float bottom);

void T1_global_update_window_size(
    float width,
    float height,
    uint64_t at_timestamp_us);

float T1_global_get_z_mul_for_depth(
    const int32_t for_mesh_id,
    const float for_depth);

float T1_global_get_y_mul_for_height(
    const int32_t for_mesh_id,
    const float for_height);

float T1_global_get_x_mul_for_width(
    const int32_t for_mesh_id,
    const float for_width);

#endif // T1_GLOBAL_H
