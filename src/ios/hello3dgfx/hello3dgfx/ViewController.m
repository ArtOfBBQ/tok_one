#import "ViewController.h"

@implementation ViewController
MTKView * _my_mtk_view;

- (void)viewDidLoad {
    printf("running viewcontroller viewDidLoad...\n");
    [super viewDidLoad];
    
    window_height = [UIScreen mainScreen].bounds.size.height; 
    window_width = [UIScreen mainScreen].bounds.size.width;    
    
    printf("setting up projection constants...¥n");
    init_projection_constants();
    printf("setting up renderer...¥n");
    init_renderer();
    
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
    [_my_mtk_view setDevice: _metal_device];
    
    NSError *Error = NULL;
    NSString * shader_lib_filepath =
        [[NSBundle mainBundle]
            pathForResource: @"default"
            ofType: @"metallib"];
    
    _mtk_view_delegate =
        [[MetalKitViewDelegate alloc] init];
    _my_mtk_view.delegate = _mtk_view_delegate;
    [_mtk_view_delegate
        configureMetalWithDevice: _metal_device
        andPixelFormat: _my_mtk_view.colorPixelFormat
        fromFolder: shader_lib_filepath];
    
    
        /*
    id<MTLCommandQueue> command_queue =
        [_metal_device newCommandQueue];
    
    // when the debugger is not attached
    
    
    
    
        
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
        [ShaderLibrary newFunctionWithName: @"vertex_shader"];
    id<MTLFunction> fragment_shader =
        [ShaderLibrary newFunctionWithName: @"fragment_shader"];
    
    // Setup Render Pipeline States
    MTLRenderPipelineDescriptor *ComboPipelineDescriptor =
        [[MTLRenderPipelineDescriptor alloc] init];
    [ComboPipelineDescriptor
        setVertexFunction: vertex_shader];
    [ComboPipelineDescriptor
        setFragmentFunction: fragment_shader];
    ComboPipelineDescriptor
        .colorAttachments[0]
        .pixelFormat =
            _my_mtk_view.colorPixelFormat;
    
    id<MTLRenderPipelineState> combo_pipeline_state =
    [_metal_device
        newRenderPipelineStateWithDescriptor:
            ComboPipelineDescriptor
        error:
            &Error];
    
    if (Error != NULL)
    {
        printf("error - can't initialize combo_pipeline_state\n");
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
        Vertex * new_vertex =
            (Vertex *)mmap(
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
    
    _mtk_view_delegate.vertex_buffers = mac_vertex_buffers;
    _mtk_view_delegate.render_commands = render_commands;
    _mtk_view_delegate.combo_pipeline_state = combo_pipeline_state;
    _mtk_view_delegate.command_queue = command_queue;
    
    
        
    printf("setting up textures...¥n");
    _mtk_view_delegate.metal_textures = [[NSMutableArray alloc] init];
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
            [_metal_device
                newTextureWithDescriptor:texture_descriptor];
        MTLRegion region = {
            { 0, 0, 0 },
            { textures[i]->width, textures[i]->height, 1}
        };
        
        assert(textures[i]->width >= 10);
        [texture
            replaceRegion:region
            mipmapLevel:0
            withBytes:textures[i]->rgba_values
            bytesPerRow:4 * textures[i]->width];
        [[_mtk_view_delegate metal_textures] addObject: texture];
        
    }
    printf("succesfully set up textures...¥n");
    */
}

@end
