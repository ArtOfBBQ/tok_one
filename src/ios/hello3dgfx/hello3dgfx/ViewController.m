#include <sys/mman.h>

#import "ViewController.h"
#import "gpu.h"


@interface ViewController ()
@end

@implementation ViewController
    MTKView * mtk_view;
    MetalKitViewDelegate * mtk_view_delegate;

- (void)viewDidLoad {
    printf("running viewcontroller viewDidLoad...\n");
    [super viewDidLoad];
    
    mtk_view = (MTKView *)self.view;
    
    mtk_view.paused = NO;
    mtk_view.enableSetNeedsDisplay = NO;
    
    id<MTLDevice> metal_device = MTLCreateSystemDefaultDevice();
    id<MTLCommandQueue> command_queue =
        [metal_device newCommandQueue];
    // TODO: this is different from ted's init,
    // study the difference closely
    mtk_view.device = metal_device;
    
    NSError *Error = NULL;
    
    NSString *ShaderLibraryFilepath =
        [[NSBundle mainBundle]
            pathForResource: @"default"
            ofType: @"metallib"];
    
    printf("shader library filepath: %s\n", ShaderLibraryFilepath);
    
    id<MTLLibrary> ShaderLibrary =
        [metal_device
            newLibraryWithFile: ShaderLibraryFilepath
            error: &Error];
    id<MTLFunction> vertex_shader =
        [ShaderLibrary newFunctionWithName: @"vertexShader"];
    id<MTLFunction> fragment_shader =
        [ShaderLibrary newFunctionWithName: @"fragmentShader"];
    
    // Setup Render Pipeline States
    MTLRenderPipelineDescriptor *SolidColorPipelineDescriptor =
        [[MTLRenderPipelineDescriptor alloc] init];
    [SolidColorPipelineDescriptor
        setVertexFunction: vertex_shader];
    [SolidColorPipelineDescriptor
        setFragmentFunction: fragment_shader];
    SolidColorPipelineDescriptor
        .colorAttachments[0]
        .pixelFormat =
            mtk_view.colorPixelFormat;
    
    id<MTLRenderPipelineState> solid_color_pipeline_state =
          [metal_device
              newRenderPipelineStateWithDescriptor:
                  SolidColorPipelineDescriptor
              error:
                  &Error];
    
    uint32_t page_size = getpagesize();
    uint32_t buffered_vertex_size = page_size * 1000;
    
    VertexBuffer render_commands = {};
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
                buffered_vertex_size,
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
                length: buffered_vertex_size
                options: MTLResourceStorageModeShared
               deallocator: nil];
        [mac_vertex_buffers addObject: MetalBufferedVertex];
    }
    
    mtk_view_delegate =
        [[MetalKitViewDelegate alloc] init];
    mtk_view.delegate = mtk_view_delegate;
    
    mtk_view_delegate.mac_vertex_buffers = mac_vertex_buffers;
    mtk_view_delegate.render_commands = render_commands;
    mtk_view_delegate.solid_color_pipeline_state = solid_color_pipeline_state;
    mtk_view_delegate.command_queue = command_queue;
    
    [mtk_view_delegate configureMetal];
    
    z_constants_init();
    renderer_init();
}

@end
