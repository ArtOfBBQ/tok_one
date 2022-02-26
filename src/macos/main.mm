#include <simd/simd.h>

// functions we must implement
#include "../shared/platform_layer.h" 

// shared functionality we can use
#include "../shared/window_size.h"
#include "../shared/vertex_types.h"
#include "../shared/zpolygon.h"
#include "../shared/software_renderer.h"
#include "../shared_apple/gpu.h"
#include "../shared/static_redefinitions.h"


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
    NSScreen *screen = [[NSScreen screens] objectAtIndex:0];
    NSRect full_screen_rect = [screen frame]; 
    
    window_height = NSHeight(full_screen_rect);
    window_width = NSWidth(full_screen_rect);

    init_projection_constants();
    
    NSWindow *window =
        [[NSWindow alloc]
            initWithContentRect: full_screen_rect 
            styleMask: (NSWindowStyleMaskTitled
                        | NSWindowStyleMaskClosable
                        | NSWindowStyleMaskFullScreen)
            backing: NSBackingStoreBuffered 
            defer: NO];
    
    GameWindowDelegate *window_delegate =
        [[GameWindowDelegate alloc] init];
   
    [window setDelegate: window_delegate];
    [window setTitle: @"Hello, 3dgfx!"];
    [window makeKeyAndOrderFront: nil];
    
    id<MTLDevice> metal_device =
        MTLCreateSystemDefaultDevice();
    id<MTLCommandQueue> command_queue =
        [metal_device newCommandQueue];
    
    MTKView * mtk_view =
        [[MTKView alloc]
            initWithFrame: full_screen_rect
            device: metal_device];
    window.contentView = mtk_view;
    
    NSError *Error = NULL;
    
    NSString *ShaderLibraryFilepath =
        [[NSBundle mainBundle]
            pathForResource: @"Shaders"
            ofType: @"metallib"];
    id<MTLLibrary> shader_library =
        [metal_device
            newLibraryWithFile: ShaderLibraryFilepath 
            error: &Error];
    id<MTLFunction> vertex_shader =
        [shader_library newFunctionWithName:
            @"vertex_shader"];
    id<MTLFunction> fragment_shader =
        [shader_library newFunctionWithName:
            @"fragment_shader"];
    id<MTLFunction> texture_vertex_shader =
        [shader_library newFunctionWithName:
            @"texture_vertex_shader"];
    id<MTLFunction> texture_fragment_shader =
        [shader_library newFunctionWithName:
            @"texture_fragment_shader"];
    
    if (Error != NULL)
    {
        [NSException
            raise: @"Can't Setup Metal" 
            format: @"Unable to shader libraries"];
    }
    
    // Setup Render Pipeline States
    // for colored triangles
    MTLRenderPipelineDescriptor *solid_color_pipeline_descriptor =
        [[MTLRenderPipelineDescriptor alloc] init];
    [solid_color_pipeline_descriptor
        setVertexFunction: vertex_shader];
    [solid_color_pipeline_descriptor
        setFragmentFunction: fragment_shader];
    solid_color_pipeline_descriptor
        .colorAttachments[0]
        .pixelFormat = 
            mtk_view.colorPixelFormat;
    
    // for textured triangles
    MTLRenderPipelineDescriptor *texture_pipeline_descriptor =
        [[MTLRenderPipelineDescriptor alloc] init];
    [texture_pipeline_descriptor
        setVertexFunction: texture_vertex_shader];
    [texture_pipeline_descriptor
        setFragmentFunction: texture_fragment_shader];
    texture_pipeline_descriptor
        .colorAttachments[0]
        .pixelFormat = 
            mtk_view.colorPixelFormat;
    
    id<MTLRenderPipelineState> solid_color_pipeline_state =
        [metal_device
            newRenderPipelineStateWithDescriptor:
                solid_color_pipeline_descriptor 
            error:
                &Error];
    if (Error != NULL)
    {
        [NSException
            raise: @"Can't Setup Metal" 
            format: @"Unable to setup rendering pipeline state"];
    }
    id<MTLRenderPipelineState> texture_pipeline_state =
        [metal_device
            newRenderPipelineStateWithDescriptor:
                texture_pipeline_descriptor 
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
    
    VertexBuffer render_commands = {};
    
    NSMutableArray *colored_vertex_buffers =
        [[NSMutableArray alloc] init];
    NSMutableArray *textured_vertex_buffers =
        [[NSMutableArray alloc] init];
    
    for (uint32_t frame_i = 0;
         frame_i < 3;
         frame_i++)
    {
        BufferedVertexCollection buffered_vertex = {};
        buffered_vertex.colored_vertices =
            (ColoredVertex *)mmap(
                0,
                BufferedVertexSize,
                PROT_READ | PROT_WRITE,
                MAP_PRIVATE | MAP_ANON,
                -1,
                0);

        buffered_vertex.textured_vertices =
            (TexturedVertex *)mmap(
                0,
                BufferedVertexSize,
                PROT_READ | PROT_WRITE,
                MAP_PRIVATE | MAP_ANON,
                -1,
                0);
        
        render_commands.vertex_buffers[frame_i] =
            buffered_vertex;
        
        id<MTLBuffer> MetalBufferedVertex =
            [metal_device
                newBufferWithBytesNoCopy:
                    buffered_vertex.colored_vertices
                length:
                    BufferedVertexSize
                options:
                    MTLResourceStorageModeShared
                deallocator:
                    nil];
        [colored_vertex_buffers
            addObject: MetalBufferedVertex];
        
        id<MTLBuffer> MetalBufferedTexturedVertices =
            [metal_device
                newBufferWithBytesNoCopy:
                    buffered_vertex.textured_vertices
                length:
                    BufferedVertexSize
                options:
                    MTLResourceStorageModeShared
                deallocator:
                    nil];
        [textured_vertex_buffers
            addObject: MetalBufferedTexturedVertices];
    }
    
    MetalKitViewDelegate *ViewDelegate =
        [[MetalKitViewDelegate alloc] init];
    [mtk_view setDelegate: ViewDelegate];
    
    [ViewDelegate
        setColored_vertex_buffers: colored_vertex_buffers];
    [ViewDelegate
        setTextured_vertex_buffers: textured_vertex_buffers];
    [ViewDelegate
        setRender_commands: render_commands];
    [ViewDelegate
        setSolid_color_pipeline_state:
            solid_color_pipeline_state];
    [ViewDelegate
        setTexture_pipeline_state:
            texture_pipeline_state];
    [ViewDelegate setCommand_queue: command_queue];
    [ViewDelegate configureMetal];
    
    init_renderer();
    
    return NSApplicationMain(argc, argv);
}

