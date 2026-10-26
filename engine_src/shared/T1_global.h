#ifndef T1_GLOBAL_H
#define T1_GLOBAL_H

#include "T1_std.h"
#include "T1_types_gpucpu.h"
#include "T1_types_public.h"

/*
Window positions and sizes are in T1's screen space, which is macOS's: (0, 0)
is the bottom left and (width, height) the top right. Other platform layers
convert to it.
*/

typedef struct {
    T1GPUFrameConsts frame_consts;
    
    uint64_t elapsed;
    uint64_t this_frame_timestamp_us;
    uint64_t last_resize_request_us;
    
    
    float timedelta_mult;
    float window_wh[2];
    float window_left;
    float window_bottom;
    
    uint8_t draw_mouseptr;
    uint8_t block_mouse;
    uint8_t fullscreen;
    uint8_t upcoming_fullscreen_request;
} T1Globals;

#define T1_GLOBAL_WINDOW_RESIZE_TIMEOUT 2500000
#define T1_GLOBAL_CLEARDEPTH 1.0f

extern T1Globals * T1_global;

void T1_global_init(void);

void T1_global_update_window_pos(
    f32 left,
    f32 bottom);

void T1_global_update_window_size(
    f32 width,
    f32 height,
    u64 at_timestamp_us);

#endif // T1_GLOBAL_H
