#import "gpu.h"

MetalKitViewDelegate * apple_gpu_delegate = NULL;

static uint32_t already_drawing = false;

@implementation MetalKitViewDelegate
{
    NSUInteger current_frame_i;
    MTLViewport viewport;
    VertexBuffer render_commands;
    
    id<MTLDevice> metal_device;
    id<MTLCommandQueue> command_queue;
    id<MTLRenderPipelineState> combo_pipeline_state;
    id<MTLDepthStencilState> depth_stencil_state;
}

- (void)
    configureMetalWithDevice: (id<MTLDevice>)with_metal_device
    andPixelFormat: (MTLPixelFormat)pixel_format
    fromFolder: (NSString *)shader_lib_filepath
{
    current_frame_i = 0;
    
    metal_device = with_metal_device;
    
    NSError *Error = NULL;
    id<MTLLibrary> shader_library =
        [with_metal_device newDefaultLibrary];
    
    if (shader_library == NULL)
    {
        log_append("failed to load default shader lib, trying ");
        log_append(
            [shader_lib_filepath
                cStringUsingEncoding: NSASCIIStringEncoding]);
        log_append("\n");
        
        shader_library =
            [with_metal_device
                newLibraryWithFile: shader_lib_filepath 
                error: &Error];
        
        if (shader_library == NULL) {
            log_append("Failed to find the shader library again\n");
            if (Error != NULL) {
                NSLog(@" error => %@ ", [Error userInfo]);
            }
            return;
        } else {
            log_append("Success! Found the shader lib on 2nd try.\n");
        }
    }
    
    id<MTLFunction> vertex_shader =
        [shader_library newFunctionWithName:
            @"vertex_shader"];
    id<MTLFunction> fragment_shader =
        [shader_library newFunctionWithName:
            @"fragment_shader"];
    
    // Setup combo pipeline that handles
    // both colored & textured triangles
    MTLRenderPipelineDescriptor * combo_pipeline_descriptor =
        [[MTLRenderPipelineDescriptor alloc] init];
    [combo_pipeline_descriptor
        setVertexFunction: vertex_shader];
    [combo_pipeline_descriptor
        setFragmentFunction: fragment_shader];
    combo_pipeline_descriptor
        .colorAttachments[0]
        .pixelFormat = pixel_format;
    // Mix colors according to alpha channel
    [combo_pipeline_descriptor
        .colorAttachments[0]
        setBlendingEnabled: YES];
    combo_pipeline_descriptor
        .colorAttachments[0].sourceRGBBlendFactor =
            MTLBlendFactorSourceAlpha;
    combo_pipeline_descriptor
        .colorAttachments[0].destinationRGBBlendFactor =
            MTLBlendFactorOneMinusSourceAlpha;
    
    combo_pipeline_descriptor.depthAttachmentPixelFormat =
        MTLPixelFormatDepth32Float;
    
    combo_pipeline_state =
        [with_metal_device
            newRenderPipelineStateWithDescriptor:
                combo_pipeline_descriptor 
            error:
                &Error];
    
    MTLDepthStencilDescriptor * depthDescriptor = [MTLDepthStencilDescriptor new];
    depthDescriptor.depthCompareFunction = MTLCompareFunctionLessEqual;
    depthDescriptor.depthWriteEnabled = true;
    depth_stencil_state = [with_metal_device newDepthStencilStateWithDescriptor:depthDescriptor];
    
    if (Error != NULL)
    {
        NSLog(@" error => %@ ", [Error userInfo]);
        [NSException
            raise: @"Can't Setup Metal" 
            format: @"Unable to setup rendering pipeline state"];
    }
        
    _vertex_buffers = [[NSMutableArray alloc] init];
    
    // TODO: use the apple-approved page size constant instead of
    // hardcoding 4096, and verify it returns 4096
    for (
        uint32_t frame_i = 0;
        frame_i < 3;
        frame_i++)
    {
        BufferedVertexCollection buffered_vertex;
        buffered_vertex.vertices_size = MAX_VERTICES_PER_BUFFER;
        uint64_t allocation_size = sizeof(Vertex) * buffered_vertex.vertices_size;
        allocation_size += (4096 - (allocation_size % 4096));
        assert(allocation_size % 4096 == 0);
        buffered_vertex.vertices =
            (Vertex *)malloc_from_unmanaged_aligned(
                allocation_size,
                4096);
        
        render_commands.vertex_buffers[frame_i] = buffered_vertex;
        
        id<MTLBuffer> MetalBufferedVertex =
            [with_metal_device
                /* the pointer needs to be page aligned */
                newBufferWithBytesNoCopy:
                    buffered_vertex.vertices
                /* the length weirdly needs to be page aligned also */
                length:
                    allocation_size
                options:
                    MTLResourceStorageModeShared
                /* deallocator = nil to opt out */
                deallocator:
                    nil];
        [_vertex_buffers addObject: MetalBufferedVertex];
    }
    
    _metal_textures = [
        [NSMutableArray alloc]
            initWithCapacity: TEXTUREARRAYS_SIZE];
    
    viewport.originX = 0;
    viewport.originY = 0;
    viewport.width = window_width * (has_retina_screen ? 2.0f : 1.0f);
    viewport.height = window_height * (has_retina_screen ? 2.0f : 1.0f);
    viewport.znear = 0.0f;
    viewport.zfar = 1.0f;
    
    command_queue = [with_metal_device newCommandQueue];
    
    log_append("finished configureMetalWithDevice\n");
}

