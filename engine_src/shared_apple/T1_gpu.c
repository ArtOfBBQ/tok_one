#include "T1_gpu.h"

#include <mach/mach.h>

#include "T1_global.h"
#include "T1_mem.h"
#include "T1_assert.h"
#include "T1_pipeline.h"
#include "T1_pass.h"
#include "T1_buf.h"
#include "T1_embedded_data.h"
#include "T1_objc.h"
#include "T1_material.h"
#include "T1_mesh_summary.h"
#include "T1_tex_array.h"
#include "T1_render_view.h"
#include "T1_os.h"
#include "T1_io.h"

#define T1MTLPrimitiveTypePoint 0
#define T1MTLPrimitiveTypeLine 1
#define T1MTLPrimitiveTypeLineStrip 2
#define T1MTLPrimitiveTypeTriangle 3
#define T1MTLPrimitiveTypeTriangleStrip 4

#define T1MTLWindingClockwise 0
#define T1MTLWindingCounterClockwise 1
#define T1MTLBlendOperationAdd 0
#define T1MTLBlendFactorOne 1
#define T1MTLBlendFactorSourceAlpha 4
#define T1MTLBlendFactorOneMinusSourceAlpha 5
#define T1MTLMutabilityImmutable 2
#define T1MTLTextureType2D 2
#define T1MTLTextureType2DArray 3
#define T1MTLCompareFunctionLessEqual 3
#define T1MTLPixelFormatInvalid 0
#define T1MTLPixelFormatRGBA8Unorm 70
#define T1MTLPixelFormatBGRA8Unorm 80
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

#define T1MTLLoadActionDontCare 0
#define T1MTLLoadActionLoad 1
#define T1MTLLoadActionClear 2
#define T1MTLCommandBufferStatusCompleted 4

#define T1MTLStoreActionStore 1

// What a pipeline state draws into. Metal bakes this into the state, so
// each T1_pipelines entry gets a state per combination, built on first use
// [[color(k)]] is T1_GPU_TARGET_COLOR_0 << k
#define T1_GPU_TARGET_COLOR_0 0x01
#define T1_GPU_TARGET_DEPTH   0x10
// color(0) is the window, in its format
#define T1_GPU_TARGET_WINDOW  0x20
#define T1_GPU_TARGETS_SIZE   0x40

typedef struct {
    uintptr_t location;
    uintptr_t length;
} T1NSRange;

static T1NSRange T1_nsrange_make(
    uintptr_t location,
    uintptr_t length)
{
    // 0 length NSRange is legal, but why do it in T1?
    T1_assert(length != 0);
    
    T1NSRange out;
    out.length = length;
    out.location = location;
    return out;
}

typedef struct {
    uintptr_t width, height, depth;
} T1MTLSize;

static T1MTLSize T1_mtlsize_make(
    uintptr_t width,
    uintptr_t height,
    uintptr_t depth)
{
    T1_assert(width > 0);
    T1_assert(height > 0);
    T1_assert(depth > 0);
    
    T1MTLSize sz;
    sz.width  = width;
    sz.height = height;
    sz.depth  = depth;
    
    return sz;
}

typedef struct {
    uintptr_t x, y, z;
} T1MTLOrigin;

static T1MTLOrigin T1_mtlorigin_make(
    uintptr_t x,
    uintptr_t y,
    uintptr_t z)
{
    T1_assert(x < UINT16_MAX); // TODO: delete me
    T1_assert(y < UINT16_MAX); // TODO: delete me
    T1_assert(z < UINT16_MAX); // TODO: delete me
    
    T1MTLOrigin out;
    out.x = x;
    out.y = y;
    out.z = z;
    
    return out;
}

typedef struct {
    double originX, originY, width, height, znear, zfar;
} T1MTLViewport;

typedef void (* dispatchThreadsFuncType)(
    void *, void *,
    /* dispatchThreads:(MTLSize): */ T1MTLSize,
    /* threadsPerThreadgroup:(MTLSize): */ T1MTLSize);

/*
- (id<MTLTexture>)
    newTextureViewWithPixelFormat:(MTLPixelFormat) pixelFormat 
    textureType:(MTLTextureType) textureType 
    levels:(NSRange) levelRange 
    slices:(NSRange) sliceRange;
*/
typedef void * (* newTextureViewFuncType)(
    void *, void *,
    /* withPixelFormat : */ uintptr_t,
    /* MTLTextureType  : */ uintptr_t,
    /* levels:(NSRange): */ T1NSRange,
    /* slices:(NSRange): */ T1NSRange);

typedef struct {
    double alpha;
    double blue;
    double green;
    double red;
} T1MTLClearColor;

static T1MTLClearColor T1_mtl_clearcolor_make(
    double alpha,
    double blue,
    double green,
    double red)
{
    T1MTLClearColor out;
    out.alpha = alpha;
    out.blue  = blue;
    out.green = green;
    out.red   = red;
    return out;
}

typedef void (* setClearColorFuncType)(
    void *, void *,
    T1MTLClearColor);

typedef struct {
    f64 width;
    f64 height;
} T1CGSize;

/*
The CPU only ever asks for the touch id under the mouse, so instead of
reading back the whole touch id texture every frame we copy a small square
around the mouse. There's one sample per frame in flight, and the CPU only
reads a sample after the GPU has finished its frame.
*/
#define T1_GPU_TOUCH_SAMPLE_DIM 32

typedef struct {
    void * buffer; // id<MTLBuffer>, shared storage
    void * combuf; // id<MTLCommandBuffer>, retained until its frame is done
    u64 frame_no;
    u32 x; // region of the touch id texture, in its pixels
    u32 y;
    u32 w;
    u32 h;
    u32 tex_w; // touch id texture size when sampled
    u32 tex_h;
    b8 valid; // false if the mouse was outside or nothing wrote touch ids
} T1GPUTouchSample;

/*
A T1_buf_write() waiting for the next frame: its bytes were copied into
staging (shared memory) right away, the frame's first blit copies them to
the T1Buf
*/
#define T1_GPU_BUF_WRITES_CAP 256
typedef struct {
    void * staging;
    u64 offset;
    u64 size;
    u32 buf_i;
} T1GPUBufWrite;

/*
A T1_buf_read_start(): the next frame copies the bytes to staging (shared
memory) after its passes, T1_buf_read_finish() hands them over once that
frame's combuf is done
*/
typedef enum : u8 {
    T1_GPU_BUF_READ_FREE = 0,
    T1_GPU_BUF_READ_QUEUED, // for the next frame
    T1_GPU_BUF_READ_ENCODED, // into combuf
} T1GPUBufReadState;

typedef struct {
    void * staging;
    void * combuf; // retained
    u64 offset;
    u64 size;
    u32 buf_i;
    T1GPUBufReadState state;
} T1GPUBufRead;

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
    void * frame_consts_buffers[T1_FRAMES_CAP];
    void * locked_vertex_populator_buffer;
    void * locked_vertex_buffer;
    void * locked_matf32_populator_buffer;
    void * locked_mats32_populator_buffer;
    void * locked_matf32_buffer;
    void * locked_mats32_buffer;
    void * projection_constants_buffer;
    
    void * depth_textures[T1_RENDER_VIEW_CAP];
    
    // per T1_pipelines entry: its functions from lib ([1] is NULL for
    // compute), and a state per T1_GPU_TARGET_* combination ([0] for
    // compute), built on first use
    void * pipeline_funcs[T1_PIPELINES_CAP][2];
    void * pipeline_states[T1_PIPELINES_CAP][T1_GPU_TARGETS_SIZE];
    
    void * opaque_depth_stencil_state;
    
    #if T1_TEXTURES_ACTIVE == T1_ACTIVE
    void * metal_textures[T1_TEXARRAYS_CAP];
    #elif T1_TEXTURES_ACTIVE == T1_INACTIVE
    void * metal_textures[1]; // for font only
    #else
    #error
    #endif
    
    // the T1Tex touch ids are read from, T1_TEX_NONE for none
    T1Tex touch_tex;
    // A pass's T1_TEX_WINDOW, at window pixel size. This is the finished
    // frame; presenting copies it to the screen.
    void * final_texture;
    u32 final_texture_w;
    u32 final_texture_h;
    // The drawableSize we last gave the CAMetalLayer
    u32 layer_drawable_w;
    u32 layer_drawable_h;
    T1GPUTouchSample touch_samples[T1_FRAMES_CAP];
    // CPU copy of the newest finished sample (buffer & combuf unused)
    T1GPUTouchSample touch_latest;
    u8 touch_latest_bytes[
        T1_GPU_TOUCH_SAMPLE_DIM * T1_GPU_TOUCH_SAMPLE_DIM * 4];
    u64 frames_encoded;
    
    // the client's T1Bufs, in private GPU memory, NULL for unused
    void * bufs[T1_BUFS_CAP];
    // T1_buf_write()s the next frame applies first, in order
    T1GPUBufWrite buf_writes[T1_GPU_BUF_WRITES_CAP];
    u32 buf_writes_size;
    // T1_buf_read_start()s, by read id
    T1GPUBufRead buf_reads[T1_BUF_READS_CAP];
    
    // a camera's first pass of the frame clears its depth buffer
    b8 cam_depth_cleared[T1_RENDER_VIEW_CAP];
    
    // objective-c classes
    void * class_mtl_texture_desc; // MTLTextureDescriptor
    void * class_mtl_render_pipeline_desc; // MTLRenderPipelineDescriptor
    void * class_mtl_depth_stencil_desc; // MTLDepthStencilDescriptor
    void * class_mtl_render_pass_desc; // MTLRenderPassDescriptor
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
    void * sel_set_bytes_length_atindex; // setBytes:length:atIndex:
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
    void * sel_status; // status
    void * sel_retain; // retain
    void * sel_release; // release
    void * sel_end_encoding; // endEncoding
    void * sel_next_drawable; // nextDrawable
    void * sel_set_drawable_size; // setDrawableSize:
    void * sel_texture; // texture
    void * sel_copy_from_texture_to_texture; // copyFromTexture:toTexture:
    void * sel_copy_from_texture_region_to_texture; // copyFromTexture:sourceSlice:sourceLevel:sourceOrigin:sourceSize:toTexture:destinationSlice:destinationLevel:destinationOrigin:
    void * sel_copy_from_texture_to_buffer; // copyFromTexture:sourceSlice:sourceLevel:sourceOrigin:sourceSize:toBuffer:destinationOffset:destinationBytesPerRow:destinationBytesPerImage:
    void * sel_new_buffer_with_bytes_no_copy; // newBufferWithBytesNoCopy:length:options:deallocator:
    void * sel_new_buffer_with_bytes_length_options; // newBufferWithBytes:length:options:
    void * sel_set_buffer_offset_atindex; // setBuffer:offset:atIndex:
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
    void * sel_render_pass_desc; // renderPassDescriptor
    void * sel_file_URL_with_path_is_directory; // fileURLWithPath:isDirectory:
    void * sel_dispatch_threads_threadsperthreadgroup; // dispatchThreads:threadsPerThreadsGroup:
    void * sel_new_lib_with_source_options_error; // newLibraryWithSource:options:error:
    void * sel_description; // description 
    T1PostProcessingVertex quad_vertices[6];
    f32 retina_scaling_factor;
    u8  frame_i;
    b8  metal_active;
    b8  shaders_ready;
    b8  objc_metal_framework_good;
} AppleGPUState;

static AppleGPUState * ags = NULL;

/*
Objects returned by new... methods (textures, texture views, descriptors) are
owned by us: autorelease pools never free them, so we have to release them
when we drop them. It's safe to release a texture the GPU is still using,
command buffers keep their own reference until they're done.
Releases *obj if it's set and clears it.
*/
static void T1_gpu_release(void ** obj) {
    if (*obj != NULL) {
        T1_objc_msg(*obj, ags->sel_release);
        *obj = NULL;
    }
}


