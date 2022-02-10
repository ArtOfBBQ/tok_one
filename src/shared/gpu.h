#ifndef GPU_H
#define GPU_H

#import <Metal/Metal.h>
#import <MetalKit/MetalKit.h>

#include "vertex_types.h"
#include "window_size.h"
#include "software_renderer.h"


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