- (void)
    initializeTextureArray  : (int32_t)texturearray_i
    textureCount            : (uint32_t)texture_count
    singleImgWidth          : (uint32_t)single_img_width
    singleImgHeight         : (uint32_t)single_img_height
{
    // we always overwrite textures, so pad them to match first
    while ((int32_t)[_metal_textures count] <= texturearray_i) {
        MTLTextureDescriptor * texture_descriptor =
            [[MTLTextureDescriptor alloc] init];
        texture_descriptor.textureType = MTLTextureType2DArray;
        texture_descriptor.arrayLength = 1;
        texture_descriptor.pixelFormat = MTLPixelFormatRGBA8Unorm;
        texture_descriptor.width = 10;
        texture_descriptor.height = 10;
        id<MTLTexture> texture =
            [metal_device newTextureWithDescriptor:texture_descriptor];
        [_metal_textures addObject: texture];
    }
    
    MTLTextureDescriptor * texture_descriptor =
        [[MTLTextureDescriptor alloc] init];
    texture_descriptor.textureType = MTLTextureType2DArray;
    texture_descriptor.arrayLength = texture_count;
    texture_descriptor.pixelFormat = MTLPixelFormatRGBA8Unorm;
    texture_descriptor.width = single_img_width;
    texture_descriptor.height = single_img_height;
    
    id<MTLTexture> texture =
        [metal_device
            newTextureWithDescriptor:texture_descriptor];
    
    [_metal_textures
        replaceObjectAtIndex:(uint32_t)texturearray_i
        withObject: texture];
}
- (void)
    updateTextureArray : (int32_t)texturearray_i
    atTexture          : (int32_t)texture_i
    ofTextureArraySize : (uint32_t)texture_array_images_size
    withImageOfWidth   : (uint32_t)image_width
    andHeight          : (uint32_t)image_height
    pixelValues        : (uint8_t *)rgba_values
{
    log_assert(texture_i >= 0);
    
    if (texturearray_i >= (int32_t)[_metal_textures count]) {
        log_append(
            "Warning: tried to update uninitialized texturearray ");
        log_append_int(texturearray_i);
        log_append("\n");
        return;
    }
        
    MTLTextureDescriptor * texture_descriptor =
        [[MTLTextureDescriptor alloc] init];
    texture_descriptor.textureType = MTLTextureType2DArray;
    texture_descriptor.arrayLength = texture_array_images_size;
    texture_descriptor.pixelFormat = MTLPixelFormatRGBA8Unorm;
    texture_descriptor.width = image_width;
    texture_descriptor.height = image_height;
    
    MTLRegion region = {
        { 0,0,0 },
        { image_width, image_height, 1 }
    };
    
    [[_metal_textures objectAtIndex: (NSUInteger)texturearray_i]
        replaceRegion:
            region
        mipmapLevel:
            0
        slice:
            (NSUInteger)texture_i
        withBytes:
            rgba_values
        bytesPerRow:
            image_width * 4
        bytesPerImage:
            /* docs: use 0 for anything other than
               MTLTextureType3D textures */
            0];
}

- (void)drawClearScreen:(MTKView *)view
{
    id<MTLCommandBuffer> command_buffer =
        [command_queue commandBuffer];
    
    MTLRenderPassDescriptor * RenderPassDescriptor =
        [view currentRenderPassDescriptor];
    RenderPassDescriptor
        .colorAttachments[0]
        .loadAction = MTLLoadActionClear;
    
    MTLClearColor clear_color = MTLClearColorMake(0.0f, 0.0f, 0.0f, 1.0f);
    RenderPassDescriptor.colorAttachments[0].clearColor = clear_color;
    
    //    id<MTLRenderCommandEncoder> render_encoder =
    //        [command_buffer
    //            renderCommandEncoderWithDescriptor:
    //                RenderPassDescriptor];
    // [render_encoder setCullMode: MTLCullModeBack];
    // [render_encoder setDepthStencilState: depth_stencil_state];
    // [render_encoder endEncoding];
    
    id<CAMetalDrawable> current_drawable = [view currentDrawable];
    [command_buffer presentDrawable: current_drawable];
    [command_buffer commit];
    [command_buffer waitUntilScheduled];
}

