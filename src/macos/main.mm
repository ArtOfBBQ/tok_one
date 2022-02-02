#import <Metal/Metal.h>
#import <MetalKit/MetalKit.h>

#include "../shared/window_size.h"
#include "../shared/vertex_types.h"
#include "../shared/software_renderer.h"


@interface
GameWindowDelegate: NSObject<NSWindowDelegate>
@end

@implementation GameWindowDelegate
- (void)windowWillClose:(NSNotification *)notification 
{
    [NSApp terminate: nil];
}
@end

@interface
MetalKitViewDelegate: NSObject<MTKViewDelegate>
@property (retain) id<MTLCommandQueue> command_queue;
@property (retain) NSMutableArray * mac_vertex_buffers;
@property VertexBuffer render_commands;
@property (retain) id<MTLRenderPipelineState>
    solid_color_pipeline_state;
@end

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
    // NSLog(@"Draw in rect...");
    
    // TODO: this only works on retina
    // because on retina screens, the MTLViewport is 2x
    // the size of the window
    MTLViewport viewport = {
        0,
        0,
        WINDOW_WIDTH * 2,
        WINDOW_HEIGHT * 2 };
    
    uint32_t frame_i = _currentFrameIndex;
    
    ColoredVertex * vertices_for_gpu =
        _render_commands.vertex_buffers[frame_i].vertices;
    uint32_t vertices_for_gpu_size = 0;
    
    software_render(
        /* next_gpu_workload: */
            vertices_for_gpu,
        /* next_gpu_workload_size: */
            &vertices_for_gpu_size);
    
    @autoreleasepool 
    {
        id<MTLCommandBuffer> CommandBuffer =
            [[self command_queue] commandBuffer];
        
        MTLRenderPassDescriptor *RenderPassDescriptor =
            [view currentRenderPassDescriptor];
        RenderPassDescriptor.colorAttachments[0].loadAction =
            MTLLoadActionClear;
        
        MTLClearColor MetalClearColor =
            MTLClearColorMake(0.2f, 0.2f, 0.2f, 1.0f);
        RenderPassDescriptor.colorAttachments[0].clearColor =
            MetalClearColor;
        
        id<MTLRenderCommandEncoder> RenderEncoder =
            [CommandBuffer
                renderCommandEncoderWithDescriptor:
                    RenderPassDescriptor];
        [RenderEncoder setViewport: viewport];
        [RenderEncoder
            setRenderPipelineState:
                [self solid_color_pipeline_state]];
        
        id<MTLBuffer> MacBufferedVertex =
            [[self mac_vertex_buffers]
                objectAtIndex: _currentFrameIndex];
        
        [RenderEncoder
            setVertexBuffer: MacBufferedVertex  
            offset: 0 
            atIndex: 0];
        
        [RenderEncoder
            drawPrimitives: MTLPrimitiveTypeTriangle
            vertexStart: 0 
            vertexCount: vertices_for_gpu_size];
        
        [RenderEncoder endEncoding];
        
        // Schedule a present once the framebuffer is complete
        // using the current drawable
        id<CAMetalDrawable> NextDrawable =
            [view currentDrawable];
        [CommandBuffer presentDrawable: NextDrawable];
        
        uint32_t NextIndex = _currentFrameIndex + 1;
        
        if (NextIndex > 2)
        {
            NextIndex = 0;
        }
        
        _currentFrameIndex = NextIndex;
       
        /* 
        [CommandBuffer
            addCompletedHandler:
                ^(id<MTLCommandBuffer> commandBuffer)
            {
            }];
        */
        
        [CommandBuffer commit];
    }
}

- (void)mtkView:(MTKView *)view
    drawableSizeWillChange:(CGSize)size
{

}

@end

