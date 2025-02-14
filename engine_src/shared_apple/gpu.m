#import "gpu.h"

bool32_t has_retina_screen = true;
bool32_t metal_active = false;

typedef struct AppleGPUState {
    MTLPixelFormat cached_pixel_format_renderpass1;
    dispatch_semaphore_t drawing_semaphore;
    NSUInteger frame_i;
    MTLViewport render_target_viewport;
    MTLViewport cached_viewport;
    id<MTLDevice> metal_device;
    id<MTLLibrary> shader_library;
    id<MTLCommandQueue> command_queue;
    id polygon_buffers[3];
    id polygon_material_buffers[3];
    id light_buffers [3];
    id vertex_buffers[3];
    id camera_buffers[3];
    id line_vertex_buffers[3];
    id point_vertex_buffers[3];
    id postprocessing_constants_buffers[3];
    id locked_vertex_populator_buffer;
    id locked_vertex_buffer;
    id projection_constants_buffer;
    
    id<MTLRenderPipelineState> diamond_pipeline_state;
    id<MTLRenderPipelineState> alphablend_pipeline_state;
    id<MTLRenderPipelineState> raw_pipeline_state;
    #if POSTPROCESSING_ACTIVE
    id<MTLComputePipelineState> downsample_compute_pls;
    id<MTLComputePipelineState> boxblur_compute_pls;
    id<MTLComputePipelineState> thres_compute_pls;
    id<MTLRenderPipelineState> singlequad_pipeline_state;
    #endif
    id<MTLDepthStencilState>   depth_stencil_state;
    
    NSMutableArray * metal_textures;
    #if POSTPROCESSING_ACTIVE
    id<MTLTexture> render_target_texture;
    id<MTLTexture> downsampled_target_textures[DOWNSAMPLES_SIZE];
    #endif
} AppleGPUState;

static AppleGPUState * ags = NULL;


MetalKitViewDelegate * apple_gpu_delegate = NULL;

static void (* funcptr_shared_gameloop_update)(GPUDataForSingleFrame *) = NULL;

