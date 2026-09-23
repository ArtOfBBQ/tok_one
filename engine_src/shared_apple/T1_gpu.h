#ifndef T1_GPU_H
#define T1_GPU_H

#include "T1_types_cpu_to_gpu.h"

// #import <MetalKit/MetalKit.h>
#import <mach/mach_time.h>

extern u64 last_resize_request_at;

u8 T1_apple_gpu_init(
    void (* arg_funcptr_shared_gameloop_update)
        (T1GPUFrame *),
    void (* arg_funcptr_gameloop_update_after_render)(void),
    void * with_metal_device,
    char * shader_lib_filepath,
    f32 backing_scale_factor,
    char * error_msg_string);

void T1_gpu_update_render_view_size(s32 at_i);
void T1_gpu_update_final_window_size(void);
void T1_gpu_draw_in_mtk_view(void * view);

// returns slice_i of new depth texture
s16 T1_apple_gpu_make_depth_tex(
    u32 width,
    u32 height);

#if 0

#endif
extern u64 previous_time;

#endif // T1_GPU_H
