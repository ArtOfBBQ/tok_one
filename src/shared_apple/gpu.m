#import "gpu.h"

@implementation MetalKitViewDelegate
{
    NSUInteger _currentFrameIndex;
}

- (void)configureMetal
{
    _currentFrameIndex = 0;
}

- (void)drawInMTKView:(MTKView *)view
{
    // TODO: this only works on retina
    // because on retina screens, the MTLViewport is 2x
    // the size of the window
    MTLViewport viewport = {
        0,
        0,
        window_width * 2.0f,
        window_height * 2.0f };
    
    uint32_t frame_i = _currentFrameIndex;
    
    ColoredVertex * colored_vertices_for_gpu =
        _render_commands.vertex_buffers[frame_i].colored_vertices;
    uint32_t colored_vertices_for_gpu_size = 0;

    TexturedVertex * textured_vertices_for_gpu =
        _render_commands.vertex_buffers[frame_i].textured_vertices;
    uint32_t textured_vertices_for_gpu_size = 0;
    
    software_render_colored_vertices(
        /* next_gpu_workload: */
            colored_vertices_for_gpu,
        /* next_gpu_workload_size: */
            &colored_vertices_for_gpu_size);
    assert(colored_vertices_for_gpu_size > 0);
    
    software_render_textured_vertices(
        /* next_gpu_workload: */
            textured_vertices_for_gpu,
        /* next_gpu_workload_size: */
            &textured_vertices_for_gpu_size);
    assert(textured_vertices_for_gpu_size > 0);
    
    @autoreleasepool 
    {
        id<MTLCommandBuffer> command_buffer =
            [[self command_queue] commandBuffer];
        
        MTLRenderPassDescriptor *RenderPassDescriptor =
            [view currentRenderPassDescriptor];
        RenderPassDescriptor.colorAttachments[0].loadAction =
            MTLLoadActionClear;
        
        MTLClearColor clear_color =
            MTLClearColorMake(0.2f, 0.2f, 0.1f, 1.0f);
        RenderPassDescriptor.colorAttachments[0].clearColor =
            clear_color;
        
        id<MTLRenderCommandEncoder> render_encoder =
            [command_buffer
                renderCommandEncoderWithDescriptor:
                    RenderPassDescriptor];
        [render_encoder setViewport: viewport];
       
        // encode the drawing of all colored triangles 
        id<MTLBuffer> current_colored_buffered_vertices =
            [[self colored_vertex_buffers]
                objectAtIndex: _currentFrameIndex];
        [render_encoder
            setVertexBuffer: current_colored_buffered_vertices  
            offset: 0 
            atIndex: 0];
        [render_encoder
            setRenderPipelineState:
                [self solid_color_pipeline_state]];
        [render_encoder
            drawPrimitives: MTLPrimitiveTypeTriangle
            vertexStart: 0 
            vertexCount: colored_vertices_for_gpu_size];
        
        // encode the drawing of all textured triangles 
        id<MTLBuffer> current_texture_buffered_vertices =
            [[self textured_vertex_buffers]
                objectAtIndex: _currentFrameIndex];
        [render_encoder
            setVertexBuffer: current_texture_buffered_vertices  
            offset: 0 
            atIndex: 0];
        [render_encoder
            setRenderPipelineState:
                [self texture_pipeline_state]];
        [render_encoder
            drawPrimitives: MTLPrimitiveTypeTriangle
            vertexStart: 0 
            vertexCount: textured_vertices_for_gpu_size];
        
        [render_encoder endEncoding];
        
        // Schedule a present once the framebuffer is complete
        // using the current drawable
        id<CAMetalDrawable> current_drawable =
            [view currentDrawable];
        [command_buffer presentDrawable: current_drawable];
        
        uint32_t next_index = _currentFrameIndex + 1;
        if (next_index > 2) { next_index = 0; }
        
        _currentFrameIndex = next_index;
        
        /* 
        [command_buffer
            addCompletedHandler:
                ^(id<MTLCommandBuffer> commandBuffer)
            {
            }];
        */
        
        [command_buffer commit];
    }
}

- (void)mtkView:(MTKView *)view
    drawableSizeWillChange:(CGSize)size
{

}

@end
