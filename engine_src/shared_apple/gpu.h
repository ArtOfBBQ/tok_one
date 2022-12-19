#ifndef GPU_H
#define GPU_H

#include "../shared/logger.h"
#include "../shared/memorystore.h"
#include "../shared/texture_array.h"
#include "../shared/vertex_types.h"
#include "../shared/vertex_buffer_types.h"
#include "../shared/window_size.h"
#include "../shared/gameloop.h"

#import <Metal/Metal.h>
#import <MetalKit/MetalKit.h>
#import <mach/mach_time.h>

extern uint64_t last_resize_request_at;
extern uint32_t block_drawinmtkview;

@interface MetalKitViewDelegate: NSObject<MTKViewDelegate>
@property (retain) NSMutableArray * vertex_buffers;
@property (retain) NSMutableArray * metal_textures;
@property (retain) id<MTLRenderPipelineState> combo_pipeline_state;

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
    drawClearScreen: (MTKView *)view;
- (void)
    configureMetalWithDevice: (id<MTLDevice>)with_metal_device
    andPixelFormat: (MTLPixelFormat)pixel_format
    fromFolder: (NSString *)shader_lib_filepath;
@end

extern MetalKitViewDelegate * apple_gpu_delegate;
extern uint64_t previous_time;
extern bool32_t has_retina_screen;

#endif