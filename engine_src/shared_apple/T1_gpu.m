#import "T1_gpu.h"

#define DRAWING_SEMAPHORE_ACTIVE 0

typedef struct AppleGPUState {
    MTLPixelFormat pixel_format_renderpass1;
    #if DRAWING_SEMAPHORE_ACTIVE
    dispatch_semaphore_t drawing_semaphore;
    #endif
    NSUInteger frame_i;
    MTLViewport render_target_viewport;
    MTLViewport cached_viewport;
    
    id<MTLDevice> device;
    id<MTLLibrary> lib;
    id<MTLCommandQueue> command_queue;
    
    id polygon_buffers[MAX_RENDERING_FRAME_BUFFERS];
    id light_buffers [MAX_RENDERING_FRAME_BUFFERS];
    id vertex_buffers[MAX_RENDERING_FRAME_BUFFERS];
    id camera_buffers[MAX_RENDERING_FRAME_BUFFERS];
    id line_vertex_buffers[MAX_RENDERING_FRAME_BUFFERS];
    id point_vertex_buffers[MAX_RENDERING_FRAME_BUFFERS];
    id postprocessing_constants_buffers[MAX_RENDERING_FRAME_BUFFERS];
    id locked_vertex_populator_buffer;
    id locked_vertex_buffer;
    id locked_materials_populator_buffer;
    id locked_materials_buffer;
    id projection_constants_buffer;
    
    // Pipeline states (pls)
    #if SHADOWS_ACTIVE
    id<MTLRenderPipelineState> shadows_pls;
    id<MTLTexture> shadows_texture;
    #endif
    id<MTLTexture> camera_depth_texture;
    
    id<MTLRenderPipelineState> diamond_pls;
    id<MTLRenderPipelineState> alphablend_pls;
    #if RAW_SHADER_ACTIVE
    id<MTLRenderPipelineState> raw_pls;
    #endif
    #if BLOOM_ACTIVE
    id<MTLComputePipelineState> downsample_compute_pls;
    id<MTLComputePipelineState> boxblur_compute_pls;
    id<MTLComputePipelineState> thres_compute_pls;
    #endif
    id<MTLRenderPipelineState> singlequad_pls;
    
    id<MTLDepthStencilState> opaque_depth_stencil_state;
    id<MTLDepthStencilState> alpha_depth_stencil_state;
    
    // Textures
    // id<MTLBuffer> texture_populator_buffer;
    #if TEXTURES_ACTIVE
    id<MTLTexture> metal_textures[TEXTUREARRAYS_SIZE];
    #else
    id<MTLTexture> metal_textures[1]; // for font only
    #endif
    id<MTLTexture> render_target_texture;
    #if BLOOM_ACTIVE
    id<MTLTexture> downsampled_target_textures[DOWNSAMPLES_SIZE];
    #endif
    id<MTLTexture> touch_id_texture;
    id<MTLBuffer> touch_id_buffer;
    id<MTLBuffer> touch_id_buffer_all_zeros;
    T1PostProcessingVertex quad_vertices[6];
    float retina_scaling_factor;
    bool32_t metal_active;
    bool32_t viewport_set;
} AppleGPUState;

static AppleGPUState * ags = NULL;


MetalKitViewDelegate * apple_gpu_delegate = NULL;

static void (* funcptr_shared_gameloop_update)(GPUFrame *) = NULL;
static void (* funcptr_shared_gameloop_update_after_render_pass)(void) = NULL;

