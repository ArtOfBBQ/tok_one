#import "gpu.h"

MetalKitViewDelegate * apple_gpu_delegate = NULL;
GPUSharedDataCollection gpu_shared_data_collection;

// objective-c "id" of the MTLBuffer objects
static id polygon_buffers[3];
static id light_buffers [3];
static id vertex_buffers[3];
static id camera_buffers[3];
static id locked_vertex_populator_buffer;
static id locked_vertex_buffer;
static id projection_constants_buffer;

// static id projection_constant_buffers[3];
static dispatch_semaphore_t drawing_semaphore;

@implementation MetalKitViewDelegate
{
    NSUInteger current_frame_i;
    MTLViewport cached_viewport;
    
    id<MTLDevice> metal_device;
    id<MTLCommandQueue> command_queue;
}

- (void) copyLockedVertices
{
    for (uint32_t i = 0; i < ALL_LOCKED_VERTICES_SIZE; i++) {
        gpu_shared_data_collection.locked_vertices[i].xyz[0] =
            all_mesh_vertices[i].gpu_data.xyz[0];
        gpu_shared_data_collection.locked_vertices[i].xyz[1] =
            all_mesh_vertices[i].gpu_data.xyz[1];
        gpu_shared_data_collection.locked_vertices[i].xyz[2] =
            all_mesh_vertices[i].gpu_data.xyz[2];
        gpu_shared_data_collection.locked_vertices[i].normal_xyz[0] =
            all_mesh_vertices[i].gpu_data.normal_xyz[0];
        gpu_shared_data_collection.locked_vertices[i].normal_xyz[1] =
            all_mesh_vertices[i].gpu_data.normal_xyz[1];
        gpu_shared_data_collection.locked_vertices[i].normal_xyz[2] =
            all_mesh_vertices[i].gpu_data.normal_xyz[2];
        gpu_shared_data_collection.locked_vertices[i].uv[0] =
            all_mesh_vertices[i].gpu_data.uv[0];
        gpu_shared_data_collection.locked_vertices[i].uv[1] =
            all_mesh_vertices[i].gpu_data.uv[1];
    }
    
    gpu_shared_data_collection.locked_vertices_size =
        all_mesh_vertices_size;
    
    printf(
        "copied %u locked vertices to gpu\n",
        gpu_shared_data_collection.locked_vertices_size);
    
    // Create a command buffer for GPU work.
    id <MTLCommandBuffer> commandBuffer = [command_queue commandBuffer];
    
    // Encode a blit pass to copy data from the source buffer to the private buffer.
    id <MTLBlitCommandEncoder> blitCommandEncoder =
        [commandBuffer blitCommandEncoder];
    [blitCommandEncoder
        copyFromBuffer:locked_vertex_populator_buffer
        sourceOffset:0
        toBuffer:locked_vertex_buffer
        destinationOffset:0
        size:gpu_shared_data_collection.locked_vertices_allocation_size];
    [blitCommandEncoder endEncoding];
    
    // Add a completion handler and commit the command buffer.
    [commandBuffer addCompletedHandler:^(id<MTLCommandBuffer> cb) {
        // Populate private buffer.
    }];
    [commandBuffer commit];
}

- (void) updateViewport
{
    cached_viewport.originX = 0;
    cached_viewport.originY = 0;
    cached_viewport.width   =
        window_globals->window_width *
        (has_retina_screen ? 2.0f : 1.0f);
    cached_viewport.height  =
        window_globals->window_height *
        (has_retina_screen ? 2.0f : 1.0f);
    assert(cached_viewport.width > 0.0f);
    assert(cached_viewport.height > 0.0f);
    
    /*
    These near/far values are the final viewport coordinates (after fragment
    shader), not to be confused with window_globals->projection_constants.near
    that's in our world space and much larger numbers
    */
    cached_viewport.znear   = 0.001f; 
    cached_viewport.zfar    = 1.0f;
    
    *gpu_shared_data_collection.locked_pjc =
        window_globals->projection_constants;
}

