#ifndef GPU_H
#define GPU_H

#include "T1_logger.h"
#include "T1_mem.h"
#include "T1_std.h"
#include "T1_cpu_gpu_shared_types.h"
#include "T1_cpu_to_gpu_types.h"
#include "T1_global.h"
#include "T1_objmodel.h"
#include "T1_io.h"

#import <Metal/Metal.h>
#import <MetalKit/MetalKit.h>
#import <mach/mach_time.h>

extern uint64_t last_resize_request_at;

bool32_t apple_gpu_init(
    void (* arg_funcptr_shared_gameloop_update)
        (T1GPUFrame *),
    void (* arg_funcptr_gameloop_update_after_render)(void),
    id<MTLDevice> with_metal_device,
    NSString * shader_lib_filepath,
    float backing_scale_factor,
    char * error_msg_string);

void apple_gpu_make_depth_texture(
    int32_t  slice_i,
    uint32_t width,
    uint32_t height);

@interface MetalKitViewDelegate: NSObject<MTKViewDelegate>
- (void)
    updateFinalWindowSize;
- (void)
    updateInternalRenderViewSize: (unsigned int)at_i;
@end

extern MetalKitViewDelegate * apple_gpu_delegate;
extern uint64_t previous_time;

#endif
