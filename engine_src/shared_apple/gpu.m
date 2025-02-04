#import "gpu.h"

bool32_t has_retina_screen = false;
bool32_t metal_active = false;

static MTLPixelFormat cached_pixel_format;

MetalKitViewDelegate * apple_gpu_delegate = NULL;

static void (* funcptr_shared_gameloop_update)(GPUDataForSingleFrame *) = NULL;

static dispatch_semaphore_t drawing_semaphore;

void apple_gpu_init(
    void (* arg_funcptr_shared_gameloop_update)(GPUDataForSingleFrame *))
{
    //    RenderPassDescriptors[0] = NULL;
    //    RenderPassDescriptors[1] = NULL;
    //    RenderPassDescriptors[2] = NULL;
    
    funcptr_shared_gameloop_update = arg_funcptr_shared_gameloop_update;
    
    drawing_semaphore = dispatch_semaphore_create(3);
}

// objective-c "id" of the MTLBuffer objects
static id polygon_buffers[3];
static id polygon_material_buffers[3];
static id light_buffers [3];
static id vertex_buffers[3];
static id camera_buffers[3];
static id line_vertex_buffers[3];
static id point_vertex_buffers[3];
static id postprocessing_constants_buffers[3];
static id locked_vertex_populator_buffer;
static id locked_vertex_buffer;
static id projection_constants_buffer;

@implementation MetalKitViewDelegate
{
    NSUInteger current_frame_i;
    MTLViewport cached_viewport;
    
    id<MTLDevice> metal_device;
    id<MTLCommandQueue> command_queue;
    #if POSTPROCESSING_ACTIVE
    id<MTLTexture> render_target_texture;
    #endif
}

- (void) copyLockedVertices
{
    gpu_shared_data_collection.locked_vertices_size = all_mesh_vertices->size;
    
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
        (void)cb;
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
    These near/far values are the final viewport coordinates (after
    fragment shader), not to be confused with
    window_globals->projection_constants.near that's in our world space
    and much larger numbers
    */
    cached_viewport.znear   = 0.001f; 
    cached_viewport.zfar    = 1.0f;
    
    *gpu_shared_data_collection.locked_pjc =
        window_globals->projection_constants;
    
    #if POSTPROCESSING_ACTIVE
    // Set up a texture for rendering to and apply post-processing to
    MTLTextureDescriptor * texture_descriptor = [MTLTextureDescriptor new];
    texture_descriptor.textureType = MTLTextureType2D;
    texture_descriptor.width = (unsigned long)cached_viewport.width;
    texture_descriptor.height = (unsigned long)cached_viewport.height;
    texture_descriptor.pixelFormat = cached_pixel_format;
    texture_descriptor.usage =
        MTLTextureUsageRenderTarget |
        MTLTextureUsageShaderRead;
    
    render_target_texture = [metal_device
        newTextureWithDescriptor: texture_descriptor];
    #endif
}