int main(int argc, const char * argv[]) 
{
    NSRect WindowRectangle = NSMakeRect(
        0.0f,
        0.0f,
        WINDOW_WIDTH,
        WINDOW_HEIGHT);
    
    NSWindow *Window =
        [[NSWindow alloc]
            initWithContentRect: WindowRectangle 
            styleMask: (NSWindowStyleMaskTitled
                        | NSWindowStyleMaskClosable)
            backing: NSBackingStoreBuffered 
            defer: NO];
    
    GameWindowDelegate *WindowDelegate =
        [[GameWindowDelegate alloc] init];
    
    [Window setDelegate: WindowDelegate];
    [Window setTitle: @"Lore Seeker"];
    [Window makeKeyAndOrderFront: nil];
    
    id<MTLDevice> MetalDevice =
        MTLCreateSystemDefaultDevice();
    id<MTLCommandQueue> CommandQueue =
        [MetalDevice newCommandQueue];
    
    MTKView *MetalKitView =
        [[MTKView alloc]
            initWithFrame: WindowRectangle
            device: MetalDevice];
    Window.contentView = MetalKitView;
    
    NSError *Error = NULL;
    
    NSString *ShaderLibraryFilepath =
        [[NSBundle mainBundle]
            pathForResource: @"Shaders"
            ofType: @"metallib"];
    id<MTLLibrary> ShaderLibrary =
        [MetalDevice
            newLibraryWithFile: ShaderLibraryFilepath 
            error: &Error];
    id<MTLFunction> VertexShader =
        [ShaderLibrary newFunctionWithName: @"vertexShader"];
    id<MTLFunction> FragmentShader =
        [ShaderLibrary newFunctionWithName: @"fragmentShader"];
    
    if (Error != NULL)
    {
        [NSException
            raise: @"Can't Setup Metal" 
            format: @"Unable to shader libraries"];
    }
    
    // Setup Render Pipeline States
    MTLRenderPipelineDescriptor *SolidColorPipelineDescriptor =
        [[MTLRenderPipelineDescriptor alloc] init];
    [SolidColorPipelineDescriptor
        setVertexFunction: VertexShader];
    [SolidColorPipelineDescriptor
        setFragmentFunction: FragmentShader];
    SolidColorPipelineDescriptor
        .colorAttachments[0]
        .pixelFormat = 
            MetalKitView.colorPixelFormat;
    
    id<MTLRenderPipelineState> SolidColorPipelineState =
        [MetalDevice
            newRenderPipelineStateWithDescriptor:
                SolidColorPipelineDescriptor 
            error:
                &Error];
    
    if (Error != NULL)
    {
        [NSException
            raise: @"Can't Setup Metal" 
            format: @"Unable to setup rendering pipeline state"];
    }
    
    uint32_t PageSize = getpagesize();
    uint32_t BufferedVertexSize = PageSize*1000;
    
    VertexBuffer RenderCommands = {};

    NSMutableArray *mac_vertex_buffers =
        [[NSMutableArray alloc] init];
    
    for (uint32_t frame_i = 0;
         frame_i < 3;
         frame_i++)
    {
        BufferedVertex buffered_vertex = {};
        buffered_vertex.vertices =
            (ColoredVertex *)mmap(
                0,
                BufferedVertexSize,
                PROT_READ | PROT_WRITE,
                MAP_PRIVATE | MAP_ANON,
                -1,
                0);
        RenderCommands.vertex_buffers[frame_i] =
            buffered_vertex;
        
        id<MTLBuffer> MetalBufferedVertex =
            [MetalDevice
                newBufferWithBytesNoCopy:
                    buffered_vertex.vertices
                length: BufferedVertexSize 
                options: MTLResourceStorageModeShared
                deallocator: nil];
        [mac_vertex_buffers addObject: MetalBufferedVertex];
    }
    
    MetalKitViewDelegate *ViewDelegate =
        [[MetalKitViewDelegate alloc] init];
    [MetalKitView setDelegate: ViewDelegate];
    
    [ViewDelegate setMac_vertex_buffers: mac_vertex_buffers];
    [ViewDelegate setRender_commands: RenderCommands];
    [ViewDelegate
        setSolid_color_pipeline_state: SolidColorPipelineState];
    [ViewDelegate setCommand_queue: CommandQueue];
    [ViewDelegate configureMetal];
    
    z_constants_init();
    
    return NSApplicationMain(argc, argv);
}

