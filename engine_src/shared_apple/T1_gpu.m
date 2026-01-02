#import "T1_gpu.h"

#define T1_DRAWING_SEMAPHORE_ACTIVE T1_INACTIVE

typedef struct AppleGPUState {
    MTLPixelFormat pixel_format_renderpass1;
    #if T1_DRAWING_SEMAPHORE_ACTIVE == T1_ACTIVE
    dispatch_semaphore_t drawing_semaphore;
    #elif T1_DRAWING_SEMAPHORE_ACTIVE == T1_INACTIVE
    #else
    #error
    #endif
    NSUInteger frame_i;
    MTLViewport window_viewport;
    MTLViewport render_viewports[T1_RENDER_VIEW_CAP];
    
    id<MTLDevice> device;
    id<MTLLibrary> lib;
    id<MTLCommandQueue> command_queue;
    
    id polygon_buffers[MAX_FRAME_BUFFERS];
    id light_buffers [MAX_FRAME_BUFFERS];
    id vertex_buffers[MAX_FRAME_BUFFERS];
    id flat_quad_buffers[MAX_FRAME_BUFFERS];
    id camera_buffers[MAX_FRAME_BUFFERS][T1_RENDER_VIEW_CAP];
    id postprocessing_constants_buffers[MAX_FRAME_BUFFERS];
    id locked_vertex_populator_buffer;
    id locked_vertex_buffer;
    id locked_materials_populator_buffer;
    id locked_materials_buffer;
    id projection_constants_buffer;
    
    id<MTLTexture> camera_depth_texture;
    id<MTLTexture> depth_textures[T1_RENDER_VIEW_CAP];
    
    #if T1_Z_PREPASS_ACTIVE == T1_ACTIVE
    id<MTLRenderPipelineState> z_prepass_pls;
    #elif T1_Z_PREPASS_ACTIVE == T1_INACTIVE
    #else
    #error
    #endif
    
    #if T1_OUTLINES_ACTIVE == T1_ACTIVE
    id<MTLRenderPipelineState> outlines_pls;
    #elif T1_OUTLINES_ACTIVE == T1_INACTIVE
    #else
    #error
    #endif
    id<MTLRenderPipelineState> diamond_touch_pls;
    id<MTLRenderPipelineState> alphablend_touch_pls;
    id<MTLRenderPipelineState> bb_touch_pls;
    id<MTLRenderPipelineState> diamond_notouch_pls;
    id<MTLRenderPipelineState> depth_only_pls;
    id<MTLRenderPipelineState> alphablend_notouch_pls;
    id<MTLRenderPipelineState> bb_notouch_pls;
    
    #if T1_BLOOM_ACTIVE == T1_ACTIVE
    id<MTLComputePipelineState> downsample_compute_pls;
    id<MTLComputePipelineState> boxblur_compute_pls;
    #elif T1_BLOOM_ACTIVE == T1_INACTIVE
    #else
    #error
    #endif
    
    id<MTLRenderPipelineState> singlequad_pls;
    
    id<MTLDepthStencilState> opaque_depth_stencil_state;
    
    // Textures
    // id<MTLBuffer> texture_populator_buffer;
    #if T1_TEXTURES_ACTIVE == T1_ACTIVE
    id<MTLTexture> metal_textures[TEXTUREARRAYS_SIZE];
    #elif T1_TEXTURES_ACTIVE == T1_INACTIVE
    id<MTLTexture> metal_textures[1]; // for font only
    #else
    #error
    #endif
    
    #if T1_BLOOM_ACTIVE == T1_ACTIVE
    id<MTLTexture> downsampled_rtts[DOWNSAMPLES_SIZE];
    #elif T1_BLOOM_ACTIVE == T1_INACTIVE
    #else
    #error
    #endif
    
    id<MTLTexture> touch_id_texture;
    id<MTLBuffer> touch_id_buffer;
    id<MTLBuffer> touch_id_buffer_all_zeros;
    T1PostProcessingVertex quad_vertices[6];
    float retina_scaling_factor;
    bool32_t viewports_set[T1_RENDER_VIEW_CAP];
    bool32_t metal_active;
} AppleGPUState;

static AppleGPUState * ags = NULL;


MetalKitViewDelegate * apple_gpu_delegate = NULL;

static void (* funcptr_gameloop_before_render)(T1GPUFrame *) = NULL;
static void (* funcptr_gameloop_after_render)(void) = NULL;