- (BOOL)
    configureMetalWithDevice: (id<MTLDevice>)with_metal_device
    andPixelFormat: (MTLPixelFormat)pixel_format
    fromFilePath: (NSString *)shader_lib_filepath
    errMsgCStr: (char *)errmsg
{
    cached_pixel_format = pixel_format;
    
    current_frame_i = 0;
        
    common_strcpy_capped(
        errmsg,
        512,
        "");
    
    // drawing_semaphore = dispatch_semaphore_create(/* initial value: */ 3);
    
    metal_device = with_metal_device;
    
    NSError * Error = NULL;
    id<MTLLibrary> shader_library = [with_metal_device newDefaultLibrary];
    
    if (shader_library == NULL)
    {
        log_append("failed to load default shader lib, trying ");
        log_append(
            [shader_lib_filepath
                cStringUsingEncoding: NSASCIIStringEncoding]);
        log_append("\n");
        
        NSURL * shader_lib_url = [NSURL
            fileURLWithPath: shader_lib_filepath
            isDirectory: false];
        
        if (shader_lib_url == NULL) {
            #ifndef LOGGER_IGNORE_ASSERTS
            log_dump_and_crash("Failed to find the shader lib\n");
            #endif
            
            NSString * errorstr = [Error localizedDescription];
            
            const char * errorcstr = [errorstr
                cStringUsingEncoding: NSASCIIStringEncoding];
            
            common_strcpy_capped(
                errmsg,
                512,
                errorcstr);
            return false;
        }
        
        shader_library =
            [with_metal_device
                newLibraryWithURL: shader_lib_url
                error: &Error];
        
        if (shader_library == NULL) {
            log_append("Failed to find the shader library\n");
            #ifndef LOGGER_IGNORE_ASSERTS
            log_dump_and_crash((char *)[
                [[Error userInfo] descriptionInStringsFileFormat]
                    cStringUsingEncoding:NSASCIIStringEncoding]);
            #endif
        
            NSString * errorstr = [Error localizedDescription];
            
            const char * errorcstr = [errorstr
                cStringUsingEncoding: NSASCIIStringEncoding];
            
            if (errorcstr != NULL && errorcstr[0] != '\0') {
                common_strcpy_capped(
                    errmsg,
                    512,
                    errorcstr);
            } else {
                common_strcpy_capped(
                    errmsg,
                    512,
                    "Failed to find shaders file");
            }
            
            return false;
        } else {
            log_append(
                "Success! Found the shader lib on 2nd try.\n");
        }
    }
    
    id<MTLFunction> vertex_shader =
        [shader_library newFunctionWithName:
            @"vertex_shader"];
    if (vertex_shader == NULL) {
        log_append("Missing function: vertex_shader()!");
        
        common_strcpy_capped(
            errmsg,
            512,
            "Missing function: vertex_shader()");
        return false;
    }
    
    id<MTLFunction> fragment_shader =
        [shader_library newFunctionWithName:
            @"fragment_shader"];
    if (fragment_shader == NULL) {
        log_append("Missing function: fragment_shader()!");
        common_strcpy_capped(
            errmsg,
            512,
            "Missing function: fragment_shader()");
        return false;
    }
    
    id<MTLFunction> alphablending_fragment_shader =
        [shader_library newFunctionWithName:
            @"alphablending_fragment_shader"];
    if (alphablending_fragment_shader == NULL) {
        log_append("Missing function: alphablending_fragment_shader()!");
        common_strcpy_capped(
            errmsg,
            512,
            "Missing function: vertex_shader()");
        return false;
    }
    
    id<MTLFunction> raw_vertex_shader =
        [shader_library newFunctionWithName:
            @"raw_vertex_shader"];
    if (raw_vertex_shader == NULL) {
        log_append("Missing function: raw_vertex_shader()!");

        common_strcpy_capped(
            errmsg,
            512,
            "Missing function: raw_vertex_shader()");
        return false;
    }
    
    id<MTLFunction> raw_fragment_shader =
        [shader_library newFunctionWithName:
            @"raw_fragment_shader"];
    if (raw_fragment_shader == NULL) {
        log_append("Missing function: raw_fragment_shader()!");
        common_strcpy_capped(
            errmsg,
            512,
            "Missing function: vertex_shader()");
        return false;
    }
    
    // Setup combo pipeline that handles
    // both colored & textured triangles
    MTLRenderPipelineDescriptor * diamond_pipeline_descriptor =
        [[MTLRenderPipelineDescriptor alloc] init];
    [diamond_pipeline_descriptor
        setVertexFunction: vertex_shader];
    [diamond_pipeline_descriptor
        setFragmentFunction: fragment_shader];
    diamond_pipeline_descriptor
        .colorAttachments[0]
        .pixelFormat = pixel_format;
    diamond_pipeline_descriptor.depthAttachmentPixelFormat =
        MTLPixelFormatDepth32Float;
    _diamond_pipeline_state =
        [with_metal_device
            newRenderPipelineStateWithDescriptor:
                diamond_pipeline_descriptor 
            error:
                &Error];
    
    if (Error != NULL)
    {
        #ifndef LOGGER_IGNORE_ASSERTS
        log_dump_and_crash("Failed to initialize diamond pipeline");
        #endif
        common_strcpy_capped(
            errmsg,
            512,
            "Failed to initialize diamond pipeline");
        return false;
    }
    
    MTLRenderPipelineDescriptor * alphablend_pipeline_descriptor =
        [[MTLRenderPipelineDescriptor alloc] init];
    [alphablend_pipeline_descriptor
        setVertexFunction: vertex_shader];
    [alphablend_pipeline_descriptor
        setFragmentFunction: alphablending_fragment_shader];
    alphablend_pipeline_descriptor
        .colorAttachments[0]
        .pixelFormat = pixel_format;
    [alphablend_pipeline_descriptor
        .colorAttachments[0]
        setBlendingEnabled: YES];
    alphablend_pipeline_descriptor
        .colorAttachments[0].sourceRGBBlendFactor =
            MTLBlendFactorSourceAlpha;
    alphablend_pipeline_descriptor
        .colorAttachments[0].destinationRGBBlendFactor =
            MTLBlendFactorOneMinusSourceAlpha;
    alphablend_pipeline_descriptor.depthAttachmentPixelFormat =
        MTLPixelFormatDepth32Float;
    _alphablend_pipeline_state =
        [with_metal_device
            newRenderPipelineStateWithDescriptor:
                alphablend_pipeline_descriptor
            error:
                &Error];
    
    if (Error != NULL)
    {
        log_append([[Error localizedDescription] cStringUsingEncoding:kCFStringEncodingASCII]);
        #ifndef LOGGER_IGNORE_ASSERTS
        log_dump_and_crash("Error loading the alphablending shader\n");
        #endif
        
        common_strcpy_capped(
            errmsg,
            512,
            "Failed to load the alphablending shader");
        return false;
    }
    
    MTLRenderPipelineDescriptor * raw_pipeline_descriptor =
        [[MTLRenderPipelineDescriptor alloc] init];
    [raw_pipeline_descriptor
        setVertexFunction: raw_vertex_shader];
    [raw_pipeline_descriptor
        setFragmentFunction: raw_fragment_shader];
    raw_pipeline_descriptor
        .colorAttachments[0]
        .pixelFormat = pixel_format;
    [raw_pipeline_descriptor
        .colorAttachments[0]
        setBlendingEnabled: NO];
    raw_pipeline_descriptor.depthAttachmentPixelFormat =
        MTLPixelFormatDepth32Float;
    _raw_pipeline_state =
        [with_metal_device
            newRenderPipelineStateWithDescriptor:
                raw_pipeline_descriptor
            error:
                &Error];
    
    if (Error != NULL)
    {
        log_append([[Error localizedDescription] cStringUsingEncoding:kCFStringEncodingASCII]);
        #ifndef LOGGER_IGNORE_ASSERTS
        log_dump_and_crash("Error loading the raw vertex shader\n");
        #endif
        
        
        common_strcpy_capped(
            errmsg,
            512,
            "Failed to load the raw vertex shader");
        return false;
    }
    
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
        log_append([[Error localizedDescription] cStringUsingEncoding:kCFStringEncodingASCII]);
        #ifndef LOGGER_IGNORE_ASSERTS
        log_dump_and_crash("Error setting the depth stencil state\n");
        #endif
        
        common_strcpy_capped(
            errmsg,
            512,
            "Failed to load the depth stencil shader");
        return false;
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
                gpu_shared_data_collection.triple_buffers[frame_i].
                    polygon_collection);
        polygon_buffers[frame_i] = MTLBufferFramePolygons;
        assert(polygon_buffers[frame_i] != nil);
        
        id<MTLBuffer> MTLBufferFramePolygonMaterials =
            [with_metal_device
                /* the pointer needs to be page aligned */
                    newBufferWithBytesNoCopy:
                        gpu_shared_data_collection.
                            triple_buffers[frame_i].polygon_materials
                /* the length weirdly needs to be page aligned also */
                    length:
                        gpu_shared_data_collection.
                            polygon_materials_allocation_size
                    options:
                        MTLResourceStorageModeShared
                /* deallocator = nil to opt out */
                    deallocator:
                        nil];
        assert(MTLBufferFramePolygonMaterials != nil);
        assert(
            [MTLBufferFramePolygonMaterials contents] ==
                gpu_shared_data_collection.triple_buffers[frame_i].
                    polygon_materials);
        polygon_material_buffers[frame_i] = MTLBufferFramePolygonMaterials;
        assert(polygon_material_buffers[frame_i] != nil);
        
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
        
        id<MTLBuffer> MTLBufferLineVertices =
            [with_metal_device
                /* the pointer needs to be page aligned */
                    newBufferWithBytesNoCopy:
                        gpu_shared_data_collection.
                            triple_buffers[frame_i].line_vertices
                /* the length weirdly needs to be page aligned also */
                    length:
                        gpu_shared_data_collection.line_vertices_allocation_size
                    options:
                        MTLResourceStorageModeShared
                /* deallocator = nil to opt out */
                    deallocator:
                        nil];
        assert(MTLBufferLineVertices != nil);
        assert(
            [MTLBufferLineVertices contents] ==
                gpu_shared_data_collection.triple_buffers[frame_i].
                    line_vertices);
        line_vertex_buffers[frame_i] = MTLBufferLineVertices;
        assert(line_vertex_buffers[frame_i] != nil);
        
        id<MTLBuffer> MTLBufferPointVertices =
            [with_metal_device
                /* the pointer needs to be page aligned */
                    newBufferWithBytesNoCopy:
                        gpu_shared_data_collection.
                            triple_buffers[frame_i].point_vertices
                /* the length weirdly needs to be page aligned also */
                    length:
                        gpu_shared_data_collection.point_vertices_allocation_size
                    options:
                        MTLResourceStorageModeShared
                /* deallocator = nil to opt out */
                    deallocator:
                        nil];
        assert(MTLBufferPointVertices != nil);
        assert(
            [MTLBufferPointVertices contents] ==
                gpu_shared_data_collection.triple_buffers[frame_i].
                    point_vertices);
        point_vertex_buffers[frame_i] = MTLBufferPointVertices;
        assert(point_vertex_buffers[frame_i] != nil);
        
        id<MTLBuffer> MTLBufferPostProcessingConstants =
            [with_metal_device
                /* the pointer needs to be page aligned */
                    newBufferWithBytesNoCopy:
                        gpu_shared_data_collection.
                            triple_buffers[frame_i].postprocessing_constants
                /* the length weirdly needs to be page aligned also */
                    length:
                        gpu_shared_data_collection.
                            postprocessing_constants_allocation_size
                    options:
                        MTLResourceStorageModeShared
                /* deallocator = nil to opt out */
                    deallocator:
                        nil];
        assert(MTLBufferPostProcessingConstants != nil);
        assert(
            [MTLBufferPostProcessingConstants contents] ==
                gpu_shared_data_collection.triple_buffers[frame_i].
                    postprocessing_constants);
        postprocessing_constants_buffers[frame_i] =
            MTLBufferPostProcessingConstants;
        assert(postprocessing_constants_buffers[frame_i] != nil);
        
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
    
    #if POSTPROCESSING_ACTIVE
    MTLRenderPipelineDescriptor * postprocess_pipeline_descriptor =
        [[MTLRenderPipelineDescriptor alloc] init];
    id<MTLFunction> postprocess_vertex_shader =
        [shader_library newFunctionWithName:
            @"postprocess_vertex_shader"];
    if (postprocess_vertex_shader == NULL) {
        log_append("Missing function: postprocess_vertex_shader()!");
        
        common_strcpy_capped(
            errmsg,
            512,
            "Missing function: postprocess_vertex_shader()");
        return false;
    }
    
    id<MTLFunction> postprocess_fragment_shader =
        [shader_library newFunctionWithName: @"postprocess_fragment_shader"];
    if (postprocess_fragment_shader == NULL) {
        log_append("Missing function: postprocess_fragment_shader()!");
        common_strcpy_capped(
            errmsg,
            512,
            "Missing function: postprocess_fragment_shader()");
        return false;
    }
    
    // Set up pipeline for rendering the texture to the screen with a simple
    // quad
    postprocess_pipeline_descriptor.label = @"Postprocessing Pipeline";
    postprocess_pipeline_descriptor.sampleCount = 1;
    [postprocess_pipeline_descriptor
        setVertexFunction: postprocess_vertex_shader];
    [postprocess_pipeline_descriptor
        setFragmentFunction: postprocess_fragment_shader];
    postprocess_pipeline_descriptor.colorAttachments[0].pixelFormat =
        pixel_format;
    [postprocess_pipeline_descriptor
        .colorAttachments[0]
        setBlendingEnabled: YES];
    postprocess_pipeline_descriptor
        .colorAttachments[0].sourceRGBBlendFactor =
            MTLBlendFactorOne;
    postprocess_pipeline_descriptor
        .colorAttachments[0].destinationRGBBlendFactor =
            MTLBlendFactorOne;
    postprocess_pipeline_descriptor.depthAttachmentPixelFormat =
        MTLPixelFormatDepth32Float;
    postprocess_pipeline_descriptor.vertexBuffers[0].mutability =
        MTLMutabilityImmutable;
    _postprocess_pipeline_state = [
        with_metal_device
        newRenderPipelineStateWithDescriptor:postprocess_pipeline_descriptor
        error:NULL];
    #endif
    
    [self updateViewport];
    
    command_queue = [with_metal_device newCommandQueue];
    
    metal_active = true;
    
    return true;
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