bool32_t apple_gpu_init(
    void (* arg_funcptr_shared_gameloop_update)(GPUFrame *),
    void (* arg_funcptr_shared_gameloop_update_after_render_pass)(void),
    id<MTLDevice> with_metal_device,
    NSString * shader_lib_filepath,
    float backing_scale_factor,
    char * error_msg_string)
{
    ags = T1_mem_malloc_from_unmanaged(sizeof(AppleGPUState)); // TODO: use malloc_from_unmanaged again
    ags->retina_scaling_factor = backing_scale_factor;
    ags->pixel_format_renderpass1 = 0;
    #if DRAWING_SEMAPHORE_ACTIVE
    ags->drawing_semaphore = NULL; // TODO: remove me
    ags->drawing_semaphore = dispatch_semaphore_create(3);
    #endif
    
    funcptr_shared_gameloop_update =
        arg_funcptr_shared_gameloop_update;
    funcptr_shared_gameloop_update_after_render_pass =
        arg_funcptr_shared_gameloop_update_after_render_pass;
    
    ags->pixel_format_renderpass1 = MTLPixelFormatRGBA16Float;
    
    ags->frame_i = 0;
    
    T1_std_strcpy_cap(
        error_msg_string,
        512,
        "");
    
    ags->device = with_metal_device;
    
    NSError * Error = NULL;
    ags->lib = [with_metal_device newDefaultLibrary];
    
    if (ags->lib == NULL)
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
            
            T1_std_strcpy_cap(
                error_msg_string,
                512,
                errorcstr);
            return false;
        }
        
        ags->lib =
            [with_metal_device
                newLibraryWithURL: shader_lib_url
                error: &Error];
        
        if (ags->lib == NULL) {
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
                T1_std_strcpy_cap(
                    error_msg_string,
                    512,
                    errorcstr);
            } else {
                T1_std_strcpy_cap(
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
        [ags->lib newFunctionWithName:
            @"vertex_shader"];
    if (vertex_shader == NULL) {
        log_append("Missing function: vertex_shader()!");
        
        T1_std_strcpy_cap(
            error_msg_string,
            512,
            "Missing function: vertex_shader()");
        return false;
    }
    
    id<MTLFunction> fragment_shader =
        [ags->lib newFunctionWithName:
            @"fragment_shader"];
    if (fragment_shader == NULL) {
        T1_std_strcpy_cap(
            error_msg_string,
            512,
            "Missing function: alphablending_fragment_shader()");
        return false;
    }
    
    id<MTLFunction> alphablending_fragment_shader =
        [ags->lib newFunctionWithName:
            @"alphablending_fragment_shader"];
    if (alphablending_fragment_shader == NULL) {
        T1_std_strcpy_cap(
            error_msg_string,
            512,
            "Missing function: alphablending_vertex_shader()");
        return false;
    }
    
    #if SHADOWS_ACTIVE
    id<MTLFunction> shadows_vertex_shader =
        [ags->lib newFunctionWithName:
            @"shadows_vertex_shader"];
    if (shadows_vertex_shader == NULL) {
        T1_std_strcpy_cap(
            error_msg_string,
            512,
            "Missing function: shadows_vertex_shader()");
        return false;
    }
    
    id<MTLFunction> shadows_fragment_shader =
        [ags->lib newFunctionWithName:
            @"shadows_fragment_shader"];
    if (shadows_fragment_shader == NULL) {
        log_append("Missing function: shadows_fragment_shader()!");
        
        T1_std_strcpy_cap(
            error_msg_string,
            512,
            "Missing function: shadows_fragment_shader()");
        return false;
    }
    
    MTLRenderPipelineDescriptor * shadows_pipeline_descriptor =
        [MTLRenderPipelineDescriptor new];
    [shadows_pipeline_descriptor
        setVertexFunction: shadows_vertex_shader];
    [shadows_pipeline_descriptor
        setFragmentFunction: nil];
    shadows_pipeline_descriptor.depthAttachmentPixelFormat =
        MTLPixelFormatDepth32Float;
    shadows_pipeline_descriptor.label = @"shadow pipeline state";
    
    ags->shadows_pls =
       [with_metal_device
            newRenderPipelineStateWithDescriptor:
                shadows_pipeline_descriptor
            error:
                &Error];
    #endif
    
    // Setup pipeline that uses diamonds instaed of alphablending
    MTLRenderPipelineDescriptor * diamond_pipeline_descriptor =
        [MTLRenderPipelineDescriptor new];
    [diamond_pipeline_descriptor
        setVertexFunction: vertex_shader];
    [diamond_pipeline_descriptor
        setFragmentFunction: fragment_shader];
    assert(ags->pixel_format_renderpass1 == MTLPixelFormatRGBA16Float);
    diamond_pipeline_descriptor
        .colorAttachments[0]
        .pixelFormat = ags->pixel_format_renderpass1;
    diamond_pipeline_descriptor.colorAttachments[1].pixelFormat =
        ags->pixel_format_renderpass1;
    diamond_pipeline_descriptor.depthAttachmentPixelFormat =
        MTLPixelFormatDepth32Float;
    diamond_pipeline_descriptor.label = @"diamond pipeline state";
    ags->diamond_pls =
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
        T1_std_strcpy_cap(
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
        .pixelFormat = ags->pixel_format_renderpass1;
    [alphablend_pipeline_descriptor
        .colorAttachments[0]
        setBlendingEnabled: YES];
    alphablend_pipeline_descriptor
        .colorAttachments[0].sourceRGBBlendFactor =
            MTLBlendFactorSourceAlpha;
    alphablend_pipeline_descriptor
        .colorAttachments[0].destinationRGBBlendFactor =
            MTLBlendFactorOneMinusSourceAlpha;
    alphablend_pipeline_descriptor
        .colorAttachments[0].rgbBlendOperation =
            MTLBlendOperationAdd;
    alphablend_pipeline_descriptor.colorAttachments[1].
        pixelFormat = ags->pixel_format_renderpass1;
    alphablend_pipeline_descriptor.depthAttachmentPixelFormat =
        MTLPixelFormatDepth32Float;
    alphablend_pipeline_descriptor.label = @"Alphablending pipeline";
    ags->alphablend_pls =
        [with_metal_device
            newRenderPipelineStateWithDescriptor:
                alphablend_pipeline_descriptor
            error:
                &Error];
    
    if (Error != NULL)
    {
        log_append(
            [[Error localizedDescription]
                cStringUsingEncoding:kCFStringEncodingASCII]);
        
        #ifndef LOGGER_IGNORE_ASSERTS
        log_dump_and_crash("Error loading the alphablending shader\n");
        #endif
        
        T1_std_strcpy_cap(
            error_msg_string,
            512,
            "Failed to load the alphablending shader");
        return false;
    }
    
    #if RAW_SHADER_ACTIVE
    id<MTLFunction> raw_vertex_shader =
        [ags->lib newFunctionWithName:
            @"raw_vertex_shader"];
    if (raw_vertex_shader == NULL) {
        T1_std_strcpy_cap(
            error_msg_string,
            512,
            "Missing function: raw_vertex_shader()");
        return false;
    }
    
    id<MTLFunction> raw_fragment_shader =
        [ags->lib newFunctionWithName:
            @"raw_fragment_shader"];
    if (raw_fragment_shader == NULL) {
        T1_std_strcpy_cap(
            error_msg_string,
            512,
            "Missing function: raw_fragment_shader()");
        return false;
    }
    
    MTLRenderPipelineDescriptor * raw_pipeline_descriptor =
        [[MTLRenderPipelineDescriptor alloc] init];
    [raw_pipeline_descriptor
        setVertexFunction: raw_vertex_shader];
    [raw_pipeline_descriptor
        setFragmentFunction: raw_fragment_shader];
    assert(ags->pixel_format_renderpass1 == MTLPixelFormatRGBA16Float);
    raw_pipeline_descriptor
        .colorAttachments[0]
        .pixelFormat = ags->pixel_format_renderpass1;
    raw_pipeline_descriptor
        .colorAttachments[1]
        .pixelFormat = ags->pixel_format_renderpass1;
    [raw_pipeline_descriptor
        .colorAttachments[0]
        setBlendingEnabled: NO];
    raw_pipeline_descriptor.depthAttachmentPixelFormat =
        MTLPixelFormatDepth32Float;
    raw_pipeline_descriptor.label = @"raw pipeline state";
    ags->raw_pls =
        [with_metal_device
            newRenderPipelineStateWithDescriptor:
                raw_pipeline_descriptor
            error:
                &Error];
    if (Error != NULL || ags->raw_pls == NULL)
    {
        log_append([[Error localizedDescription] cStringUsingEncoding:kCFStringEncodingASCII]);
        #ifndef LOGGER_IGNORE_ASSERTS
        log_dump_and_crash("Error loading the raw vertex shader\n");
        #endif
        
        T1_std_strcpy_cap(
            error_msg_string,
            512,
            "Failed to load the raw vertex shader");
        return false;
    }
    #endif
    
    MTLDepthStencilDescriptor * depth_descriptor =
        [MTLDepthStencilDescriptor new];
    depth_descriptor.depthWriteEnabled = YES;
    [depth_descriptor setDepthCompareFunction:MTLCompareFunctionLessEqual];
    // [depth_descriptor setDepthCompareFunction:MTLCompareFunctionAlways];
    ags->opaque_depth_stencil_state = [with_metal_device
        newDepthStencilStateWithDescriptor:depth_descriptor];
    
    if (Error != NULL)
    {
        log_append(
            [[Error localizedDescription]
                cStringUsingEncoding:kCFStringEncodingASCII]);
        #ifndef LOGGER_IGNORE_ASSERTS
        log_dump_and_crash("Error setting the depth stencil state\n");
        #endif
        
        T1_std_strcpy_cap(
            error_msg_string,
            512,
            "Failed to load the depth stencil shader");
        return false;
    }
    
    MTLDepthStencilDescriptor * alpha_depth_descriptor =
        [MTLDepthStencilDescriptor new];
    alpha_depth_descriptor.depthWriteEnabled = YES; // TODO: test with NO
    [alpha_depth_descriptor setDepthCompareFunction:MTLCompareFunctionLessEqual];
    // [depth_descriptor setDepthCompareFunction:MTLCompareFunctionAlways];
    ags->alpha_depth_stencil_state = [with_metal_device
        newDepthStencilStateWithDescriptor:alpha_depth_descriptor];
    
    if (Error != NULL)
    {
        log_append(
            [[Error localizedDescription]
                cStringUsingEncoding:kCFStringEncodingASCII]);
        #ifndef LOGGER_IGNORE_ASSERTS
        log_dump_and_crash("Error setting the depth stencil state\n");
        #endif
        
        T1_std_strcpy_cap(
            error_msg_string,
            512,
            "Failed to load the depth stencil shader");
        return false;
    }
    
    for (
        uint32_t buf_i = 0;
        buf_i < MAX_RENDERING_FRAME_BUFFERS;
        buf_i++)
    {
        id<MTLBuffer> MTLBufferFramePolygons =
            [with_metal_device
                /* the pointer needs to be page aligned */
                    newBufferWithBytesNoCopy:
                        gpu_shared_data_collection->
                            triple_buffers[buf_i].
                                zsprite_list->polygons
                /* the length weirdly needs to be page aligned also */
                    length:
                        gpu_shared_data_collection->polygons_allocation_size
                    options:
                        MTLResourceStorageModeShared
                /* deallocator = nil to opt out */
                    deallocator:
                        nil];
        ags->polygon_buffers[buf_i] = MTLBufferFramePolygons;
                
        id<MTLBuffer> MTLBufferFrameVertices =
            [with_metal_device
                /* the pointer needs to be page aligned */
                    newBufferWithBytesNoCopy:
                        gpu_shared_data_collection->
                            triple_buffers[buf_i].verts
                /* the length weirdly needs to be page aligned also */
                    length:
                        gpu_shared_data_collection->vertices_allocation_size
                    options:
                        MTLResourceStorageModeShared
                /* deallocator = nil to opt out */
                    deallocator:
                        nil];
        
        ags->vertex_buffers[buf_i] = MTLBufferFrameVertices;
        
        #if RAW_SHADER_ACTIVE
        id<MTLBuffer> MTLBufferLineVertices =
            [with_metal_device
                /* the pointer needs to be page aligned */
                    newBufferWithBytesNoCopy:
                        gpu_shared_data_collection->
                            triple_buffers[buf_i].line_vertices
                /* the length weirdly needs to be page aligned also */
                    length:
                        gpu_shared_data_collection->
                            line_vertices_allocation_size
                    options:
                        MTLResourceStorageModeShared
                /* deallocator = nil to opt out */
                    deallocator:
                        nil];
        
        ags->line_vertex_buffers[buf_i] = MTLBufferLineVertices;
        
        id<MTLBuffer> MTLBufferPointVertices =
            [with_metal_device
                /* the pointer needs to be page aligned */
                    newBufferWithBytesNoCopy:
                        gpu_shared_data_collection->
                            triple_buffers[buf_i].point_vertices
                /* the length weirdly needs to be page aligned also */
                    length:
                        gpu_shared_data_collection->
                            point_vertices_allocation_size
                    options:
                        MTLResourceStorageModeShared
                /* deallocator = nil to opt out */
                    deallocator:
                        nil];
        
        ags->point_vertex_buffers[buf_i] = MTLBufferPointVertices;
        #endif
        
        id<MTLBuffer> MTLBufferPostProcessingConstants =
            [with_metal_device
                /* the pointer needs to be page aligned */
                    newBufferWithBytesNoCopy:
                        gpu_shared_data_collection->
                            triple_buffers[buf_i].postproc_consts
                /* the length weirdly needs to be page aligned also */
                    length:
                        gpu_shared_data_collection->
                            postprocessing_constants_allocation_size
                    options:
                        MTLResourceStorageModeShared
                /* deallocator = nil to opt out */
                    deallocator:
                        nil];
        assert(MTLBufferPostProcessingConstants != nil);
        
        ags->postprocessing_constants_buffers[buf_i] =
            MTLBufferPostProcessingConstants;
        
        id<MTLBuffer> MTLBufferFrameLights =
            [with_metal_device
                /* the pointer needs to be page aligned */
                    newBufferWithBytesNoCopy:
                        gpu_shared_data_collection->
                            triple_buffers[buf_i].lights
                /* the length weirdly needs to be page aligned also */
                    length:
                        gpu_shared_data_collection->lights_allocation_size
                    options:
                        MTLResourceStorageModeShared | MTLResourceUsageRead
                /* deallocator = nil to opt out */
                    deallocator:
                        nil];
        
        ags->light_buffers[buf_i] = MTLBufferFrameLights;
        
        id<MTLBuffer> MTLBufferFrameCamera =
            [with_metal_device
                /* the pointer needs to be page aligned */
                    newBufferWithBytesNoCopy:
                        gpu_shared_data_collection->
                            triple_buffers[buf_i].camera
                /* the length weirdly needs to be page aligned also */
                    length:
                        gpu_shared_data_collection->camera_allocation_size
                    options:
                        MTLResourceStorageModeShared
                /* deallocator = nil to opt out */
                    deallocator:
                        nil];
        
        ags->camera_buffers[buf_i] = MTLBufferFrameCamera;
    }
    
    id<MTLBuffer> MTLBufferLockedVerticesPopulator =
        [with_metal_device
            /* the pointer needs to be page aligned */
                newBufferWithBytesNoCopy:
                    gpu_shared_data_collection->locked_vertices
            /* the length weirdly needs to be page aligned also */
                length:
                    gpu_shared_data_collection->locked_vertices_allocation_size
                options:
                    MTLResourceStorageModeShared
            /* deallocator = nil to opt out */
                deallocator:
                    nil];
    
    ags->locked_vertex_populator_buffer = MTLBufferLockedVerticesPopulator;
    
    id<MTLBuffer> MTLBufferLockedVertices =
        [with_metal_device
            newBufferWithLength:
                gpu_shared_data_collection->locked_vertices_allocation_size
            options:
                MTLResourceStorageModePrivate];
    ags->locked_vertex_buffer = MTLBufferLockedVertices;
    
    
    id<MTLBuffer> MTLBufferLockedMaterialsPopulator =
        [with_metal_device
            /* the pointer needs to be page aligned */
                newBufferWithBytesNoCopy:
                    gpu_shared_data_collection->const_mats
            /* the length weirdly needs to be page aligned also */
                length:
                    gpu_shared_data_collection->const_mats_allocation_size
                options:
                    MTLResourceStorageModeShared
            /* deallocator = nil to opt out */
                deallocator:
                    nil];
    
    ags->locked_materials_populator_buffer = MTLBufferLockedMaterialsPopulator;
    
    id<MTLBuffer> MTLBufferLockedMaterials =
        [with_metal_device
            newBufferWithLength:
                gpu_shared_data_collection->const_mats_allocation_size
            options:
                MTLResourceStorageModePrivate];
    ags->locked_materials_buffer = MTLBufferLockedMaterials;
    
    
    id<MTLBuffer> MTLBufferProjectionConstants =
        [with_metal_device
            /* the pointer needs to be page aligned */
                 newBufferWithBytesNoCopy:
                     gpu_shared_data_collection->locked_pjc
            /* the length weirdly needs to be page aligned also */
                length:
                    gpu_shared_data_collection->
                        projection_constants_allocation_size
                options:
                    MTLResourceStorageModeShared
            /* deallocator = nil to opt out */
                deallocator:
                    nil];
    
    ags->projection_constants_buffer = MTLBufferProjectionConstants;
    
    // TODO: check this again
    // common_memset_char(ags->metal_textures, 0, sizeof(ags->metal_textures));
    
    
    #define FLVERT 1.0f
    #define TEX_MAX 1.0f
    #define TEX_MIN 0.0f
    ags->quad_vertices[0].position[0] =   FLVERT;
    ags->quad_vertices[0].position[1] =  -FLVERT;
    ags->quad_vertices[0].texcoord[0] =  TEX_MAX;
    ags->quad_vertices[0].texcoord[1] =  TEX_MAX;
    
    ags->quad_vertices[1].position[0] =  -FLVERT;
    ags->quad_vertices[1].position[1] =  -FLVERT;
    ags->quad_vertices[1].texcoord[0] =  TEX_MIN;
    ags->quad_vertices[1].texcoord[1] =  TEX_MAX;
    
    ags->quad_vertices[2].position[0] =  -FLVERT;
    ags->quad_vertices[2].position[1] =   FLVERT;
    ags->quad_vertices[2].texcoord[0] =  TEX_MIN;
    ags->quad_vertices[2].texcoord[1] =  TEX_MIN;
    
    ags->quad_vertices[3].position[0] =   FLVERT;
    ags->quad_vertices[3].position[1] =  -FLVERT;
    ags->quad_vertices[3].texcoord[0] =  TEX_MAX;
    ags->quad_vertices[3].texcoord[1] =  TEX_MAX;
    
    ags->quad_vertices[4].position[0] =  -FLVERT;
    ags->quad_vertices[4].position[1] =   FLVERT;
    ags->quad_vertices[4].texcoord[0] =  TEX_MIN;
    ags->quad_vertices[4].texcoord[1] =  TEX_MIN;
    
    ags->quad_vertices[5].position[0] =   FLVERT;
    ags->quad_vertices[5].position[1] =   FLVERT;
    ags->quad_vertices[5].texcoord[0] =  TEX_MAX;
    ags->quad_vertices[5].texcoord[1] =  TEX_MIN;
    
    #if BLOOM_ACTIVE
    id<MTLFunction> downsample_func =
        [ags->lib newFunctionWithName: @"downsample_texture"];
    
    ags->downsample_compute_pls =
        [ags->device
            newComputePipelineStateWithFunction:downsample_func
            error:nil];
    
    id<MTLFunction> boxblur_func =
        [ags->lib newFunctionWithName: @"boxblur_texture"];
    ags->boxblur_compute_pls =
        [ags->device
            newComputePipelineStateWithFunction:boxblur_func
            error:nil];
    
    id<MTLFunction> threshold_func =
        [ags->lib newFunctionWithName: @"threshold_texture"];
    
    ags->thres_compute_pls =
        [ags->device
            newComputePipelineStateWithFunction:threshold_func
            error:nil];
    #endif
    
    id<MTLFunction> singlequad_vertex_shader =
        [ags->lib newFunctionWithName:
            @"single_quad_vertex_shader"];
    if (singlequad_vertex_shader == NULL) {
        log_append("Missing function: postprocess_vertex_shader()!");
        
        T1_std_strcpy_cap(
            error_msg_string,
            512,
            "Missing function: postprocess_vertex_shader()");
        return false;
    }
    
    id<MTLFunction> singlequad_fragment_shader =
        [ags->lib
            newFunctionWithName: @"single_quad_fragment_shader"];
    
    if (singlequad_fragment_shader == NULL) {
        log_append("Missing function: downsampling_fragment_shader()!");
        T1_std_strcpy_cap(
            error_msg_string,
            512,
            "Missing function: downsampling_fragment_shader()");
        return false;
    }
    
    MTLRenderPipelineDescriptor * singlequad_pipeline_descriptor =
        [[MTLRenderPipelineDescriptor alloc] init];
    
    // Set up pipeline for rendering the texture to the screen with a simple
    // quad
    singlequad_pipeline_descriptor.label = @"single-quad pipeline";
    // singlequad_pipeline_descriptor.sampleCount = 1;
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
    ags->singlequad_pls = [
        with_metal_device
        newRenderPipelineStateWithDescriptor:singlequad_pipeline_descriptor
        error:NULL];
    
    ags->command_queue = [with_metal_device newCommandQueue];
    
    ags->metal_active = true;
    
    return true;
}

#if BLOOM_ACTIVE
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
#endif

void platform_gpu_get_device_name(
    char * recipient,
    const uint32_t recipient_cap)
{
    #ifdef LOGGER_IGNORE_ASSERTS
    (void)recipient_cap;
    #endif
    
    const char * device_name_cstr =
        [[ags->device name] cStringUsingEncoding:NSASCIIStringEncoding];
    T1_std_strcpy_cap(
        recipient,
        recipient_cap,
        device_name_cstr);
}

uint32_t platform_get_cpu_logical_core_count(void) {
    NSUInteger core_count = [
        [NSProcessInfo processInfo] activeProcessorCount];
    return (core_count > 0) ? (unsigned int)core_count : 1;
}

int32_t platform_gpu_get_touchable_id_at_screen_pos(
    const float screen_x,
    const float screen_y)
{
    if (
        screen_x < 0 ||
        screen_y < 0 ||
        screen_x >= (engine_globals->window_width) ||
        screen_y >= (engine_globals->window_height))
    {
        return -1;
    }
    
    uint32_t rtt_width  =
        (uint32_t)ags->render_target_texture.width;
    uint32_t rtt_height =
        (uint32_t)ags->render_target_texture.height;
    
    uint32_t screen_x_adj = (uint32_t)(
        (screen_x / engine_globals->window_width) *
            rtt_width);
    uint32_t screen_y_adj = (uint32_t)(
        (screen_y / engine_globals->window_height) *
            rtt_height);
    
    if (screen_x_adj >= rtt_width ) { screen_x_adj = rtt_width;  }
    if (screen_y_adj >= rtt_height) { screen_y_adj = rtt_height; }
    
    screen_y_adj = rtt_height - screen_y_adj;
    
    uint16_t * data = (uint16_t *)[ags->touch_id_buffer contents];
    uint64_t size = [ags->touch_id_buffer allocatedSize];
    
    uint32_t pixel_i = (screen_y_adj * rtt_width) + screen_x_adj;
    if (
        ((pixel_i * 4) + 3) >= (size / 2))
    {
        return -1;
    }
    
    uint16_t first_8bits = data[(pixel_i*4)+0] & 0xFF;
    uint16_t second_8bits = data[(pixel_i*4)+1] & 0xFF;
    uint16_t third_8bits = data[(pixel_i*4)+2] & 0xFF;
    uint16_t fourth_8bits = data[(pixel_i*4)+3] & 0xFF;
    
    uint32_t first_8  = first_8bits;  // red channel, see shaders
    uint32_t second_8 = second_8bits; // green channel
    uint32_t third_8  = third_8bits;  // blue channel
    uint32_t fourth_8 = fourth_8bits; // alpha channel
    
    uint32_t uid = (fourth_8 << 24) | (third_8 << 16) | (second_8 << 8) | first_8;
    int32_t final_id = *(int32_t *)&uid; // Direct reinterpretation
    
    if (final_id < -1) { final_id = -1; }
    
    return final_id;
}

void platform_gpu_init_texture_array(
    const int32_t texture_array_i,
    const uint32_t num_images,
    const uint32_t single_image_width,
    const uint32_t single_image_height,
    const bool32_t use_bc1_compression)
{
    assert(texture_array_i >=  0);
    assert(texture_array_i <  31);
    
    if (ags->metal_textures[texture_array_i] != NULL) {
        return;
    }
    
    MTLTextureDescriptor * texture_descriptor =
        [[MTLTextureDescriptor alloc] init];
    texture_descriptor.textureType = MTLTextureType2DArray;
    texture_descriptor.arrayLength = num_images;
    texture_descriptor.pixelFormat = use_bc1_compression ?
        MTLPixelFormatBC1_RGBA :
        MTLPixelFormatRGBA8Unorm;
    texture_descriptor.storageMode = MTLStorageModePrivate;
    texture_descriptor.width = single_image_width;
    texture_descriptor.height = single_image_height;
    texture_descriptor.mipmapLevelCount =
        use_bc1_compression || texture_array_i == 0 ?
        1 :
        (NSUInteger)floor(
            log2((double)MAX(
                single_image_width,
                single_image_height))) + 1;
    id<MTLTexture> texture = [ags->device
        newTextureWithDescriptor:texture_descriptor];
    
    log_assert(ags->metal_textures[texture_array_i] == NULL);
    ags->metal_textures[texture_array_i] = texture;
}

void platform_gpu_fetch_rgba_at(
    const int32_t texture_array_i,
    const int32_t texture_i,
    uint8_t * rgba_recipient,
    uint32_t * recipient_size,
    uint32_t * recipient_width,
    uint32_t * recipient_height,
    const uint32_t recipient_cap,
    uint32_t * good)
{
    // Validate inputs
    log_assert(texture_i >= 0);
    log_assert(texture_array_i >= 0);
    log_assert(rgba_recipient != NULL);
    log_assert(recipient_size != NULL);
    log_assert(good != NULL);
    
    *good = false;
    
    // Check if the texture array exists
    id<MTLTexture> texture = ags->metal_textures[texture_array_i];
    if (texture == nil || texture.textureType != MTLTextureType2DArray) {
        return;
    }

    if (texture.pixelFormat != MTLPixelFormatRGBA8Unorm) {
        // Ensure the texture format is RGBA8Unorm for direct copying to uint8_t RGBA
        return;
    }
    
    *recipient_width = (uint32_t)texture.width;
    *recipient_height = (uint32_t)texture.height;
    if (*recipient_width < 1 || *recipient_height < 1) {
        return;
    }
    
    // Calculate required buffer size
    NSUInteger bytes_per_row = texture.width * 4; // 4 bytes per pixel (RGBA)
    NSUInteger bytes_per_image = bytes_per_row * texture.height;
    if (recipient_cap < bytes_per_image) {
        *recipient_size = 0;
        return;
    }
    
    // Create a temporary buffer for the copy
    id<MTLBuffer> temp_buffer = [ags->device
        newBufferWithLength: bytes_per_image
        options: MTLResourceStorageModeShared];
    
    if (temp_buffer == nil) {
        return;
    }
    
    if (texture_i >= (int32_t)texture.arrayLength) {
        return;
    }
    
    // Create command buffer and blit encoder
    id<MTLCommandBuffer> command_buffer = [ags->command_queue commandBuffer];
    id<MTLBlitCommandEncoder> blit_encoder = [command_buffer blitCommandEncoder];
    
    // Copy from texture to buffer
    [blit_encoder
        copyFromTexture: texture
        sourceSlice: (NSUInteger)texture_i
        sourceLevel: 0
        sourceOrigin: MTLOriginMake(0, 0, 0)
        sourceSize: MTLSizeMake(texture.width, texture.height, 1)
        toBuffer: temp_buffer
        destinationOffset: 0
        destinationBytesPerRow: bytes_per_row
        destinationBytesPerImage: bytes_per_image];
    
    [blit_encoder endEncoding];

    // Add completion handler to copy data to rgba_recipient
    [command_buffer addCompletedHandler:^(id<MTLCommandBuffer> cb) {
        if (cb.error == nil) {
            // Copy buffer contents to rgba_recipient
            memcpy(rgba_recipient, [temp_buffer contents], bytes_per_image);
            *good = true;
        }
        // Release the temporary buffer
        [temp_buffer setPurgeableState:MTLPurgeableStateEmpty];
    }];
    
    // Commit the command buffer
    [command_buffer commit];
    [command_buffer waitUntilCompleted];
    
    *recipient_size = (uint32_t)bytes_per_image;
    
    *good = true;
}

#if MIPMAPS_ACTIVE
void platform_gpu_generate_mipmaps_for_texture_array(
    const int32_t texture_array_i)
{
    // no mipmaps for font
    log_assert(texture_array_i != 0);
    // no mipmaps for bc1 compressed arrays
    log_assert(!T1_texture_arrays[texture_array_i].bc1_compressed);
    
    id <MTLCommandBuffer> combuf = [ags->command_queue commandBuffer];
    
    // Create a blit command encoder
    id<MTLBlitCommandEncoder> blit_mipmap_encoder = [combuf blitCommandEncoder];
    
    // Generate mipmaps
    [blit_mipmap_encoder
        generateMipmapsForTexture:
            ags->metal_textures[texture_array_i]
        ];
    
    [blit_mipmap_encoder endEncoding];
    
    // Add a completion handler and commit the command buffer.
    [combuf addCompletedHandler:^(id<MTLCommandBuffer> cb) {
        // Populate private buffer.
        (void)cb;
    }];
    [combuf commit];
    [combuf waitUntilCompleted];
}
#endif

void platform_gpu_push_texture_slice_and_free_rgba_values(
    const int32_t texture_array_i,
    const int32_t texture_i,
    const uint32_t parent_texture_array_images_size,
    const uint32_t image_width,
    const uint32_t image_height,
    uint8_t * rgba_values_freeable,
    uint8_t * rgba_values_page_aligned)
{
    (void)parent_texture_array_images_size;
    
    log_assert(rgba_values_freeable != NULL);
    log_assert(rgba_values_page_aligned != NULL);
    
    log_assert(texture_i >= 0);
    log_assert(texture_array_i >= 0);
    
    if (ags->metal_textures[texture_array_i] == NULL) {
        #ifndef LOGGER_IGNORE_ASSERTS
        char errmsg[256];
        T1_std_strcpy_cap(
            errmsg,
            256,
            "Tried to update uninitialized texturearray")
        T1_std_strcat_int_cap(errmsg, 256, texture_array_i);
        T1_std_strcat_cap(errmsg, 256, "\n");
        
        log_dump_and_crash(errmsg);
        #endif
        return;
    }
    
    id<MTLBuffer> temp_buf =
        [ags->device
            /* the pointer needs to be page aligned */
            newBufferWithBytesNoCopy:
                rgba_values_page_aligned
            /* the length weirdly needs to be page aligned also */
            length:
                image_width * image_height * 4
            options:
                MTLResourceStorageModeShared
            /* deallocator = nil to opt out */
            deallocator:
                nil];
    
    if (temp_buf == NULL) {
        return;
    }
    
    id <MTLCommandBuffer> combuf = [ags->command_queue commandBuffer];
    
    id <MTLBlitCommandEncoder> blit_copy_encoder =
        [combuf blitCommandEncoder];
    
    [blit_copy_encoder
        copyFromBuffer:
            temp_buf
        sourceOffset:
            0
        sourceBytesPerRow:
            image_width * 4
        sourceBytesPerImage:
            image_width * image_height * 4
        sourceSize:
            MTLSizeMake(image_width, image_height, 1)
        toTexture:
            ags->metal_textures[texture_array_i]
        destinationSlice:
            (NSUInteger)texture_i
        destinationLevel:
            0
        destinationOrigin:
            MTLOriginMake(0, 0, 0)];
    
    [blit_copy_encoder endEncoding];
    
    // Add a completion handler and commit the command buffer.
    [combuf addCompletedHandler:^(id<MTLCommandBuffer> cb) {
        // Populate private buffer.
        (void)cb;
    }];
    [combuf commit];
    [combuf waitUntilCompleted];
    
    T1_mem_free_from_managed(rgba_values_freeable);
}

#if TEXTURES_ACTIVE
void platform_gpu_push_bc1_texture_slice_and_free_bc1_values(
    const int32_t texture_array_i,
    const int32_t texture_i,
    const uint32_t parent_texture_array_images_size,
    const uint32_t image_width,
    const uint32_t image_height,
    uint8_t * raw_bc1_file_freeable,
    uint8_t * raw_bc1_file_page_aligned)
{
    (void)parent_texture_array_images_size;
    
    log_assert(raw_bc1_file_page_aligned != NULL);
    
    log_assert(texture_i >= 0);
    log_assert(texture_array_i >= 1); // 0 is resered for font
    
    if (ags->metal_textures[texture_array_i] == NULL) {
        #ifndef LOGGER_IGNORE_ASSERTS
        char errmsg[256];
        T1_std_strcpy_cap(
            errmsg,
            256,
            "Tried to update uninitialized texturearray")
        T1_std_strcat_int_cap(errmsg, 256, texture_array_i);
        T1_std_strcat_cap(errmsg, 256, "\n");
        
        log_dump_and_crash(errmsg);
        #endif
        return;
    }
    
    id<MTLBuffer> temp_buf =
        [ags->device
            /* the pointer needs to be page aligned */
        newBufferWithBytesNoCopy:
            raw_bc1_file_page_aligned + 128
            /* the length weirdly needs to be page aligned also */
        length:
            ((image_width + 3) / 4) * ((image_height + 3) / 4) * 8
        options:
            MTLResourceStorageModeShared
            /* deallocator = nil to opt out */
            deallocator:
        nil];
    
    id <MTLCommandBuffer> combuf = [ags->command_queue commandBuffer];
    
    id <MTLBlitCommandEncoder> blit_copy_encoder =
        [combuf blitCommandEncoder];
    
    [blit_copy_encoder
        copyFromBuffer:
            temp_buf
        sourceOffset:
            0
        sourceBytesPerRow:
            ((image_width + 3) / 4) * 8
        sourceBytesPerImage:
            ((image_width + 3) / 4) * ((image_height + 3) / 4) * 8
        sourceSize:
            MTLSizeMake(image_width, image_height, 1)
        toTexture:
            ags->metal_textures[texture_array_i]
        destinationSlice:
            (NSUInteger)texture_i
        destinationLevel:
            0
        destinationOrigin:
            MTLOriginMake(0, 0, 0)];
    
    [blit_copy_encoder endEncoding];
    
    // Add a completion handler and commit the command buffer.
    [combuf addCompletedHandler:^(id<MTLCommandBuffer> cb) {
        // Populate private buffer.
        (void)cb;
    }];
    [combuf commit];
    
    T1_mem_free_from_managed(raw_bc1_file_freeable);
}
#endif

void platform_gpu_copy_locked_vertices(void)
{
    gpu_shared_data_collection->locked_vertices_size = all_mesh_vertices->size;
    
    id <MTLCommandBuffer> combuf = [ags->command_queue commandBuffer];
    
    id <MTLBlitCommandEncoder> blit_copy_encoder =
        [combuf blitCommandEncoder];
    [blit_copy_encoder
        copyFromBuffer:
            ags->locked_vertex_populator_buffer
        sourceOffset:
            0
        toBuffer:
            ags->locked_vertex_buffer
        destinationOffset:
            0
        size:
            gpu_shared_data_collection->locked_vertices_allocation_size];
    [blit_copy_encoder endEncoding];
    
    // Add a completion handler and commit the command buffer.
    [combuf addCompletedHandler:^(id<MTLCommandBuffer> cb) {
        // Populate private buffer.
        (void)cb;
    }];
    [combuf commit];
}

void platform_gpu_copy_locked_materials(void)
{
    gpu_shared_data_collection->const_mats_size = all_mesh_materials->size;
    
    id <MTLCommandBuffer> combuf = [ags->command_queue commandBuffer];
    
    id <MTLBlitCommandEncoder> blit_copy_encoder = [combuf blitCommandEncoder];
    [blit_copy_encoder
        copyFromBuffer:
            ags->locked_materials_populator_buffer
        sourceOffset:
            0
        toBuffer:
            ags->locked_materials_buffer
        destinationOffset:
            0
        size:
            gpu_shared_data_collection->const_mats_allocation_size];
    [blit_copy_encoder endEncoding];
    
    // Add a completion handler and commit the command buffer.
    [combuf addCompletedHandler:^(id<MTLCommandBuffer> cb) {
        // Populate private buffer.
        (void)cb;
    }];
    [combuf commit];
}

@implementation MetalKitViewDelegate
{
}
- (void) updateViewport
{
    ags->cached_viewport.originX = 0;
    ags->cached_viewport.originY = 0;
    ags->cached_viewport.width   =
        engine_globals->window_width *
            ags->retina_scaling_factor;
    ags->cached_viewport.height  =
        engine_globals->window_height *
            ags->retina_scaling_factor;
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
    
    ags->render_target_viewport = ags->cached_viewport;
    ags->render_target_viewport.width /= engine_globals->pixelation_div;
    ags->render_target_viewport.height /= engine_globals->pixelation_div;
    
    *gpu_shared_data_collection->locked_pjc =
        engine_globals->project_consts;
    
    MTLTextureDescriptor * touch_id_texture_descriptor =
        [MTLTextureDescriptor new];
    touch_id_texture_descriptor.width =
        (unsigned long)ags->cached_viewport.width /
            engine_globals->pixelation_div;
    touch_id_texture_descriptor.height =
        (unsigned long)ags->cached_viewport.height /
            engine_globals->pixelation_div;
    touch_id_texture_descriptor.pixelFormat = ags->pixel_format_renderpass1;
    touch_id_texture_descriptor.mipmapLevelCount = 1;
    touch_id_texture_descriptor.storageMode = MTLStorageModePrivate;
    touch_id_texture_descriptor.usage =
        MTLTextureUsageRenderTarget |
        MTLTextureUsageShaderRead;
    ags->touch_id_texture =
        [ags->device
            newTextureWithDescriptor: touch_id_texture_descriptor];
    
    #if SHADOWS_ACTIVE
    MTLTextureDescriptor * shadows_texture_descriptor =
        [[MTLTextureDescriptor alloc] init];
    shadows_texture_descriptor.textureType = MTLTextureType2D;
    shadows_texture_descriptor.pixelFormat = MTLPixelFormatDepth32Float;
    shadows_texture_descriptor.width =
        (unsigned long)ags->render_target_viewport.width;
    shadows_texture_descriptor.height =
        (unsigned long)ags->render_target_viewport.height;
    shadows_texture_descriptor.storageMode = MTLStorageModePrivate;
    shadows_texture_descriptor.usage =
        MTLTextureUsageRenderTarget |
        MTLTextureUsageShaderRead;
    
    ags->shadows_texture =
        [ags->device newTextureWithDescriptor: shadows_texture_descriptor];
    #endif
    
    MTLTextureDescriptor * camera_depth_texture_descriptor =
        [[MTLTextureDescriptor alloc] init];
    camera_depth_texture_descriptor.textureType = MTLTextureType2D;
    camera_depth_texture_descriptor.pixelFormat = MTLPixelFormatDepth32Float;
    camera_depth_texture_descriptor.width =
        (unsigned long)ags->render_target_viewport.width;
    camera_depth_texture_descriptor.height =
        (unsigned long)ags->render_target_viewport.height;
    camera_depth_texture_descriptor.storageMode = MTLStorageModePrivate;
    camera_depth_texture_descriptor.usage =
        MTLTextureUsageRenderTarget |
        MTLTextureUsageShaderRead;
    
    ags->camera_depth_texture =
        [ags->device newTextureWithDescriptor: camera_depth_texture_descriptor];
    
    uint64_t touch_buffer_size_bytes =
        touch_id_texture_descriptor.width *
            touch_id_texture_descriptor.height *
            8;
    
    ags->touch_id_buffer = [ags->device
        newBufferWithLength:
            touch_buffer_size_bytes
        options:
            MTLResourceStorageModeShared];
    
    ags->touch_id_buffer_all_zeros = [ags->device
        newBufferWithLength:
            touch_buffer_size_bytes
        options:
            MTLResourceStorageModeShared];
    int32_t minus_one = -1;
    T1_std_memset_i32(
        ags->touch_id_buffer_all_zeros.contents,
        minus_one,
        (uint32_t)touch_buffer_size_bytes);
    // Set up a texture for rendering to and apply post-processing to
    MTLTextureDescriptor * render_target_texture_desc =
        [MTLTextureDescriptor new];
    render_target_texture_desc.textureType = MTLTextureType2D;
    render_target_texture_desc.width =
        (unsigned long)ags->render_target_viewport.width;
    render_target_texture_desc.height =
        (unsigned long)ags->render_target_viewport.height;
    render_target_texture_desc.pixelFormat = MTLPixelFormatRGBA16Float;
    render_target_texture_desc.storageMode = MTLStorageModePrivate;
    render_target_texture_desc.usage =
        MTLTextureUsageRenderTarget |
        MTLTextureUsageShaderWrite |
        MTLTextureUsageShaderRead;
    render_target_texture_desc.mipmapLevelCount = 1;
    
    ags->render_target_texture = [ags->device
        newTextureWithDescriptor: render_target_texture_desc];
    
    #if BLOOM_ACTIVE
    for (uint32_t i = 0; i < DOWNSAMPLES_SIZE; i++) {
        
        MTLTextureDescriptor * downsampled_target_texture_desc =
            [MTLTextureDescriptor new];
        downsampled_target_texture_desc.textureType = MTLTextureType2D;
        downsampled_target_texture_desc.width = (NSUInteger)get_ds_width(i);
        downsampled_target_texture_desc.height = (NSUInteger)get_ds_height(i);
        downsampled_target_texture_desc.pixelFormat = MTLPixelFormatRGBA16Float;
        downsampled_target_texture_desc.mipmapLevelCount = 1;
        downsampled_target_texture_desc.storageMode = MTLStorageModePrivate;
        downsampled_target_texture_desc.usage =
            MTLTextureUsageShaderWrite |
            MTLTextureUsageShaderRead;
        ags->downsampled_target_textures[i] = [ags->device
            newTextureWithDescriptor: downsampled_target_texture_desc];
    }
    #endif
    
    ags->viewport_set = true;
}

- (void)drawInMTKView:(MTKView *)view
{
    #if DRAWING_SEMAPHORE_ACTIVE
    dispatch_semaphore_wait(ags->drawing_semaphore, DISPATCH_TIME_FOREVER);
    #endif
    
    funcptr_shared_gameloop_update(
        &gpu_shared_data_collection->triple_buffers[ags->frame_i]);
    
    if (!ags || !ags->metal_active || !ags->viewport_set) {
        return;
    }
    
    id<MTLCommandBuffer> command_buffer = [ags->command_queue commandBuffer];
    
    if (command_buffer == nil) {
        #ifndef LOGGER_IGNORE_ASSERTS
        log_dump_and_crash("error - failed to get metal command buffer\n");
        #endif
        
        return;
    }
    
    uint32_t diamond_verts_size =
        gpu_shared_data_collection->
            triple_buffers[ags->frame_i].first_alphablend_i;
    log_assert(
        diamond_verts_size <= gpu_shared_data_collection->
            triple_buffers[ags->frame_i].verts_size);
    
    #if SHADOWS_ACTIVE
    if (
        engine_globals->draw_triangles &&
        diamond_verts_size > 0 &&
        gpu_shared_data_collection->triple_buffers[ags->frame_i].
            postproc_consts->shadowcaster_i <
                gpu_shared_data_collection->triple_buffers[ags->frame_i].
                    postproc_consts->lights_size)
    {
        assert(ags->shadows_texture != NULL);
        MTLRenderPassDescriptor * render_pass_shadows =
            [MTLRenderPassDescriptor new];
        render_pass_shadows.depthAttachment.loadAction = MTLLoadActionClear;
        render_pass_shadows.depthAttachment.storeAction = MTLStoreActionStore;
        render_pass_shadows.depthAttachment.clearDepth = 1.0f;
        render_pass_shadows.depthAttachment.texture = ags->shadows_texture;
        id<MTLRenderCommandEncoder> shadow_pass_encoder =
            [command_buffer
                renderCommandEncoderWithDescriptor:
                    render_pass_shadows];
        
        [shadow_pass_encoder
            setViewport:ags->render_target_viewport];
        
        [shadow_pass_encoder
            setVertexBuffer:
                ags->vertex_buffers[ags->frame_i]
            offset:
                0
            atIndex:
                0];
        
        [shadow_pass_encoder
            setVertexBuffer:
                ags->polygon_buffers[ags->frame_i]
            offset:
                0
            atIndex:
                1];
        
        [shadow_pass_encoder
            setVertexBuffer:
                ags->light_buffers[ags->frame_i]
            offset:
                0
            atIndex:
                2];
        
        [shadow_pass_encoder
            setVertexBuffer:
                ags->camera_buffers[ags->frame_i]
            offset: 0
            atIndex: 3];
        
        [shadow_pass_encoder
            setVertexBuffer:
                ags->locked_vertex_buffer
            offset:
                0
            atIndex:
                4];
        
        [shadow_pass_encoder
            setVertexBuffer:
                ags->projection_constants_buffer
            offset:
                0
            atIndex:
                5];
        
        [shadow_pass_encoder
            setVertexBuffer:
                ags->postprocessing_constants_buffers[ags->frame_i]
            offset:
                0
            atIndex:
                6];
        
        [shadow_pass_encoder
            setRenderPipelineState:ags->shadows_pls];
        [shadow_pass_encoder
            setDepthStencilState: ags->opaque_depth_stencil_state];
        [shadow_pass_encoder
            drawPrimitives: MTLPrimitiveTypeTriangle
            vertexStart: 0
            vertexCount: diamond_verts_size];
        
        [shadow_pass_encoder endEncoding];
    }
    #endif
        
    // To kick off our render loop, we blit to clear the touch id buffer
    uint32_t size_bytes = (uint32_t)(
        [ags->touch_id_texture height] *
        [ags->touch_id_texture width] *
        8);
    
    id <MTLBlitCommandEncoder> clear_touch_texture_blit_encoder =
        [command_buffer blitCommandEncoder];
    
    [clear_touch_texture_blit_encoder
        copyFromBuffer: ags->touch_id_buffer_all_zeros
        sourceOffset: 0
        sourceBytesPerRow: [ags->touch_id_texture width] * 8
        sourceBytesPerImage: size_bytes
        sourceSize:
            MTLSizeMake(
                [ags->touch_id_texture width],
                [ags->touch_id_texture height],
                1)
        toTexture: ags->touch_id_texture
        destinationSlice: 0
        destinationLevel: 0
        destinationOrigin: MTLOriginMake(0, 0, 0)];
    
    [clear_touch_texture_blit_encoder endEncoding];
    
    
    MTLRenderPassDescriptor * all_triangles_descriptor =
    [view currentRenderPassDescriptor];
    
    all_triangles_descriptor.depthAttachment.loadAction =
        MTLLoadActionClear;
    all_triangles_descriptor.depthAttachment.clearDepth = 1.0f;
    all_triangles_descriptor.depthAttachment.storeAction =
        MTLStoreActionStore;
    all_triangles_descriptor.depthAttachment.texture =
        ags->camera_depth_texture;
    
    assert(ags->render_target_texture != NULL);
    all_triangles_descriptor.colorAttachments[0].texture =
        ags->render_target_texture;
    all_triangles_descriptor.colorAttachments[0].storeAction =
        MTLStoreActionStore;
    
    all_triangles_descriptor.colorAttachments[0].clearColor =
        MTLClearColorMake(0.0f, 0.03f, 0.15f, 1.0f);
    
    // ID Buffer for touchables
    all_triangles_descriptor.colorAttachments[1].texture =
        ags->touch_id_texture;
    all_triangles_descriptor.colorAttachments[1].loadAction =
        MTLLoadActionLoad; // We clear manually with a blit before this pass
    all_triangles_descriptor.colorAttachments[1].storeAction =
        MTLStoreActionStore;
    
    id<MTLRenderCommandEncoder> render_pass_1_draw_triangles_encoder =
        [command_buffer
            renderCommandEncoderWithDescriptor:
                all_triangles_descriptor];

    
    assert(ags->cached_viewport.zfar > ags->cached_viewport.znear);
    [render_pass_1_draw_triangles_encoder
        setViewport: ags->render_target_viewport];
    assert(ags->cached_viewport.width > 0.0f);
    assert(ags->cached_viewport.height > 0.0f);
    
    
    [render_pass_1_draw_triangles_encoder
        setRenderPipelineState: ags->diamond_pls];
    assert(ags->opaque_depth_stencil_state != nil);
    [render_pass_1_draw_triangles_encoder
        setDepthStencilState: ags->opaque_depth_stencil_state];
    [render_pass_1_draw_triangles_encoder
        setDepthClipMode: MTLDepthClipModeClip];
    
    [render_pass_1_draw_triangles_encoder
        setVertexBuffer:
            ags->vertex_buffers[ags->frame_i]
        offset:
            0
        atIndex:
            0];
    
    [render_pass_1_draw_triangles_encoder
        setVertexBuffer:
            ags->polygon_buffers[ags->frame_i]
        offset:
            0
        atIndex:
            1];
    
    [render_pass_1_draw_triangles_encoder
        setVertexBuffer:
            ags->camera_buffers[ags->frame_i]
        offset: 0
        atIndex: 3];
    
    [render_pass_1_draw_triangles_encoder
        setVertexBuffer:
            ags->locked_vertex_buffer
        offset:
            0 
        atIndex:
            4];
    
    [render_pass_1_draw_triangles_encoder
        setVertexBuffer:
            ags->projection_constants_buffer
        offset:
            0
        atIndex:
            5];
    
    [render_pass_1_draw_triangles_encoder
        setFragmentBuffer:
            ags->locked_vertex_buffer
        offset:
            0
        atIndex:
            0];
    
    [render_pass_1_draw_triangles_encoder
        setFragmentBuffer:
            ags->polygon_buffers[ags->frame_i]
        offset:
            0
        atIndex:
            1];
    
    [render_pass_1_draw_triangles_encoder
        setFragmentBuffer:
            ags->light_buffers[ags->frame_i]
        offset:
            0
        atIndex:
            2];
    
    [render_pass_1_draw_triangles_encoder
        setFragmentBuffer:
            ags->camera_buffers[ags->frame_i]
        offset:
            0
        atIndex:
            3];
    
    [render_pass_1_draw_triangles_encoder
        setFragmentBuffer:
            ags->projection_constants_buffer
        offset:
            0
        atIndex:
            4];
    
    [render_pass_1_draw_triangles_encoder
        setFragmentBuffer:
            ags->locked_materials_buffer
        offset:
            0
        atIndex:
            6];
    
    [render_pass_1_draw_triangles_encoder
        setFragmentBuffer:
            ags->postprocessing_constants_buffers[ags->frame_i]
        offset:
            0
        atIndex:
            7];
    
    #if TEXTURES_ACTIVE
    for (
        uint32_t i = 0;
        i < TEXTUREARRAYS_SIZE;
        i++)
    {
        if (ags->metal_textures[i] != NULL) {
            [render_pass_1_draw_triangles_encoder
                setFragmentTexture: ags->metal_textures[i]
                atIndex: i];
        }
    }
    #else
    [render_pass_1_draw_triangles_encoder
        setFragmentTexture: ags->metal_textures[0]
        atIndex: 0];
    #endif
    
    #if SHADOWS_ACTIVE
    [render_pass_1_draw_triangles_encoder
        setFragmentTexture: ags->shadows_texture
        atIndex: SHADOWMAP_TEXTUREARRAY_I];
    #endif
    
    #ifndef IGNORE_LOGGER_ASSERTS
    for (
        uint32_t i = 0;
        i < gpu_shared_data_collection->
            triple_buffers[ags->frame_i].
            verts_size;
        i++)
    {
        assert(
            gpu_shared_data_collection->
                triple_buffers[ags->frame_i].
                    verts[i].locked_vertex_i < ALL_LOCKED_VERTICES_SIZE);
    }
    #endif
    
    if (engine_globals->draw_triangles && diamond_verts_size > 0) {
        assert(diamond_verts_size < MAX_VERTICES_PER_BUFFER);
        assert(diamond_verts_size % 3 == 0);
        [render_pass_1_draw_triangles_encoder
            drawPrimitives:
                MTLPrimitiveTypeTriangle
            vertexStart:
                0
            vertexCount:
                diamond_verts_size];
    }
    
    log_assert(
        gpu_shared_data_collection->triple_buffers[ags->frame_i].
            first_alphablend_i <=
        gpu_shared_data_collection->triple_buffers[ags->frame_i].
            verts_size);
    uint32_t alphablend_verts_size =
        gpu_shared_data_collection->
            triple_buffers[ags->frame_i].verts_size -
        gpu_shared_data_collection->
            triple_buffers[ags->frame_i].first_alphablend_i;
    
    if (engine_globals->draw_triangles && alphablend_verts_size > 0) {
        //        assert(alphablend_verts_size < MAX_VERTICES_PER_BUFFER);
        //        assert(alphablend_verts_size % 3 == 0);
        [render_pass_1_draw_triangles_encoder setRenderPipelineState:
            ags->alphablend_pls];
        [render_pass_1_draw_triangles_encoder setDepthStencilState:
            ags->alpha_depth_stencil_state];
        
        [render_pass_1_draw_triangles_encoder
            drawPrimitives: MTLPrimitiveTypeTriangle
            vertexStart: gpu_shared_data_collection->
                triple_buffers[ags->frame_i].first_alphablend_i
            vertexCount:
                alphablend_verts_size];
    }
    
    #if RAW_SHADER_ACTIVE
    if ((
        gpu_shared_data_collection->triple_buffers[ags->frame_i].
            line_vertices_size +
        gpu_shared_data_collection->triple_buffers[ags->frame_i].
            point_vertices_size) > 0)
    {
        [render_pass_1_draw_triangles_encoder
            setRenderPipelineState: ags->raw_pls];
        
        assert(ags->depth_stencil_state != nil);
        [render_pass_1_draw_triangles_encoder
            setDepthStencilState: ags->depth_stencil_state];
        [render_pass_1_draw_triangles_encoder
            setDepthClipMode: MTLDepthClipModeClip];
    }
    
    if (gpu_shared_data_collection->
        triple_buffers[ags->frame_i].line_vertices_size > 0)
    {
        [render_pass_1_draw_triangles_encoder
            setVertexBuffer:
                ags->line_vertex_buffers[ags->frame_i]
            offset:
                0
            atIndex:
                0];
        
        //        [render_pass_1_draw_triangles_encoder
        //            setVertexBuffer:
        //                ags->camera_buffers[ags->frame_i]
        //            offset:
        //                0
        //            atIndex:
        //                3];
        //
        //        [render_pass_1_draw_triangles_encoder
        //            setVertexBuffer:
        //                ags->projection_constants_buffer
        //            offset:
        //                0
        //            atIndex:
        //                5];
        
        assert(gpu_shared_data_collection->
            triple_buffers[ags->frame_i].line_vertices_size <=
                MAX_LINE_VERTICES);
        [render_pass_1_draw_triangles_encoder
            drawPrimitives: MTLPrimitiveTypeLine
            vertexStart: 0
            vertexCount: gpu_shared_data_collection->
                triple_buffers[ags->frame_i].line_vertices_size];
    }
    
    if (gpu_shared_data_collection->
        triple_buffers[ags->frame_i].point_vertices_size > 0)
    {
        [render_pass_1_draw_triangles_encoder
            setVertexBuffer:
                ags->point_vertex_buffers[ags->frame_i]
            offset:
                0
            atIndex:
                0];
        assert(gpu_shared_data_collection->
            triple_buffers[ags->frame_i].point_vertices_size <=
                MAX_POINT_VERTICES);
        
        [render_pass_1_draw_triangles_encoder
            drawPrimitives: MTLPrimitiveTypePoint
            vertexStart: 0
            vertexCount: gpu_shared_data_collection->
                triple_buffers[ags->frame_i].point_vertices_size];
    }
    #endif
    
    [render_pass_1_draw_triangles_encoder endEncoding];
    
    // copy the touch id buffer to a CPU buffer so we can query
    // what the top touchable_id is
    id <MTLBlitCommandEncoder> blit_touch_texture_to_cpu_buffer_encoder =
        [command_buffer blitCommandEncoder];
    [blit_touch_texture_to_cpu_buffer_encoder
        copyFromTexture: ags->touch_id_texture
        sourceSlice: 0
        sourceLevel: 0
        sourceOrigin: MTLOriginMake(0, 0, 0)
        sourceSize:
            MTLSizeMake(
                [ags->touch_id_texture width],
                [ags->touch_id_texture height],
                1)
        toBuffer: ags->touch_id_buffer
        destinationOffset: 0
        destinationBytesPerRow: [ags->touch_id_texture width] * 8
        destinationBytesPerImage:
            [ags->touch_id_texture width] * [ags->touch_id_texture height] * 8];
    [blit_touch_texture_to_cpu_buffer_encoder endEncoding];
    
    #if BLOOM_ACTIVE
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
    }
    #endif
    
    // Render pass 4 puts a quad on the full screen
    MTLRenderPassDescriptor * render_pass_4_composition_descriptor =
        [view currentRenderPassDescriptor];
    
    render_pass_4_composition_descriptor.colorAttachments[0].clearColor =
        MTLClearColorMake(0.0f, 0.0f, 0.0f, 1.0f);;
    render_pass_4_composition_descriptor.depthAttachment.loadAction =
        MTLLoadActionClear;
    id<MTLRenderCommandEncoder> render_pass_4_composition =
        [command_buffer
            renderCommandEncoderWithDescriptor:
                render_pass_4_composition_descriptor];
    
    [render_pass_4_composition setViewport: ags->cached_viewport];
    
    [render_pass_4_composition setCullMode:MTLCullModeNone];
    
    [render_pass_4_composition
        setRenderPipelineState: ags->singlequad_pls];
    
    assert(ags->quad_vertices != NULL);
    [render_pass_4_composition
        setVertexBytes:ags->quad_vertices
        length:sizeof(T1PostProcessingVertex)*6
        atIndex:0];
    
    [render_pass_4_composition
        setVertexBuffer:ags->postprocessing_constants_buffers[ags->frame_i]
        offset:0
        atIndex:1];
    
    [render_pass_4_composition
        setFragmentTexture: ags->render_target_texture
        atIndex:0];
    
    #if BLOOM_ACTIVE
    [render_pass_4_composition
        setFragmentTexture: ags->downsampled_target_textures[0]
        atIndex:1];
    [render_pass_4_composition
        setFragmentTexture: ags->downsampled_target_textures[1]
        atIndex:2];
    [render_pass_4_composition
        setFragmentTexture: ags->downsampled_target_textures[2]
        atIndex:3];
    [render_pass_4_composition
        setFragmentTexture: ags->downsampled_target_textures[3]
        atIndex:4];
    [render_pass_4_composition
        setFragmentTexture: ags->downsampled_target_textures[4]
        atIndex:5];
    #endif
    
    int32_t perlin_ta_i = gpu_shared_data_collection->triple_buffers[ags->frame_i].postproc_consts->perlin_texturearray_i;
    #ifndef LOGGER_IGNORE_ASSERTS
    int32_t perlin_t_i =
        gpu_shared_data_collection->triple_buffers[ags->frame_i].postproc_consts->perlin_texture_i;
    #endif
    // log_assert(perlin_ta_i >= 1);
    log_assert(perlin_t_i == 0);
    
    [render_pass_4_composition
        setFragmentTexture: ags->metal_textures[perlin_ta_i]
        atIndex:6];
    
    [render_pass_4_composition
        setFragmentTexture: ags->camera_depth_texture
        atIndex:CAMERADEPTH_TEXTUREARRAY_I];
    
    [render_pass_4_composition
        drawPrimitives:MTLPrimitiveTypeTriangle
        vertexStart:0
        vertexCount:6];
    
    [render_pass_4_composition endEncoding];
    
    // Schedule a present once the framebuffer is complete
    // using the current drawable
    [command_buffer presentDrawable: [view currentDrawable]];
    
    ags->frame_i += 1;
    ags->frame_i %= MAX_RENDERING_FRAME_BUFFERS;
    assert(ags->frame_i < MAX_RENDERING_FRAME_BUFFERS);
    
    [command_buffer addCompletedHandler:^(id<MTLCommandBuffer> arg_cmd_buffer) {
        (void)arg_cmd_buffer;
        
        #if DRAWING_SEMAPHORE_ACTIVE
        dispatch_semaphore_signal(ags->drawing_semaphore);
        #endif
    }];
    
    [command_buffer commit];
    // [command_buffer waitUntilCompleted];
    
    funcptr_shared_gameloop_update_after_render_pass();
}

- (void)mtkView:(MTKView *)view
    drawableSizeWillChange:(CGSize)size
{
}
@end

void platform_gpu_update_viewport(void)
{
    ags->viewport_set = false;
    [apple_gpu_delegate updateViewport];
}
