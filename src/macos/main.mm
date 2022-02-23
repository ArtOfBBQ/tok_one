#include <simd/simd.h>

// functions we must implement
#include "../shared/platform_layer.h" 

// shared functionality we can use
#include "../shared/window_size.h"
#include "../shared/vertex_types.h"
#include "../shared/box.h"
#include "../shared/software_renderer.h"
#include "../shared_apple/gpu.h"

@interface
GameWindowDelegate: NSObject<NSWindowDelegate>
@end

@implementation GameWindowDelegate
- (void)windowWillClose:(NSNotification *)notification 
{
    [NSApp terminate: nil];
}
@end


int main(int argc, const char * argv[]) 
{
    init_z_constants();
    
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
    [Window setTitle: @"Hello, 3dgfx!"];
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
    uint32_t BufferedVertexSize = PageSize * 1000;
    
    VertexBuffer RenderCommands = {};

    NSMutableArray *mac_vertex_buffers =
        [[NSMutableArray alloc] init];
    
    for (uint32_t frame_i = 0;
         frame_i < 3;
         frame_i++)
    {
        BufferedVertexCollection buffered_vertex = {};
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
                length:
                    BufferedVertexSize
                options:
                    MTLResourceStorageModeShared
                deallocator:
                    nil];
        
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
    
    init_renderer();
    
    return NSApplicationMain(argc, argv);
}