static void * T1_gpu_new_render_pipeline_descriptor(
    void * vertex_shader,
    void * fragment_shader,
    u8 targets,
    u8 blend)
{
    T1_assert(targets < T1_GPU_TARGETS_SIZE);
    T1_assert(blend <= T1_BLEND_STRAIGHT_ALPHA);
    
    void * pool = T1_objc_autorelease_pool_push();
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
    void * color_attachments = (void *)T1_objc_msg(
        desc,
        ags->sel_color_attachments);
    for (u32 k = 0; k < T1_PASS_COLOR_TEXS_CAP; k++) {
        if (!(targets & (T1_GPU_TARGET_COLOR_0 << k))) { continue; }
        
        void * color_attachment = (void *)T1_objc_msg_1arg(
            color_attachments,
            ags->sel_object_at_indexed_subscript,
            k);
        T1_objc_msg_1arg(
            color_attachment,
            ags->sel_set_pixel_format,
            (k == 0 && (targets & T1_GPU_TARGET_WINDOW)) ?
                T1MTLPixelFormatBGRA8Unorm :
                ags->pixel_format_renderpass1);
        // only color(0) blends, the others are data (like touch ids)
        if (k == 0 && blend != T1_BLEND_NONE) {
            T1_objc_msg_1arg(
                color_attachment,
                ags->sel_set_blending_enabled,
                1);
            T1_objc_msg_1arg(
                color_attachment,
                ags->sel_set_source_rgb_blend_factor,
                blend == T1_BLEND_STRAIGHT_ALPHA ?
                    T1MTLBlendFactorSourceAlpha :
                    T1MTLBlendFactorOne);
            T1_objc_msg_1arg(
                color_attachment,
                ags->sel_set_dest_rgb_blend_factor,
                blend == T1_BLEND_ADDITIVE ?
                    T1MTLBlendFactorOne :
                    T1MTLBlendFactorOneMinusSourceAlpha);
            T1_objc_msg_1arg(
                color_attachment,
                ags->sel_set_rgb_blend_operation,
                T1MTLBlendOperationAdd);
        }
    }
    T1_objc_msg_1arg(
        desc,
        ags->sel_set_depth_attachment_pixel_format,
        (targets & T1_GPU_TARGET_DEPTH) ?
            T1MTLPixelFormatDepth32Float :
            T1MTLPixelFormatInvalid);
    
    T1_objc_autorelease_pool_pop(pool);
    return desc;
}

