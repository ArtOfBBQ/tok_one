// functions we must implement
#include "../shared/platform_layer.h" 

// shared functionality we can use
#include "../shared/window_size.h"
#include "../shared/vertex_types.h"
#include "../shared/box.h"
#include "../shared/software_renderer.h"
#include "../shared/gpu.h"

@interface
GameWindowDelegate: NSObject<NSWindowDelegate>
@end

@implementation GameWindowDelegate
- (void)windowWillClose:(NSNotification *)notification 
{
    [NSApp terminate: nil];
}
@end

/*
This functionality must be provided by the platform because
of iOS, where reading your own app's files is a security
ordeal
*/
FileBuffer * platform_read_file(char * filename) {

    FileBuffer * return_value = malloc(sizeof(FileBuffer));
    
    FILE * modelfile = fopen(
        filename,
        "rb");
    
    fseek(modelfile, 0, SEEK_END);
    unsigned long fsize = (unsigned long)ftell(modelfile);
    fseek(modelfile, 0, SEEK_SET);
    
    char * buffer = malloc(fsize);
    
    size_t bytes_read = fread(
        /* ptr: */
            buffer,
        /* size of each element to be read: */
            1,
        /* nmemb (no of members) to read: */
            fsize,
        /* stream: */
            modelfile);
    
    fclose(modelfile);
    if (bytes_read != fsize) {
        printf("Error - expected bytes read equal to fsize\n");
        return NULL;
    }

    return_value->contents = buffer;
    return_value->size = bytes_read;
    
    return return_value;
}

int main(int argc, const char * argv[]) 
{
    z_constants_init();
    
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
    
    renderer_init();
    
    return NSApplicationMain(argc, argv);
}

