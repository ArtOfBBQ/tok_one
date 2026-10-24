#ifndef T1_GPU_H
#define T1_GPU_H

#include "T1_types_cpu_to_gpu.h"

#include <mach/mach_time.h>

extern u64 last_resize_request_at;

void T1_apple_gpu_init(
    void (* arg_funcptr_shared_gameloop_update)
        (T1GPUFrame *),
    void (* arg_funcptr_gameloop_update_after_render)(void),
    void * with_metal_device,
    f32 backing_scale_factor,
    char ** const sticky_error);

void T1_gpu_update_render_view_size(s32 at_i);
void T1_gpu_update_final_window_size(void);
/*
Runs the game loop callbacks, renders a frame and presents it on the given
CAMetalLayer. Returns false if nothing was presented (and so nothing paced
the caller to the display's refresh rate).
*/
b8 T1_gpu_draw_frame(void * metal_layer);

// returns slice_i of new depth texture
s16 T1_apple_gpu_make_depth_tex(
    u32 width,
    u32 height);

#if 0

#endif
extern u64 previous_time;

#endif // T1_GPU_H
