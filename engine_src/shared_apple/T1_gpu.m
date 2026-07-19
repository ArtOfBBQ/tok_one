#import "T1_gpu.h"

#include "T1_global.h"
#include "T1_mem.h"
#include "T1_log.h"
#include "T1_settings.h"
#include "T1_material.h"
#include "T1_mesh_summary.h"
#include "T1_tex_array.h"
#include "T1_render_view.h"
#include "T1_platform_layer.h"

#define T1_DRAWING_SEMAPHORE_ACTIVE T1_INACTIVE

typedef struct {
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
    
    id polygon_buffers[T1_FRAMES_CAP];
    id matrix_buffers[T1_FRAMES_CAP];
    id light_buffers [T1_FRAMES_CAP];
    id vertex_buffers[T1_FRAMES_CAP];
    id flat_quad_buffers[T1_FRAMES_CAP];
    id flat_texquad_buffers[T1_FRAMES_CAP];
    id cam_buffers[T1_FRAMES_CAP];
    id postprocessing_constants_buffers[T1_FRAMES_CAP];
    id locked_vertex_populator_buffer;
    id locked_vertex_buffer;
    id locked_matf32_populator_buffer;
    id locked_mats32_populator_buffer;
    id locked_matf32_buffer;
    id locked_mats32_buffer;
    id projection_constants_buffer;
    
    // id<MTLTexture> cam_depth_texture;
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
    id<MTLRenderPipelineState> blend_touch_pls;
    id<MTLRenderPipelineState> bb_touch_pls;
    id<MTLRenderPipelineState> diamond_notouch_pls;
    id<MTLRenderPipelineState> depth_only_pls;
    id<MTLRenderPipelineState> blend_notouch_pls;
    id<MTLRenderPipelineState> bb_notouch_pls;
    id<MTLRenderPipelineState> flat_texquad_touch_pls;
    
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
    id<MTLTexture> metal_textures[T1_TEXARRAYS_CAP];
    #elif T1_TEXTURES_ACTIVE == T1_INACTIVE
    id<MTLTexture> metal_textures[1]; // for font only
    #else
    #error
    #endif
    
    #if T1_BLOOM_ACTIVE == T1_ACTIVE
    id<MTLTexture> downsampled_rtts[T1_DOWNSAMPLES_SIZE];
    #elif T1_BLOOM_ACTIVE == T1_INACTIVE
    #else
    #error
    #endif
    
    id<MTLTexture> touch_id_texture;
    id<MTLBuffer>  touch_id_buffer;
    id<MTLBuffer>  touch_id_buffer_all_zeros;
    
    id<MTLTexture> cur_rtt;
    id<MTLTexture> cur_depth;
    id<MTLRenderPipelineState> cur_opq_pls;
    id<MTLRenderPipelineState> cur_blnd_pls;
    id<MTLRenderPipelineState> cur_bloom_pls;
    id<MTLRenderPipelineState> cur_bb_pls;
    id<MTLRenderPipelineState> cur_flat_texquad_pls;
    
    T1PostProcessingVertex quad_vertices[6];
    float retina_scaling_factor;
    u8 viewports_set[T1_RENDER_VIEW_CAP];
    b8 metal_active;
    b8 zbuf_cleared;
    b8 rtt_cleared;
} AppleGPUState;

static AppleGPUState * ags = NULL;

MetalKitViewDelegate * apple_gpu_delegate = NULL;

static void (* funcptr_gameloop_before_render)(T1GPUFrame *) = NULL;
static void (* funcptr_gameloop_after_render)(void) = NULL;

