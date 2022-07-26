#import "gpu.h"

MetalKitViewDelegate * apple_gpu_delegate = NULL;

uint64_t previous_time;
static uint32_t already_drawing = false;

@implementation MetalKitViewDelegate
{
    NSUInteger _currentFrameIndex;
}

- (void)
    configureMetalWithDevice: (id<MTLDevice>)metal_device
    andPixelFormat: (MTLPixelFormat)pixel_format
    fromFolder: (NSString *)shader_lib_filepath
{
    previous_time = platform_get_current_time_microsecs();
    
    _currentFrameIndex = 0;
    
    _metal_device = metal_device;
    _command_queue = [metal_device newCommandQueue];
    
    NSError *Error = NULL;
    id<MTLLibrary> shader_library =
        [metal_device newDefaultLibrary];
    
    if (shader_library == NULL)
    {
        log_append("failed to load default shader lib, trying ");
        log_append(
            [shader_lib_filepath
                cStringUsingEncoding: NSASCIIStringEncoding]);
        log_append("\n");
        
        shader_library =
            [metal_device
                newLibraryWithFile: shader_lib_filepath 
                error: &Error];
        
        if (shader_library == NULL) {
            log_append("Failed to find the shader library again\n");
            if (Error != NULL) {
                NSLog(@" error => %@ ", [Error userInfo]);
            }
            log_dump_and_crash();
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
    // TODO: set pixel format
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
    
    _combo_pipeline_state =
        [metal_device
            newRenderPipelineStateWithDescriptor:
                combo_pipeline_descriptor 
            error:
                &Error];
    
    if (Error != NULL)
    {
        NSLog(@" error => %@ ", [Error userInfo]);
        [NSException
            raise: @"Can't Setup Metal" 
            format: @"Unable to setup rendering pipeline state"];
    }
    
    int32_t page_size = getpagesize();
    uint32_t buffered_vertex_size = (uint32_t)page_size * 5000;
    
    _vertex_buffers = [[NSMutableArray alloc] init];
    
    for (
        uint32_t frame_i = 0;
        frame_i < 3;
        frame_i++)
    {
        BufferedVertexCollection buffered_vertex = {};
        buffered_vertex.vertices =
            (Vertex *)mmap(
                0,
                buffered_vertex_size,
                PROT_READ | PROT_WRITE,
                MAP_PRIVATE | MAP_ANON,
                -1,
                0);
        
        _render_commands.vertex_buffers[frame_i] =
            buffered_vertex;
        
        id<MTLBuffer> MetalBufferedVertex =
            [metal_device
                newBufferWithBytesNoCopy:
                    buffered_vertex.vertices
                length:
                    buffered_vertex_size
                options:
                    MTLResourceStorageModeShared
                deallocator:
                    nil];
        [_vertex_buffers
            addObject: MetalBufferedVertex];
    }
    
    _metal_textures = [
        [NSMutableArray alloc]
            initWithCapacity: TEXTUREARRAYS_SIZE];
        
    log_append("finished configureMetalWithDevice\n");
}

- (void)
    initializeTextureArray:(int32_t)texturearray_i
    textureCount:(uint32_t)texture_count
    singleImgWidth: (uint32_t)single_img_width
    singleImgHeight: (uint32_t)single_img_height;
{
    log_assert(single_img_width > 0);
    log_assert(single_img_height > 0);
    
    texture_arrays[texturearray_i].request_init = false;
    
    if (texturearray_i >= TEXTUREARRAYS_SIZE) { return; }
    if (texturearray_i >= (int32_t)texture_arrays_size) { return; }
    int32_t i = texturearray_i;
    
    // we always overwrite textures, so pad them to match first
    while ((int32_t)[_metal_textures count] <= i) {
        MTLTextureDescriptor * texture_descriptor =
            [[MTLTextureDescriptor alloc] init];
        texture_descriptor.textureType = MTLTextureType2DArray;
        texture_descriptor.arrayLength = 1;
        texture_descriptor.pixelFormat = MTLPixelFormatRGBA8Unorm;
        texture_descriptor.width = 10;
        texture_descriptor.height = 10;
        id<MTLTexture> texture =
            [_metal_device
                newTextureWithDescriptor:texture_descriptor];
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
        [_metal_device
            newTextureWithDescriptor:texture_descriptor];
    
    [_metal_textures
        replaceObjectAtIndex:(uint32_t)i
        withObject: texture];
}

- (void)
    updateTextureArray: (int32_t)texturearray_i
    atTexture: (int32_t)texture_i;
{    
    log_assert(texturearray_i < TEXTUREARRAYS_SIZE);
    log_assert(texturearray_i < (int32_t)texture_arrays_size);
    log_assert(texture_i >= 0);
    log_assert(texture_i < texture_arrays[texturearray_i].images_size);
    log_assert(texture_arrays[texturearray_i].single_img_width > 0);
    log_assert(texture_arrays[texturearray_i].single_img_height > 0);
    
    if (texture_arrays[texturearray_i].request_init
        || texture_arrays[texturearray_i].images_size == 0
        || texture_arrays[texturearray_i].images[texture_i].image == NULL
        || texture_arrays[texturearray_i].images[texture_i].image
            ->rgba_values == NULL)
    {
        return;
    }
    
    if (!texture_arrays[texturearray_i].images[texture_i].image->good)
    {
        return;
    }
    
    if (texturearray_i >= [_metal_textures count]) {
        log_append("Warning: tried to update uninitialized texturearray ");
        log_append_int(texturearray_i);
        log_append(" at texture_i: ");
        log_append_int(texture_i);
        log_append("\n");
        return;
    }
    
    texture_arrays[texturearray_i].images[texture_i].request_update = false;
    
    MTLTextureDescriptor * texture_descriptor =
        [[MTLTextureDescriptor alloc] init];
    texture_descriptor.textureType = MTLTextureType2DArray;
    texture_descriptor.arrayLength = texture_arrays[texturearray_i].images_size;
    texture_descriptor.pixelFormat = MTLPixelFormatRGBA8Unorm;
    texture_descriptor.width =
        texture_arrays[texturearray_i].images[texture_i].image->width;
    texture_descriptor.height =
        texture_arrays[texturearray_i].images[texture_i].image->height;
    
    DecodedImage * new_slice =
        texture_arrays[texturearray_i].images[texture_i].image;
    
    MTLRegion region = {
        {
            0,
            0,
            0
        },
        {
            new_slice->width,
            new_slice->height,
            1
        }
    };
    
    [[_metal_textures objectAtIndex:texturearray_i]
        replaceRegion:
            region
        mipmapLevel:
            0
        slice:
            texture_i
        withBytes:
            new_slice->rgba_values
        bytesPerRow:
            new_slice->width * 4
        bytesPerImage:
            /* docs: use 0 for anything other than
               MTLTextureType3D textures */
            0];    
}

- (void)drawInMTKView:(MTKView *)view
{
    log_assert(!already_drawing);
    uint64_t time = platform_get_current_time_microsecs();
    uint64_t elapsed = time - previous_time;
    previous_time = time;
    
    if (
        time - last_resize_request_at < 1500000
        && !request_post_resize_clearscreen)
    {
        if (time - last_resize_request_at < 300000) {
            return;
        }
        
        client_logic_window_resize(
            window_height,
            window_width);
        last_resize_request_at = 999999999;
    }
    
    already_drawing = true;
    
    for (uint32_t i = 0; i < texture_arrays_size; i++) {
        if (texture_arrays[i].request_init) {
            [self
                initializeTextureArray: (int32_t)i
                textureCount: texture_arrays[i].images_size
                singleImgWidth: texture_arrays[i].single_img_width
                singleImgHeight: texture_arrays[i].single_img_height];
            break;
        } else {
            for (uint32_t j = 0; j < texture_arrays[i].images_size; j++) {
                if (texture_arrays[i].images[j].request_update) {
                    [self
                        updateTextureArray: i
                        atTexture: j];
                    break;
                }
            }
        }
    }
    
    MTLViewport viewport = {
        0,
        0,
        window_width * (has_retina_screen ? 2.0f : 1.0f),
        window_height * (has_retina_screen ? 2.0f : 1.0f)
    };
    
    uint32_t frame_i = (uint32_t)_currentFrameIndex;
    
    Vertex * vertices_for_gpu =
        _render_commands.vertex_buffers[frame_i].vertices;
    uint32_t vertices_for_gpu_size = 0;
    
    resolve_animation_effects(elapsed);
    
    touchable_triangles_size = 0;
    
    update_camera_position();
    
    clean_deleted_lights();
    clean_deleted_texquads();
    
    // translate all lights
    translate_lights();
    
    client_logic_update(elapsed);
    
    software_render(
        /* next_gpu_workload: */
            vertices_for_gpu,
        /* next_gpu_workload_size: */
            &vertices_for_gpu_size,
        /* zlights_transformed: */
            zlights_transformed,
        /* elapsed_microseconds: */
            elapsed);
    
    draw_texquads_to_render(
        /* next_gpu_workload: */
            vertices_for_gpu,
        /* next_gpu_workload_size: */
            &vertices_for_gpu_size,
        /* zlights_transformed: */
            zlights_transformed);
    
    id<MTLCommandBuffer> command_buffer =
        [[self command_queue] commandBuffer];
    
    if (command_buffer == nil) {
        log_append("error - failed to get metal command buffer\n");
        log_dump_and_crash();
        already_drawing = false;
        return;
    }
    
    MTLRenderPassDescriptor * RenderPassDescriptor =
        [view currentRenderPassDescriptor];
    RenderPassDescriptor
        .colorAttachments[0]
        .loadAction = MTLLoadActionClear;
    
    MTLClearColor clear_color =
        MTLClearColorMake(0.0f, 0.0f, 0.0f, 0.0f);
    RenderPassDescriptor.colorAttachments[0].clearColor =
        clear_color;
    
    id<MTLRenderCommandEncoder> render_encoder =
        [command_buffer
            renderCommandEncoderWithDescriptor:
                RenderPassDescriptor];
    [render_encoder setViewport: viewport];
    
    // encode the drawing of all triangles 
    id<MTLBuffer> current_buffered_vertices =
        [[self vertex_buffers]
            objectAtIndex: _currentFrameIndex];
    [render_encoder
        setVertexBuffer: current_buffered_vertices  
        offset: 0 
        atIndex: 0];
    [render_encoder
        setRenderPipelineState:
            [self combo_pipeline_state]];
    
    for (
        uint32_t i = 0;
        i < texture_arrays_size;
        i++)
    {
        if (i >= [_metal_textures count]) {
            assert(texture_arrays_size < 750);
            continue;
        }
        
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
    id<CAMetalDrawable> current_drawable =
        [view currentDrawable];
    [command_buffer presentDrawable: current_drawable];
    
    uint32_t next_index = (uint32_t)_currentFrameIndex + 1;
    if (next_index > 2) { next_index = 0; }
    
    _currentFrameIndex = next_index;
    
    [command_buffer
        addCompletedHandler:
            ^(id<MTLCommandBuffer> commandBuffer)
        {
        }];
    
    [command_buffer commit];
    
    request_post_resize_clearscreen = false;
    already_drawing = false;
}

- (void)mtkView:(MTKView *)view
    drawableSizeWillChange:(CGSize)size
{
    window_height = platform_get_current_window_height(); 
    window_width = platform_get_current_window_width();
}

@end
