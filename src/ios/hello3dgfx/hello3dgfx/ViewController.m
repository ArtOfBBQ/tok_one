#include <sys/mman.h>

#import "ViewController.h"
#import "gpu.h"

@implementation ViewController
MTKView * _my_mtk_view;

- (void)viewDidLoad {
    printf("running viewcontroller viewDidLoad...\n");
    [super viewDidLoad];
    
    _my_mtk_view = (MTKView *)self.view;
    
    _my_mtk_view.paused = NO;
    
    _my_mtk_view.enableSetNeedsDisplay = NO;
    
    _metal_device = MTLCreateSystemDefaultDevice();
    
    if (_metal_device == nil) {
        printf("error - _metal_device was nil after MTLCreateSystemDefaultDevice()\n");
        return;
    } else {
        printf(
            "_metal_device address: %p\n",
            _metal_device);
    }
    
    id<MTLCommandQueue> command_queue =
        [_metal_device newCommandQueue];
    
    // TODO: this triggers the error on device
    // when the debugger is not attached
    [_my_mtk_view setDevice: _metal_device];
    
    
    NSError *Error = NULL;
    
    NSString * shader_lib_filepath =
        [[NSBundle mainBundle]
            pathForResource: @"default"
            ofType: @"metallib"];
    
    printf(
        "shader_lib_filepath: %s\n",
        shader_lib_filepath);
    
    if (shader_lib_filepath == nil) {
        printf("error - no shader library found\n");
        return;
    }
    
    id<MTLLibrary> ShaderLibrary =
    [_metal_device
            newLibraryWithFile: shader_lib_filepath
            error: &Error];
    
    if (Error != NULL)
    {
        printf("error - couldn't load shader library\n");
        return;
    }
    
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
            _my_mtk_view.colorPixelFormat;
    
    id<MTLRenderPipelineState> solid_color_pipeline_state =
    [_metal_device
              newRenderPipelineStateWithDescriptor:
                  SolidColorPipelineDescriptor
              error:
                  &Error];
    
    if (Error != NULL)
    {
        printf("error - can't initialize solid_color_pipeline_state\n");
        return;
    }
    
    uint32_t page_size = getpagesize();
    uint32_t buffered_vertex_size = page_size * 1000;
    
    VertexBuffer render_commands = {};
    NSMutableArray * mac_vertex_buffers =
        [[NSMutableArray alloc] init];
    
    if (mac_vertex_buffers == NULL) {
        printf("error: max_vertex_buffers wasn't allocated\n");
        return;
    }
    
    for (uint32_t frame_i = 0;
          frame_i < 3;
          frame_i++)
    {
        BufferedVertexCollection buffered_vertex = {};
        ColoredVertex * new_vertex =
            (ColoredVertex *)mmap(
                0,
                buffered_vertex_size,
                PROT_READ | PROT_WRITE,
                MAP_PRIVATE | MAP_ANON,
                -1,
                0);
        
        if (new_vertex == NULL) {
            printf("ERROR - new_vertex wax NULL after mmap allocation!\n");
            return;
        }
        buffered_vertex.vertices = new_vertex;
        render_commands.vertex_buffers[frame_i] =
            buffered_vertex;
        
        id<MTLBuffer> MetalBufferedVertex =
        [_metal_device
                newBufferWithBytesNoCopy:
                    buffered_vertex.vertices
                length: buffered_vertex_size
                options: MTLResourceStorageModeShared
               deallocator: nil];
        [mac_vertex_buffers addObject: MetalBufferedVertex];
    }
    
    _mtk_view_delegate =
        [[MetalKitViewDelegate alloc] init];
    _my_mtk_view.delegate = _mtk_view_delegate;
    
    _mtk_view_delegate.mac_vertex_buffers = mac_vertex_buffers;
    _mtk_view_delegate.render_commands = render_commands;
    _mtk_view_delegate.solid_color_pipeline_state = solid_color_pipeline_state;
    _mtk_view_delegate.command_queue = command_queue;
    
    [_mtk_view_delegate configureMetal];
    
    init_projection_constants();
    init_renderer();
}

@end