- (void)
    configureMetalWithDevice: (id<MTLDevice>)with_metal_device
    andPixelFormat: (MTLPixelFormat)pixel_format
    fromFolder: (NSString *)shader_lib_filepath
{
    current_frame_i = 0;
    
    drawing_semaphore = dispatch_semaphore_create(/* initial value: */ 3);
    
    metal_device = with_metal_device;
    
    NSError *Error = NULL;
    id<MTLLibrary> shader_library = [with_metal_device newDefaultLibrary];
    
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
    //    [combo_pipeline_descriptor
    //        .colorAttachments[0]
    //        setBlendingEnabled: YES];
    //    combo_pipeline_descriptor
    //        .colorAttachments[0].sourceRGBBlendFactor =
    //            MTLBlendFactorSourceAlpha;
    //    combo_pipeline_descriptor
    //        .colorAttachments[0].destinationRGBBlendFactor =
    //            MTLBlendFactorOneMinusSourceAlpha;
    
    // note: this must be the same as the MTKView's pixel format 
    combo_pipeline_descriptor.depthAttachmentPixelFormat =
        MTLPixelFormatDepth32Float;
    
    _combo_pipeline_state =
        [with_metal_device
            newRenderPipelineStateWithDescriptor:
                combo_pipeline_descriptor 
            error:
                &Error];
    
    MTLDepthStencilDescriptor * depth_descriptor =
        [MTLDepthStencilDescriptor new];
    depth_descriptor.depthWriteEnabled = YES;
    [depth_descriptor setDepthCompareFunction:MTLCompareFunctionLessEqual];
    // [depth_descriptor setDepthCompareFunction:MTLCompareFunctionAlways];
    assert(depth_descriptor.depthWriteEnabled == true);
    _depth_stencil_state = [with_metal_device
        newDepthStencilStateWithDescriptor:depth_descriptor];
    
    if (Error != NULL)
    {
        NSLog(@" error => %@ ", [Error userInfo]);
        [NSException
            raise: @"Can't Setup Metal" 
            format: @"Unable to setup rendering pipeline state"];
    }
    
    // TODO: use the apple-approved page size constant instead of
    // hardcoding 4096, and verify it returns 4096
    for (
        uint32_t frame_i = 0;
        frame_i < 3;
        frame_i++)
    {
        id<MTLBuffer> MTLBufferFramePolygons =
            [with_metal_device
                /* the pointer needs to be page aligned */
                    newBufferWithBytesNoCopy:
                        gpu_shared_data_collection.
                            triple_buffers[frame_i].polygon_collection
                /* the length weirdly needs to be page aligned also */
                    length:
                        gpu_shared_data_collection.polygons_allocation_size
                    options:
                        MTLResourceStorageModeShared
                /* deallocator = nil to opt out */
                    deallocator:
                        nil];
        assert(MTLBufferFramePolygons != nil);
        assert(
            [MTLBufferFramePolygons contents] ==
                gpu_shared_data_collection.triple_buffers[frame_i].polygon_collection);
        polygon_buffers[frame_i] = MTLBufferFramePolygons;
        assert(polygon_buffers[frame_i] != nil);
        
        id<MTLBuffer> MTLBufferFrameVertices =
            [with_metal_device
                /* the pointer needs to be page aligned */
                    newBufferWithBytesNoCopy:
                        gpu_shared_data_collection.
                            triple_buffers[frame_i].vertices
                /* the length weirdly needs to be page aligned also */
                    length:
                        gpu_shared_data_collection.vertices_allocation_size
                    options:
                        MTLResourceStorageModeShared
                /* deallocator = nil to opt out */
                    deallocator:
                        nil];
        assert(MTLBufferFrameVertices != nil);
        assert(
            [MTLBufferFrameVertices contents] ==
                gpu_shared_data_collection.triple_buffers[frame_i].vertices);
        vertex_buffers[frame_i] = MTLBufferFrameVertices;
        assert(vertex_buffers[frame_i] != nil);
        
        id<MTLBuffer> MTLBufferFrameLights =
            [with_metal_device
                /* the pointer needs to be page aligned */
                    newBufferWithBytesNoCopy:
                        gpu_shared_data_collection.
                            triple_buffers[frame_i].light_collection
                /* the length weirdly needs to be page aligned also */
                    length:
                        gpu_shared_data_collection.lights_allocation_size
                    options:
                        MTLResourceStorageModeShared
                /* deallocator = nil to opt out */
                    deallocator:
                        nil];
        assert(
            [MTLBufferFrameLights contents] ==
                gpu_shared_data_collection.
                    triple_buffers[frame_i].
                    light_collection);
        light_buffers[frame_i] = MTLBufferFrameLights;
         
        id<MTLBuffer> MTLBufferFrameCamera =
            [with_metal_device
                /* the pointer needs to be page aligned */
                    newBufferWithBytesNoCopy:
                        gpu_shared_data_collection.
                            triple_buffers[frame_i].camera
                /* the length weirdly needs to be page aligned also */
                    length:
                        gpu_shared_data_collection.camera_allocation_size
                    options:
                        MTLResourceStorageModeShared
                /* deallocator = nil to opt out */
                    deallocator:
                        nil];
        assert(
            [MTLBufferFrameCamera contents] ==
                gpu_shared_data_collection.triple_buffers[frame_i].camera);
        camera_buffers[frame_i] = MTLBufferFrameCamera;
    }
    
    id<MTLBuffer> MTLBufferLockedVerticesPopulator =
        [with_metal_device
            /* the pointer needs to be page aligned */
                newBufferWithBytesNoCopy:
                    gpu_shared_data_collection.locked_vertices
            /* the length weirdly needs to be page aligned also */
                length:
                    gpu_shared_data_collection.locked_vertices_allocation_size
                options:
                    MTLResourceStorageModeShared
            /* deallocator = nil to opt out */
                deallocator:
                    nil];
    assert(
        [MTLBufferLockedVerticesPopulator contents] ==
            gpu_shared_data_collection.locked_vertices);
    locked_vertex_populator_buffer = MTLBufferLockedVerticesPopulator;
    
    id<MTLBuffer> MTLBufferLockedVertices =
        [with_metal_device
            newBufferWithLength:
                gpu_shared_data_collection.locked_vertices_allocation_size
            options:
                MTLResourceStorageModePrivate];
    locked_vertex_buffer = MTLBufferLockedVertices;
    
    id<MTLBuffer> MTLBufferProjectionConstants =
        [with_metal_device
            /* the pointer needs to be page aligned */
                 newBufferWithBytesNoCopy:
                     gpu_shared_data_collection.locked_pjc
            /* the length weirdly needs to be page aligned also */
                length:
                    gpu_shared_data_collection.
                        projection_constants_allocation_size
                options:
                    MTLResourceStorageModeShared
            /* deallocator = nil to opt out */
                deallocator:
                    nil];
    assert(
        [MTLBufferProjectionConstants contents] ==
            gpu_shared_data_collection.locked_pjc);
    projection_constants_buffer = MTLBufferProjectionConstants;
    
    
    _metal_textures = [
        [NSMutableArray alloc]
            initWithCapacity: TEXTUREARRAYS_SIZE];
    
    [self updateViewport];
    
    command_queue = [with_metal_device newCommandQueue];
    
    log_append("finished configureMetalWithDevice\n");
}

