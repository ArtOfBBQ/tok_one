/*
The GPU through WebGPU, the browser's (see T1_os_web.c for how the C side
calls T1_web.html). WebGPU's objects live in JS: T1 knows its textures by
their array index, its buffers by their T1Buf, its pipelines by their
index in T1_pipelines, and JS keeps the GPUTexture, GPUBuffer or
GPURenderPipeline for each.

The client's shaders are translated to WGSL (T1_shader_to_wgsl()), one
module per entry point; how a pass binds what they use is in THE GPU in
T1_web.html. A frame's passes go into one command encoder, submitted when
T1_os_gpu_render_frame() is done with them.

Not yet: compressed textures (every array is rgba8), mipmaps, the GPU's
frame time.
*/

#include "T1_os.h"

#include "T1_assert.h"
#include "T1_buf.h"
#include "T1_global.h"
#include "T1_mem.h"
#include "T1_pass.h"
#include "T1_pipeline.h"
#include "T1_shader.h"
#include "T1_std.h"
#include "T1_tex_array.h"

#define T1_WEB_IMPORT(name) \
    __attribute__((import_module("t1"), import_name(#name)))

// asks for an adapter and a device (suspends, like t1.wait()): 1 if it got one
T1_WEB_IMPORT(gpu_init) s32 T1_web_gpu_init(void);
T1_WEB_IMPORT(gpu_lost) s32 T1_web_gpu_lost(void);
T1_WEB_IMPORT(gpu_name) void T1_web_gpu_name(char * recip, u32 recip_cap);
/*
Makes the array's GPUTexture this size with at least this many slices,
copying the slices it had if only the count grows
*/
T1_WEB_IMPORT(gpu_tex_array) void T1_web_gpu_tex_array(
    s32 array_i, u32 width, u32 height, u32 slices);
T1_WEB_IMPORT(gpu_tex_delete) void T1_web_gpu_tex_delete(s32 array_i);
T1_WEB_IMPORT(gpu_tex_write) void T1_web_gpu_tex_write(
    s32 array_i,
    s32 slice_i,
    u32 x,
    u32 y,
    u32 w,
    u32 h,
    const u8 * pixels,
    u32 bytes_per_row,
    u32 size);
T1_WEB_IMPORT(gpu_present) s32 T1_web_gpu_present(s32 array_i, s32 slice_i);
T1_WEB_IMPORT(gpu_buf_new) s32 T1_web_gpu_buf_new(u32 buf_i, f64 size);
T1_WEB_IMPORT(gpu_buf_write) void T1_web_gpu_buf_write(
    u32 buf_i, f64 offset, const void * data, u32 size);
T1_WEB_IMPORT(gpu_buf_delete) void T1_web_gpu_buf_delete(u32 buf_i);
/*
The translated shaders (compiled, suspends while WebGPU does): 1 if every
unit compiled, else 0 with WebGPU's errors in error_recip
*/
T1_WEB_IMPORT(gpu_shaders) s32 T1_web_gpu_shaders(
    const char * wgsl, u32 size, char * error_recip, u32 error_recip_cap);
/*
A pipeline from T1_pipelines (its functions' names): 1 if the shaders
have its functions
*/
T1_WEB_IMPORT(gpu_pipeline) s32 T1_web_gpu_pipeline(
    s32 pipeline_i,
    s32 is_compute,
    const char * vertex_fn,
    u32 vertex_fn_size,
    const char * fragment_fn,
    u32 fragment_fn_size,
    s32 blend,
    s32 cull);
T1_WEB_IMPORT(gpu_pipeline_delete) void T1_web_gpu_pipeline_delete(
    s32 pipeline_i);
// a frame's start: its T1GPUFrameConsts
T1_WEB_IMPORT(gpu_frame_begin) void T1_web_gpu_frame_begin(
    const void * frame_consts, u32 size);
T1_WEB_IMPORT(gpu_clear) void T1_web_gpu_clear(
    s32 array_i, s32 slice_i, f32 r, f32 g, f32 b, f32 a);
// a draw or compute pass, its T1PassU32s (see T1_pass.h) at u32s
T1_WEB_IMPORT(gpu_pass) void T1_web_gpu_pass(
    u32 pass_i, s32 pipeline_i, const T1PassU32s * u32s);
// the frame's reads, then it's submitted
T1_WEB_IMPORT(gpu_frame_end) void T1_web_gpu_frame_end(void);
// see T1_os_gpu_buf_read_start(): its index, or T1_BUF_READ_NONE
T1_WEB_IMPORT(gpu_read_buf) u32 T1_web_gpu_read_buf(
    u32 buf_i, f64 offset, f64 size);
T1_WEB_IMPORT(gpu_read_tex) u32 T1_web_gpu_read_tex(
    s32 array_i, s32 slice_i, u32 x, u32 y, u32 w, u32 h);
// 1 once the read is done, its bytes copied to recip
T1_WEB_IMPORT(gpu_read_finish) s32 T1_web_gpu_read_finish(
    u32 read_i, void * recip);

typedef struct {
    u32 tex_widths[T1_TEXARRAYS_CAP];
    u32 tex_heights[T1_TEXARRAYS_CAP];
    u32 tex_slices[T1_TEXARRAYS_CAP]; // 0: none on the GPU
    b8 active;
    b8 shaders_ready;
} T1WebGPUState;

static T1WebGPUState T1_wgpu;

void T1_os_gpu_init(char ** const fatal_error) {
    if (*fatal_error != 0) { return; }
    if (!T1_web_gpu_init()) {
        *fatal_error =
            "No WebGPU: this browser doesn't have it, or it's switched off";
        return;
    }
    T1_wgpu.active = 1;
}

u8 T1_os_gpu_active(void) {
    return T1_wgpu.active;
}

u8 T1_os_gpu_lost(void) {
    return T1_wgpu.active && T1_web_gpu_lost();
}

u64 T1_os_gpu_frame_us(void) {
    // unknown: timestamp queries are an optional WebGPU feature (TODO)
    return 0;
}

const char * T1_os_gpu_name(void) {
    static char name[128];
    if (name[0] == '\0' && T1_wgpu.active) {
        T1_web_gpu_name(name, 128);
    }
    return name;
}

/*
TEXTURES
*/

u8 T1_os_gpu_compressed_format(void) {
    // (TODO: BC3 where the adapter has "texture-compression-bc", else ETC2)
    return T1_TEX_FORMAT_RGBA8;
}

void T1_os_gpu_update_capacity_if_needed(s32 tex_array_i) {
    if (!T1_wgpu.active) { return; }
    T1_assert(tex_array_i >= 0 && tex_array_i < T1_TEXARRAYS_CAP);
    if (tex_array_i < 0 || tex_array_i >= T1_TEXARRAYS_CAP) { return; }

    T1TexArray * array = &T1_tex_arrays[tex_array_i];
    if (array->deleted) {
        T1_os_gpu_delete_texture_array(tex_array_i);
        return;
    }

    const u32 slices = array->images_size > 0 ? array->images_size : 1;
    if (
        T1_wgpu.tex_slices[tex_array_i] >= slices &&
        T1_wgpu.tex_widths[tex_array_i] == array->single_img_width &&
        T1_wgpu.tex_heights[tex_array_i] == array->single_img_height)
    {
        return;
    }

    T1_web_gpu_tex_array(
        tex_array_i,
        array->single_img_width,
        array->single_img_height,
        slices);
    T1_wgpu.tex_slices[tex_array_i] = slices;
    T1_wgpu.tex_widths[tex_array_i] = array->single_img_width;
    T1_wgpu.tex_heights[tex_array_i] = array->single_img_height;
    array->gpu_capacity = slices;
}

void T1_os_gpu_delete_texture_array(s32 array_i) {
    if (!T1_wgpu.active) { return; }
    T1_web_gpu_tex_delete(array_i);
    T1_wgpu.tex_slices[array_i] = 0;
}

void T1_os_gpu_write_tex_slice(
    s32 tex_array_i,
    s32 tex_slice_i,
    u32 x,
    u32 y,
    u32 w,
    u32 h,
    const u8 * pixels)
{
    if (!T1_wgpu.active) { return; }
    T1_assert(tex_array_i >= 0 && tex_array_i < T1_TEXARRAYS_CAP);
    T1_assert(tex_slice_i >= 0);
    T1_assert(pixels != NULL);
    if (pixels == NULL) { return; }

    T1_os_gpu_update_capacity_if_needed(tex_array_i);
    if (T1_wgpu.tex_slices[tex_array_i] == 0) { return; }

    const T1TexFormat format = T1_tex_arrays[tex_array_i].format;
    // (copied by the queue before it returns: the caller's again)
    T1_web_gpu_tex_write(
        tex_array_i,
        tex_slice_i,
        x,
        y,
        w,
        h,
        pixels,
        T1_tex_format_row_bytes(format, w),
        T1_tex_format_bytes(format, w, h));
}

#if T1_MIPMAPS_ACTIVE == T1_ACTIVE
void T1_os_gpu_generate_mipmaps_for_texture_array(s32 texture_array_i) {
    // one mip level for now (WebGPU has no generateMipmap, it's a pass)
    (void)texture_array_i;
}
#elif T1_MIPMAPS_ACTIVE == T1_INACTIVE
#else
#error
#endif

/*
SHADERS AND PIPELINES
*/

static void T1_os_web_pipeline_to_js(s32 pipeline_i, u8 * good) {
    const T1Pipeline * p = &T1_pipelines[pipeline_i];
    *good = (u8)T1_web_gpu_pipeline(
        pipeline_i,
        p->is_compute,
        p->vertex_fn,
        (u32)T1_std_strlen(p->vertex_fn),
        p->fragment_fn,
        (u32)T1_std_strlen(p->fragment_fn),
        p->blend,
        p->cull);
}

void T1_os_gpu_try_shader_source(
    const char * shader_source,
    u8 * good,
    char * error_recip,
    const u32 error_recip_cap,
    char ** const fatal_error)
{
    *good = 0;
    if (*fatal_error != 0) { return; }
    if (error_recip != NULL) {
        if (error_recip_cap < 512) {
            *fatal_error = "T1_os_gpu_try_shader_source() needs an "
                "error_recip_cap of at least 512";
            return;
        }
        error_recip[0] = '\0';
    }
    if (!T1_wgpu.active) {
        *fatal_error = "T1_gpu_try_shader_source() before T1_gpu_init()";
        return;
    }
    if (shader_source == NULL || shader_source[0] == '\0') {
        if (error_recip != NULL) {
            T1_std_strcpy_cap(error_recip, error_recip_cap,
                "Empty shader source");
        }
        return;
    }

    // the WGSL: every unit with T1's prelude, so many times the source
    const u32 cap =
        4 * 1024 * 1024 + (u32)T1_std_strlen(shader_source) * 64;
    char * wgsl = T1_mem_malloc_managed(cap);
    if (wgsl == NULL) { *fatal_error = "Malloc fail"; return; }
    char local_error[512];
    char * errors = error_recip != NULL ? error_recip : local_error;
    const u32 errors_cap = error_recip != NULL ? error_recip_cap : 512;
    T1_shader_to_wgsl(shader_source, wgsl, cap, good, errors, errors_cap);
    if (*good) {
        *good = (u8)T1_web_gpu_shaders(
            wgsl, (u32)T1_std_strlen(wgsl), errors, errors_cap);
    }
    T1_mem_free_managed(wgsl);
    if (!*good) { return; }
    T1_wgpu.shaders_ready = 1;

    // the pipelines made before, their functions checked now
    for (u32 i = 0; i < T1_pipelines_size; i++) {
        if (T1_pipelines[i].id == T1_ID_NONE) { continue; }
        u8 found = 0;
        T1_os_web_pipeline_to_js((s32)i, &found);
    }
}

u8 T1_os_gpu_shaders_ready(void) {
    return T1_wgpu.active && T1_wgpu.shaders_ready;
}

void T1_os_gpu_pipeline_added(s32 pipeline_i, u8 * good) {
    *good = 1;
    if (!T1_os_gpu_shaders_ready()) {
        // checked when the shaders load
        return;
    }
    T1_os_web_pipeline_to_js(pipeline_i, good);
}

void T1_os_gpu_pipeline_deleted(s32 pipeline_i) {
    if (!T1_wgpu.active) { return; }
    T1_web_gpu_pipeline_delete(pipeline_i);
}

/*
BUFFERS
*/

void T1_os_gpu_buf_new(u32 buf_i, u64 size, u8 * good) {
    *good = T1_wgpu.active && T1_web_gpu_buf_new(buf_i, (f64)size);
}

void T1_os_gpu_buf_write(
    u32 buf_i,
    u64 offset,
    const void * data,
    u64 size)
{
    if (!T1_wgpu.active) { return; }
    T1_web_gpu_buf_write(buf_i, (f64)offset, data, (u32)size);
}

void T1_os_gpu_buf_delete(u32 buf_i) {
    if (!T1_wgpu.active) { return; }
    T1_web_gpu_buf_delete(buf_i);
}

u32 T1_os_gpu_buf_read_start(u32 buf_i, u64 offset, u64 size) {
    if (!T1_wgpu.active) { return T1_BUF_READ_NONE; }
    return T1_web_gpu_read_buf(buf_i, (f64)offset, (f64)size);
}

u32 T1_os_gpu_tex_read_start(u32 tex, u32 x, u32 y, u32 w, u32 h) {
    if (!T1_wgpu.active || tex == T1_TEX_NONE) { return T1_BUF_READ_NONE; }
    return T1_web_gpu_read_tex(
        (s32)T1_TEX_ARRAY_I(tex), (s32)T1_TEX_SLICE_I(tex), x, y, w, h);
}

u8 T1_os_gpu_buf_read_finish(u32 read_i, void * recip) {
    if (!T1_wgpu.active || read_i == T1_BUF_READ_NONE) { return 0; }
    return (u8)T1_web_gpu_read_finish(read_i, recip);
}

/*
FRAMES
*/

u8 T1_os_gpu_begin_frame(void) {
    // WebGPU paces the frames in flight itself
    return T1_wgpu.active && !T1_web_gpu_lost();
}

u8 T1_os_gpu_render_frame(void) {
    if (!T1_wgpu.active) { return 0; }

    T1GPUFrameConsts consts = T1_global->frame_consts;
    T1_web_gpu_frame_begin(&consts, sizeof(T1GPUFrameConsts));

    // the client's passes, in the order they were made. Without shaders
    // only clear passes run
    for (u32 pass_i = 0; pass_i < T1_passes_size; pass_i++) {
        const T1Pass * pass = T1_passes + pass_i;
        if (pass->u32s.off) { continue; }

        if (pass->kind == T1_PASS_CLEAR) {
            const u32 tex = pass->u32s.color_tex[0];
            if (tex == T1_TEX_NONE) { continue; }
            const s32 array_i = (s32)T1_TEX_ARRAY_I(tex);
            const s32 slice_i = (s32)T1_TEX_SLICE_I(tex);
            if (
                array_i >= T1_TEXARRAYS_CAP ||
                (u32)slice_i >= T1_wgpu.tex_slices[array_i])
            {
                continue;
            }
            T1_web_gpu_clear(
                array_i,
                slice_i,
                pass->clear_rgba[0],
                pass->clear_rgba[1],
                pass->clear_rgba[2],
                pass->clear_rgba[3]);
            continue;
        }
        if (!T1_os_gpu_shaders_ready()) { continue; }
        const s32 pipeline_i = T1_pipeline_id_to_i(pass->pipeline);
        T1_assert(pipeline_i >= 0);
        if (pipeline_i < 0) { continue; }
        T1_web_gpu_pass(pass_i, pipeline_i, &pass->u32s);
    }

    T1_web_gpu_frame_end();
    return 1;
}

u8 T1_os_window_present(const u32 tex) {
    if (!T1_wgpu.active || tex == T1_TEX_NONE) { return 0; }
    const s32 array_i = (s32)T1_TEX_ARRAY_I(tex);
    const s32 slice_i = (s32)T1_TEX_SLICE_I(tex);
    if (
        array_i >= T1_TEXARRAYS_CAP ||
        (u32)slice_i >= T1_wgpu.tex_slices[array_i])
    {
        return 0;
    }
    // as large as it fits in the canvas, black around it; the browser
    // shows it when the app waits next (T1_os_window_pump_events())
    return T1_web_gpu_present(array_i, slice_i) != 0;
}
