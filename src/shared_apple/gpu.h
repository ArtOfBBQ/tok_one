#ifndef GPU_H
#define GPU_H

#import <Metal/Metal.h>
#import <MetalKit/MetalKit.h>

#include "../shared/vertex_types.h"
#include "../shared/window_size.h"
#include "../shared/software_renderer.h"


@interface
MetalKitViewDelegate: NSObject<MTKViewDelegate>
@property (retain) id<MTLCommandQueue> command_queue;
@property (retain) NSMutableArray * mac_vertex_buffers;
@property VertexBuffer render_commands;
@property (retain) id<MTLRenderPipelineState>
    solid_color_pipeline_state;

- (void)configureMetal;
@end

#endif

