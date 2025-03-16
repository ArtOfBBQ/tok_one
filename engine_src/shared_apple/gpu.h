#ifndef GPU_H
#define GPU_H

#include "logger.h"
#include "memorystore.h"
#include "common.h"
#include "cpu_gpu_shared_types.h"
#include "cpu_to_gpu_types.h"
#include "window_size.h"
#include "objmodel.h"
#include "userinput.h"

#import <Metal/Metal.h>
#import <MetalKit/MetalKit.h>
#import <mach/mach_time.h>

extern uint64_t last_resize_request_at;

bool32_t apple_gpu_init(
    void (* arg_funcptr_shared_gameloop_update)(GPUDataForSingleFrame *),
    id<MTLDevice> with_metal_device,
    NSString * shader_lib_filepath,
    char * error_msg_string);

void apple_gpu_initialize_texture_array(
    int32_t texturearray_i,
    uint32_t textures_count,
    uint32_t single_img_width,
    uint32_t single_img_height);

@interface MetalKitViewDelegate: NSObject<MTKViewDelegate>
- (void)
    updateViewport;
@end

extern MetalKitViewDelegate * apple_gpu_delegate;
extern uint64_t previous_time;
extern bool32_t has_retina_screen;

#endif
