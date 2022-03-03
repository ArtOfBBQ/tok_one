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
    init_renderer();
    
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
    
    if (Error != NULL)
    {
        [NSException
            raise: @"Can't Setup Metal" 
            format: @"Unable to shader libraries"];
    }
    
    // Setup combo pipeline that handles
    // both colored & textured triangles
    MTLRenderPipelineDescriptor *combo_pipeline_descriptor =
        [[MTLRenderPipelineDescriptor alloc] init];
    [combo_pipeline_descriptor
        setVertexFunction: vertex_shader];
    [combo_pipeline_descriptor
        setFragmentFunction: fragment_shader];
    combo_pipeline_descriptor
        .colorAttachments[0]
        .pixelFormat = 
            mtk_view.colorPixelFormat;
    
    id<MTLRenderPipelineState> combo_pipeline_state =
        [metal_device
            newRenderPipelineStateWithDescriptor:
                combo_pipeline_descriptor 
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
    
    NSMutableArray * vertex_buffers =
        [[NSMutableArray alloc] init];
    
    for (uint32_t frame_i = 0;
         frame_i < 3;
         frame_i++)
    {
        BufferedVertexCollection buffered_vertex = {};
        buffered_vertex.vertices =
            (Vertex *)mmap(
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
                    buffered_vertex.vertices
                length:
                    BufferedVertexSize
                options:
                    MTLResourceStorageModeShared
                deallocator:
                    nil];
        [vertex_buffers
            addObject: MetalBufferedVertex];
    }
    
    MetalKitViewDelegate *ViewDelegate =
        [[MetalKitViewDelegate alloc] init];
    [mtk_view setDelegate: ViewDelegate];
    
    [ViewDelegate
        setVertex_buffers: vertex_buffers];
    [ViewDelegate
        setRender_commands: render_commands];
    [ViewDelegate
        setCombo_pipeline_state:
            combo_pipeline_state];
    [ViewDelegate setCommand_queue: command_queue];
    [ViewDelegate configureMetal];
    
    ViewDelegate.metal_textures = [[NSMutableArray alloc] init];
    assert(texture_count > 0);
    for (uint32_t i = 0; i < texture_count; i++) {
        MTLTextureDescriptor * texture_descriptor =
            [[MTLTextureDescriptor alloc] init]; 
        texture_descriptor.pixelFormat =
            MTLPixelFormatRGBA8Unorm;
        texture_descriptor.width =
            textures[i]->width;
        texture_descriptor.height =
            textures[i]->height;
        id<MTLTexture> texture =
            [metal_device
                newTextureWithDescriptor:texture_descriptor];
        MTLRegion region = {
            { 0, 0, 0 },                   // MTLOrigin
            { textures[i]->width, textures[i]->height, 1 }
        };
        
        assert(textures[i]->width >= 10);
        assert(4 * textures[i]->width >= 40);
        [texture
            replaceRegion:region
            mipmapLevel: 0
            withBytes: textures[i]->rgba_values
            bytesPerRow: 4 * textures[i]->width];
        [[ViewDelegate metal_textures]
            addObject: texture];
    }
    assert([[ViewDelegate metal_textures] count] > 0);
     
    return NSApplicationMain(argc, argv);
}

