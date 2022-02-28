#ifndef GPU_H
#define GPU_H

#import <Metal/Metal.h>
#import <MetalKit/MetalKit.h>

#include "../shared/vertex_types.h"
#include "../shared/vertex_buffer_types.h"
#include "../shared/window_size.h"
#include "../shared/software_renderer.h"

@interface
MetalKitViewDelegate: NSObject<MTKViewDelegate>
@property (retain) id<MTLCommandQueue> command_queue;
@property (retain) NSMutableArray * vertex_buffers;
@property VertexBuffer render_commands;
@property (retain) id<MTLRenderPipelineState>
    combo_pipeline_state;
@property (retain) NSMutableArray * metal_textures;
- (void)configureMetal;
@end

#endif