void T1_apple_gpu_init(
    void * with_metal_device,
    f32 backing_scale_factor,
    char ** const sticky_error)
{
    if (*sticky_error != 0) { return; }
    
    if (T1_cpu_to_gpu_data == NULL) {
        *sticky_error = "T1_apple_gpu_init() before T1_cpu_to_gpu_data was allocated";
        return;
    }
    
    void * pool = T1_objc_autorelease_pool_push();
    
    ags = T1_mem_malloc_unmanaged(sizeof(AppleGPUState));
    T1_std_memset(ags, 0, sizeof(AppleGPUState));
    ags->retina_scaling_factor = backing_scale_factor;
    ags->touch_tex = T1_TEX_NONE;
    ags->pixel_format_renderpass1 = 0;
    
    ags->class_mtl_texture_desc = T1_objc_get_class("MTLTextureDescriptor");
    ags->class_mtl_render_pipeline_desc = T1_objc_get_class("MTLRenderPipelineDescriptor");
    ags->class_mtl_depth_stencil_desc = T1_objc_get_class("MTLDepthStencilDescriptor");
    ags->class_mtl_render_pass_desc = T1_objc_get_class("MTLRenderPassDescriptor");
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
    ags->sel_set_bytes_length_atindex = T1_objc_reg_sel(
        "setBytes:length:atIndex:");
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
    T1_assert(ags->sel_new_compute_pls_with_func != NULL);
    ags->sel_new_tex_with_desc =
        T1_objc_reg_sel("newTextureWithDescriptor:");
    T1_assert(ags->sel_new_tex_with_desc != NULL);
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
    ags->sel_set_rgb_blend_operation = T1_objc_reg_sel(
        "setRgbBlendOperation:");
    ags->sel_set_storage_mode = T1_objc_reg_sel(
        "setStorageMode:");
    ags->sel_set_usage = T1_objc_reg_sel("setUsage:");
    ags->sel_set_width = T1_objc_reg_sel(
        "setWidth:"); // setWidth:
    ags->sel_set_height = T1_objc_reg_sel(
        "setHeight:"); // setWidth:
    ags->sel_set_mipmap_level_count = T1_objc_reg_sel(
        "setMipmapLevelCount:");
    ags->sel_usage = T1_objc_reg_sel("usage");
    T1_assert(ags->sel_usage != NULL);
    ags->sel_mipmap_level_count = T1_objc_reg_sel(
        "mipmapLevelCount");
    ags->sel_new_depth_stencil_state_with_desc = T1_objc_reg_sel(
        "newDepthStencilStateWithDescriptor:");
    T1_assert(ags->sel_new_depth_stencil_state_with_desc != NULL);
    ags->sel_blit_command_encoder = T1_objc_reg_sel(
        "blitCommandEncoder");
    ags->sel_copy_from_buffer_source_offset_to_buffer = T1_objc_reg_sel(
        "copyFromBuffer:sourceOffset:toBuffer:destinationOffset:size:");
    ags->sel_copy_from_buffer_source_offset_source_bytes_per_row = T1_objc_reg_sel(
        "copyFromBuffer:sourceOffset:sourceBytesPerRow:sourceBytesPerImage:sourceSize:toTexture:destinationSlice:destinationLevel:destinationOrigin:");
    ags->sel_generate_mipmaps_for_texture = T1_objc_reg_sel(
        "generateMipmapsForTexture:");
    ags->sel_present_drawable = T1_objc_reg_sel(
        "presentDrawable:");
    ags->sel_commit = T1_objc_reg_sel(
        "commit");
    ags->sel_wait_until_completed = T1_objc_reg_sel(
        "waitUntilCompleted");
    ags->sel_status = T1_objc_reg_sel(
        "status");
    ags->sel_retain = T1_objc_reg_sel(
        "retain");
    ags->sel_release = T1_objc_reg_sel(
        "release");
    ags->sel_end_encoding = T1_objc_reg_sel(
        "endEncoding");
    ags->sel_next_drawable = T1_objc_reg_sel(
        "nextDrawable");
    ags->sel_set_drawable_size = T1_objc_reg_sel(
        "setDrawableSize:");
    ags->sel_texture = T1_objc_reg_sel(
        "texture");
    ags->sel_copy_from_texture_to_texture = T1_objc_reg_sel(
        "copyFromTexture:toTexture:");
    ags->sel_copy_from_texture_region_to_texture = T1_objc_reg_sel(
        "copyFromTexture:sourceSlice:sourceLevel:sourceOrigin:sourceSize:"
        "toTexture:destinationSlice:destinationLevel:destinationOrigin:");
    ags->sel_copy_from_texture_to_buffer = T1_objc_reg_sel(
        "copyFromTexture:sourceSlice:sourceLevel:sourceOrigin:sourceSize:toBuffer:destinationOffset:destinationBytesPerRow:destinationBytesPerImage:");
    ags->sel_new_buffer_with_bytes_no_copy = T1_objc_reg_sel(
        "newBufferWithBytesNoCopy:length:options:deallocator:");
    ags->sel_new_buffer_with_bytes_length_options = T1_objc_reg_sel(
        "newBufferWithBytes:length:options:");
    ags->sel_set_buffer_offset_atindex = T1_objc_reg_sel(
        "setBuffer:offset:atIndex:");
    ags->sel_new_command_queue = T1_objc_reg_sel(
        "newCommandQueue");
    ags->sel_vertex_buffers = T1_objc_reg_sel(
        "vertexBuffers");
    ags->sel_new_default_library = T1_objc_reg_sel(
        "newDefaultLibrary");
    ags->sel_depth_attachment = T1_objc_reg_sel(
        "depthAttachment"); 
    ags->sel_set_load_action = T1_objc_reg_sel(
        "setLoadAction:");
    ags->sel_set_clear_depth = T1_objc_reg_sel(
        "setClearDepth:");
    ags->sel_set_store_action = T1_objc_reg_sel(
        "setStoreAction:");
    ags->sel_set_texture = T1_objc_reg_sel(
        "setTexture:");
    ags->sel_set_depth_write_enabled = T1_objc_reg_sel(
        "setDepthWriteEnabled:");
    ags->sel_set_depth_compare_function = T1_objc_reg_sel(
        "setDepthCompareFunction:");
    ags->sel_render_pass_desc = T1_objc_reg_sel(
        "renderPassDescriptor");
    ags->sel_file_URL_with_path_is_directory = T1_objc_reg_sel(
        "fileURLWithPath:isDirectory:");
    ags->sel_dispatch_threads_threadsperthreadgroup = T1_objc_reg_sel(
        "dispatchThreads:threadsPerThreadgroup:");
    ags->sel_new_lib_with_source_options_error = T1_objc_reg_sel(
        "newLibraryWithSource:options:error:");
    ags->sel_description = T1_objc_reg_sel(
        "description");
    
    ags->pixel_format_renderpass1 = T1MTLPixelFormatRGBA8Unorm; // 70
    
    ags->frame_i = 0;
    
    ags->device = with_metal_device;
    
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
        *sticky_error = "Failed to load the depth stencil state";
        T1_objc_autorelease_pool_pop(pool);
        return;
    }
    
    for (
        u32 fram_i = 0;
        fram_i < T1_FRAMES_CAP;
        fram_i++)
    {
        T1GPUFrame * f = &T1_cpu_to_gpu_data->
            triple_buffers[fram_i];
        
        T1_assert(
            T1_cpu_to_gpu_data->
                polygons_alloc_size >=
            (sizeof(T1GPUzSprite) *
                T1_ZSPRITES_CAP));
        
        T1_assert(f->zsprite_list != NULL);
        ags->polygon_buffers[fram_i] = (void *)T1_objc_msg_4arg(
            ags->device,
            ags->sel_new_buffer_with_bytes_no_copy,
            (uintptr_t)f->zsprite_list->polygons,
            T1_cpu_to_gpu_data->polygons_alloc_size,
            T1MTLResourceStorageModeShared,
            (uintptr_t)NULL);
        T1_assert(ags->polygon_buffers[fram_i] != NULL);
        
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
        
        ags->frame_consts_buffers[fram_i] = (void *)T1_objc_msg_4arg(
            ags->device,
            ags->sel_new_buffer_with_bytes_no_copy,
            (uintptr_t)f->frame_consts,
            T1_cpu_to_gpu_data->frame_consts_alloc_size,
            T1MTLResourceStorageModeShared,
            (uintptr_t)NULL);
        
        ags->light_buffers[fram_i] = (void *)T1_objc_msg_4arg(
            ags->device,
            ags->sel_new_buffer_with_bytes_no_copy,
            (uintptr_t)f->lights,
            T1_cpu_to_gpu_data->lights_alloc_size,
            T1MTLResourceStorageModeShared | T1MTLResourceUsageRead,
            (uintptr_t)NULL);
        
        T1_assert(
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
    
    ags->command_queue = (void *)T1_objc_msg(
        ags->device,
        ags->sel_new_command_queue);
    
    ags->metal_active = true;
    
    T1_objc_autorelease_pool_pop(pool);
    return;
}

// Copies a description of a failed shader attempt into the caller's
// (optional) error recipient. nserror may be NULL.
static void T1_gpu_shader_attempt_failed(
    const char * msg,
    void * nserror,
    u8 * ok,
    char * error_recip,
    const u32 error_recip_cap)
{
    *ok = false;
    
    if (error_recip == NULL) { return; }
    
    T1_std_strcpy_cap(error_recip, error_recip_cap, msg);
    
    if (nserror != NULL) {
        void * nsstr_error_desc = (void *)T1_objc_msg(
            nserror,
            ags->sel_description);
        char * error_desc = T1_objc_nsstring_to_cstring(
            nsstr_error_desc);
        if (error_desc != NULL) {
            if (T1_std_strlen(error_desc) > 400) {
                error_desc[400] = '\0';
            }
            T1_std_strcat_cap(error_recip, error_recip_cap, ": ");
            T1_std_strcat_cap(
                error_recip,
                error_recip_cap,
                error_desc);
        }
    }
}

// Sticky: does nothing if *ok is already false
static void * T1_gpu_new_shader_func(
    void * lib,
    const char * func_name,
    u8 * ok,
    char * error_recip,
    const u32 error_recip_cap)
{
    if (!*ok) { return NULL; }
    
    void * nsstring_func_name = T1_objc_nsstring_construct(func_name);
    void * func = (void *)T1_objc_msg_1arg(
        lib,
        ags->sel_new_function_with_name,
        (uintptr_t)nsstring_func_name);
    
    if (func == NULL) {
        char msg[128];
        T1_std_strcpy_cap(msg, 128, "Missing function: ");
        T1_std_strcat_cap(msg, 128, func_name);
        T1_std_strcat_cap(msg, 128, "()");
        T1_gpu_shader_attempt_failed(
            msg, NULL, ok, error_recip, error_recip_cap);
    }
    
    return func;
}

// Sticky: does nothing if *ok is already false
static void * T1_gpu_new_render_pls(
    void * desc,
    const char * pls_name,
    u8 * ok,
    char * error_recip,
    const u32 error_recip_cap)
{
    if (!*ok) { return NULL; }
    
    void * error = NULL;
    void * pls = (void *)T1_objc_msg_2arg(
        ags->device,
        ags->sel_new_render_pls_with_descriptor_error,
        (uintptr_t)desc,
        (uintptr_t)&error);
    
    if (pls == NULL) {
        char msg[128];
        T1_std_strcpy_cap(msg, 128, "Failed to create pipeline: ");
        T1_std_strcat_cap(msg, 128, pls_name);
        T1_gpu_shader_attempt_failed(
            msg, error, ok, error_recip, error_recip_cap);
    }
    
    return pls;
}

// Sticky: does nothing if *ok is already false
static void * T1_gpu_new_compute_pls(
    void * func,
    const char * pls_name,
    u8 * ok,
    char * error_recip,
    const u32 error_recip_cap)
{
    if (!*ok) { return NULL; }
    
    void * error = NULL;
    void * pls = (void *)T1_objc_msg_2arg(
        ags->device,
        ags->sel_new_compute_pls_with_func,
        (uintptr_t)func,
        (uintptr_t)&error);
    
    if (pls == NULL) {
        char msg[128];
        T1_std_strcpy_cap(msg, 128, "Failed to create pipeline: ");
        T1_std_strcat_cap(msg, 128, pls_name);
        T1_gpu_shader_attempt_failed(
            msg, error, ok, error_recip, error_recip_cap);
    }
    
    return pls;
}

// Sticky: does nothing if *ok is already false
static void * T1_gpu_build_pipeline_state(
    s32 pipeline_i,
    u8 targets,
    void * funcs[2],
    u8 * ok,
    char * error_recip,
    const u32 error_recip_cap)
{
    if (!*ok) { return NULL; }

    T1Pipeline * p = T1_pipelines + pipeline_i;

    if (p->is_compute) {
        return T1_gpu_new_compute_pls(
            funcs[0], p->vertex_fn, ok, error_recip, error_recip_cap);
    }

    void * desc = T1_gpu_new_render_pipeline_descriptor(
        funcs[0], funcs[1], targets, p->blend);
    void * pls = T1_gpu_new_render_pls(
        desc, p->fragment_fn, ok, error_recip, error_recip_cap);
    T1_objc_msg(desc, ags->sel_release);
    return pls;
}

// Sticky: does nothing if *ok is already false
static void T1_gpu_get_pipeline_funcs(
    void * lib,
    s32 pipeline_i,
    void * recip[2],
    u8 * ok,
    char * error_recip,
    const u32 error_recip_cap)
{
    T1Pipeline * p = T1_pipelines + pipeline_i;
    recip[0] = T1_gpu_new_shader_func(
        lib, p->vertex_fn, ok, error_recip, error_recip_cap);
    recip[1] = p->is_compute ?
        NULL :
        T1_gpu_new_shader_func(
            lib, p->fragment_fn, ok, error_recip, error_recip_cap);
}

static void T1_gpu_release_pipeline(
    void * funcs[2],
    void * states[T1_GPU_TARGETS_SIZE])
{
    T1_gpu_release(&funcs[0]);
    T1_gpu_release(&funcs[1]);
    for (u32 t = 0; t < T1_GPU_TARGETS_SIZE; t++) {
        T1_gpu_release(&states[t]);
    }
}

void T1_os_gpu_pipeline_added(s32 pipeline_i, u8 * good) {
    *good = true;

    T1_assert(pipeline_i >= 0);
    T1_assert(pipeline_i < T1_PIPELINES_CAP);

    // checked when the shaders load
    if (ags == NULL || !ags->shaders_ready) { return; }

    T1_assert(ags->pipeline_funcs[pipeline_i][0] == NULL);

    void * pool = T1_objc_autorelease_pool_push();
    T1_gpu_get_pipeline_funcs(
        ags->lib,
        pipeline_i,
        ags->pipeline_funcs[pipeline_i],
        good,
        NULL,
        0);
    if (!*good) {
        T1_gpu_release_pipeline(
            ags->pipeline_funcs[pipeline_i],
            ags->pipeline_states[pipeline_i]);
    }
    T1_objc_autorelease_pool_pop(pool);
}

void T1_os_gpu_pipeline_deleted(s32 pipeline_i) {
    if (ags == NULL) { return; }

    T1_assert(pipeline_i >= 0);
    T1_assert(pipeline_i < T1_PIPELINES_CAP);

    T1_gpu_release_pipeline(
        ags->pipeline_funcs[pipeline_i],
        ags->pipeline_states[pipeline_i]);
}

/*
The state to draw (or compute) with pipeline_T1_id into targets, NULL for
T1_ID_NONE
*/
static void * T1_gpu_pipeline_state(
    const u32 pipeline_T1_id,
    u8 targets)
{
    if (pipeline_T1_id == T1_ID_NONE) { return NULL; }

    s32 i = T1_pipeline_T1_id_to_i(pipeline_T1_id);
    T1_assert(i >= 0);
    if (i < 0) { return NULL; }

    if (T1_pipelines[i].is_compute) { targets = 0; }

    if (ags->pipeline_states[i][targets] == NULL) {
        u8 ok = true;
        ags->pipeline_states[i][targets] = T1_gpu_build_pipeline_state(
            i, targets, ags->pipeline_funcs[i], &ok, NULL, 0);
        T1_assert(ok);
    }

    return ags->pipeline_states[i][targets];
}

void T1_os_gpu_try_shader_source(
    const char * shader_source,
    u8 * good,
    char * error_recip,
    const u32 error_recip_cap,
    char ** const fatal_error)
{
    *good = false;
    
    if (*fatal_error != 0) { return; }
    
    if (error_recip != NULL) {
        if (error_recip_cap < 512) {
            *fatal_error = "T1_os_gpu_try_shader_source() needs an error_recip_cap of at least 512";
            return;
        }
        error_recip[0] = '\0';
    }
    
    if (!ags || !ags->metal_active) {
        *fatal_error = "T1_gpu_try_shader_source() before T1_gpu_init()";
        return;
    }
    
    u8 ok = true;
    
    if (shader_source == NULL || shader_source[0] == '\0') {
        T1_gpu_shader_attempt_failed(
            "Empty shader source", NULL,
            &ok, error_recip, error_recip_cap);
        return;
    }
    
    void * pool = T1_objc_autorelease_pool_push();
    
    /*
    The macro settings T1 was compiled with, T1's embedded shader includes,
    then the client's shaders
    */
    u32 full_source_cap = (u32)(
        T1_std_strlen(T1_embedded_data_shader_settings) +
        T1_std_strlen(T1_embedded_data_shader_includes) +
        T1_std_strlen(shader_source) + 1);
    char * full_source = T1_mem_malloc_managed(full_source_cap);
    T1_std_strcpy_cap(
        full_source,
        full_source_cap,
        T1_embedded_data_shader_settings);
    T1_std_strcat_cap(
        full_source,
        full_source_cap,
        T1_embedded_data_shader_includes);
    T1_std_strcat_cap(
        full_source,
        full_source_cap,
        shader_source);
    
    // stringWithUTF8String: copies, so we can free right away
    void * nsstr_source = T1_objc_nsstring_construct(full_source);
    T1_mem_free_managed(full_source);
    
    // Metal also reports mere warnings through 'error', so only a
    // NULL library counts as a failure
    void * error = NULL;
    void * lib = (void *)T1_objc_msg_3arg(
        ags->device,
        ags->sel_new_lib_with_source_options_error,
        (uintptr_t)nsstr_source,
        /* options: */ (uintptr_t)NULL,
        /* error: */ (uintptr_t)&error);
    
    if (lib == NULL) {
        T1_gpu_shader_attempt_failed(
            "Failed to compile shaders", error,
            &ok, error_recip, error_recip_cap);
        T1_objc_autorelease_pool_pop(pool);
        return;
    }
    
    // Build every pipeline into locals first, so a failed attempt
    // leaves any previously loaded shaders untouched
    void * (* new_funcs)[2] = T1_mem_malloc_managed(
        sizeof(void *) * 2 * T1_PIPELINES_CAP);
    void * (* new_states)[T1_GPU_TARGETS_SIZE] = T1_mem_malloc_managed(
        sizeof(void *) * T1_GPU_TARGETS_SIZE * T1_PIPELINES_CAP);
    T1_std_memset(new_funcs, 0, sizeof(void *) * 2 * T1_PIPELINES_CAP);
    T1_std_memset(
        new_states,
        0,
        sizeof(void *) * T1_GPU_TARGETS_SIZE * T1_PIPELINES_CAP);

    for (s32 i = 0; i < (s32)T1_pipelines_size; i++) {
        if (T1_pipelines[i].T1_id == T1_ID_NONE) { continue; }

        T1_gpu_get_pipeline_funcs(
            lib, i, new_funcs[i], &ok, error_recip, error_recip_cap);

        // Rebuild the states in use. Pipelines without any get one, so
        // their shaders are known to link before they're first drawn with
        b8 any_states = false;
        for (u8 t = 0; t < T1_GPU_TARGETS_SIZE; t++) {
            if (ags->pipeline_states[i][t] == NULL) { continue; }
            any_states = true;
            new_states[i][t] = T1_gpu_build_pipeline_state(
                i, t, new_funcs[i], &ok, error_recip, error_recip_cap);
        }
        if (!any_states) {
            u8 t = T1_pipelines[i].is_compute ? 0 : (
                T1_GPU_TARGET_COLOR_0 |
                T1_GPU_TARGET_DEPTH);
            new_states[i][t] = T1_gpu_build_pipeline_state(
                i, t, new_funcs[i], &ok, error_recip, error_recip_cap);
        }
    }

    for (s32 i = 0; i < T1_PIPELINES_CAP; i++) {
        if (ok) {
            T1_gpu_release_pipeline(
                ags->pipeline_funcs[i], ags->pipeline_states[i]);
            ags->pipeline_funcs[i][0] = new_funcs[i][0];
            ags->pipeline_funcs[i][1] = new_funcs[i][1];
            for (u32 t = 0; t < T1_GPU_TARGETS_SIZE; t++) {
                ags->pipeline_states[i][t] = new_states[i][t];
            }
        } else {
            T1_gpu_release_pipeline(new_funcs[i], new_states[i]);
        }
    }
    T1_mem_free_managed(new_funcs);
    T1_mem_free_managed(new_states);

    if (!ok) {
        T1_gpu_release(&lib);
        T1_objc_autorelease_pool_pop(pool);
        return;
    }

    T1_gpu_release(&ags->lib);
    ags->lib = lib;
    ags->shaders_ready = true;

    *good = true;

    T1_objc_autorelease_pool_pop(pool);
}

u8 T1_os_gpu_shaders_ready(void) {
    return ags != NULL && ags->shaders_ready;
}


void T1_os_gpu_get_device_name(
    char * recipient,
    u32 recipient_cap)
{
    (void)recipient;
    (void)recipient_cap;
    
    T1_assert(0); // TODO: reimplement me!
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
    
    T1_assert(tex_array_i >=  0);
    T1_assert(tex_array_i <  31);
    u8 copy_prev = false;
    void * prev_copy = NULL;
    
    if (T1_tex_arrays[tex_array_i].deleted) {
        T1_gpu_release(&ags->metal_textures[tex_array_i]);
        return;
    } else if (
        ags->metal_textures[tex_array_i] == NULL ||
        T1_objc_msg(
            ags->metal_textures[tex_array_i],
            ags->sel_pixel_format) !=
                T1MTLPixelFormatRGBA8Unorm ||
            T1_tex_arrays[tex_array_i].single_img_width !=
                T1_objc_msg(
                    ags->metal_textures[tex_array_i],
                    ags->sel_width) ||
            T1_tex_arrays[tex_array_i].single_img_height !=
                T1_objc_msg(
                    ags->metal_textures[tex_array_i],
                    ags->sel_height))
    {
        T1_gpu_release(&ags->metal_textures[tex_array_i]);
        copy_prev = false;
    } else if (
        T1_tex_arrays[tex_array_i].images_size >
            T1_objc_msg(
                ags->metal_textures[tex_array_i],
                ags->sel_array_length))
    {
        copy_prev = true;
        prev_copy = ags->metal_textures[tex_array_i];
        ags->metal_textures[tex_array_i] = NULL;
    } else {
        return;
    }
    
    void * texture_descriptor =
        (void *)T1_objc_msg(
            ags->class_mtl_texture_desc,
            ags->sel_new);
    if (!texture_descriptor) { return; }
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
        T1MTLPixelFormatRGBA8Unorm);
    T1_objc_msg_1arg(
        texture_descriptor,
        ags->sel_set_storage_mode,
        T1MTLStorageModePrivate);
    
    // any texture can be sampled, drawn to and written by compute passes
    T1_objc_msg_1arg(
        texture_descriptor,
        ags->sel_set_usage,
        T1MTLTextureUsageShaderRead |
            T1MTLTextureUsageShaderWrite |
            T1MTLTextureUsageRenderTarget);
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
        tex_array_i == 0 ?
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
    
    T1_assert(ags->metal_textures[tex_array_i] == NULL);
    ags->metal_textures[tex_array_i] = (void *)T1_objc_msg_1arg(
        ags->device,
        ags->sel_new_tex_with_desc,
        (uintptr_t)texture_descriptor);
    T1_assert(ags->metal_textures[tex_array_i] != NULL);
    T1_gpu_release(&texture_descriptor);
    
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
        
        // We were its only owner once metal_textures moved on
        T1_gpu_release(&prev_copy);
    }
}

