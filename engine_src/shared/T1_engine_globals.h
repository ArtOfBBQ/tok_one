#ifndef ENGINE_GLOBALS_H
#define ENGINE_GLOBALS_H

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

#include <math.h>

#include "T1_std.h"
#include "T1_cpu_gpu_shared_types.h"

typedef struct EngineGlobals {
    bool32_t draw_mouseptr;
    bool32_t draw_imputed_normals;
    bool32_t draw_triangles;
    bool32_t draw_axes;
    bool32_t draw_fps;
    bool32_t draw_top_touchable_id;
    bool32_t show_profiler;
    bool32_t pause_profiler;
    bool32_t block_mouse;
    bool32_t fullscreen;
    
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
    float aspect_ratio;
    
    uint64_t elapsed;
    uint64_t this_frame_timestamp_us;
    uint64_t last_resize_request_us;
    GPUPostProcConsts postproc_consts;
    GPUProjectConsts project_consts;
} EngineGlobals;

extern EngineGlobals * engine_globals;

void engineglobals_register_transformed_imputed_normal_for_debugging(
    const float origin[3],
    const float imputed_normal[3]);

// To convert from our screenspace system to 'world x' that is used for
// the position of zpolygons
float engineglobals_screenspace_x_to_x(const float screenspace_x, const float given_z);

// To convert from our screenspace system to 'world y' that is used for
// the position of zpolygons
float engineglobals_screenspace_y_to_y(const float screenspace_y, const float given_z);

float engineglobals_screenspace_height_to_height(
    const float screenspace_height, const float given_z);
float engineglobals_screenspace_width_to_width(
    const float screenspace_width, const float given_z);

void engineglobals_init(void);

void engineglobals_update_window_position(
    float left,
    float bottom);

void engineglobals_update_window_size(
    float width,
    float height,
    uint64_t at_timestamp_us);

#endif // ENGINE_GLOBALS_H
