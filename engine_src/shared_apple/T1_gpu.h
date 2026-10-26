#ifndef T1_GPU_H
#define T1_GPU_H


#include <mach/mach_time.h>

extern u64 last_resize_request_at;

void T1_apple_gpu_init(
    void * with_metal_device,
    f32 backing_scale_factor,
    char ** const sticky_error);

// The id<MTLDevice> T1_gpu_init() created, or NULL before that
void * T1_gpu_get_device(void);

void T1_gpu_update_render_view_size(s32 at_i);
void T1_gpu_update_final_window_size(void);
/*
Copies the last rendered frame to the next drawable of the given
CAMetalLayer and presents it. Blocks while all of the layer's drawables are
in use. Returns false if nothing was presented.
*/
b8 T1_gpu_present_final_texture(void * metal_layer);


#if 0

#endif
extern u64 previous_time;

#endif // T1_GPU_H