bool32_t apple_gpu_init(
    void (* arg_funcptr_shared_gameloop_update)(GPUDataForSingleFrame *),
    id<MTLDevice> with_metal_device,
    NSString * shader_lib_filepath,
    char * error_msg_string)
{
    //    RenderPassDescriptors[0] = NULL;
    //    RenderPassDescriptors[1] = NULL;
    //    RenderPassDescriptors[2] = NULL;
    
    ags = malloc_from_unmanaged(sizeof(AppleGPUState));
    log_assert(ags != NULL);
    
    funcptr_shared_gameloop_update = arg_funcptr_shared_gameloop_update;
    
    ags->drawing_semaphore = dispatch_semaphore_create(3);
    
    #if POSTPROCESSING_ACTIVE
    MTLPixelFormat render_pass_1_format = MTLPixelFormatRGBA16Float;
    #else
    MTLPixelFormat render_pass_1_format = MTLPixelFormatBGRA8Unorm;
    #endif
    ags->cached_pixel_format_renderpass1 = render_pass_1_format;
    
    ags->frame_i = 0;
        
    common_strcpy_capped(
        error_msg_string,
        512,
        "");
    
    // drawing_semaphore = dispatch_semaphore_create(/* initial value: */ 3);
    
    ags->metal_device = with_metal_device;
    
    NSError * Error = NULL;
    ags->shader_library = [with_metal_device newDefaultLibrary];
    
    if (ags->shader_library == NULL)
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
                error_msg_string,
                512,
                errorcstr);
            return false;
        }
        
        ags->shader_library =
            [with_metal_device
                newLibraryWithURL: shader_lib_url
                error: &Error];
        
        if (ags->shader_library == NULL) {
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
                    error_msg_string,
                    512,
                    errorcstr);
            } else {
                common_strcpy_capped(
                    error_msg_string,
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
        [ags->shader_library newFunctionWithName:
            @"vertex_shader"];
    if (vertex_shader == NULL) {
        log_append("Missing function: vertex_shader()!");
        
        common_strcpy_capped(
            error_msg_string,
            512,
            "Missing function: vertex_shader()");
        return false;
    }
    
    id<MTLFunction> fragment_shader =
        [ags->shader_library newFunctionWithName:
            @"fragment_shader"];
    if (fragment_shader == NULL) {
        log_append("Missing function: fragment_shader()!");
        common_strcpy_capped(
            error_msg_string,
            512,
            "Missing function: fragment_shader()");
        return false;
    }
    
    id<MTLFunction> alphablending_fragment_shader =
        [ags->shader_library newFunctionWithName:
            @"alphablending_fragment_shader"];
    if (alphablending_fragment_shader == NULL) {
        log_append("Missing function: alphablending_fragment_shader()!");
        common_strcpy_capped(
            error_msg_string,
            512,
            "Missing function: vertex_shader()");
        return false;
    }
    
    id<MTLFunction> raw_vertex_shader =
        [ags->shader_library newFunctionWithName:
            @"raw_vertex_shader"];
    if (raw_vertex_shader == NULL) {
        log_append("Missing function: raw_vertex_shader()!");

        common_strcpy_capped(
            error_msg_string,
            512,
            "Missing function: raw_vertex_shader()");
        return false;
    }
    
    id<MTLFunction> raw_fragment_shader =
        [ags->shader_library newFunctionWithName:
            @"raw_fragment_shader"];
    if (raw_fragment_shader == NULL) {
        log_append("Missing function: raw_fragment_shader()!");
        common_strcpy_capped(
            error_msg_string,
            512,
            "Missing function: vertex_shader()");
        return false;
    }
    
    // Setup pipeline that uses diamonds instaed of alphablending
    MTLRenderPipelineDescriptor * diamond_pipeline_descriptor =
        [[MTLRenderPipelineDescriptor alloc] init];
    [diamond_pipeline_descriptor
        setVertexFunction: vertex_shader];
    [diamond_pipeline_descriptor
        setFragmentFunction: fragment_shader];
    diamond_pipeline_descriptor
        .colorAttachments[0]
        .pixelFormat = render_pass_1_format;
    diamond_pipeline_descriptor.depthAttachmentPixelFormat =
        MTLPixelFormatDepth32Float;
    ags->diamond_pipeline_state =
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
            error_msg_string,
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
        .pixelFormat = render_pass_1_format;
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
    ags->alphablend_pipeline_state =
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
            error_msg_string,
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
        .pixelFormat = render_pass_1_format;
    [raw_pipeline_descriptor
        .colorAttachments[0]
        setBlendingEnabled: NO];
    raw_pipeline_descriptor.depthAttachmentPixelFormat =
        MTLPixelFormatDepth32Float;
    ags->raw_pipeline_state =
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
            error_msg_string,
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
    ags->depth_stencil_state = [with_metal_device
        newDepthStencilStateWithDescriptor:depth_descriptor];
    
    if (Error != NULL)
    {
        log_append([[Error localizedDescription] cStringUsingEncoding:kCFStringEncodingASCII]);
        #ifndef LOGGER_IGNORE_ASSERTS
        log_dump_and_crash("Error setting the depth stencil state\n");
        #endif
        
        common_strcpy_capped(
            error_msg_string,
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
        ags->polygon_buffers[frame_i] = MTLBufferFramePolygons;
        assert(ags->polygon_buffers[frame_i] != nil);
        
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
        ags->polygon_material_buffers[frame_i] = MTLBufferFramePolygonMaterials;
        assert(ags->polygon_material_buffers[frame_i] != nil);
        
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
        ags->vertex_buffers[frame_i] = MTLBufferFrameVertices;
        assert(ags->vertex_buffers[frame_i] != nil);
        
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
        ags->line_vertex_buffers[frame_i] = MTLBufferLineVertices;
        assert(ags->line_vertex_buffers[frame_i] != nil);
        
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
        ags->point_vertex_buffers[frame_i] = MTLBufferPointVertices;
        assert(ags->point_vertex_buffers[frame_i] != nil);
        
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
        ags->postprocessing_constants_buffers[frame_i] =
            MTLBufferPostProcessingConstants;
        assert(ags->postprocessing_constants_buffers[frame_i] != nil);
        
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
        ags->light_buffers[frame_i] = MTLBufferFrameLights;
        
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
        ags->camera_buffers[frame_i] = MTLBufferFrameCamera;
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
    ags->locked_vertex_populator_buffer = MTLBufferLockedVerticesPopulator;
    
    id<MTLBuffer> MTLBufferLockedVertices =
        [with_metal_device
            newBufferWithLength:
                gpu_shared_data_collection.locked_vertices_allocation_size
            options:
                MTLResourceStorageModePrivate];
    ags->locked_vertex_buffer = MTLBufferLockedVertices;
    
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
    ags->projection_constants_buffer = MTLBufferProjectionConstants;
    
    ags->metal_textures = [
        [NSMutableArray alloc]
            initWithCapacity: TEXTUREARRAYS_SIZE];
    
    #if POSTPROCESSING_ACTIVE
    
    id<MTLFunction> downsample_func =
        [ags->shader_library newFunctionWithName: @"downsample_texture"];
        log_assert(downsample_func != nil);
    
    ags->downsample_compute_pls =
        [ags->metal_device
            newComputePipelineStateWithFunction:downsample_func
            error:nil];
    
    id<MTLFunction> boxblur_func =
        [ags->shader_library newFunctionWithName: @"boxblur_texture"];
    log_assert(boxblur_func != nil);
    ags->boxblur_compute_pls =
        [ags->metal_device
            newComputePipelineStateWithFunction:boxblur_func
            error:nil];
    
    id<MTLFunction> threshold_func =
        [ags->shader_library newFunctionWithName: @"threshold_texture"];
        log_assert(threshold_func != nil);
    
    ags->thres_compute_pls =
        [ags->metal_device
            newComputePipelineStateWithFunction:threshold_func
            error:nil];
    
    id<MTLFunction> singlequad_vertex_shader =
        [ags->shader_library newFunctionWithName:
            @"single_quad_vertex_shader"];
    if (singlequad_vertex_shader == NULL) {
        log_append("Missing function: postprocess_vertex_shader()!");
        
        common_strcpy_capped(
            error_msg_string,
            512,
            "Missing function: postprocess_vertex_shader()");
        return false;
    }
    
    id<MTLFunction> singlequad_fragment_shader =
        [ags->shader_library
            newFunctionWithName: @"single_quad_fragment_shader"];
    
    if (singlequad_fragment_shader == NULL) {
        log_append("Missing function: downsampling_fragment_shader()!");
        common_strcpy_capped(
            error_msg_string,
            512,
            "Missing function: downsampling_fragment_shader()");
        return false;
    }
    
    MTLRenderPipelineDescriptor * singlequad_pipeline_descriptor =
        [[MTLRenderPipelineDescriptor alloc] init];
    
    // Set up pipeline for rendering the texture to the screen with a simple
    // quad
    singlequad_pipeline_descriptor.label = @"SingleQuad Pipeline";
    singlequad_pipeline_descriptor.sampleCount = 1;
    [singlequad_pipeline_descriptor
        setVertexFunction: singlequad_vertex_shader];
    [singlequad_pipeline_descriptor
        setFragmentFunction: singlequad_fragment_shader];
    singlequad_pipeline_descriptor.colorAttachments[0].pixelFormat =
        MTLPixelFormatBGRA8Unorm;
    [singlequad_pipeline_descriptor
        .colorAttachments[0]
        setBlendingEnabled: YES];
    singlequad_pipeline_descriptor
        .colorAttachments[0].sourceRGBBlendFactor =
            MTLBlendFactorSourceAlpha;
    singlequad_pipeline_descriptor
        .colorAttachments[0].destinationRGBBlendFactor =
            MTLBlendFactorOneMinusSourceAlpha;
    singlequad_pipeline_descriptor.depthAttachmentPixelFormat =
        MTLPixelFormatDepth32Float;
    singlequad_pipeline_descriptor.vertexBuffers[0].mutability =
        MTLMutabilityImmutable;
    ags->singlequad_pipeline_state = [
        with_metal_device
        newRenderPipelineStateWithDescriptor:singlequad_pipeline_descriptor
        error:NULL];
    #endif
    
    // TODO:
    // [self updateViewport];
    
    ags->command_queue = [with_metal_device newCommandQueue];
    
    metal_active = true;
    
    return true;
}

static float get_ds_width(
    const uint32_t ds_i)
{
    float return_value = (float)ags->render_target_texture.width * 0.5f;
    
    for (uint32_t i = 0; i < ds_i && i < DOWNSAMPLES_CUTOFF; i++) {
        return_value *= 0.5f;
    }
    
    return return_value;
}

static float get_ds_height(
    const uint32_t ds_i)
{
    float return_value = (float)ags->render_target_texture.height * 0.5f;
    
    for (uint32_t i = 0; i < ds_i && i < DOWNSAMPLES_CUTOFF; i++) {
        return_value *= 0.5f;
    }
    
    return return_value;
}

void platform_gpu_init_texture_array(
    const int32_t texture_array_i,
    const uint32_t num_images,
    const uint32_t single_image_width,
    const uint32_t single_image_height)
{
    assert(texture_array_i < 31);
    
    // we always overwrite textures, so pad them to match first
    while ((int32_t)[ags->metal_textures count] <= texture_array_i) {
        MTLTextureDescriptor * texture_descriptor =
            [[MTLTextureDescriptor alloc] init];
        texture_descriptor.textureType = MTLTextureType2DArray;
        texture_descriptor.arrayLength = 1;
        texture_descriptor.pixelFormat = MTLPixelFormatRGBA8Unorm;
        texture_descriptor.width = 10;
        texture_descriptor.height = 10;
        id<MTLTexture> texture =
            [ags->metal_device newTextureWithDescriptor:texture_descriptor];
        [ags->metal_textures addObject: texture];
    }
    
    MTLTextureDescriptor * texture_descriptor =
        [[MTLTextureDescriptor alloc] init];
    texture_descriptor.textureType = MTLTextureType2DArray;
    texture_descriptor.arrayLength = num_images;
    texture_descriptor.pixelFormat = MTLPixelFormatRGBA8Unorm;
    texture_descriptor.width = single_image_width;
    texture_descriptor.height = single_image_height;
    
    id<MTLTexture> texture = [ags->metal_device
        newTextureWithDescriptor:texture_descriptor];
    
    [ags->metal_textures
        replaceObjectAtIndex:(uint32_t)texture_array_i
        withObject: texture];
}

static bool32_t font_already_pushed = 0;
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
    
    log_assert(texture_i >= 0);
    log_assert(texture_array_i >= 0);
    
    if (texture_array_i == 0 && texture_i == 0) {
        assert(!font_already_pushed);
        font_already_pushed = true;
    }
    
    if (texture_array_i >= (int32_t)[ags->metal_textures count]) {
        #ifndef LOGGER_IGNORE_ASSERTS
        char errmsg[256];
        common_strcpy_capped(errmsg, 256, "Tried to update uninitialized texturearray")
        common_strcat_int_capped(errmsg, 256, texture_array_i);
        common_strcat_capped(errmsg, 256, "\n");
        
        log_dump_and_crash(errmsg);
        #endif
        return;
    }
    
    MTLTextureDescriptor * texture_descriptor =
        [[MTLTextureDescriptor alloc] init];
    texture_descriptor.textureType = MTLTextureType2DArray;
    texture_descriptor.arrayLength = parent_texture_array_images_size;
    texture_descriptor.pixelFormat = MTLPixelFormatRGBA8Unorm;
    texture_descriptor.width = image_width;
    texture_descriptor.height = image_height;
    
    MTLRegion region = {
        { 0,0,0 },
        { image_width, image_height, 1 }
    };
    
    [[ags->metal_textures
        objectAtIndex:
            (NSUInteger)texture_array_i]
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

void platform_gpu_copy_locked_vertices(void)
{
    for (uint32_t i = 0; i < ALL_LOCKED_VERTICES_SIZE; i++) {
        assert(
            gpu_shared_data_collection.locked_vertices[i].parent_material_i <
                MAX_MATERIALS_PER_POLYGON);
    }
    
    gpu_shared_data_collection.locked_vertices_size = all_mesh_vertices->size;
    
    id <MTLCommandBuffer> commandBuffer = [ags->command_queue commandBuffer];
    
    // Encode a blit pass to copy data from the source buffer to the private
    // buffer.
    id <MTLBlitCommandEncoder> blitCommandEncoder =
        [commandBuffer blitCommandEncoder];
    [blitCommandEncoder
        copyFromBuffer:ags->locked_vertex_populator_buffer
        sourceOffset:0
        toBuffer:ags->locked_vertex_buffer
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

@implementation MetalKitViewDelegate
{
}
- (void) updateViewport
{
    ags->cached_viewport.originX = 0;
    ags->cached_viewport.originY = 0;
    ags->cached_viewport.width   =
        window_globals->window_width *
        (has_retina_screen ? 2.0f : 1.0f);
    ags->cached_viewport.height  =
        window_globals->window_height *
        (has_retina_screen ? 2.0f : 1.0f);
    assert(ags->cached_viewport.width > 0.0f);
    assert(ags->cached_viewport.height > 0.0f);
    
    /*
    These near/far values are the final viewport coordinates (after
    fragment shader), not to be confused with
    window_globals->projection_constants.near that's in our world space
    and much larger numbers
    */
    ags->cached_viewport.znear   = 0.001f; 
    ags->cached_viewport.zfar    = 1.0f;
    
    *gpu_shared_data_collection.locked_pjc =
        window_globals->projection_constants;
    
    #if POSTPROCESSING_ACTIVE
    // Set up a texture for rendering to and apply post-processing to
    MTLTextureDescriptor * texture_descriptor = [MTLTextureDescriptor new];
    texture_descriptor.textureType = MTLTextureType2D;
    texture_descriptor.width = (unsigned long)ags->cached_viewport.width / 2;
    texture_descriptor.height = (unsigned long)ags->cached_viewport.height / 2;
    texture_descriptor.pixelFormat = MTLPixelFormatRGBA16Float;
    texture_descriptor.usage =
        MTLTextureUsageRenderTarget |
        MTLTextureUsageShaderWrite |
        MTLTextureUsageShaderRead;
    texture_descriptor.mipmapLevelCount = 1;
    
    ags->render_target_texture = [ags->metal_device
        newTextureWithDescriptor: texture_descriptor];
    
    ags->render_target_viewport = ags->cached_viewport;
    ags->render_target_viewport.width = texture_descriptor.width;
    ags->render_target_viewport.height = texture_descriptor.height;
    
    for (uint32_t i = 0; i < DOWNSAMPLES_SIZE; i++) {
        
        MTLTextureDescriptor * downsampled_target_texture_desc =
            [MTLTextureDescriptor new];
        downsampled_target_texture_desc.textureType = MTLTextureType2D;
        downsampled_target_texture_desc.width = (NSUInteger)get_ds_width(i);
        downsampled_target_texture_desc.height = (NSUInteger)get_ds_height(i);
        downsampled_target_texture_desc.pixelFormat = MTLPixelFormatRGBA16Float;
        downsampled_target_texture_desc.mipmapLevelCount = 1;
        downsampled_target_texture_desc.usage =
            MTLTextureUsageShaderWrite |
            MTLTextureUsageShaderRead;
        ags->downsampled_target_textures[i] = [ags->metal_device
            newTextureWithDescriptor: downsampled_target_texture_desc];
    }
    #endif
}

- (void)drawInMTKView:(MTKView *)view
{
    #ifdef PROFILER_ACTIVE
    profiler_new_frame();
    profiler_start("drawInMTKView");
    #endif
    
    dispatch_semaphore_wait(ags->drawing_semaphore, DISPATCH_TIME_FOREVER);
    
    funcptr_shared_gameloop_update(
        &gpu_shared_data_collection.triple_buffers[ags->frame_i]);
    
    if (!metal_active) {
        #ifdef PROFILER_ACTIVE
        profiler_end("drawInMTKView");
        #endif
        return;
    }
    
    id<MTLCommandBuffer> command_buffer = [ags->command_queue commandBuffer];
    
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
        ags->render_target_texture;
    render_pass_1_descriptor.colorAttachments[0].storeAction =
        MTLStoreActionStore;
    #else
    render_pass_1_descriptor
        .colorAttachments[0]
        .loadAction = MTLLoadActionClear;
    #endif
    
    render_pass_1_descriptor.colorAttachments[0].clearColor =
        MTLClearColorMake(0.0f, 0.03f, 0.15f, 1.0f);
    
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
    assert(ags->cached_viewport.zfar > ags->cached_viewport.znear);
    [render_pass_1_encoder setViewport: ags->render_target_viewport];
    assert(ags->cached_viewport.width > 0.0f);
    assert(ags->cached_viewport.height > 0.0f);
    
    [render_pass_1_encoder setRenderPipelineState: ags->diamond_pipeline_state];
    assert(ags->depth_stencil_state != nil);
    [render_pass_1_encoder setDepthStencilState: ags->depth_stencil_state];
    [render_pass_1_encoder setDepthClipMode: MTLDepthClipModeClip];
    #ifdef PROFILER_ACTIVE
    profiler_end("setViewport, RenderPipeline, Stencil, ClipMode");
    #endif
    
    #ifdef PROFILER_ACTIVE
    profiler_start("setVertexBuffers");
    #endif
    [render_pass_1_encoder
        setVertexBuffer:
            ags->vertex_buffers[ags->frame_i]
        offset:
            0
        atIndex:
            0];
    
    [render_pass_1_encoder
        setVertexBuffer:
            ags->polygon_buffers[ags->frame_i]
        offset:
            0
        atIndex:
            1];
    
    [render_pass_1_encoder
        setVertexBuffer:
            ags->light_buffers[ags->frame_i]
        offset: 0
        atIndex: 2];
    
    [render_pass_1_encoder
        setVertexBuffer:
            ags->camera_buffers[ags->frame_i]
        offset: 0
        atIndex: 3];
    
    [render_pass_1_encoder
        setVertexBuffer:
            ags->locked_vertex_buffer
        offset:
            0 
        atIndex:
            4];
    
    [render_pass_1_encoder
        setVertexBuffer:
            ags->projection_constants_buffer
        offset:
            0
        atIndex:
            5];
    
    [render_pass_1_encoder
        setVertexBuffer:
            ags->polygon_material_buffers[ags->frame_i]
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
        i < [ags->metal_textures count];
        i++)
    {
        [render_pass_1_encoder
            setFragmentTexture: ags->metal_textures[i]
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
            triple_buffers[ags->frame_i].
            vertices_size;
        i++)
    {
        assert(
            gpu_shared_data_collection.
                triple_buffers[ags->frame_i].
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
            triple_buffers[ags->frame_i].first_alphablend_i;
    
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
        gpu_shared_data_collection.triple_buffers[ags->frame_i].
            first_alphablend_i <=
        gpu_shared_data_collection.triple_buffers[ags->frame_i].
            vertices_size);
    uint32_t alphablend_verts_size =
        gpu_shared_data_collection.
            triple_buffers[ags->frame_i].vertices_size -
        gpu_shared_data_collection.
            triple_buffers[ags->frame_i].first_alphablend_i;
    
    if (window_globals->draw_triangles && alphablend_verts_size > 0) {
        assert(alphablend_verts_size < MAX_VERTICES_PER_BUFFER);
        assert(alphablend_verts_size % 3 == 0);
        [render_pass_1_encoder setRenderPipelineState:
            ags->alphablend_pipeline_state];
        
        [render_pass_1_encoder
            drawPrimitives: MTLPrimitiveTypeTriangle
            vertexStart: gpu_shared_data_collection.
                triple_buffers[ags->frame_i].first_alphablend_i
            vertexCount:
                alphablend_verts_size];
    }
    
    if ((
        gpu_shared_data_collection.triple_buffers[ags->frame_i].
            line_vertices_size +
        gpu_shared_data_collection.triple_buffers[ags->frame_i].
            point_vertices_size) > 0)
    {
        [render_pass_1_encoder setRenderPipelineState: ags->raw_pipeline_state];
        assert(ags->depth_stencil_state != nil);
        [render_pass_1_encoder setDepthStencilState: ags->depth_stencil_state];
        [render_pass_1_encoder setDepthClipMode: MTLDepthClipModeClip];
    }
    
    if (gpu_shared_data_collection.
        triple_buffers[ags->frame_i].line_vertices_size > 0)
    {
        [render_pass_1_encoder
            setVertexBuffer:
                ags->line_vertex_buffers[ags->frame_i]
            offset:
                0
            atIndex:
                0];
        assert(gpu_shared_data_collection.
            triple_buffers[ags->frame_i].line_vertices_size <=
                MAX_LINE_VERTICES);
        [render_pass_1_encoder
            drawPrimitives: MTLPrimitiveTypeLine
            vertexStart: 0
            vertexCount: gpu_shared_data_collection.
                triple_buffers[ags->frame_i].line_vertices_size];
    }
    
    if (gpu_shared_data_collection.
        triple_buffers[ags->frame_i].point_vertices_size > 0)
    {
        [render_pass_1_encoder
            setVertexBuffer:
                ags->point_vertex_buffers[ags->frame_i]
            offset:
                0
            atIndex:
                0];
        assert(gpu_shared_data_collection.
            triple_buffers[ags->frame_i].point_vertices_size <=
                MAX_POINT_VERTICES);
        [render_pass_1_encoder
            drawPrimitives: MTLPrimitiveTypePoint
            vertexStart: 0
            vertexCount: gpu_shared_data_collection.
                triple_buffers[ags->frame_i].point_vertices_size];
    }
    #ifdef PROFILER_ACTIVE
    profiler_end("draw calls");
    #endif
    
    [render_pass_1_encoder endEncoding];
    
    #if POSTPROCESSING_ACTIVE
    #define FLVERT 1.0f
    #define TEX_MAX 1.0f
    #define TEX_MIN 0.0f
    static const PostProcessingVertex quad_vertices[] =
    {
        // Positions     , Texture coordinates,   bloom threshold
        {{  FLVERT,  -FLVERT },  { TEX_MAX, TEX_MAX }},
        {{ -FLVERT,  -FLVERT },  { TEX_MIN, TEX_MAX }},
        {{ -FLVERT,   FLVERT },  { TEX_MIN, TEX_MIN }},
        
        {{  FLVERT,  -FLVERT },  { TEX_MAX, TEX_MAX }},
        {{ -FLVERT,   FLVERT },  { TEX_MIN, TEX_MIN }},
        {{  FLVERT,   FLVERT },  { TEX_MAX, TEX_MIN }},
    };
    
    // Render pass 2 downsamples the original texture
    for (uint32_t ds_i = 0; ds_i < DOWNSAMPLES_SIZE; ds_i++) {
        
        MTLViewport smaller_viewport = ags->cached_viewport;
        smaller_viewport.width = ags->downsampled_target_textures[ds_i].width;
        smaller_viewport.height = ags->downsampled_target_textures[ds_i].height;
        
        MTLSize grid = MTLSizeMake(
            (uint32_t)smaller_viewport.width,
            (uint32_t)smaller_viewport.height,
            1);
        
        MTLSize threadgroup = MTLSizeMake(16, 16, 1);
        
        if (ds_i < DOWNSAMPLES_CUTOFF) {
            id<MTLComputeCommandEncoder> compute_encoder =
            [command_buffer computeCommandEncoder];
            
            [compute_encoder
                setComputePipelineState: ags->downsample_compute_pls];
            [compute_encoder
             setTexture:ds_i > 0 ?
             ags->downsampled_target_textures[ds_i-1] :
                 ags->render_target_texture
             atIndex:0];
            [compute_encoder
             setTexture:ags->downsampled_target_textures[ds_i]
             atIndex:1];
            
            [compute_encoder
                dispatchThreads:grid
                threadsPerThreadgroup:threadgroup];
            
            [compute_encoder endEncoding];
            
            // Mask only the brightest values
            if (ds_i == 0) {
                id<MTLComputeCommandEncoder> thres_encoder =
                    [command_buffer computeCommandEncoder];
                
                [thres_encoder setComputePipelineState:ags->thres_compute_pls];
                [thres_encoder
                    setTexture: ags->downsampled_target_textures[0]
                    atIndex:0];
                [thres_encoder
                    dispatchThreads:grid
                    threadsPerThreadgroup:threadgroup];
                
                [thres_encoder endEncoding];
            }
        }
        
        id<MTLComputeCommandEncoder> boxblur_encoder =
            [command_buffer computeCommandEncoder];
        
        [boxblur_encoder setComputePipelineState:ags->boxblur_compute_pls];
        [boxblur_encoder
            setTexture: ags->downsampled_target_textures[ds_i]
            atIndex:0];
        [boxblur_encoder
            dispatchThreads:grid
            threadsPerThreadgroup:threadgroup];
        [boxblur_encoder endEncoding];
        #endif
    }
    
    // Render pass 4 puts a quad on the full screen
    MTLRenderPassDescriptor * render_pass_4_descriptor =
        [view currentRenderPassDescriptor];
    
    render_pass_4_descriptor.colorAttachments[0].clearColor =
        MTLClearColorMake(0.0f, 0.0f, 0.0f, 1.0f);;
    render_pass_4_descriptor.depthAttachment.loadAction =
        MTLLoadActionClear;
    id<MTLRenderCommandEncoder> render_pass_4_encoder =
        [command_buffer
            renderCommandEncoderWithDescriptor:
                render_pass_4_descriptor];
    
    [render_pass_4_encoder setViewport: ags->cached_viewport];
    
    [render_pass_4_encoder setCullMode:MTLCullModeNone];
    
    [render_pass_4_encoder
        setRenderPipelineState: ags->singlequad_pipeline_state];
    
    [render_pass_4_encoder
        setVertexBytes:&quad_vertices
        length:sizeof(quad_vertices)
        atIndex:0];
    
    [render_pass_4_encoder
        setVertexBuffer:ags->postprocessing_constants_buffers[ags->frame_i]
        offset:0
        atIndex:1];
    
    [render_pass_4_encoder
        setFragmentTexture: ags->render_target_texture
        atIndex:0];
    
    [render_pass_4_encoder
        setFragmentTexture: ags->downsampled_target_textures[0]
        atIndex:1];
    [render_pass_4_encoder
        setFragmentTexture: ags->downsampled_target_textures[1]
        atIndex:2];
    [render_pass_4_encoder
        setFragmentTexture: ags->downsampled_target_textures[2]
        atIndex:3];
    [render_pass_4_encoder
        setFragmentTexture: ags->downsampled_target_textures[3]
        atIndex:4];
    [render_pass_4_encoder
        setFragmentTexture: ags->downsampled_target_textures[4]
        atIndex:5];
    
    [render_pass_4_encoder
        drawPrimitives:MTLPrimitiveTypeTriangle
        vertexStart:0
        vertexCount:6];
    
    [render_pass_4_encoder endEncoding];
    
    // Schedule a present once the framebuffer is complete
    // using the current drawable
    [command_buffer presentDrawable: [view currentDrawable]];
    
    ags->frame_i += 1;
    ags->frame_i %= 3;
    
    [command_buffer addCompletedHandler:^(id<MTLCommandBuffer> arg_cmd_buffer) {
        (void)arg_cmd_buffer;
        
        dispatch_semaphore_signal(ags->drawing_semaphore);
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