u8 T1_apple_gpu_init(
    void (* arg_funcptr_shared_gameloop_update)(T1GPUFrame *),
    void (* arg_funcptr_shared_gameloop_update_after_render_pass)(void),
    id<MTLDevice> with_metal_device,
    NSString * shader_lib_filepath,
    float backing_scale_factor,
    char * error_msg_string)
{
    if (T1_cpu_to_gpu_data == NULL) {
        T1_std_strcpy_cap(error_msg_string, 128, "GPU frame buffer was not initialized");
        return false;
    }
    
    ags = T1_mem_malloc_unmanaged(sizeof(AppleGPUState));
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
        T1_log_append("failed to load default shader lib, trying ");
        T1_log_append(
            [shader_lib_filepath
                cStringUsingEncoding: NSASCIIStringEncoding]);
        T1_log_append("\n");
        
        NSURL * shader_lib_url = [NSURL
            fileURLWithPath: shader_lib_filepath
            isDirectory: false];
        
        if (shader_lib_url == NULL) {
            #if T1_LOG_ASSERTS_ACTIVE == T1_ACTIVE
            T1_log_dump_and_crash("Failed to find the shader lib\n");
            #elif T1_LOG_ASSERTS_ACTIVE == T1_INACTIVE
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
            T1_log_append("Failed to find the shader library\n");
            #if T1_LOG_ASSERTS_ACTIVE == T1_ACTIVE
            T1_log_dump_and_crash((char *)[
                [[Error userInfo] descriptionInStringsFileFormat]
                    cStringUsingEncoding:NSASCIIStringEncoding]);
            #elif T1_LOG_ASSERTS_ACTIVE == T1_INACTIVE
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
            T1_log_append(
                "Success! Found the shader lib on 2nd try.\n");
        }
    }
    
    id<MTLFunction> vertex_shader =
        [ags->lib newFunctionWithName:
            @"vertex_shader"];
    if (vertex_shader == NULL) {
        T1_log_append(
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
    if (
        z_prepass_fragment_shader == NULL)
    {
        T1_log_append("Missing function: z_prepass_fragment_shader()!");
        
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
        T1_log_dump_and_crash((char *)[
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
    
    T1_log_assert(Error == nil);
    flat_billboard_quad_pls_desc.
        colorAttachments[1] = nil;
    ags->bb_notouch_pls =
       [with_metal_device
            newRenderPipelineStateWithDescriptor:
                flat_billboard_quad_pls_desc
            error:
                &Error];
    
    id<MTLFunction>
        flat_texquad_vert_shader =
            [ags->lib newFunctionWithName:
                @"flat_texquad_vertex_shader"];
    if (
        flat_texquad_vert_shader == NULL)
    {
        T1_std_strcpy_cap(
            error_msg_string,
            512,
            "Missing function: "
            "flat_texquad_vertex_shader()");
        return false;
    }
    
    id<MTLFunction> flat_texquad_frag_shader =
        [ags->lib newFunctionWithName:
            @"flat_texquad_fragment_shader"];
    if (flat_texquad_frag_shader == NULL) {
        T1_std_strcpy_cap(
            error_msg_string,
            512,
            "Missing function: "
            "flat_texquad_fragment_shader()");
        return false;
    }
    
    MTLRenderPipelineDescriptor *
        flat_texquad_pls_desc =
            [MTLRenderPipelineDescriptor new];
    [flat_texquad_pls_desc
        setVertexFunction: flat_texquad_vert_shader];
    [flat_texquad_pls_desc
        setFragmentFunction:
            flat_texquad_frag_shader];
    flat_texquad_pls_desc.label =
        @"flat texquad pipeline state";
    flat_texquad_pls_desc
        .colorAttachments[0]
        .pixelFormat = ags->pixel_format_renderpass1;
    [flat_texquad_pls_desc
        .colorAttachments[0]
        setBlendingEnabled: YES];
    flat_texquad_pls_desc
        .colorAttachments[0].sourceRGBBlendFactor =
            MTLBlendFactorSourceAlpha;
    flat_texquad_pls_desc
        .colorAttachments[0].destinationRGBBlendFactor =
            MTLBlendFactorOneMinusSourceAlpha;
    flat_texquad_pls_desc
        .colorAttachments[0].rgbBlendOperation =
            MTLBlendOperationAdd;
    flat_texquad_pls_desc.colorAttachments[1].
        pixelFormat = ags->pixel_format_renderpass1;
    flat_texquad_pls_desc.
        depthAttachmentPixelFormat =
            MTLPixelFormatDepth32Float;
    
    ags->flat_texquad_touch_pls =
        [with_metal_device
            newRenderPipelineStateWithDescriptor:
                flat_texquad_pls_desc
            error:
                &Error];
    
    T1_log_assert(Error == nil);
    
    
    
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
        #if T1_LOG_ASSERTS_ACTIVE == T1_ACTIVE
        T1_log_dump_and_crash(error_msg_string);
        #elif T1_LOG_ASSERTS_ACTIVE == T1_INACTIVE
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
    T1_log_assert(Error == NULL);
    
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
    alpha_pls_desc.label =
        @"Alphablending pipeline";
    ags->blend_touch_pls =
        [with_metal_device
            newRenderPipelineStateWithDescriptor:
                alpha_pls_desc
            error:
                &Error];
    
    if (Error != NULL)
    {
        T1_log_append(
            [[Error localizedDescription]
                cStringUsingEncoding:
                    kCFStringEncodingASCII]);
        
        #if T1_LOG_ASSERTS_ACTIVE == T1_ACTIVE
        T1_log_dump_and_crash(
            "Error loading the alpha "
            "blending shader\n");
        #elif T1_LOG_ASSERTS_ACTIVE == T1_INACTIVE
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
    ags->blend_notouch_pls =
        [with_metal_device
            newRenderPipelineStateWithDescriptor:
                alpha_pls_desc 
            error:
                &Error];
    T1_log_assert(Error == NULL);
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
        T1_log_append(
            [[Error localizedDescription]
                cStringUsingEncoding:
                    kCFStringEncodingASCII]);
        #if T1_LOG_ASSERTS_ACTIVE == T1_ACTIVE
        T1_log_dump_and_crash(
            "Error setting the depth "
            "stencil state\n");
        #elif T1_LOG_ASSERTS_ACTIVE == T1_INACTIVE
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
        u32 fram_i = 0;
        fram_i < T1_FRAMES_CAP;
        fram_i++)
    {
        T1GPUFrame * f = &T1_cpu_to_gpu_data->
            triple_buffers[fram_i];
        
        T1_log_assert(
            T1_cpu_to_gpu_data->
                polygons_alloc_size >=
            (sizeof(T1GPUzSprite) *
                T1_ZSPRITES_CAP));
        
        T1_log_assert(f->zsprite_list != NULL);
        id<MTLBuffer> MTLBufferFramePolygons =
            [with_metal_device
                /* the ptr needs to be page aligned */
                    newBufferWithBytesNoCopy:
                        f->zsprite_list->polygons
                /* the length weirdly needs to be page aligned also */
                    length:
                        T1_cpu_to_gpu_data->polygons_alloc_size
                    options:
                        MTLResourceStorageModeShared
                /* deallocator = nil to opt out */
                    deallocator:
                        nil];
        ags->polygon_buffers[fram_i] = MTLBufferFramePolygons;
        
        id<MTLBuffer> MTLBufferFrameMatrices =
            [with_metal_device
                /* the ptr needs to be page aligned */
                    newBufferWithBytesNoCopy:
                        f->matrices
                /* the length weirdly needs to be page aligned also */
                    length:
                        T1_cpu_to_gpu_data->matrices_alloc_size
                    options:
                        MTLResourceStorageModeShared
                /* deallocator = nil to opt out */
                    deallocator:
                        nil];
        ags->matrix_buffers[fram_i] = MTLBufferFrameMatrices;
        
        id<MTLBuffer> MTLBufferFrameVertices =
            [with_metal_device
                /* the ptr needs to be page aligned */
                    newBufferWithBytesNoCopy:
                        f->verts
                /* the length weirdly needs to be page aligned also */
                    length:
                        T1_cpu_to_gpu_data->vertices_alloc_size
                    options:
                        MTLResourceStorageModeShared
                /* deallocator = nil to opt out */
                    deallocator:
                        nil];
        
        ags->vertex_buffers[fram_i] = MTLBufferFrameVertices;
        
        id<MTLBuffer> MTLBufferFrameCircles =
            [with_metal_device
                /* the ptr needs to be page aligned */
                    newBufferWithBytesNoCopy:
                        f->flat_bb_quads
                /* the length weirdly needs to be page aligned also */
                    length:
                        T1_cpu_to_gpu_data->flat_quads_alloc_size
                    options:
                        MTLResourceStorageModeShared
                /* deallocator = nil to opt out */
                    deallocator:
                        nil];
        
        ags->flat_quad_buffers[fram_i] =
            MTLBufferFrameCircles;
        
        id<MTLBuffer> MTLBufferTexQuads =
            [with_metal_device
                /* the ptr needs to be page aligned */
                    newBufferWithBytesNoCopy:
                        f->flat_tex_quads
                /* the length weirdly needs to be page aligned also */
                    length:
                        T1_cpu_to_gpu_data->
                            flat_texquads_alloc_size
                    options:
                        MTLResourceStorageModeShared
                /* deallocator = nil to opt out */
                    deallocator:
                        nil];
        
        ags->flat_texquad_buffers[fram_i] =
            MTLBufferTexQuads;
        
        id<MTLBuffer> MTLBufferPostProcessingConstants =
            [with_metal_device
                /* the ptr needs to be page aligned */
                    newBufferWithBytesNoCopy:
                        f->postproc_consts
                /* the length weirdly needs to be page aligned also */
                    length:
                        T1_cpu_to_gpu_data->
                            postprocessing_constants_alloc_size
                    options:
                        MTLResourceStorageModeShared
                /* deallocator = nil to opt out */
                    deallocator:
                        nil];
        T1_log_assert(MTLBufferPostProcessingConstants != nil);
        
        ags->postprocessing_constants_buffers[fram_i] =
            MTLBufferPostProcessingConstants;
        
        id<MTLBuffer> MTLBufferFrameLights =
            [with_metal_device
                /* the ptr needs to be page aligned */
                    newBufferWithBytesNoCopy:
                        f->lights
                /* the length weirdly needs to be page aligned also */
                    length:
                        T1_cpu_to_gpu_data->lights_alloc_size
                    options:
                        MTLResourceStorageModeShared | MTLResourceUsageRead
                /* deallocator = nil to opt out */
                    deallocator:
                        nil];
        
        ags->light_buffers[fram_i] = MTLBufferFrameLights;
        
        T1_log_assert(
            T1_cpu_to_gpu_data->
                render_views_alloc_size >=
                    (sizeof(T1GPURenderView) *
                        T1_RENDER_VIEW_CAP));
        id<MTLBuffer> MTLBufferFrameCamera =
        [with_metal_device
            /* needs to be page aligned */
                newBufferWithBytesNoCopy:
                    f->render_views
            /* also needs to be aligned */
                length:
                    T1_cpu_to_gpu_data->
                        render_views_alloc_size
                options:
                    MTLResourceStorageModeShared
            /* deallocator = nil to opt out */
                deallocator:
                    nil];
        
        ags->cam_buffers[fram_i] =
            MTLBufferFrameCamera;
    }
    
    id<MTLBuffer> MTLBufferLockedVerticesPopulator =
        [with_metal_device
            /* the ptr needs to be page aligned */
                newBufferWithBytesNoCopy:
                    T1_cpu_to_gpu_data->locked_vertices
            /* the length weirdly needs to be page aligned also */
                length:
                    T1_cpu_to_gpu_data->locked_vertices_alloc_size
                options:
                    MTLResourceStorageModeShared
            /* deallocator = nil to opt out */
                deallocator:
                    nil];
    
    ags->locked_vertex_populator_buffer = MTLBufferLockedVerticesPopulator;
    
    id<MTLBuffer> MTLBufferLockedVertices =
        [with_metal_device
            newBufferWithLength:
                T1_cpu_to_gpu_data->
                    locked_vertices_alloc_size
            options:
                MTLResourceStorageModePrivate];
    ags->locked_vertex_buffer = MTLBufferLockedVertices;
    
    id<MTLBuffer> MTLBufferLockedMatf32Populator =
        [with_metal_device
            /* the ptr needs to be page aligned */
                newBufferWithBytesNoCopy:
                    T1_cpu_to_gpu_data->const_mats_f32
            /* the length weirdly needs to be page aligned also */
                length:
                    T1_cpu_to_gpu_data->const_matsf32_alloc_size
                options:
                    MTLResourceStorageModeShared
            /* deallocator = nil to opt out */
                deallocator:
                    nil];
    
    id<MTLBuffer> MTLBufferLockedMats32Populator =
        [with_metal_device
            /* the ptr needs to be page aligned */
                newBufferWithBytesNoCopy:
                    T1_cpu_to_gpu_data->const_mats_s32
            /* the length weirdly needs to be page aligned also */
                length:
                    T1_cpu_to_gpu_data->const_matss32_alloc_size
                options:
                    MTLResourceStorageModeShared
            /* deallocator = nil to opt out */
                deallocator:
                    nil];
    
    ags->locked_matf32_populator_buffer = MTLBufferLockedMatf32Populator;
    
    ags->locked_mats32_populator_buffer = MTLBufferLockedMats32Populator;
    
    id<MTLBuffer> MTLBufferLockedMatsf32 =
        [with_metal_device
            newBufferWithLength:
                T1_cpu_to_gpu_data->const_matsf32_alloc_size
            options:
                MTLResourceStorageModePrivate];
    ags->locked_matf32_buffer = MTLBufferLockedMatsf32;
    
    id<MTLBuffer> MTLBufferLockedMatss32 =
        [with_metal_device
            newBufferWithLength:
                T1_cpu_to_gpu_data->const_matss32_alloc_size
            options:
                MTLResourceStorageModePrivate];
    ags->locked_mats32_buffer = MTLBufferLockedMatss32;
    
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
        T1_log_append(
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
        [ags->lib newFunctionWithName:
            @"single_quad_fragment_shader"];
    
    if (singlequad_fragment_shader == NULL) {
        T1_log_append("Missing function: downsampling_fragment_shader()!");
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
    singlequad_pipeline_descriptor.label =
        @"single-quad pipeline";
    // singlequad_pipeline_descriptor.sampleCount = 1;
    [singlequad_pipeline_descriptor
        setVertexFunction: singlequad_vertex_shader];
    [singlequad_pipeline_descriptor
        setFragmentFunction: singlequad_fragment_shader];
    singlequad_pipeline_descriptor.colorAttachments[0].pixelFormat =
        MTLPixelFormatBGRA8Unorm;
    [singlequad_pipeline_descriptor.colorAttachments[0]
        setBlendingEnabled: YES];
    singlequad_pipeline_descriptor.colorAttachments[0].
        sourceRGBBlendFactor = MTLBlendFactorSourceAlpha;
    singlequad_pipeline_descriptor.colorAttachments[0].
        destinationRGBBlendFactor =
            MTLBlendFactorOneMinusSourceAlpha;
    singlequad_pipeline_descriptor.depthAttachmentPixelFormat =
        MTLPixelFormatDepth32Float;
    singlequad_pipeline_descriptor.vertexBuffers[0].
        mutability = MTLMutabilityImmutable;
    ags->singlequad_pls = [with_metal_device
        newRenderPipelineStateWithDescriptor:
            singlequad_pipeline_descriptor
        error:NULL];
    
    ags->command_queue = [with_metal_device newCommandQueue];
    
    ags->metal_active = true;
    
    return true;
}

#if T1_BLOOM_ACTIVE == T1_ACTIVE
static float get_ds_width(
    const u32 ds_i,
    const u32 base_width)
{
    float return_value = base_width;
    
    for (u32 i = 0; i < ds_i && i < T1_DOWNSAMPLES_CUTOFF; i++) {
        return_value *= 0.5f;
    }
    
    return return_value;
}

static float get_ds_height(
    const u32 ds_i,
    const u32 base_height)
{
    float return_value = base_height;
    
    for (u32 i = 0; i < ds_i && i < T1_DOWNSAMPLES_CUTOFF; i++) {
        return_value *= 0.5f;
    }
    
    return return_value;
}
#elif T1_BLOOM_ACTIVE == T1_INACTIVE
#else
#error
#endif

void T1_os_gpu_get_device_name(
    char * recipient,
    const u32 recipient_cap)
{
    #if T1_LOG_ASSERTS_ACTIVE == T1_ACTIVE
    // pass
    #elif T1_LOG_ASSERTS_ACTIVE == T1_INACTIVE
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

void T1_os_gpu_update_capacity_if_needed(
    const s32 tex_array_i)
{
    T1_log_assert(tex_array_i >=  0);
    T1_log_assert(tex_array_i <  31);
    u8 copy_prev = false;
    id<MTLTexture> prev_copy = nil;
    
    if (T1_tex_arrays[tex_array_i].deleted) {
        ags->metal_textures[tex_array_i] = nil;
        return;
    } else if (
        ags->metal_textures[tex_array_i] == NULL ||
        (T1_tex_arrays[tex_array_i].bc1_compressed &&
            ags->metal_textures[tex_array_i].pixelFormat !=
                    MTLPixelFormatBC1_RGBA) ||
        ((!T1_tex_arrays[tex_array_i].bc1_compressed) &&
            ags->metal_textures[tex_array_i].pixelFormat !=
                    MTLPixelFormatRGBA8Unorm) ||
        T1_tex_arrays[tex_array_i].single_img_width !=
            ags->metal_textures[tex_array_i].width ||
        T1_tex_arrays[tex_array_i].single_img_height !=
            ags->metal_textures[tex_array_i].height)
    {
        ags->metal_textures[tex_array_i] = nil;
        copy_prev = false;
    } else if (
        T1_tex_arrays[tex_array_i].images_size >
            [ags->metal_textures[tex_array_i] arrayLength] )
    {
        #if T1_LOG_ASSERTS_ACTIVE == T1_ACTIVE
        T1_log_assert(
            T1_tex_arrays[tex_array_i].single_img_width ==
                ags->metal_textures[tex_array_i].width);
        T1_log_assert(
            T1_tex_arrays[tex_array_i].single_img_height ==
                ags->metal_textures[tex_array_i].height);
        T1_log_assert(
            T1_tex_arrays[tex_array_i].is_render_target ==
                ((ags->metal_textures[tex_array_i].usage &
                    MTLTextureUsageRenderTarget) > 0));
        if (T1_tex_arrays[tex_array_i].bc1_compressed) {
            T1_log_assert(
                ags->metal_textures[tex_array_i].pixelFormat ==
                    MTLPixelFormatBC1_RGBA);
        }
        #elif T1_LOG_ASSERTS_ACTIVE == T1_INACTIVE
        #else
        #error
        #endif
        
        copy_prev = true;
        prev_copy = ags->metal_textures[tex_array_i];
        ags->metal_textures[tex_array_i] = NULL;
    } else {
        T1_log_assert(
            T1_tex_arrays[tex_array_i].is_render_target ==
                ((ags->metal_textures[tex_array_i].usage &
                    MTLTextureUsageRenderTarget) > 0));
        return;
    }
    
    MTLTextureDescriptor * texture_descriptor =
        [[MTLTextureDescriptor alloc] init];
    texture_descriptor.textureType = MTLTextureType2DArray;
    texture_descriptor.arrayLength = T1_tex_arrays[tex_array_i].
        images_size;
    texture_descriptor.pixelFormat = T1_tex_arrays[tex_array_i].
        bc1_compressed ?
            MTLPixelFormatBC1_RGBA :
            MTLPixelFormatRGBA8Unorm;
    texture_descriptor.storageMode = MTLStorageModePrivate;
    
    if (T1_tex_arrays[tex_array_i].is_render_target)
    {
        texture_descriptor.usage =
            MTLTextureUsageShaderRead |
            MTLTextureUsageRenderTarget;
    } else {
        texture_descriptor.usage = MTLTextureUsageShaderRead;
    }
    texture_descriptor.width =
        T1_tex_arrays[tex_array_i].single_img_width;
    texture_descriptor.height =
        T1_tex_arrays[tex_array_i].single_img_height;
    
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
    
    T1_log_assert(ags->metal_textures[tex_array_i] == NULL);
    ags->metal_textures[tex_array_i] = texture;
    
    T1_tex_arrays[tex_array_i].gpu_capacity =
        T1_tex_arrays[tex_array_i].images_size;
    
    if (copy_prev) {
        id<MTLCommandBuffer> combuf = [ags->command_queue commandBuffer];
        id<MTLBlitCommandEncoder> blitenc = [combuf blitCommandEncoder];
        
        [blitenc
            copyFromTexture:
                prev_copy
            toTexture:
                ags->metal_textures[tex_array_i]];
        
        [blitenc endEncoding];
        [combuf commit];
        [combuf waitUntilCompleted];
    }
}

// returns the slice_i of the new depth texture
// in the array of depth textures
s16
T1_os_gpu_make_depth_tex(
    const u32 width,
    const u32 height)
{
    T1_log_assert(height <= 4000);
    T1_log_assert(width <= 4000);
    
    return T1_apple_gpu_make_depth_tex(
        width,
        height);
}

s32 T1_os_gpu_get_touch_id_at_screen_pos(
    const f32 screen_x,
    const f32 screen_y)
{
    if (
        screen_x < 0 ||
        screen_y < 0 ||
        screen_x >= T1_global->window_wh[0] ||
        screen_y >= T1_global->window_wh[1])
    {
        return -1;
    }
    
    u32 rtt_width  = (u32)ags->render_viewports[0].width;
    u32 rtt_height = (u32)ags->render_viewports[0].height;
    u32 win_width  = (u32)T1_global->window_wh[0];
    u32 win_height = (u32)T1_global->window_wh[1];
    
    u32 screen_x_adj = (u32)((screen_x * rtt_width) /
        win_width);
    u32 screen_y_adj = (u32)(
        ((win_height - screen_y) * rtt_height) /
        win_height);
    
    if (screen_x_adj >= rtt_width ) {
        screen_x_adj = rtt_width;
    }
    if (screen_y_adj >= rtt_height)
    {
        screen_y_adj = rtt_height;
    }
    
    u8 * data = (u8 *)[ags->touch_id_buffer contents];
    u64 size = [ags->touch_id_buffer allocatedSize];
    
    u32 pixel_i = (screen_y_adj * rtt_width) + screen_x_adj;
    
    if (((pixel_i * 4) + 3) >= size)
    {
        return -1;
    }
    
    // See shaders for the packing logic
    u32 first_8bits  = data[(pixel_i*4)+0];
    u32 second_8bits = data[(pixel_i*4)+1];
    u32 third_8bits  = data[(pixel_i*4)+2];
    u32 fourth_8bits = data[(pixel_i*4)+3];
    
    u32 uid =
        (fourth_8bits << 24) |
        (third_8bits  << 16) |
        (second_8bits << 8) |
        first_8bits;
    s32 final_id = *(s32 *)&uid;
    
    if (final_id < -1) { final_id = -1; }
    
    return final_id;
}

void T1_os_gpu_delete_texture_array(
    const s32 array_i)
{
    T1_log_assert(array_i != T1_DEPTH_TEXTUREARRAYS_I);
    
    ags->metal_textures[array_i] = nil;
}

void T1_os_gpu_delete_depth_tex(
    const s32 slice_i)
{
    T1_log_assert(slice_i < T1_RENDER_VIEW_CAP);
    
    ags->depth_textures[slice_i] = nil;
}

#if T1_TEXTURES_ACTIVE == T1_ACTIVE
void T1_os_gpu_fetch_rgba_at(
    const s32 texture_array_i,
    const s32 texture_i,
    u8 * rgba_recipient,
    u32 * recipient_size,
    u32 * recipient_width,
    u32 * recipient_height,
    const u32 recipient_cap,
    u32 * good)
{
    // Validate inputs
    T1_log_assert(texture_i >= 0);
    T1_log_assert(texture_array_i >= 0);
    T1_log_assert(texture_array_i < T1_TEXARRAYS_CAP);
    T1_log_assert(rgba_recipient != NULL);
    T1_log_assert(recipient_size != NULL);
    T1_log_assert(good != NULL);
    
    *good = false;
    
    // Check if the texture array exists
    id<MTLTexture> texture = ags->metal_textures[texture_array_i];
    if (texture == nil || texture.textureType != MTLTextureType2DArray) {
        return;
    }
    
    if (texture.pixelFormat != MTLPixelFormatRGBA8Unorm)
    {
        // Ensure the texture format is RGBA8Unorm
        // for direct copying to u8 RGBA
        return;
    }
    
    *recipient_width = (u32)texture.width;
    *recipient_height = (u32)texture.height;
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
    
    if (texture_i >= (s32)texture.arrayLength) {
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
    
    *recipient_size = (u32)bytes_per_image;
    
    *good = true;
}
#elif T1_TEXTURES_ACTIVE == T1_INACTIVE
// Pass
#else
#error
#endif

#if T1_MIPMAPS_ACTIVE == T1_ACTIVE
void T1_os_gpu_generate_mipmaps_for_texture_array(
    const s32 texture_array_i)
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

void
T1_os_gpu_push_tex_slice_and_free_rgba(
    const s32 tex_array_i,
    const s32 tex_slice_i)
{
    u8 * rgba_freeable =
        T1_tex_arrays[tex_array_i].images[tex_slice_i].image.rgba_values_freeable;
    u8 * rgba_page_aligned =
        T1_tex_arrays[tex_array_i].images[tex_slice_i].image.rgba_values_page_aligned;
    
    T1_log_assert(rgba_freeable != NULL);
    T1_log_assert(rgba_page_aligned != NULL);
    
    T1_log_assert(tex_slice_i >= 0);
    T1_log_assert(tex_array_i >= 0);
    T1_log_assert(tex_array_i < T1_TEXARRAYS_CAP);
    
    T1_os_gpu_update_capacity_if_needed(tex_array_i);
    
    u32 img_width = T1_tex_arrays[tex_array_i].single_img_width;
    u32 img_height = T1_tex_arrays[tex_array_i].single_img_height;
    
    u32 temp_buf_cap = img_width * img_height * 4;
    T1_log_assert(temp_buf_cap > 0);
    while (temp_buf_cap % T1_mem_page_size != 0) {
        temp_buf_cap++;
    }
    
    T1_log_assert(T1_mem_is_page_aligned(rgba_page_aligned));
    
    vm_address_t vm_ptr;
    vm_size_t vm_size = temp_buf_cap;  // Already a page multiple
    kern_return_t err = vm_allocate(mach_task_self(), &vm_ptr, vm_size, VM_FLAGS_ANYWHERE);
    if (err != KERN_SUCCESS) {
        T1_log_assert(0);
        return;
    }
    
    u8 * temp_src_buf_ptr = (u8 *)vm_ptr;
    
    T1_std_memset(temp_src_buf_ptr, 0, temp_buf_cap);
    if (T1_tex_arrays[tex_array_i].bc1_compressed) {
        T1_std_memcpy(
            temp_src_buf_ptr ,
            rgba_page_aligned + 128,
            T1_tex_arrays[tex_array_i].images[tex_slice_i].
                image.rgba_values_size);
    } else {
        T1_std_memcpy(
            temp_src_buf_ptr,
            rgba_page_aligned,
            T1_tex_arrays[tex_array_i].images[tex_slice_i].
                image.rgba_values_size);
    }
    
    id<MTLBuffer> temp_source_buf =
        [ags->device
            /* the ptr needs to be page aligned */
            newBufferWithBytesNoCopy:
                temp_src_buf_ptr
            /* the length weirdly needs to be page aligned also */
            length:
                temp_buf_cap
            options:
                MTLResourceStorageModeShared
            /* deallocator = nil to opt out */
            deallocator:
                nil];
    
    if (temp_source_buf == NULL) {
        T1_log_assert(0);
        return;
    }
    
    id <MTLCommandBuffer> combuf = [ags->command_queue commandBuffer];
    
    id <MTLBlitCommandEncoder> blit_copy_encoder =
        [combuf blitCommandEncoder];
    
    [blit_copy_encoder
        copyFromBuffer:
            temp_source_buf
        sourceOffset:
            0
        sourceBytesPerRow:
            T1_tex_arrays[tex_array_i].bc1_compressed ?
                ((img_width + 3) / 4) * 8 :
                img_width * 4
        sourceBytesPerImage:
            T1_tex_arrays[tex_array_i].bc1_compressed ?
                ((img_width + 3) / 4) * ((img_height + 3) / 4) * 8 :
                img_width * img_height * 4
        sourceSize:
            MTLSizeMake(img_width, img_height, 1)
        toTexture:
            ags->metal_textures[tex_array_i]
        destinationSlice:
            (NSUInteger)tex_slice_i
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
    
    vm_deallocate(mach_task_self(), vm_ptr, vm_size);
    
    T1_mem_free_managed(rgba_freeable);
    T1_tex_arrays[tex_array_i].images[tex_slice_i].image.
        rgba_values_freeable = NULL;
    T1_tex_arrays[tex_array_i].images[tex_slice_i].image.
        rgba_values_page_aligned = NULL;
}

void T1_os_gpu_copy_locked_vertices(void)
{
    T1_cpu_to_gpu_data->locked_vertices_size = T1_mesh_summary_all_vertices->size;
    
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
            T1_cpu_to_gpu_data->locked_vertices_alloc_size];
    [blit_copy_encoder endEncoding];
    
    // Add a completion handler and commit the command buffer.
    [combuf addCompletedHandler:^(id<MTLCommandBuffer> cb) {
        // Populate private buffer.
        (void)cb;
    }];
    [combuf commit];
}

void T1_os_gpu_copy_locked_materials(void)
{
    T1_cpu_to_gpu_data->const_mats_size = all_mesh_materials->size;
    
    id <MTLCommandBuffer> combuf = [ags->command_queue commandBuffer];
    
    id <MTLBlitCommandEncoder> blit_copy_encoder = [combuf blitCommandEncoder];
    [blit_copy_encoder
        copyFromBuffer:
            ags->locked_matf32_populator_buffer
        sourceOffset:
            0
        toBuffer:
            ags->locked_matf32_buffer
        destinationOffset:
            0
        size:
            T1_cpu_to_gpu_data->const_matsf32_alloc_size];
    [blit_copy_encoder
        copyFromBuffer:
            ags->locked_mats32_populator_buffer
        sourceOffset:
            0
        toBuffer:
            ags->locked_mats32_buffer
        destinationOffset:
            0
        size:
            T1_cpu_to_gpu_data->const_matss32_alloc_size];
    [blit_copy_encoder endEncoding];
    
    // Add a completion handler and commit the command buffer.
    [combuf addCompletedHandler:^(id<MTLCommandBuffer> cb) {
        // Populate private buffer.
        (void)cb;
    }];
    [combuf commit];
}

static id<MTLTexture>
get_tex_slice(
    const s32 at_array_i,
    const s32 at_slice_i)
{
    T1_log_assert(at_array_i >= 0);
    T1_log_assert(at_array_i < T1_TEXARRAYS_CAP);
    T1_log_assert(at_slice_i >= 0);
    
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
    
    return retval;
}

static void
set_defaults_for_render_descriptor(
    MTLRenderPassDescriptor * desc,
    const s32 cam_i)
{
    if (!ags->zbuf_cleared) {
        desc.depthAttachment.loadAction = MTLLoadActionClear;
        desc.depthAttachment.clearDepth = 1.0f;
        
        ags->zbuf_cleared = true;
    } else {
        desc.depthAttachment.loadAction = MTLLoadActionLoad;
    }
    T1_log_assert(ags->cur_depth != nil);
    desc.depthAttachment. storeAction = MTLStoreActionStore;
    desc.depthAttachment.texture = ags->cur_depth;
    
    if (!ags->rtt_cleared) {
        desc.colorAttachments[0].loadAction = MTLLoadActionClear;
        desc.colorAttachments[0].clearColor =
            MTLClearColorMake(0.0f, 0.0f, 0.1f, 1.0f);
        ags->rtt_cleared = true;
    } else {
        desc.colorAttachments[0].loadAction = MTLLoadActionLoad;
    }
    
    desc.colorAttachments[0].
        texture = ags->cur_rtt;
    desc.colorAttachments[0].storeAction =
        MTLStoreActionStore;
    
    // ID Buffer for touchables
    if (
        T1_render_views->cpu[cam_i].write_type ==
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
    const u32 cam_i)
{
    T1_log_assert(cam_i < T1_RENDER_VIEW_CAP);
    
    T1_log_assert(ags->opaque_depth_stencil_state != nil);
    [encoder
        setDepthStencilState:
            ags->opaque_depth_stencil_state];
    
    [encoder setDepthClipMode: MTLDepthClipModeClip];
    [encoder setCullMode: MTLCullModeBack];
    [encoder setFrontFacingWinding:
        MTLWindingCounterClockwise];
    
    [encoder setViewport: ags->render_viewports[cam_i]];
    
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
    
    T1_log_assert(ags->matrix_buffers[ags->frame_i] != nil);
    [encoder
        setVertexBuffer:
            ags->matrix_buffers[ags->frame_i]
        offset:
            0
        atIndex:
            2];
    
    [encoder
        setVertexBuffer:
            ags->cam_buffers[ags->frame_i]
        offset: 0
        atIndex: 3];
    
    [encoder
        setVertexBytes: &cam_i
        length: sizeof(u32)
        atIndex: 4];
    
    [encoder
        setVertexBuffer:
            ags->locked_vertex_buffer
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
            ags->cam_buffers[ags->frame_i]
        offset:
            0
        atIndex:
            3];
    
    [encoder
        setFragmentBytes: &cam_i
        length: sizeof(u32)
        atIndex: 4];
    
    [encoder
        setFragmentBuffer:
            ags->locked_matf32_buffer
        offset:
            0
        atIndex:
            6];
    
    [encoder
        setFragmentBuffer:
            ags->locked_mats32_buffer
        offset:
            0
        atIndex:
            8];
    
    [encoder
        setFragmentBuffer:
            ags->postprocessing_constants_buffers[ags->frame_i]
        offset:
            0
        atIndex:
            7];
    
    #if T1_TEXTURES_ACTIVE == T1_ACTIVE
    for (
        u32 i = 0;
        i < T1_TEXARRAYS_CAP;
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
    for (
        u32 rv_i = 0;
        rv_i < T1_RENDER_VIEW_CAP;
        rv_i++)
    {
        [encoder
            setFragmentTexture: ags->depth_textures[rv_i]
            atIndex: T1_SHADOW_MAPS_1ST_FRAGARG_I + rv_i];
    }
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
    ags->window_viewport.width =
        T1_global->window_wh[0] *
            ags->retina_scaling_factor;
    ags->window_viewport.height  =
        T1_global->window_wh[1] *
            ags->retina_scaling_factor;
    T1_log_assert(ags->window_viewport.width > 0.0f);
    T1_log_assert(ags->window_viewport.height > 0.0f);
    
    /*
    These near/far values are the final viewport coordinates (after
    fragment shader), not to be confused with
    window_globals->projection_constants.near that's in our world space
    and much larger numbers
    */
    ags->window_viewport.znear = 0.001f;
    ags->window_viewport.zfar = 1.0f;
    
    #if 0
    MTLTextureDescriptor * camera_depth_texture_descriptor =
        [[MTLTextureDescriptor alloc] init];
    camera_depth_texture_descriptor.textureType = MTLTextureType2D;
    camera_depth_texture_descriptor.pixelFormat = MTLPixelFormatDepth32Float;
    camera_depth_texture_descriptor.width =
        (u64)ags->window_viewport.width;
    camera_depth_texture_descriptor.height =
        (u64)ags->window_viewport.height;
    camera_depth_texture_descriptor.storageMode = MTLStorageModePrivate;
    camera_depth_texture_descriptor.usage =
        MTLTextureUsageRenderTarget |
        MTLTextureUsageShaderRead;
    
    ags->cam_depth_texture =
        [ags->device newTextureWithDescriptor:
            camera_depth_texture_descriptor];
    #endif
}

- (void) updateRenderViewSize: (s32)at_i
{
    T1_log_assert(at_i >= 0);
    T1_log_assert(at_i < T1_RENDER_VIEW_CAP);
    
    ags->render_viewports[at_i].originX = 0;
    ags->render_viewports[at_i].originY = 0;
    ags->render_viewports[at_i].width   =
        T1_render_views->cpu[at_i].width;
    ags->render_viewports[at_i].height  =
        T1_render_views->cpu[at_i].height;
    T1_log_assert(ags->render_viewports[at_i].width > 0.0f);
    T1_log_assert(ags->render_viewports[at_i].height > 0.0f);
    
    /*
    These near/far values are the final viewport coordinates (after
    fragment shader), not to be confused with
    window_globals->projection_constants.near that's in our world space
    and much larger numbers
    */
    ags->render_viewports[at_i].znear = 0.001f;
    ags->render_viewports[at_i].zfar = 1.0f;
    
    ags->viewports_set[at_i] = true;
    
    MTLTextureDescriptor * zbufer_desc =
        [[MTLTextureDescriptor alloc] init];
    zbufer_desc.textureType = MTLTextureType2D;
    zbufer_desc.pixelFormat = MTLPixelFormatDepth32Float;
    zbufer_desc.width = T1_render_views->cpu[at_i].width;
    zbufer_desc.height = T1_render_views->cpu[at_i].height;
    zbufer_desc.storageMode = MTLStorageModePrivate;
    zbufer_desc.usage =
        MTLTextureUsageRenderTarget |
        MTLTextureUsageShaderRead;
    
    ags->depth_textures[at_i] =
        [ags->device newTextureWithDescriptor:
            zbufer_desc];
    
    if (at_i != 0) { return; }
    
    MTLTextureDescriptor * touch_id_tex_desc =
        [MTLTextureDescriptor new];
    touch_id_tex_desc.width =
        (NSUInteger)ags->render_viewports[at_i].width;
    touch_id_tex_desc.height =
        (NSUInteger)ags->render_viewports[at_i].height;
    touch_id_tex_desc.pixelFormat =
        MTLPixelFormatRGBA8Unorm;
    touch_id_tex_desc.mipmapLevelCount = 1;
    touch_id_tex_desc.storageMode = MTLStorageModePrivate;
    touch_id_tex_desc.usage =
        MTLTextureUsageRenderTarget |
        MTLTextureUsageShaderRead;
    ags->touch_id_texture =
        [ags->device
            newTextureWithDescriptor:
                touch_id_tex_desc];
    
    u64 touch_buffer_size_bytes =
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
    s32 minus_one = -1;
    T1_std_memset_s32(
        ags->touch_id_buffer_all_zeros.contents,
        minus_one,
        (u32)touch_buffer_size_bytes);
    
    #if T1_BLOOM_ACTIVE == T1_ACTIVE
    for (
        u32 i = 0;
        i < T1_DOWNSAMPLES_SIZE;
        i++)
    {
        ags->downsampled_rtts[i] = nil;
        
        MTLTextureDescriptor * downsampled_rtt_desc =
            [MTLTextureDescriptor new];
        downsampled_rtt_desc.textureType =
            MTLTextureType2D;
        downsampled_rtt_desc.width =
            (NSUInteger)get_ds_width(
                i,
                (u32)ags->
                    render_viewports[0].width);
        downsampled_rtt_desc.height =
            (NSUInteger)get_ds_height(
                i,
                (u32)ags->
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

-(void)drawSinglePass:(s32)pass_i
    forCamera: (s32)cam_i
    withView: (MTKView *)view
    withComBuf: (id<MTLCommandBuffer>)combuf
{
    T1RenderPass * pass =
        &T1_render_views->cpu[cam_i].passes[pass_i];
    
    switch (pass->type)
    {
        case T1RENDERPASS_DEPTH_PREPASS:
        {
            // TODO: reimplement z prepasses
            #if T1_Z_PREPASS_ACTIVE == T1_ACTIVE
            #elif T1_Z_PREPASS_ACTIVE == T1_INACTIVE
            #else
            #error
            #endif
        }
        T1_log_assert(ags->zbuf_cleared);
        break;
        case T1RENDERPASS_OUTLINES:
        {
            #if T1_OUTLINES_ACTIVE == T1_ACTIVE
            // Drawing outlines to a depth target
            // seems pointless, not supported
            T1_log_assert(
                T1_render_views->cpu[cam_i].
                    write_type !=
                        T1RENDERVIEW_WRITE_DEPTH);
            
            MTLRenderPassDescriptor *
                outlines_desc = [view
                    currentRenderPassDescriptor];
            
            set_defaults_for_render_descriptor(
                outlines_desc,
                cam_i);
            
            outlines_desc.colorAttachments[1].texture =
                nil;
            
            id<MTLRenderCommandEncoder>
                pass_1_outline_enc =
                    [combuf
                        renderCommandEncoderWithDescriptor:
                            outlines_desc];
            
            set_defaults_for_encoder(
                pass_1_outline_enc,
                (u32)cam_i);
            
            [pass_1_outline_enc
                setViewport:
                    ags->render_viewports[cam_i]];
            
            // outlines pipeline
            [pass_1_outline_enc
                setRenderPipelineState:
                    ags->outlines_pls];
            [pass_1_outline_enc
                setDepthStencilState:
                    ags->opaque_depth_stencil_state];
            [pass_1_outline_enc
                setDepthClipMode: MTLDepthClipModeClip];
            [pass_1_outline_enc setCullMode: MTLCullModeFront];
            
            if (
                T1_global->draw_triangles &&
                pass->verts_size > 0)
            {
                [pass_1_outline_enc
                    drawPrimitives:
                        MTLPrimitiveTypeTriangle
                    vertexStart:
                        (NSUInteger)pass->vert_i
                    vertexCount:
                        (NSUInteger)pass->
                            verts_size];
            }
            [pass_1_outline_enc endEncoding];
            #elif T1_OUTLINES_ACTIVE == T1_INACTIVE
            #else
            #error
            #endif
        }
        break;
        case T1RENDERPASS_DIAMOND_ALPHA:
        {
            if (
                !T1_global->draw_triangles)
            {
                break;
            }
            
            // opaque triangles
            MTLRenderPassDescriptor * diamond_desc =
                [view currentRenderPassDescriptor];
            
            set_defaults_for_render_descriptor(
                diamond_desc,
                cam_i);
            
            id<MTLRenderCommandEncoder> pass_2_opaque_tris_enc =
                [combuf renderCommandEncoderWithDescriptor:
                    diamond_desc];
            
            [pass_2_opaque_tris_enc
                setRenderPipelineState:
                    ags->cur_opq_pls];
            
            set_defaults_for_encoder(
                pass_2_opaque_tris_enc,
                (u32)cam_i);
            
            T1_log_assert(
                (pass->verts_size + pass->vert_i) <
                    T1_MAX_VERTS_PER_BUFFER);
            T1_log_assert(pass->verts_size % 3 == 0);
            
            [pass_2_opaque_tris_enc
                drawPrimitives:
                    MTLPrimitiveTypeTriangle
                vertexStart:
                    (NSUInteger)pass->vert_i
                vertexCount:
                    (NSUInteger)pass->verts_size];
            
            [pass_2_opaque_tris_enc endEncoding];
        }
        T1_log_assert(ags->zbuf_cleared);
        break;
        case T1RENDERPASS_ALPHA_BLEND:
        {
            #if T1_BLENDING_SHADER_ACTIVE == T1_ACTIVE
            if (!T1_global->draw_triangles)
            {
                break;
            }
            
            MTLRenderPassDescriptor * alpha_desc =
                [view currentRenderPassDescriptor];
            
            set_defaults_for_render_descriptor(
                alpha_desc,
                cam_i);
            
            id<MTLRenderCommandEncoder>
                alpha_pass = [combuf
                    renderCommandEncoderWithDescriptor:
                        alpha_desc];
            
            [alpha_pass
                setRenderPipelineState:
                    ags->cur_blnd_pls];
            set_defaults_for_encoder(
                alpha_pass,
                (u32)cam_i);
            
            [alpha_pass
                setCullMode: MTLCullModeBack];
            [alpha_pass
                setFrontFacingWinding:
                    MTLWindingCounterClockwise];
            
            [alpha_pass
                drawPrimitives:
                    MTLPrimitiveTypeTriangle
                vertexStart:
                    (NSUInteger)pass->vert_i
                vertexCount:
                    (NSUInteger)pass->verts_size];
            
            [alpha_pass
                endEncoding];
            #elif T1_BLENDING_SHADER_ACTIVE == T1_INACTIVE
            #else
            #error
            #endif
        }
        T1_log_assert(ags->zbuf_cleared);
        break;
        case T1RENDERPASS_BILLBOARDS:
        {
            if (
                pass->verts_size < 1 ||
                ags->cur_bb_pls == nil)
            {
                break;
            }
            
            MTLRenderPassDescriptor * bb_desc =
                [view currentRenderPassDescriptor];
            
            set_defaults_for_render_descriptor(
                bb_desc,
                cam_i);
            
            id<MTLRenderCommandEncoder>
                bb_enc = [combuf
                renderCommandEncoderWithDescriptor:
                        bb_desc];
            
            [bb_enc
                setRenderPipelineState:
                    ags->cur_bb_pls];
            [bb_enc
                setDepthStencilState:
                    ags->opaque_depth_stencil_state];
            
            [bb_enc
                setVertexBuffer:
                    ags->flat_quad_buffers[ags->frame_i]
                offset: 0
                atIndex: 2];
            
            [bb_enc
                setVertexBuffer:
                    ags->cam_buffers[ags->frame_i]
                offset: 0
                atIndex: 3];
            
            [bb_enc
                drawPrimitives:
                    MTLPrimitiveTypeTriangle
                vertexStart:
                    0
                vertexCount:
                    (NSUInteger)
                        pass->verts_size * 6
                ];
            
            [bb_enc endEncoding];
        }
        T1_log_assert(ags->zbuf_cleared);
        break;
        case T1RENDERPASS_BLOOM:
        {
            T1_log_assert(
                T1_render_views->cpu[cam_i].
                    write_type ==
                        T1RENDERVIEW_WRITE_RENDER_TARGET);
            T1_log_assert(ags->cur_blnd_pls != nil);
            
            if (!T1_global->draw_triangles ||
                pass->verts_size < 1)
            {
                break;
            }
            
            #if T1_BLOOM_ACTIVE == T1_ACTIVE
            // only render target can bloom atm
            T1_log_assert(cam_i == 0);
            
            MTLRenderPassDescriptor *
                bloom_desc = [view
                    currentRenderPassDescriptor];
            set_defaults_for_render_descriptor(
                bloom_desc,
                cam_i);
            
            id<MTLTexture> bloom_rtt =
                ags->downsampled_rtts[0];
            bloom_desc.colorAttachments[0].
                texture = bloom_rtt;
            bloom_desc.colorAttachments[0].
                clearColor = MTLClearColorMake(
                    0.0, 0.0, 0.0, 0.0);
            bloom_desc.colorAttachments[0].
                loadAction = MTLLoadActionClear;
            
            id<MTLRenderCommandEncoder>
                bloom_enc = [combuf
                    renderCommandEncoderWithDescriptor:
                    bloom_desc];
            
            [bloom_enc
                setRenderPipelineState:
                    ags->cur_bloom_pls];
            
            set_defaults_for_encoder(
                bloom_enc,
                (u32)cam_i);
            
            [bloom_enc
                setCullMode: MTLCullModeBack];
            [bloom_enc
                setFrontFacingWinding:
                    MTLWindingCounterClockwise];
            
            [bloom_enc
                drawPrimitives:
                    MTLPrimitiveTypeTriangle
                vertexStart:
                    (NSUInteger)pass->vert_i
                vertexCount:
                    (NSUInteger)pass->verts_size];
            
            [bloom_enc endEncoding];
            
            for (
                u32 ds_i = 1;
                ds_i < T1_DOWNSAMPLES_SIZE;
                ds_i++)
            {
                MTLViewport smaller_viewport =
                    ags->render_viewports[0];
                
                smaller_viewport.width = ags->
                    downsampled_rtts[ds_i].width;
                smaller_viewport.height = ags->
                    downsampled_rtts[ds_i].height;
                
                MTLSize grid = MTLSizeMake(
                    (u32)smaller_viewport.width,
                    (u32)smaller_viewport.height,
                    1);
                
                MTLSize threadgroup = MTLSizeMake(16, 16, 1);
                
                if (ds_i < T1_DOWNSAMPLES_CUTOFF) {
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
            #elif T1_BLOOM_ACTIVE == T1_INACTIVE
            #else
            #error
            #endif
        }
        break;
        case T1RENDERPASS_FLAT_TEXQUADS:
        {
            if (
                pass->verts_size < 1 ||
                ags->cur_flat_texquad_pls == nil)
            {
                break;
            }
            
            MTLRenderPassDescriptor * flat_texq_desc =
                [view currentRenderPassDescriptor];
            
            set_defaults_for_render_descriptor(
                flat_texq_desc,
                cam_i);
            
            id<MTLRenderCommandEncoder>
                flat_texq_enc = [combuf
                renderCommandEncoderWithDescriptor:
                        flat_texq_desc];
            
            [flat_texq_enc
                setRenderPipelineState:
                    ags->cur_flat_texquad_pls];
            [flat_texq_enc
                setDepthStencilState:
                    ags->opaque_depth_stencil_state];
            
            [flat_texq_enc
                setVertexBuffer:
                    ags->flat_texquad_buffers[ags->frame_i]
                offset: 0
                atIndex: 0];
            
            [flat_texq_enc
                setVertexBuffer:
                    ags->matrix_buffers[ags->frame_i]
                offset: 0
                atIndex: 2];
            
            [flat_texq_enc
                setVertexBuffer:
                    ags->cam_buffers[ags->frame_i]
                offset: 0
                atIndex: 3];
            
            #if T1_TEXTURES_ACTIVE == T1_ACTIVE
            for (
                u32 i = 0;
                i < T1_TEXARRAYS_CAP;
                i++)
            {
                if (ags->metal_textures[i] != NULL) {
                    [flat_texq_enc
                        setFragmentTexture: ags->metal_textures[i]
                        atIndex: i];
                }
            }
            #elif T1_TEXTURES_ACTIVE == T1_INACTIVE
            [flat_texq_enc
                setFragmentTexture: ags->metal_textures[0]
                atIndex: 0];
            #else
            #error
            #endif
            
            [flat_texq_enc
                drawPrimitives:
                    MTLPrimitiveTypeTriangle
                vertexStart:
                    0
                vertexCount:
                    (NSUInteger)pass->verts_size * 6];
            
            [flat_texq_enc endEncoding];
        }
        T1_log_assert(ags->zbuf_cleared);
        break;
        default:
            // render pass type not set
            T1_log_assert(0);
    }
    
    pass->verts_size = 0;
}

- (void)drawInMTKView:(MTKView *)view
{
    if (
        funcptr_gameloop_before_render == NULL ||
        T1_render_views->size < 1 ||
        T1_render_views->cpu[0].deleted)
    {
        return;
    }
    
    T1_log_assert(
        T1_render_views->cpu[0].write_tex != T1_TEX_NONE);
    
    #if T1_DRAWING_SEMAPHORE_ACTIVE == T1_ACTIVE
    dispatch_semaphore_wait(ags->drawing_semaphore, DISPATCH_TIME_FOREVER);
    #elif T1_DRAWING_SEMAPHORE_ACTIVE == T1_INACTIVE
    #else
    #error
    #endif
    
    T1GPUFrame * f = &T1_cpu_to_gpu_data->
        triple_buffers[ags->frame_i];
    
    T1_log_assert(f->verts_size % 3 == 0);
    
    funcptr_gameloop_before_render(
        T1_cpu_to_gpu_data->triple_buffers + 
            ags->frame_i);
    
    if (
        (f->postproc_consts->timestamp -
            T1_global->last_resize_request_us)
                < T1_GLOBAL_WINDOW_RESIZE_TIMEOUT)
    {
        return;
    }
    
    if (!ags || !ags->metal_active || !ags->viewports_set[0])
    {
        return;
    }
    
    id<MTLCommandBuffer> combuf = [ags->command_queue commandBuffer];
    
    if (combuf == nil) {
        #if T1_LOGGER_ASSERTS_ACTIVE
        log_dump_and_crash("error - can't get metal command buffer\n");
        #endif
        
        return;
    }
    
    // Blit to clear the touch id buffer
    u32 size_bytes = (u32)(
        [ags->touch_id_texture width] *
        [ags->touch_id_texture height] *
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
        s32 cam_i =
            (s32)f->render_views_size - 1;
        cam_i >= 0;
        cam_i--)
    {
        if (
            T1_render_views->cpu[cam_i].deleted ||
            T1_render_views->cpu[cam_i].passes_size == 0)
        {
            continue;
        }
        
        ags->cur_rtt = nil;
        ags->cur_depth = nil;
        ags->cur_opq_pls = nil;
        ags->cur_blnd_pls = nil;
        ags->cur_bloom_pls = nil;
        ags->cur_bb_pls = nil;
        ags->cur_flat_texquad_pls = nil;
        
        ags->rtt_cleared = false;
        ags->zbuf_cleared = false;
        
        switch (T1_render_views->cpu[cam_i].write_type)
        {
            case T1RENDERVIEW_WRITE_BELOWBOUNDS:
            {
                T1_log_assert(0);
            }
            break;
            case T1RENDERVIEW_WRITE_RENDER_TARGET:
            {
                ags->cur_rtt = get_tex_slice(
                    T1_tex_to_array_i(
                        T1_render_views->cpu[cam_i].write_tex),
                    T1_tex_to_slice_i(
                        T1_render_views->cpu[cam_i].write_tex));
                if (ags->cur_rtt == nil) {
                    continue;
                }
                
                ags->cur_depth = ags->depth_textures[0];
                T1_log_assert(ags->cur_depth != nil);
                ags->cur_opq_pls = ags->diamond_touch_pls;
                ags->cur_blnd_pls = ags->blend_touch_pls;
                ags->cur_bloom_pls = ags->blend_touch_pls;
                ags->cur_bb_pls = ags->bb_touch_pls;
                ags->cur_flat_texquad_pls = ags->flat_texquad_touch_pls;
            }
            break;
            case T1RENDERVIEW_WRITE_RGBA:
            {
                ags->cur_rtt =
                    get_tex_slice(
                        T1_tex_to_array_i(T1_render_views->cpu[cam_i].write_tex),
                        T1_tex_to_slice_i(T1_render_views->cpu[cam_i].write_tex));
                T1_log_assert(ags->cur_rtt != NULL);
                ags->cur_depth = ags->depth_textures[cam_i];
                ags->cur_opq_pls = ags->diamond_notouch_pls;
                ags->cur_blnd_pls = ags->blend_notouch_pls;
                ags->cur_bloom_pls = ags->blend_notouch_pls;
                ags->cur_bb_pls = ags->bb_notouch_pls;
            }
            break;
            case T1RENDERVIEW_WRITE_DEPTH:
            {
                s16 array_i = T1_tex_to_array_i(
                    T1_render_views->cpu[cam_i].write_tex);
                s16 slice_i = T1_tex_to_slice_i(
                    T1_render_views->cpu[cam_i].write_tex);
                T1_log_assert(array_i == T1_DEPTH_TEXTUREARRAYS_I);
                T1_log_assert(slice_i >= 0);
                T1_log_assert(slice_i < T1_RENDER_VIEW_CAP);
                ags->cur_rtt = nil;
                ags->cur_depth = ags->depth_textures[slice_i];
                
                T1_log_assert(ags->cur_depth != nil);
                T1_log_assert(
                    (ags->cur_depth.usage &
                        MTLTextureUsageRenderTarget) > 0);
                ags->cur_opq_pls =
                    ags->depth_only_pls;
                ags->cur_blnd_pls =
                    ags->depth_only_pls;
                ags->cur_bloom_pls =
                    ags->depth_only_pls;
                ags->cur_bb_pls = nil;
            }
            break;
            case T1RENDERVIEW_WRITE_ABOVEBOUNDS:
            {
                T1_log_assert(0);
            }
            break;
        }
        
        for (
            s32 pass_i = 0;
            pass_i < T1_render_views->
                cpu[cam_i].passes_size;
            pass_i++)
        {
            [self
                drawSinglePass: pass_i
                forCamera: cam_i
                withView: view
                withComBuf: combuf];
        }
        
        T1_log_assert(ags->viewports_set[cam_i]);
        
        T1_log_assert(T1_tex_to_array_i(T1_render_views->cpu[cam_i].
            write_tex) >= 1);
        T1_log_assert(T1_tex_to_slice_i(T1_render_views->cpu[cam_i].
            write_tex) >= 0);
    }
    
    // copy the touch id buffer for CPU use
    id <MTLBlitCommandEncoder>
        blit_touch_tex_to_cpu_enc =
            [combuf blitCommandEncoder];
    [blit_touch_tex_to_cpu_enc
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
    [blit_touch_tex_to_cpu_enc endEncoding];
        
    // Render pass 4 puts a quad on the full screen
    MTLRenderPassDescriptor *
        pass_5_comp_desc =
            [view currentRenderPassDescriptor];
    pass_5_comp_desc.colorAttachments[0].
        clearColor =
            MTLClearColorMake(0.0f, 0.0f, 0.1f, 1.0f);
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
    
    // The main camera must have a target to write to
    T1_log_assert(
        T1_tex_to_array_i(T1_render_views->cpu[0].write_tex) >= 0);
    T1_log_assert(
        T1_tex_to_array_i(T1_render_views->cpu[0].write_tex) <
            (s32)T1_tex_arrays_size);
    
    id<MTLTexture> arr_tex = ags->metal_textures[
        T1_tex_to_array_i(T1_render_views->cpu[0].write_tex)];
    id<MTLTexture> sliced_tex = [arr_tex
        newTextureViewWithPixelFormat:
            arr_tex.pixelFormat
        textureType:
            MTLTextureType2D
        levels:
            NSMakeRange(0, arr_tex.mipmapLevelCount)
        slices:
            NSMakeRange(
                (NSUInteger)T1_tex_to_slice_i(
                    T1_render_views->cpu[0].write_tex),
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
    
    s32 perlin_ta_i =
        f->postproc_consts->perlin_texturearray_i;
    #if T1_LOG_ASSERTS_ACTIVE == T1_ACTIVE
    s32 perlin_t_i =
        f->postproc_consts->perlin_texture_i;
    // log_assert(perlin_ta_i >= 1);
    T1_log_assert(perlin_t_i == 0);
    
    [pass_5_comp
        setFragmentTexture: ags->metal_textures[perlin_ta_i]
        atIndex:6];
    #elif T1_LOG_ASSERTS_ACTIVE == T1_INACTIVE
    // Pass
    #else
    #error
    #endif
    
    [pass_5_comp
        setFragmentTexture: ags->depth_textures[0]
        atIndex: T1_CAM_DEPTH_FRAGARG_I];
    [pass_5_comp
        drawPrimitives:MTLPrimitiveTypeTriangle
        vertexStart:0
        vertexCount:6];
    [pass_5_comp endEncoding];
    [combuf presentDrawable: [view currentDrawable]];
    
    ags->frame_i += 1;
    ags->frame_i %= T1_FRAMES_CAP;
    T1_log_assert(ags->frame_i < T1_FRAMES_CAP);
    
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

void T1_os_gpu_update_internal_render_viewport(
    const s32 at_i)
{
    T1_log_assert(at_i >= 0);
    T1_log_assert(at_i < T1_RENDER_VIEW_CAP);
    
    ags->viewports_set[at_i] = false;
    [apple_gpu_delegate updateRenderViewSize:at_i];
}

void T1_os_gpu_update_window_viewport(void)
{
    [apple_gpu_delegate updateFinalWindowSize];
}

s16 T1_apple_gpu_make_depth_tex(
    const u32 width,
    const u32 height)
{
    s16 slice_i = 0;
    while (ags->depth_textures[slice_i] != nil) {
        slice_i += 1;
        T1_log_assert(slice_i < T1_RENDER_VIEW_CAP);
    }
    
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
    
    T1_log_assert(ags->depth_textures[slice_i] != nil);
    T1_log_assert(
        (ags->depth_textures[slice_i].usage &
            MTLTextureUsageRenderTarget) > 0);
    
    return slice_i;
}