- (void)
    initializeTextureArray  : ( int32_t)texturearray_i
    textureCount            : (uint32_t)texture_count
    singleImgWidth          : (uint32_t)single_img_width
    singleImgHeight         : (uint32_t)single_img_height
{
    assert(texturearray_i < 31);
    
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
    
    id<MTLTexture> texture = [metal_device
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
    
    [[_metal_textures
        objectAtIndex:
            (NSUInteger)texturearray_i]
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

- (void)drawInMTKView:(MTKView *)view
{
    dispatch_semaphore_wait(
        /* dispatch_semaphore_t _Nonnull dsema: */ drawing_semaphore,
        /* dispatch_time_t timeout: */ DISPATCH_TIME_FOREVER);
    
    shared_gameloop_update(
        &gpu_shared_data_collection.triple_buffers[current_frame_i]);
    
    printf("metal code receiving: %u vertices in %u polygons, locked vertices: %u\n",
        gpu_shared_data_collection.triple_buffers[current_frame_i].
            vertices_size,
        gpu_shared_data_collection.triple_buffers[current_frame_i].
            polygon_collection->size,
        gpu_shared_data_collection.locked_vertices_size);
        
    id<MTLCommandBuffer> command_buffer = [command_queue commandBuffer];
    
    if (command_buffer == nil) {
        log_append("error - failed to get metal command buffer\n");
        #ifndef LOGGER_IGNORE_ASSERTS
        log_dump_and_crash("error - failed to get metal command buffer\n");
        #endif
        return;
    }
    
    MTLRenderPassDescriptor * RenderPassDescriptor =
        [view currentRenderPassDescriptor];
    
    RenderPassDescriptor.
        depthAttachment.
        loadAction = MTLLoadActionClear;
    
    // this inherits from the view's cleardepth (in macos/main.m for mac os),
    // don't set it here
    // assert(RenderPassDescriptor.depthAttachment.clearDepth == CLEARDEPTH);
    
    RenderPassDescriptor
        .colorAttachments[0]
        .loadAction = MTLLoadActionClear;
    
    RenderPassDescriptor.colorAttachments[0].clearColor =
        MTLClearColorMake(0.0f, 0.03f, 0.15f, 1.0f);;
    
    id<MTLRenderCommandEncoder> render_encoder =
        [command_buffer
            renderCommandEncoderWithDescriptor:
                RenderPassDescriptor];
    
    assert(cached_viewport.zfar > cached_viewport.znear);
    [render_encoder setViewport: cached_viewport];
    assert(cached_viewport.width > 0.0f);
    assert(cached_viewport.height > 0.0f);
    
    [render_encoder setRenderPipelineState: _combo_pipeline_state];
    assert(_depth_stencil_state != nil);
    [render_encoder setDepthStencilState: _depth_stencil_state];
    [render_encoder setDepthClipMode: MTLDepthClipModeClip];
    
    [render_encoder
        setVertexBuffer:
            vertex_buffers[current_frame_i]
        offset:
            0
        atIndex:
            0];
    
    [render_encoder
        setVertexBuffer:
            polygon_buffers[current_frame_i]
        offset:
            0
        atIndex:
            1];
    
    [render_encoder
        setVertexBuffer:
            light_buffers[current_frame_i]
        offset: 0
        atIndex: 2];
    
    [render_encoder
        setVertexBuffer:
            camera_buffers[current_frame_i]
        offset: 0
        atIndex: 3];
    
    [render_encoder
        setVertexBuffer:
            locked_vertex_buffer
        offset:
            0 
        atIndex:
            4];
        
    [render_encoder
        setVertexBuffer:
            projection_constants_buffer
        offset:
            0
        atIndex:
            5];
    
    for (
        uint32_t i = 0;
        i < [_metal_textures count];
        i++)
    {
        [render_encoder
            setFragmentTexture: _metal_textures[i]
            atIndex: i];
    }
    
    #ifndef IGNORE_LOGGER_ASSERTS
    for (
        uint32_t i = 0;
        i < gpu_shared_data_collection.
            triple_buffers[current_frame_i].
            vertices_size;
        i++)
    {
        log_assert(
            gpu_shared_data_collection.
                triple_buffers[current_frame_i].
                vertices[i].locked_vertex_i < 5000);
    }
    #endif
    
    [render_encoder
        drawPrimitives: MTLPrimitiveTypeTriangle
        vertexStart: 0
        vertexCount: gpu_shared_data_collection.
            triple_buffers[current_frame_i].vertices_size];
    
    [render_encoder endEncoding];
    
    // Schedule a present once the framebuffer is complete
    // using the current drawable
    [command_buffer presentDrawable: [view currentDrawable]];
    
    current_frame_i += 1;
    current_frame_i -= ((current_frame_i > 2)*3);
    
    [command_buffer addCompletedHandler:^(id<MTLCommandBuffer> arg_cmd_buffer) {
        (void)arg_cmd_buffer;
        
        dispatch_semaphore_signal(drawing_semaphore);
    }];
    
    [command_buffer commit];    
}

- (void)mtkView:(MTKView *)view
    drawableSizeWillChange:(CGSize)size
{
    //[self updateViewport];
}
@end

void platform_gpu_update_viewport(void)
{
    [apple_gpu_delegate updateViewport];
}

void platform_gpu_copy_locked_vertices(void)
{
    [apple_gpu_delegate copyLockedVertices];
}

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
