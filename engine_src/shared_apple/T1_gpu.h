#ifndef T1_GPU_H
#define T1_GPU_H

#include "T1_stdint.h"

#include <mach/mach_time.h>

extern u64 last_resize_request_at;

void T1_apple_gpu_init(
    void * with_metal_device,
    f32 backing_scale_factor,
    char ** const sticky_error);

// The id<MTLDevice> T1_gpu_init() created, or NULL before that
void * T1_gpu_get_device(void);

void T1_gpu_update_render_view_size(s32 at_i);
/*
Draws the T1Tex tex into the next drawable of the given CAMetalLayer, at
tex's size, and presents it. Blocks while all of the layer's drawables are
in use. Returns false if nothing was presented.
*/
b8 T1_gpu_present_tex(void * metal_layer, const u32 tex);


#if 0

#endif
extern u64 previous_time;

#endif // T1_GPU_H
