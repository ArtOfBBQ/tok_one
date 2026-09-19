#include "T1_gpu.h"

#include "T1_global.h"
#include "T1_mem.h"
#include "T1_log.h"
#include "T1_settings.h"
#include "T1_material.h"
#include "T1_mesh_summary.h"
#include "T1_tex_array.h"
#include "T1_render_view.h"
#include "T1_platform_layer.h"

typedef struct {
    MTLPixelFormat pixel_format_renderpass1;
    NSUInteger frame_i;
    MTLViewport window_viewport;
    MTLViewport render_viewports[T1_RENDER_VIEW_CAP];
    
    void * device;
    void * lib;
    void * command_queue;
    
    void * polygon_buffers[T1_FRAMES_CAP];
    void * matrix_buffers[T1_FRAMES_CAP];
    void * light_buffers [T1_FRAMES_CAP];
    void * vertex_buffers[T1_FRAMES_CAP];
    void * flat_quad_buffers[T1_FRAMES_CAP];
    void * flat_texquad_buffers[T1_FRAMES_CAP];
    void * cam_buffers[T1_FRAMES_CAP];
    void * postprocessing_constants_buffers[T1_FRAMES_CAP];
    void * locked_vertex_populator_buffer;
    void * locked_vertex_buffer;
    void * locked_matf32_populator_buffer;
    void * locked_mats32_populator_buffer;
    void * locked_matf32_buffer;
    void * locked_mats32_buffer;
    void * projection_constants_buffer;
    
    void * depth_textures[T1_RENDER_VIEW_CAP];
    
    #if T1_Z_PREPASS_ACTIVE == T1_ACTIVE
    void * z_prepass_pls;
    #elif T1_Z_PREPASS_ACTIVE == T1_INACTIVE
    #else
    #error
    #endif
    
    #if T1_OUTLINES_ACTIVE == T1_ACTIVE
    void * outlines_pls;
    #elif T1_OUTLINES_ACTIVE == T1_INACTIVE
    #else
    #error
    #endif
    void * diamond_touch_pls;
    void * blend_touch_pls;
    void * bb_touch_pls;
    void * diamond_notouch_pls;
    void * depth_only_pls;
    void * blend_notouch_pls;
    void * bb_notouch_pls;
    void * flat_texquad_touch_pls;
    
    #if T1_BLOOM_ACTIVE == T1_ACTIVE
    void * downsample_compute_pls;
    void * boxblur_compute_pls;
    #elif T1_BLOOM_ACTIVE == T1_INACTIVE
    #else
    #error
    #endif
    
    void * singlequad_pls;
    void * opaque_depth_stencil_state;
    
    #if T1_TEXTURES_ACTIVE == T1_ACTIVE
    void * metal_textures[T1_TEXARRAYS_CAP];
    #elif T1_TEXTURES_ACTIVE == T1_INACTIVE
    void * metal_textures[1]; // for font only
    #else
    #error
    #endif
    
    #if T1_BLOOM_ACTIVE == T1_ACTIVE
    void * downsampled_rtts[T1_DOWNSAMPLES_SIZE];
    #elif T1_BLOOM_ACTIVE == T1_INACTIVE
    #else
    #error
    #endif
    
    void * touch_id_texture;
    void * touch_id_buffer;
    void * touch_id_buffer_all_zeros;
    
    void * cur_rtt;
    void * cur_depth;
    void * cur_opq_pls;
    void * cur_blnd_pls;
    void * cur_bloom_pls;
    void * cur_bb_pls;
    void * cur_flat_texquad_pls;
    
    // objective-c classes
    void * class_mtl_texture_desc; // MTLTextureDescriptor
    // objective-c selectors
    void * sel_new;
    void * sel_command_buffer; // commandBuffer
    void * sel_new_function_with_name; // newFunctionWithName
    void * sel_width;
    void * sel_height;
    void * sel_new_compute_pls_with_func; // newComputePipelineStateWithFunction
    void * sel_new_tex_with_desc; // newTextureWithDescriptor
    void * sel_new_buf_with_len_options; // newBufferWithLength:options:
    void * sel_new_render_pls_with_descriptor_error; // newRenderPipelineStateWithDescriptor:error:
    void * sel_contents;
    void * sel_allocated_size;
    void * sel_pixel_format; // pixelFormat
    void * sel_array_length; // arrayLength
    void * sel_texture_type;
    void * sel_set_texture_type; // setTextureType:
    void * sel_set_array_length; // setArrayLength;
    void * sel_set_pixel_format; // setPixelFormat:
    void * sel_set_storage_mode; // setStorageMode:
    void * sel_set_usage; // setUsage:
    void * sel_set_width; // setWidth:
    void * sel_set_height; // setHeight:
    void * sel_set_mipmap_level_count; // setMipmapLevelCount:
    void * sel_usage;
    void * sel_mipmap_level_count; // mipmapLevelCount
    void * sel_new_depth_stencil_state_with_desc;
    void * sel_blit_command_encoder; // blitCommandEncoder
    void * sel_commit; // commit
    void * sel_wait_until_completed; // waitUntilCompleted
    void * sel_end_encoding; // endEncoding
    void * sel_copy_from_texture_to_texture; // copyFromTexture:toTexture:
    void * sel_copy_from_texture_to_buffer; // copyFromTexture:sourceSlice:sourceLevel:sourceOrigin:sourceSize:toBuffer:destinationOffset:destinationBytesPerRow:destinationBytesPerImage:
    void * sel_new_buffer_with_bytes_no_copy; // newBufferWithBytesNoCopy:length:options:deallocator:
    T1PostProcessingVertex quad_vertices[6];
    f32 retina_scaling_factor;
    u8  viewports_set[T1_RENDER_VIEW_CAP];
    b8  metal_active;
    b8  zbuf_cleared;
    b8  rtt_cleared;
    b8  objc_metal_framework_good;
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
    f32 backing_scale_factor,
    c8 * error_msg_string)
{
    if (T1_cpu_to_gpu_data == NULL) {
        T1_std_strcpy_cap(error_msg_string, 128, "GPU frame buffer was not initialized");
        return false;
    }
    
    ags = T1_mem_malloc_unmanaged(sizeof(AppleGPUState));
    ags->retina_scaling_factor = backing_scale_factor;
    ags->pixel_format_renderpass1 = 0;
    
    T1_objc_open_framework_and_link_perma_good_val(
        "/System/Library/Frameworks/MetalKit.framework/MetalKit",
        &ags->objc_metal_framework_good);
    ags->class_mtl_texture_desc = T1_objc_get_class("MTLTextureDescriptor");
    ags->sel_new = T1_objc_reg_sel("new");
    ags->sel_command_buffer = T1_objc_reg_sel("commandBuffer");
    ags->sel_new_function_with_name = T1_objc_reg_sel("newFunctionWithName:"); 
    ags->sel_width  = T1_objc_reg_sel("width");
    ags->sel_height = T1_objc_reg_sel("height");
    ags->sel_new_compute_pls_with_func =
        T1_objc_reg_sel("newComputePipelineStateWithFunction:error:");
    T1_log_assert(ags->sel_new_compute_pls_with_func != NULL);
    ags->sel_new_tex_with_desc =
        T1_objc_reg_sel("newTextureWithDescriptor:");
    T1_log_assert(ags->sel_new_tex_with_desc != NULL);
    ags->sel_new_buf_with_len_options =
        T1_objc_reg_sel("newBufferWithLength:options:");
    ags->sel_new_render_pls_with_descriptor_error =
        T1_objc_reg_sel("newRenderPipelineStateWithDescriptor:error:");
    ags->sel_contents =
        T1_objc_reg_sel("contents");
    ags->sel_allocated_size =
        T1_objc_reg_sel("allocatedSize");
    ags->sel_pixel_format =
        T1_objc_reg_sel("pixelFormat");
    ags->sel_array_length =
        T1_objc_reg_sel("arrayLength");
    ags->sel_texture_type =
        T1_objc_reg_sel("textureType");
    ags->sel_set_texture_type =
        T1_objc_reg_sel("setTextureType:");
    ags->sel_set_array_length =
        T1_objc_reg_sel("setArrayLength:");
    ags->sel_set_pixel_format =
        T1_objc_reg_sel("setPixelFormat:");
    ags->sel_set_storage_mode =
        T1_objc_reg_sel("setStorageMode:");
    ags->sel_set_usage =
        T1_objc_reg_sel("setUsage:");
    ags->sel_set_width =
        T1_objc_reg_sel("setWidth:"); // setWidth:
    ags->sel_set_height =
        T1_objc_reg_sel("setHeight:"); // setWidth:
    ags->sel_set_mipmap_level_count =
        T1_objc_reg_sel("setMipmapLevelCount:");
    ags->sel_usage =
        T1_objc_reg_sel("usage");
    T1_log_assert(ags->sel_usage != NULL);
    ags->sel_mipmap_level_count =
        T1_objc_reg_sel("mipmapLevelCount");
    ags->sel_new_depth_stencil_state_with_desc =
        T1_objc_reg_sel("newDepthStencilStateWithDescriptor:");
    T1_log_assert(ags->sel_new_depth_stencil_state_with_desc != NULL);
    ags->sel_blit_command_encoder =
        T1_objc_reg_sel("blitCommandEncoder");
    ags->sel_commit =
        T1_objc_reg_sel("commit");
    ags->sel_wait_until_completed =
        T1_objc_reg_sel("waitUntilCompleted");
    ags->sel_end_encoding =
        T1_objc_reg_sel("endEncoding");
    ags->sel_copy_from_texture_to_texture =
        T1_objc_reg_sel("copyFromTexture:toTexture:");
    ags->sel_copy_from_texture_to_buffer =
        T1_objc_reg_sel("copyFromTexture:sourceSlice:sourceLevel:sourceOrigin:sourceSize:toBuffer:destinationOffset:destinationBytesPerRow:destinationBytesPerImage:");
    ags->sel_new_buffer_with_bytes_no_copy =
        T1_objc_reg_sel("newBufferWithBytesNoCopy:length:options:deallocator:");
    funcptr_gameloop_before_render =
        arg_funcptr_shared_gameloop_update;
    funcptr_gameloop_after_render =
        arg_funcptr_shared_gameloop_update_after_render_pass;
    
    T1_objc_close_current_framework();
    
    ags->pixel_format_renderpass1 = MTLPixelFormatRGBA8Unorm;
    
    ags->frame_i = 0;
    
    T1_std_strcpy_cap(
        error_msg_string,
        512,
        "");
    
    ags->device = (__bridge void *)(with_metal_device);
    
    NSError * Error = NULL;
    ags->lib = (__bridge_retained void *)([with_metal_device newDefaultLibrary]);
    
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
            
            const c8 * errorcstr = [errorstr
                cStringUsingEncoding: NSASCIIStringEncoding];
            
            T1_std_strcpy_cap(
                error_msg_string,
                512,
                errorcstr);
            return false;
        }
        
        ags->lib =
            (__bridge_retained void *)([with_metal_device
                newLibraryWithURL: shader_lib_url
                error: &Error]);
        
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
    
    void * nsstring_vert_shader = T1_objc_nsstring_construct(
        "vertex_shader");
    void * vertex_shader = T1_objc_msg_with_1arg_expect_ptr(
        ags->lib,
        ags->sel_new_function_with_name,
        (uintptr_t)nsstring_vert_shader);
    if (vertex_shader == NULL) {
        T1_std_strcpy_cap(
            error_msg_string,
            512,
            "Missing function: vertex_shader()");
        return false;
    }
    
    void * nsstring_fragment_shader = T1_objc_nsstring_construct(
        "frag_shader");
    void * fragment_shader = T1_objc_msg_with_1arg_expect_ptr(
        ags->lib,
        ags->sel_new_function_with_name,
        (uintptr_t)nsstring_fragment_shader);
    if (fragment_shader == NULL) {
        T1_std_strcpy_cap(
            error_msg_string,
            512,
            "Missing function: alphablending_frag_shader()");
        return false;
    }
    
    #if T1_BLENDING_SHADER_ACTIVE == T1_ACTIVE
    void * nsstring_alphablending_fragment_shader = T1_objc_nsstring_construct(
        "alphablending_frag_shader");
    void * alphablending_fragment_shader = T1_objc_msg_with_1arg_expect_ptr(
        ags->lib,
        ags->sel_new_function_with_name,
        (uintptr_t)nsstring_alphablending_fragment_shader);
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
    void * nsstring_vertex_shader = T1_objc_nsstring_construct(
        "vertex_shader");
    void * z_prepass_vertex_shader = T1_objc_msg_with_1arg_expect_ptr(
        ags->lib,
        ags->sel_new_function_with_name,
        (uintptr_t)nsstring_vertex_shader);
    if (z_prepass_vertex_shader == NULL) {
        T1_std_strcpy_cap(
            error_msg_string,
            512,
            "Missing function: vertex_shader()");
        return false;
    }
    
    void * nsstring_z_prepass_frag_shader = T1_objc_nsstring_construct(
        "z_prepass_frag_shader");
    void * z_prepass_fragment_shader =
        T1_objc_msg_with_1arg_expect_ptr(
            ags->lib,
            ags->sel_new_function_with_name,
            (uintptr_t)nsstring_z_prepass_frag_shader);
    if (z_prepass_fragment_shader == NULL)
    {
        T1_log_append("Missing function: z_prepass_frag_shader()!");
        
        T1_std_strcpy_cap(
            error_msg_string,
            512,
            "Missing function: z_prepass_fragment_shader()");
        return false;
    }
    
    MTLRenderPipelineDescriptor * z_prepass_pls_desc =
        [MTLRenderPipelineDescriptor new];
    [z_prepass_pls_desc
        setVertexFunction:
            (__bridge id<MTLFunction> _Nullable)(z_prepass_vertex_shader)];
    [z_prepass_pls_desc
        setFragmentFunction:
            (__bridge id<MTLFunction> _Nullable)(z_prepass_fragment_shader)];
    z_prepass_pls_desc.depthAttachmentPixelFormat =
        MTLPixelFormatDepth32Float;
    z_prepass_pls_desc.label = @"z prepass pipeline state";
    z_prepass_pls_desc
        .colorAttachments[0]
        .pixelFormat = ags->pixel_format_renderpass1;
    
    ags->z_prepass_pls = T1_objc_msg_with_2arg_expect_ptr(
        ags->device,
        ags->sel_new_render_pls_with_descriptor_error,
        (uintptr_t)(__bridge void *)(z_prepass_pls_desc),
        (uintptr_t)NULL);
    #elif T1_Z_PREPASS_ACTIVE == T1_INACTIVE
    #else
    #error
    #endif
    
    #if T1_OUTLINES_ACTIVE == T1_ACTIVE
    void * nsstring_outlines_vertex_shader =
        T1_objc_nsstring_construct("outlines_vertex_shader");
    void * outlines_vertex_shader = T1_objc_msg_with_1arg_expect_ptr(
        ags->lib,
        ags->sel_new_function_with_name,
        (uintptr_t)nsstring_outlines_vertex_shader);
    if (outlines_vertex_shader == NULL) {
        T1_std_strcpy_cap(
            error_msg_string,
            512,
            "Missing function: outlines_vertex_shader()");
        return false;
    }
    
    void * nsstring_outlines_fragment_shader =
        T1_objc_nsstring_construct(
            "outlines_frag_shader");
    void * outlines_fragment_shader = T1_objc_msg_with_1arg_expect_ptr(
        ags->lib,
        ags->sel_new_function_with_name,
        (uintptr_t)nsstring_outlines_fragment_shader);
    if (outlines_fragment_shader == NULL) {
        T1_std_strcpy_cap(
            error_msg_string,
            512,
            "Missing function: outlines_frag_shader()");
        return false;
    }
    
    MTLRenderPipelineDescriptor * outlines_pls_desc =
        [MTLRenderPipelineDescriptor new];
    [outlines_pls_desc
        setVertexFunction:
            (__bridge id<MTLFunction> _Nullable)(outlines_vertex_shader)];
    [outlines_pls_desc
        setFragmentFunction:
            (__bridge id<MTLFunction> _Nullable)(outlines_fragment_shader)];
    outlines_pls_desc.label =
        @"outlines pipeline state";
    outlines_pls_desc
        .colorAttachments[0]
        .pixelFormat = ags->pixel_format_renderpass1;
    outlines_pls_desc.
        depthAttachmentPixelFormat =
            MTLPixelFormatDepth32Float;
    ags->outlines_pls =
        T1_objc_msg_with_2arg_expect_ptr(
            ags->device,
            ags->sel_new_render_pls_with_descriptor_error,
            (uintptr_t)(__bridge void *)(outlines_pls_desc),
            (uintptr_t)&Error);
    
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
    
    void * nsstring_flat_billboard_quad_vert_shader =
        T1_objc_nsstring_construct(
            "flat_billboard_quad_vertex_shader");
    void * flat_billboard_quad_vert_shader = T1_objc_msg_with_1arg_expect_ptr(
        ags->lib,
        ags->sel_new_function_with_name,
        (uintptr_t)nsstring_flat_billboard_quad_vert_shader);
    if (flat_billboard_quad_vert_shader == NULL)
    {
        T1_std_strcpy_cap(
            error_msg_string,
            512,
            "Missing function: "
            "flat_billboard_quad_vertex_shader()");
        return false;
    }
    
    void * nsstring_flat_billboard_quad_frag_shader =
        T1_objc_nsstring_construct(
            "flat_billboard_quad_frag_shader");
    void * flat_billboard_quad_frag_shader = T1_objc_msg_with_1arg_expect_ptr(
        ags->lib,
        ags->sel_new_function_with_name,
        (uintptr_t)nsstring_flat_billboard_quad_frag_shader);
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
        setVertexFunction:
            (__bridge id<MTLFunction> _Nullable)(flat_billboard_quad_vert_shader)];
    [flat_billboard_quad_pls_desc
        setFragmentFunction:
            (__bridge id<MTLFunction> _Nullable)(flat_billboard_quad_frag_shader)];
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
        T1_objc_msg_with_2arg_expect_ptr(
            ags->device,
            ags->sel_new_render_pls_with_descriptor_error,
            (uintptr_t)(__bridge void *)(flat_billboard_quad_pls_desc),
            (uintptr_t)&Error);
    
    T1_log_assert(Error == nil);
    flat_billboard_quad_pls_desc.
        colorAttachments[1] = nil;
    ags->bb_notouch_pls =
        (__bridge_retained void *)([with_metal_device
            newRenderPipelineStateWithDescriptor:
                flat_billboard_quad_pls_desc
            error:
                &Error]);
    
    void * nsstring_flat_texquad_vert_shader =
        T1_objc_nsstring_construct(
            "flat_texquad_vertex_shader");
    void * flat_texquad_vert_shader = T1_objc_msg_with_1arg_expect_ptr(
        ags->lib,
        ags->sel_new_function_with_name,
        (uintptr_t)nsstring_flat_texquad_vert_shader);
    if (flat_texquad_vert_shader == NULL)
    {
        T1_std_strcpy_cap(
            error_msg_string,
            512,
            "Missing function: "
            "flat_texquad_vertex_shader()");
        return false;
    }
    
    void * nsstring_flat_texquad_frag_shader =
        T1_objc_nsstring_construct(
            "flat_texquad_frag_shader");
    void * flat_texquad_frag_shader = T1_objc_msg_with_1arg_expect_ptr(
        ags->lib,
        ags->sel_new_function_with_name,
        (uintptr_t)nsstring_flat_texquad_frag_shader);
    if (flat_texquad_frag_shader == NULL) {
        T1_std_strcpy_cap(
            error_msg_string,
            512,
            "Missing function: "
            "flat_texquad_frag_shader()");
        return false;
    }
    
    MTLRenderPipelineDescriptor *
        flat_texquad_pls_desc =
            [MTLRenderPipelineDescriptor new];
    [flat_texquad_pls_desc
        setVertexFunction:
            (__bridge id<MTLFunction> _Nullable)(flat_texquad_vert_shader)];
    [flat_texquad_pls_desc
        setFragmentFunction:
            (__bridge id<MTLFunction> _Nullable)(flat_texquad_frag_shader)];
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
        T1_objc_msg_with_2arg_expect_ptr(
            ags->device,
            ags->sel_new_render_pls_with_descriptor_error,
            (uintptr_t)(__bridge void *)(flat_texquad_pls_desc),
            (uintptr_t)&Error);
    
    T1_log_assert(Error == nil);
    
    
    // Setup pipeline that uses diamonds instead of alphablending
    MTLRenderPipelineDescriptor * diamond_pls_desc =
        [MTLRenderPipelineDescriptor new];
    [diamond_pls_desc
        setVertexFunction:
            (__bridge id<MTLFunction> _Nullable)(vertex_shader)];
    [diamond_pls_desc
        setFragmentFunction:
            (__bridge id<MTLFunction> _Nullable)(fragment_shader)];
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
        (__bridge_retained void *)([with_metal_device
            newRenderPipelineStateWithDescriptor:
                diamond_pls_desc 
            error:
                &Error]);
    
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
        T1_objc_msg_with_2arg_expect_ptr(
            ags->device,
            ags->sel_new_render_pls_with_descriptor_error,
            (uintptr_t)(__bridge void *)(diamond_pls_desc),
            (uintptr_t)&Error);
    T1_log_assert(Error == NULL);
    
    diamond_pls_desc.colorAttachments[0] = nil;
    diamond_pls_desc.fragmentFunction = nil;
    
    ags->depth_only_pls =
        T1_objc_msg_with_2arg_expect_ptr(
            ags->device,
            ags->sel_new_render_pls_with_descriptor_error,
            (uintptr_t)(__bridge void *)(diamond_pls_desc),
            (uintptr_t)&Error);
    
    #if T1_BLENDING_SHADER_ACTIVE == T1_ACTIVE
    MTLRenderPipelineDescriptor * alpha_pls_desc =
        [MTLRenderPipelineDescriptor new];
    [alpha_pls_desc
        setVertexFunction:
            (__bridge id<MTLFunction> _Nullable)(vertex_shader)];
    [alpha_pls_desc
        setFragmentFunction:
            (__bridge id<MTLFunction> _Nullable)(alphablending_fragment_shader)];
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
        T1_objc_msg_with_2arg_expect_ptr(
            ags->device,
            ags->sel_new_render_pls_with_descriptor_error,
            (uintptr_t)(__bridge void *)(alpha_pls_desc),
            (uintptr_t)&Error);
    
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
        (__bridge_retained void *)([with_metal_device
            newRenderPipelineStateWithDescriptor:
                alpha_pls_desc 
            error:
                &Error]);
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
        T1_objc_msg_with_1arg_expect_ptr(
            ags->device,
            ags->sel_new_depth_stencil_state_with_desc,
            (uintptr_t)(__bridge void *)(depth_desc));
    
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
        ags->polygon_buffers[fram_i] =
            (__bridge_retained void *)([with_metal_device
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
                   nil]);
        T1_log_assert(ags->polygon_buffers[fram_i] != NULL);
        
        ags->matrix_buffers[fram_i] =
            (__bridge_retained void *)([with_metal_device
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
               nil]);
        
        ags->vertex_buffers[fram_i] =
            (__bridge_retained void *)([with_metal_device
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
                    nil]);
        
        ags->flat_quad_buffers[fram_i] =
            (__bridge_retained void *)([with_metal_device
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
               nil]);
        
        ags->flat_texquad_buffers[fram_i] =
            (__bridge_retained void *)([with_metal_device
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
                    nil]);
        
        ags->postprocessing_constants_buffers[fram_i] =
            (__bridge_retained void *)([with_metal_device
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
                    nil]);
        
        ags->light_buffers[fram_i] =
            (__bridge_retained void *)([with_metal_device
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
                    nil]);
        
        T1_log_assert(
            T1_cpu_to_gpu_data->
                render_views_alloc_size >=
                    (sizeof(T1GPURenderView) *
                        T1_RENDER_VIEW_CAP));
        ags->cam_buffers[fram_i] =
        (__bridge_retained void *)([with_metal_device
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
                nil]);
    }
    
    ags->locked_vertex_populator_buffer =
        (__bridge_retained void *)([with_metal_device
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
                nil]);
    
    ags->locked_vertex_buffer =
        T1_objc_msg_with_2arg_sizet_expect_ptr(
            ags->device,
            ags->sel_new_buf_with_len_options,
            T1_cpu_to_gpu_data->locked_vertices_alloc_size,
            MTLResourceStorageModePrivate);
    
    ags->locked_matf32_populator_buffer =
        (__bridge_retained void *)[with_metal_device
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
    
    ags->locked_matf32_buffer =
        T1_objc_msg_with_2arg_sizet_expect_ptr(
            ags->device,
            ags->sel_new_buf_with_len_options,
            T1_cpu_to_gpu_data->const_matsf32_alloc_size,
            MTLResourceStorageModePrivate);
    
    ags->locked_mats32_populator_buffer =
        (__bridge_retained void *)[with_metal_device
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
    
    ags->locked_mats32_buffer =
        T1_objc_msg_with_2arg_sizet_expect_ptr(
            ags->device,
            ags->sel_new_buf_with_len_options,
            T1_cpu_to_gpu_data->const_matss32_alloc_size,
            MTLResourceStorageModePrivate);
    
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
    void * nsstring_downsample_texture = T1_objc_nsstring_construct(
        "downsample_texture");
    void * downsample_func = T1_objc_msg_with_1arg_expect_ptr(
        ags->lib,
        ags->sel_new_function_with_name,
        (uintptr_t)nsstring_downsample_texture);
    
    ags->downsample_compute_pls =
        T1_objc_msg_with_2arg_expect_ptr(
            ags->device,
            ags->sel_new_compute_pls_with_func,
            (uintptr_t)downsample_func,
            (uintptr_t)NULL);
    
    void * nsstring_boxblur_texture = T1_objc_nsstring_construct(
        "boxblur_texture");
    void * boxblur_texture = T1_objc_msg_with_1arg_expect_ptr(
        ags->lib,
        ags->sel_new_function_with_name,
        (uintptr_t)nsstring_boxblur_texture);
    void * error = NULL;
    ags->boxblur_compute_pls =
        T1_objc_msg_with_2arg_expect_ptr(
            ags->device,
            ags->sel_new_compute_pls_with_func,
            (uintptr_t)boxblur_texture,
            (uintptr_t)&error);
    #elif T1_BLOOM_ACTIVE == T1_INACTIVE
    #else
    #error
    #endif
    
    void * nsstring_singlequad_vertex_shader = T1_objc_nsstring_construct(
        "single_quad_vertex_shader");
    void * singlequad_vertex_shader = T1_objc_msg_with_1arg_expect_ptr(
        ags->lib,
        ags->sel_new_function_with_name,
        (uintptr_t)nsstring_singlequad_vertex_shader);
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
    
    void * nsstring_singlequad_fragment_shader = T1_objc_nsstring_construct(
        "single_quad_frag_shader");
    void * singlequad_fragment_shader = T1_objc_msg_with_1arg_expect_ptr(
        ags->lib,
        ags->sel_new_function_with_name,
        (uintptr_t)nsstring_singlequad_fragment_shader);
    if (singlequad_fragment_shader == NULL)
    {
        T1_log_append("Missing function: downsampling_frag_shader()!");
        T1_std_strcpy_cap(
            error_msg_string,
            512,
            "Missing function: downsampling_fragment_shader()");
        return false;
    }
    
    MTLRenderPipelineDescriptor * singlequad_pipeline_descriptor =
        [MTLRenderPipelineDescriptor new];
    
    // Set up pipeline for rendering the texture to the screen with a simple
    // quad
    singlequad_pipeline_descriptor.label =
        @"single-quad pipeline";
    // singlequad_pipeline_descriptor.sampleCount = 1;
    [singlequad_pipeline_descriptor
        setVertexFunction:
            (__bridge id<MTLFunction> _Nullable)(singlequad_vertex_shader)];
    [singlequad_pipeline_descriptor
        setFragmentFunction:
            (__bridge id<MTLFunction> _Nullable)(singlequad_fragment_shader)];
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
    ags->singlequad_pls =
        T1_objc_msg_with_2arg_expect_ptr(
            ags->device,
            ags->sel_new_render_pls_with_descriptor_error,
            (uintptr_t)(__bridge void *)(singlequad_pipeline_descriptor),
            (uintptr_t)NULL);
    
    ags->command_queue = (__bridge_retained void *)([with_metal_device newCommandQueue]);
    
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
    u32 recipient_cap)
{
    #if T1_LOG_ASSERTS_ACTIVE == T1_ACTIVE
    // pass
    #elif T1_LOG_ASSERTS_ACTIVE == T1_INACTIVE
    (void)recipient_cap;
    #else
    #error
    #endif
    
    T1_log_assert(0); // TODO: reimplement me!
    #if 0
    const char * device_name_cstr =
        [[ags->device name]
            cStringUsingEncoding:
            NSASCIIStringEncoding];
    
    T1_std_strcpy_cap(
        recipient,
        recipient_cap,
        device_name_cstr);
    #endif
}

void T1_os_gpu_update_capacity_if_needed(
    s32 tex_array_i)
{
    if (!ags || !ags->metal_active) { return; }
    
    T1_log_assert(tex_array_i >=  0);
    T1_log_assert(tex_array_i <  31);
    u8 copy_prev = false;
    void * prev_copy = nil;
    
    if (T1_tex_arrays[tex_array_i].deleted) {
        ags->metal_textures[tex_array_i] = nil;
        return;
    } else if (
        ags->metal_textures[tex_array_i] == NULL ||
        (T1_tex_arrays[tex_array_i].bc1_compressed &&
            T1_objc_msg_expect_u64(
                ags->metal_textures[tex_array_i],
                ags->sel_pixel_format) !=
                    MTLPixelFormatBC1_RGBA) ||
        ((!T1_tex_arrays[tex_array_i].bc1_compressed) &&
            T1_objc_msg_expect_u64(
                ags->metal_textures[tex_array_i],
                ags->sel_pixel_format) !=
                    MTLPixelFormatRGBA8Unorm) ||
            T1_tex_arrays[tex_array_i].single_img_width !=
                T1_objc_msg_expect_u64(
                    ags->metal_textures[tex_array_i],
                    ags->sel_width) ||
            T1_tex_arrays[tex_array_i].single_img_height !=
                T1_objc_msg_expect_u64(
                    ags->metal_textures[tex_array_i],
                    ags->sel_height))
    {
        ags->metal_textures[tex_array_i] = nil;
        copy_prev = false;
    } else if (
        T1_tex_arrays[tex_array_i].images_size >
            T1_objc_msg_expect_u64(
                ags->metal_textures[tex_array_i],
                ags->sel_array_length))
    {
        #if T1_LOG_ASSERTS_ACTIVE == T1_ACTIVE
        T1_log_assert(
            T1_tex_arrays[tex_array_i].single_img_width ==
                T1_objc_msg_expect_u64(
                    ags->metal_textures[tex_array_i],
                    ags->sel_width));
        T1_log_assert(
            T1_tex_arrays[tex_array_i].single_img_height ==
                T1_objc_msg_expect_u64(ags->metal_textures[tex_array_i], ags->sel_height));
        T1_log_assert(
            T1_tex_arrays[tex_array_i].is_render_target == (
                T1_objc_msg_expect_u64(
                    ags->metal_textures[tex_array_i],
                    ags->sel_usage) &
                MTLTextureUsageRenderTarget) > 0);
        if (T1_tex_arrays[tex_array_i].bc1_compressed) {
            T1_log_assert(
                T1_objc_msg_expect_u64(
                    ags->metal_textures[tex_array_i],
                    ags->sel_pixel_format) ==
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
                ((
                T1_objc_msg_expect_u64(
                    ags->metal_textures[tex_array_i],
                    ags->sel_usage) &
                MTLTextureUsageRenderTarget) > 0));
        return;
    }
    
    #if 1
    void * texture_descriptor =
        T1_objc_msg_expect_ptr(ags->class_mtl_texture_desc, ags->sel_new);
    T1_objc_msg_with_1arg_expect_ptr(
        texture_descriptor,
        ags->sel_set_texture_type,
        MTLTextureType2DArray);
    T1_objc_msg_with_1arg_expect_ptr(
        texture_descriptor,
        ags->sel_set_array_length,
        T1_tex_arrays[tex_array_i].images_size);
    T1_objc_msg_with_1arg_expect_ptr(
        texture_descriptor,
        ags->sel_set_pixel_format,
        T1_tex_arrays[tex_array_i].bc1_compressed ?
            MTLPixelFormatBC1_RGBA :
            MTLPixelFormatRGBA8Unorm);
    T1_objc_msg_with_1arg_expect_ptr(
        texture_descriptor,
        ags->sel_set_storage_mode,
        MTLStorageModePrivate);
    
    if (T1_tex_arrays[tex_array_i].is_render_target)
    {
        T1_objc_msg_with_1arg_expect_ptr(
            texture_descriptor,
            ags->sel_set_usage,
            MTLTextureUsageShaderRead |
                MTLTextureUsageRenderTarget);
    } else {
        T1_objc_msg_with_1arg_expect_ptr(
            texture_descriptor,
            ags->sel_set_usage,
            MTLTextureUsageShaderRead);
    }
    T1_objc_msg_with_1arg_expect_ptr(
        texture_descriptor,
        ags->sel_set_width,
        T1_tex_arrays[tex_array_i].single_img_width);
    T1_objc_msg_with_1arg_expect_ptr(
        texture_descriptor,
        ags->sel_set_height,
        T1_tex_arrays[tex_array_i].single_img_height);
    
    #if T1_MIPMAPS_ACTIVE == T1_ACTIVE
    T1_objc_msg_with_1arg_expect_ptr(
        texture_descriptor,
        ags->sel_set_mipmap_level_count,
        T1_tex_arrays[tex_array_i].bc1_compressed || tex_array_i == 0 ?
        1 :
        (uintptr_t)floor(
            log2((double)MAX(
                T1_tex_arrays[tex_array_i].single_img_width,
                T1_tex_arrays[tex_array_i].single_img_height))) + 1);
    #elif T1_MIPMAPS_ACTIVE == T1_INACTIVE
    T1_objc_msg_with_1arg_expect_ptr(
        texture_descriptor,
        ags->sel_set_mipmap_level_count,
        1);
    #else
    #error
    #endif
    
    #else
    MTLTextureDescriptor * texture_descriptor =
        [MTLTextureDescriptor new];
    [texture_descriptor setTextureType: MTLTextureType2DArray];
    [texture_descriptor setArrayLength: T1_tex_arrays[tex_array_i].
        images_size];
    [texture_descriptor
        setPixelFormat: T1_tex_arrays[tex_array_i].
            bc1_compressed ?
                MTLPixelFormatBC1_RGBA :
                MTLPixelFormatRGBA8Unorm];
    [texture_descriptor
        setStorageMode: MTLStorageModePrivate];
    if (T1_tex_arrays[tex_array_i].is_render_target)
    {
        [texture_descriptor setUsage:
            MTLTextureUsageShaderRead |
            MTLTextureUsageRenderTarget];
    } else {
        [texture_descriptor setUsage:
            MTLTextureUsageShaderRead];
    }
    [texture_descriptor setWidth:
        T1_tex_arrays[tex_array_i].single_img_width];
    [texture_descriptor setHeight:
        T1_tex_arrays[tex_array_i].single_img_height];
    
    #if T1_MIPMAPS_ACTIVE == T1_ACTIVE
    [texture_descriptor setMipmapLevelCount:
        use_bc1_compression || texture_array_i == 0 ?
        1 :
        (NSUInteger)floor(
            log2((double)MAX(
                single_image_width,
                single_image_height))) + 1];
    #elif T1_MIPMAPS_ACTIVE == T1_INACTIVE
    [texture_descriptor setMipmapLevelCount: 1];
    #else
    #error
    #endif
    
    #endif
    
    T1_log_assert(ags->metal_textures[tex_array_i] == NULL);
    ags->metal_textures[tex_array_i] = T1_objc_msg_with_1arg_expect_ptr(
        ags->device,
        ags->sel_new_tex_with_desc,
        (uintptr_t)texture_descriptor);
    T1_log_assert(ags->metal_textures[tex_array_i] != NULL);
    
    T1_tex_arrays[tex_array_i].gpu_capacity =
        T1_tex_arrays[tex_array_i].images_size;
    
    if (copy_prev) {
        void * combuf =
            T1_objc_msg_expect_ptr(
                ags->command_queue,
                ags->sel_command_buffer);
        void * blitenc =
            T1_objc_msg_expect_ptr(
                combuf,
                ags->sel_blit_command_encoder);
        
        T1_objc_msg_with_2arg_expect_ptr(
            blitenc,
            ags->sel_copy_from_texture_to_texture,
            (uintptr_t)prev_copy,
            (uintptr_t)ags->metal_textures[tex_array_i]);
        T1_objc_msg_expect_ptr(blitenc, ags->sel_end_encoding);
        
        T1_objc_msg_expect_ptr(combuf, ags->sel_commit);
        T1_objc_msg_expect_ptr(combuf, ags->sel_wait_until_completed);
    }
}

// returns the slice_i of the new depth texture
// in the array of depth textures
s16 T1_os_gpu_make_depth_tex(
    u32 width,
    u32 height)
{
    T1_log_assert(height <= 4000);
    T1_log_assert(width <= 4000);
    
    return T1_apple_gpu_make_depth_tex(
        width,
        height);
}

u32 T1_os_gpu_get_touch_id_at_screen_pos(
    f32 screen_x,
    f32 screen_y)
{
    if (
        screen_x < 0 ||
        screen_y < 0 ||
        screen_x >= T1_global->window_wh[0] ||
        screen_y >= T1_global->window_wh[1])
    {
        return T1_TOUCH_ID_NONE;
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
    
    u8 * data = (u8 *)T1_objc_msg_expect_ptr(
        ags->touch_id_buffer, ags->sel_contents);
    u64 size = T1_objc_msg_expect_u64(
        ags->touch_id_buffer,
        ags->sel_allocated_size);
    
    u32 pixel_i = (screen_y_adj * rtt_width) + screen_x_adj;
    
    if (((pixel_i * 4) + 3) >= size)
    {
        return T1_TOUCH_ID_NONE;
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
    
    return uid;
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
    
    ags->depth_textures[slice_i] = NULL;
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
    void * texture = ags->metal_textures[texture_array_i];
    if (
        texture == NULL ||
        T1_objc_msg_expect_u64(
            texture,
            ags->sel_texture_type) !=
                MTLTextureType2DArray)
    {
        return;
    }
    
    if (T1_objc_msg_expect_u64(texture, ags->sel_pixel_format) != MTLPixelFormatRGBA8Unorm)
    {
        // Ensure the texture format is RGBA8Unorm
        // for direct copying to u8 RGBA
        return;
    }
    
    *recipient_width = (u32)T1_objc_msg_expect_u64(texture, ags->sel_width);
    *recipient_height = (u32)T1_objc_msg_expect_u64(texture, ags->sel_height);
    if (*recipient_width < 1 || *recipient_height < 1) {
        return;
    }
    
    // Calculate required buffer size
    NSUInteger bytes_per_row = T1_objc_msg_expect_u64(
        texture,
        ags->sel_width) * 4; // 4 bytes per pixel (RGBA)
    NSUInteger bytes_per_image =
        bytes_per_row *
        T1_objc_msg_expect_u64(texture, ags->sel_height);
    if (recipient_cap < bytes_per_image) {
        *recipient_size = 0;
        return;
    }
    
    // Create a temporary buffer for the copy
    // T1_objc_msg_with_2arg_expect_ptr(ags->device, ags->objc_sel_new_buf_with_len_options, bytes_per_image, MTLResourceStorageModeShared);
    void * temp_buffer =
        T1_objc_msg_with_2arg_sizet_expect_ptr(
            ags->device,
            ags->sel_new_buf_with_len_options,
            bytes_per_image,
            MTLResourceStorageModeShared);
    if (temp_buffer == NULL) {
        return;
    }
    T1_log_assert(temp_buffer != nil);
    
    if (
        texture_i >= (s32)T1_objc_msg_expect_u64(
            texture,
            ags->sel_array_length))
    {
        return;
    }
    
    // Create command buffer and blit encoder
    void * command_buffer =
        T1_objc_msg_expect_ptr(
            ags->command_queue,
            ags->sel_command_buffer);
    void * blit_encoder =
        T1_objc_msg_expect_ptr(
            command_buffer,
            ags->sel_blit_command_encoder);
    
    T1_cmd_copy_texture_to_buffer(
        blit_encoder,
        ags->sel_copy_from_texture_to_buffer,
        texture,
        (uintptr_t)texture_i,
        0,
        T1_objc_set_make(0, 0, 0),
        T1_objc_set_make(
            T1_objc_msg_expect_u64(texture, ags->sel_width),
            T1_objc_msg_expect_u64(texture, ags->sel_height),
            1),
        temp_buffer,
        0,
        bytes_per_row,
        bytes_per_image);
    
    T1_cmd_copy_texture_to_buffer(
        blit_encoder,
        ags->sel_copy_from_texture_to_buffer,
        texture,
        (uintptr_t)texture_i,
        0,
        T1_objc_set_make(0, 0, 0),
        T1_objc_set_make(
            T1_objc_msg_expect_u64(texture, ags->sel_width),
            T1_objc_msg_expect_u64(texture, ags->sel_height),
            1),
        temp_buffer,
        0,
        bytes_per_row,
        bytes_per_image);
    
    T1_objc_msg_expect_ptr(
        blit_encoder,
        ags->sel_end_encoding);
    
    // Add completion handler to copy data
    // to rgba_recipient
    [(__bridge id<MTLCommandBuffer>)command_buffer
        addCompletedHandler:
            ^(id<MTLCommandBuffer> cb) {
        if (cb.error == nil)
        {
            // Copy buffer contents to rgba_recipient
            T1_std_memcpy(
                rgba_recipient,
                [(__bridge id<MTLBuffer>)temp_buffer contents],
                bytes_per_image);
            *good = true;
        }
        // Release the temporary buffer
        [(__bridge id<MTLBuffer>)temp_buffer setPurgeableState:MTLPurgeableStateEmpty];
    }];
    
    // Commit the command buffer
    T1_objc_msg_expect_ptr(command_buffer, ags->sel_commit);
    T1_objc_msg_expect_ptr(command_buffer, ags->sel_wait_until_completed);
    
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
    s32 texture_array_i)
{
    // no mipmaps for font
    T1_log_assert(texture_array_i != 0);
    // no mipmaps for bc1 compressed arrays
    T1_log_assert(!T1_tex_arrays[texture_array_i].bc1_compressed);
    
    void * combuf =
        T1_objc_msg_expect_ptr(
            ags->command_queue,
            ags->sel_command_buffer);
    
    // Create a blit command encoder
    void * blit_mipmap_encoder =
        T1_objc_msg_expect_ptr(
            combuf,
            ags->sel_blit_command_encoder);
    
    // Generate mipmaps
    [(__bridge id<MTLBlitCommandEncoder>)blit_mipmap_encoder
        generateMipmapsForTexture:
            (__bridge id<MTLTexture> _Nonnull)(ags->metal_textures[texture_array_i])
        ];
    
    T1_objc_msg_expect_ptr(
        blit_mipmap_encoder,
        ags->sel_end_encoding);
    
    T1_objc_msg_expect_ptr(combuf, ags->sel_commit);
    T1_objc_msg_expect_ptr(combuf, ags->sel_wait_until_completed);
}
#elif T1_MIPMAPS_ACTIVE == T1_INACTIVE
// Pass
#else
#error
#endif

void T1_os_gpu_push_tex_slice(
    s32 tex_array_i,
    s32 tex_slice_i,
    b8 free_rgba)
{
    if (!ags || !ags->metal_active) { return; }
    
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
    
    void * temp_source_buf =
        T1_objc_msg_with_4arg_expect_ptr(
            ags->device,
            ags->sel_new_buffer_with_bytes_no_copy,
            (uintptr_t)temp_src_buf_ptr,
            temp_buf_cap,
            MTLResourceStorageModeShared,
            (uintptr_t)NULL);
    
    if (temp_source_buf == NULL) {
        T1_log_assert(0);
        return;
    }
    
    void * combuf = T1_objc_msg_expect_ptr(
        ags->command_queue,
        ags->sel_command_buffer);
    
    void * blit_copy_encoder = T1_objc_msg_expect_ptr(
        combuf,
        ags->sel_blit_command_encoder);
    
    [(__bridge id<MTLBlitCommandEncoder>)blit_copy_encoder
        copyFromBuffer:
            (__bridge id<MTLBuffer> _Nonnull)(temp_source_buf)
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
            (__bridge id<MTLTexture> _Nonnull)(ags->metal_textures[tex_array_i])
        destinationSlice:
            (NSUInteger)tex_slice_i
        destinationLevel:
            0
        destinationOrigin:
            MTLOriginMake(0, 0, 0)];
    
    T1_objc_msg_expect_ptr(blit_copy_encoder, ags->sel_end_encoding);
    
    T1_objc_msg_expect_ptr(combuf, ags->sel_commit);
    T1_objc_msg_expect_ptr(combuf, ags->sel_wait_until_completed);
    
    vm_deallocate(mach_task_self(), vm_ptr, vm_size);
    
    if (free_rgba) {
        T1_mem_free_managed(rgba_freeable);
        T1_tex_arrays[tex_array_i].images[tex_slice_i].image.
            rgba_values_freeable = NULL;
        T1_tex_arrays[tex_array_i].images[tex_slice_i].image.
            rgba_values_page_aligned = NULL;    
    }
}

void T1_os_gpu_copy_locked_vertices(void)
{
    if (!ags || !ags->metal_active) { return; }
    
    T1_cpu_to_gpu_data->locked_vertices_size = T1_mesh_summary_all_vertices->size;
    
    id <MTLCommandBuffer> combuf =
        (__bridge id<MTLCommandBuffer>)(
            T1_objc_msg_expect_ptr(
                ags->command_queue,
                ags->sel_command_buffer));
    
    id <MTLBlitCommandEncoder> blit_copy_encoder =
        [combuf blitCommandEncoder];
    [blit_copy_encoder
        copyFromBuffer:
            (__bridge id<MTLBuffer> _Nonnull)(ags->locked_vertex_populator_buffer)
        sourceOffset:
            0
        toBuffer:
            (__bridge id<MTLBuffer> _Nonnull)(ags->locked_vertex_buffer)
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
    
    id <MTLCommandBuffer> combuf = (__bridge id<MTLCommandBuffer>)(
        T1_objc_msg_expect_ptr(
            ags->command_queue,
            ags->sel_command_buffer));
    
    id <MTLBlitCommandEncoder> blit_copy_encoder = [combuf blitCommandEncoder];
    [blit_copy_encoder
        copyFromBuffer:
            (__bridge id<MTLBuffer> _Nonnull)(ags->locked_matf32_populator_buffer)
        sourceOffset:
            0
        toBuffer:
            (__bridge id<MTLBuffer> _Nonnull)(ags->locked_matf32_buffer)
        destinationOffset:
            0
        size:
            T1_cpu_to_gpu_data->const_matsf32_alloc_size];
    [blit_copy_encoder
        copyFromBuffer:
            (__bridge id<MTLBuffer> _Nonnull)(ags->locked_mats32_populator_buffer)
        sourceOffset:
            0
        toBuffer:
            (__bridge id<MTLBuffer> _Nonnull)(ags->locked_mats32_buffer)
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
    
    void * parent = ags->metal_textures[at_array_i];
    
    NSRange level_range = NSMakeRange(
        0,
        T1_objc_msg_expect_u64(
            parent,
            ags->sel_mipmap_level_count));
    NSRange slice_range = NSMakeRange(
        (NSUInteger)at_slice_i,
        1);
    
    id<MTLTexture> retval = [(__bridge id<MTLTexture>)parent
        newTextureViewWithPixelFormat:
            T1_objc_msg_expect_u64(parent, ags->sel_pixel_format)
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
    desc.depthAttachment.texture = (__bridge id<MTLTexture> _Nullable)(ags->cur_depth);
    
    if (!ags->rtt_cleared) {
        desc.colorAttachments[0].loadAction = MTLLoadActionClear;
        desc.colorAttachments[0].clearColor =
            MTLClearColorMake(0.0f, 0.0f, 0.1f, 1.0f);
        ags->rtt_cleared = true;
    } else {
        desc.colorAttachments[0].loadAction = MTLLoadActionLoad;
    }
    
    desc.colorAttachments[0].texture =
        (__bridge id<MTLTexture> _Nullable)(ags->cur_rtt);
    desc.colorAttachments[0].storeAction =
        MTLStoreActionStore;
    
    // ID Buffer for touchables
    if (
        T1_render_views->cpu[cam_i].write_type ==
            T1RENDERVIEW_WRITE_RENDER_TARGET)
    {
        desc.colorAttachments[1].texture =
            (__bridge id<MTLTexture> _Nullable)(ags->touch_id_texture);
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
            (__bridge id<MTLDepthStencilState> _Nullable)ags->opaque_depth_stencil_state];
    
    [encoder setDepthClipMode: MTLDepthClipModeClip];
    [encoder setCullMode: MTLCullModeBack];
    [encoder setFrontFacingWinding:
        MTLWindingCounterClockwise];
    
    [encoder setViewport: ags->render_viewports[cam_i]];
    
    [encoder
        setVertexBuffer:
            (__bridge id<MTLBuffer> _Nullable)(ags->vertex_buffers[ags->frame_i])
        offset:
            0
        atIndex:
            0];
    
    [encoder
        setVertexBuffer:
            (__bridge id<MTLBuffer> _Nullable)(ags->polygon_buffers[ags->frame_i])
        offset:
            0
        atIndex:
            1];
    
    T1_log_assert(ags->matrix_buffers[ags->frame_i] != nil);
    [encoder
        setVertexBuffer:
         (__bridge id<MTLBuffer> _Nullable)(ags->matrix_buffers[ags->frame_i])
        offset:
            0
        atIndex:
            2];
    
    [encoder
        setVertexBuffer:
         (__bridge id<MTLBuffer> _Nullable)(ags->cam_buffers[ags->frame_i])
        offset: 0
        atIndex: 3];
    
    [encoder
        setVertexBytes: &cam_i
        length: sizeof(u32)
        atIndex: 4];
    
    [encoder
        setVertexBuffer:
            (__bridge id<MTLBuffer> _Nullable)(ags->locked_vertex_buffer)
        offset:
            0 
        atIndex:
            5];
    
    [encoder
        setFragmentBuffer:
            (__bridge id<MTLBuffer> _Nullable)(ags->locked_vertex_buffer)
        offset:
            0
        atIndex:
            0];
    
    [encoder
        setFragmentBuffer:
            (__bridge id<MTLBuffer> _Nullable)(ags->polygon_buffers[ags->frame_i])
        offset:
            0
        atIndex:
            1];
    
    [encoder
        setFragmentBuffer:
            (__bridge id<MTLBuffer> _Nullable)(ags->light_buffers[ags->frame_i])
        offset:
            0
        atIndex:
            2];
    
    [encoder
        setFragmentBuffer:
         (__bridge id<MTLBuffer> _Nullable)(ags->cam_buffers[ags->frame_i])
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
            (__bridge id<MTLBuffer> _Nullable)(ags->locked_matf32_buffer)
        offset:
            0
        atIndex:
            6];
    
    [encoder
        setFragmentBuffer:
            (__bridge id<MTLBuffer> _Nullable)(ags->locked_mats32_buffer)
        offset:
            0
        atIndex:
            8];
    
    [encoder
        setFragmentBuffer:
            (__bridge id<MTLBuffer> _Nullable)(ags->postprocessing_constants_buffers[ags->frame_i])
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
                setFragmentTexture:
                    (__bridge id<MTLTexture> _Nullable)(ags->metal_textures[i])
                atIndex:
                    i];
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
            setFragmentTexture:
                (__bridge id<MTLTexture> _Nullable)(ags->depth_textures[rv_i])
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
        [MTLTextureDescriptor new];
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
    
    MTLTextureDescriptor * zbuffer_desc =
        [MTLTextureDescriptor new];
    zbuffer_desc.textureType = MTLTextureType2D;
    zbuffer_desc.pixelFormat = MTLPixelFormatDepth32Float;
    zbuffer_desc.width       = T1_render_views->cpu[at_i].width;
    zbuffer_desc.height      = T1_render_views->cpu[at_i].height;
    zbuffer_desc.storageMode = MTLStorageModePrivate;
    zbuffer_desc.usage       =
        MTLTextureUsageRenderTarget |
        MTLTextureUsageShaderRead;
    
    ags->depth_textures[at_i] = T1_objc_msg_with_1arg_expect_ptr(
        ags->device,
        ags->sel_new_tex_with_desc,
        (uintptr_t)(__bridge void *)(zbuffer_desc));
    
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
    ags->touch_id_texture = T1_objc_msg_with_1arg_expect_ptr(
        ags->device,
        ags->sel_new_tex_with_desc,
        (uintptr_t)(__bridge void *)(touch_id_tex_desc));
    
    u64 touch_buffer_size_bytes =
        touch_id_tex_desc.width *
            touch_id_tex_desc.height *
            4;
    
    T1_log_assert(ags->device != NULL);
    ags->touch_id_buffer =
        T1_objc_msg_with_2arg_sizet_expect_ptr(
            ags->device,
            ags->sel_new_buf_with_len_options,
            touch_buffer_size_bytes,
            MTLResourceStorageModeShared);
    T1_log_assert(ags->touch_id_buffer != NULL);
    
    ags->touch_id_buffer_all_zeros =
        T1_objc_msg_with_2arg_sizet_expect_ptr(
            ags->device,
            ags->sel_new_buf_with_len_options,
            touch_buffer_size_bytes,
            MTLResourceStorageModeShared);
    
    s32 minus_one = -1;
    T1_std_memset_s32(
        T1_objc_msg_expect_ptr(
            ags->touch_id_buffer_all_zeros,
            ags->sel_contents),
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
        ags->downsampled_rtts[i] = T1_objc_msg_with_1arg_expect_ptr(
            ags->device,
            ags->sel_new_tex_with_desc,
            (uintptr_t)(__bridge void *)(downsampled_rtt_desc));
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
            
            outlines_desc.colorAttachments[1].texture = nil;
            
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
                    (__bridge id<MTLRenderPipelineState> _Nonnull)(ags->outlines_pls)];
            [pass_1_outline_enc
                setDepthStencilState:
                    (__bridge id<MTLDepthStencilState> _Nullable)(ags->opaque_depth_stencil_state)];
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
                    (__bridge id<MTLRenderPipelineState> _Nonnull)(ags->cur_opq_pls)];
            
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
                    (__bridge id<MTLRenderPipelineState> _Nonnull)(ags->cur_blnd_pls)];
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
                    (__bridge id<MTLRenderPipelineState> _Nonnull)(ags->cur_bb_pls)];
            [bb_enc
                setDepthStencilState:
                    (__bridge id<MTLDepthStencilState> _Nullable)(ags->opaque_depth_stencil_state)];
            
            [bb_enc
                setVertexBuffer:
                    (__bridge id<MTLBuffer> _Nullable)(ags->flat_quad_buffers[ags->frame_i])
                offset: 0
                atIndex: 2];
            
            [bb_enc
                setVertexBuffer:
                    (__bridge id<MTLBuffer> _Nullable)(ags->cam_buffers[ags->frame_i])
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
            
            void * bloom_rtt = ags->downsampled_rtts[0];
            bloom_desc.colorAttachments[0].
                texture = (__bridge id<MTLTexture> _Nullable)(bloom_rtt);
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
                    (__bridge id<MTLRenderPipelineState> _Nonnull)(ags->cur_bloom_pls)];
            
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
                
                smaller_viewport.width =
                    T1_objc_msg_expect_u64(
                        ags->downsampled_rtts[ds_i],
                        ags->sel_width);
                smaller_viewport.height =
                    T1_objc_msg_expect_u64(
                        ags->downsampled_rtts[ds_i],
                        ags->sel_height);
                
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
                            (__bridge id<MTLComputePipelineState> _Nonnull)(ags->downsample_compute_pls)];
                    [compute_enc
                        setTexture:
                            (__bridge id<MTLTexture> _Nullable)(ds_i > 0 ?
                                ags->downsampled_rtts[ds_i-1] :
                                bloom_rtt)
                        atIndex:0];
                    [compute_enc
                        setTexture:
                            (__bridge id<MTLTexture> _Nullable)(ags->downsampled_rtts[ds_i])
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
                        (__bridge id<MTLComputePipelineState> _Nonnull)(ags->boxblur_compute_pls)];
                [boxblur_enc
                    setTexture:
                        (__bridge id<MTLTexture> _Nullable)(ags->downsampled_rtts[ds_i])
                    atIndex:
                        0];
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
                    (__bridge id<MTLRenderPipelineState> _Nonnull)(ags->cur_flat_texquad_pls)];
            [flat_texq_enc
                setDepthStencilState:
                    (__bridge id<MTLDepthStencilState> _Nullable)(ags->opaque_depth_stencil_state)];
            
            [flat_texq_enc
                setVertexBuffer:
                    (__bridge id<MTLBuffer> _Nullable)(ags->flat_texquad_buffers[ags->frame_i])
                offset: 0
                atIndex: 0];
            
            [flat_texq_enc
                setVertexBuffer:
                    (__bridge id<MTLBuffer> _Nullable)(ags->matrix_buffers[ags->frame_i])
                offset: 0
                atIndex: 2];
            
            [flat_texq_enc
                setVertexBuffer:
                    (__bridge id<MTLBuffer> _Nullable)(ags->cam_buffers[ags->frame_i])
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
                        setFragmentTexture:
                            (__bridge id<MTLTexture> _Nullable)(ags->metal_textures[i])
                        atIndex:
                            i];
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
    
    id<MTLCommandBuffer> combuf = (__bridge id<MTLCommandBuffer>)(
        T1_objc_msg_expect_ptr(
            ags->command_queue,
            ags->sel_command_buffer));
    
    if (combuf == nil) {
        #if T1_LOGGER_ASSERTS_ACTIVE
        log_dump_and_crash("error - can't get metal command buffer\n");
        #endif
        
        return;
    }
    
    // Blit to clear the touch id buffer
    T1_log_assert(ags->touch_id_texture != NULL);
    u64 touch_id_w = T1_objc_msg_expect_u64(
        ags->touch_id_texture,
        ags->sel_width);
    u64 touch_id_h = T1_objc_msg_expect_u64(
        ags->touch_id_texture,
        ags->sel_height);
    u64 size_bytes = touch_id_w * touch_id_h * 8;
    
    id <MTLBlitCommandEncoder>
        clear_touch_tex_blit_enc =
            [combuf blitCommandEncoder];
    
    [clear_touch_tex_blit_enc
        copyFromBuffer:
            (__bridge id<MTLBuffer> _Nonnull)(ags->touch_id_buffer_all_zeros)
        sourceOffset:
            0
        sourceBytesPerRow:
            touch_id_w * 4
        sourceBytesPerImage:
            size_bytes
        sourceSize:
            MTLSizeMake(touch_id_w, touch_id_h, 1)
        toTexture:
            (__bridge id<MTLTexture> _Nonnull)(ags->touch_id_texture)
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
                ags->cur_rtt = (__bridge void *)(get_tex_slice(
                    T1_tex_to_array_i(
                        T1_render_views->cpu[cam_i].write_tex),
                    T1_tex_to_slice_i(
                        T1_render_views->cpu[cam_i].write_tex)));
                if (ags->cur_rtt == nil) { continue; }
                
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
                    (__bridge void *)(get_tex_slice(
                        T1_tex_to_array_i(T1_render_views->cpu[cam_i].write_tex),
                        T1_tex_to_slice_i(T1_render_views->cpu[cam_i].write_tex)));
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
            pass_i < T1_render_views->cpu[cam_i].
                passes_size;
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
        copyFromTexture: (__bridge id<MTLTexture> _Nonnull)(ags->touch_id_texture)
        sourceSlice: 0
        sourceLevel: 0
        sourceOrigin: MTLOriginMake(0, 0, 0)
        sourceSize:
            MTLSizeMake(
                touch_id_w,
                touch_id_h,
                1)
        toBuffer:
            (__bridge id<MTLBuffer> _Nonnull)(ags->touch_id_buffer)
        destinationOffset: 0
        destinationBytesPerRow:
            touch_id_w * 4
        destinationBytesPerImage:
            touch_id_w * touch_id_h * 4];
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
        setRenderPipelineState:
            (__bridge id<MTLRenderPipelineState> _Nonnull)(ags->singlequad_pls)];
    [pass_5_comp
        setVertexBytes:
            ags->quad_vertices
        length:
            sizeof(T1PostProcessingVertex)*6
        atIndex:
            0];
    [pass_5_comp
        setVertexBuffer:
            (__bridge id<MTLBuffer> _Nullable)(ags->postprocessing_constants_buffers[ags->frame_i])
        offset:0
        atIndex:1];
    
    // The main camera must have a target to write to
    T1_log_assert(
        T1_tex_to_array_i(T1_render_views->cpu[0].write_tex) >= 0);
    T1_log_assert(
        T1_tex_to_array_i(T1_render_views->cpu[0].write_tex) <
            (s32)T1_tex_arrays_size);
    
    id<MTLTexture> arr_tex = (__bridge id<MTLTexture>)(ags->metal_textures[
        T1_tex_to_array_i(T1_render_views->cpu[0].write_tex)]);
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
            (__bridge id<MTLTexture> _Nullable)(ags->downsampled_rtts[1])
        atIndex: 1];
    [pass_5_comp
        setFragmentTexture:
            (__bridge id<MTLTexture> _Nullable)(ags->downsampled_rtts[2])
        atIndex:2];
    [pass_5_comp
        setFragmentTexture:
            (__bridge id<MTLTexture> _Nullable)ags->downsampled_rtts[3]
        atIndex:3];
    [pass_5_comp
        setFragmentTexture:
            (__bridge id<MTLTexture> _Nullable)ags->downsampled_rtts[4]
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
        setFragmentTexture:
            (__bridge id<MTLTexture> _Nullable)(
                ags->metal_textures[perlin_ta_i])
        atIndex:
            6];
    #elif T1_LOG_ASSERTS_ACTIVE == T1_INACTIVE
    // Pass
    #else
    #error
    #endif
    
    [pass_5_comp
         setFragmentTexture:
             (__bridge id<MTLTexture> _Nullable)(ags->depth_textures[0])
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
    if (!ags || !ags->metal_active) { return; }
    
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
    u32 width,
    u32 height)
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
        [MTLTextureDescriptor new];;
    
    desc.width = width;
    desc.height = height;
    desc.textureType = MTLTextureType2D;
    desc.pixelFormat = MTLPixelFormatDepth32Float;
    desc.usage =
        MTLTextureUsageRenderTarget |
        MTLTextureUsageShaderRead;
    
    ags->depth_textures[slice_i] =
        T1_objc_msg_with_1arg_expect_ptr(
            ags->device,
            ags->sel_new_tex_with_desc,
            (uintptr_t)(__bridge void *)(desc));
    
    T1_log_assert(ags->depth_textures[slice_i] != nil);
    
    return slice_i;
}