static bool32_t font_already_pushed = 0;
- (void)
    updateTextureArray : (int32_t)texturearray_i
    atTexture          : (int32_t)texture_i
    ofTextureArraySize : (uint32_t)texture_array_images_size
    withImageOfWidth   : (uint32_t)image_width
    andHeight          : (uint32_t)image_height
    pixelValues        : (uint8_t *)rgba_values
{
    log_assert(texture_i >= 0);
    log_assert(texturearray_i >= 0);
    
    if (texturearray_i == 0 && texture_i == 0) {
        assert(!font_already_pushed);
        font_already_pushed = true;
    }
    
    if (texturearray_i >= (int32_t)[_metal_textures count]) {
        #ifndef LOGGER_IGNORE_ASSERTS
        char errmsg[256];
        common_strcpy_capped(errmsg, 256, "Tried to update uninitialized texturearray")
        common_strcat_int_capped(errmsg, 256, texturearray_i);
        common_strcat_capped(errmsg, 256, "\n");
        
        log_dump_and_crash(errmsg);
        #endif
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
    #ifdef PROFILER_ACTIVE
    profiler_new_frame();
    profiler_start("drawInMTKView");
    #endif
    
    dispatch_semaphore_wait(drawing_semaphore, DISPATCH_TIME_FOREVER);
    
    funcptr_shared_gameloop_update(
        &gpu_shared_data_collection.triple_buffers[current_frame_i]);
    
    if (!metal_active) {
        #ifdef PROFILER_ACTIVE
        profiler_end("drawInMTKView");
        #endif
        return;
    }
    
    id<MTLCommandBuffer> command_buffer = [command_queue commandBuffer];
    
    if (command_buffer == nil) {
        #ifndef LOGGER_IGNORE_ASSERTS
        log_dump_and_crash("error - failed to get metal command buffer\n");
        #endif
        
        #ifdef PROFILER_ACTIVE
        profiler_end("drawInMTKView");
        #endif
        return;
    }
    
    #ifdef PROFILER_ACTIVE
    profiler_start("Create MTLRenderPassDescriptor etc.");
    #endif
    MTLRenderPassDescriptor * render_pass_1_descriptor =
        [view currentRenderPassDescriptor];
    
    render_pass_1_descriptor.depthAttachment.loadAction =
        MTLLoadActionClear;
    
    #if POSTPROCESSING_ACTIVE
    render_pass_1_descriptor.colorAttachments[0].texture =
        render_target_texture;
    render_pass_1_descriptor.colorAttachments[0].storeAction =
        MTLStoreActionStore;
    #else
    render_pass_1_descriptor
        .colorAttachments[0]
        .loadAction = MTLLoadActionClear;
    #endif
    
    render_pass_1_descriptor.colorAttachments[0].clearColor =
        MTLClearColorMake(0.0f, 0.03f, 0.15f, 1.0f);;
    
    // this inherits from the view's cleardepth (in macos/main.m for mac os),
    // don't set it here
    // assert(RenderPassDescriptor.depthAttachment.clearDepth == CLEARDEPTH);
    #ifdef PROFILER_ACTIVE
    profiler_end("Create MTLRenderPassDescriptor etc.");
    #endif
    
    #ifdef PROFILER_ACTIVE
    profiler_start("Create MTLRenderCommandEncoder");
    #endif
    id<MTLRenderCommandEncoder> render_pass_1_encoder =
            [command_buffer
                renderCommandEncoderWithDescriptor:
                    render_pass_1_descriptor];
    #ifdef PROFILER_ACTIVE
    profiler_end("Create MTLRenderCommandEncoder");
    #endif
    
    #ifdef PROFILER_ACTIVE
    profiler_start("setViewport, RenderPipeline, Stencil, ClipMode");
    #endif
    assert(cached_viewport.zfar > cached_viewport.znear);
    [render_pass_1_encoder setViewport: cached_viewport];
    assert(cached_viewport.width > 0.0f);
    assert(cached_viewport.height > 0.0f);
    
    [render_pass_1_encoder setRenderPipelineState: _diamond_pipeline_state];
    assert(_depth_stencil_state != nil);
    [render_pass_1_encoder setDepthStencilState: _depth_stencil_state];
    [render_pass_1_encoder setDepthClipMode: MTLDepthClipModeClip];
    #ifdef PROFILER_ACTIVE
    profiler_end("setViewport, RenderPipeline, Stencil, ClipMode");
    #endif
    
    #ifdef PROFILER_ACTIVE
    profiler_start("setVertexBuffers");
    #endif
    [render_pass_1_encoder
        setVertexBuffer:
            vertex_buffers[current_frame_i]
        offset:
            0
        atIndex:
            0];
    
    [render_pass_1_encoder
        setVertexBuffer:
            polygon_buffers[current_frame_i]
        offset:
            0
        atIndex:
            1];
    
    [render_pass_1_encoder
        setVertexBuffer:
            light_buffers[current_frame_i]
        offset: 0
        atIndex: 2];
    
    [render_pass_1_encoder
        setVertexBuffer:
            camera_buffers[current_frame_i]
        offset: 0
        atIndex: 3];
    
    [render_pass_1_encoder
        setVertexBuffer:
            locked_vertex_buffer
        offset:
            0 
        atIndex:
            4];
    
    [render_pass_1_encoder
        setVertexBuffer:
            projection_constants_buffer
        offset:
            0
        atIndex:
            5];
    
    [render_pass_1_encoder
        setVertexBuffer:
            polygon_material_buffers[current_frame_i]
        offset:
            0
        atIndex:
            6];
    #ifdef PROFILER_ACTIVE
    profiler_end("setVertexBuffers");
    #endif
    
    #ifdef PROFILER_ACTIVE
    profiler_start("setFragmentTexture");
    #endif
    for (
        uint32_t i = 0;
        i < [_metal_textures count];
        i++)
    {
        [render_pass_1_encoder
            setFragmentTexture: _metal_textures[i]
            atIndex: i];
    }
    #ifdef PROFILER_ACTIVE
    profiler_end("setFragmentTexture");
    #endif
    
    #ifndef IGNORE_LOGGER_ASSERTS
    #ifdef PROFILER_ACTIVE
    profiler_start("debug-only assertions");
    #endif
    for (
        uint32_t i = 0;
        i < gpu_shared_data_collection.
            triple_buffers[current_frame_i].
            vertices_size;
        i++)
    {
        assert(
            gpu_shared_data_collection.
                triple_buffers[current_frame_i].
                vertices[i].locked_vertex_i < ALL_LOCKED_VERTICES_SIZE);
    }
    #ifdef PROFILER_ACTIVE
    profiler_end("debug-only assertions");
    #endif
    #endif
    
    #ifdef PROFILER_ACTIVE
    profiler_start("draw calls");
    #endif
    uint32_t diamond_verts_size =
        gpu_shared_data_collection.
                triple_buffers[current_frame_i].first_alphablend_i;
    
    if (window_globals->draw_triangles && diamond_verts_size > 0) {
        assert(diamond_verts_size < MAX_VERTICES_PER_BUFFER);
        assert(diamond_verts_size % 3 == 0);
        [render_pass_1_encoder
            drawPrimitives:
                MTLPrimitiveTypeTriangle
            vertexStart:
                0
            vertexCount:
                diamond_verts_size];
    }
    
    log_assert(
        gpu_shared_data_collection.triple_buffers[current_frame_i].
            first_alphablend_i <=
        gpu_shared_data_collection.triple_buffers[current_frame_i].
            vertices_size);
    uint32_t alphablend_verts_size =
        gpu_shared_data_collection.
            triple_buffers[current_frame_i].vertices_size -
        gpu_shared_data_collection.
            triple_buffers[current_frame_i].first_alphablend_i;
    
    if (window_globals->draw_triangles && alphablend_verts_size > 0) {
        assert(alphablend_verts_size < MAX_VERTICES_PER_BUFFER);
        assert(alphablend_verts_size % 3 == 0);
        [render_pass_1_encoder setRenderPipelineState:
            _alphablend_pipeline_state];
        
        [render_pass_1_encoder
            drawPrimitives: MTLPrimitiveTypeTriangle
            vertexStart: gpu_shared_data_collection.
                triple_buffers[current_frame_i].first_alphablend_i
            vertexCount:
                alphablend_verts_size];
    }
    
    if ((
        gpu_shared_data_collection.triple_buffers[current_frame_i].
            line_vertices_size +
        gpu_shared_data_collection.triple_buffers[current_frame_i].
            point_vertices_size) > 0)
    {
        [render_pass_1_encoder setRenderPipelineState:
            _raw_pipeline_state];
        assert(_depth_stencil_state != nil);
        [render_pass_1_encoder setDepthStencilState:
            _depth_stencil_state];
        [render_pass_1_encoder setDepthClipMode:
            MTLDepthClipModeClip];
    }
    
    if (gpu_shared_data_collection.
        triple_buffers[current_frame_i].line_vertices_size > 0)
    {
        [render_pass_1_encoder
            setVertexBuffer:
                line_vertex_buffers[current_frame_i]
            offset:
                0
            atIndex:
                0];
        assert(gpu_shared_data_collection.
            triple_buffers[current_frame_i].line_vertices_size <=
                MAX_LINE_VERTICES);
        [render_pass_1_encoder
            drawPrimitives: MTLPrimitiveTypeLine
            vertexStart: 0
            vertexCount: gpu_shared_data_collection.
                triple_buffers[current_frame_i].line_vertices_size];
    }
    
    if (gpu_shared_data_collection.
        triple_buffers[current_frame_i].point_vertices_size > 0)
    {
        [render_pass_1_encoder
            setVertexBuffer:
                point_vertex_buffers[current_frame_i]
            offset:
                0
            atIndex:
                0];
        assert(gpu_shared_data_collection.
            triple_buffers[current_frame_i].point_vertices_size <=
                MAX_POINT_VERTICES);
        [render_pass_1_encoder
            drawPrimitives: MTLPrimitiveTypePoint
            vertexStart: 0
            vertexCount: gpu_shared_data_collection.
                triple_buffers[current_frame_i].point_vertices_size];
    }
    #ifdef PROFILER_ACTIVE
    profiler_end("draw calls");
    #endif
    
    [render_pass_1_encoder endEncoding];
    
    #if POSTPROCESSING_ACTIVE
    // TODO: add post processing pipeline
    MTLRenderPassDescriptor * render_pass_2_descriptor =
        [view currentRenderPassDescriptor];
    
    render_pass_2_descriptor.colorAttachments[0].clearColor =
        MTLClearColorMake(0.0f, 0.0f, 0.0f, 1.0f);;
    render_pass_2_descriptor.depthAttachment.loadAction =
        MTLLoadActionClear;
    
    id<MTLRenderCommandEncoder> render_pass_2_encoder =
        [command_buffer
            renderCommandEncoderWithDescriptor:
                render_pass_2_descriptor];
    
    [render_pass_2_encoder setViewport: cached_viewport];
    
    [render_pass_2_encoder setCullMode:MTLCullModeNone];
    
    [render_pass_2_encoder
        setRenderPipelineState: _postprocess_pipeline_state];
    // assert(_depth_stencil_state != nil);
    // [render_encoder setDepthStencilState: _depth_stencil_state];
    //    [render_pass_2_encoder
    //        setDepthClipMode: MTLDepthClipModeClip];
    
    #define FLVERT 1.0f
    static const PostProcessingVertex quad_vertices[] =
    {
        // Positions     , Texture coordinates,   bloom threshold
        #define F1_TEX_MAX 1.0f
        #define F1_TEX_MIN 0.0f
        #define F1_BLOOM 0.0f
        { {  FLVERT,  -FLVERT },  { F1_TEX_MAX, F1_TEX_MAX }, F1_BLOOM },
        { { -FLVERT,  -FLVERT },  { F1_TEX_MIN, F1_TEX_MAX }, F1_BLOOM },
        { { -FLVERT,   FLVERT },  { F1_TEX_MIN, F1_TEX_MIN }, F1_BLOOM },
        
        { {  FLVERT,  -FLVERT },  { F1_TEX_MAX, F1_TEX_MAX }, F1_BLOOM },
        { { -FLVERT,   FLVERT },  { F1_TEX_MIN, F1_TEX_MIN }, F1_BLOOM },
        { {  FLVERT,   FLVERT },  { F1_TEX_MAX, F1_TEX_MIN }, F1_BLOOM },
        
        // frame 2
        #define F2_TEX_MAX 0.995f
        #define F2_TEX_MIN 0.005f
        #define F2_BLOOM 0.6f
        { {  FLVERT,  -FLVERT },  { F2_TEX_MAX, F2_TEX_MAX }, F2_BLOOM },
        { { -FLVERT,  -FLVERT },  { F2_TEX_MIN, F2_TEX_MAX }, F2_BLOOM },
        { { -FLVERT,   FLVERT },  { F2_TEX_MIN, F2_TEX_MIN }, F2_BLOOM },
        
        { {  FLVERT,  -FLVERT },  { F2_TEX_MAX, F2_TEX_MAX }, F2_BLOOM },
        { { -FLVERT,   FLVERT },  { F2_TEX_MIN, F2_TEX_MIN }, F2_BLOOM },
        { {  FLVERT,   FLVERT },  { F2_TEX_MAX, F2_TEX_MIN }, F2_BLOOM },
        
        // frame 3
        #define F3_TEX_MAX 0.990f
        #define F3_TEX_MIN 0.010f
        #define F3_BLOOM 0.75f
        { {  FLVERT,  -FLVERT },  { F3_TEX_MAX, F3_TEX_MAX }, F3_BLOOM },
        { { -FLVERT,  -FLVERT },  { F3_TEX_MIN, F3_TEX_MAX }, F3_BLOOM },
        { { -FLVERT,   FLVERT },  { F3_TEX_MIN, F3_TEX_MIN }, F3_BLOOM },
        
        { {  FLVERT,  -FLVERT },  { F3_TEX_MAX, F3_TEX_MAX }, F3_BLOOM },
        { { -FLVERT,   FLVERT },  { F3_TEX_MIN, F3_TEX_MIN }, F3_BLOOM },
        { {  FLVERT,   FLVERT },  { F3_TEX_MAX, F3_TEX_MIN }, F3_BLOOM },
    };
    
    [render_pass_2_encoder
        setVertexBytes:&quad_vertices
        length:sizeof(quad_vertices)
        atIndex:0];
    
    [render_pass_2_encoder
        setVertexBuffer:
            postprocessing_constants_buffers[current_frame_i]
        offset:
            0
        atIndex:
            1];
    
    [render_pass_2_encoder
        setFragmentTexture: render_target_texture
        atIndex:0];
    
    [render_pass_2_encoder
        drawPrimitives:MTLPrimitiveTypeTriangle
        vertexStart:0
        vertexCount:6];
    
    [render_pass_2_encoder endEncoding];
    #endif
    
    // Schedule a present once the framebuffer is complete
    // using the current drawable
    [command_buffer presentDrawable: [view currentDrawable]];
    
    current_frame_i += 1;
    current_frame_i %= 3;
    
    [command_buffer addCompletedHandler:^(id<MTLCommandBuffer> arg_cmd_buffer) {
        (void)arg_cmd_buffer;
        
        dispatch_semaphore_signal(drawing_semaphore);
    }];
    
    [command_buffer commit];
    
    #ifdef PROFILER_ACTIVE
    profiler_end("drawInMTKView");
    #endif
}

- (void)mtkView:(MTKView *)view
    drawableSizeWillChange:(CGSize)size
{
}
@end

void platform_gpu_update_viewport(void)
{
    [apple_gpu_delegate updateViewport];
}

void platform_gpu_copy_locked_vertices(void)
{
    for (uint32_t i = 0; i < ALL_LOCKED_VERTICES_SIZE; i++) {
        assert(
            gpu_shared_data_collection.locked_vertices[i].parent_material_i <
                MAX_MATERIALS_PER_POLYGON);
    }
    
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
    
    log_assert(metal_active);
    
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
    if (rgba_values == NULL) {
        return;
    }
        
    [apple_gpu_delegate
        updateTextureArray : (int32_t)texture_array_i
        atTexture          : (int32_t)texture_i
        ofTextureArraySize : (uint32_t)parent_texture_array_images_size
        withImageOfWidth   : (uint32_t)image_width
        andHeight          : (uint32_t)image_height
        pixelValues        : (uint8_t *)rgba_values];
}
