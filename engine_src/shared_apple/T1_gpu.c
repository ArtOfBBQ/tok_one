#include "T1_gpu.h"

#include <mach/mach.h>

#include "T1_global.h"
#include "T1_mem.h"
#include "T1_log.h"
#include "T1_objc.h"
#include "T1_settings.h"
#include "T1_material.h"
#include "T1_mesh_summary.h"
#include "T1_tex_array.h"
#include "T1_render_view.h"
#include "T1_platform_layer.h"

#define T1MTLPrimitiveTypePoint 0
#define T1MTLPrimitiveTypeLine 1
#define T1MTLPrimitiveTypeLineStrip 2
#define T1MTLPrimitiveTypeTriangle 3
#define T1MTLPrimitiveTypeTriangleStrip 4

#define T1MTLWindingClockwise 0
#define T1MTLWindingCounterClockwise 1
#define T1MTLBlendOperationAdd 0
#define T1MTLBlendFactorOne 1
#define T1MTLMutabilityImmutable 2
#define T1MTLTextureType2D 2
#define T1MTLTextureType2DArray 3
#define T1MTLCompareFunctionLessEqual 3
#define T1MTLPixelFormatRGBA8Unorm 70
#define T1MTLPixelFormatBGRA8Unorm 80
#define T1MTLPixelFormatBC1_RGBA 130
#define T1MTLPixelFormatDepth32Float 252
#define T1MTLDepthClipModeClip 0
#define T1MTLDepthClipModeClamp 1
#define T1MTLCullModeNone 0
#define T1MTLCullModeFront 1
#define T1MTLCullModeBack 2
#define T1MTLTextureUsageShaderRead 0x0001
#define T1MTLTextureUsageShaderWrite 0x0002
#define T1MTLResourceStorageModeShared  0
#define T1MTLResourceUsageRead 1
#define T1MTLStorageModePrivate 2
#define T1MTLResourceStorageModeManaged 16
#define T1MTLResourceStorageModePrivate 32
#define T1MTLTextureUsageRenderTarget 0x0004
#define T1MTLLoadActionClear 2

#define T1MTLLoadActionLoad 1
#define T1MTLStoreActionStore 1

typedef struct {
    double originX, originY, width, height, znear, zfar;
} T1MTLViewport;

typedef struct {
    uintptr_t pixel_format_renderpass1;
    T1MTLViewport window_viewport;
    T1MTLViewport render_viewports[T1_RENDER_VIEW_CAP];
    
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
    void * class_mtl_render_pipeline_desc; // MTLRenderPipelineDescriptor
    void * class_mtl_depth_stencil_desc; // MTLDepthStencilDescriptor
    void * class_nsurl; // NSURL;
    // objective-c selectors
    void * sel_set_depth_attachment_pixel_format; // setDepthAttachmentPixelFormat
    void * sel_set_vertex_function; // setVertexFunction:
    void * sel_set_fragment_function; // setFragmentFunction:
    void * sel_set_label; // setLabel:
    void * sel_color_attachments; // colorAttachments
    void * sel_set_clear_color; // setClearColor:
    void * sel_object_at_indexed_subscript; // objectAtIndexedSubscript:
    void * sel_set_mutability; // setMutability:
    void * sel_set_object_at_indexed_subscript; // setObject:atIndexedSubscript:
    void * sel_new;
    void * sel_command_buffer; // commandBuffer
    void * sel_compute_cmd_enc; // computeCommandEncoder
    void * sel_set_texture_at_index; // setTexture:atIndex:
    void * sel_set_compute_pls; // setComputePipelineState:
    void * sel_new_library_with_URL_error; // newLibraryWithURL:error:
    void * sel_render_cmd_enc_with_desc; // renderCommandEncoderWithDescriptor:
    void * sel_set_viewport; // setViewport:
    void * sel_set_vertex_bytes_length_atindex; // setVertexBytes:length:atIndex:
    void * sel_set_vertex_buffer_offset_atindex; // setVertexBuffer:offset:atIndex:
    void * sel_set_fragment_buffer_offset_atindex; // setFragmentBuffer:Offset:atIndex:
    void * sel_set_fragment_bytes_length_atindex; // setFragmentBytes:length:atIndex:
    void * sel_set_fragment_texture_at_index; // setFragmentTexture:atIndex:
    void * sel_set_render_pls; // setRenderPipelineState
    void * sel_set_depth_stencil_state; // setDepthStencilState:
    void * sel_set_depth_clip_mode; // setDepthClipMode:
    void * sel_set_cull_mode; // setCullMode:
    void * sel_draw_primitives_vertex_start_vertex_count; // drawPrimitives:vertexStart:vertexCount:
    void * sel_set_front_facing_winding; // setFrontFacingWinding:
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
    void * sel_new_texture_view_with_pixel_format; // newTextureViewWithPixelFormat:textureType:levels:slices: 
    void * sel_set_array_length; // setArrayLength;
    void * sel_set_pixel_format; // setPixelFormat:
    void * sel_set_blending_enabled; // setBlendingEnabled:
    void * sel_set_source_rgb_blend_factor; // setSourceRGBBlendFactor:
    void * sel_set_dest_rgb_blend_factor; // setDestinationRGBBlendFactor:
    void * sel_set_rgb_blend_operation; // setRgbBlendOperation:
    void * sel_set_storage_mode; // setStorageMode:
    void * sel_set_usage; // setUsage:
    void * sel_set_width; // setWidth:
    void * sel_set_height; // setHeight:
    void * sel_set_mipmap_level_count; // setMipmapLevelCount:
    void * sel_usage;
    void * sel_mipmap_level_count; // mipmapLevelCount
    void * sel_new_depth_stencil_state_with_desc;
    void * sel_blit_command_encoder; // blitCommandEncoder
    void * sel_copy_from_buffer_source_offset_to_buffer; // copyFromBuffer:sourceOffset:toBuffer:destinationOffset:size:
    void * sel_copy_from_buffer_source_offset_source_bytes_per_row; 
    void * sel_generate_mipmaps_for_texture; // generateMipmapsForTexture:
    void * sel_present_drawable; // presentDrawable:
    void * sel_commit; // commit
    void * sel_wait_until_completed; // waitUntilCompleted
    void * sel_end_encoding; // endEncoding
    void * sel_current_drawable; // currentDrawable
    void * sel_copy_from_texture_to_texture; // copyFromTexture:toTexture:
    void * sel_copy_from_texture_to_buffer; // copyFromTexture:sourceSlice:sourceLevel:sourceOrigin:sourceSize:toBuffer:destinationOffset:destinationBytesPerRow:destinationBytesPerImage:
    void * sel_new_buffer_with_bytes_no_copy; // newBufferWithBytesNoCopy:length:options:deallocator:
    void * sel_new_command_queue; // newCommandQueue
    void * sel_vertex_buffers; // vertexBuffers
    void * sel_new_default_library; // newDefaultLibrary
    void * sel_depth_attachment; // depthAttachment
    void * sel_set_load_action; // setLoadAction:
    void * sel_set_clear_depth; // setClearDepth:
    void * sel_set_store_action; // setStoreAction:
    void * sel_set_texture; // setTexture:
    void * sel_set_depth_write_enabled; // setDepthWriteEnabled:
    void * sel_set_depth_compare_function; // setDepthCompareFunction:
    void * sel_current_render_pass_desc; // currentRenderPassDescriptor
    void * sel_file_URL_with_path_is_directory; // fileURLWithPath:isDirectory:
    void * sel_dispatch_threads_threadsperthreadgroup; // dispatchThreads:threadsPerThreadsGroup:
    T1PostProcessingVertex quad_vertices[6];
    f32 retina_scaling_factor;
    u8  viewports_set[T1_RENDER_VIEW_CAP];
    u8  frame_i;
    b8  metal_active;
    b8  zbuf_cleared;
    b8  rtt_cleared;
    b8  objc_metal_framework_good;
} AppleGPUState;

static AppleGPUState * ags = NULL;

static void (* funcptr_gameloop_before_render)(T1GPUFrame *) = NULL;
static void (* funcptr_gameloop_after_render)(void) = NULL;

static void * T1_gpu_new_render_pipeline_descriptor(
    void * vertex_shader,
    void * fragment_shader,
    b8 has_touch_tex,
    b8 blending_enabled)
{
    void * desc = (void *)T1_objc_msg(
        ags->class_mtl_render_pipeline_desc,
        ags->sel_new);
    T1_objc_msg_1arg(
        desc,
        ags->sel_set_vertex_function,
        (uintptr_t)vertex_shader);
    T1_objc_msg_1arg(
        desc,
        ags->sel_set_fragment_function,
        (uintptr_t)fragment_shader);
    {
        void * color_attachments = (void *)T1_objc_msg(
            desc,
            ags->sel_color_attachments);
        void * color_attachment_0 = (void *)T1_objc_msg_1arg(
            color_attachments,
            ags->sel_object_at_indexed_subscript,
            0);
        T1_objc_msg_1arg(
            color_attachment_0,
            ags->sel_set_pixel_format,
            ags->pixel_format_renderpass1);
        if (blending_enabled) {
        T1_objc_msg_1arg(
            color_attachment_0,
            ags->sel_set_blending_enabled,
            1);
        T1_objc_msg_1arg(
            color_attachment_0,
            ags->sel_set_source_rgb_blend_factor,
            T1MTLBlendFactorOne);
        T1_objc_msg_1arg(
            color_attachment_0,
            ags->sel_set_dest_rgb_blend_factor,
            T1MTLBlendFactorOne);
        T1_objc_msg_1arg(
            color_attachment_0,
            ags->sel_set_rgb_blend_operation,
            T1MTLBlendOperationAdd);
        }
        if (has_touch_tex) {
            void * color_attachment_1 = (void *)T1_objc_msg_1arg(
                color_attachments,
                ags->sel_object_at_indexed_subscript,
                1);
            T1_objc_msg_1arg(
                color_attachment_1,
                ags->sel_set_pixel_format,
                ags->pixel_format_renderpass1);
        }
    }
    T1_objc_msg_1arg(
        desc,
        ags->sel_set_depth_attachment_pixel_format,
        T1MTLPixelFormatDepth32Float);
    
    return desc;
}

static void T1_gpu_render_pl_descriptor_remove_color_attachment_at(
    void * desc,
    u32 index)
{
    void * color_attachments = (void *)T1_objc_msg(
        desc,
        ags->sel_color_attachments);
    T1_objc_msg_2arg(
        color_attachments,
        ags->sel_set_object_at_indexed_subscript,
        0,
        index);
}