- (void)drawInMTKView:(MTKView *)view
{
    if (already_drawing) { return; }
    already_drawing = true;
    
    Vertex * vertices_for_gpu =
        render_commands.vertex_buffers[current_frame_i].vertices;
    uint32_t vertices_for_gpu_size = 0;
    
    shared_gameloop_update(
        vertices_for_gpu,
        &vertices_for_gpu_size);
    
    id<MTLCommandBuffer> command_buffer =
        [command_queue commandBuffer];
    
    if (command_buffer == nil) {
        log_append("error - failed to get metal command buffer\n");
        log_dump_and_crash("error - failed to get metal command buffer\n");
        already_drawing = false;
        return;
    }
    
    MTLRenderPassDescriptor * RenderPassDescriptor =
        [view currentRenderPassDescriptor];
    RenderPassDescriptor
        .colorAttachments[0]
        .loadAction = MTLLoadActionClear;
    
    RenderPassDescriptor.colorAttachments[0].clearColor =
        MTLClearColorMake(0.0f, 0.0f, 0.0f, 1.0f);;
    
    id<MTLRenderCommandEncoder> render_encoder =
        [command_buffer
            renderCommandEncoderWithDescriptor:
                RenderPassDescriptor];
    [render_encoder setViewport: viewport];
    
    [render_encoder setRenderPipelineState: combo_pipeline_state];
    [render_encoder setDepthStencilState: depth_stencil_state];
    // [render_encoder setDepthClipMode: MTLDepthClipModeClip];
    
    // encode the drawing of all triangles 
    [render_encoder
        setVertexBuffer: [[self vertex_buffers]
            objectAtIndex: current_frame_i]  
        offset: 0 
        atIndex: 0];
    
    for (
        uint32_t i = 0;
        i < [_metal_textures count];
        i++)
    {
        [render_encoder
            setFragmentTexture: _metal_textures[i]
            atIndex: i];
    }
    
    [render_encoder
        drawPrimitives: MTLPrimitiveTypeTriangle
        vertexStart: 0
        vertexCount: vertices_for_gpu_size];
    [render_encoder endEncoding];
    
    
    // Schedule a present once the framebuffer is complete
    // using the current drawable
    [command_buffer presentDrawable: [view currentDrawable]];
    
    current_frame_i += 1;
    current_frame_i -= ((current_frame_i > 2)*3);
    
    [command_buffer commit];
    
    request_post_resize_clearscreen = false;
    already_drawing = false;
}

- (void)mtkView:(MTKView *)view
    drawableSizeWillChange:(CGSize)size
{
    window_height = platform_get_current_window_height(); 
    window_width = platform_get_current_window_width();
    
    viewport.originX = 0;
    viewport.originY = 0;
    viewport.width = window_width * (has_retina_screen ? 2.0f : 1.0f);
    viewport.height = window_height * (has_retina_screen ? 2.0f : 1.0f);
    viewport.znear = 0.0f;
    viewport.zfar = 1.0f;
}
@end

void platform_gpu_init_texture_array(
    const int32_t texture_array_i,
    const uint32_t num_images,
    const uint32_t single_image_width,
    const uint32_t single_image_height)
{
    log_append("platform_gpu_init_texture_array texture_array_i: ");
    log_append_int(texture_array_i);
    log_append(", num_images: ");
    log_append_uint(num_images);
    log_append(", single_image_width: ");
    log_append_uint(single_image_width);
    log_append(", single_image_height: ");
    log_append_uint(single_image_height);
    log_append("\n");
    
    log_assert(apple_gpu_delegate != NULL);
    
    [apple_gpu_delegate
        initializeTextureArray : texture_array_i
        textureCount           : num_images
        singleImgWidth         : single_image_width
        singleImgHeight        : single_image_height];    
}

void platform_gpu_push_texture_slice(
    const int32_t texture_array_i,
    const int32_t texture_i,
    const uint32_t parent_texture_array_images_size,
    const uint32_t image_width,
    const uint32_t image_height,
    const uint8_t * rgba_values)
{
    [apple_gpu_delegate
        updateTextureArray : (int32_t)texture_array_i
        atTexture          : (int32_t)texture_i
        ofTextureArraySize : (uint32_t)parent_texture_array_images_size
        withImageOfWidth   : (uint32_t)image_width
        andHeight          : (uint32_t)image_height
        pixelValues        : (uint8_t *)rgba_values];
}