/*
Converts a window position (in points, origin bottom left, like mouse
events) to a pixel of the touch id texture (origin top left).
*/
static b8 T1_gpu_screen_to_touch_tex_pos(
    const f32 screen_x,
    const f32 screen_y,
    u32 * tex_x,
    u32 * tex_y)
{
    if (
        screen_x < 0 ||
        screen_y < 0 ||
        screen_x >= T1_global->window_wh[0] ||
        screen_y >= T1_global->window_wh[1])
    {
        return false;
    }
    
    if (ags->touch_tex == T1_TEX_NONE) { return false; }
    u32 rtt_width  = T1_tex_get_width(ags->touch_tex);
    u32 rtt_height = T1_tex_get_height(ags->touch_tex);
    u32 win_width  = (u32)T1_global->window_wh[0];
    u32 win_height = (u32)T1_global->window_wh[1];
    
    if (
        rtt_width < 1 || rtt_height < 1 ||
        win_width < 1 || win_height < 1)
    {
        return false;
    }
    
    *tex_x = (u32)((screen_x * rtt_width) / win_width);
    *tex_y = (u32)(((win_height - screen_y) * rtt_height) / win_height);
    
    if (*tex_x >= rtt_width ) { *tex_x = rtt_width - 1; }
    if (*tex_y >= rtt_height) { *tex_y = rtt_height - 1; }
    
    return true;
}