bool32_t T1_apple_gpu_init(
    void (* arg_funcptr_shared_gameloop_update)(T1GPUFrame *),
    void (* arg_funcptr_shared_gameloop_update_after_render_pass)(void),
    id<MTLDevice> with_metal_device,
    NSString * shader_lib_filepath,
    float backing_scale_factor,
    char * error_msg_string)
{
    if (gpu_shared_data_collection == NULL) {
        T1_std_strcpy_cap(error_msg_string, 128, "GPU frame buffer was not initialized");
        return false;
    }
    
    ags = T1_mem_malloc_from_unmanaged(
        sizeof(AppleGPUState));
    ags->retina_scaling_factor = backing_scale_factor;
    ags->pixel_format_renderpass1 = 0;
    #if T1_DRAWING_SEMAPHORE_ACTIVE == T1_ACTIVE
    ags->drawing_semaphore = NULL; // TODO: remove me
    ags->drawing_semaphore = dispatch_semaphore_create(3);
    #elif T1_DRAWING_SEMAPHORE_ACTIVE == T1_INACTIVE
    #else
    #error
    #endif
    
    funcptr_gameloop_before_render =
        arg_funcptr_shared_gameloop_update;
    funcptr_gameloop_after_render =
        arg_funcptr_shared_gameloop_update_after_render_pass;
    
    ags->pixel_format_renderpass1 = MTLPixelFormatRGBA8Unorm;
    
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
            #if T1_LOGGER_ASSERTS_ACTIVE == T1_ACTIVE
            log_dump_and_crash("Failed to find the shader lib\n");
            #elif T1_LOGGER_ASSERTS_ACTIVE == T1_INACTIVE
            #else
            #error
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
            #if T1_LOGGER_ASSERTS_ACTIVE == T1_ACTIVE
            log_dump_and_crash((char *)[
                [[Error userInfo] descriptionInStringsFileFormat]
                    cStringUsingEncoding:NSASCIIStringEncoding]);
            #elif T1_LOGGER_ASSERTS_ACTIVE == T1_INACTIVE
            // Pass
            #else
            #error
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
        log_append(
            "Missing function: vertex_shader()!");
        
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
    
    #if T1_BLENDING_SHADER_ACTIVE == T1_ACTIVE
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
    #elif T1_BLENDING_SHADER_ACTIVE == T1_INACTIVE
    #else
    #error
    #endif
    
    #if T1_SHADOWS_ACTIVE == T1_ACTIVE
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
    
    MTLRenderPipelineDescriptor * shadows_pls_desc =
        [MTLRenderPipelineDescriptor new];
    [shadows_pls_desc
        setVertexFunction: shadows_vertex_shader];
    [shadows_pls_desc
        setFragmentFunction: nil];
    shadows_pls_desc.depthAttachmentPixelFormat =
        MTLPixelFormatDepth32Float;
    shadows_pls_desc.label = @"shadow pipeline state";
    
    ags->shadows_pls =
       [with_metal_device
            newRenderPipelineStateWithDescriptor:
                shadows_pls_desc
            error:
                &Error];
    #elif T1_SHADOWS_ACTIVE == T1_INACTIVE
    #else
    #error
    #endif
    
    #if T1_Z_PREPASS_ACTIVE == T1_ACTIVE
    id<MTLFunction> z_prepass_vertex_shader =
        [ags->lib newFunctionWithName:
            @"vertex_shader"];
    if (z_prepass_vertex_shader == NULL) {
        T1_std_strcpy_cap(
            error_msg_string,
            512,
            "Missing function: vertex_shader()");
        return false;
    }
    
    id<MTLFunction> z_prepass_fragment_shader =
        [ags->lib newFunctionWithName:
            @"z_prepass_fragment_shader"];
    if (z_prepass_fragment_shader == NULL) {
        log_append("Missing function: z_prepass_fragment_shader()!");
        
        T1_std_strcpy_cap(
            error_msg_string,
            512,
            "Missing function: z_prepass_fragment_shader()");
        return false;
    }
    
    MTLRenderPipelineDescriptor * z_prepass_pls_desc =
        [MTLRenderPipelineDescriptor new];
    [z_prepass_pls_desc
        setVertexFunction: z_prepass_vertex_shader];
    [z_prepass_pls_desc
        setFragmentFunction: z_prepass_fragment_shader];
    z_prepass_pls_desc.depthAttachmentPixelFormat =
        MTLPixelFormatDepth32Float;
    z_prepass_pls_desc.label = @"z prepass pipeline state";
    z_prepass_pls_desc
        .colorAttachments[0]
        .pixelFormat = ags->pixel_format_renderpass1;
    
    ags->z_prepass_pls =
       [with_metal_device
            newRenderPipelineStateWithDescriptor:
                z_prepass_pls_desc
            error:
                &Error];
    #elif T1_Z_PREPASS_ACTIVE == T1_INACTIVE
    #else
    #error
    #endif
    
    #if T1_OUTLINES_ACTIVE == T1_ACTIVE
    id<MTLFunction> outlines_vertex_shader =
        [ags->lib newFunctionWithName:
            @"outlines_vertex_shader"];
    if (outlines_vertex_shader == NULL) {
        T1_std_strcpy_cap(
            error_msg_string,
            512,
            "Missing function: outlines_vertex_shader()");
        return false;
    }
    
    id<MTLFunction> outlines_fragment_shader =
        [ags->lib newFunctionWithName:
            @"outlines_fragment_shader"];
    if (outlines_fragment_shader == NULL) {
        T1_std_strcpy_cap(
            error_msg_string,
            512,
            "Missing function: outlines_fragment_shader()");
        return false;
    }
    
    MTLRenderPipelineDescriptor * outlines_pls_desc =
        [MTLRenderPipelineDescriptor new];
    [outlines_pls_desc
        setVertexFunction: outlines_vertex_shader];
    [outlines_pls_desc
        setFragmentFunction:
            outlines_fragment_shader];
    outlines_pls_desc.label =
        @"outlines pipeline state";
    outlines_pls_desc
        .colorAttachments[0]
        .pixelFormat = ags->pixel_format_renderpass1;
    outlines_pls_desc.
        depthAttachmentPixelFormat =
            MTLPixelFormatDepth32Float;
    ags->outlines_pls =
       [with_metal_device
            newRenderPipelineStateWithDescriptor:
                outlines_pls_desc
            error:
                &Error];
    
    if (Error != nil) {
        log_dump_and_crash((char *)[
            [[Error userInfo] descriptionInStringsFileFormat]
                cStringUsingEncoding:
                    NSASCIIStringEncoding]);
        return false;
    }
    #elif T1_OUTLINES_ACTIVE == T1_INACTIVE
    #else
    #error
    #endif
    
    id<MTLFunction>
        flat_billboard_quad_vert_shader =
            [ags->lib newFunctionWithName:
                @"flat_billboard_quad_vertex_shader"];
    if (
        flat_billboard_quad_vert_shader == NULL)
    {
        T1_std_strcpy_cap(
            error_msg_string,
            512,
            "Missing function: "
            "flat_billboard_quad_vertex_shader()");
        return false;
    }
    
    id<MTLFunction> flat_billboard_quad_frag_shader =
        [ags->lib newFunctionWithName:
            @"flat_billboard_quad_fragment_shader"];
    if (flat_billboard_quad_frag_shader == NULL) {
        T1_std_strcpy_cap(
            error_msg_string,
            512,
            "Missing function: "
            "flat_billboard_quad_fragment_shader()");
        return false;
    }
    
    MTLRenderPipelineDescriptor *
        flat_billboard_quad_pls_desc =
            [MTLRenderPipelineDescriptor new];
    [flat_billboard_quad_pls_desc
        setVertexFunction: flat_billboard_quad_vert_shader];
    [flat_billboard_quad_pls_desc
        setFragmentFunction:
            flat_billboard_quad_frag_shader];
    flat_billboard_quad_pls_desc.label =
        @"flat billboard quad pipeline state";
    flat_billboard_quad_pls_desc
        .colorAttachments[0]
        .pixelFormat = ags->pixel_format_renderpass1;
    [flat_billboard_quad_pls_desc
        .colorAttachments[0]
        setBlendingEnabled: YES];
    flat_billboard_quad_pls_desc
        .colorAttachments[0].sourceRGBBlendFactor =
            MTLBlendFactorOne;
    flat_billboard_quad_pls_desc
        .colorAttachments[0].destinationRGBBlendFactor =
            MTLBlendFactorOne;
    flat_billboard_quad_pls_desc
        .colorAttachments[0].rgbBlendOperation =
            MTLBlendOperationAdd;
    flat_billboard_quad_pls_desc.colorAttachments[1].
        pixelFormat = ags->pixel_format_renderpass1;
    flat_billboard_quad_pls_desc.
        depthAttachmentPixelFormat =
            MTLPixelFormatDepth32Float;
    ags->bb_touch_pls =
       [with_metal_device
            newRenderPipelineStateWithDescriptor:
                flat_billboard_quad_pls_desc
            error:
                &Error];
    
    log_assert(Error == nil);
    
    flat_billboard_quad_pls_desc.
        colorAttachments[1] = nil;
    ags->bb_notouch_pls =
       [with_metal_device
            newRenderPipelineStateWithDescriptor:
                flat_billboard_quad_pls_desc
            error:
                &Error];
    
    // Setup pipeline that uses diamonds instead of alphablending
    MTLRenderPipelineDescriptor * diamond_pls_desc =
        [MTLRenderPipelineDescriptor new];
    [diamond_pls_desc
        setVertexFunction: vertex_shader];
    [diamond_pls_desc
        setFragmentFunction: fragment_shader];
    diamond_pls_desc
        .colorAttachments[0]
        .pixelFormat = ags->pixel_format_renderpass1;
    diamond_pls_desc.colorAttachments[1].pixelFormat =
        ags->pixel_format_renderpass1;
    diamond_pls_desc.depthAttachmentPixelFormat =
        MTLPixelFormatDepth32Float;
    diamond_pls_desc.label =
        @"diamond pipeline state";
    ags->diamond_touch_pls =
        [with_metal_device
            newRenderPipelineStateWithDescriptor:
                diamond_pls_desc 
            error:
                &Error];
    
    if (Error != NULL)
    {
        T1_std_strcpy_cap(
            error_msg_string,
            512,
            "Failed to init diamond pipeline: ");
        T1_std_strcat_cap(
            error_msg_string,
            512,
            [[[Error userInfo] descriptionInStringsFileFormat]
                    cStringUsingEncoding:
                        NSASCIIStringEncoding]);
        #if T1_LOGGER_ASSERTS_ACTIVE == T1_ACTIVE
        log_dump_and_crash(error_msg_string);
        #elif T1_LOGGER_ASSERTS_ACTIVE == T1_INACTIVE
        #else
        #error
        #endif
        
        return false;
    }
    
    diamond_pls_desc.colorAttachments[1] = nil;
    ags->diamond_notouch_pls =
        [with_metal_device
            newRenderPipelineStateWithDescriptor:
                diamond_pls_desc 
            error:
                &Error];
    log_assert(Error == NULL);
    
    diamond_pls_desc.colorAttachments[0] = nil;
    diamond_pls_desc.fragmentFunction = nil;
    ags->depth_only_pls =
        [with_metal_device
            newRenderPipelineStateWithDescriptor:
                diamond_pls_desc 
            error:
                &Error];
    
    #if T1_BLENDING_SHADER_ACTIVE == T1_ACTIVE
    MTLRenderPipelineDescriptor * alpha_pls_desc =
        [[MTLRenderPipelineDescriptor alloc] init];
    [alpha_pls_desc
        setVertexFunction: vertex_shader];
    [alpha_pls_desc
        setFragmentFunction: alphablending_fragment_shader];
    alpha_pls_desc
        .colorAttachments[0]
        .pixelFormat = ags->pixel_format_renderpass1;
    [alpha_pls_desc
        .colorAttachments[0]
        setBlendingEnabled: YES];
    alpha_pls_desc
        .colorAttachments[0].sourceRGBBlendFactor =
            MTLBlendFactorSourceAlpha;
    alpha_pls_desc
        .colorAttachments[0].destinationRGBBlendFactor =
            MTLBlendFactorOneMinusSourceAlpha;
    alpha_pls_desc
        .colorAttachments[0].rgbBlendOperation =
            MTLBlendOperationAdd;
    alpha_pls_desc.colorAttachments[1].
        pixelFormat = ags->pixel_format_renderpass1;
    alpha_pls_desc.depthAttachmentPixelFormat =
        MTLPixelFormatDepth32Float;
    alpha_pls_desc.label = @"Alphablending pipeline";
    ags->alphablend_touch_pls =
        [with_metal_device
            newRenderPipelineStateWithDescriptor:
                alpha_pls_desc
            error:
                &Error];
    
    if (Error != NULL)
    {
        log_append(
            [[Error localizedDescription]
                cStringUsingEncoding:
                    kCFStringEncodingASCII]);
        
        #if T1_LOGGER_ASSERTS_ACTIVE == T1_ACTIVE
        log_dump_and_crash(
            "Error loading the alpha "
            "blending shader\n");
        #elif T1_LOGGER_ASSERTS_ACTIVE == T1_INACTIVE
        #else
        #error
        #endif
        
        T1_std_strcpy_cap(
            error_msg_string,
            512,
            "Failed to load the alphablending shader");
        return false;
    }
    
    alpha_pls_desc.colorAttachments[1] = nil;
    ags->alphablend_notouch_pls =
        [with_metal_device
            newRenderPipelineStateWithDescriptor:
                alpha_pls_desc 
            error:
                &Error];
    log_assert(Error == NULL);
    #elif T1_BLENDING_SHADER_ACTIVE == T1_INACTIVE
    #else
    #error
    #endif
    
    MTLDepthStencilDescriptor * depth_desc =
        [MTLDepthStencilDescriptor new];
    depth_desc.depthWriteEnabled = YES;
    [depth_desc
        setDepthCompareFunction:
            MTLCompareFunctionLessEqual];
    ags->opaque_depth_stencil_state =
        [with_metal_device
            newDepthStencilStateWithDescriptor:
                depth_desc];
    
    if (Error != NULL)
    {
        log_append(
            [[Error localizedDescription]
                cStringUsingEncoding:
                    kCFStringEncodingASCII]);
        #if T1_LOGGER_ASSERTS_ACTIVE == T1_ACTIVE
        log_dump_and_crash(
            "Error setting the depth "
            "stencil state\n");
        #elif T1_LOGGER_ASSERTS_ACTIVE == T1_INACTIVE
        #else
        #error
        #endif
        
        T1_std_strcpy_cap(
            error_msg_string,
            512,
            "Failed to load the depth "
            "stencil shader");
        return false;
    }
    
    for (
        uint32_t frame_i = 0;
        frame_i < MAX_FRAME_BUFFERS;
        frame_i++)
    {
        T1GPUFrame * f = &gpu_shared_data_collection->
            triple_buffers[frame_i];
        
        id<MTLBuffer> MTLBufferFramePolygons =
            [with_metal_device
                /* the pointer needs to be page aligned */
                    newBufferWithBytesNoCopy:
                        f->zsprite_list->polygons
                /* the length weirdly needs to be page aligned also */
                    length:
                        gpu_shared_data_collection->polygons_alloc_size
                    options:
                        MTLResourceStorageModeShared
                /* deallocator = nil to opt out */
                    deallocator:
                        nil];
        ags->polygon_buffers[frame_i] = MTLBufferFramePolygons;
        
        id<MTLBuffer> MTLBufferFrameVertices =
            [with_metal_device
                /* the pointer needs to be page aligned */
                    newBufferWithBytesNoCopy:
                        f->verts
                /* the length weirdly needs to be page aligned also */
                    length:
                        gpu_shared_data_collection->vertices_alloc_size
                    options:
                        MTLResourceStorageModeShared
                /* deallocator = nil to opt out */
                    deallocator:
                        nil];
        
        ags->vertex_buffers[frame_i] = MTLBufferFrameVertices;
        
        id<MTLBuffer> MTLBufferFrameCircles =
            [with_metal_device
                /* the pointer needs to be page aligned */
                    newBufferWithBytesNoCopy:
                        f->flat_billboard_quads
                /* the length weirdly needs to be page aligned also */
                    length:
                        gpu_shared_data_collection->flat_quads_alloc_size
                    options:
                        MTLResourceStorageModeShared
                /* deallocator = nil to opt out */
                    deallocator:
                        nil];
        
        ags->flat_quad_buffers[frame_i] = MTLBufferFrameCircles;
        
        id<MTLBuffer> MTLBufferPostProcessingConstants =
            [with_metal_device
                /* the pointer needs to be page aligned */
                    newBufferWithBytesNoCopy:
                        f->postproc_consts
                /* the length weirdly needs to be page aligned also */
                    length:
                        gpu_shared_data_collection->
                            postprocessing_constants_alloc_size
                    options:
                        MTLResourceStorageModeShared
                /* deallocator = nil to opt out */
                    deallocator:
                        nil];
        log_assert(MTLBufferPostProcessingConstants != nil);
        
        ags->postprocessing_constants_buffers[frame_i] =
            MTLBufferPostProcessingConstants;
        
        id<MTLBuffer> MTLBufferFrameLights =
            [with_metal_device
                /* the pointer needs to be page aligned */
                    newBufferWithBytesNoCopy:
                        f->lights
                /* the length weirdly needs to be page aligned also */
                    length:
                        gpu_shared_data_collection->lights_alloc_size
                    options:
                        MTLResourceStorageModeShared | MTLResourceUsageRead
                /* deallocator = nil to opt out */
                    deallocator:
                        nil];
        
        ags->light_buffers[frame_i] =
            MTLBufferFrameLights;
        
        for (
            uint32_t rv_i = 0;
            rv_i < T1_RENDER_VIEW_CAP;
            rv_i++)
        {
            id<MTLBuffer> MTLBufferFrameCamera =
            [with_metal_device
                /* needs to be page aligned */
                    newBufferWithBytesNoCopy:
                        f->render_views[rv_i]
                /* also needs to be aligned */
                    length:
                        gpu_shared_data_collection->
                            render_view_alloc_size
                    options:
                        MTLResourceStorageModeShared
                /* deallocator = nil to opt out */
                    deallocator:
                        nil];
            
            ags->camera_buffers[frame_i][rv_i] =
                MTLBufferFrameCamera;
        }
    }
    
    id<MTLBuffer> MTLBufferLockedVerticesPopulator =
        [with_metal_device
            /* the pointer needs to be page aligned */
                newBufferWithBytesNoCopy:
                    gpu_shared_data_collection->locked_vertices
            /* the length weirdly needs to be page aligned also */
                length:
                    gpu_shared_data_collection->locked_vertices_alloc_size
                options:
                    MTLResourceStorageModeShared
            /* deallocator = nil to opt out */
                deallocator:
                    nil];
    
    ags->locked_vertex_populator_buffer = MTLBufferLockedVerticesPopulator;
    
    id<MTLBuffer> MTLBufferLockedVertices =
        [with_metal_device
            newBufferWithLength:
                gpu_shared_data_collection->
                    locked_vertices_alloc_size
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
                    gpu_shared_data_collection->const_mats_alloc_size
                options:
                    MTLResourceStorageModeShared
            /* deallocator = nil to opt out */
                deallocator:
                    nil];
    
    ags->locked_materials_populator_buffer = MTLBufferLockedMaterialsPopulator;
    
    id<MTLBuffer> MTLBufferLockedMaterials =
        [with_metal_device
            newBufferWithLength:
                gpu_shared_data_collection->const_mats_alloc_size
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
                        projection_constants_alloc_size
                options:
                    MTLResourceStorageModeShared
            /* deallocator = nil to opt out */
                deallocator:
                    nil];
    
    ags->projection_constants_buffer = MTLBufferProjectionConstants;
    
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
    
    #if T1_BLOOM_ACTIVE == T1_ACTIVE
    id<MTLFunction> downsample_func =
        [ags->lib newFunctionWithName: @"downsample_texture"];
    
    ags->downsample_compute_pls =
        [ags->device
            newComputePipelineStateWithFunction:
                downsample_func
            error:nil];
    
    id<MTLFunction> boxblur_func =
        [ags->lib newFunctionWithName: @"boxblur_texture"];
    ags->boxblur_compute_pls =
        [ags->device
            newComputePipelineStateWithFunction:boxblur_func
            error:nil];
    #elif T1_BLOOM_ACTIVE == T1_INACTIVE
    #else
    #error
    #endif
    
    id<MTLFunction> singlequad_vertex_shader =
        [ags->lib newFunctionWithName:
            @"single_quad_vertex_shader"];
    if (singlequad_vertex_shader == NULL) {
        log_append(
            "Missing function: "
            "postprocess_vertex_shader()!");
        
        T1_std_strcpy_cap(
            error_msg_string,
            512,
            "Missing function: "
            "postprocess_vertex_shader()");
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
    singlequad_pipeline_descriptor.
        colorAttachments[0].
            destinationRGBBlendFactor =
                MTLBlendFactorOneMinusSourceAlpha;
    singlequad_pipeline_descriptor.
        depthAttachmentPixelFormat =
            MTLPixelFormatDepth32Float;
    singlequad_pipeline_descriptor.vertexBuffers[0].
        mutability =
            MTLMutabilityImmutable;
    ags->singlequad_pls = [
        with_metal_device
        newRenderPipelineStateWithDescriptor:singlequad_pipeline_descriptor
        error:NULL];
    
    ags->command_queue = [with_metal_device newCommandQueue];
    
    ags->metal_active = true;
    
    return true;
}

#if T1_BLOOM_ACTIVE == T1_ACTIVE
static float get_ds_width(
    const uint32_t ds_i,
    const uint32_t base_width)
{
    float return_value = base_width;
    
    for (uint32_t i = 0; i < ds_i && i < DOWNSAMPLES_CUTOFF; i++) {
        return_value *= 0.5f;
    }
    
    return return_value;
}

static float get_ds_height(
    const uint32_t ds_i,
    const uint32_t base_height)
{
    float return_value = base_height;
    
    for (uint32_t i = 0; i < ds_i && i < DOWNSAMPLES_CUTOFF; i++) {
        return_value *= 0.5f;
    }
    
    return return_value;
}
#elif T1_BLOOM_ACTIVE == T1_INACTIVE
#else
#error
#endif

void T1_platform_gpu_get_device_name(
    char * recipient,
    const uint32_t recipient_cap)
{
    #if T1_LOGGER_ASSERTS_ACTIVE == T1_ACTIVE
    // pass
    #elif T1_LOGGER_ASSERTS_ACTIVE == T1_INACTIVE
    (void)recipient_cap;
    #else
    #error
    #endif
    
    const char * device_name_cstr =
        [[ags->device name] cStringUsingEncoding:
            NSASCIIStringEncoding];
    
    T1_std_strcpy_cap(
        recipient,
        recipient_cap,
        device_name_cstr);
}

// returns the slice_i of the new depth texture
// in the array of depth textures
int32_t
T1_platform_gpu_make_depth_tex(
    const uint32_t width,
    const uint32_t height)
{
    return T1_apple_gpu_make_depth_tex(
        width,
        height);
}

int32_t T1_platform_gpu_get_touch_id_at_screen_pos(
    const float screen_x,
    const float screen_y)
{
    if (
        screen_x < 0 ||
        screen_y < 0 ||
        screen_x >=
            (T1_global->window_width) ||
        screen_y >= (T1_global->window_height))
    {
        return -1;
    }
    
    uint32_t rtt_width  =
        (uint32_t)ags->render_viewports[0].width;
    uint32_t rtt_height =
        (uint32_t)ags->render_viewports[0].height;
    
    uint32_t screen_x_adj = (uint32_t)(
        (screen_x * rtt_width) /
            T1_global->window_width);
    uint32_t screen_y_adj = (uint32_t)(
        ((T1_global->window_height - screen_y) * rtt_height) /
            T1_global->window_height);
    
    if (screen_x_adj >= rtt_width )
    {
        screen_x_adj = rtt_width;
    }
    if (screen_y_adj >= rtt_height)
    {
        screen_y_adj = rtt_height;
    }
    
    uint8_t * data = (uint8_t *)[ags->touch_id_buffer contents];
    uint64_t size = [ags->touch_id_buffer allocatedSize];
    
    uint32_t pixel_i =
        (screen_y_adj * rtt_width) + screen_x_adj;
    
    if (((pixel_i * 4) + 3) >= size)
    {
        return -1;
    }
    
    // See shaders for the packing logic
    uint32_t first_8bits  = data[(pixel_i*4)+0];
    uint32_t second_8bits = data[(pixel_i*4)+1];
    uint32_t third_8bits  = data[(pixel_i*4)+2];
    uint32_t fourth_8bits = data[(pixel_i*4)+3];
    
    uint32_t uid =
        (fourth_8bits << 24) |
        (third_8bits  << 16) |
        (second_8bits << 8) |
        first_8bits;
    int32_t final_id = *(int32_t *)&uid;
    
    if (final_id < -1) { final_id = -1; }
    
    return final_id;
}

void T1_platform_gpu_copy_texture_array(
    const int32_t texture_array_i,
    const uint32_t num_images,
    const uint32_t single_image_width,
    const uint32_t single_image_height,
    const bool32_t is_render_target,
    const bool32_t use_bc1_compression)
{
    log_assert(texture_array_i >=  0);
    log_assert(texture_array_i <  31);
    bool32_t copy_prev = false;
    id<MTLTexture> prev_copy = nil;
    
    if (
        ags->metal_textures[texture_array_i] ==
            NULL)
    {
        copy_prev = false;
    } else if (
        num_images >
            [ags->metal_textures[texture_array_i]
                arrayLength] ||
        single_image_width != ags->
            metal_textures[texture_array_i].width ||
        single_image_height != ags->
            metal_textures[texture_array_i].height)
    {
        copy_prev = true;
        prev_copy =
            ags->metal_textures[texture_array_i];
        ags->metal_textures[texture_array_i] = NULL;
    } else {
        // TODO: blit a flat color to the existing
        // TODO: textures to highlight missing
        log_assert(0);
        log_assert(ags->metal_textures[texture_array_i].pixelFormat !=
                MTLPixelFormatBC1_RGBA);
        log_assert(ags->metal_textures[texture_array_i].usage == MTLTextureUsageRenderTarget);
        return;
    }
    
    MTLTextureDescriptor * texture_descriptor =
        [[MTLTextureDescriptor alloc] init];
    texture_descriptor.textureType =
        MTLTextureType2DArray;
    texture_descriptor.arrayLength = num_images;
    texture_descriptor.pixelFormat = use_bc1_compression ?
        MTLPixelFormatBC1_RGBA :
        MTLPixelFormatRGBA8Unorm;
    texture_descriptor.storageMode =
        MTLStorageModePrivate;
    
    if (is_render_target)
    {
        texture_descriptor.usage =
            MTLTextureUsageShaderRead |
            MTLTextureUsageRenderTarget;
    } else {
        texture_descriptor.usage = MTLTextureUsageShaderRead;
    }
    texture_descriptor.width = single_image_width;
    texture_descriptor.height = single_image_height;
    #if T1_MIPMAPS_ACTIVE == T1_ACTIVE
    texture_descriptor.mipmapLevelCount =
        use_bc1_compression || texture_array_i == 0 ?
        1 :
        (NSUInteger)floor(
            log2((double)MAX(
                single_image_width,
                single_image_height))) + 1;
    #elif T1_MIPMAPS_ACTIVE == T1_INACTIVE
    texture_descriptor.mipmapLevelCount = 1;
    #else
    #error
    #endif
    id<MTLTexture> texture = [ags->device
        newTextureWithDescriptor:texture_descriptor];
    
    log_assert(ags->metal_textures[texture_array_i] == NULL);
    ags->metal_textures[texture_array_i] = texture;
    
    T1_texture_arrays[texture_array_i].
        gpu_capacity = num_images;
    
    if (copy_prev) {
        id<MTLCommandBuffer> combuf = [ags->command_queue commandBuffer];
        id<MTLBlitCommandEncoder> blitenc = [combuf blitCommandEncoder];
        
        [blitenc
            copyFromTexture:
                prev_copy
            toTexture:
                ags->metal_textures[texture_array_i]];
        
        [blitenc endEncoding];
        [combuf commit];
        [combuf waitUntilCompleted];
    }
}

#if T1_TEXTURES_ACTIVE == T1_ACTIVE
void T1_platform_gpu_fetch_rgba_at(
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
    log_assert(texture_array_i < TEXTUREARRAYS_SIZE);
    log_assert(rgba_recipient != NULL);
    log_assert(recipient_size != NULL);
    log_assert(good != NULL);
    
    *good = false;
    
    // Check if the texture array exists
    id<MTLTexture> texture = ags->metal_textures[texture_array_i];
    if (texture == nil || texture.textureType != MTLTextureType2DArray) {
        return;
    }
    
    if (texture.pixelFormat != MTLPixelFormatRGBA8Unorm)
    {
        // Ensure the texture format is RGBA8Unorm
        // for direct copying to uint8_t RGBA
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
        copyFromTexture:
            texture
        sourceSlice:
            (NSUInteger)texture_i
        sourceLevel:
            0
        sourceOrigin:
            MTLOriginMake(0, 0, 0)
        sourceSize:
            MTLSizeMake(
                texture.width, texture.height, 1)
        toBuffer:
            temp_buffer
        destinationOffset:
            0
        destinationBytesPerRow:
            bytes_per_row
        destinationBytesPerImage:
            bytes_per_image];
    
    [blit_encoder endEncoding];
    
    // Add completion handler to copy data
    // to rgba_recipient
    [command_buffer
        addCompletedHandler:
            ^(id<MTLCommandBuffer> cb) {
        if (cb.error == nil)
        {
            // Copy buffer contents to rgba_recipient
            T1_std_memcpy(
                rgba_recipient,
                [temp_buffer contents],
                bytes_per_image);
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
#elif T1_TEXTURES_ACTIVE == T1_INACTIVE
// Pass
#else
#error
#endif

#if T1_MIPMAPS_ACTIVE == T1_ACTIVE
void T1_platform_gpu_generate_mipmaps_for_texture_array(
    const int32_t texture_array_i)
{
    // no mipmaps for font
    log_assert(texture_array_i != 0);
    // no mipmaps for bc1 compressed arrays
    log_assert(!T1_texture_arrays[texture_array_i].bc1_compressed);
    
    id <MTLCommandBuffer> combuf =
        [ags->command_queue commandBuffer];
    
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
#elif T1_MIPMAPS_ACTIVE == T1_INACTIVE
// Pass
#else
#error
#endif

void T1_platform_gpu_push_texture_slice_and_free_rgba_values(
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
    log_assert(texture_array_i < TEXTUREARRAYS_SIZE);
    
    if (ags->metal_textures[texture_array_i] == NULL) {
        #if T1_LOGGER_ASSERTS_ACTIVE == T1_ACTIVE
        char errmsg[256];
        T1_std_strcpy_cap(
            errmsg,
            256,
            "Tried to update uninitialized texturearray")
        T1_std_strcat_int_cap(errmsg, 256, texture_array_i);
        T1_std_strcat_cap(errmsg, 256, "\n");
        
        log_dump_and_crash(errmsg);
        #elif T1_LOGGER_ASSERTS_ACTIVE == T1_INACTIVE
        #else
        #error
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

#if T1_TEXTURES_ACTIVE
void T1_platform_gpu_push_bc1_texture_slice_and_free_bc1_values(
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
        #if T1_LOGGER_ASSERTS_ACTIVE == T1_ACTIVE
        char errmsg[256];
        T1_std_strcpy_cap(
            errmsg,
            256,
            "Tried to update uninitialized texturearray")
        T1_std_strcat_int_cap(errmsg, 256, texture_array_i);
        T1_std_strcat_cap(errmsg, 256, "\n");
        
        log_dump_and_crash(errmsg);
        #elif T1_LOGGER_ASSERTS_ACTIVE == T1_INACTIVE
        // Pass
        #else
        #error
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

void T1_platform_gpu_copy_locked_vertices(void)
{
    gpu_shared_data_collection->locked_vertices_size = T1_objmodel_all_vertices->size;
    
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
            gpu_shared_data_collection->locked_vertices_alloc_size];
    [blit_copy_encoder endEncoding];
    
    // Add a completion handler and commit the command buffer.
    [combuf addCompletedHandler:^(id<MTLCommandBuffer> cb) {
        // Populate private buffer.
        (void)cb;
    }];
    [combuf commit];
}

void T1_platform_gpu_copy_locked_materials(void)
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
            gpu_shared_data_collection->const_mats_alloc_size];
    [blit_copy_encoder endEncoding];
    
    // Add a completion handler and commit the command buffer.
    [combuf addCompletedHandler:^(id<MTLCommandBuffer> cb) {
        // Populate private buffer.
        (void)cb;
    }];
    [combuf commit];
}

static id<MTLTexture> get_texture_array_slice(
    const int32_t at_array_i,
    const int32_t at_slice_i)
{
    log_assert(at_array_i >= 0);
    log_assert(at_array_i < TEXTUREARRAYS_SIZE);
    log_assert(at_slice_i >= 0);
    
    id<MTLTexture> parent =
        ags->metal_textures[at_array_i];
    NSRange level_range = NSMakeRange(
        0, parent.mipmapLevelCount);
    NSRange slice_range = NSMakeRange(
        (NSUInteger)at_slice_i,
        1);
    
    id<MTLTexture> retval = [parent
        newTextureViewWithPixelFormat:
            parent.pixelFormat
        textureType:
            MTLTextureType2D
        levels:
            level_range
        slices:
            slice_range];
    
    log_assert(retval != nil);
    return retval;
}

static void
set_defaults_for_render_descriptor(
    MTLRenderPassDescriptor * desc,
    const int32_t cam_i)
{
    desc.depthAttachment.loadAction =
        MTLLoadActionLoad;
    desc.depthAttachment.storeAction =
        MTLStoreActionStore;
    desc.colorAttachments[0].loadAction = MTLLoadActionLoad;
    desc.colorAttachments[0].storeAction =
        MTLStoreActionStore;
    
    // ID Buffer for touchables
    if (
        T1_render_views[cam_i].write_type ==
            T1RENDERVIEW_WRITE_RENDER_TARGET)
    {
        desc.colorAttachments[1].texture =
            ags->touch_id_texture;
        desc.colorAttachments[1].loadAction =
            MTLLoadActionLoad; // We clear manually
        desc.colorAttachments[1].storeAction =
            MTLStoreActionStore;
    }
}

static void set_defaults_for_encoder(
    id<MTLRenderCommandEncoder> encoder,
    const uint32_t cam_i)
{
    log_assert(ags->opaque_depth_stencil_state != nil);
    [encoder
        setDepthStencilState: ags->opaque_depth_stencil_state];
    
    [encoder setDepthClipMode: MTLDepthClipModeClip];
    [encoder setCullMode: MTLCullModeBack];
    [encoder setFrontFacingWinding:
        MTLWindingCounterClockwise];
    
    [encoder
        setVertexBuffer:
            ags->vertex_buffers[ags->frame_i]
        offset:
            0
        atIndex:
            0];
    
    [encoder
        setVertexBuffer:
            ags->polygon_buffers[ags->frame_i]
        offset:
            0
        atIndex:
            1];
    
    [encoder
        setVertexBuffer:
            ags->flat_quad_buffers[ags->frame_i]
        offset:
            0
        atIndex:
            2];
    
    [encoder
        setVertexBuffer:
            ags->camera_buffers[ags->frame_i][cam_i]
        offset: 0
        atIndex: 3];
    
    [encoder
        setVertexBuffer:
            ags->locked_vertex_buffer
        offset:
            0 
        atIndex:
            4];
    
    [encoder
        setVertexBuffer:
            ags->projection_constants_buffer
        offset:
            0
        atIndex:
            5];
    
    [encoder
        setFragmentBuffer:
            ags->locked_vertex_buffer
        offset:
            0
        atIndex:
            0];
    
    [encoder
        setFragmentBuffer:
            ags->polygon_buffers[ags->frame_i]
        offset:
            0
        atIndex:
            1];
    
    [encoder
        setFragmentBuffer:
            ags->light_buffers[ags->frame_i]
        offset:
            0
        atIndex:
            2];
    
    [encoder
        setFragmentBuffer:
            ags->camera_buffers[ags->frame_i][cam_i]
        offset:
            0
        atIndex:
            3];
    
    [encoder
        setFragmentBuffer:
            ags->projection_constants_buffer
        offset:
            0
        atIndex:
            4];
    
    [encoder
        setFragmentBuffer:
            ags->locked_materials_buffer
        offset:
            0
        atIndex:
            6];
    
    [encoder
        setFragmentBuffer:
            ags->postprocessing_constants_buffers[ags->frame_i]
        offset:
            0
        atIndex:
            7];
    
    #if T1_TEXTURES_ACTIVE == T1_ACTIVE
    for (
        uint32_t i = 0;
        i < TEXTUREARRAYS_SIZE;
        i++)
    {
        if (ags->metal_textures[i] != NULL) {
            [encoder
                setFragmentTexture: ags->metal_textures[i]
                atIndex: i];
        }
    }
    #elif T1_TEXTURES_ACTIVE == T1_INACTIVE
    [encoder
        setFragmentTexture: ags->metal_textures[0]
        atIndex: 0];
    #else
    #error
    #endif
    
    #if T1_SHADOWS_ACTIVE == T1_ACTIVE
    [encoder
        setFragmentTexture: ags->shadows_texture
        atIndex: SHADOWMAP_TEXTUREARRAY_I];
    #elif T1_SHADOWS_ACTIVE == T1_INACTIVE
    #else
    #error
    #endif

}

@implementation MetalKitViewDelegate
- (void) updateFinalWindowSize
{
    ags->window_viewport.originX = 0;
    ags->window_viewport.originY = 0;
    ags->window_viewport.width   =
        T1_global->window_width *
            ags->retina_scaling_factor;
    ags->window_viewport.height  =
        T1_global->window_height *
            ags->retina_scaling_factor;
    log_assert(ags->window_viewport.width > 0.0f);
    log_assert(ags->window_viewport.height > 0.0f);
    
    /*
    These near/far values are the final viewport coordinates (after
    fragment shader), not to be confused with
    window_globals->projection_constants.near that's in our world space
    and much larger numbers
    */
    ags->window_viewport.znear = 0.001f;
    ags->window_viewport.zfar = 1.0f;
    
    ags->window_viewport.width /= T1_global->pixelation_div;
    ags->window_viewport.height /= T1_global->pixelation_div;
    
    *gpu_shared_data_collection->locked_pjc =
        T1_global->project_consts;
    
    MTLTextureDescriptor * camera_depth_texture_descriptor =
        [[MTLTextureDescriptor alloc] init];
    camera_depth_texture_descriptor.textureType = MTLTextureType2D;
    camera_depth_texture_descriptor.pixelFormat = MTLPixelFormatDepth32Float;
    camera_depth_texture_descriptor.width =
        (unsigned long)ags->window_viewport.width;
    camera_depth_texture_descriptor.height =
        (unsigned long)ags->window_viewport.height;
    camera_depth_texture_descriptor.storageMode = MTLStorageModePrivate;
    camera_depth_texture_descriptor.usage =
        MTLTextureUsageRenderTarget |
        MTLTextureUsageShaderRead;
    
    ags->camera_depth_texture =
        [ags->device newTextureWithDescriptor:
            camera_depth_texture_descriptor];
    
    #if T1_BLOOM_ACTIVE == T1_ACTIVE
    for (
        uint32_t i = 0;
        i < DOWNSAMPLES_SIZE;
        i++)
    {
        MTLTextureDescriptor * downsampled_rtt_desc =
            [MTLTextureDescriptor new];
        downsampled_rtt_desc.textureType =
            MTLTextureType2D;
        downsampled_rtt_desc.width =
            (NSUInteger)get_ds_width(
                i,
                (uint32_t)ags->
                    render_viewports[0].width);
        downsampled_rtt_desc.height =
            (NSUInteger)get_ds_height(
                i,
                (uint32_t)ags->
                    render_viewports[0].height);
        downsampled_rtt_desc.pixelFormat =
            MTLPixelFormatRGBA8Unorm;
        downsampled_rtt_desc.mipmapLevelCount = 1;
        downsampled_rtt_desc.storageMode =
            MTLStorageModePrivate;
        downsampled_rtt_desc.usage =
            MTLTextureUsageShaderWrite |
            MTLTextureUsageShaderRead;
        if (i == 0) {
            downsampled_rtt_desc.usage |=
                MTLTextureUsageRenderTarget;
        }
        ags->downsampled_rtts[i] =
            [ags->device
                newTextureWithDescriptor:
                    downsampled_rtt_desc];
    }
    #elif T1_BLOOM_ACTIVE == T1_INACTIVE
    #else
    #error
    #endif
}

- (void) updateRenderViewSize: (unsigned int)at_i
{
    ags->render_viewports[at_i].originX = 0;
    ags->render_viewports[at_i].originY = 0;
    ags->render_viewports[at_i].width   =
        T1_render_views[at_i].width;
    ags->render_viewports[at_i].height  =
        T1_render_views[at_i].height;
    log_assert(ags->render_viewports[at_i].width > 0.0f);
    log_assert(ags->render_viewports[at_i].height > 0.0f);
    
    /*
    These near/far values are the final viewport coordinates (after
    fragment shader), not to be confused with
    window_globals->projection_constants.near that's in our world space
    and much larger numbers
    */
    ags->render_viewports[at_i].znear = 0.001f;
    ags->render_viewports[at_i].zfar = 1.0f;
    
    ags->viewports_set[at_i] = true;
    
    if (at_i != 0) { return; }
    
    MTLTextureDescriptor * touch_id_tex_desc =
        [MTLTextureDescriptor new];
    touch_id_tex_desc.width =
        (NSUInteger)ags->render_viewports[0].width;
    touch_id_tex_desc.height =
        (NSUInteger)ags->render_viewports[0].height;
    touch_id_tex_desc.pixelFormat =
        MTLPixelFormatRGBA8Unorm;
    touch_id_tex_desc.mipmapLevelCount = 1;
    touch_id_tex_desc.storageMode = MTLStorageModePrivate;
    touch_id_tex_desc.usage =
        MTLTextureUsageRenderTarget |
        MTLTextureUsageShaderRead;
    ags->touch_id_texture =
        [ags->device
            newTextureWithDescriptor: touch_id_tex_desc];
    
    uint64_t touch_buffer_size_bytes =
        touch_id_tex_desc.width *
            touch_id_tex_desc.height *
            4;
    
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
}

-(void)renderViewToColor:(MTKView *)view
    renderView:(int)cam_i
{
    
}

- (void)drawInMTKView:(MTKView *)view
{
    if (funcptr_gameloop_before_render == NULL) {
        return;
    }
    
    #if T1_DRAWING_SEMAPHORE_ACTIVE == T1_ACTIVE
    dispatch_semaphore_wait(ags->drawing_semaphore, DISPATCH_TIME_FOREVER);
    #elif T1_DRAWING_SEMAPHORE_ACTIVE == T1_INACTIVE
    #else
    #error
    #endif
    
    T1GPUFrame * f =
        &gpu_shared_data_collection->
            triple_buffers[ags->frame_i];
    
    log_assert(f->opaq_verts_size <= f->verts_size);
    log_assert(f->bloom_verts_size <= f->verts_size);
    log_assert(f->alpha_verts_size <= f->verts_size);
    
    funcptr_gameloop_before_render(
        gpu_shared_data_collection->
            triple_buffers + ags->frame_i);
    
    log_assert(f->opaq_verts_size <= f->verts_size);
    log_assert(f->bloom_verts_size <= f->verts_size);
    log_assert(f->alpha_verts_size <= f->verts_size);
    
    if (
        !ags ||
        !ags->metal_active ||
        !ags->viewports_set[0])
    {
        return;
    }
    
    id<MTLCommandBuffer> combuf =
        [ags->command_queue commandBuffer];
    
    if (combuf == nil) {
        #if T1_LOGGER_ASSERTS_ACTIVE
        log_dump_and_crash("error - can't get metal command buffer\n");
        #endif
        
        return;
    }
    
    log_assert(f->opaq_verts_size <= f->verts_size);
    log_assert(f->bloom_verts_size <= f->verts_size);
    log_assert(f->alpha_verts_size <= f->verts_size);
    
    // Blit to clear the touch id buffer
    uint32_t size_bytes = (uint32_t)(
        [ags->touch_id_texture height] *
        [ags->touch_id_texture width] *
        8);
    
    id <MTLBlitCommandEncoder>
        clear_touch_tex_blit_enc =
            [combuf blitCommandEncoder];
    
    [clear_touch_tex_blit_enc
        copyFromBuffer:
            ags->touch_id_buffer_all_zeros
        sourceOffset: 0
        sourceBytesPerRow:
            [ags->touch_id_texture width] * 4
        sourceBytesPerImage:
            size_bytes
        sourceSize:
            MTLSizeMake(
                [ags->touch_id_texture width],
                [ags->touch_id_texture height],
                1)
        toTexture:
            ags->touch_id_texture
        destinationSlice:
            0
        destinationLevel:
            0
        destinationOrigin:
            MTLOriginMake(0, 0, 0)];
    
    [clear_touch_tex_blit_enc endEncoding];
    
    for (
        int32_t cam_i =
            (int32_t)f->render_views_size - 1;
        cam_i >= 0;
        cam_i--)
    {
        log_assert(ags->viewports_set[cam_i]);
        
        log_assert(
            f->opaq_verts_size <=
                f->verts_size);
        
        log_assert(T1_render_views[cam_i].
            write_array_i >= 1);
        log_assert(T1_render_views[cam_i].
            write_slice_i >= 0);
        
        id<MTLTexture> current_rtt = nil;
        id<MTLTexture> current_depth = nil;
        uint32_t z_buffer_cleared = false;
        id<MTLRenderPipelineState> opq_pls = nil;
        id<MTLRenderPipelineState> blnd_pls = nil;
        id<MTLRenderPipelineState> bloom_pls = nil;
        id<MTLRenderPipelineState> bb_pls = nil;
        
        switch (T1_render_views[cam_i].write_type) {
            case T1RENDERVIEW_WRITE_RENDER_TARGET:
                current_rtt =
                    get_texture_array_slice(
                        T1_render_views[cam_i].
                            write_array_i,
                        T1_render_views[cam_i].
                            write_slice_i);
                log_assert(current_rtt != NULL);
                current_depth =
                    ags->camera_depth_texture;
                opq_pls =
                    ags->diamond_touch_pls;
                blnd_pls =
                    ags->alphablend_touch_pls;
                bloom_pls =
                    ags->alphablend_touch_pls;
                bb_pls = ags->bb_touch_pls;
            break;
            case T1RENDERVIEW_WRITE_RGBA:
                current_rtt =
                    get_texture_array_slice(
                        T1_render_views[cam_i].
                            write_array_i,
                        T1_render_views[cam_i].
                            write_slice_i);
                log_assert(current_rtt != NULL);
                current_depth =
                    ags->camera_depth_texture;
                opq_pls =
                    ags->diamond_notouch_pls;
                blnd_pls =
                    ags->alphablend_notouch_pls;
                bloom_pls =
                    ags->alphablend_notouch_pls;
                bb_pls = ags->bb_notouch_pls;
            break;
            case T1RENDERVIEW_WRITE_DEPTH:
                log_assert(
                    T1_render_views[cam_i].
                        write_array_i ==
                            DEPTH_TEXTUREARRAYS_I);
                log_assert(
                    T1_render_views[cam_i].
                        write_slice_i >= 0);
                log_assert(
                    T1_render_views[cam_i].
                        write_slice_i <
                            T1_RENDER_VIEW_CAP);
                current_rtt = nil;
                current_depth = ags->
                    depth_textures[
                        T1_render_views[cam_i].
                            write_slice_i];
                
                log_assert(current_depth != nil);
                log_assert((current_depth.usage & MTLTextureUsageRenderTarget) > 0);
                opq_pls = ags->depth_only_pls;
                blnd_pls = ags->depth_only_pls;
                bloom_pls = ags->depth_only_pls;
                bb_pls = nil;
            break;
        }
        
        // TODO: reimplement z prepasses
        #if T1_Z_PREPASS_ACTIVE == T1_ACTIVE
        #elif T1_Z_PREPASS_ACTIVE == T1_INACTIVE
        #else
        #error
        #endif
        
        #if T1_OUTLINES_ACTIVE == T1_ACTIVE
        if (
            T1_render_views[cam_i].draw_outlines)
        {
            // Drawing outlines to a depth target
            // seems pointless, not supported
            log_assert(
                T1_render_views[cam_i].
                    write_type !=
                        T1RENDERVIEW_WRITE_DEPTH);
            
            MTLRenderPassDescriptor * outlines_desc =
                [view currentRenderPassDescriptor];
            outlines_desc.depthAttachment.loadAction =
                z_buffer_cleared ?
                    MTLLoadActionLoad :
                    MTLLoadActionClear;
            z_buffer_cleared = true;
            outlines_desc.depthAttachment.
                clearDepth = 1.0f;
            outlines_desc.depthAttachment.
                storeAction = MTLStoreActionStore;
            outlines_desc.depthAttachment.texture =
                current_depth;
            
            outlines_desc.colorAttachments[0].
                texture = current_rtt;
            outlines_desc.
                colorAttachments[0].storeAction =
                    MTLStoreActionStore;
            
            outlines_desc.colorAttachments[0].clearColor =
                MTLClearColorMake(0.0f, 0.03f, 0.15f, 1.0f);
            
            id<MTLRenderCommandEncoder> render_pass_1_draw_outlines_encoder =
                [combuf
                    renderCommandEncoderWithDescriptor:
                        outlines_desc];
            
            [render_pass_1_draw_outlines_encoder
                setViewport:
                    ags->render_viewports[cam_i]];
            
            // outlines pipeline
            [render_pass_1_draw_outlines_encoder
                setRenderPipelineState:
                    ags->outlines_pls];
            [render_pass_1_draw_outlines_encoder
                setDepthStencilState:
                    ags->opaque_depth_stencil_state];
            [render_pass_1_draw_outlines_encoder
                setDepthClipMode: MTLDepthClipModeClip];
            [render_pass_1_draw_outlines_encoder setCullMode: MTLCullModeFront];
            [render_pass_1_draw_outlines_encoder
                setFrontFacingWinding:
                    MTLWindingCounterClockwise];
            
            [render_pass_1_draw_outlines_encoder
                setVertexBuffer:
                    ags->vertex_buffers[ags->frame_i]
                offset:
                    0
                atIndex:
                    0];
            
            [render_pass_1_draw_outlines_encoder
                setVertexBuffer:
                    ags->polygon_buffers[ags->frame_i]
                offset:
                    0
                atIndex:
                    1];
            
            [render_pass_1_draw_outlines_encoder
                setVertexBuffer:
                    ags->camera_buffers[ags->frame_i][cam_i]
                offset: 0
                atIndex: 3];
            
            [render_pass_1_draw_outlines_encoder
                setVertexBuffer:
                    ags->locked_vertex_buffer
                offset:
                    0
                atIndex:
                    4];
            
            if (
                T1_global->draw_triangles &&
                f->opaq_verts_size > 0)
            {
                log_assert(
                    f->opaq_verts_size <
                        MAX_VERTICES_PER_BUFFER);
                log_assert(f->opaq_verts_size % 3 == 0);
                [render_pass_1_draw_outlines_encoder
                    drawPrimitives:
                        MTLPrimitiveTypeTriangle
                    vertexStart:
                        0
                    vertexCount:
                        f->opaq_verts_size];
            }
            [render_pass_1_draw_outlines_encoder endEncoding];
        }
        #elif T1_OUTLINES_ACTIVE == T1_INACTIVE
        #else
        #error
        #endif
        
        // opaque triangles
        MTLRenderPassDescriptor * opaque_tris_desc =
        [view currentRenderPassDescriptor];
        
        set_defaults_for_render_descriptor(
            opaque_tris_desc,
            cam_i);
        
        opaque_tris_desc.depthAttachment.texture =
            current_depth;
        opaque_tris_desc.colorAttachments[0].texture = current_rtt;
        
        if (!z_buffer_cleared) {
            opaque_tris_desc.depthAttachment.
                loadAction = MTLLoadActionClear;
            opaque_tris_desc.depthAttachment.
                clearDepth = 1.0f;
            z_buffer_cleared = true;
        }
        
        #if T1_OUTLINES_ACTIVE == T1_ACTIVE
        // handled in basic
        #elif T1_OUTLINES_ACTIVE == T1_INACTIVE
        opaque_tris_desc.colorAttachments[0].
            texture = current_rtt;
        opaque_tris_desc.colorAttachments[0].loadAction = MTLLoadActionClear;
        opaque_tris_desc.colorAttachments[0].storeAction =
            MTLStoreActionStore;
        opaque_tris_desc.colorAttachments[0].clearColor =
            MTLClearColorMake(0.0f, 0.03f, 0.15f, 1.0f);
        #else
        #error
        #endif
        
        id<MTLRenderCommandEncoder>
            pass_2_opaque_tris_enc = [combuf
                renderCommandEncoderWithDescriptor:
                    opaque_tris_desc];
        
        [pass_2_opaque_tris_enc
            setRenderPipelineState: opq_pls];
        
        set_defaults_for_encoder(
            pass_2_opaque_tris_enc,
            (uint32_t)cam_i);
        
        #if T1_LOGGER_ASSERTS_ACTIVE == T1_ACTIVE
        for (
            uint32_t i = 0;
            i < f->verts_size;
            i++)
        {
            log_assert(f->verts[i].locked_vertex_i <
                ALL_LOCKED_VERTICES_SIZE);
        }
        #elif T1_LOGGER_ASSERTS_ACTIVE == T1_INACTIVE
        #else
        #error
        #endif
        
        if (
            T1_global->draw_triangles &&
            f->opaq_verts_size > 0)
        {
            log_assert(f->opaq_verts_size < MAX_VERTICES_PER_BUFFER);
            log_assert(f->opaq_verts_size % 3 == 0);
            [pass_2_opaque_tris_enc
                drawPrimitives:
                    MTLPrimitiveTypeTriangle
                vertexStart:
                    0
                vertexCount:
                    f->opaq_verts_size];
        }
        [pass_2_opaque_tris_enc
            endEncoding];
        
        log_assert(
            f->first_alpha_i <=
            f->verts_size);
        
        #if T1_BLENDING_SHADER_ACTIVE == T1_ACTIVE
        if (
            T1_global->draw_triangles &&
            f->alpha_verts_size > 0 &&
            blnd_pls != nil)
        {
            MTLRenderPassDescriptor *
                alpha_tris_descriptor =
                    [view
                        currentRenderPassDescriptor];
            set_defaults_for_render_descriptor(
                alpha_tris_descriptor,
                cam_i);
            
            alpha_tris_descriptor.colorAttachments[0].
                texture = current_rtt;
            alpha_tris_descriptor.depthAttachment.
                texture = current_depth;
            
            id<MTLRenderCommandEncoder>
                render_pass_3_alpha_triangles_encoder =
                    [combuf
                        renderCommandEncoderWithDescriptor:
                    alpha_tris_descriptor];
            
            [render_pass_3_alpha_triangles_encoder
                setRenderPipelineState:
                    blnd_pls];
            set_defaults_for_encoder(
                render_pass_3_alpha_triangles_encoder,
                (uint32_t)cam_i);
            
            [render_pass_3_alpha_triangles_encoder
                setCullMode: MTLCullModeBack];
            [render_pass_3_alpha_triangles_encoder
                setFrontFacingWinding:
                    MTLWindingCounterClockwise];
            
            [render_pass_3_alpha_triangles_encoder
                drawPrimitives:
                    MTLPrimitiveTypeTriangle
                vertexStart:
                    f->first_alpha_i
                vertexCount:
                    f->alpha_verts_size];
            
            [render_pass_3_alpha_triangles_encoder
                endEncoding];
        }
        #elif T1_BLENDING_SHADER_ACTIVE == T1_INACTIVE
        #else
        #error
        #endif
        
        if (
            // T1_global->draw_billboards &&
            f->flat_billboard_quads_size > 0 &&
            bb_pls != nil)
        {
            MTLRenderPassDescriptor * bb_desc =
                [view currentRenderPassDescriptor];
            
            set_defaults_for_render_descriptor(
                bb_desc,
                cam_i);
            
            bb_desc.depthAttachment.texture =
                current_depth;
            bb_desc.colorAttachments[0].texture = current_rtt;
            
            if (!z_buffer_cleared) {
                bb_desc.depthAttachment.
                    loadAction = MTLLoadActionClear;
                bb_desc.depthAttachment.
                    clearDepth = 1.0f;
                z_buffer_cleared = true;
            }
                
            id<MTLRenderCommandEncoder>
                pass_4_bb_enc = [combuf
                    renderCommandEncoderWithDescriptor:
                        bb_desc];
            
            [pass_4_bb_enc
                setRenderPipelineState: bb_pls];
            [pass_4_bb_enc setDepthStencilState:
                ags->opaque_depth_stencil_state];
            
            [pass_4_bb_enc
                setVertexBuffer:
                    ags->flat_quad_buffers[ags->frame_i]
                offset: 0
                atIndex: 2];
            
            [pass_4_bb_enc
                setVertexBuffer:
                    ags->camera_buffers[ags->frame_i][cam_i]
                offset: 0
                atIndex: 3];
            
            [pass_4_bb_enc
                drawPrimitives:
                    MTLPrimitiveTypeTriangle
                vertexStart:
                    0
                vertexCount:
                    f->flat_billboard_quads_size * 6
                ];
            
            [pass_4_bb_enc endEncoding];
        }
        
        #if T1_BLOOM_ACTIVE == T1_ACTIVE
        log_assert(
            f->first_bloom_i <=
            f->verts_size);
        
        if (
            T1_global->draw_triangles &&
            f->bloom_verts_size > 0 &&
            blnd_pls != nil &&
            f->render_views[cam_i]->write_type ==
                T1RENDERVIEW_WRITE_RENDER_TARGET)
        {
            // only render target can bloom atm
            log_assert(cam_i == 0);
            
            MTLRenderPassDescriptor *
                bloom_tris_desc = [view
                    currentRenderPassDescriptor];
            set_defaults_for_render_descriptor(
                bloom_tris_desc,
                cam_i);
            
            bloom_tris_desc.depthAttachment.
                texture = current_depth;
            
            id<MTLTexture> bloom_rtt =
                ags->downsampled_rtts[0];
            bloom_tris_desc.colorAttachments[0].
                texture = bloom_rtt;
            bloom_tris_desc.colorAttachments[0].
                clearColor = MTLClearColorMake(
                    0.0, 0.0, 0.0, 0.0);
            bloom_tris_desc.colorAttachments[0].
                loadAction = MTLLoadActionClear;
            
            id<MTLRenderCommandEncoder>
                pass_4_bloom_enc = [combuf
                    renderCommandEncoderWithDescriptor:
                    bloom_tris_desc];
            
            [pass_4_bloom_enc
                setRenderPipelineState:
                    bloom_pls];
            
            set_defaults_for_encoder(
                pass_4_bloom_enc,
                (uint32_t)cam_i);
            
            [pass_4_bloom_enc
                setCullMode: MTLCullModeBack];
            [pass_4_bloom_enc
                setFrontFacingWinding:
                    MTLWindingCounterClockwise];
            
            [pass_4_bloom_enc
                drawPrimitives:
                    MTLPrimitiveTypeTriangle
                vertexStart:
                    f->first_bloom_i
                vertexCount:
                    f->bloom_verts_size];
            
            [pass_4_bloom_enc endEncoding];
            
            for (
                uint32_t ds_i = 1;
                ds_i < DOWNSAMPLES_SIZE;
                ds_i++)
            {
                MTLViewport smaller_viewport =
                    ags->render_viewports[0];
                
                smaller_viewport.width = ags->
                    downsampled_rtts[ds_i].width;
                smaller_viewport.height = ags->
                    downsampled_rtts[ds_i].height;
                
                MTLSize grid = MTLSizeMake(
                    (uint32_t)smaller_viewport.width,
                    (uint32_t)smaller_viewport.height,
                    1);
                
                MTLSize threadgroup = MTLSizeMake(16, 16, 1);
                
                if (ds_i < DOWNSAMPLES_CUTOFF) {
                    id<MTLComputeCommandEncoder>
                        compute_enc = [combuf computeCommandEncoder];
                    [compute_enc
                        setComputePipelineState:
                            ags->downsample_compute_pls];
                    [compute_enc
                        setTexture: ds_i > 0 ?
                            ags->downsampled_rtts[ds_i-1] :
                            bloom_rtt
                        atIndex:0];
                    [compute_enc
                        setTexture:
                            ags->downsampled_rtts[ds_i]
                        atIndex:
                            1];
                    [compute_enc
                        dispatchThreads:grid
                        threadsPerThreadgroup:threadgroup];
                    [compute_enc endEncoding];
                }
                
                id<MTLComputeCommandEncoder> boxblur_enc =
                    [combuf computeCommandEncoder];
                [boxblur_enc
                    setComputePipelineState:
                        ags->boxblur_compute_pls];
                [boxblur_enc
                    setTexture: ags->downsampled_rtts[ds_i]
                    atIndex:0];
                [boxblur_enc
                    dispatchThreads:grid
                    threadsPerThreadgroup:threadgroup];
                [boxblur_enc endEncoding];
            }
            
        }
        #elif T1_BLOOM_ACTIVE == T1_INACTIVE
        #else
        #error
        #endif
    }
    
    // copy the touch id buffer for CPU use
    id <MTLBlitCommandEncoder> blit_touch_texture_to_cpu_buffer_enc =
        [combuf blitCommandEncoder];
    [blit_touch_texture_to_cpu_buffer_enc
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
        destinationBytesPerRow:
            [ags->touch_id_texture width] * 4
        destinationBytesPerImage:
            [ags->touch_id_texture width] * [ags->touch_id_texture height] * 4];
    [blit_touch_texture_to_cpu_buffer_enc endEncoding];
        
    // Render pass 4 puts a quad on the full screen
    MTLRenderPassDescriptor *
        pass_5_comp_desc =
            [view currentRenderPassDescriptor];
    pass_5_comp_desc.colorAttachments[0].
        clearColor =
            MTLClearColorMake(0.0f, 0.0f, 0.0f, 1.0f);
    pass_5_comp_desc.
        depthAttachment.loadAction =
            MTLLoadActionClear;
    
    id<MTLRenderCommandEncoder> pass_5_comp =
        [combuf
            renderCommandEncoderWithDescriptor:
                pass_5_comp_desc];
    [pass_5_comp setViewport: ags->window_viewport];
    [pass_5_comp setCullMode: MTLCullModeNone];
    [pass_5_comp
        setRenderPipelineState: ags->singlequad_pls];
    log_assert(ags->quad_vertices != NULL);
    [pass_5_comp
        setVertexBytes:
            ags->quad_vertices
        length:
            sizeof(T1PostProcessingVertex)*6
        atIndex:
            0];
    [pass_5_comp
        setVertexBuffer:
            ags->postprocessing_constants_buffers[ags->frame_i]
        offset:0
        atIndex:1];
    
    id<MTLTexture> arr_tex = ags->metal_textures[
        T1_render_views[0].write_array_i];
    id<MTLTexture> sliced_tex = [arr_tex
        newTextureViewWithPixelFormat:
            arr_tex.pixelFormat
        textureType:
            MTLTextureType2D
        levels:
            NSMakeRange(0, arr_tex.mipmapLevelCount)
        slices:
            NSMakeRange(
                (NSUInteger)T1_render_views[0].
                    write_slice_i,
                1)];
    [pass_5_comp
        setFragmentTexture: sliced_tex
        atIndex:0];
    
    #if T1_BLOOM_ACTIVE == T1_ACTIVE
    [pass_5_comp
        setFragmentTexture:
            ags->downsampled_rtts[1]
        atIndex: 1];
    [pass_5_comp
        setFragmentTexture:
            ags->downsampled_rtts[2]
        atIndex:2];
    [pass_5_comp
        setFragmentTexture:
            ags->downsampled_rtts[3]
        atIndex:3];
    [pass_5_comp
        setFragmentTexture:
            ags->downsampled_rtts[4]
        atIndex:4];
    #elif T1_BLOOM_ACTIVE == T1_INACTIVE
    // Pass
    #else
    #error
    #endif
    
    int32_t perlin_ta_i =
        f->postproc_consts->perlin_texturearray_i;
    #if T1_LOGGER_ASSERTS_ACTIVE == T1_ACTIVE
    int32_t perlin_t_i =
        f->postproc_consts->perlin_texture_i;
    // log_assert(perlin_ta_i >= 1);
    log_assert(perlin_t_i == 0);
    
    [pass_5_comp
        setFragmentTexture: ags->metal_textures[perlin_ta_i]
        atIndex:6];
    #elif T1_LOGGER_ASSERTS_ACTIVE == T1_INACTIVE
    // Pass
    #else
    #error
    #endif
    
    [pass_5_comp
        setFragmentTexture: ags->camera_depth_texture
        atIndex:CAMERADEPTH_TEXTUREARRAY_I];
    [pass_5_comp
        drawPrimitives:MTLPrimitiveTypeTriangle
        vertexStart:0
        vertexCount:6];
    [pass_5_comp endEncoding];
    [combuf presentDrawable: [view currentDrawable]];
    
    ags->frame_i += 1;
    ags->frame_i %= MAX_FRAME_BUFFERS;
    log_assert(ags->frame_i < MAX_FRAME_BUFFERS);
    
    [combuf addCompletedHandler:^(id<MTLCommandBuffer> arg_cmd_buffer) {
        (void)arg_cmd_buffer;
        
        #if T1_DRAWING_SEMAPHORE_ACTIVE == T1_ACTIVE
        dispatch_semaphore_signal(ags->drawing_semaphore);
        #elif T1_DRAWING_SEMAPHORE_ACTIVE == T1_INACTIVE
        #else
        #error
        #endif
    }];
    
    [combuf commit];
    
    funcptr_gameloop_after_render();
}

- (void)mtkView:(MTKView *)view
    drawableSizeWillChange:(CGSize)size
{
}
@end

void T1_platform_gpu_update_internal_render_viewport(
    const uint32_t at_i)
{
    ags->viewports_set[at_i] = false;
    [apple_gpu_delegate
        updateRenderViewSize:at_i];
}

void T1_platform_gpu_update_window_viewport(void)
{
    [apple_gpu_delegate updateFinalWindowSize];
}

int32_t T1_apple_gpu_make_depth_tex(
    const uint32_t width,
    const uint32_t height)
{
    int32_t slice_i = 0;
    while (ags->depth_textures[slice_i] != nil) {
        slice_i += 1;
    }
    
    log_assert(slice_i < T1_RENDER_VIEW_CAP);
    
    if (slice_i >= T1_RENDER_VIEW_CAP) {
        return -1;
    }
    
    MTLTextureDescriptor * desc =
        [[MTLTextureDescriptor alloc] init];;
    
    desc.width = width;
    desc.height = height;
    desc.textureType = MTLTextureType2D;
    desc.pixelFormat = MTLPixelFormatDepth32Float;
    desc.usage =
        MTLTextureUsageRenderTarget |
        MTLTextureUsageShaderRead;
    
    ags->depth_textures[slice_i] =
        [ags->device newTextureWithDescriptor: desc];
    
    log_assert(ags->depth_textures[slice_i] != nil);
    log_assert(
        (ags->depth_textures[slice_i].usage &
            MTLTextureUsageRenderTarget) > 0);
    
    return slice_i;
}
