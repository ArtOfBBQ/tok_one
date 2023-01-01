#import "gpu.h"

MetalKitViewDelegate * apple_gpu_delegate = NULL;
uint32_t block_drawinmtkview = true;

// objective-c "id" of the MTLBuffer objects
static id vertex_buffers[3];
static id light_buffers [3];
static id camera_buffers[3];
static id projection_constant_buffers[3];


@implementation MetalKitViewDelegate
{
    NSUInteger current_frame_i;
    MTLViewport viewport;
    GPUSharedDataCollection gpu_shared_data_collection;
    
    id<MTLDevice> metal_device;
    id<MTLCommandQueue> command_queue;
    
    // TODO: consider adding another pipeline state
    // id<MTLRenderPipelineState> transparent_bitmap_state;
    
    // TODO: study semaphores
    dispatch_semaphore_t _frameBoundarySemaphore;
}

- (void)
    configureMetalWithDevice: (id<MTLDevice>)with_metal_device
    andPixelFormat: (MTLPixelFormat)pixel_format
    fromFolder: (NSString *)shader_lib_filepath
{
    _frameBoundarySemaphore = dispatch_semaphore_create(3);
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
        GPUDataForSingleFrame gpu_single_frame_data =
            gpu_shared_data_collection.triple_buffers[frame_i];
        gpu_single_frame_data.vertices_size = MAX_VERTICES_PER_BUFFER;
        
        uint64_t vertices_allocation_size =
            sizeof(GPU_Vertex) * gpu_single_frame_data.vertices_size;
        vertices_allocation_size += (4096 - (vertices_allocation_size % 4096));
        assert(vertices_allocation_size % 4096 == 0);
        gpu_single_frame_data.vertices =
            (GPU_Vertex *)malloc_from_unmanaged_aligned(
                vertices_allocation_size,
                4096);
        
        uint64_t lights_allocation_size = sizeof(GPU_LightCollection);
        lights_allocation_size += (4096 - (lights_allocation_size % 4096));
        assert(lights_allocation_size % 4096 == 0);
        gpu_single_frame_data.light_collection =
            (GPU_LightCollection *)malloc_from_unmanaged_aligned(
                lights_allocation_size,
                4096);
        
        uint64_t camera_allocation_size = sizeof(GPU_Camera);
        camera_allocation_size += (4096 - (camera_allocation_size % 4096));
        assert(camera_allocation_size % 4096 == 0);
        gpu_single_frame_data.camera =
            (GPU_Camera *)malloc_from_unmanaged_aligned(
                camera_allocation_size,
                4096);
        
        uint64_t projection_constants_allocation_size =
            sizeof(GPU_ProjectionConstants);
        projection_constants_allocation_size +=
            (4096 - (projection_constants_allocation_size % 4096));
        assert(projection_constants_allocation_size % 4096 == 0);
        gpu_single_frame_data.projection_constants =
            (GPU_ProjectionConstants *)malloc_from_unmanaged_aligned(
                projection_constants_allocation_size,
                4096);
        
        gpu_shared_data_collection.triple_buffers[frame_i] =
            gpu_single_frame_data;
        
        id<MTLBuffer> MTLBufferFrameVertices =
            [with_metal_device
                /* the pointer needs to be page aligned */
                    newBufferWithBytesNoCopy:
                        gpu_single_frame_data.vertices
                /* the length weirdly needs to be page aligned also */
                    length:
                        vertices_allocation_size
                    options:
                        MTLResourceStorageModeShared
                /* deallocator = nil to opt out */
                    deallocator:
                        nil];
        assert(MTLBufferFrameVertices != nil);
        assert(
            [MTLBufferFrameVertices contents] ==
                gpu_single_frame_data.vertices);
        assert(
            [MTLBufferFrameVertices contents] ==
                gpu_shared_data_collection.triple_buffers[frame_i].vertices);
        vertex_buffers[frame_i] = MTLBufferFrameVertices;
        assert(vertex_buffers[frame_i] != nil);
        
        id<MTLBuffer> MTLBufferFrameLights =
            [with_metal_device
                /* the pointer needs to be page aligned */
                    newBufferWithBytesNoCopy:
                        gpu_single_frame_data.light_collection
                /* the length weirdly needs to be page aligned also */
                    length:
                        lights_allocation_size
                    options:
                        MTLResourceStorageModeShared
                /* deallocator = nil to opt out */
                    deallocator:
                        nil];
         assert(
            [MTLBufferFrameLights contents] ==
                gpu_single_frame_data.light_collection);
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
                        gpu_single_frame_data.camera
                /* the length weirdly needs to be page aligned also */
                    length:
                        camera_allocation_size
                    options:
                        MTLResourceStorageModeShared
                /* deallocator = nil to opt out */
                    deallocator:
                        nil];
         assert([MTLBufferFrameCamera contents] == gpu_single_frame_data.camera);
         assert([MTLBufferFrameCamera contents] == gpu_shared_data_collection.triple_buffers[frame_i].camera);
         camera_buffers[frame_i] = MTLBufferFrameCamera;
         
         id<MTLBuffer> MTLBufferProjectionConstants =
         [with_metal_device
             /* the pointer needs to be page aligned */
             newBufferWithBytesNoCopy:
                 gpu_single_frame_data.projection_constants
             /* the length weirdly needs to be page aligned also */
             length:
                 projection_constants_allocation_size
             options:
                 MTLResourceStorageModeShared
             /* deallocator = nil to opt out */
             deallocator:
                 nil];
        projection_constant_buffers[frame_i] = MTLBufferProjectionConstants;
    }
    
    _metal_textures = [
        [NSMutableArray alloc]
            initWithCapacity: TEXTUREARRAYS_SIZE];
    
    viewport.originX = 0;
    viewport.originY = 0;
    viewport.width   = window_width * (has_retina_screen ? 2.0f : 1.0f);
    viewport.height  = window_height * (has_retina_screen ? 2.0f : 1.0f);
    viewport.znear   = projection_constants.near;
    // TODO: restore to pjc's value
    viewport.zfar    = 1.0f; // projection_constants.far;
    
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
    if (block_drawinmtkview) { return; }
    block_drawinmtkview = true;
    
    dispatch_semaphore_wait(
        _frameBoundarySemaphore,
        DISPATCH_TIME_FOREVER);
    
    GPU_Vertex * vertices_for_gpu =
        gpu_shared_data_collection.triple_buffers[current_frame_i].vertices;
    uint32_t vertices_for_gpu_size = 0;
    
    GPU_LightCollection * lights_for_gpu =
        gpu_shared_data_collection.
            triple_buffers[current_frame_i].
            light_collection;
    lights_for_gpu->lights_size = 0;
    
    GPU_Camera * camera_for_gpu =
        gpu_shared_data_collection.triple_buffers[current_frame_i].camera;
    
    GPU_ProjectionConstants * projection_constants_for_gpu =
        gpu_shared_data_collection.
            triple_buffers[current_frame_i]
            .projection_constants;
    
    shared_gameloop_update(
        vertices_for_gpu,
        &vertices_for_gpu_size,
        lights_for_gpu,
        camera_for_gpu,
        projection_constants_for_gpu);
    
    if (vertices_for_gpu_size < 1) {
        block_drawinmtkview = false;
        return;
    }
    
    assert(vertices_for_gpu_size < MAX_VERTICES_PER_BUFFER);
    assert(lights_for_gpu->light_x[0] == lights_for_gpu->light_x[0]);
    assert(lights_for_gpu->light_y[0] == lights_for_gpu->light_y[0]);
    assert(lights_for_gpu->light_z[0] == lights_for_gpu->light_z[0]);
    assert(camera_for_gpu->x_angle == camera_for_gpu->x_angle);
    assert(camera_for_gpu->y_angle == camera_for_gpu->y_angle);
    assert(camera_for_gpu->z_angle == camera_for_gpu->z_angle);
    assert(projection_constants_for_gpu->x_multiplier > 0.0f);
    assert(projection_constants_for_gpu->y_multiplier > 0.0f);
    assert(projection_constants_for_gpu->q > 0.0f);
    
    // TODO: remove debug code checking for NaN
    for (uint32_t i = 0; i < vertices_for_gpu_size; i++) {
        assert(vertices_for_gpu[i].x == vertices_for_gpu[i].x);
        assert(vertices_for_gpu[i].parent_x == vertices_for_gpu[i].parent_x);
    }
    
    id<MTLCommandBuffer> command_buffer =
        [command_queue commandBuffer];
    
    if (command_buffer == nil) {
        log_append("error - failed to get metal command buffer\n");
        log_dump_and_crash("error - failed to get metal command buffer\n");
        block_drawinmtkview = false;
        return;
    }
    
    MTLRenderPassDescriptor * RenderPassDescriptor =
        [view currentRenderPassDescriptor];
    RenderPassDescriptor.
        depthAttachment.
        loadAction = MTLLoadActionClear;
    // this inherits from the view's cleardepth (in macos/main.m for mac os),
    // don't set it here
    assert(RenderPassDescriptor.depthAttachment.clearDepth == 1.0f);
    
    RenderPassDescriptor
        .colorAttachments[0]
        .loadAction = MTLLoadActionClear;
    RenderPassDescriptor.colorAttachments[0].clearColor =
        MTLClearColorMake(0.0f, 0.03f, 0.03f, 1.0f);;
    
    id<MTLRenderCommandEncoder> render_encoder =
        [command_buffer
            renderCommandEncoderWithDescriptor:
                RenderPassDescriptor];
    assert(viewport.zfar >= viewport.znear);
    [render_encoder setViewport: viewport];
    
    [render_encoder setRenderPipelineState: _combo_pipeline_state];
    assert(_depth_stencil_state != nil);
    [render_encoder setDepthStencilState: _depth_stencil_state];
    // [render_encoder setDepthClipMode: MTLDepthClipModeClip];
    
    id<MTLBuffer> vertex_buffer_for_this_frame =
        vertex_buffers[current_frame_i];
    assert([vertex_buffer_for_this_frame contents] ==
        gpu_shared_data_collection.triple_buffers[current_frame_i].vertices);
    [render_encoder
        setVertexBuffer:
            vertex_buffer_for_this_frame
        offset:
            0 
        atIndex:
            0];
    
    id<MTLBuffer> light_buffer_for_this_Frame = light_buffers[current_frame_i];
    assert([light_buffer_for_this_Frame contents] ==
        gpu_shared_data_collection.
            triple_buffers[current_frame_i].
            light_collection);
    [render_encoder
        setVertexBuffer:
            light_buffers[current_frame_i]
        offset: 0
        atIndex: 1];
    
    id<MTLBuffer> camera_buffer_for_this_frame =
        camera_buffers[current_frame_i];
    assert([camera_buffer_for_this_frame contents] ==
        gpu_shared_data_collection.triple_buffers[current_frame_i].camera);
    [render_encoder
        setVertexBuffer:
            camera_buffers[current_frame_i]
        offset: 0
        atIndex: 2];
    
    id<MTLBuffer> projection_constants_for_this_frame =
        projection_constant_buffers[current_frame_i];
    [render_encoder
        setVertexBuffer:
            projection_constant_buffers[current_frame_i]
        offset: 0
        atIndex: 3];
    
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
    
    __block dispatch_semaphore_t semaphore = _frameBoundarySemaphore;
    [command_buffer
        addCompletedHandler:^(id<MTLCommandBuffer> commandBuffer)
    {
        dispatch_semaphore_signal(semaphore);
    }];
    
    [command_buffer commit];
    
    request_post_resize_clearscreen = false;
    block_drawinmtkview = false;
}

- (void)mtkView:(MTKView *)view
    drawableSizeWillChange:(CGSize)size
{    
    viewport.originX = 0.0f;
    viewport.originY = 0.0f;
    viewport.width   = window_width * (has_retina_screen ? 2.0f : 1.0f);
    viewport.height  = window_height * (has_retina_screen ? 2.0f : 1.0f);
    viewport.znear   = projection_constants.near;
    viewport.zfar    = projection_constants.far;
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