u8 T1_apple_gpu_init(
    void (* arg_funcptr_shared_gameloop_update)(T1GPUFrame *),
    void (* arg_funcptr_shared_gameloop_update_after_render_pass)(void),
    void * with_metal_device,
    char * shader_lib_filepath,
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
    ags->class_mtl_render_pipeline_desc = T1_objc_get_class("MTLRenderPipelineDescriptor");
    ags->class_mtl_depth_stencil_desc = T1_objc_get_class("MTLDepthStencilDescriptor");
    ags->class_nsurl = T1_objc_get_class("NSURL");
    ags->sel_set_depth_attachment_pixel_format = T1_objc_reg_sel(
        "setDepthAttachmentPixelFormat:");
    ags->sel_set_vertex_function = T1_objc_reg_sel(
        "setVertexFunction:");
    ags->sel_set_fragment_function = T1_objc_reg_sel(
        "setFragmentFunction:");
    ags->sel_set_label = T1_objc_reg_sel("setLabel:");
    ags->sel_color_attachments = T1_objc_reg_sel(
        "colorAttachments");
    ags->sel_set_clear_color = T1_objc_reg_sel(
        "setClearColor:");
    ags->sel_object_at_indexed_subscript = T1_objc_reg_sel(
        "objectAtIndexedSubscript:");
    ags->sel_set_mutability = T1_objc_reg_sel("setMutability:");
    ags->sel_set_object_at_indexed_subscript = T1_objc_reg_sel(
        "setObject:atIndexedSubscript:");
    ags->sel_new = T1_objc_reg_sel("new");
    ags->sel_command_buffer = T1_objc_reg_sel("commandBuffer");
    ags->sel_compute_cmd_enc = T1_objc_reg_sel(
        "computeCommandEncoder");
    ags->sel_set_texture_at_index = T1_objc_reg_sel(
        "setTexture:atIndex:");
    ags->sel_set_compute_pls = T1_objc_reg_sel(
        "setComputePipelineState:");
    ags->sel_render_cmd_enc_with_desc = T1_objc_reg_sel(
        "renderCommandEncoderWithDescriptor:");
    ags->sel_set_viewport = T1_objc_reg_sel("setViewport:");
    ags->sel_set_render_pls = T1_objc_reg_sel(
        "setRenderPipelineState:");
    ags->sel_set_vertex_bytes_length_atindex = T1_objc_reg_sel(
        "setVertexBytes:length:atIndex:");
    ags->sel_set_vertex_buffer_offset_atindex = T1_objc_reg_sel(
        "setVertexBuffer:offset:atIndex:");
    ags->sel_set_fragment_buffer_offset_atindex = T1_objc_reg_sel(
        "setFragmentBuffer:offset:atIndex:");
    ags->sel_set_fragment_bytes_length_atindex = T1_objc_reg_sel(
        "setFragmentBytes:length:atIndex:");
    ags->sel_set_fragment_texture_at_index = T1_objc_reg_sel(
        "setFragmentTexture:atIndex:");
    ags->sel_set_depth_stencil_state = T1_objc_reg_sel(
        "setDepthStencilState:");
    ags->sel_set_depth_clip_mode = T1_objc_reg_sel(
        "setDepthClipMode:");
    ags->sel_set_cull_mode = T1_objc_reg_sel("setCullMode:");
    ags->sel_draw_primitives_vertex_start_vertex_count =
        T1_objc_reg_sel(
            "drawPrimitives:vertexStart:vertexCount:");
    ags->sel_set_front_facing_winding = T1_objc_reg_sel(
        "setFrontFacingWinding:");
    ags->sel_new_function_with_name = T1_objc_reg_sel("newFunctionWithName:"); 
    ags->sel_width  = T1_objc_reg_sel("width");
    ags->sel_height = T1_objc_reg_sel("height");
    ags->sel_new_compute_pls_with_func = T1_objc_reg_sel(
        "newComputePipelineStateWithFunction:error:");
    T1_log_assert(ags->sel_new_compute_pls_with_func != NULL);
    ags->sel_new_tex_with_desc =
        T1_objc_reg_sel("newTextureWithDescriptor:");
    T1_log_assert(ags->sel_new_tex_with_desc != NULL);
    ags->sel_new_buf_with_len_options =
        T1_objc_reg_sel("newBufferWithLength:options:");
    ags->sel_new_render_pls_with_descriptor_error =
        T1_objc_reg_sel(
            "newRenderPipelineStateWithDescriptor:error:");
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
    ags->sel_new_texture_view_with_pixel_format =
        T1_objc_reg_sel("newTextureViewWithPixelFormat:textureType:levels:slices:");
    ags->sel_set_array_length =
        T1_objc_reg_sel("setArrayLength:");
    ags->sel_set_pixel_format =
        T1_objc_reg_sel("setPixelFormat:");
    ags->sel_set_blending_enabled =
        T1_objc_reg_sel("setBlendingEnabled:");
    ags->sel_set_source_rgb_blend_factor =
        T1_objc_reg_sel("setSourceRGBBlendFactor:");
    ags->sel_set_dest_rgb_blend_factor =
        T1_objc_reg_sel("setDestinationRGBBlendFactor:");
    ags->sel_set_rgb_blend_operation =
        T1_objc_reg_sel("setRgbBlendOperation:");
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
        T1_objc_reg_sel(
            "newDepthStencilStateWithDescriptor:");
    T1_log_assert(ags->sel_new_depth_stencil_state_with_desc != NULL);
    ags->sel_blit_command_encoder =
        T1_objc_reg_sel("blitCommandEncoder");
    ags->sel_copy_from_buffer_source_offset_to_buffer =
        T1_objc_reg_sel("copyFromBuffer:sourceOffset:toBuffer:destinationOffset:size:");
    ags->sel_copy_from_buffer_source_offset_source_bytes_per_row =
        T1_objc_reg_sel("copyFromBuffer:sourceOffset:sourceBytesPerRow:sourceBytesPerImage:sourceSize:toTexture:destinationSlice:destinationLevel:destinationOrigin:");
    ags->sel_generate_mipmaps_for_texture =
        T1_objc_reg_sel("generateMipmapsForTexture:");
    ags->sel_present_drawable = T1_objc_reg_sel(
        "presentDrawable:");
    ags->sel_commit =
        T1_objc_reg_sel("commit");
    ags->sel_wait_until_completed =
        T1_objc_reg_sel("waitUntilCompleted");
    ags->sel_end_encoding =
        T1_objc_reg_sel("endEncoding");
    ags->sel_current_drawable = T1_objc_reg_sel(
        "currentDrawable");
    ags->sel_copy_from_texture_to_texture =
        T1_objc_reg_sel("copyFromTexture:toTexture:");
    ags->sel_copy_from_texture_to_buffer =
        T1_objc_reg_sel(
            "copyFromTexture:sourceSlice:sourceLevel:sourceOrigin:sourceSize:toBuffer:destinationOffset:destinationBytesPerRow:destinationBytesPerImage:");
    ags->sel_new_buffer_with_bytes_no_copy =
        T1_objc_reg_sel(
            "newBufferWithBytesNoCopy:length:options:deallocator:");
    ags->sel_new_command_queue =
        T1_objc_reg_sel("newCommandQueue");
    ags->sel_vertex_buffers =
        T1_objc_reg_sel("vertexBuffers");
    ags->sel_new_default_library =
        T1_objc_reg_sel("newDefaultLibrary");
    ags->sel_depth_attachment = T1_objc_reg_sel("depthAttachment"); 
    ags->sel_set_load_action = T1_objc_reg_sel("setLoadAction:");
    ags->sel_set_clear_depth = T1_objc_reg_sel("setClearDepth:");
    ags->sel_set_store_action = T1_objc_reg_sel("setStoreAction:");
    ags->sel_set_texture = T1_objc_reg_sel("setTexture:");
    ags->sel_set_depth_write_enabled = T1_objc_reg_sel("setDepthWriteEnabled:");
    ags->sel_set_depth_compare_function = T1_objc_reg_sel("setDepthCompareFunction:");
    ags->sel_current_render_pass_desc = T1_objc_reg_sel(
        "currentRenderPassDescriptor");
    ags->sel_file_URL_with_path_is_directory = T1_objc_reg_sel(
        "fileURLWithPath:isDirectory:");
    ags->sel_dispatch_threads_threadsperthreadgroup = T1_objc_reg_sel(
        "dispatchThreads:threadsPerThreadgroup:");
    funcptr_gameloop_before_render =
        arg_funcptr_shared_gameloop_update;
    funcptr_gameloop_after_render =
        arg_funcptr_shared_gameloop_update_after_render_pass;
    
    T1_objc_close_current_framework();
    
    ags->pixel_format_renderpass1 = T1MTLPixelFormatRGBA8Unorm; // 70
    
    ags->frame_i = 0;
    
    T1_std_strcpy_cap(
        error_msg_string,
        512,
        "");
    
    ags->device = with_metal_device;
    
    ags->lib = (void *)T1_objc_msg(
        ags->device,
        ags->sel_new_default_library);
    
    if (ags->lib == NULL)
    {        
        #if 1
        void * nsstring_shader_lib_fpath =
            T1_objc_nsstring_construct(shader_lib_filepath);
        
        void * shader_lib_url = (void *)T1_objc_msg_2arg(
            ags->class_nsurl,
            ags->sel_file_URL_with_path_is_directory,
            (uintptr_t)nsstring_shader_lib_fpath,
            false);
        #else
        NSURL * shader_lib_url = [NSURL
            fileURLWithPath: shader_lib_filepath
            isDirectory: false];
        #endif
        
        if (shader_lib_url == NULL) {
            T1_std_strcpy_cap(
                error_msg_string,
                512,
                "Failed to find the shader library");
            return false;
        }
        
        // newLibraryWithURL:error:
        ags->lib = (void *)T1_objc_msg_2arg(
            with_metal_device,
            ags->sel_new_library_with_URL_error,
            (uintptr_t)shader_lib_url,
            0);
        
        if (ags->lib == NULL) {
            T1_std_strcpy_cap(
                error_msg_string,
                512,
                "Failed to find shader library file");
            
            return false;
        }
    }
    
    void * nsstring_vert_shader = T1_objc_nsstring_construct(
        "vertex_shader");
    void * vertex_shader = (void *)T1_objc_msg_1arg(
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
    void * fragment_shader = (void *)T1_objc_msg_1arg(
        ags->lib,
        ags->sel_new_function_with_name,
        (uintptr_t)nsstring_fragment_shader);
    if (fragment_shader == NULL) {
        T1_std_strcpy_cap(
            error_msg_string,
            512,
            "Missing function: frag_shader()");
        return false;
    }
    
    #if T1_BLENDING_SHADER_ACTIVE == T1_ACTIVE
    void * nsstring_alphablending_fragment_shader = T1_objc_nsstring_construct(
        "alphablending_frag_shader");
    void * alphablending_fragment_shader = (void *)T1_objc_msg_1arg(
        ags->lib,
        ags->sel_new_function_with_name,
        (uintptr_t)nsstring_alphablending_fragment_shader);
    if (alphablending_fragment_shader == NULL) {
        T1_std_strcpy_cap(
            error_msg_string,
            512,
            "Missing function: alphablending_frag_shader()");
        return false;
    }
    #elif T1_BLENDING_SHADER_ACTIVE == T1_INACTIVE
    #else
    #error
    #endif
        
    #if T1_Z_PREPASS_ACTIVE == T1_ACTIVE
    void * nsstring_vertex_shader = T1_objc_nsstring_construct(
        "vertex_shader");
    void * z_prepass_vertex_shader = (void *)T1_objc_msg_1arg(
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
        (void *)T1_objc_msg_1arg(
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
    
    void * z_prepass_pls_desc = T1_gpu_new_render_pipeline_descriptor(
        z_prepass_vertex_shader,
        z_prepass_fragment_shader,
        /* has touch tex: */ false,
        /* blending_enabled: */ false);
    ags->z_prepass_pls = (void *)T1_objc_msg_2arg(
        ags->device,
        ags->sel_new_render_pls_with_descriptor_error,
        (uintptr_t)z_prepass_pls_desc,
        (uintptr_t)NULL);
    #elif T1_Z_PREPASS_ACTIVE == T1_INACTIVE
    #else
    #error
    #endif
    
    #if T1_OUTLINES_ACTIVE == T1_ACTIVE
    void * nsstring_outlines_vertex_shader =
        T1_objc_nsstring_construct("outlines_vertex_shader");
    void * outlines_vertex_shader = (void *)T1_objc_msg_1arg(
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
    void * outlines_fragment_shader =
        (void *)T1_objc_msg_1arg(
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
    
    void * outlines_pls_desc = (void *)T1_objc_msg(
        ags->class_mtl_render_pipeline_desc,
        ags->sel_new);
    T1_objc_msg_1arg(
        outlines_pls_desc,
        ags->sel_set_vertex_function,
        (uintptr_t)outlines_vertex_shader);
    T1_objc_msg_1arg(
        outlines_pls_desc,
        ags->sel_set_fragment_function,
        (uintptr_t)outlines_fragment_shader);
    {
        void * color_attachments = (void *)T1_objc_msg(
            outlines_pls_desc,
            ags->sel_color_attachments);
        void * color_attachment_0 = (void *)T1_objc_msg_1arg(
            color_attachments,
            ags->sel_object_at_indexed_subscript,
            0);
        T1_objc_msg_1arg(
            color_attachment_0,
            ags->sel_set_pixel_format,
            ags->pixel_format_renderpass1);
    }
    T1_objc_msg_1arg(
        outlines_pls_desc,
        ags->sel_set_depth_attachment_pixel_format,
        T1MTLPixelFormatDepth32Float);
    ags->outlines_pls = (void *)T1_objc_msg_2arg(
        ags->device,
        ags->sel_new_render_pls_with_descriptor_error,
        (uintptr_t)outlines_pls_desc,
        0);
    
    if (ags->outlines_pls == NULL) {
        return false;
    }
    #elif T1_OUTLINES_ACTIVE == T1_INACTIVE
    #else
    #error
    #endif
    
    void * nsstring_flat_billboard_quad_vert_shader =
        T1_objc_nsstring_construct(
            "flat_billboard_quad_vertex_shader");
    void * flat_billboard_quad_vert_shader = (void *)T1_objc_msg_1arg(
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
    void * flat_billboard_quad_frag_shader = (void *)T1_objc_msg_1arg(
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
    
    void * flat_billboard_quad_pls_desc =
        T1_gpu_new_render_pipeline_descriptor(
            flat_billboard_quad_vert_shader,
            flat_billboard_quad_frag_shader,
            /* has_touch_tex: */ true,
            /* blending_enabled: */ false);
    ags->bb_touch_pls = (void *)T1_objc_msg_2arg(
        ags->device,
        ags->sel_new_render_pls_with_descriptor_error,
        (uintptr_t)flat_billboard_quad_pls_desc,
        0);
    
    T1_log_assert(ags->bb_touch_pls != NULL);
    T1_gpu_render_pl_descriptor_remove_color_attachment_at(
        flat_billboard_quad_pls_desc,
        1);
    
    ags->bb_notouch_pls =
        (void *)T1_objc_msg_2arg(
            ags->device,
            ags->sel_new_render_pls_with_descriptor_error,
            (uintptr_t)flat_billboard_quad_pls_desc,
            0);
    
    void * nsstring_flat_texquad_vert_shader =
        T1_objc_nsstring_construct(
            "flat_texquad_vertex_shader");
    void * flat_texquad_vert_shader =
        (void *)T1_objc_msg_1arg(
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
    void * flat_texquad_frag_shader = (void *)T1_objc_msg_1arg(
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
    
    void * flat_texquad_pls_desc =
        T1_gpu_new_render_pipeline_descriptor(
            flat_texquad_vert_shader,
            flat_texquad_frag_shader,
            /* has_touch_tex: */ true,
            /* blending_enabled: */ true);
    ags->flat_texquad_touch_pls = (void *)T1_objc_msg_2arg(
        ags->device,
        ags->sel_new_render_pls_with_descriptor_error,
        (uintptr_t)flat_texquad_pls_desc,
        0);
    
    // Setup pipeline that uses diamonds instead of alphablending
    void * diamond_pls_desc =
        T1_gpu_new_render_pipeline_descriptor(
            vertex_shader,
            fragment_shader,
            /* u8 has_touch_tex: */ true,
            /* blending_enabled: */ false);
    ags->diamond_touch_pls =
        (void *)T1_objc_msg_2arg(
            ags->device,
            ags->sel_new_render_pls_with_descriptor_error,
            (uintptr_t)diamond_pls_desc,
            0);
    
    if (ags->diamond_touch_pls == NULL)
    {
        T1_std_strcpy_cap(
            error_msg_string,
            512,
            "Failed to init diamond pipeline");
        return false;
    }
    T1_gpu_render_pl_descriptor_remove_color_attachment_at(
        diamond_pls_desc,
        1);
    ags->diamond_notouch_pls = (void *)T1_objc_msg_2arg(
        ags->device,
        ags->sel_new_render_pls_with_descriptor_error,
        (uintptr_t)diamond_pls_desc,
        0);
    T1_log_assert(ags->diamond_notouch_pls != NULL);
    
    T1_gpu_render_pl_descriptor_remove_color_attachment_at(
        diamond_pls_desc,
        0);
    ags->depth_only_pls = (void *)T1_objc_msg_2arg(
        ags->device,
        ags->sel_new_render_pls_with_descriptor_error,
        (uintptr_t)diamond_pls_desc,
        0);
    
    #if T1_BLENDING_SHADER_ACTIVE == T1_ACTIVE
    void * alpha_pls_desc =
        T1_gpu_new_render_pipeline_descriptor(
            vertex_shader,
            alphablending_fragment_shader,
            true,
            /* blending_enabled: */ true);
    ags->blend_touch_pls = (void *)T1_objc_msg_2arg(
        ags->device,
        ags->sel_new_render_pls_with_descriptor_error,
        (uintptr_t)alpha_pls_desc,
        0);
    if (ags->blend_touch_pls == NULL)
    {
        T1_std_strcpy_cap(
            error_msg_string,
            512,
            "Failed to load the alphablending shader");
        return false;
    }
    
    T1_gpu_render_pl_descriptor_remove_color_attachment_at(
        alpha_pls_desc,
        1);
    ags->blend_notouch_pls =
        (void *)T1_objc_msg_2arg(
            ags->device,
            ags->sel_new_render_pls_with_descriptor_error,
            (uintptr_t)alpha_pls_desc,
            0);
    
    #elif T1_BLENDING_SHADER_ACTIVE == T1_INACTIVE
    #else
    #error
    #endif
    
    void * depth_desc = (void *)T1_objc_msg(
        ags->class_mtl_depth_stencil_desc,
        ags->sel_new);
    T1_objc_msg_1arg(depth_desc,
        ags->sel_set_depth_write_enabled, true); 
    T1_objc_msg_1arg(depth_desc,
        ags->sel_set_depth_compare_function,
        T1MTLCompareFunctionLessEqual);
    ags->opaque_depth_stencil_state = (void *)T1_objc_msg_1arg(
        ags->device,
        ags->sel_new_depth_stencil_state_with_desc,
        (uintptr_t)depth_desc);
    if (ags->opaque_depth_stencil_state == NULL)
    {
        T1_std_strcpy_cap(
            error_msg_string,
            512,
            "Failed to load the depth stencil state");
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
        ags->polygon_buffers[fram_i] = (void *)T1_objc_msg_4arg(
            ags->device,
            ags->sel_new_buffer_with_bytes_no_copy,
            (uintptr_t)f->zsprite_list->polygons,
            T1_cpu_to_gpu_data->polygons_alloc_size,
            T1MTLResourceStorageModeShared,
            (uintptr_t)NULL);
        T1_log_assert(ags->polygon_buffers[fram_i] != NULL);
        
        ags->matrix_buffers[fram_i] = (void *)T1_objc_msg_4arg(
            ags->device,
            ags->sel_new_buffer_with_bytes_no_copy,
            (uintptr_t)f->matrices,
            T1_cpu_to_gpu_data->matrices_alloc_size,
            T1MTLResourceStorageModeShared,
            (uintptr_t)NULL);
        
        ags->vertex_buffers[fram_i] = (void *)T1_objc_msg_4arg(
            ags->device,
            ags->sel_new_buffer_with_bytes_no_copy,
            (uintptr_t)f->verts,
            T1_cpu_to_gpu_data->vertices_alloc_size,
            T1MTLResourceStorageModeShared,
            (uintptr_t)NULL);
        
        ags->flat_quad_buffers[fram_i] = (void *)T1_objc_msg_4arg(
            ags->device,
            ags->sel_new_buffer_with_bytes_no_copy,
            (uintptr_t)f->flat_bb_quads,
            T1_cpu_to_gpu_data->flat_quads_alloc_size,
            T1MTLResourceStorageModeShared,
            (uintptr_t)NULL);
        
        ags->flat_texquad_buffers[fram_i] = (void *)T1_objc_msg_4arg(
            ags->device,
            ags->sel_new_buffer_with_bytes_no_copy,
            (uintptr_t)f->flat_tex_quads,
            T1_cpu_to_gpu_data->flat_texquads_alloc_size,
            T1MTLResourceStorageModeShared,
            (uintptr_t)NULL);
        
        ags->postprocessing_constants_buffers[fram_i] = (void *)T1_objc_msg_4arg(
            ags->device,
            ags->sel_new_buffer_with_bytes_no_copy,
            (uintptr_t)f->postproc_consts,
            T1_cpu_to_gpu_data->postprocessing_constants_alloc_size,
            T1MTLResourceStorageModeShared,
            (uintptr_t)NULL);
        
        ags->light_buffers[fram_i] = (void *)T1_objc_msg_4arg(
            ags->device,
            ags->sel_new_buffer_with_bytes_no_copy,
            (uintptr_t)f->lights,
            T1_cpu_to_gpu_data->lights_alloc_size,
            T1MTLResourceStorageModeShared | T1MTLResourceUsageRead,
            (uintptr_t)NULL);
        
        T1_log_assert(
            T1_cpu_to_gpu_data->
                render_views_alloc_size >=
                    (sizeof(T1GPURenderView) *
                        T1_RENDER_VIEW_CAP));
        ags->cam_buffers[fram_i] = (void *)T1_objc_msg_4arg(
            ags->device,
            ags->sel_new_buffer_with_bytes_no_copy,
            (uintptr_t)f->render_views,
            T1_cpu_to_gpu_data->render_views_alloc_size,
            T1MTLResourceStorageModeShared,
            (uintptr_t)NULL);
    }
    
    ags->locked_vertex_populator_buffer = (void *)T1_objc_msg_4arg(
        ags->device,
        ags->sel_new_buffer_with_bytes_no_copy,
        (uintptr_t)T1_cpu_to_gpu_data->locked_vertices,
        T1_cpu_to_gpu_data->locked_vertices_alloc_size,
        T1MTLResourceStorageModeShared,
        (uintptr_t)NULL);
    
    ags->locked_vertex_buffer = (void *)T1_objc_msg_2arg(
        ags->device,
        ags->sel_new_buf_with_len_options,
        T1_cpu_to_gpu_data->locked_vertices_alloc_size,
        T1MTLResourceStorageModePrivate);
    
    ags->locked_matf32_populator_buffer = (void *)T1_objc_msg_4arg(
        ags->device,
        ags->sel_new_buffer_with_bytes_no_copy,
        (uintptr_t)T1_cpu_to_gpu_data->const_mats_f32,
        T1_cpu_to_gpu_data->const_matsf32_alloc_size,
        T1MTLResourceStorageModeShared,
        (uintptr_t)NULL);
    
    ags->locked_matf32_buffer = (void *)T1_objc_msg_2arg(
        ags->device,
        ags->sel_new_buf_with_len_options,
        T1_cpu_to_gpu_data->const_matsf32_alloc_size,
        T1MTLResourceStorageModePrivate);
    
    ags->locked_mats32_populator_buffer = (void *)T1_objc_msg_4arg(
        ags->device,
        ags->sel_new_buffer_with_bytes_no_copy,
        (uintptr_t)T1_cpu_to_gpu_data->const_mats_s32,
        T1_cpu_to_gpu_data->const_matss32_alloc_size,
        T1MTLResourceStorageModeShared,
        (uintptr_t)NULL);
    
    ags->locked_mats32_buffer = (void *)T1_objc_msg_2arg(
        ags->device,
        ags->sel_new_buf_with_len_options,
        T1_cpu_to_gpu_data->const_matss32_alloc_size,
        T1MTLResourceStorageModePrivate);
    
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
    void * downsample_func = (void *)T1_objc_msg_1arg(
        ags->lib,
        ags->sel_new_function_with_name,
        (uintptr_t)nsstring_downsample_texture);
    
    ags->downsample_compute_pls = (void *)T1_objc_msg_2arg(
        ags->device,
        ags->sel_new_compute_pls_with_func,
        (uintptr_t)downsample_func,
        (uintptr_t)NULL);
    
    void * nsstring_boxblur_texture = T1_objc_nsstring_construct(
        "boxblur_texture");
    void * boxblur_texture = (void *)T1_objc_msg_1arg(
        ags->lib,
        ags->sel_new_function_with_name,
        (uintptr_t)nsstring_boxblur_texture);
    void * error = NULL;
    ags->boxblur_compute_pls =
        (void *)T1_objc_msg_2arg(
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
    void * singlequad_vertex_shader = (void *)T1_objc_msg_1arg(
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
    void * singlequad_fragment_shader = (void *)T1_objc_msg_1arg(
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
    
    void * singlequad_pipeline_descriptor =
        T1_gpu_new_render_pipeline_descriptor(
            singlequad_vertex_shader,
            singlequad_fragment_shader,
            /* touch texture: */ false,
            /* blending_enabled: */ true);
    {
        // Match the drawable's pixelformat with BGRA
        void * color_attachments = (void *)T1_objc_msg(
            singlequad_pipeline_descriptor,
            ags->sel_color_attachments);
        void * color_attachment_0 = (void *)T1_objc_msg_1arg(
            color_attachments,
            ags->sel_object_at_indexed_subscript,
            0);
        T1_objc_msg_1arg(
            color_attachment_0,
            ags->sel_set_pixel_format,
            T1MTLPixelFormatBGRA8Unorm);
    }
    void * vert_buffers = (void *)T1_objc_msg(
        singlequad_pipeline_descriptor,
        ags->sel_vertex_buffers);
    vert_buffers = (void *)T1_objc_msg_1arg(
        vert_buffers,
        ags->sel_object_at_indexed_subscript,
        0);
    T1_objc_msg_1arg(
        vert_buffers,
        ags->sel_set_mutability,
        T1MTLMutabilityImmutable);
    
    #if 0
    [0].
        mutability = MTLMutabilityImmutable;
    #endif
    
    #if 0
    // These settings may be slightly different than our
    // combined path, here are the original settings for
    // reference if something goes wrong
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
    #endif
    ags->singlequad_pls = (void *)T1_objc_msg_2arg(
        ags->device,
        ags->sel_new_render_pls_with_descriptor_error,
        (uintptr_t)singlequad_pipeline_descriptor,
        (uintptr_t)NULL);
    
    ags->command_queue = (void *)T1_objc_msg(
        ags->device,
        ags->sel_new_command_queue);
    
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
    (void)recipient;
    (void)recipient_cap;
    
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
    void * prev_copy = NULL;
    
    if (T1_tex_arrays[tex_array_i].deleted) {
        ags->metal_textures[tex_array_i] = NULL;
        return;
    } else if (
        ags->metal_textures[tex_array_i] == NULL ||
        (T1_tex_arrays[tex_array_i].bc1_compressed &&
            T1_objc_msg(
                ags->metal_textures[tex_array_i],
                ags->sel_pixel_format) !=
                    T1MTLPixelFormatBC1_RGBA) ||
        ((!T1_tex_arrays[tex_array_i].bc1_compressed) &&
            T1_objc_msg(
                ags->metal_textures[tex_array_i],
                ags->sel_pixel_format) !=
                    T1MTLPixelFormatRGBA8Unorm) ||
            T1_tex_arrays[tex_array_i].single_img_width !=
                T1_objc_msg(
                    ags->metal_textures[tex_array_i],
                    ags->sel_width) ||
            T1_tex_arrays[tex_array_i].single_img_height !=
                T1_objc_msg(
                    ags->metal_textures[tex_array_i],
                    ags->sel_height))
    {
        ags->metal_textures[tex_array_i] = NULL;
        copy_prev = false;
    } else if (
        T1_tex_arrays[tex_array_i].images_size >
            T1_objc_msg(
                ags->metal_textures[tex_array_i],
                ags->sel_array_length))
    {
        #if T1_LOG_ASSERTS_ACTIVE == T1_ACTIVE
        T1_log_assert(
            T1_tex_arrays[tex_array_i].single_img_width ==
                T1_objc_msg(
                    ags->metal_textures[tex_array_i],
                    ags->sel_width));
        T1_log_assert(
            T1_tex_arrays[tex_array_i].single_img_height ==
                T1_objc_msg(ags->metal_textures[tex_array_i], ags->sel_height));
        T1_log_assert(
            T1_tex_arrays[tex_array_i].is_render_target == (
                T1_objc_msg(
                    ags->metal_textures[tex_array_i],
                    ags->sel_usage) &
                T1MTLTextureUsageRenderTarget) > 0);
        if (T1_tex_arrays[tex_array_i].bc1_compressed) {
            T1_log_assert(
                T1_objc_msg(
                    ags->metal_textures[tex_array_i],
                    ags->sel_pixel_format) ==
                        T1MTLPixelFormatBC1_RGBA);
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
                T1_objc_msg(
                    ags->metal_textures[tex_array_i],
                    ags->sel_usage) &
                T1MTLTextureUsageRenderTarget) > 0));
        return;
    }
    
    void * texture_descriptor =
        (void *)T1_objc_msg(ags->class_mtl_texture_desc, ags->sel_new);
    T1_objc_msg_1arg(
        texture_descriptor,
        ags->sel_set_texture_type,
        T1MTLTextureType2DArray);
    T1_objc_msg_1arg(
        texture_descriptor,
        ags->sel_set_array_length,
        T1_tex_arrays[tex_array_i].images_size);
    T1_objc_msg_1arg(
        texture_descriptor,
        ags->sel_set_pixel_format,
        T1_tex_arrays[tex_array_i].bc1_compressed ?
            T1MTLPixelFormatBC1_RGBA :
            T1MTLPixelFormatRGBA8Unorm);
    T1_objc_msg_1arg(
        texture_descriptor,
        ags->sel_set_storage_mode,
        T1MTLStorageModePrivate);
    
    if (T1_tex_arrays[tex_array_i].is_render_target)
    {
        T1_objc_msg_1arg(
            texture_descriptor,
            ags->sel_set_usage,
            T1MTLTextureUsageShaderRead |
                T1MTLTextureUsageRenderTarget);
    } else {
        T1_objc_msg_1arg(
            texture_descriptor,
            ags->sel_set_usage,
            T1MTLTextureUsageShaderRead);
    }
    T1_objc_msg_1arg(
        texture_descriptor,
        ags->sel_set_width,
        T1_tex_arrays[tex_array_i].single_img_width);
    T1_objc_msg_1arg(
        texture_descriptor,
        ags->sel_set_height,
        T1_tex_arrays[tex_array_i].single_img_height);
    
    #if T1_MIPMAPS_ACTIVE == T1_ACTIVE
    T1_objc_msg_1arg(
        texture_descriptor,
        ags->sel_set_mipmap_level_count,
        T1_tex_arrays[tex_array_i].bc1_compressed || tex_array_i == 0 ?
        1 :
        (uintptr_t)floor(
            log2((double)MAX(
                T1_tex_arrays[tex_array_i].single_img_width,
                T1_tex_arrays[tex_array_i].single_img_height))) + 1);
    #elif T1_MIPMAPS_ACTIVE == T1_INACTIVE
    T1_objc_msg_1arg(
        texture_descriptor,
        ags->sel_set_mipmap_level_count,
        1);
    #else
    #error
    #endif
    
    T1_log_assert(ags->metal_textures[tex_array_i] == NULL);
    ags->metal_textures[tex_array_i] = (void *)T1_objc_msg_1arg(
        ags->device,
        ags->sel_new_tex_with_desc,
        (uintptr_t)texture_descriptor);
    T1_log_assert(ags->metal_textures[tex_array_i] != NULL);
    
    T1_tex_arrays[tex_array_i].gpu_capacity =
        T1_tex_arrays[tex_array_i].images_size;
    
    if (copy_prev) {
        void * combuf = (void *)T1_objc_msg(
            ags->command_queue,
            ags->sel_command_buffer);
        void * blitenc = (void *)T1_objc_msg(
            combuf,
            ags->sel_blit_command_encoder);
        
        T1_objc_msg_2arg(
            blitenc,
            ags->sel_copy_from_texture_to_texture,
            (uintptr_t)prev_copy,
            (uintptr_t)ags->metal_textures[tex_array_i]);
        T1_objc_msg(blitenc, ags->sel_end_encoding);
        
        T1_objc_msg(combuf, ags->sel_commit);
        T1_objc_msg(combuf, ags->sel_wait_until_completed);
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
    
    u8 * data = (u8 *)T1_objc_msg(
        ags->touch_id_buffer, ags->sel_contents);
    u64 size = T1_objc_msg(
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
    
    ags->metal_textures[array_i] = NULL;
}

void T1_os_gpu_delete_depth_tex(
    const s32 slice_i)
{
    T1_log_assert(slice_i < T1_RENDER_VIEW_CAP);
    
    ags->depth_textures[slice_i] = NULL;
}

#if T1_TEXTURES_ACTIVE == T1_ACTIVE
void T1_os_gpu_fetch_rgba_at(
    s32 texture_array_i,
    s32 texture_i,
    u8 * rgba_recipient,
    u32 * recipient_size,
    u32 * recipient_width,
    u32 * recipient_height,
    u32 recipient_cap,
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
        T1_objc_msg(
            texture,
            ags->sel_texture_type) !=
                T1MTLTextureType2DArray)
    {
        return;
    }
    
    if (T1_objc_msg(texture, ags->sel_pixel_format) != T1MTLPixelFormatRGBA8Unorm)
    {
        // Ensure the texture format is RGBA8Unorm
        // for direct copying to u8 RGBA
        return;
    }
    
    *recipient_width = (u32)T1_objc_msg(texture, ags->sel_width);
    *recipient_height = (u32)T1_objc_msg(texture, ags->sel_height);
    if (*recipient_width < 1 || *recipient_height < 1) {
        return;
    }
    
    // Calculate required buffer size
    uintptr_t bytes_per_row = T1_objc_msg(
        texture,
        ags->sel_width) * 4; // 4 bytes per pixel (RGBA)
    uintptr_t bytes_per_image =
        bytes_per_row *
        T1_objc_msg(texture, ags->sel_height);
    if (recipient_cap < bytes_per_image) {
        *recipient_size = 0;
        return;
    }
    
    // Create a temporary buffer for the copy
    // T1_objc_msg_2arg(ags->device, ags->objc_sel_new_buf_with_len_options, bytes_per_image, MTLResourceStorageModeShared);
    void * temp_buffer = (void *)T1_objc_msg_2arg(
        ags->device,
        ags->sel_new_buf_with_len_options,
        bytes_per_image,
        T1MTLResourceStorageModeShared);
    if (temp_buffer == NULL) {
        return;
    }
    T1_log_assert(temp_buffer != NULL);
    
    if (
        texture_i >= (s32)T1_objc_msg(
            texture,
            ags->sel_array_length))
    {
        return;
    }
    
    // Create command buffer and blit encoder
    void * command_buffer = (void *)T1_objc_msg(
        ags->command_queue,
        ags->sel_command_buffer);
    void * blit_encoder = (void *)T1_objc_msg(
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
            T1_objc_msg(texture, ags->sel_width),
            T1_objc_msg(texture, ags->sel_height),
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
            T1_objc_msg(texture, ags->sel_width),
            T1_objc_msg(texture, ags->sel_height),
            1),
        temp_buffer,
        0,
        bytes_per_row,
        bytes_per_image);
    
    T1_objc_msg(
        blit_encoder,
        ags->sel_end_encoding);
    
    // Commit the command buffer
    T1_objc_msg(command_buffer, ags->sel_commit);
    T1_objc_msg(command_buffer, ags->sel_wait_until_completed);
    
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
    
    void * combuf = (void *)T1_objc_msg(
        ags->command_queue,
        ags->sel_command_buffer);
    
    // Create a blit command encoder
    void * blit_mipmap_encoder = (void *)T1_objc_msg(
        combuf,
        ags->sel_blit_command_encoder);
    
    T1_objc_msg_1arg(
        blit_mipmap_encoder,
        ags->sel_generate_mipmaps_for_texture,
        (uintptr_t)ags->metal_textures[texture_array_i]);
    
    T1_objc_msg(
        blit_mipmap_encoder,
        ags->sel_end_encoding);
    
    T1_objc_msg(combuf, ags->sel_commit);
    T1_objc_msg(combuf, ags->sel_wait_until_completed);
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
    
    void * temp_source_buf = (void *)T1_objc_msg_4arg(
        ags->device,
        ags->sel_new_buffer_with_bytes_no_copy,
        (uintptr_t)temp_src_buf_ptr,
        temp_buf_cap,
        T1MTLResourceStorageModeShared,
        (uintptr_t)NULL);
    
    if (temp_source_buf == NULL) {
        T1_log_assert(0);
        return;
    }
    
    void * combuf = (void *)T1_objc_msg(
        ags->command_queue,
        ags->sel_command_buffer);
    
    void * blit_copy_encoder = (void *)T1_objc_msg(
        combuf,
        ags->sel_blit_command_encoder);
    
    T1ObjcSet size = T1_objc_set_make(img_width, img_height, 1);
    T1ObjcSet origin = T1_objc_set_make(0, 0, 0);
    
    T1_objc_msg_4arg_1set_3arg_1set(
        blit_copy_encoder,
        ags->sel_copy_from_buffer_source_offset_source_bytes_per_row,
        (uintptr_t)temp_source_buf,
        0,
        T1_tex_arrays[tex_array_i].bc1_compressed ?
                ((img_width + 3) / 4) * 8 :
                img_width * 4,
        T1_tex_arrays[tex_array_i].bc1_compressed ?
                ((img_width + 3) / 4) * ((img_height + 3) / 4) * 8 :
                img_width * img_height * 4,
        size,
        (uintptr_t)(ags->metal_textures[tex_array_i]),
        (uintptr_t)tex_slice_i,
        0,
        origin);
    
    T1_objc_msg(blit_copy_encoder, ags->sel_end_encoding);
    
    T1_objc_msg(combuf, ags->sel_commit);
    T1_objc_msg(combuf, ags->sel_wait_until_completed);
    
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
    
    void * combuf = (void *)T1_objc_msg(
        ags->command_queue,
        ags->sel_command_buffer);
    
    void * blit_copy_encoder = (void *)T1_objc_msg(
        combuf,
        ags->sel_blit_command_encoder);
    
    T1_objc_msg_5arg(
        blit_copy_encoder,
        ags->sel_copy_from_buffer_source_offset_to_buffer,
        (uintptr_t)ags->locked_vertex_populator_buffer,
        0,
        (uintptr_t)ags->locked_vertex_buffer,
        0,
        T1_cpu_to_gpu_data->locked_vertices_alloc_size);
    
    T1_objc_msg(blit_copy_encoder, ags->sel_end_encoding);
    
    T1_objc_msg(combuf, ags->sel_commit);
}

void T1_os_gpu_copy_locked_materials(void)
{
    T1_cpu_to_gpu_data->const_mats_size =
        all_mesh_materials->size;
    
    void * combuf = (void *)T1_objc_msg(
        ags->command_queue,
        ags->sel_command_buffer);
    
    void * blit_copy_encoder = (void *)T1_objc_msg(
        combuf,
        ags->sel_blit_command_encoder);
    
    T1_objc_msg_5arg(
        blit_copy_encoder,
        ags->sel_copy_from_buffer_source_offset_to_buffer,
        (uintptr_t)ags->locked_matf32_populator_buffer,
        0,
        (uintptr_t)ags->locked_matf32_buffer,
        0,
        T1_cpu_to_gpu_data->const_matsf32_alloc_size);
    
    T1_objc_msg_5arg(
        blit_copy_encoder,
        ags->sel_copy_from_buffer_source_offset_to_buffer,
        (uintptr_t)ags->locked_mats32_populator_buffer,
        0,
        (uintptr_t)ags->locked_mats32_buffer,
        0,
        T1_cpu_to_gpu_data->const_matss32_alloc_size);
    
    T1_objc_msg(
        blit_copy_encoder,
        ags->sel_end_encoding);
    
    T1_objc_msg(
        combuf,
        ags->sel_commit);
}

static void * get_tex_slice(
    s32 at_array_i,
    s32 at_slice_i)
{
    T1_log_assert(at_array_i >= 0);
    T1_log_assert(at_array_i < T1_TEXARRAYS_CAP);
    T1_log_assert(at_slice_i >= 0);
    
    void * parent = ags->metal_textures[at_array_i];
    
    T1ObjcPair ns_range_level = T1_objc_pair_make(
        0,
        T1_objc_msg(
            parent,
            ags->sel_mipmap_level_count));
    T1ObjcPair ns_range_slice = T1_objc_pair_make(
        (uintptr_t)at_slice_i,
        1);
    
    void * retval = (void *)T1_objc_msg_2arg_2pair(
        parent,
        ags->sel_new_texture_view_with_pixel_format,
        T1_objc_msg(parent, ags->sel_pixel_format),
        T1MTLTextureType2D,
        ns_range_level,
        ns_range_slice);
    
    return retval;
}

static void set_defaults_for_render_descriptor(
    void * desc,
    s32 cam_i)
{
    void * depth_attachm = (void *)T1_objc_msg(desc, ags->sel_depth_attachment);
    if (!ags->zbuf_cleared) {
        T1_objc_msg_1arg(depth_attachm, ags->sel_set_load_action, T1MTLLoadActionClear);
        T1_objc_msg_f64(depth_attachm, ags->sel_set_clear_depth, 1.0);
        
        ags->zbuf_cleared = true;
    } else {
        T1_objc_msg_1arg(depth_attachm, ags->sel_set_load_action, T1MTLLoadActionLoad);
    }
    T1_log_assert(ags->cur_depth != NULL);
    
    T1_objc_msg_1arg(depth_attachm, ags->sel_set_store_action, T1MTLStoreActionStore);
    T1_objc_msg_1arg(depth_attachm, ags->sel_set_texture, (uintptr_t)ags->cur_depth);
    
    void * color_attachments = (void *)T1_objc_msg(
        desc, ags->sel_color_attachments);
    void * color_attachment_0 = (void *)T1_objc_msg_1arg(
        color_attachments, ags->sel_object_at_indexed_subscript, 0);
    if (!ags->rtt_cleared) {
        T1_objc_msg_1arg(
            color_attachment_0,
            ags->sel_set_load_action,
            T1MTLLoadActionClear);
        
        T1_objc_msg_1quadf64(
            color_attachment_0,
            ags->sel_set_clear_color,
            T1_objc_quadf64_make(0.0f, 0.0f, 0.1f, 1.0f));
        
        ags->rtt_cleared = true;
    } else {
        T1_objc_msg_1arg(
            color_attachment_0,
            ags->sel_set_load_action,
            T1MTLLoadActionLoad);
    }
    
    T1_objc_msg_1arg(
        color_attachment_0,
        ags->sel_set_texture,
        (uintptr_t)ags->cur_rtt);
    T1_objc_msg_1arg(
        color_attachment_0,
        ags->sel_set_store_action,
        T1MTLStoreActionStore);
    
    // ID Buffer for touchables
    if (
        T1_render_views->cpu[cam_i].write_type ==
            T1RENDERVIEW_WRITE_RENDER_TARGET)
    {
        void * color_attachment_1 = (void *)T1_objc_msg_1arg(
            color_attachments, ags->sel_object_at_indexed_subscript, 1);
        T1_objc_msg_1arg(
            color_attachment_1,
            ags->sel_set_texture,
            (uintptr_t)ags->touch_id_texture);
        T1_objc_msg_1arg(
            color_attachment_1,
            ags->sel_set_load_action,
            (uintptr_t)T1MTLLoadActionLoad);
        T1_objc_msg_1arg(
            color_attachment_1,
            ags->sel_set_store_action,
            (uintptr_t)T1MTLStoreActionStore);
    }
}

static void set_defaults_for_encoder(
    void * encoder,
    const u32 cam_i)
{
    T1_log_assert(cam_i < T1_RENDER_VIEW_CAP);
    T1_log_assert(ags->opaque_depth_stencil_state != NULL);
    
    T1_objc_msg_1arg(encoder,
        ags->sel_set_depth_stencil_state,
            (uintptr_t)ags->opaque_depth_stencil_state);
    T1_objc_msg_1arg(encoder,
        ags->sel_set_depth_clip_mode, T1MTLDepthClipModeClip);
    T1_objc_msg_1arg(encoder,
        ags->sel_set_cull_mode, T1MTLCullModeBack);
    T1_objc_msg_1arg(encoder,
        ags->sel_set_front_facing_winding,
            T1MTLWindingCounterClockwise);
    T1_objc_msg_1sextf64(encoder,
        ags->sel_set_viewport,
            *(T1ObjcSextf64 *)(ags->render_viewports + cam_i));
    T1_objc_msg_3arg(encoder,
        ags->sel_set_vertex_buffer_offset_atindex,
            /* setVertexBuffer: */
                (uintptr_t)ags->vertex_buffers[ags->frame_i],
            /* offset: */ 0,
            /* atIndex: */ 0);
    T1_objc_msg_3arg(encoder,
        ags->sel_set_vertex_buffer_offset_atindex,
            /* setVertexBuffer: */
                (uintptr_t)ags->polygon_buffers[ags->frame_i],
            /* offset: */ 0,
            /* atIndex: */ 1);
    T1_objc_msg_3arg(encoder,
        ags->sel_set_vertex_buffer_offset_atindex,
            /* setVertexBuffer: */
                (uintptr_t)ags->matrix_buffers[ags->frame_i],
            /* offset: */ 0,
            /* atIndex: */ 2);
    T1_objc_msg_3arg(encoder,
        ags->sel_set_vertex_buffer_offset_atindex,
            /* setVertexBuffer: */
                (uintptr_t)ags->cam_buffers[ags->frame_i],
            /* offset: */ 0,
            /* atIndex: */ 3);
    T1_objc_msg_3arg(encoder,
        ags->sel_set_vertex_bytes_length_atindex,
            /* setVertexBytes: */ (uintptr_t)(void *)&cam_i,
            /* offset: */ sizeof(u32),
            /* atIndex: */ 4);
    T1_objc_msg_3arg(encoder,
        ags->sel_set_vertex_buffer_offset_atindex,
            /* setVertexBuffer: */
                (uintptr_t)ags->locked_vertex_buffer,
            /* offset: */ 0,
            /* atIndex: */ 5);
    
    T1_objc_msg_3arg(encoder,
        ags->sel_set_fragment_buffer_offset_atindex,
            /* setFragmentBuffer: */
                (uintptr_t)ags->locked_vertex_buffer,
            /* offset: */ 0,
            /* atIndex: */ 0);
    
    T1_objc_msg_3arg(encoder,
        ags->sel_set_fragment_buffer_offset_atindex,
            /* setFragmentBuffer: */
                (uintptr_t)ags->polygon_buffers[ags->frame_i],
            /* offset: */ 0,
            /* atIndex: */ 1);
    
    T1_objc_msg_3arg(encoder,
        ags->sel_set_fragment_buffer_offset_atindex,
            /* setFragmentBuffer: */
                (uintptr_t)ags->light_buffers[ags->frame_i],
            /* offset: */
                0,
            /* atIndex: */
                2);
    
    T1_objc_msg_3arg(encoder,
        ags->sel_set_fragment_buffer_offset_atindex,
            /* setFragmentBuffer: */
                (uintptr_t)ags->cam_buffers[ags->frame_i],
            /* offset: */
                0,
            /* atIndex: */
                3);
    
    T1_objc_msg_3arg(encoder,
        ags->sel_set_fragment_bytes_length_atindex,
            /* setFragmentBytes: */
                (uintptr_t)(void *)&cam_i,
            /* length: */
                sizeof(u32),
            /* atIndex: */
                4);
    
    T1_objc_msg_3arg(encoder,
        ags->sel_set_fragment_buffer_offset_atindex,
            /* setFragmentBuffer: */
                (uintptr_t)ags->locked_matf32_buffer,
            /* offset: */
                0,
            /* atIndex: */
                6);
    
    T1_objc_msg_3arg(encoder,
        ags->sel_set_fragment_buffer_offset_atindex,
            /* setFragmentBuffer: */
                (uintptr_t)ags->
                    postprocessing_constants_buffers[ags->frame_i],
            /* offset: */
                0,
            /* atIndex: */
                7);
    
    T1_objc_msg_3arg(encoder,
        ags->sel_set_fragment_buffer_offset_atindex,
            /* setFragmentBuffer: */
                (uintptr_t)ags->locked_mats32_buffer,
            /* offset: */
                0,
            /* atIndex: */
                8);
    
    #if T1_TEXTURES_ACTIVE == T1_ACTIVE
    for (
        u32 i = 0;
        i < T1_TEXARRAYS_CAP;
        i++)
    {
        if (ags->metal_textures[i] != NULL) {
            T1_objc_msg_2arg(encoder,
                ags->sel_set_fragment_texture_at_index,
                (uintptr_t)ags->metal_textures[i],
                i);
        }
    }
    #elif T1_TEXTURES_ACTIVE == T1_INACTIVE
    T1_objc_msg_2arg(encoder,
        ags->sel_set_fragment_texture_at_index,
        (uintptr_t)ags->metal_textures[0],
        0);
    #else
    #error
    #endif
    
    #if T1_SHADOWS_ACTIVE == T1_ACTIVE
    for (
        u32 rv_i = 0;
        rv_i < T1_RENDER_VIEW_CAP;
        rv_i++)
    {
        T1_objc_msg_2arg(encoder,
            ags->sel_set_fragment_texture_at_index,
            (uintptr_t)ags->depth_textures[rv_i],
            T1_SHADOW_MAPS_1ST_FRAGARG_I + rv_i);
    }
    #elif T1_SHADOWS_ACTIVE == T1_INACTIVE
    #else
    #error
    #endif
}

static void T1_gpu_draw_single_pass(
    s32 pass_i,
    s32 cam_i,
    void * view,
    void * combuf)
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
            
            void * outlines_desc = (void *)T1_objc_msg(
                view,
                ags->sel_current_render_pass_desc);
            
            set_defaults_for_render_descriptor(
                outlines_desc,
                cam_i);
            
            {
                void * color_attachments = (void *)T1_objc_msg(
                    outlines_desc,
                    ags->sel_color_attachments);
                void * color_att_1 = (void *)T1_objc_msg_1arg(
                    color_attachments, ags->sel_object_at_indexed_subscript,
                    /* object_at_indexed_subscript: */ 1);
                T1_objc_msg_1arg(
                    color_att_1,
                    ags->sel_set_texture,
                    (uintptr_t)NULL);
            }
            
            void * pass_1_outline_enc =
                (void *)T1_objc_msg_1arg(
                    combuf,
                    ags->sel_render_cmd_enc_with_desc,
                    (uintptr_t)outlines_desc);
            
            set_defaults_for_encoder(
                pass_1_outline_enc,
                (u32)cam_i);
            
            T1_objc_msg_1bigstructarg(
                pass_1_outline_enc,
                ags->sel_set_viewport,
                &ags->render_viewports[cam_i]);
            
            // outlines pipeline
            T1_objc_msg_1arg(
                pass_1_outline_enc,
                ags->sel_set_render_pls,
                (uintptr_t)ags->outlines_pls);
            T1_objc_msg_1arg(
                pass_1_outline_enc,
                ags->sel_set_depth_stencil_state,
                (uintptr_t)ags->opaque_depth_stencil_state);
            T1_objc_msg_1arg(
                pass_1_outline_enc,
                ags->sel_set_depth_clip_mode,
                T1MTLDepthClipModeClip);
            T1_objc_msg_1arg(
                pass_1_outline_enc,
                ags->sel_set_cull_mode,
                T1MTLCullModeFront);
            
            if (
                T1_global->draw_triangles &&
                pass->verts_size > 0)
            {
                T1_objc_msg_3arg(
                    pass_1_outline_enc,
                    ags->sel_draw_primitives_vertex_start_vertex_count,
                    T1MTLPrimitiveTypeTriangle,
                    (uintptr_t)pass->vert_i,
                    (uintptr_t)pass->verts_size);
            }
            T1_objc_msg(
                pass_1_outline_enc,
                ags->sel_end_encoding);
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
            void * diamond_desc = (void *)T1_objc_msg(
                view,
                ags->sel_current_render_pass_desc);
            
            set_defaults_for_render_descriptor(
                diamond_desc,
                cam_i);
            
            void * pass_2_opaque_tris_enc =
                (void *)T1_objc_msg_1arg(
                    combuf, ags->sel_render_cmd_enc_with_desc,
                    (uintptr_t)diamond_desc);
            
            T1_objc_msg_1arg(
                pass_2_opaque_tris_enc,
                ags->sel_set_render_pls,
                (uintptr_t)ags->cur_opq_pls);
            
            set_defaults_for_encoder(
                pass_2_opaque_tris_enc,
                (u32)cam_i);
            
            T1_log_assert(
                (pass->verts_size + pass->vert_i) <
                    T1_MAX_VERTS_PER_BUFFER);
            T1_log_assert(pass->verts_size % 3 == 0);
            
            T1_objc_msg_3arg(
                pass_2_opaque_tris_enc,
                ags->sel_draw_primitives_vertex_start_vertex_count,
                /* drawPrimitives: */ T1MTLPrimitiveTypeTriangle,
                /* vertexStart   : */ (uintptr_t)pass->vert_i,
                /* vertexCount   : */ (uintptr_t)pass->verts_size);
            
            T1_objc_msg(pass_2_opaque_tris_enc, ags->sel_end_encoding);
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
            
            void * alpha_desc = (void *)T1_objc_msg(
                view, ags->sel_current_render_pass_desc);
            
            set_defaults_for_render_descriptor(
                alpha_desc,
                cam_i);
            
            void * alpha_pass = (void *)T1_objc_msg_1arg(
                combuf, ags->sel_render_cmd_enc_with_desc,
                (uintptr_t)alpha_desc);
            
            T1_objc_msg_1arg(
                alpha_pass, ags->sel_set_render_pls,
                (uintptr_t)ags->cur_blnd_pls);
            
            set_defaults_for_encoder(
                alpha_pass, (u32)cam_i);
            
            T1_objc_msg_1arg(alpha_pass,
                ags->sel_set_cull_mode, T1MTLCullModeBack);
            T1_objc_msg_1arg(alpha_pass,
                ags->sel_set_front_facing_winding,
                T1MTLWindingCounterClockwise);
            
            T1_objc_msg_3arg(
                alpha_pass, ags->sel_draw_primitives_vertex_start_vertex_count,
                /* drawPrimitives: */ T1MTLPrimitiveTypeTriangle,
                /* vertexStart   : */ (uintptr_t)pass->vert_i,
                /* vertexCount   : */ (uintptr_t)pass->verts_size);
            
            T1_objc_msg(alpha_pass, ags->sel_end_encoding);
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
                ags->cur_bb_pls == NULL)
            {
                break;
            }
            
            void * bb_desc = (void *)T1_objc_msg(
                view, ags->sel_current_render_pass_desc);
            
            set_defaults_for_render_descriptor(
                bb_desc,
                cam_i);
            
            void * bb_enc = (void *)T1_objc_msg_1arg(
                combuf, ags->sel_render_cmd_enc_with_desc,
                /* renderCommandEncoderWithDescriptor: */
                    (uintptr_t)bb_desc);
            
            T1_objc_msg_1arg(bb_enc,
                ags->sel_set_render_pls,
                (uintptr_t)ags->cur_bb_pls);
            T1_objc_msg_1arg(bb_enc,
                ags->sel_set_depth_stencil_state,
                    (uintptr_t)ags->opaque_depth_stencil_state);
            
            T1_objc_msg_3arg(
                bb_enc,
                ags->sel_set_vertex_buffer_offset_atindex,
                /* setVertexBuffer: */
                    (uintptr_t)ags->flat_quad_buffers[ags->frame_i],
                /* offset: */ 0,
                /* atIndex: */ 2);
            
            T1_objc_msg_3arg(
                bb_enc,
                ags->sel_set_vertex_buffer_offset_atindex,
                /* setVertexBuffer: */
                    (uintptr_t)ags->cam_buffers[ags->frame_i],
                /* offset: */ 0,
                /* atIndex: */ 3);
            
            T1_objc_msg_3arg(
                bb_enc,
                ags->sel_draw_primitives_vertex_start_vertex_count,
                /* drawPrimitives: */ T1MTLPrimitiveTypeTriangle,
                /* vertexStart: */ 0,
                /* vertexCount: */ (uintptr_t)pass->verts_size * 6);
            
            T1_objc_msg(bb_enc, ags->sel_end_encoding);
        }
        T1_log_assert(ags->zbuf_cleared);
        break;
        case T1RENDERPASS_BLOOM:
        {
            T1_log_assert(
                T1_render_views->cpu[cam_i].
                    write_type ==
                        T1RENDERVIEW_WRITE_RENDER_TARGET);
            T1_log_assert(ags->cur_blnd_pls != NULL);
            
            if (!T1_global->draw_triangles ||
                pass->verts_size < 1)
            {
                break;
            }
            
            #if T1_BLOOM_ACTIVE == T1_ACTIVE
            // only render target can bloom atm
            T1_log_assert(cam_i == 0);
            
            void * bloom_desc = (void *)T1_objc_msg(view,
                ags->sel_current_render_pass_desc);
            set_defaults_for_render_descriptor(
                bloom_desc,
                cam_i);
            void * bloom_rtt = ags->downsampled_rtts[0];
            {
                void * color_attachments = (void *)T1_objc_msg(
                    bloom_desc,
                    ags->sel_color_attachments);
                void * color_att_0 = (void *)T1_objc_msg_1arg(
                    color_attachments,
                    ags->sel_object_at_indexed_subscript,
                    0);
                T1_objc_msg_1arg(color_att_0,
                    ags->sel_set_texture, (uintptr_t)bloom_rtt);
                T1_objc_msg_1quadf64(color_att_0,
                    ags->sel_set_clear_color,
                        T1_objc_quadf64_make(0.0, 0.0, 0.0, 0.0));
                T1_objc_msg_1arg(
                    color_att_0,
                    ags->sel_set_load_action, T1MTLLoadActionClear);
                
            }
            
            void * bloom_enc = (void *)T1_objc_msg_1arg(
                combuf,
                ags->sel_render_cmd_enc_with_desc,
                (uintptr_t)bloom_desc);
            
            T1_objc_msg_1arg(bloom_enc,
                ags->sel_set_render_pls,
                    (uintptr_t)ags->cur_bloom_pls);
            
            set_defaults_for_encoder(
                bloom_enc,
                (u32)cam_i);
            
            T1_objc_msg_1arg(bloom_enc, ags->sel_set_cull_mode, T1MTLCullModeBack);
            T1_objc_msg_1arg(bloom_enc, ags->sel_set_front_facing_winding, T1MTLWindingCounterClockwise);
            
            T1_objc_msg_3arg(bloom_enc,
                ags->sel_draw_primitives_vertex_start_vertex_count,
                /* drawPrimitives: */ T1MTLPrimitiveTypeTriangle,
                /* vertexStart   : */ (uintptr_t)pass->vert_i,
                /* vertexCount   : */ (uintptr_t)pass->verts_size);
            T1_objc_msg(bloom_enc, ags->sel_end_encoding);
            
            for (
                u32 ds_i = 1;
                ds_i < T1_DOWNSAMPLES_SIZE;
                ds_i++)
            {
                T1MTLViewport smaller_viewport =
                    ags->render_viewports[0];
                
                smaller_viewport.width =
                    T1_objc_msg(
                        ags->downsampled_rtts[ds_i],
                        ags->sel_width);
                smaller_viewport.height =
                    T1_objc_msg(
                        ags->downsampled_rtts[ds_i],
                        ags->sel_height);
                
                T1ObjcSet grid = T1_objc_set_make(
                    (uintptr_t)smaller_viewport.width,
                    (uintptr_t)smaller_viewport.height,
                    1);
                
                T1ObjcSet threadgroup = T1_objc_set_make(16, 16, 1);
                
                if (ds_i < T1_DOWNSAMPLES_CUTOFF) {
                    void * compute_enc = (void *)T1_objc_msg(
                        combuf,
                        ags->sel_compute_cmd_enc);
                    
                    T1_objc_msg_1arg(
                        compute_enc,
                        ags->sel_set_compute_pls,
                        /* setComputePipelineState: */
                            (uintptr_t)ags->downsample_compute_pls);
                    
                    T1_objc_msg_2arg(compute_enc,
                        ags->sel_set_texture_at_index, 
                        (uintptr_t)(ds_i > 0 ?
                            ags->downsampled_rtts[ds_i-1] :
                            bloom_rtt),
                        0);
                    T1_objc_msg_2arg(compute_enc,
                        ags->sel_set_texture_at_index, 
                        (uintptr_t)ags->downsampled_rtts[ds_i],
                        1);
                    
                    T1_objc_msg_2set(
                        compute_enc,
                        ags->sel_dispatch_threads_threadsperthreadgroup,
                        grid,
                        threadgroup);
                    
                    T1_objc_msg(compute_enc, ags->sel_end_encoding);
                }
                
                void * boxblur_enc = (void *)T1_objc_msg(
                    combuf, ags->sel_compute_cmd_enc);
                T1_objc_msg_1arg(
                    boxblur_enc, ags->sel_set_compute_pls,
                        (uintptr_t)ags->boxblur_compute_pls);
                T1_objc_msg_2arg(
                    boxblur_enc,
                    ags->sel_set_texture_at_index,
                    (uintptr_t)ags->downsampled_rtts[ds_i],
                    0);
                T1_objc_msg_2set(
                        boxblur_enc,
                        ags->sel_dispatch_threads_threadsperthreadgroup,
                    /* dispatchThreads: */
                        grid,
                    /* threadsPerThreadgroup: */
                        threadgroup);
                T1_objc_msg(boxblur_enc, ags->sel_end_encoding);
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
                ags->cur_flat_texquad_pls == NULL)
            {
                break;
            }
            
            void * flat_texq_desc = (void *)T1_objc_msg(
                view,
                ags->sel_current_render_pass_desc);
            
            set_defaults_for_render_descriptor(
                flat_texq_desc,
                cam_i);
            
            void * flat_texq_enc = (void *)T1_objc_msg_1arg(
                combuf,
                ags->sel_render_cmd_enc_with_desc,
                (uintptr_t)flat_texq_desc);
            
            T1_objc_msg_1arg(flat_texq_enc,
                ags->sel_set_render_pls,
                (uintptr_t)ags->cur_flat_texquad_pls);
            T1_objc_msg_1arg(flat_texq_enc,
                ags->sel_set_depth_stencil_state,
                (uintptr_t)ags->opaque_depth_stencil_state);
            
            T1_objc_msg_3arg(flat_texq_enc,
                ags->sel_set_vertex_buffer_offset_atindex,
                    /* setVertexBuffer: */
                        (uintptr_t)ags->flat_texquad_buffers[ags->frame_i],
                    /* offset: */ 0,
                    /* atIndex: */ 0);
            
            T1_objc_msg_3arg(flat_texq_enc,
                ags->sel_set_vertex_buffer_offset_atindex,
                    /* setVertexBuffer: */
                        (uintptr_t)ags->matrix_buffers[ags->frame_i],
                    /* offset: */ 0,
                    /* atIndex: */ 2);
            
            T1_objc_msg_3arg(flat_texq_enc,
                ags->sel_set_vertex_buffer_offset_atindex,
                    /* setVertexBuffer: */
                        (uintptr_t)ags->cam_buffers[ags->frame_i],
                    /* offset: */ 0,
                    /* atIndex: */ 3);
            
            #if T1_TEXTURES_ACTIVE == T1_ACTIVE
            for (
                u32 i = 0;
                i < T1_TEXARRAYS_CAP;
                i++)
            {
                if (ags->metal_textures[i] != NULL) {
                    T1_objc_msg_2arg(flat_texq_enc,
                        ags->sel_set_fragment_texture_at_index,
                            /* setVertexBuffer: */
                                (uintptr_t)ags->metal_textures[i],
                            /* atIndex: */ i);
                }
            }
            #elif T1_TEXTURES_ACTIVE == T1_INACTIVE
            T1_objc_msg_2arg(flat_texq_enc,
                ags->sel_set_fragment_texture_at_index,
                    /* setVertexBuffer: */
                        (uintptr_t)ags->metal_textures[0],
                    /* atIndex: */ 0);
            #else
            #error
            #endif
            
            T1_objc_msg_3arg(
                flat_texq_enc,
                ags->sel_draw_primitives_vertex_start_vertex_count,
                /* drawPrimitives: */ T1MTLPrimitiveTypeTriangle,
                /* vertexStart: */ 0,
                /* vertexCount: */ (uintptr_t)pass->verts_size * 6);
            
            T1_objc_msg(flat_texq_enc, ags->sel_end_encoding);
        }
        T1_log_assert(ags->zbuf_cleared);
        break;
        default:
            // render pass type not set
            T1_log_assert(0);
    }
    
    pass->verts_size = 0;
}

void T1_gpu_update_final_window_size(void) {
    if (!ags) { return; }
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
    camera_depth_texture_descriptor.textureType = T1MTLTextureType2D;
    camera_depth_texture_descriptor.pixelFormat = T1MTLPixelFormatDepth32Float;
    camera_depth_texture_descriptor.width =
        (u64)ags->window_viewport.width;
    camera_depth_texture_descriptor.height =
        (u64)ags->window_viewport.height;
    camera_depth_texture_descriptor.storageMode =
        T1MTLStorageModePrivate;
    camera_depth_texture_descriptor.usage =
        T1MTLTextureUsageRenderTarget |
        T1MTLTextureUsageShaderRead;
    
    ags->cam_depth_texture =
        [ags->device newTextureWithDescriptor:
            camera_depth_texture_descriptor];
    #endif
}

void T1_gpu_update_render_view_size(s32 at_i) {
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
    
    void * zbuffer_desc = (void *)T1_objc_msg(
        ags->class_mtl_texture_desc,
        ags->sel_new);
    T1_objc_msg_1arg(zbuffer_desc,
        ags->sel_set_texture_type, T1MTLTextureType2D);
    T1_objc_msg_1arg(zbuffer_desc,
        ags->sel_set_pixel_format, T1MTLPixelFormatDepth32Float);
    T1_objc_msg_1arg(zbuffer_desc,
        ags->sel_set_width, T1_render_views->cpu[at_i].width);
    T1_objc_msg_1arg(zbuffer_desc,
        ags->sel_set_height, T1_render_views->cpu[at_i].height);
    T1_objc_msg_1arg(zbuffer_desc,
        ags->sel_set_storage_mode,
        T1MTLStorageModePrivate);
    T1_objc_msg_1arg(
        zbuffer_desc,
        ags->sel_set_usage,
        T1MTLTextureUsageRenderTarget |
        T1MTLTextureUsageShaderRead);
    
    ags->depth_textures[at_i] = (void *)T1_objc_msg_1arg(
        ags->device,
        ags->sel_new_tex_with_desc,
        (uintptr_t)zbuffer_desc);
    
    if (at_i != 0) { return; }
    
    void * touch_id_tex_desc = (void *)T1_objc_msg(
        ags->class_mtl_texture_desc,
        ags->sel_new);
    T1_objc_msg_1arg(touch_id_tex_desc,
        ags->sel_set_width, (uintptr_t)ags->render_viewports[at_i].width);
    T1_objc_msg_1arg(touch_id_tex_desc,
        ags->sel_set_height, (uintptr_t)ags->render_viewports[at_i].height);
    T1_objc_msg_1arg(touch_id_tex_desc,
        ags->sel_set_pixel_format, T1MTLPixelFormatRGBA8Unorm);
    T1_objc_msg_1arg(touch_id_tex_desc,
        ags->sel_set_mipmap_level_count, 1);
    T1_objc_msg_1arg(touch_id_tex_desc,
        ags->sel_set_storage_mode,
        T1MTLStorageModePrivate);
    T1_objc_msg_1arg(
        touch_id_tex_desc,
        ags->sel_set_usage,
        T1MTLTextureUsageRenderTarget |
        T1MTLTextureUsageShaderRead);
    ags->touch_id_texture = (void *)T1_objc_msg_1arg(
        ags->device,
        ags->sel_new_tex_with_desc,
        (uintptr_t)touch_id_tex_desc);
    
    u64 touch_buffer_size_bytes =
        (uintptr_t)ags->render_viewports[at_i].width *
        (uintptr_t)ags->render_viewports[at_i].height * 4;
    
    T1_log_assert(ags->device != NULL);
    ags->touch_id_buffer = (void *)T1_objc_msg_2arg(
        ags->device,
        ags->sel_new_buf_with_len_options,
        touch_buffer_size_bytes,
        T1MTLResourceStorageModeShared);
    T1_log_assert(ags->touch_id_buffer != NULL);
    
    ags->touch_id_buffer_all_zeros = (void *)T1_objc_msg_2arg(
        ags->device,
        ags->sel_new_buf_with_len_options,
        touch_buffer_size_bytes,
        T1MTLResourceStorageModeShared);
    
    s32 minus_one = -1;
    T1_std_memset_s32(
        (void *)T1_objc_msg(
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
        ags->downsampled_rtts[i] = NULL;
        
        void * downsampled_rtt_desc = (void *)T1_objc_msg(
            ags->class_mtl_texture_desc,
            ags->sel_new);
        T1_objc_msg_1arg(
            downsampled_rtt_desc,
            ags->sel_set_texture_type,
            T1MTLTextureType2D);
        T1_objc_msg_1arg(
            downsampled_rtt_desc,
            ags->sel_set_width,
            (uintptr_t)get_ds_width(
                i,
                (u32)ags->render_viewports[0].width));
        T1_objc_msg_1arg(
            downsampled_rtt_desc,
            ags->sel_set_height,
            (uintptr_t)get_ds_height(
                i,
                (u32)ags->render_viewports[0].height));
        T1_objc_msg_1arg(
            downsampled_rtt_desc,
            ags->sel_set_pixel_format,
            T1MTLPixelFormatRGBA8Unorm);
        T1_objc_msg_1arg(
            downsampled_rtt_desc,
            ags->sel_set_mipmap_level_count,
            1);
        T1_objc_msg_1arg(
            downsampled_rtt_desc,
            ags->sel_set_storage_mode,
            T1MTLStorageModePrivate);
        if (i == 0) {
            T1_objc_msg_1arg(
                downsampled_rtt_desc,
                ags->sel_set_usage,
                T1MTLTextureUsageShaderWrite |
                T1MTLTextureUsageShaderRead |
                T1MTLTextureUsageRenderTarget);
        } else {
            T1_objc_msg_1arg(
                downsampled_rtt_desc,
                ags->sel_set_usage,
                T1MTLTextureUsageShaderWrite |
                T1MTLTextureUsageShaderRead);
        }
        ags->downsampled_rtts[i] = (void *)T1_objc_msg_1arg(
            ags->device,
            ags->sel_new_tex_with_desc,
            (uintptr_t)downsampled_rtt_desc);
    }
    #elif T1_BLOOM_ACTIVE == T1_INACTIVE
    #else
    #error
    #endif
}

void T1_gpu_draw_in_mtk_view(void * view)
{
    if (!ags || !ags->metal_active) {
        return;
    }
    
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
    
    void * combuf = (void *)T1_objc_msg(
        ags->command_queue,
        ags->sel_command_buffer);
    
    if (combuf == NULL) {
        #if T1_LOGGER_ASSERTS_ACTIVE
        log_dump_and_crash("error - can't get metal command buffer\n");
        #endif
        
        return;
    }
    
    // Blit to clear the touch id buffer
    T1_log_assert(ags->touch_id_texture != NULL);
    u64 touch_id_w = T1_objc_msg(
        ags->touch_id_texture,
        ags->sel_width);
    u64 touch_id_h = T1_objc_msg(
        ags->touch_id_texture,
        ags->sel_height);
    u64 size_bytes = touch_id_w * touch_id_h * 8;
    
    T1ObjcSet touch_buf_size = T1_objc_set_make(touch_id_w, touch_id_h, 1);
    T1ObjcSet touch_buf_origin = T1_objc_set_make(0, 0, 0);
    
    {
    void * clear_touch_tex_blit_enc = (void *)T1_objc_msg(
        combuf, ags->sel_blit_command_encoder);
    
    T1_objc_msg_4arg_1set_3arg_1set(
        clear_touch_tex_blit_enc,
        ags->sel_copy_from_buffer_source_offset_source_bytes_per_row,
        (uintptr_t)ags->touch_id_buffer_all_zeros,
        0,
        touch_id_w * 4,
        size_bytes,
        touch_buf_size,
        (uintptr_t)(ags->touch_id_texture),
        0,
        0,
        touch_buf_origin);
    
    T1_objc_msg(clear_touch_tex_blit_enc, ags->sel_end_encoding);
    }
    
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
        
        ags->cur_rtt = NULL;
        ags->cur_depth = NULL;
        ags->cur_opq_pls = NULL;
        ags->cur_blnd_pls = NULL;
        ags->cur_bloom_pls = NULL;
        ags->cur_bb_pls = NULL;
        ags->cur_flat_texquad_pls = NULL;
        
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
                if (ags->cur_rtt == NULL) { continue; }
                
                ags->cur_depth = ags->depth_textures[0];
                T1_log_assert(ags->cur_depth != NULL);
                ags->cur_opq_pls = ags->diamond_touch_pls;
                ags->cur_blnd_pls = ags->blend_touch_pls;
                ags->cur_bloom_pls = ags->blend_touch_pls;
                ags->cur_bb_pls = ags->bb_touch_pls;
                ags->cur_flat_texquad_pls = ags->flat_texquad_touch_pls;
            }
            break;
            case T1RENDERVIEW_WRITE_RGBA:
            {
                ags->cur_rtt = get_tex_slice(
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
                ags->cur_rtt = NULL;
                ags->cur_depth = ags->depth_textures[slice_i];
                
                T1_log_assert(ags->cur_depth != NULL);
                ags->cur_opq_pls =
                    ags->depth_only_pls;
                ags->cur_blnd_pls =
                    ags->depth_only_pls;
                ags->cur_bloom_pls =
                    ags->depth_only_pls;
                ags->cur_bb_pls = NULL;
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
            T1_gpu_draw_single_pass(pass_i, cam_i, view, combuf);
        }
        
        T1_log_assert(ags->viewports_set[cam_i]);
        
        T1_log_assert(T1_tex_to_array_i(T1_render_views->cpu[cam_i].
            write_tex) >= 1);
        T1_log_assert(T1_tex_to_slice_i(T1_render_views->cpu[cam_i].
            write_tex) >= 0);
    }
    
    // copy the touch id buffer for CPU use
    {
    void * blit_touch_tex_to_cpu_enc = (void *)T1_objc_msg(
        combuf, ags->sel_blit_command_encoder);
    
    T1_objc_msg_3arg_2set_4arg(
        blit_touch_tex_to_cpu_enc,
        ags->sel_copy_from_texture_to_buffer,
        (uintptr_t)ags->touch_id_texture,
        /* source_slice: */ 0,
        /* source_level: */ 0,
        /* source_origin: */ touch_buf_origin,
        /* source_size: */ touch_buf_size,
        /* to_buffer: */ (uintptr_t)ags->touch_id_buffer,
        /* destination_offset: */ 0,
        /* destination_bytes_per_row: */ touch_id_w * 4,
        /* destination_bytes_per_image: */ touch_id_w * touch_id_h * 4);
    
    T1_objc_msg(blit_touch_tex_to_cpu_enc, ags->sel_end_encoding);
    }
     
    // Pass 5 puts a quad on the full screen
    void * pass_5_comp_desc = (void *)T1_objc_msg(
        view,
        ags->sel_current_render_pass_desc);
    
    void * pass_5_comp = (void *)T1_objc_msg_1arg(
        combuf,
        ags->sel_render_cmd_enc_with_desc,
        (uintptr_t)pass_5_comp_desc);
    
    T1_objc_msg_1sextf64(
        pass_5_comp,
        ags->sel_set_viewport,
        *((T1ObjcSextf64 *)&ags->window_viewport));
    
    T1_objc_msg_1arg(
        pass_5_comp,
        ags->sel_set_cull_mode,
        T1MTLCullModeNone);
    T1_objc_msg_1arg(
        pass_5_comp,
        ags->sel_set_render_pls, (uintptr_t)ags->singlequad_pls);
    T1_objc_msg_3arg(
        pass_5_comp,
        ags->sel_set_vertex_bytes_length_atindex,
        (uintptr_t)ags->quad_vertices,
        sizeof(T1PostProcessingVertex)*6,
        0);
    T1_objc_msg_3arg(
        pass_5_comp,
        ags->sel_set_vertex_buffer_offset_atindex,
        /* setVertexBuffer: */
            (uintptr_t)ags->postprocessing_constants_buffers[ags->frame_i],
        /* offset: */
            0,
        /* atIndex: */
            1);
    
    // The main camera must have a target to write to
    T1_log_assert(
        T1_tex_to_array_i(T1_render_views->cpu[0].write_tex) >= 0);
    T1_log_assert(
        T1_tex_to_array_i(T1_render_views->cpu[0].write_tex) <
            (s32)T1_tex_arrays_size);
    
    void * arr_tex =
        ags->metal_textures[T1_tex_to_array_i(T1_render_views->cpu[0].write_tex)];
    T1ObjcPair levels_range = T1_objc_pair_make(0, T1_objc_msg(arr_tex, ags->sel_mipmap_level_count));
    T1ObjcPair slices_range = T1_objc_pair_make(
        (uintptr_t)T1_tex_to_slice_i(
            T1_render_views->cpu[0].write_tex),
        1);
    void * sliced_tex = (void *)T1_objc_msg_2arg_2pair(
        arr_tex,
        ags->sel_new_texture_view_with_pixel_format,
        T1_objc_msg(arr_tex, ags->sel_pixel_format),
        T1MTLTextureType2D,
        levels_range,
        slices_range);
    
    T1_objc_msg_2arg(
        pass_5_comp,
        ags->sel_set_fragment_texture_at_index,
            /* setFragmentTexture: */ (uintptr_t)sliced_tex,
            /* atIndex: */ 0);
    
    #if T1_BLOOM_ACTIVE == T1_ACTIVE
    T1_objc_msg_2arg(
        pass_5_comp,
        ags->sel_set_fragment_texture_at_index,
        /* setFragmentTexture: */ (uintptr_t)ags->downsampled_rtts[1],
        /* atIndex: */ 1);
    T1_objc_msg_2arg(
        pass_5_comp,
        ags->sel_set_fragment_texture_at_index,
        /* setFragmentTexture: */ (uintptr_t)ags->downsampled_rtts[2],
        /* atIndex: */ 2);
    T1_objc_msg_2arg(
        pass_5_comp,
        ags->sel_set_fragment_texture_at_index,
        /* setFragmentTexture: */ (uintptr_t)ags->downsampled_rtts[3],
        /* atIndex: */ 3);
    T1_objc_msg_2arg(
        pass_5_comp,
        ags->sel_set_fragment_texture_at_index,
        /* setFragmentTexture: */ (uintptr_t)ags->downsampled_rtts[4],
        /* atIndex: */ 4);
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
    #elif T1_LOG_ASSERTS_ACTIVE == T1_INACTIVE
    // Pass
    #else
    #error
    #endif
    
    T1_objc_msg_2arg(
        pass_5_comp,
        ags->sel_set_fragment_texture_at_index,
        /* setFragmentTexture: */ (uintptr_t)ags->metal_textures[perlin_ta_i],
        /* atIndex: */ 6);
    
    T1_objc_msg_2arg(
        pass_5_comp,
        ags->sel_set_fragment_texture_at_index,
        /* setFragmentTexture: */ (uintptr_t)ags->depth_textures[0],
        /* atIndex: */ T1_CAM_DEPTH_FRAGARG_I);
    
    T1_objc_msg_3arg(
        pass_5_comp,
        ags->sel_draw_primitives_vertex_start_vertex_count,
        /* drawPrimitives: */ T1MTLPrimitiveTypeTriangle,
        /* vertexStart: */ 0,
        /* vertexCount: */ 6);
    
    T1_objc_msg(pass_5_comp, ags->sel_end_encoding);
    
    void * current_drawable = (void *)T1_objc_msg(
        view,
        ags->sel_current_drawable);
    T1_log_assert(current_drawable != NULL);
    
    T1_objc_msg_1arg(
        combuf, ags->sel_present_drawable,
        /* presentDrawable: */
            (uintptr_t)current_drawable);
    
    ags->frame_i += 1;
    ags->frame_i %= T1_FRAMES_CAP;
    T1_log_assert(ags->frame_i < T1_FRAMES_CAP);
    
    T1_objc_msg(combuf, ags->sel_commit);
    // T1_objc_msg(combuf, ags->sel_wait_until_completed);
    
    funcptr_gameloop_after_render();
}

void T1_os_gpu_update_internal_render_viewport(
    const s32 at_i)
{
    if (!ags || !ags->metal_active) { return; }
    
    T1_log_assert(at_i >= 0);
    T1_log_assert(at_i < T1_RENDER_VIEW_CAP);
    
    ags->viewports_set[at_i] = false;
    T1_gpu_update_render_view_size(at_i);
}

void T1_os_gpu_update_window_viewport(void)
{
    T1_gpu_update_final_window_size();
}

s16 T1_apple_gpu_make_depth_tex(
    u32 width,
    u32 height)
{
    s16 slice_i = 0;
    while (ags->depth_textures[slice_i] != NULL) {
        slice_i += 1;
        T1_log_assert(slice_i < T1_RENDER_VIEW_CAP);
    }
    
    if (slice_i >= T1_RENDER_VIEW_CAP) {
        return -1;
    }
    
    void * desc = (void *)T1_objc_msg(
        ags->class_mtl_texture_desc, ags->sel_new);
    T1_objc_msg_1arg(desc, ags->sel_set_width, width);
    T1_objc_msg_1arg(desc, ags->sel_set_height, height);
    T1_objc_msg_1arg(desc, ags->sel_set_texture_type, T1MTLTextureType2D);
    T1_objc_msg_1arg(desc, ags->sel_set_pixel_format, T1MTLPixelFormatDepth32Float);
    T1_objc_msg_1arg(desc, ags->sel_set_usage,
        T1MTLTextureUsageRenderTarget | T1MTLTextureUsageShaderRead);
    
    ags->depth_textures[slice_i] = (void *)T1_objc_msg_1arg(
        ags->device,
        ags->sel_new_tex_with_desc,
        (uintptr_t)desc);
    
    T1_log_assert(ags->depth_textures[slice_i] != NULL);
    
    return slice_i;
}
