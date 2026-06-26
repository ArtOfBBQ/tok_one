#ifndef T1_GPU_H
#define T1_GPU_H

#include "T1_cpu_to_gpu.h"

#import <Metal/Metal.h>
#import <MetalKit/MetalKit.h>
#import <mach/mach_time.h>

extern u64 last_resize_request_at;

u8 T1_apple_gpu_init(
    void (* arg_funcptr_shared_gameloop_update)
        (T1GPUFrame *),
    void (* arg_funcptr_gameloop_update_after_render)(void),
    id<MTLDevice> with_metal_device,
    NSString * shader_lib_filepath,
    float backing_scale_factor,
    char * error_msg_string);

// returns slice_i of new depth texture
s16 T1_apple_gpu_make_depth_tex(
    u32 width,
    u32 height);

@interface MetalKitViewDelegate: NSObject<MTKViewDelegate>
- (void)
    updateFinalWindowSize;
- (void)
    updateRenderViewSize: (int)at_i;
@end

extern MetalKitViewDelegate * apple_gpu_delegate;
extern u64 previous_time;

#endif // T1_GPU_H
