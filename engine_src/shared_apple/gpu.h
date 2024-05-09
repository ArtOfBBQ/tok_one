#ifndef GPU_H
#define GPU_H

#include "logger.h"
#include "memorystore.h"
#include "cpu_gpu_shared_types.h"
#include "cpu_to_gpu_types.h"
#include "window_size.h"
#include "objmodel.h"

#import <Metal/Metal.h>
#import <MetalKit/MetalKit.h>
#import <mach/mach_time.h>

extern uint64_t last_resize_request_at;

void apple_gpu_init(
    void (* arg_funcptr_shared_gameloop_update)(GPUDataForSingleFrame *));

@interface MetalKitViewDelegate: NSObject<MTKViewDelegate>
@property (retain) NSMutableArray * metal_textures;
@property (retain) id<MTLRenderPipelineState> diamond_pipeline_state;
@property (retain) id<MTLRenderPipelineState> alphablend_pipeline_state;
@property (retain) id<MTLDepthStencilState>   depth_stencil_state;

- (void)
    initializeTextureArray:(int32_t)texturearray_i
    textureCount:(uint32_t)texture_count
    singleImgWidth: (uint32_t)single_img_width
    singleImgHeight: (uint32_t)single_img_height;
- (void)
    updateTextureArray: (int32_t)texturearray_i
    atTexture: (int32_t)texture_i
    ofTextureArraySize: (uint32_t)texture_array_images_size
    withImageOfWidth: (uint32_t)image_width
    andHeight: (uint32_t)image_height
    pixelValues: (uint8_t *)rgba_values;
- (void)
    configureMetalWithDevice: (id<MTLDevice>)with_metal_device
    andPixelFormat: (MTLPixelFormat)pixel_format
    fromFolder: (NSString *)shader_lib_filepath;
- (void)
    copyLockedVertices;
- (void)
    updateViewport;
@end

extern MetalKitViewDelegate * apple_gpu_delegate;
extern uint64_t previous_time;
extern bool32_t has_retina_screen;

#endif