u32 T1_os_gpu_get_touch_id_at_screen_pos(
    f32 screen_x,
    f32 screen_y)
{
    if (!ags || !ags->touch_latest.valid) {
        return T1_TOUCH_ID_NONE;
    }
    
    u32 tex_x;
    u32 tex_y;
    if (!T1_gpu_screen_to_touch_tex_pos(
        screen_x,
        screen_y,
        &tex_x,
        &tex_y))
    {
        return T1_TOUCH_ID_NONE;
    }
    
    T1GPUTouchSample * sample = &ags->touch_latest;
    
    // Sampled before a resize, the pixels don't line up anymore
    if (
        ags->touch_tex == T1_TEX_NONE ||
        sample->tex_w != T1_tex_get_width(ags->touch_tex) ||
        sample->tex_h != T1_tex_get_height(ags->touch_tex))
    {
        return T1_TOUCH_ID_NONE;
    }
    
    if (
        tex_x <  sample->x ||
        tex_y <  sample->y ||
        tex_x >= sample->x + sample->w ||
        tex_y >= sample->y + sample->h)
    {
        return T1_TOUCH_ID_NONE;
    }
    
    u32 pixel_i =
        ((tex_y - sample->y) * sample->w) + (tex_x - sample->x);
    u8 * data = ags->touch_latest_bytes;
    
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
    T1_gpu_release(&ags->metal_textures[array_i]);
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
    T1_assert(texture_i >= 0);
    T1_assert(texture_array_i >= 0);
    T1_assert(texture_array_i < T1_TEXARRAYS_CAP);
    T1_assert(rgba_recipient != NULL);
    T1_assert(recipient_size != NULL);
    T1_assert(good != NULL);
    
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
    T1_assert(temp_buffer != NULL);
    
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
    
    /*
    - (void)
        copyFromTexture:(id<MTLTexture>) sourceTexture 
        sourceSlice:(NSUInteger) sourceSlice 
        sourceLevel:(NSUInteger) sourceLevel 
        sourceOrigin:(MTLOrigin) sourceOrigin 
        sourceSize:(MTLSize) sourceSize 
        toBuffer:(id<MTLBuffer>) destinationBuffer 
        destinationOffset:(NSUInteger) destinationOffset 
        destinationBytesPerRow:(NSUInteger) destinationBytesPerRow 
        destinationBytesPerImage:(NSUInteger) destinationBytesPerImage 
        options:(MTLBlitOption) options;
    */
    typedef void (* mtl_copy_texture_to_buffer_type)(
        void *, void *,
        /* copyFromTexture:(id<MTLTexture>) */ void *,
        /* sourceSlice:(NSUInteger): */ uintptr_t,
        /* sourceLevel:(NSUInteger): */ uintptr_t,
        /* sourceOrigin:(MTLOrigin): */ T1MTLOrigin,
        /* sourceSize:(MTLSize)    : */ T1MTLSize,
        /* toBuffer:(id<MTLBuffer>): */ void *,
        /* destinationOffset:(NSUInteger): */ uintptr_t,
        /* destinationBytesPerRow:(NSUInteger): */ uintptr_t,
        /* destinationBytesPerImage:(NSUInteger): */ uintptr_t);
    ((mtl_copy_texture_to_buffer_type)T1_objc_unsafe_get_raw_msg_send())(
        blit_encoder,
        ags->sel_copy_from_texture_to_buffer,
        texture,
        (uintptr_t)texture_i,
        0,
        T1_mtlorigin_make(0, 0, 0),
        T1_mtlsize_make(
            T1_objc_msg(texture, ags->sel_width),
            T1_objc_msg(texture, ags->sel_height),
            1),
        temp_buffer,
        0,
        bytes_per_row,
        bytes_per_image);
    
    ((mtl_copy_texture_to_buffer_type)T1_objc_unsafe_get_raw_msg_send())(
        blit_encoder,
        ags->sel_copy_from_texture_to_buffer,
        texture,
        (uintptr_t)texture_i,
        0,
        T1_mtlorigin_make(0, 0, 0),
        T1_mtlsize_make(
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
    if (!ags || !ags->metal_active) { return; }

    // no mipmaps for font
    T1_assert(texture_array_i != 0);

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
    s32 tex_slice_i)
{
    if (!ags || !ags->metal_active) { return; }
    
    u8 * rgba_freeable =
        T1_tex_arrays[tex_array_i].images[tex_slice_i].image.rgba_values_freeable;
    u8 * rgba_page_aligned =
        T1_tex_arrays[tex_array_i].images[tex_slice_i].image.rgba_values_page_aligned;
    
    if (!T1_tex_arrays[tex_array_i].images[tex_slice_i].image.never_delete_me) {
        T1_assert(rgba_freeable != NULL);
    } else {
        T1_assert(rgba_freeable == NULL);
    }
    T1_assert(rgba_page_aligned != NULL);
    
    T1_assert(tex_slice_i >= 0);
    T1_assert(tex_array_i >= 0);
    T1_assert(tex_array_i < T1_TEXARRAYS_CAP);
    
    T1_os_gpu_update_capacity_if_needed(tex_array_i);
    
    u32 img_width = T1_tex_arrays[tex_array_i].single_img_width;
    u32 img_height = T1_tex_arrays[tex_array_i].single_img_height;
    
    u32 temp_buf_cap = img_width * img_height * 4;
    T1_assert(temp_buf_cap > 0);
    while (temp_buf_cap % T1_mem_page_size != 0) {
        temp_buf_cap++;
    }
    
    T1_assert(T1_mem_is_page_aligned(rgba_page_aligned));
    
    vm_address_t vm_ptr;
    vm_size_t vm_size = temp_buf_cap;  // Already a page multiple
    kern_return_t err = vm_allocate(mach_task_self(), &vm_ptr, vm_size, VM_FLAGS_ANYWHERE);
    if (err != KERN_SUCCESS) {
        T1_assert(0);
        return;
    }
    
    u8 * temp_src_buf_ptr = (u8 *)vm_ptr;
    
    T1_std_memset(temp_src_buf_ptr, 0, temp_buf_cap);
    T1_std_memcpy(
        temp_src_buf_ptr,
        rgba_page_aligned,
        T1_tex_arrays[tex_array_i].images[tex_slice_i].
            image.rgba_values_size);
    
    void * temp_source_buf = (void *)T1_objc_msg_4arg(
        ags->device,
        ags->sel_new_buffer_with_bytes_no_copy,
        (uintptr_t)temp_src_buf_ptr,
        temp_buf_cap,
        T1MTLResourceStorageModeShared,
        (uintptr_t)NULL);
    
    if (temp_source_buf == NULL) {
        T1_assert(0);
        return;
    }
    
    void * combuf = (void *)T1_objc_msg(
        ags->command_queue,
        ags->sel_command_buffer);
    
    void * blit_copy_encoder = (void *)T1_objc_msg(
        combuf,
        ags->sel_blit_command_encoder);
    
    T1MTLSize   size   = T1_mtlsize_make(img_width, img_height, 1);
    T1MTLOrigin origin = T1_mtlorigin_make(0, 0, 0);
    
    typedef void (* copy_from_buf_src_offset_type)(
        void *, void *,
        /* copyFromBuffer:(id<MTLBuffer>): */ void *,
        /* sourceOffset:(NSUInteger): */ uintptr_t,
        /* sourceBytesPerRow:(NSUInteger): */ uintptr_t,
        /* sourceBytesPerImage:(NSUInteger): */ uintptr_t,
        /* sourceSize:(MTLSize): */ T1MTLSize,
        /* toTexture:(id<MTLTexture>): */ void *,
        /* destinationSlice:(NSUInteger): */ uintptr_t,
        /* destinationLevel:(NSUInteger): */ uintptr_t,
        /* destinationOrigin:(MTLOrigin): */ T1MTLOrigin);
    ((copy_from_buf_src_offset_type)T1_objc_unsafe_get_raw_msg_send())(
        blit_copy_encoder,
        ags->sel_copy_from_buffer_source_offset_source_bytes_per_row,
        temp_source_buf,
        0,
        img_width * 4,
        img_width * img_height * 4,
        size,
        ags->metal_textures[tex_array_i],
        (uintptr_t)tex_slice_i,
        0,
        origin);
    
    T1_objc_msg(blit_copy_encoder, ags->sel_end_encoding);
    
    T1_objc_msg(combuf, ags->sel_commit);
    T1_objc_msg(combuf, ags->sel_wait_until_completed);
    
    vm_deallocate(mach_task_self(), vm_ptr, vm_size);
    
    if (rgba_freeable != NULL) {
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
    
    /*
    - (void) copyFromBuffer:(id<MTLBuffer>) sourceBuffer 
           sourceOffset:(NSUInteger) sourceOffset 
               toBuffer:(id<MTLBuffer>) destinationBuffer 
      destinationOffset:(NSUInteger) destinationOffset 
                   size:(NSUInteger) size;
    */
    typedef void (* copy_from_buf_type)(void *, void *, void *, uintptr_t, void *, uintptr_t, uintptr_t);
    ((copy_from_buf_type)T1_objc_unsafe_get_raw_msg_send())(
        blit_copy_encoder,
        ags->sel_copy_from_buffer_source_offset_to_buffer,
        ags->locked_vertex_populator_buffer,
        0,
        ags->locked_vertex_buffer,
        0,
        T1_cpu_to_gpu_data->locked_vertices_alloc_size);
    
    T1_objc_msg(blit_copy_encoder, ags->sel_end_encoding);
    
    T1_objc_msg(combuf, ags->sel_commit);
}

void T1_os_gpu_copy_locked_materials(void)
{
    if (!ags || !ags->metal_active) { return; }
    
    T1_cpu_to_gpu_data->const_mats_size =
        all_mesh_materials->size;
    
    void * combuf = (void *)T1_objc_msg(
        ags->command_queue,
        ags->sel_command_buffer);
    
    void * blit_copy_encoder = (void *)T1_objc_msg(
        combuf,
        ags->sel_blit_command_encoder);
    
    typedef void (* copy_from_buf_type)(void *, void *, void *, uintptr_t, void *, uintptr_t, uintptr_t);
    ((copy_from_buf_type)T1_objc_unsafe_get_raw_msg_send())(
        blit_copy_encoder,
        ags->sel_copy_from_buffer_source_offset_to_buffer,
        ags->locked_matf32_populator_buffer,
        0,
        ags->locked_matf32_buffer,
        0,
        T1_cpu_to_gpu_data->const_matsf32_alloc_size);
    
    ((copy_from_buf_type)T1_objc_unsafe_get_raw_msg_send())(
        blit_copy_encoder,
        ags->sel_copy_from_buffer_source_offset_to_buffer,
        ags->locked_mats32_populator_buffer,
        0,
        ags->locked_mats32_buffer,
        0,
        T1_cpu_to_gpu_data->const_matss32_alloc_size);
    
    T1_objc_msg(
        blit_copy_encoder,
        ags->sel_end_encoding);
    
    T1_objc_msg(
        combuf,
        ags->sel_commit);
}

/*
Returns a fresh, autoreleased MTLRenderPassDescriptor with no attachments,
so each pass starts from a clean slate instead of inheriting the previous
pass's settings. It lives until the frame's autorelease pool is popped.
*/
static void * T1_gpu_new_render_pass_descriptor(void) {
    return (void *)T1_objc_msg(
        ags->class_mtl_render_pass_desc,
        ags->sel_render_pass_desc);
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
    if (ags->window_viewport.width < 10.0f) {
        ags->window_viewport.width = 10.0f;
    }
    if (ags->window_viewport.height < 10.0f) {
        ags->window_viewport.height = 10.0f;
    }
    
    /*
    These near/far values are the final viewport coordinates (after
    fragment shader), not to be confused with
    window_globals->projection_constants.near that's in our world space
    and much larger numbers
    */
    ags->window_viewport.znear = 0.001f;
    ags->window_viewport.zfar = 1.0f;
}

void T1_gpu_update_render_view_size(s32 at_i) {
    T1_assert(at_i >= 0);
    T1_assert(at_i < T1_RENDER_VIEW_CAP);
    
    void * pool = T1_objc_autorelease_pool_push();
    
    ags->render_viewports[at_i].originX = 0;
    ags->render_viewports[at_i].originY = 0;
    ags->render_viewports[at_i].width   =
        T1_render_views->cpu[at_i].width;
    ags->render_viewports[at_i].height  =
        T1_render_views->cpu[at_i].height;
    T1_assert(ags->render_viewports[at_i].width > 0.0f);
    T1_assert(ags->render_viewports[at_i].height > 0.0f);
    
    /*
    These near/far values are the final viewport coordinates (after
    fragment shader), not to be confused with
    window_globals->projection_constants.near that's in our world space
    and much larger numbers
    */
    ags->render_viewports[at_i].znear = 0.001f;
    ags->render_viewports[at_i].zfar = 1.0f;
    
    
    void * zbuffer_desc = (void *)T1_objc_msg(
        ags->class_mtl_texture_desc,
        ags->sel_new);
    if (!zbuffer_desc) {
        T1_objc_autorelease_pool_pop(pool);
        return;
    }
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
    
    T1_gpu_release(&ags->depth_textures[at_i]);
    ags->depth_textures[at_i] = (void *)T1_objc_msg_1arg(
        ags->device,
        ags->sel_new_tex_with_desc,
        (uintptr_t)zbuffer_desc);
    T1_gpu_release(&zbuffer_desc);
    
    T1_objc_autorelease_pool_pop(pool);
}

/*
Returns a new texture view (that the caller owns and must release) of one
slice of a texture array.
*/
static void * get_tex_slice(
    s32 at_array_i,
    s32 at_slice_i)
{
    T1_assert(at_array_i >= 0);
    T1_assert(at_array_i < T1_TEXARRAYS_CAP);
    T1_assert(at_slice_i >= 0);
    
    void * parent = ags->metal_textures[at_array_i];
    
    T1NSRange ns_range_level = T1_nsrange_make(
        0,
        T1_objc_msg(
            parent,
            ags->sel_mipmap_level_count));
    T1NSRange ns_range_slice = T1_nsrange_make(
        (uintptr_t)at_slice_i,
        1);
    
    void * retval = ((newTextureViewFuncType)T1_objc_unsafe_get_raw_msg_send())(
        parent,
        ags->sel_new_texture_view_with_pixel_format,
        T1_objc_msg(parent, ags->sel_pixel_format),
        T1MTLTextureType2D,
        ns_range_level,
        ns_range_slice);
    
    return retval;
}

static void T1_gpu_bind_texture_arrays(
    void * encoder,
    void * sel_set_texture_at_index)
{
    #if T1_TEXTURES_ACTIVE == T1_ACTIVE
    for (
        u32 i = 0;
        i < T1_TEXARRAYS_CAP;
        i++)
    {
        if (ags->metal_textures[i] != NULL) {
            T1_objc_msg_2arg(encoder,
                sel_set_texture_at_index,
                (uintptr_t)ags->metal_textures[i],
                i);
        }
    }
    #elif T1_TEXTURES_ACTIVE == T1_INACTIVE
    T1_objc_msg_2arg(encoder,
        sel_set_texture_at_index,
        (uintptr_t)ags->metal_textures[0],
        0);
    #else
    #error
    #endif
}

static void set_defaults_for_encoder(
    void * encoder,
    const u32 cam_i)
{
    T1_assert(cam_i < T1_RENDER_VIEW_CAP);

    T1_objc_msg_1arg(encoder,
        ags->sel_set_depth_clip_mode, T1MTLDepthClipModeClip);
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
                (uintptr_t)ags->locked_mats32_buffer,
            /* offset: */
                0,
            /* atIndex: */
                8);
}

/*
What a pass's T1Tex draws into: the finished frame for T1_TEX_WINDOW, else
a new view of the texture's slice that *owned says the caller releases.
NULL if the texture doesn't exist (yet).
*/
static void * T1_gpu_pass_target(
    const u32 tex,
    b8 * owned)
{
    *owned = false;

    if (tex == T1_TEX_WINDOW) {
        return ags->final_texture;
    }

    s32 array_i = T1_TEX_ARRAY_I(tex);
    s32 slice_i = T1_TEX_SLICE_I(tex);
    T1_assert(array_i >= 0);
    T1_assert(array_i < T1_TEXARRAYS_CAP);
    if (
        array_i >= T1_TEXARRAYS_CAP ||
        ags->metal_textures[array_i] == NULL)
    {
        return NULL;
    }

    *owned = true;
    return get_tex_slice(array_i, slice_i);
}

typedef void (* T1GPUCopyBufFn)(
    void *, void *, void *, uintptr_t, void *, uintptr_t, uintptr_t);

// Encodes the waiting T1_buf_write()s into combuf, before anything else
static void T1_gpu_encode_buf_writes(void * combuf) {
    if (ags->buf_writes_size == 0) { return; }
    
    void * blit = (void *)T1_objc_msg(combuf, ags->sel_blit_command_encoder);
    for (u32 i = 0; i < ags->buf_writes_size; i++) {
        T1GPUBufWrite * w = ags->buf_writes + i;
        T1_assert(ags->bufs[w->buf_i] != NULL);
        ((T1GPUCopyBufFn)T1_objc_unsafe_get_raw_msg_send())(
            blit,
            ags->sel_copy_from_buffer_source_offset_to_buffer,
            /* copyFromBuffer: */ w->staging,
            /* sourceOffset: */ 0,
            /* toBuffer: */ ags->bufs[w->buf_i],
            /* destinationOffset: */ (uintptr_t)w->offset,
            /* size: */ (uintptr_t)w->size);
        // the command buffer keeps it until the copy is done
        T1_gpu_release(&w->staging);
    }
    T1_objc_msg(blit, ags->sel_end_encoding);
    
    ags->buf_writes_size = 0;
}

void T1_os_gpu_buf_new(u32 buf_i, u64 size, u8 * good) {
    *good = 0;
    T1_assert(buf_i < T1_BUFS_CAP);
    if (!ags || !ags->metal_active || buf_i >= T1_BUFS_CAP) { return; }
    T1_assert(ags->bufs[buf_i] == NULL);
    
    // Metal zero-fills new buffers
    ags->bufs[buf_i] = (void *)T1_objc_msg_2arg(
        ags->device,
        ags->sel_new_buf_with_len_options,
        (uintptr_t)size,
        T1MTLResourceStorageModePrivate);
    *good = ags->bufs[buf_i] != NULL;
}

void T1_os_gpu_buf_write(
    u32 buf_i,
    u64 offset,
    const void * data,
    u64 size)
{
    if (!ags || !ags->metal_active) { return; }
    T1_assert(buf_i < T1_BUFS_CAP && ags->bufs[buf_i] != NULL);
    
    // Too many waiting: apply them now, in a command buffer of their own.
    // It's committed first, so it still runs before the next frame.
    if (ags->buf_writes_size >= T1_GPU_BUF_WRITES_CAP) {
        void * combuf = (void *)T1_objc_msg(
            ags->command_queue,
            ags->sel_command_buffer);
        T1_gpu_encode_buf_writes(combuf);
        T1_objc_msg(combuf, ags->sel_commit);
    }
    
    void * staging = (void *)T1_objc_msg_3arg(
        ags->device,
        ags->sel_new_buffer_with_bytes_length_options,
        (uintptr_t)data,
        (uintptr_t)size,
        T1MTLResourceStorageModeShared);
    T1_assert(staging != NULL);
    if (staging == NULL) { return; }
    
    T1GPUBufWrite * w = ags->buf_writes + ags->buf_writes_size;
    ags->buf_writes_size += 1;
    w->staging = staging;
    w->offset = offset;
    w->size = size;
    w->buf_i = buf_i;
}

u32 T1_os_gpu_buf_read_start(u32 buf_i, u64 offset, u64 size) {
    if (!ags || !ags->metal_active) { return T1_BUF_READ_NONE; }
    T1_assert(buf_i < T1_BUFS_CAP && ags->bufs[buf_i] != NULL);
    
    u32 read_i = 0;
    while (
        read_i < T1_BUF_READS_CAP &&
        ags->buf_reads[read_i].state != T1_GPU_BUF_READ_FREE)
    {
        read_i++;
    }
    if (read_i == T1_BUF_READS_CAP) { return T1_BUF_READ_NONE; }
    
    void * staging = (void *)T1_objc_msg_2arg(
        ags->device,
        ags->sel_new_buf_with_len_options,
        (uintptr_t)size,
        T1MTLResourceStorageModeShared);
    T1_assert(staging != NULL);
    if (staging == NULL) { return T1_BUF_READ_NONE; }
    
    T1GPUBufRead * r = ags->buf_reads + read_i;
    r->staging = staging;
    r->combuf = NULL;
    r->offset = offset;
    r->size = size;
    r->buf_i = buf_i;
    r->state = T1_GPU_BUF_READ_QUEUED;
    
    return read_i;
}

// Copies the queued reads' bytes to their staging, after the frame's passes
static void T1_gpu_encode_buf_reads(void * combuf) {
    void * blit = NULL;
    
    for (u32 i = 0; i < T1_BUF_READS_CAP; i++) {
        T1GPUBufRead * r = ags->buf_reads + i;
        if (r->state != T1_GPU_BUF_READ_QUEUED) { continue; }
        
        // its buffer was deleted: staging stays zeroed
        if (ags->bufs[r->buf_i] != NULL) {
            if (blit == NULL) {
                blit = (void *)T1_objc_msg(
                    combuf, ags->sel_blit_command_encoder);
            }
            ((T1GPUCopyBufFn)T1_objc_unsafe_get_raw_msg_send())(
                blit,
                ags->sel_copy_from_buffer_source_offset_to_buffer,
                /* copyFromBuffer: */ ags->bufs[r->buf_i],
                /* sourceOffset: */ (uintptr_t)r->offset,
                /* toBuffer: */ r->staging,
                /* destinationOffset: */ 0,
                /* size: */ (uintptr_t)r->size);
        }
        
        r->combuf = (void *)T1_objc_msg(combuf, ags->sel_retain);
        r->state = T1_GPU_BUF_READ_ENCODED;
    }
    
    if (blit != NULL) {
        T1_objc_msg(blit, ags->sel_end_encoding);
    }
}

u8 T1_os_gpu_buf_read_finish(u32 read_i, void * recip) {
    if (!ags || read_i >= T1_BUF_READS_CAP) { return 0; }
    
    T1GPUBufRead * r = ags->buf_reads + read_i;
    T1_assert(r->state != T1_GPU_BUF_READ_FREE);
    if (r->state != T1_GPU_BUF_READ_ENCODED) { return 0; }
    
    if (
        T1_objc_msg(r->combuf, ags->sel_status) <
            T1MTLCommandBufferStatusCompleted)
    {
        return 0;
    }
    
    T1_std_memcpy(
        recip,
        (void *)T1_objc_msg(r->staging, ags->sel_contents),
        r->size);
    
    T1_gpu_release(&r->staging);
    T1_gpu_release(&r->combuf);
    r->state = T1_GPU_BUF_READ_FREE;
    
    return 1;
}

void T1_os_gpu_buf_delete(u32 buf_i) {
    if (!ags || buf_i >= T1_BUFS_CAP) { return; }
    
    // Its waiting writes go too, the slot may get a new buffer before
    // they'd run
    u32 kept = 0;
    for (u32 i = 0; i < ags->buf_writes_size; i++) {
        if (ags->buf_writes[i].buf_i == buf_i) {
            T1_gpu_release(&ags->buf_writes[i].staging);
        } else {
            ags->buf_writes[kept] = ags->buf_writes[i];
            kept += 1;
        }
    }
    ags->buf_writes_size = kept;
    
    // frames in flight keep their own reference
    T1_gpu_release(&ags->bufs[buf_i]);
}

/*
Binds the frame's T1GPUFrameConsts and the pass's "buf[k]"s, set_buffer
being the encoder's set...Buffer:offset:atIndex:
*/
static void T1_gpu_bind_pass_buffers(
    void * enc,
    void * set_buffer,
    const T1Pass * pass)
{
    T1_objc_msg_3arg(
        enc,
        set_buffer,
        (uintptr_t)ags->frame_consts_buffers[ags->frame_i],
        0,
        T1_FRAME_CONSTS_I);
    
    for (u32 k = 0; k < T1_PASS_BUFS_CAP; k++) {
        u32 buf = pass->u32s.buf[k];
        if (buf == T1_BUF_NONE) { continue; }
        
        T1_assert(buf < T1_BUFS_CAP && ags->bufs[buf] != NULL);
        if (buf >= T1_BUFS_CAP || ags->bufs[buf] == NULL) { continue; }
        
        T1_objc_msg_3arg(
            enc,
            set_buffer,
            (uintptr_t)ags->bufs[buf],
            0,
            T1_BUFS_I + k);
    }
}

static void T1_gpu_encode_clear_pass(
    const T1Pass * pass,
    void * combuf)
{
    b8 owned;
    void * tex = T1_gpu_pass_target(pass->u32s.color_tex[0], &owned);
    if (tex == NULL) { return; }

    void * desc = T1_gpu_new_render_pass_descriptor();
    void * color_attachments = (void *)T1_objc_msg(
        desc, ags->sel_color_attachments);
    void * color_attachment_0 = (void *)T1_objc_msg_1arg(
        color_attachments, ags->sel_object_at_indexed_subscript, 0);
    T1_objc_msg_1arg(
        color_attachment_0,
        ags->sel_set_texture,
        (uintptr_t)tex);
    T1_objc_msg_1arg(
        color_attachment_0,
        ags->sel_set_load_action,
        T1MTLLoadActionClear);
    ((setClearColorFuncType)T1_objc_unsafe_get_raw_msg_send())(
        color_attachment_0,
        ags->sel_set_clear_color,
        T1_mtl_clearcolor_make(
            pass->clear_rgba[0],
            pass->clear_rgba[1],
            pass->clear_rgba[2],
            pass->clear_rgba[3]));
    T1_objc_msg_1arg(
        color_attachment_0,
        ags->sel_set_store_action,
        T1MTLStoreActionStore);

    void * enc = (void *)T1_objc_msg_1arg(
        combuf,
        ags->sel_render_cmd_enc_with_desc,
        (uintptr_t)desc);
    T1_objc_msg(enc, ags->sel_end_encoding);

    if (owned) { T1_gpu_release(&tex); }
}

static void T1_gpu_encode_draw_pass(
    const T1Pass * pass,
    const T1GPUFrame * f,
    void * combuf)
{
    s32 pipeline_i = T1_pipeline_T1_id_to_i(pass->pipeline);
    T1_assert(pipeline_i >= 0);
    if (pipeline_i < 0) { return; }

    s32 cam_i = T1_render_view_T1_id_to_i(pass->u32s.cam);
    // a camera is optional for these, the rest is drawn from one
    b8 cam_optional =
        pass->sources == T1_DRAW_FULLSCREEN ||
        pass->sources == T1_DRAW_PROCEDURAL;
    T1_assert(cam_i >= 0 || cam_optional);
    if (cam_i < 0 && !cam_optional) { return; }

    u32 verts_size = 0;
    if (pass->sources & T1_DRAW_ZSPRITES) {
        if (!T1_global->draw_triangles) { return; }
        if (pass->sources & T1_DRAW_OPAQUE) {
            verts_size +=
                f->vert_ranges[T1_VERT_RANGE_OPAQUE].verts_size;
        }
        if (pass->sources & T1_DRAW_BLENDED) {
            verts_size +=
                f->vert_ranges[T1_VERT_RANGE_BLENDED].verts_size;
        }
        if (pass->sources & T1_DRAW_BLOOMING) {
            verts_size +=
                f->vert_ranges[T1_VERT_RANGE_BLOOMING].verts_size;
        }
    } else if (pass->sources == T1_DRAW_TEXQUADS) {
        verts_size = f->flat_tex_quads_size * 6;
    } else if (pass->sources == T1_DRAW_BILLBOARDS) {
        verts_size = f->flat_bb_quads_size * 6;
    } else if (pass->sources == T1_DRAW_FULLSCREEN) {
        verts_size = 6;
    } else if (pass->sources == T1_DRAW_PROCEDURAL) {
        verts_size = pass->u32s.vertex_count;
    } else {
        T1_assert(0);
    }

    if (verts_size < 1) { return; }

    void * desc = T1_gpu_new_render_pass_descriptor();
    void * color_attachments = (void *)T1_objc_msg(
        desc, ags->sel_color_attachments);

    void * targets[T1_PASS_COLOR_TEXS_CAP];
    b8 owned[T1_PASS_COLOR_TEXS_CAP];
    u8 target_bits = 0;
    for (u32 k = 0; k < T1_PASS_COLOR_TEXS_CAP; k++) {
        targets[k] = NULL;
        owned[k] = false;

        u32 tex = pass->u32s.color_tex[k];
        if (tex == T1_TEX_NONE) { continue; }

        // the window can only be the first target
        T1_assert(tex != T1_TEX_WINDOW || k == 0);

        targets[k] = T1_gpu_pass_target(tex, &owned[k]);
        if (targets[k] == NULL) { continue; }

        // every target must be the camera's size
        T1_assert(
            cam_i < 0 ||
            T1_objc_msg(targets[k], ags->sel_width) ==
                T1_render_views->cpu[cam_i].width);
        T1_assert(
            cam_i < 0 ||
            T1_objc_msg(targets[k], ags->sel_height) ==
                T1_render_views->cpu[cam_i].height);

        target_bits |= (u8)(T1_GPU_TARGET_COLOR_0 << k);
        if (tex == T1_TEX_WINDOW) {
            target_bits |= T1_GPU_TARGET_WINDOW;
        }

        void * color_attachment = (void *)T1_objc_msg_1arg(
            color_attachments, ags->sel_object_at_indexed_subscript, k);
        T1_objc_msg_1arg(
            color_attachment,
            ags->sel_set_texture,
            (uintptr_t)targets[k]);
        T1_objc_msg_1arg(
            color_attachment,
            ags->sel_set_load_action,
            T1MTLLoadActionLoad);
        T1_objc_msg_1arg(
            color_attachment,
            ags->sel_set_store_action,
            T1MTLStoreActionStore);
    }

    // A camera's depth buffer is cleared by its first pass of the frame
    if (cam_i >= 0) {
        // made (again) for cameras that are new, or new since T1_gpu_init()
        if (
            ags->depth_textures[cam_i] == NULL ||
            T1_objc_msg(ags->depth_textures[cam_i], ags->sel_width) !=
                T1_render_views->cpu[cam_i].width ||
            T1_objc_msg(ags->depth_textures[cam_i], ags->sel_height) !=
                T1_render_views->cpu[cam_i].height)
        {
            T1_gpu_update_render_view_size(cam_i);
        }
        T1_assert(ags->depth_textures[cam_i] != NULL);
        target_bits |= T1_GPU_TARGET_DEPTH;

        void * depth_attachm = (void *)T1_objc_msg(
            desc, ags->sel_depth_attachment);
        if (!ags->cam_depth_cleared[cam_i]) {
            T1_objc_msg_1arg(
                depth_attachm,
                ags->sel_set_load_action,
                T1MTLLoadActionClear);
            T1_objc_msg_1f64(depth_attachm, ags->sel_set_clear_depth, 1.0);
            ags->cam_depth_cleared[cam_i] = true;
        } else {
            T1_objc_msg_1arg(
                depth_attachm,
                ags->sel_set_load_action,
                T1MTLLoadActionLoad);
        }
        T1_objc_msg_1arg(
            depth_attachm,
            ags->sel_set_store_action,
            T1MTLStoreActionStore);
        T1_objc_msg_1arg(
            depth_attachm,
            ags->sel_set_texture,
            (uintptr_t)ags->depth_textures[cam_i]);
    }

    // nothing to draw into
    if ((target_bits & ~T1_GPU_TARGET_WINDOW) == 0) { return; }

    void * enc = (void *)T1_objc_msg_1arg(
        combuf,
        ags->sel_render_cmd_enc_with_desc,
        (uintptr_t)desc);

    T1_objc_msg_1arg(
        enc,
        ags->sel_set_render_pls,
        (uintptr_t)T1_gpu_pipeline_state(pass->pipeline, target_bits));
    if (cam_i >= 0) {
        T1_objc_msg_1arg(enc,
            ags->sel_set_depth_stencil_state,
            (uintptr_t)ags->opaque_depth_stencil_state);
    }
    T1_objc_msg_1arg(
        enc,
        ags->sel_set_cull_mode,
        T1_pipelines[pipeline_i].cull == T1_CULL_BACK ?
            T1MTLCullModeBack :
        T1_pipelines[pipeline_i].cull == T1_CULL_FRONT ?
            T1MTLCullModeFront :
            T1MTLCullModeNone);
    T1_objc_msg_1arg(
        enc,
        ags->sel_set_front_facing_winding,
        T1MTLWindingCounterClockwise);

    u32 rv_i = cam_i >= 0 ? (u32)cam_i : 0;

    T1_objc_msg_3arg(enc,
        ags->sel_set_vertex_bytes_length_atindex,
        (uintptr_t)(void *)pass->u32s.read_tex,
        sizeof(u32) * T1_READ_TEXS_CAP,
        T1_READ_TEXS_BUFFER_I);
    T1_objc_msg_3arg(enc,
        ags->sel_set_fragment_bytes_length_atindex,
        (uintptr_t)(void *)pass->u32s.read_tex,
        sizeof(u32) * T1_READ_TEXS_CAP,
        T1_READ_TEXS_BUFFER_I);
    T1_gpu_bind_texture_arrays(enc, ags->sel_set_fragment_texture_at_index);
    T1_gpu_bind_pass_buffers(
        enc, ags->sel_set_vertex_buffer_offset_atindex, pass);
    T1_gpu_bind_pass_buffers(
        enc, ags->sel_set_fragment_buffer_offset_atindex, pass);

    if (pass->sources & T1_DRAW_ZSPRITES) {
        set_defaults_for_encoder(enc, rv_i);

        const u32 range_bits[T1_VERT_RANGES_SIZE] = {
            T1_DRAW_OPAQUE,
            T1_DRAW_BLENDED,
            T1_DRAW_BLOOMING,
        };
        for (u32 r = 0; r < T1_VERT_RANGES_SIZE; r++) {
            if (
                !(pass->sources & range_bits[r]) ||
                f->vert_ranges[r].verts_size < 1)
            {
                continue;
            }
            T1_assert(
                f->vert_ranges[r].vert_i +
                    f->vert_ranges[r].verts_size <
                        T1_MAX_VERTS_PER_BUFFER);
            T1_assert(f->vert_ranges[r].verts_size % 3 == 0);
            T1_objc_msg_3arg(
                enc,
                ags->sel_draw_primitives_vertex_start_vertex_count,
                /* drawPrimitives: */ T1MTLPrimitiveTypeTriangle,
                /* vertexStart   : */ (uintptr_t)f->vert_ranges[r].vert_i,
                /* vertexCount   : */
                    (uintptr_t)f->vert_ranges[r].verts_size);
        }
    } else if (pass->sources == T1_DRAW_TEXQUADS) {
        T1_objc_msg_3arg(enc,
            ags->sel_set_vertex_buffer_offset_atindex,
                /* setVertexBuffer: */
                    (uintptr_t)ags->flat_texquad_buffers[ags->frame_i],
                /* offset: */ 0,
                /* atIndex: */ 0);
        T1_objc_msg_3arg(enc,
            ags->sel_set_vertex_buffer_offset_atindex,
                /* setVertexBuffer: */
                    (uintptr_t)ags->matrix_buffers[ags->frame_i],
                /* offset: */ 0,
                /* atIndex: */ 2);
        T1_objc_msg_3arg(enc,
            ags->sel_set_vertex_buffer_offset_atindex,
                /* setVertexBuffer: */
                    (uintptr_t)ags->cam_buffers[ags->frame_i],
                /* offset: */ 0,
                /* atIndex: */ 3);
        T1_objc_msg_3arg(enc,
            ags->sel_set_vertex_bytes_length_atindex,
                /* setVertexBytes: */ (uintptr_t)(void *)&rv_i,
                /* length: */ sizeof(u32),
                /* atIndex: */ 4);
        T1_objc_msg_3arg(
            enc,
            ags->sel_draw_primitives_vertex_start_vertex_count,
            /* drawPrimitives: */ T1MTLPrimitiveTypeTriangle,
            /* vertexStart: */ 0,
            /* vertexCount: */ (uintptr_t)verts_size);
    } else if (pass->sources == T1_DRAW_BILLBOARDS) {
        T1_objc_msg_3arg(
            enc,
            ags->sel_set_vertex_buffer_offset_atindex,
            /* setVertexBuffer: */
                (uintptr_t)ags->flat_quad_buffers[ags->frame_i],
            /* offset: */ 0,
            /* atIndex: */ 2);
        T1_objc_msg_3arg(
            enc,
            ags->sel_set_vertex_buffer_offset_atindex,
            /* setVertexBuffer: */
                (uintptr_t)ags->cam_buffers[ags->frame_i],
            /* offset: */ 0,
            /* atIndex: */ 3);
        T1_objc_msg_3arg(enc,
            ags->sel_set_vertex_bytes_length_atindex,
                /* setVertexBytes: */ (uintptr_t)(void *)&rv_i,
                /* length: */ sizeof(u32),
                /* atIndex: */ 4);
        T1_objc_msg_3arg(
            enc,
            ags->sel_draw_primitives_vertex_start_vertex_count,
            /* drawPrimitives: */ T1MTLPrimitiveTypeTriangle,
            /* vertexStart: */ 0,
            /* vertexCount: */ (uintptr_t)verts_size);
    } else if (pass->sources == T1_DRAW_FULLSCREEN) {
        T1_objc_msg_3arg(
            enc,
            ags->sel_set_vertex_bytes_length_atindex,
            (uintptr_t)ags->quad_vertices,
            sizeof(T1PostProcessingVertex)*6,
            0);
        T1_objc_msg_3arg(
            enc,
            ags->sel_draw_primitives_vertex_start_vertex_count,
            /* drawPrimitives: */ T1MTLPrimitiveTypeTriangle,
            /* vertexStart: */ 0,
            /* vertexCount: */ 6);
    } else if (pass->sources == T1_DRAW_PROCEDURAL) {
        if (cam_i >= 0) {
            void * set_buffer[2] = {
                ags->sel_set_vertex_buffer_offset_atindex,
                ags->sel_set_fragment_buffer_offset_atindex,
            };
            void * set_bytes[2] = {
                ags->sel_set_vertex_bytes_length_atindex,
                ags->sel_set_fragment_bytes_length_atindex,
            };
            for (u32 stage = 0; stage < 2; stage++) {
                T1_objc_msg_3arg(enc,
                    set_buffer[stage],
                    (uintptr_t)ags->cam_buffers[ags->frame_i],
                    0,
                    3);
                T1_objc_msg_3arg(enc,
                    set_bytes[stage],
                    (uintptr_t)(void *)&rv_i,
                    sizeof(u32),
                    4);
            }
        }
        T1_objc_msg_3arg(
            enc,
            ags->sel_draw_primitives_vertex_start_vertex_count,
            /* drawPrimitives: */ T1MTLPrimitiveTypeTriangle,
            /* vertexStart: */ 0,
            /* vertexCount: */ (uintptr_t)verts_size);
    }

    T1_objc_msg(enc, ags->sel_end_encoding);

    for (u32 k = 0; k < T1_PASS_COLOR_TEXS_CAP; k++) {
        if (owned[k]) { T1_gpu_release(&targets[k]); }
    }
}

static void T1_gpu_encode_compute_pass(
    const T1Pass * pass,
    void * combuf)
{
    // the window can't be written by compute shaders
    T1_assert(pass->u32s.write_tex != T1_TEX_WINDOW);
    if (pass->u32s.write_tex == T1_TEX_WINDOW) { return; }
    
    // a grid of threads, a texture to write, or both
    b8 has_threads = pass->u32s.threads[0] > 0;
    T1_assert(has_threads || pass->u32s.write_tex != T1_TEX_NONE);
    
    b8 owned = false;
    void * out = NULL;
    if (pass->u32s.write_tex != T1_TEX_NONE) {
        out = T1_gpu_pass_target(pass->u32s.write_tex, &owned);
        if (out == NULL) { return; }
    } else if (!has_threads) {
        return;
    }

    void * enc = (void *)T1_objc_msg(combuf, ags->sel_compute_cmd_enc);

    T1_objc_msg_1arg(
        enc,
        ags->sel_set_compute_pls,
        (uintptr_t)T1_gpu_pipeline_state(pass->pipeline, 0));

    T1_gpu_bind_texture_arrays(enc, ags->sel_set_texture_at_index);
    if (out != NULL) {
        T1_objc_msg_2arg(
            enc,
            ags->sel_set_texture_at_index,
            (uintptr_t)out,
            T1_WRITE_TEX_I);
    }
    T1_objc_msg_3arg(
        enc,
        ags->sel_set_bytes_length_atindex,
        (uintptr_t)(void *)pass->u32s.read_tex,
        sizeof(u32) * T1_READ_TEXS_CAP,
        T1_READ_TEXS_BUFFER_I);
    T1_gpu_bind_pass_buffers(enc, ags->sel_set_buffer_offset_atindex, pass);

    // "threads", else one thread per pixel of the texture written to
    T1MTLSize grid;
    if (has_threads) {
        grid = T1_mtlsize_make(
            pass->u32s.threads[0],
            pass->u32s.threads[1] > 0 ? pass->u32s.threads[1] : 1,
            pass->u32s.threads[2] > 0 ? pass->u32s.threads[2] : 1);
    } else {
        grid = T1_mtlsize_make(
            T1_objc_msg(out, ags->sel_width),
            T1_objc_msg(out, ags->sel_height),
            1);
    }
    // 256 threads per group, along the dimensions the grid uses
    T1MTLSize threadgroup =
        grid.height == 1 && grid.depth == 1 ?
            T1_mtlsize_make(256, 1, 1) :
        grid.depth == 1 ?
            T1_mtlsize_make(16, 16, 1) :
            T1_mtlsize_make(8, 8, 4);
    ((dispatchThreadsFuncType)T1_objc_unsafe_get_raw_msg_send())(
        enc,
        ags->sel_dispatch_threads_threadsperthreadgroup,
        grid,
        threadgroup);

    T1_objc_msg(enc, ags->sel_end_encoding);

    if (owned) { T1_gpu_release(&out); }
}

/*
Once a sample's frame is done, copies it to touch_latest if it's the newest
we've seen and lets go of its command buffer. With wait == false this does
nothing for frames that are still in flight.
*/
static void T1_gpu_retire_touch_sample(
    T1GPUTouchSample * sample,
    const b8 wait)
{
    if (sample->combuf == NULL) {
        return;
    }
    
    if (wait) {
        T1_objc_msg(sample->combuf, ags->sel_wait_until_completed);
    }
    
    uintptr_t status = T1_objc_msg(sample->combuf, ags->sel_status);
    if (status < T1MTLCommandBufferStatusCompleted) {
        T1_assert(!wait);
        return;
    }
    
    // An invalid sample still replaces an older one, so we don't keep
    // reporting touch ids that are no longer on screen
    if (
        status == T1MTLCommandBufferStatusCompleted &&
        sample->frame_no > ags->touch_latest.frame_no)
    {
        if (sample->valid) {
            T1_assert(sample->w * sample->h * 4 <=
                sizeof(ags->touch_latest_bytes));
            T1_std_memcpy(
                /* void * dest: */
                    ags->touch_latest_bytes,
                /* const void * src: */
                    (void *)T1_objc_msg(sample->buffer, ags->sel_contents),
                /* u64 n_bytes: */
                    sample->w * sample->h * 4);
        }
        ags->touch_latest = *sample;
        ags->touch_latest.buffer = NULL;
        ags->touch_latest.combuf = NULL;
    }
    
    T1_objc_msg(sample->combuf, ags->sel_release);
    sample->combuf = NULL;
}

/*
Copies the touch ids in a small square around the mouse to the sample's
buffer, to be read by the CPU after this frame is done.
*/
static void T1_gpu_encode_touch_sample(
    void * combuf,
    T1GPUTouchSample * sample)
{
    T1_assert(sample->combuf == NULL);
    
    if (sample->buffer == NULL) {
        sample->buffer = (void *)T1_objc_msg_2arg(
            ags->device,
            ags->sel_new_buf_with_len_options,
            T1_GPU_TOUCH_SAMPLE_DIM * T1_GPU_TOUCH_SAMPLE_DIM * 4,
            T1MTLResourceStorageModeShared);
        T1_assert(sample->buffer != NULL);
    }
    
    // Retained so we can check on this frame later
    sample->combuf = (void *)T1_objc_msg(combuf, ags->sel_retain);
    sample->frame_no = ++ags->frames_encoded;
    sample->valid = false;
    
    if (
        ags->touch_tex == T1_TEX_NONE ||
        ags->metal_textures[T1_tex_to_array_i(ags->touch_tex)] == NULL)
    {
        return;
    }
    sample->tex_w = T1_tex_get_width(ags->touch_tex);
    sample->tex_h = T1_tex_get_height(ags->touch_tex);
    
    u32 mouse_x;
    u32 mouse_y;
    if (
        sample->buffer == NULL ||
        sample->tex_w < 1 ||
        sample->tex_h < 1 ||
        !T1_gpu_screen_to_touch_tex_pos(
            T1_io_get_pos_x_this_frame(T1_IO_MOUSE),
            T1_io_get_pos_y_this_frame(T1_IO_MOUSE),
            &mouse_x,
            &mouse_y))
    {
        return;
    }
    
    // A square centered on the mouse, shifted to stay inside the texture
    sample->w = sample->tex_w < T1_GPU_TOUCH_SAMPLE_DIM ?
        sample->tex_w : T1_GPU_TOUCH_SAMPLE_DIM;
    sample->h = sample->tex_h < T1_GPU_TOUCH_SAMPLE_DIM ?
        sample->tex_h : T1_GPU_TOUCH_SAMPLE_DIM;
    sample->x = mouse_x > sample->w / 2 ? mouse_x - sample->w / 2 : 0;
    sample->y = mouse_y > sample->h / 2 ? mouse_y - sample->h / 2 : 0;
    if (sample->x + sample->w > sample->tex_w) {
        sample->x = sample->tex_w - sample->w;
    }
    if (sample->y + sample->h > sample->tex_h) {
        sample->y = sample->tex_h - sample->h;
    }
    
    void * blit_enc = (void *)T1_objc_msg(
        combuf, ags->sel_blit_command_encoder);
    
    typedef void (* copy_from_tex_to_buf_type)(
        void *, void *,
        /* copyFromTexture:(id<MTLTexture>) */ void *,
        /* sourceSlice:(NSUInteger)    : */ uintptr_t,
        /* sourceLevel:(NSUInteger)    : */ uintptr_t,
        /* sourceOrigin:(MTLOrigin)    : */ T1MTLOrigin,
        /* sourceSize:(MTLSize)        : */ T1MTLSize,
        /* toBuffer:(id<MTLBuffer>)    : */ void *,
        /* destOffset:(NSUInteger)     : */ uintptr_t,
        /* destBytesPerRow:(NSUInteger): */ uintptr_t,
        /* destBytesPerImg:(NSUInteger): */ uintptr_t);
    ((copy_from_tex_to_buf_type)T1_objc_unsafe_get_raw_msg_send())(
        blit_enc,
        ags->sel_copy_from_texture_to_buffer,
        ags->metal_textures[T1_tex_to_array_i(ags->touch_tex)],
        /* source_slice: */ (uintptr_t)T1_tex_to_slice_i(ags->touch_tex),
        /* source_level: */ 0,
        /* source_origin: */
            T1_mtlorigin_make(sample->x, sample->y, 0),
        /* source_size: */
            T1_mtlsize_make(sample->w, sample->h, 1),
        /* to_buffer: */ sample->buffer,
        /* destination_offset: */ 0,
        /* destination_bytes_per_row: */ sample->w * 4,
        /* destination_bytes_per_image: */ sample->w * sample->h * 4);
    
    T1_objc_msg(blit_enc, ags->sel_end_encoding);
    
    sample->valid = true;
}

/*
Pick up any finished frames' touch samples without blocking, then wait for
the frame that last used this slot: its buffers (and touch sample) are about
to be overwritten. This also limits the number of frames in flight to
T1_FRAMES_CAP.
*/
static void T1_gpu_wait_for_frame_slot(void) {
    for (u32 i = 0; i < T1_FRAMES_CAP; i++) {
        T1_gpu_retire_touch_sample(
            &ags->touch_samples[i],
            /* wait: */ false);
    }
    T1_gpu_retire_touch_sample(
        &ags->touch_samples[ags->frame_i],
        /* wait: */ true);
}

/*
(Re)creates the final texture if the window's pixel size changed.
*/
static b8 T1_gpu_ensure_final_texture(void) {
    u32 w = (u32)ags->window_viewport.width;
    u32 h = (u32)ags->window_viewport.height;
    
    if (
        ags->final_texture != NULL &&
        ags->final_texture_w == w &&
        ags->final_texture_h == h)
    {
        return true;
    }
    
    if (ags->final_texture != NULL) {
        // In-flight command buffers keep their own reference
        T1_objc_msg(ags->final_texture, ags->sel_release);
        ags->final_texture = NULL;
    }
    
    void * desc = (void *)T1_objc_msg(
        ags->class_mtl_texture_desc,
        ags->sel_new);
    if (desc == NULL) { return false; }
    T1_objc_msg_1arg(desc,
        ags->sel_set_texture_type, T1MTLTextureType2D);
    // Matches T1_GPU_TARGET_WINDOW pipelines and the drawable
    T1_objc_msg_1arg(desc,
        ags->sel_set_pixel_format, T1MTLPixelFormatBGRA8Unorm);
    T1_objc_msg_1arg(desc,
        ags->sel_set_width, w);
    T1_objc_msg_1arg(desc,
        ags->sel_set_height, h);
    T1_objc_msg_1arg(desc,
        ags->sel_set_mipmap_level_count, 1);
    T1_objc_msg_1arg(desc,
        ags->sel_set_storage_mode, T1MTLStorageModePrivate);
    T1_objc_msg_1arg(desc,
        ags->sel_set_usage,
        T1MTLTextureUsageRenderTarget |
        T1MTLTextureUsageShaderRead);
    
    ags->final_texture = (void *)T1_objc_msg_1arg(
        ags->device,
        ags->sel_new_tex_with_desc,
        (uintptr_t)desc);
    T1_objc_msg(desc, ags->sel_release);
    
    if (ags->final_texture == NULL) {
        return false;
    }
    
    ags->final_texture_w = w;
    ags->final_texture_h = h;
    return true;
}

/*
Encodes and commits the client's passes, the window being
ags->final_texture. Doesn't touch the screen.
Returns false if nothing was rendered.
*/
static b8 T1_gpu_render_frame(T1GPUFrame * f) {
    if (!T1_gpu_ensure_final_texture()) {
        T1_assert(0);
        return false;
    }
    
    void * combuf = (void *)T1_objc_msg(
        ags->command_queue,
        ags->sel_command_buffer);
    
    if (combuf == NULL) {
        #if T1_ASSERTS_ACTIVE == T1_ACTIVE
        T1_crash("error - can't get metal command buffer\n");
        #elif T1_ASSERTS_ACTIVE == T1_INACTIVE
        #else
        #error
        #endif
        
        return false;
    }
    
    for (u32 i = 0; i < T1_RENDER_VIEW_CAP; i++) {
        ags->cam_depth_cleared[i] = false;
    }
    
    T1_gpu_encode_buf_writes(combuf);
    
    // the client's passes, in the order they were made
    for (u32 pass_i = 0; pass_i < T1_passes_size; pass_i++) {
        const T1Pass * pass = T1_passes + pass_i;
        switch (pass->kind) {
            case T1_PASS_CLEAR:
                T1_gpu_encode_clear_pass(pass, combuf);
                break;
            case T1_PASS_DRAW:
                T1_gpu_encode_draw_pass(pass, f, combuf);
                break;
            case T1_PASS_COMPUTE:
                T1_gpu_encode_compute_pass(pass, combuf);
                break;
            default:
                T1_assert(0);
        }
    }
    
    T1_gpu_encode_buf_reads(combuf);
    
    T1_gpu_encode_touch_sample(
        combuf,
        &ags->touch_samples[ags->frame_i]);
    
    ags->frame_i += 1;
    ags->frame_i %= T1_FRAMES_CAP;
    T1_assert(ags->frame_i < T1_FRAMES_CAP);
    
    T1_objc_msg(combuf, ags->sel_commit);
    
    return true;
}

/*
Copies the final texture to the layer's next drawable and presents it.
Grabs the drawable only now, so we hold it as briefly as possible. The copy
is queued after the frame's command buffer, so it sees the finished frame.
nextDrawable blocks while all of the layer's drawables are in use, which is
what paces the main loop to the display's refresh rate.
Returns false if nothing was presented.
*/
b8 T1_gpu_present_final_texture(void * metal_layer) {
    if (metal_layer == NULL || ags->final_texture == NULL) {
        return false;
    }
    
    if (
        ags->layer_drawable_w != ags->final_texture_w ||
        ags->layer_drawable_h != ags->final_texture_h)
    {
        T1CGSize size;
        size.width  = ags->final_texture_w;
        size.height = ags->final_texture_h;
        typedef void (* set_drawable_size_type)(void *, void *, T1CGSize);
        ((set_drawable_size_type)T1_objc_unsafe_get_raw_msg_send())(
            metal_layer,
            ags->sel_set_drawable_size,
            size);
        ags->layer_drawable_w = ags->final_texture_w;
        ags->layer_drawable_h = ags->final_texture_h;
    }
    
    // Can be nil, e.g. after a 1 second timeout while the window is hidden
    void * drawable = (void *)T1_objc_msg(
        metal_layer,
        ags->sel_next_drawable);
    if (drawable == NULL) {
        return false;
    }
    
    void * drawable_tex = (void *)T1_objc_msg(drawable, ags->sel_texture);
    if (drawable_tex == NULL) {
        return false;
    }
    
    void * combuf = (void *)T1_objc_msg(
        ags->command_queue,
        ags->sel_command_buffer);
    if (combuf == NULL) {
        return false;
    }
    
    // Shouldn't differ since we set the drawable size above, but be safe
    u64 drawable_w = T1_objc_msg(drawable_tex, ags->sel_width);
    u64 drawable_h = T1_objc_msg(drawable_tex, ags->sel_height);
    u64 copy_w = drawable_w < ags->final_texture_w ?
        drawable_w : ags->final_texture_w;
    u64 copy_h = drawable_h < ags->final_texture_h ?
        drawable_h : ags->final_texture_h;
    
    void * blit_enc = (void *)T1_objc_msg(
        combuf, ags->sel_blit_command_encoder);
    
    typedef void (* copy_tex_region_to_tex_type)(
        void *, void *,
        /* copyFromTexture:(id<MTLTexture>)    : */ void *,
        /* sourceSlice:(NSUInteger)            : */ uintptr_t,
        /* sourceLevel:(NSUInteger)            : */ uintptr_t,
        /* sourceOrigin:(MTLOrigin)            : */ T1MTLOrigin,
        /* sourceSize:(MTLSize)                : */ T1MTLSize,
        /* toTexture:(id<MTLTexture>)          : */ void *,
        /* destinationSlice:(NSUInteger)       : */ uintptr_t,
        /* destinationLevel:(NSUInteger)       : */ uintptr_t,
        /* destinationOrigin:(MTLOrigin)       : */ T1MTLOrigin);
    ((copy_tex_region_to_tex_type)T1_objc_unsafe_get_raw_msg_send())(
        blit_enc,
        ags->sel_copy_from_texture_region_to_texture,
        ags->final_texture,
        /* source_slice: */ 0,
        /* source_level: */ 0,
        /* source_origin: */ T1_mtlorigin_make(0, 0, 0),
        /* source_size: */ T1_mtlsize_make(copy_w, copy_h, 1),
        drawable_tex,
        /* destination_slice: */ 0,
        /* destination_level: */ 0,
        /* destination_origin: */ T1_mtlorigin_make(0, 0, 0));
    
    T1_objc_msg(blit_enc, ags->sel_end_encoding);
    
    T1_objc_msg_1arg(
        combuf, ags->sel_present_drawable,
        /* presentDrawable: */
            (uintptr_t)drawable);
    
    T1_objc_msg(combuf, ags->sel_commit);
    
    return true;
}

T1GPUFrame * T1_os_gpu_begin_frame(void) {
    if (!ags || !ags->metal_active || !ags->shaders_ready) {
        return NULL;
    }
    
    void * pool = T1_objc_autorelease_pool_push();
    
    T1GPUFrame * f = &T1_cpu_to_gpu_data->
        triple_buffers[ags->frame_i];
    
    T1_assert(f->verts_size % 3 == 0);
    
    T1_gpu_wait_for_frame_slot();
    
    T1_objc_autorelease_pool_pop(pool);
    
    return f;
}

u8 T1_os_gpu_render_frame(void) {
    if (!ags || !ags->metal_active || !ags->shaders_ready) {
        return false;
    }
    
    // The frame T1_os_gpu_begin_frame() returned, frame_i hasn't moved
    T1GPUFrame * f = &T1_cpu_to_gpu_data->
        triple_buffers[ags->frame_i];
    
    if (
        (f->frame_consts->timestamp -
            T1_global->last_resize_request_us)
                < T1_GLOBAL_WINDOW_RESIZE_TIMEOUT)
    {
        return false;
    }
    
    void * pool = T1_objc_autorelease_pool_push();
    
    b8 rendered = T1_gpu_render_frame(f);
    
    T1_objc_autorelease_pool_pop(pool);
    
    return rendered;
}

void T1_os_gpu_set_touch_tex(u16 tex) {
    if (!ags) { return; }
    ags->touch_tex = tex;
}

void T1_os_gpu_update_internal_render_viewport(
    const s32 at_i)
{
    if (!ags || !ags->metal_active) { return; }
    
    T1_assert(at_i >= 0);
    T1_assert(at_i < T1_RENDER_VIEW_CAP);
    
    T1_gpu_update_render_view_size(at_i);
}

void T1_os_gpu_update_window_viewport(void)
{
    T1_gpu_update_final_window_size();
}


void T1_os_gpu_init(char ** const fatal_error) {
    if (*fatal_error != 0) { return; }
    
    void * pool = T1_objc_autorelease_pool_push();
    
    typedef void *(* mtl_fn)(void);
    mtl_fn fn = (mtl_fn)T1_objc_get_func(
        "MTLCreateSystemDefaultDevice");
    if (!fn) {
        *fatal_error = "Failed to fetch basic Metal function";
        T1_objc_autorelease_pool_pop(pool);
        return;
    }
    
    void * device = fn();
    if (!device) {
        *fatal_error = "Failed to create Metal device";
        T1_objc_autorelease_pool_pop(pool);
        return;
    }
    
    T1_apple_gpu_init(
        /* id<MTLDevice> with_metal_device: */
            device,
        /* f32 backing_scale_factor: */
            T1_os_get_screen_backing_scale_factor(),
        /* fatal_error: */
            fatal_error);
    
    T1_objc_autorelease_pool_pop(pool);
}

u8 T1_os_gpu_active(void) {
    return ags != NULL && ags->metal_active;
}

void * T1_gpu_get_device(void) {
    return ags != NULL ? ags->device : NULL;
}
