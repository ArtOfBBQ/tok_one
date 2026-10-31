#include "T1_gpu_d3d11.h"

#include "T1_std.h"
#include "T1_mem.h"
#include "T1_assert.h"
#include "T1_global.h"
#include "T1_buf.h"
#include "T1_pass.h"
#include "T1_pipeline.h"
#include "T1_shader.h"
#include "T1_tex_array.h"
#include "T1_os.h"
#include "T1_d3d11.h"

/*
The GPU through Direct3D 11, at feature level 11_0 (any GPU since about
2010, or WARP, Windows' software renderer, without one). d3d11.dll and
d3dcompiler_47.dll are loaded at runtime, nothing is linked.

The client's shaders are translated to HLSL (see T1_shader_to_hlsl()) and
compiled when they're loaded. Every texture array is one Direct3D texture,
every T1Buf a buffer of raw bytes, like on Metal.

Unlike Metal, Direct3D 11 won't let a pass read a texture array while it
draws or writes into one of its slices. Such a pass reads a copy of that
array, made just before it, instead (see T1_gpu_d3d11_tex_srvs()).
*/

#define T1_D3D_RELEASE(object) \
    do { \
        if ((object) != NULL) { \
            ((T1ComObject *)(object))->vtbl->Release((T1ComObject *)(object)); \
            (object) = NULL; \
        } \
    } while (0)

// SRV registers: textures, then buffer(n) at T1_HLSL_BUFS_T + n
#define T1_D3D_SRVS_SIZE (T1_HLSL_BUFS_T + T1_BUFS_I + T1_PASS_BUFS_CAP)
#define T1_D3D_UAVS_SIZE 8

static const T1D3DGUID T1_IID_IDXGIDevice = {
    0x54ec77fa, 0x1377, 0x44e6,
    {0x8c, 0x32, 0x88, 0xfd, 0x5f, 0x44, 0xc8, 0x4c}};
static const T1D3DGUID T1_IID_IDXGIFactory = {
    0x7b7166ec, 0x21c7, 0x44ae,
    {0xb2, 0x1a, 0xc9, 0xae, 0x32, 0x1a, 0xe3, 0x69}};
static const T1D3DGUID T1_IID_ID3D11Texture2D = {
    0x6f15aaf2, 0xd208, 0x4e89,
    {0x9a, 0xb4, 0x48, 0x95, 0x35, 0xd3, 0x4f, 0x9c}};

// An entry point of the loaded shaders
#define T1_D3D_UNITS_CAP 256
typedef struct {
    c8 name[T1_PIPELINE_FUNC_NAME_CAP];
    T1D3D11Shader * shader;
    u32 threads[3]; // a kernel's group size
    u8 uav_buf[T1_D3D_UAVS_SIZE]; // the buffer(n) at u slot, 0xFF for none
    u8 stage; // 1 vertex, 2 fragment, 3 kernel
    b8 reads_textures;
} T1D3DUnit;

typedef struct {
    T1D3DUnit units[T1_D3D_UNITS_CAP];
    u32 units_size;
} T1D3DShaders;

// A T1_buf_read_start(), copied to staging after the next frame's passes
typedef u8 T1D3DReadState;
enum {
    T1_D3D_READ_FREE = 0,
    T1_D3D_READ_QUEUED,
    T1_D3D_READ_ENCODED,
};

typedef struct {
    T1D3D11Resource * staging;
    u64 offset;
    u64 size;
    u32 buf_i; // T1_BUFS_CAP for a texture read
    u32 tex;
    u32 tex_x;
    u32 tex_y;
    u32 tex_w;
    u32 tex_h;
    b8 missing; // its buffer or texture was gone: reads zeros
    T1D3DReadState state;
} T1D3DRead;

// The depth buffer of a pass's "depth_tex", see T1_gpu_d3d11_tex_depth()
#define T1_D3D_TEX_DEPTHS_CAP 16
typedef struct {
    T1D3D11Resource * depth;
    T1D3D11View * dsv;
    u64 last_used_frame;
    u32 tex;
    u32 width;
    u32 height;
    b8 cleared;
} T1D3DTexDepth;

typedef struct {
    T1D3D11Device * device;
    T1D3D11Context * ctx;
    T1D3DCompileFn compile;

    // a texture array each, and what reads it
    T1D3D11Resource * texs[T1_TEXARRAYS_CAP];
    T1D3D11View * tex_srvs[T1_TEXARRAYS_CAP];
    u32 tex_slices[T1_TEXARRAYS_CAP];
    u32 tex_mips[T1_TEXARRAYS_CAP];
    u32 tex_widths[T1_TEXARRAYS_CAP];
    u32 tex_heights[T1_TEXARRAYS_CAP];
    // copies for passes that draw into the array, made on first use
    T1D3D11Resource * tex_copies[T1_TEXARRAYS_CAP];
    T1D3D11View * tex_copy_srvs[T1_TEXARRAYS_CAP];
    // what passes draw and write into, per slice, made on first use
    T1D3D11View * tex_rtvs[T1_TEXARRAYS_CAP][T1_TEX_SLICES_CAP];
    T1D3D11View * tex_uavs[T1_TEXARRAYS_CAP][T1_TEX_SLICES_CAP];

    T1D3D11Resource * bufs[T1_BUFS_CAP];
    T1D3D11View * buf_srvs[T1_BUFS_CAP];
    T1D3D11View * buf_uavs[T1_BUFS_CAP];
    T1D3DRead reads[T1_BUF_READS_CAP];

    // what every pass's shaders get at T1_FRAME_CONSTS_I and T1_PARAMS_I
    T1D3D11Resource * frame_consts_buf;
    T1D3D11View * frame_consts_srv;
    T1D3D11Resource * params_buf;
    T1D3D11View * params_srv;
    // a kernel's grid, see T1_shader_to_hlsl()
    T1D3D11Resource * grid_cb;

    T1D3D11State * samplers[3]; // nearest, linear, linear repeat
    T1D3D11State * blend_states[4]; // by T1_BLEND_*
    T1D3D11State * raster_states[3]; // by T1_CULL_*
    T1D3D11State * depth_state;
    T1D3D11State * no_depth_state;
    T1D3DTexDepth tex_depths[T1_D3D_TEX_DEPTHS_CAP];

    T1D3DShaders * shaders; // NULL until the client's first load
    // per T1_pipelines entry: its units ([1] unused for compute), -1
    s32 pipeline_units[T1_PIPELINES_CAP][2];

    // a query per frame in flight, see T1_os_gpu_begin_frame()
    T1ComObject * frame_queries[T1_FRAMES_CAP];
    b8 frame_query_issued[T1_FRAMES_CAP];
    u32 frame_i;
    u64 frames_encoded;

    // the window
    T1DXGISwapChain * swap_chain;
    T1D3D11View * back_buffer_rtv;
    T1WinHandle swap_chain_hwnd;
    u32 back_buffer_w;
    u32 back_buffer_h;
    T1D3D11Shader * present_vs;
    T1D3D11Shader * present_ps;

    b8 active;
} T1D3DState;

static T1D3DState * T1_d3d = NULL;

// The SRVs, UAVs etc. of a stage, all unset
static T1D3D11View * T1_d3d_null_views[T1_D3D_SRVS_SIZE];

static void T1_gpu_d3d11_unbind_all(void) {
    T1D3D11Context * ctx = T1_d3d->ctx;
    ctx->vtbl->OMSetRenderTargets(ctx, 0, NULL, NULL);
    ctx->vtbl->VSSetShaderResources(
        ctx, 0, T1_D3D_SRVS_SIZE, T1_d3d_null_views);
    ctx->vtbl->PSSetShaderResources(
        ctx, 0, T1_D3D_SRVS_SIZE, T1_d3d_null_views);
    ctx->vtbl->CSSetShaderResources(
        ctx, 0, T1_D3D_SRVS_SIZE, T1_d3d_null_views);
    ctx->vtbl->CSSetUnorderedAccessViews(
        ctx, 0, T1_D3D_UAVS_SIZE, T1_d3d_null_views, NULL);
}

/*
Compiles HLSL source for an entry point and target (e.g. "vs_5_0").
Returns the bytecode blob, or NULL with the compiler's messages in
error_recip (may be NULL)
*/
static T1D3DBlob * T1_gpu_d3d11_compile(
    const c8 * source,
    const u32 source_size,
    const c8 * entry,
    const c8 * target,
    c8 * error_recip,
    const u32 error_recip_cap)
{
    T1D3DBlob * code = NULL;
    T1D3DBlob * errors = NULL;
    s32 result = T1_d3d->compile(
        source,
        source_size,
        entry,
        NULL,
        NULL,
        entry,
        target,
        T1D3DCOMPILE_OPTIMIZATION_LEVEL1,
        0,
        &code,
        &errors);

    if (result < 0 && error_recip != NULL) {
        T1_std_strcat_cap(error_recip, error_recip_cap, "Failed to compile ");
        T1_std_strcat_cap(error_recip, error_recip_cap, entry);
        T1_std_strcat_cap(error_recip, error_recip_cap, ":\n");
        if (errors != NULL) {
            // the compiler's messages, cut to what fits
            u32 at = (u32)T1_std_strlen(error_recip);
            const c8 * text = errors->vtbl->GetBufferPointer(errors);
            u32 size = (u32)errors->vtbl->GetBufferSize(errors);
            for (u32 i = 0; i < size && text[i] != '\0' && at + 1 < error_recip_cap; i++) {
                error_recip[at++] = text[i];
            }
            error_recip[at] = '\0';
        }
    }
    T1_D3D_RELEASE(errors);
    if (result < 0) {
        T1_D3D_RELEASE(code);
    }
    return code;
}

static T1D3D11View * T1_gpu_d3d11_raw_srv(T1D3D11Resource * buffer, u64 size) {
    T1D3D11SRVDesc desc;
    T1_std_memset(&desc, 0, sizeof(T1D3D11SRVDesc));
    desc.format = T1DXGI_FORMAT_R32_TYPELESS;
    desc.view_dimension = T1D3D11_SRV_DIMENSION_BUFFEREX;
    desc.u[0] = 0; // first element
    desc.u[1] = (u32)(size / 4); // elements
    desc.u[2] = T1D3D11_BUFFEREX_SRV_FLAG_RAW;
    T1D3D11View * srv = NULL;
    T1_d3d->device->vtbl->CreateShaderResourceView(
        T1_d3d->device, buffer, &desc, &srv);
    return srv;
}

static T1D3D11View * T1_gpu_d3d11_raw_uav(T1D3D11Resource * buffer, u64 size) {
    T1D3D11UAVDesc desc;
    T1_std_memset(&desc, 0, sizeof(T1D3D11UAVDesc));
    desc.format = T1DXGI_FORMAT_R32_TYPELESS;
    desc.view_dimension = T1D3D11_UAV_DIMENSION_BUFFER;
    desc.u[0] = 0; // first element
    desc.u[1] = (u32)(size / 4); // elements
    desc.u[2] = T1D3D11_BUFFER_UAV_FLAG_RAW;
    T1D3D11View * uav = NULL;
    T1_d3d->device->vtbl->CreateUnorderedAccessView(
        T1_d3d->device, buffer, &desc, &uav);
    return uav;
}

/*
A buffer of raw bytes that shaders read (and kernels write), its size a
multiple of 4, zeroed
*/
static T1D3D11Resource * T1_gpu_d3d11_new_raw_buffer(u64 size) {
    T1D3D11BufferDesc desc;
    T1_std_memset(&desc, 0, sizeof(T1D3D11BufferDesc));
    desc.byte_width = (u32)size;
    desc.usage = T1D3D11_USAGE_DEFAULT;
    desc.bind_flags =
        T1D3D11_BIND_SHADER_RESOURCE | T1D3D11_BIND_UNORDERED_ACCESS;
    desc.misc_flags = T1D3D11_RESOURCE_MISC_BUFFER_ALLOW_RAW_VIEWS;

    // Direct3D doesn't zero new buffers
    void * zeros = T1_mem_malloc_managed(size);
    if (zeros == NULL) { return NULL; }
    T1_std_memset(zeros, 0, size);
    T1D3D11SubresourceData data;
    data.sys_mem = zeros;
    data.sys_mem_pitch = 0;
    data.sys_mem_slice_pitch = 0;

    T1D3D11Resource * buffer = NULL;
    T1_d3d->device->vtbl->CreateBuffer(T1_d3d->device, &desc, &data, &buffer);
    T1_mem_free_managed(zeros);
    return buffer;
}

static T1D3D11State * T1_gpu_d3d11_new_sampler(u32 filter, u32 address) {
    T1D3D11SamplerDesc desc;
    T1_std_memset(&desc, 0, sizeof(T1D3D11SamplerDesc));
    desc.filter = filter;
    desc.address_u = address;
    desc.address_v = address;
    desc.address_w = address;
    desc.max_anisotropy = 1;
    desc.comparison_func = T1D3D11_COMPARISON_NEVER;
    desc.min_lod = 0.0f;
    desc.max_lod = T1_F32_MAX;
    T1D3D11State * sampler = NULL;
    T1_d3d->device->vtbl->CreateSamplerState(T1_d3d->device, &desc, &sampler);
    return sampler;
}

/*
T1's own shaders that draw the window's T1Tex into the back buffer: one
triangle over the whole viewport, sampling the T1Tex's slice
*/
static const c8 * const T1_gpu_d3d11_present_source =
    "Texture2DArray t1_window_tex : register(t0);\n"
    "SamplerState t1_nearest_smp : register(s0);\n"
    "struct T1PresentVertex {\n"
    "    float4 pos : SV_Position;\n"
    "    float2 uv : TEXCOORD0;\n"
    "};\n"
    "T1PresentVertex T1_present_vertex(uint vid : SV_VertexID) {\n"
    "    float2 xy = float2((vid << 1) & 2, vid & 2);\n"
    "    T1PresentVertex o;\n"
    "    o.pos = float4(xy.x * 2.0f - 1.0f, 1.0f - xy.y * 2.0f, 0.0f, 1.0f);\n"
    "    o.uv = xy;\n"
    "    return o;\n"
    "}\n"
    "float4 T1_present_fragment(T1PresentVertex i) : SV_Target {\n"
    "    return t1_window_tex.Sample(t1_nearest_smp, float3(i.uv, 0.0f));\n"
    "}\n";

static void T1_gpu_d3d11_init_states(char ** const fatal_error) {
    T1D3D11Device * device = T1_d3d->device;

    T1_d3d->samplers[0] = T1_gpu_d3d11_new_sampler(
        T1D3D11_FILTER_MIN_MAG_MIP_POINT, T1D3D11_TEXTURE_ADDRESS_CLAMP);
    T1_d3d->samplers[1] = T1_gpu_d3d11_new_sampler(
        T1D3D11_FILTER_MIN_MAG_LINEAR_MIP_POINT,
        T1D3D11_TEXTURE_ADDRESS_CLAMP);
    T1_d3d->samplers[2] = T1_gpu_d3d11_new_sampler(
        T1D3D11_FILTER_MIN_MAG_LINEAR_MIP_POINT,
        T1D3D11_TEXTURE_ADDRESS_WRAP);

    // only color(0) blends, the others are data (like touch ids)
    for (u32 blend = 0; blend < 4; blend++) {
        T1D3D11BlendDesc desc;
        T1_std_memset(&desc, 0, sizeof(T1D3D11BlendDesc));
        desc.independent_blend_enable = 1;
        for (u32 k = 0; k < 8; k++) {
            desc.render_target[k].src_blend = T1D3D11_BLEND_ONE;
            desc.render_target[k].dest_blend = T1D3D11_BLEND_ZERO;
            desc.render_target[k].blend_op = T1D3D11_BLEND_OP_ADD;
            desc.render_target[k].src_blend_alpha = T1D3D11_BLEND_ONE;
            desc.render_target[k].dest_blend_alpha = T1D3D11_BLEND_ZERO;
            desc.render_target[k].blend_op_alpha = T1D3D11_BLEND_OP_ADD;
            desc.render_target[k].write_mask = T1D3D11_COLOR_WRITE_ENABLE_ALL;
        }
        // like Metal, only the rgb blends, alpha is the output's
        if (blend != T1_BLEND_NONE) {
            desc.render_target[0].blend_enable = 1;
            desc.render_target[0].src_blend =
                blend == T1_BLEND_STRAIGHT_ALPHA ?
                    T1D3D11_BLEND_SRC_ALPHA :
                    T1D3D11_BLEND_ONE;
            desc.render_target[0].dest_blend =
                blend == T1_BLEND_ADDITIVE ?
                    T1D3D11_BLEND_ONE :
                    T1D3D11_BLEND_INV_SRC_ALPHA;
        }
        device->vtbl->CreateBlendState(
            device, &desc, &T1_d3d->blend_states[blend]);
    }

    // Metal's front faces: counter-clockwise
    static const u32 culls[3] = {
        T1D3D11_CULL_NONE, T1D3D11_CULL_BACK, T1D3D11_CULL_FRONT,
    };
    for (u32 cull = 0; cull < 3; cull++) {
        T1D3D11RasterizerDesc desc;
        T1_std_memset(&desc, 0, sizeof(T1D3D11RasterizerDesc));
        desc.fill_mode = T1D3D11_FILL_SOLID;
        desc.cull_mode = culls[cull];
        desc.front_counter_clockwise = 1;
        desc.depth_clip_enable = 1;
        device->vtbl->CreateRasterizerState(
            device, &desc, &T1_d3d->raster_states[cull]);
    }

    T1D3D11DepthStencilDesc depth;
    T1_std_memset(&depth, 0, sizeof(T1D3D11DepthStencilDesc));
    depth.depth_enable = 1;
    depth.depth_write_mask = T1D3D11_DEPTH_WRITE_MASK_ALL;
    depth.depth_func = T1D3D11_COMPARISON_LESS_EQUAL;
    depth.stencil_read_mask = 0xFF;
    depth.stencil_write_mask = 0xFF;
    depth.front_face.fail_op = T1D3D11_STENCIL_OP_KEEP;
    depth.front_face.depth_fail_op = T1D3D11_STENCIL_OP_KEEP;
    depth.front_face.pass_op = T1D3D11_STENCIL_OP_KEEP;
    depth.front_face.func = T1D3D11_COMPARISON_ALWAYS;
    depth.back_face = depth.front_face;
    device->vtbl->CreateDepthStencilState(
        device, &depth, &T1_d3d->depth_state);
    depth.depth_enable = 0;
    device->vtbl->CreateDepthStencilState(
        device, &depth, &T1_d3d->no_depth_state);

    // the frame consts, a pass's params and a kernel's grid
    T1_d3d->frame_consts_buf = T1_gpu_d3d11_new_raw_buffer(
        sizeof(T1GPUFrameConsts));
    T1_d3d->params_buf = T1_gpu_d3d11_new_raw_buffer(
        sizeof(u32) * T1_PARAMS_CAP);
    if (T1_d3d->frame_consts_buf != NULL) {
        T1_d3d->frame_consts_srv = T1_gpu_d3d11_raw_srv(
            T1_d3d->frame_consts_buf, sizeof(T1GPUFrameConsts));
    }
    if (T1_d3d->params_buf != NULL) {
        T1_d3d->params_srv = T1_gpu_d3d11_raw_srv(
            T1_d3d->params_buf, sizeof(u32) * T1_PARAMS_CAP);
    }
    T1D3D11BufferDesc grid;
    T1_std_memset(&grid, 0, sizeof(T1D3D11BufferDesc));
    grid.byte_width = 16;
    grid.usage = T1D3D11_USAGE_DEFAULT;
    grid.bind_flags = T1D3D11_BIND_CONSTANT_BUFFER;
    device->vtbl->CreateBuffer(device, &grid, NULL, &T1_d3d->grid_cb);

    for (u32 f = 0; f < T1_FRAMES_CAP; f++) {
        T1D3D11QueryDesc query;
        query.query = T1D3D11_QUERY_EVENT;
        query.misc_flags = 0;
        device->vtbl->CreateQuery(device, &query, &T1_d3d->frame_queries[f]);
    }

    u32 source_size = (u32)T1_std_strlen(T1_gpu_d3d11_present_source);
    T1D3DBlob * vs = T1_gpu_d3d11_compile(
        T1_gpu_d3d11_present_source, source_size,
        "T1_present_vertex", "vs_5_0", NULL, 0);
    T1D3DBlob * ps = T1_gpu_d3d11_compile(
        T1_gpu_d3d11_present_source, source_size,
        "T1_present_fragment", "ps_5_0", NULL, 0);
    if (vs != NULL) {
        device->vtbl->CreateVertexShader(
            device,
            vs->vtbl->GetBufferPointer(vs),
            vs->vtbl->GetBufferSize(vs),
            NULL,
            &T1_d3d->present_vs);
    }
    if (ps != NULL) {
        device->vtbl->CreatePixelShader(
            device,
            ps->vtbl->GetBufferPointer(ps),
            ps->vtbl->GetBufferSize(ps),
            NULL,
            &T1_d3d->present_ps);
    }
    T1_D3D_RELEASE(vs);
    T1_D3D_RELEASE(ps);

    for (u32 i = 0; i < 3; i++) {
        if (T1_d3d->samplers[i] == NULL || T1_d3d->raster_states[i] == NULL) {
            *fatal_error = "Failed to create Direct3D 11 states";
            return;
        }
    }
    for (u32 i = 0; i < 4; i++) {
        if (T1_d3d->blend_states[i] == NULL) {
            *fatal_error = "Failed to create Direct3D 11 states";
            return;
        }
    }
    if (
        T1_d3d->depth_state == NULL ||
        T1_d3d->no_depth_state == NULL ||
        T1_d3d->frame_consts_srv == NULL ||
        T1_d3d->params_srv == NULL ||
        T1_d3d->grid_cb == NULL ||
        T1_d3d->present_vs == NULL ||
        T1_d3d->present_ps == NULL)
    {
        *fatal_error = "Failed to create Direct3D 11 states";
        return;
    }
}

void T1_os_gpu_init(char ** const fatal_error) {
    if (*fatal_error != 0) { return; }

    T1_d3d = T1_mem_malloc_unmanaged(sizeof(T1D3DState));
    if (T1_d3d == NULL) {
        *fatal_error = "Malloc fail";
        return;
    }
    T1_std_memset(T1_d3d, 0, sizeof(T1D3DState));
    for (u32 p = 0; p < T1_PIPELINES_CAP; p++) {
        T1_d3d->pipeline_units[p][0] = -1;
        T1_d3d->pipeline_units[p][1] = -1;
    }

    T1WinHandle d3d11 = LoadLibraryW(L"d3d11.dll");
    T1WinHandle compiler = LoadLibraryW(L"d3dcompiler_47.dll");
    T1D3D11CreateDeviceFn create_device = NULL;
    T1WinFarProc found = d3d11 ? GetProcAddress(d3d11, "D3D11CreateDevice") : NULL;
    T1_std_memcpy(&create_device, &found, sizeof(T1WinFarProc));
    found = compiler ? GetProcAddress(compiler, "D3DCompile") : NULL;
    T1_std_memcpy(&T1_d3d->compile, &found, sizeof(T1WinFarProc));
    if (create_device == NULL) {
        *fatal_error = "Failed to load Direct3D 11 (d3d11.dll)";
        return;
    }
    if (T1_d3d->compile == NULL) {
        *fatal_error =
            "Failed to load the HLSL compiler (d3dcompiler_47.dll, part of "
            "Windows since 8.1)";
        return;
    }

    /*
    The GPU, else WARP, Windows' software renderer. Windows 7 doesn't know
    feature level 11_1 and fails if it's asked for, so 11_0 is asked alone
    then
    */
    static const u32 levels[2] = {
        T1D3D_FEATURE_LEVEL_11_1, T1D3D_FEATURE_LEVEL_11_0,
    };
    static const u32 drivers[2] = {
        T1D3D_DRIVER_TYPE_HARDWARE, T1D3D_DRIVER_TYPE_WARP,
    };
    for (u32 d = 0; d < 2 && T1_d3d->device == NULL; d++) {
        for (u32 first = 0; first < 2 && T1_d3d->device == NULL; first++) {
            u32 level = 0;
            create_device(
                NULL,
                drivers[d],
                NULL,
                0,
                levels + first,
                2 - first,
                T1D3D11_SDK_VERSION,
                &T1_d3d->device,
                &level,
                &T1_d3d->ctx);
        }
    }
    if (T1_d3d->device == NULL || T1_d3d->ctx == NULL) {
        *fatal_error =
            "Failed to create a Direct3D 11 device (feature level 11_0)";
        return;
    }

    T1_gpu_d3d11_init_states(fatal_error);
    if (*fatal_error != 0) { return; }

    T1_d3d->active = 1;
}

u8 T1_os_gpu_active(void) {
    return T1_d3d != NULL && T1_d3d->active;
}

// The DXGI adapter (the GPU) of the device, the caller releases it
static T1DXGIObject * T1_gpu_d3d11_adapter(void) {
    T1DXGIObject * dxgi_device = NULL;
    T1DXGIObject * adapter = NULL;
    T1_d3d->device->vtbl->QueryInterface(
        T1_d3d->device, &T1_IID_IDXGIDevice, (void **)&dxgi_device);
    if (dxgi_device != NULL) {
        // IDXGIDevice::GetAdapter
        dxgi_device->vtbl->Method7(dxgi_device, &adapter);
    }
    T1_D3D_RELEASE(dxgi_device);
    return adapter;
}

void T1_os_gpu_get_device_name(c8 * recipient, u32 recipient_cap) {
    recipient[0] = '\0';
    if (!T1_os_gpu_active()) { return; }

    T1DXGIObject * adapter = T1_gpu_d3d11_adapter();
    if (adapter == NULL) { return; }
    // a DXGI_ADAPTER_DESC starts with the name, 128 WCHARs
    u8 desc[512];
    T1_std_memset(desc, 0, sizeof(desc));
    if (adapter->vtbl->Method8(adapter, desc, 0) >= 0) {
        WideCharToMultiByte(
            T1CP_UTF8, 0, (const T1WChar *)desc, -1,
            recipient, (s32)recipient_cap, NULL, NULL);
    }
    T1_D3D_RELEASE(adapter);
}

/*
TEXTURES
*/

static void T1_gpu_d3d11_release_tex_views(s32 array_i) {
    T1_D3D_RELEASE(T1_d3d->tex_srvs[array_i]);
    T1_D3D_RELEASE(T1_d3d->tex_copy_srvs[array_i]);
    T1_D3D_RELEASE(T1_d3d->tex_copies[array_i]);
    for (u32 s = 0; s < T1_TEX_SLICES_CAP; s++) {
        T1_D3D_RELEASE(T1_d3d->tex_rtvs[array_i][s]);
        T1_D3D_RELEASE(T1_d3d->tex_uavs[array_i][s]);
    }
}

static T1D3D11View * T1_gpu_d3d11_array_srv(
    T1D3D11Resource * tex,
    const u32 slices)
{
    T1D3D11SRVDesc desc;
    T1_std_memset(&desc, 0, sizeof(T1D3D11SRVDesc));
    desc.format = T1DXGI_FORMAT_R8G8B8A8_UNORM;
    desc.view_dimension = T1D3D11_SRV_DIMENSION_TEXTURE2DARRAY;
    desc.u[0] = 0; // most detailed mip
    desc.u[1] = 0xFFFFFFFF; // all mips
    desc.u[2] = 0; // first slice
    desc.u[3] = slices;
    T1D3D11View * srv = NULL;
    T1_d3d->device->vtbl->CreateShaderResourceView(
        T1_d3d->device, tex, &desc, &srv);
    return srv;
}

// What draws into a slice, made on first use
static T1D3D11View * T1_gpu_d3d11_tex_rtv(s32 array_i, s32 slice_i) {
    if (
        array_i < 0 || array_i >= T1_TEXARRAYS_CAP ||
        slice_i < 0 || slice_i >= T1_TEX_SLICES_CAP ||
        T1_d3d->texs[array_i] == NULL ||
        (u32)slice_i >= T1_d3d->tex_slices[array_i])
    {
        return NULL;
    }
    if (T1_d3d->tex_rtvs[array_i][slice_i] == NULL) {
        T1D3D11RTVDesc desc;
        T1_std_memset(&desc, 0, sizeof(T1D3D11RTVDesc));
        desc.format = T1DXGI_FORMAT_R8G8B8A8_UNORM;
        desc.view_dimension = T1D3D11_RTV_DIMENSION_TEXTURE2DARRAY;
        desc.u[0] = 0; // mip
        desc.u[1] = (u32)slice_i;
        desc.u[2] = 1;
        T1_d3d->device->vtbl->CreateRenderTargetView(
            T1_d3d->device,
            T1_d3d->texs[array_i],
            &desc,
            &T1_d3d->tex_rtvs[array_i][slice_i]);
    }
    return T1_d3d->tex_rtvs[array_i][slice_i];
}

/*
What a kernel writes a slice through, made on first use: Direct3D 11.0
can't read rgba8 through a view of that format, so it's a view of uints
that T1's shader helpers pack and unpack
*/
static T1D3D11View * T1_gpu_d3d11_tex_uav(s32 array_i, s32 slice_i) {
    if (
        array_i < 0 || array_i >= T1_TEXARRAYS_CAP ||
        slice_i < 0 || slice_i >= T1_TEX_SLICES_CAP ||
        T1_d3d->texs[array_i] == NULL ||
        (u32)slice_i >= T1_d3d->tex_slices[array_i])
    {
        return NULL;
    }
    if (T1_d3d->tex_uavs[array_i][slice_i] == NULL) {
        T1D3D11UAVDesc desc;
        T1_std_memset(&desc, 0, sizeof(T1D3D11UAVDesc));
        desc.format = T1DXGI_FORMAT_R32_UINT;
        desc.view_dimension = T1D3D11_UAV_DIMENSION_TEXTURE2DARRAY;
        desc.u[0] = 0; // mip
        desc.u[1] = (u32)slice_i;
        desc.u[2] = 1;
        T1_d3d->device->vtbl->CreateUnorderedAccessView(
            T1_d3d->device,
            T1_d3d->texs[array_i],
            &desc,
            &T1_d3d->tex_uavs[array_i][slice_i]);
    }
    return T1_d3d->tex_uavs[array_i][slice_i];
}

static u32 T1_gpu_d3d11_mip_levels(s32 array_i) {
    #if T1_MIPMAPS_ACTIVE == T1_ACTIVE
    // like Metal: down to 1 pixel, except array 0
    if (array_i == 0) { return 1; }
    u32 size = T1_tex_arrays[array_i].single_img_width;
    if (T1_tex_arrays[array_i].single_img_height > size) {
        size = T1_tex_arrays[array_i].single_img_height;
    }
    u32 levels = 1;
    while (size > 1) { size /= 2; levels++; }
    return levels;
    #elif T1_MIPMAPS_ACTIVE == T1_INACTIVE
    (void)array_i;
    return 1;
    #else
    #error
    #endif
}

void T1_os_gpu_update_capacity_if_needed(s32 tex_array_i) {
    if (!T1_os_gpu_active()) { return; }
    T1_assert(tex_array_i >= 0 && tex_array_i < T1_TEXARRAYS_CAP);
    if (tex_array_i < 0 || tex_array_i >= T1_TEXARRAYS_CAP) { return; }

    T1TexArray * array = &T1_tex_arrays[tex_array_i];
    T1D3D11Resource * prev = NULL;
    u32 prev_slices = 0;

    if (array->deleted) {
        T1_gpu_d3d11_release_tex_views(tex_array_i);
        T1_D3D_RELEASE(T1_d3d->texs[tex_array_i]);
        return;
    } else if (T1_d3d->texs[tex_array_i] != NULL) {
        b8 same_size =
            T1_d3d->tex_widths[tex_array_i] == array->single_img_width &&
            T1_d3d->tex_heights[tex_array_i] == array->single_img_height;
        if (
            same_size &&
            array->images_size <= T1_d3d->tex_slices[tex_array_i])
        {
            return;
        }
        if (same_size) {
            // more slices: a bigger texture, the old slices copied over
            prev = T1_d3d->texs[tex_array_i];
            prev_slices = T1_d3d->tex_slices[tex_array_i];
            T1_d3d->texs[tex_array_i] = NULL;
        } else {
            T1_D3D_RELEASE(T1_d3d->texs[tex_array_i]);
        }
    }
    T1_gpu_d3d11_release_tex_views(tex_array_i);

    u32 slices = array->images_size > 0 ? array->images_size : 1;
    u32 mips = T1_gpu_d3d11_mip_levels(tex_array_i);
    T1D3D11Texture2DDesc desc;
    T1_std_memset(&desc, 0, sizeof(T1D3D11Texture2DDesc));
    desc.width = array->single_img_width;
    desc.height = array->single_img_height;
    desc.mip_levels = mips;
    desc.array_size = slices;
    // typeless, so kernels can write it as uints, see T1_gpu_d3d11_tex_uav()
    desc.format = T1DXGI_FORMAT_R8G8B8A8_TYPELESS;
    desc.sample_count = 1;
    desc.usage = T1D3D11_USAGE_DEFAULT;
    // any texture can be sampled, drawn to and written by compute passes
    desc.bind_flags =
        T1D3D11_BIND_SHADER_RESOURCE |
        T1D3D11_BIND_RENDER_TARGET |
        T1D3D11_BIND_UNORDERED_ACCESS;
    desc.misc_flags = mips > 1 ? T1D3D11_RESOURCE_MISC_GENERATE_MIPS : 0;

    T1_d3d->device->vtbl->CreateTexture2D(
        T1_d3d->device, &desc, NULL, &T1_d3d->texs[tex_array_i]);
    T1_assert(T1_d3d->texs[tex_array_i] != NULL);
    if (T1_d3d->texs[tex_array_i] == NULL) {
        T1_D3D_RELEASE(prev);
        return;
    }
    T1_d3d->tex_slices[tex_array_i] = slices;
    T1_d3d->tex_mips[tex_array_i] = mips;
    T1_d3d->tex_widths[tex_array_i] = array->single_img_width;
    T1_d3d->tex_heights[tex_array_i] = array->single_img_height;
    T1_d3d->tex_srvs[tex_array_i] = T1_gpu_d3d11_array_srv(
        T1_d3d->texs[tex_array_i], slices);
    array->gpu_capacity = slices;

    // Direct3D doesn't clear new textures, blank T1Texs start transparent
    f32 clear[4] = {0.0f, 0.0f, 0.0f, 0.0f};
    for (u32 s = prev_slices; s < slices; s++) {
        T1D3D11View * rtv = T1_gpu_d3d11_tex_rtv(tex_array_i, (s32)s);
        if (rtv != NULL) {
            T1_d3d->ctx->vtbl->ClearRenderTargetView(T1_d3d->ctx, rtv, clear);
        }
    }

    // the old slices, every mip of them
    for (u32 s = 0; s < prev_slices && prev != NULL; s++) {
        for (u32 m = 0; m < mips; m++) {
            T1_d3d->ctx->vtbl->CopySubresourceRegion(
                T1_d3d->ctx,
                T1_d3d->texs[tex_array_i], m + s * mips,
                0, 0, 0,
                prev, m + s * mips,
                NULL);
        }
    }
    T1_D3D_RELEASE(prev);
}

void T1_os_gpu_delete_texture_array(s32 array_i) {
    if (!T1_os_gpu_active()) { return; }
    T1_gpu_d3d11_release_tex_views(array_i);
    T1_D3D_RELEASE(T1_d3d->texs[array_i]);
    T1_d3d->tex_slices[array_i] = 0;
}

void T1_os_gpu_push_tex_slice(s32 tex_array_i, s32 tex_slice_i) {
    if (!T1_os_gpu_active()) { return; }
    T1_assert(tex_array_i >= 0 && tex_array_i < T1_TEXARRAYS_CAP);
    T1_assert(tex_slice_i >= 0);

    T1Img * image = &T1_tex_arrays[tex_array_i].images[tex_slice_i].image;
    T1_assert(image->rgba_values_page_aligned != NULL);
    if (image->rgba_values_page_aligned == NULL) { return; }

    T1_os_gpu_update_capacity_if_needed(tex_array_i);
    if (T1_d3d->texs[tex_array_i] == NULL) { return; }

    u32 width = T1_tex_arrays[tex_array_i].single_img_width;
    u32 height = T1_tex_arrays[tex_array_i].single_img_height;
    T1_d3d->ctx->vtbl->UpdateSubresource(
        T1_d3d->ctx,
        T1_d3d->texs[tex_array_i],
        (u32)tex_slice_i * T1_d3d->tex_mips[tex_array_i],
        NULL,
        image->rgba_values_page_aligned,
        width * 4,
        width * height * 4);

    // like on Metal, the GPU has the pixels now
    if (image->rgba_values_freeable != NULL) {
        T1_mem_free_managed(image->rgba_values_freeable);
        image->rgba_values_freeable = NULL;
        image->rgba_values_page_aligned = NULL;
    }
}

#if T1_MIPMAPS_ACTIVE == T1_ACTIVE
void T1_os_gpu_generate_mipmaps_for_texture_array(s32 texture_array_i) {
    if (!T1_os_gpu_active()) { return; }
    if (
        T1_d3d->tex_srvs[texture_array_i] != NULL &&
        T1_d3d->tex_mips[texture_array_i] > 1)
    {
        T1_d3d->ctx->vtbl->GenerateMips(
            T1_d3d->ctx, T1_d3d->tex_srvs[texture_array_i]);
    }
}
#elif T1_MIPMAPS_ACTIVE == T1_INACTIVE
#else
#error
#endif

// A texture the CPU reads, w * h rgba8 pixels, the caller releases it
static T1D3D11Resource * T1_gpu_d3d11_new_staging_tex(u32 w, u32 h) {
    T1D3D11Texture2DDesc desc;
    T1_std_memset(&desc, 0, sizeof(T1D3D11Texture2DDesc));
    desc.width = w;
    desc.height = h;
    desc.mip_levels = 1;
    desc.array_size = 1;
    desc.format = T1DXGI_FORMAT_R8G8B8A8_UNORM;
    desc.sample_count = 1;
    desc.usage = T1D3D11_USAGE_STAGING;
    desc.cpu_access_flags = T1D3D11_CPU_ACCESS_READ;
    T1D3D11Resource * staging = NULL;
    T1_d3d->device->vtbl->CreateTexture2D(T1_d3d->device, &desc, NULL, &staging);
    return staging;
}

// Copies w * h pixels from a mapped staging texture, row by row
static void T1_gpu_d3d11_copy_rows(
    u8 * recip,
    const T1D3D11MappedSubresource * mapped,
    u32 w,
    u32 h)
{
    for (u32 row = 0; row < h; row++) {
        T1_std_memcpy(
            recip + (u64)row * w * 4,
            (u8 *)mapped->data + (u64)row * mapped->row_pitch,
            (u64)w * 4);
    }
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
    *good = 0;
    *recipient_size = 0;
    if (
        !T1_os_gpu_active() ||
        texture_array_i < 0 || texture_array_i >= T1_TEXARRAYS_CAP ||
        T1_d3d->texs[texture_array_i] == NULL)
    {
        return;
    }

    u32 w = T1_tex_arrays[texture_array_i].single_img_width;
    u32 h = T1_tex_arrays[texture_array_i].single_img_height;
    if (w < 1 || h < 1 || recipient_cap < w * h * 4) { return; }

    T1D3D11Resource * staging = T1_gpu_d3d11_new_staging_tex(w, h);
    if (staging == NULL) { return; }
    T1_d3d->ctx->vtbl->CopySubresourceRegion(
        T1_d3d->ctx,
        staging, 0, 0, 0, 0,
        T1_d3d->texs[texture_array_i],
        (u32)texture_i * T1_d3d->tex_mips[texture_array_i],
        NULL);

    // waits for the GPU
    T1D3D11MappedSubresource mapped;
    if (
        T1_d3d->ctx->vtbl->Map(
            T1_d3d->ctx, staging, 0, T1D3D11_MAP_READ, 0, &mapped) >= 0)
    {
        T1_gpu_d3d11_copy_rows(rgba_recipient, &mapped, w, h);
        T1_d3d->ctx->vtbl->Unmap(T1_d3d->ctx, staging, 0);
        *recipient_width = w;
        *recipient_height = h;
        *recipient_size = w * h * 4;
        *good = 1;
    }
    T1_D3D_RELEASE(staging);
}
#elif T1_TEXTURES_ACTIVE == T1_INACTIVE
#else
#error
#endif

/*
BUFFERS
*/

void T1_os_gpu_buf_new(u32 buf_i, u64 size, u8 * good) {
    *good = 0;
    T1_assert(buf_i < T1_BUFS_CAP);
    if (!T1_os_gpu_active() || buf_i >= T1_BUFS_CAP) { return; }
    T1_assert(T1_d3d->bufs[buf_i] == NULL);

    // raw views count in 4 byte steps
    u64 rounded = (size + 3) & ~(u64)3;
    if (rounded > 0xFFFFFFF0u) { return; }
    T1_d3d->bufs[buf_i] = T1_gpu_d3d11_new_raw_buffer(rounded);
    if (T1_d3d->bufs[buf_i] == NULL) { return; }
    T1_d3d->buf_srvs[buf_i] = T1_gpu_d3d11_raw_srv(T1_d3d->bufs[buf_i], rounded);
    T1_d3d->buf_uavs[buf_i] = T1_gpu_d3d11_raw_uav(T1_d3d->bufs[buf_i], rounded);
    *good =
        T1_d3d->buf_srvs[buf_i] != NULL &&
        T1_d3d->buf_uavs[buf_i] != NULL;
}

/*
Direct3D applies updates in the order they're made, before anything
encoded after them: before the next frame's passes, after the ones of the
frame before
*/
void T1_os_gpu_buf_write(
    u32 buf_i,
    u64 offset,
    const void * data,
    u64 size)
{
    if (!T1_os_gpu_active()) { return; }
    T1_assert(buf_i < T1_BUFS_CAP && T1_d3d->bufs[buf_i] != NULL);
    if (buf_i >= T1_BUFS_CAP || T1_d3d->bufs[buf_i] == NULL) { return; }

    T1D3D11Box box;
    box.left = (u32)offset;
    box.right = (u32)(offset + size);
    box.top = 0;
    box.bottom = 1;
    box.front = 0;
    box.back = 1;
    T1_d3d->ctx->vtbl->UpdateSubresource(
        T1_d3d->ctx, T1_d3d->bufs[buf_i], 0, &box, data, 0, 0);
}

void T1_os_gpu_buf_delete(u32 buf_i) {
    if (!T1_os_gpu_active() || buf_i >= T1_BUFS_CAP) { return; }
    T1_D3D_RELEASE(T1_d3d->buf_srvs[buf_i]);
    T1_D3D_RELEASE(T1_d3d->buf_uavs[buf_i]);
    T1_D3D_RELEASE(T1_d3d->bufs[buf_i]);
}

static u32 T1_gpu_d3d11_new_read(void) {
    for (u32 i = 0; i < T1_BUF_READS_CAP; i++) {
        if (T1_d3d->reads[i].state == T1_D3D_READ_FREE) {
            T1_std_memset(&T1_d3d->reads[i], 0, sizeof(T1D3DRead));
            T1_d3d->reads[i].state = T1_D3D_READ_QUEUED;
            return i;
        }
    }
    return T1_BUF_READ_NONE;
}

u32 T1_os_gpu_buf_read_start(u32 buf_i, u64 offset, u64 size) {
    if (!T1_os_gpu_active()) { return T1_BUF_READ_NONE; }

    T1D3D11BufferDesc desc;
    T1_std_memset(&desc, 0, sizeof(T1D3D11BufferDesc));
    desc.byte_width = (u32)((size + 3) & ~(u64)3);
    desc.usage = T1D3D11_USAGE_STAGING;
    desc.cpu_access_flags = T1D3D11_CPU_ACCESS_READ;
    T1D3D11Resource * staging = NULL;
    T1_d3d->device->vtbl->CreateBuffer(T1_d3d->device, &desc, NULL, &staging);
    if (staging == NULL) { return T1_BUF_READ_NONE; }

    u32 read_i = T1_gpu_d3d11_new_read();
    if (read_i == T1_BUF_READ_NONE) {
        T1_D3D_RELEASE(staging);
        return T1_BUF_READ_NONE;
    }
    T1D3DRead * r = &T1_d3d->reads[read_i];
    r->staging = staging;
    r->buf_i = buf_i;
    r->offset = offset;
    r->size = size;
    return read_i;
}

u32 T1_os_gpu_tex_read_start(u32 tex, u32 x, u32 y, u32 w, u32 h) {
    if (!T1_os_gpu_active()) { return T1_BUF_READ_NONE; }

    T1D3D11Resource * staging = T1_gpu_d3d11_new_staging_tex(w, h);
    if (staging == NULL) { return T1_BUF_READ_NONE; }

    u32 read_i = T1_gpu_d3d11_new_read();
    if (read_i == T1_BUF_READ_NONE) {
        T1_D3D_RELEASE(staging);
        return T1_BUF_READ_NONE;
    }
    T1D3DRead * r = &T1_d3d->reads[read_i];
    r->staging = staging;
    r->buf_i = T1_BUFS_CAP;
    r->tex = tex;
    r->tex_x = x;
    r->tex_y = y;
    r->tex_w = w;
    r->tex_h = h;
    r->size = (u64)w * h * 4;
    return read_i;
}

// Copies the queued reads to their staging, after the frame's passes
static void T1_gpu_d3d11_encode_reads(void) {
    for (u32 i = 0; i < T1_BUF_READS_CAP; i++) {
        T1D3DRead * r = &T1_d3d->reads[i];
        if (r->state != T1_D3D_READ_QUEUED) { continue; }
        r->state = T1_D3D_READ_ENCODED;

        if (r->buf_i == T1_BUFS_CAP) {
            s32 array_i = T1_TEX_ARRAY_I(r->tex);
            s32 slice_i = T1_TEX_SLICE_I(r->tex);
            if (
                array_i >= T1_TEXARRAYS_CAP ||
                T1_d3d->texs[array_i] == NULL ||
                (u32)slice_i >= T1_d3d->tex_slices[array_i])
            {
                r->missing = 1;
                continue;
            }
            T1D3D11Box box;
            box.left = r->tex_x;
            box.top = r->tex_y;
            box.front = 0;
            box.right = r->tex_x + r->tex_w;
            box.bottom = r->tex_y + r->tex_h;
            box.back = 1;
            T1_d3d->ctx->vtbl->CopySubresourceRegion(
                T1_d3d->ctx,
                r->staging, 0, 0, 0, 0,
                T1_d3d->texs[array_i],
                (u32)slice_i * T1_d3d->tex_mips[array_i],
                &box);
        } else {
            if (r->buf_i >= T1_BUFS_CAP || T1_d3d->bufs[r->buf_i] == NULL) {
                r->missing = 1;
                continue;
            }
            T1D3D11Box box;
            box.left = (u32)r->offset;
            box.right = (u32)(r->offset + ((r->size + 3) & ~(u64)3));
            box.top = 0;
            box.bottom = 1;
            box.front = 0;
            box.back = 1;
            T1_d3d->ctx->vtbl->CopySubresourceRegion(
                T1_d3d->ctx,
                r->staging, 0, 0, 0, 0,
                T1_d3d->bufs[r->buf_i], 0,
                &box);
        }
    }
}

u8 T1_os_gpu_buf_read_finish(u32 read_i, void * recip) {
    if (!T1_os_gpu_active() || read_i >= T1_BUF_READS_CAP) { return 0; }

    T1D3DRead * r = &T1_d3d->reads[read_i];
    T1_assert(r->state != T1_D3D_READ_FREE);
    if (r->state != T1_D3D_READ_ENCODED) { return 0; }

    if (r->missing) {
        // a deleted buffer or texture reads as zeros
        T1_std_memset(recip, 0, r->size);
    } else {
        // only if the GPU is done with it, else again later
        T1D3D11MappedSubresource mapped;
        s32 result = T1_d3d->ctx->vtbl->Map(
            T1_d3d->ctx,
            r->staging,
            0,
            T1D3D11_MAP_READ,
            T1D3D11_MAP_FLAG_DO_NOT_WAIT,
            &mapped);
        if (result == T1DXGI_ERROR_WAS_STILL_DRAWING) { return 0; }
        if (result < 0) {
            T1_std_memset(recip, 0, r->size);
        } else {
            if (r->buf_i == T1_BUFS_CAP) {
                T1_gpu_d3d11_copy_rows(recip, &mapped, r->tex_w, r->tex_h);
            } else {
                T1_std_memcpy(recip, mapped.data, r->size);
            }
            T1_d3d->ctx->vtbl->Unmap(T1_d3d->ctx, r->staging, 0);
        }
    }

    T1_D3D_RELEASE(r->staging);
    r->state = T1_D3D_READ_FREE;
    return 1;
}

/*
SHADERS AND PIPELINES
*/

static void T1_gpu_d3d11_free_shaders(T1D3DShaders * shaders) {
    if (shaders == NULL) { return; }
    for (u32 u = 0; u < shaders->units_size; u++) {
        T1_D3D_RELEASE(shaders->units[u].shader);
    }
    T1_mem_free_managed(shaders);
}

static s32 T1_gpu_d3d11_find_unit(
    const T1D3DShaders * shaders,
    const c8 * name,
    const u8 stage)
{
    if (shaders == NULL) { return -1; }
    for (u32 u = 0; u < shaders->units_size; u++) {
        if (
            shaders->units[u].stage == stage &&
            T1_std_are_equal_strings(shaders->units[u].name, name))
        {
            return (s32)u;
        }
    }
    return -1;
}

/*
The units of a T1_pipelines entry in shaders, 0 if one is missing (then
the names of what's missing go to error_recip, may be NULL)
*/
static b8 T1_gpu_d3d11_pipeline_units(
    const T1D3DShaders * shaders,
    const s32 pipeline_i,
    s32 * recip,
    c8 * error_recip,
    const u32 error_recip_cap)
{
    const T1Pipeline * p = &T1_pipelines[pipeline_i];
    recip[0] = T1_gpu_d3d11_find_unit(
        shaders, p->vertex_fn, p->is_compute ? 3 : 1);
    recip[1] = p->is_compute ?
        -1 :
        T1_gpu_d3d11_find_unit(shaders, p->fragment_fn, 2);

    b8 ok = recip[0] >= 0 && (p->is_compute || recip[1] >= 0);
    if (!ok && error_recip != NULL) {
        T1_std_strcat_cap(error_recip, error_recip_cap,
            "No such function in the shaders: ");
        T1_std_strcat_cap(error_recip, error_recip_cap,
            recip[0] < 0 ? p->vertex_fn : p->fragment_fn);
        T1_std_strcat_cap(error_recip, error_recip_cap, "\n");
    }
    return ok;
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
            *fatal_error = "T1_os_gpu_try_shader_source() needs an error_recip_cap of at least 512";
            return;
        }
        error_recip[0] = '\0';
    }
    if (!T1_os_gpu_active()) {
        *fatal_error = "T1_gpu_try_shader_source() before T1_gpu_init()";
        return;
    }
    if (shader_source == NULL || shader_source[0] == '\0') {
        if (error_recip != NULL) {
            T1_std_strcpy_cap(error_recip, error_recip_cap, "Empty shader source");
        }
        return;
    }

    // every entry point an HLSL unit
    u32 hlsl_cap = 4 * 1024 * 1024 + (u32)T1_std_strlen(shader_source) * 64;
    c8 * hlsl = T1_mem_malloc_managed(hlsl_cap);
    if (hlsl == NULL) { return; }
    u8 translated = 0;
    T1_shader_to_hlsl(
        shader_source,
        hlsl,
        hlsl_cap,
        &translated,
        error_recip,
        error_recip_cap);
    if (!translated) {
        T1_mem_free_managed(hlsl);
        return;
    }

    T1D3DShaders * shaders = T1_mem_malloc_managed(sizeof(T1D3DShaders));
    T1_std_memset(shaders, 0, sizeof(T1D3DShaders));
    b8 ok = 1;

    c8 * unit = hlsl;
    while (ok && unit != NULL && *unit != '\0') {
        // the unit goes up to the next one's marker line
        c8 * next = unit + 1;
        while (*next != '\0') {
            if (
                next[-1] == '\n' &&
                T1_std_are_equal_strings_of_length(next, "//T1_UNIT ", 10))
            {
                break;
            }
            next++;
        }
        if (!T1_std_are_equal_strings_of_length(unit, "//T1_UNIT ", 10)) {
            unit = *next != '\0' ? next : NULL;
            continue;
        }
        if (shaders->units_size >= T1_D3D_UNITS_CAP) {
            ok = 0;
            break;
        }

        T1D3DUnit * u = &shaders->units[shaders->units_size];
        T1_std_memset(u->uav_buf, 0xFF, T1_D3D_UAVS_SIZE);

        // "//T1_UNIT comp name", then "//T1_THREADS x y z", "//T1_UAV s n"
        const c8 * line = unit + 10;
        u->stage =
            line[0] == 'v' ? 1 :
            line[0] == 'f' ? 2 : 3;
        while (*line != ' ' && *line != '\n') { line++; }
        if (*line == ' ') { line++; }
        u32 n = 0;
        while (
            line[n] != '\n' && line[n] != '\r' &&
            n + 1 < T1_PIPELINE_FUNC_NAME_CAP)
        {
            u->name[n] = line[n];
            n++;
        }
        u->name[n] = '\0';

        line = unit;
        while (*line != '\0' && line < next) {
            while (*line != '\n' && *line != '\0') { line++; }
            if (*line == '\n') { line++; }
            if (T1_std_are_equal_strings_of_length(line, "//T1_THREADS ", 13)) {
                const c8 * t = line + 13;
                for (u32 d = 0; d < 3; d++) {
                    u32 value = 0;
                    while (*t >= '0' && *t <= '9') {
                        value = value * 10 + (u32)(*t - '0');
                        t++;
                    }
                    u->threads[d] = value > 0 ? value : 1;
                    while (*t == ' ') { t++; }
                }
            } else if (T1_std_are_equal_strings_of_length(line, "//T1_UAV ", 9)) {
                const c8 * t = line + 9;
                u32 slot = 0;
                u32 buf = 0;
                while (*t >= '0' && *t <= '9') { slot = slot * 10 + (u32)(*t++ - '0'); }
                while (*t == ' ') { t++; }
                while (*t >= '0' && *t <= '9') { buf = buf * 10 + (u32)(*t++ - '0'); }
                if (slot < T1_D3D_UAVS_SIZE && buf < 0xFF) {
                    u->uav_buf[slot] = (u8)buf;
                }
            } else if (!T1_std_are_equal_strings_of_length(line, "//T1_", 5)) {
                break;
            }
        }

        u32 unit_size = (u32)(next - unit);
        // T1_shader_to_hlsl() only defines these when the unit reads textures
        u->reads_textures = 0;
        for (u32 i = 0; i + 17 < unit_size; i++) {
            if (T1_std_are_equal_strings_of_length(unit + i, "float4 t1_sample(", 17)) {
                u->reads_textures = 1;
                break;
            }
        }

        T1D3DBlob * code = T1_gpu_d3d11_compile(
            unit,
            unit_size,
            u->name,
            u->stage == 1 ? "vs_5_0" : u->stage == 2 ? "ps_5_0" : "cs_5_0",
            error_recip,
            error_recip_cap);
        if (code == NULL) {
            ok = 0;
            break;
        }
        const void * bytes = code->vtbl->GetBufferPointer(code);
        size_t size = code->vtbl->GetBufferSize(code);
        T1D3D11Device * device = T1_d3d->device;
        s32 created =
            u->stage == 1 ?
                device->vtbl->CreateVertexShader(device, bytes, size, NULL, &u->shader) :
            u->stage == 2 ?
                device->vtbl->CreatePixelShader(device, bytes, size, NULL, &u->shader) :
                device->vtbl->CreateComputeShader(device, bytes, size, NULL, &u->shader);
        T1_D3D_RELEASE(code);
        if (created < 0 || u->shader == NULL) {
            ok = 0;
            break;
        }
        shaders->units_size += 1;

        unit = *next != '\0' ? next : NULL;
    }
    T1_mem_free_managed(hlsl);

    // the pipelines made so far must find their functions
    s32 (* units)[2] = T1_mem_malloc_managed(sizeof(s32) * 2 * T1_PIPELINES_CAP);
    for (s32 i = 0; i < T1_PIPELINES_CAP; i++) {
        units[i][0] = -1;
        units[i][1] = -1;
        if (
            ok &&
            i < (s32)T1_pipelines_size &&
            T1_pipelines[i].id != T1_ID_NONE &&
            !T1_gpu_d3d11_pipeline_units(
                shaders, i, units[i], error_recip, error_recip_cap))
        {
            ok = 0;
        }
    }

    if (!ok) {
        // the shaders loaded before stay
        T1_gpu_d3d11_free_shaders(shaders);
        T1_mem_free_managed(units);
        return;
    }

    T1_gpu_d3d11_free_shaders(T1_d3d->shaders);
    T1_d3d->shaders = shaders;
    for (s32 i = 0; i < T1_PIPELINES_CAP; i++) {
        T1_d3d->pipeline_units[i][0] = units[i][0];
        T1_d3d->pipeline_units[i][1] = units[i][1];
    }
    T1_mem_free_managed(units);
    *good = 1;
}

u8 T1_os_gpu_shaders_ready(void) {
    return T1_os_gpu_active() && T1_d3d->shaders != NULL;
}

void T1_os_gpu_pipeline_added(s32 pipeline_i, u8 * good) {
    *good = 1;
    if (!T1_os_gpu_shaders_ready()) {
        // checked when the shaders load
        return;
    }
    *good = T1_gpu_d3d11_pipeline_units(
        T1_d3d->shaders,
        pipeline_i,
        T1_d3d->pipeline_units[pipeline_i],
        NULL,
        0);
}

void T1_os_gpu_pipeline_deleted(s32 pipeline_i) {
    if (!T1_os_gpu_active()) { return; }
    T1_d3d->pipeline_units[pipeline_i][0] = -1;
    T1_d3d->pipeline_units[pipeline_i][1] = -1;
}

/*
PASSES
*/

/*
The depth buffer of a pass's "depth_tex", made (again) when it's new or its
size changed. *clear is set for the first pass of the frame using it. When
all slots are taken, the one used longest ago is reused.
*/
static T1D3D11View * T1_gpu_d3d11_tex_depth(
    u32 tex,
    u32 width,
    u32 height,
    b8 * clear)
{
    T1D3DTexDepth * found = NULL;
    T1D3DTexDepth * oldest = &T1_d3d->tex_depths[0];
    for (u32 i = 0; i < T1_D3D_TEX_DEPTHS_CAP; i++) {
        T1D3DTexDepth * d = &T1_d3d->tex_depths[i];
        if (d->depth != NULL && d->tex == tex) { found = d; break; }
        if (d->last_used_frame < oldest->last_used_frame) { oldest = d; }
    }
    if (found == NULL) {
        found = oldest;
        T1_D3D_RELEASE(found->dsv);
        T1_D3D_RELEASE(found->depth);
        found->tex = tex;
    }

    if (found->depth == NULL || found->width != width || found->height != height) {
        T1_D3D_RELEASE(found->dsv);
        T1_D3D_RELEASE(found->depth);
        T1D3D11Texture2DDesc desc;
        T1_std_memset(&desc, 0, sizeof(T1D3D11Texture2DDesc));
        desc.width = width;
        desc.height = height;
        desc.mip_levels = 1;
        desc.array_size = 1;
        desc.format = T1DXGI_FORMAT_D32_FLOAT;
        desc.sample_count = 1;
        desc.usage = T1D3D11_USAGE_DEFAULT;
        desc.bind_flags = T1D3D11_BIND_DEPTH_STENCIL;
        T1_d3d->device->vtbl->CreateTexture2D(
            T1_d3d->device, &desc, NULL, &found->depth);
        if (found->depth != NULL) {
            T1D3D11DSVDesc dsv;
            T1_std_memset(&dsv, 0, sizeof(T1D3D11DSVDesc));
            dsv.format = T1DXGI_FORMAT_D32_FLOAT;
            dsv.view_dimension = T1D3D11_DSV_DIMENSION_TEXTURE2D;
            T1_d3d->device->vtbl->CreateDepthStencilView(
                T1_d3d->device, found->depth, &dsv, &found->dsv);
        }
        found->width = width;
        found->height = height;
        found->cleared = 0;
    }

    found->last_used_frame = T1_d3d->frames_encoded + 1;
    *clear = !found->cleared;
    found->cleared = 1;
    return found->dsv;
}

/*
The texture arrays a pass's shaders read, in srvs. An array the pass also
draws or writes into (target_arrays) can't be read: if the pass reads
textures, it reads a copy of the array made now instead
*/
static void T1_gpu_d3d11_tex_srvs(
    T1D3D11View ** srvs,
    const b8 * target_arrays,
    const b8 reads_textures)
{
    for (u32 a = 0; a < T1_TEXARRAYS_CAP; a++) {
        srvs[a] = T1_d3d->tex_srvs[a];
        if (!target_arrays[a] || T1_d3d->texs[a] == NULL) { continue; }
        srvs[a] = NULL;
        if (!reads_textures) { continue; }

        if (T1_d3d->tex_copies[a] == NULL) {
            T1D3D11Texture2DDesc desc;
            T1_std_memset(&desc, 0, sizeof(T1D3D11Texture2DDesc));
            desc.width = T1_tex_arrays[a].single_img_width;
            desc.height = T1_tex_arrays[a].single_img_height;
            desc.mip_levels = T1_d3d->tex_mips[a];
            desc.array_size = T1_d3d->tex_slices[a];
            desc.format = T1DXGI_FORMAT_R8G8B8A8_TYPELESS;
            desc.sample_count = 1;
            desc.usage = T1D3D11_USAGE_DEFAULT;
            desc.bind_flags = T1D3D11_BIND_SHADER_RESOURCE;
            T1_d3d->device->vtbl->CreateTexture2D(
                T1_d3d->device, &desc, NULL, &T1_d3d->tex_copies[a]);
            if (T1_d3d->tex_copies[a] == NULL) { continue; }
            T1_d3d->tex_copy_srvs[a] = T1_gpu_d3d11_array_srv(
                T1_d3d->tex_copies[a], T1_d3d->tex_slices[a]);
        }
        T1_d3d->ctx->vtbl->CopyResource(
            T1_d3d->ctx, T1_d3d->tex_copies[a], T1_d3d->texs[a]);
        srvs[a] = T1_d3d->tex_copy_srvs[a];
    }
}

/*
Every SRV a pass's shaders can read: textures (see above), the frame
consts, the params (set for this pass now) and the pass's buffers, except
those in except_bufs (a kernel writes them)
*/
static void T1_gpu_d3d11_pass_srvs(
    T1D3D11View ** srvs,
    const T1Pass * pass,
    const b8 * target_arrays,
    const b8 reads_textures,
    const b8 * except_bufs)
{
    T1_std_memset(srvs, 0, sizeof(T1D3D11View *) * T1_D3D_SRVS_SIZE);
    T1_gpu_d3d11_tex_srvs(srvs, target_arrays, reads_textures);

    T1_d3d->ctx->vtbl->UpdateSubresource(
        T1_d3d->ctx, T1_d3d->params_buf, 0, NULL, pass->u32s.params, 0, 0);
    srvs[T1_HLSL_BUFS_T + T1_FRAME_CONSTS_I] = T1_d3d->frame_consts_srv;
    srvs[T1_HLSL_BUFS_T + T1_PARAMS_I] = T1_d3d->params_srv;

    for (u32 k = 0; k < T1_PASS_BUFS_CAP; k++) {
        u32 buf = pass->u32s.buf[k];
        if (buf == T1_BUF_NONE || except_bufs[k]) { continue; }
        T1_assert(buf < T1_BUFS_CAP && T1_d3d->bufs[buf] != NULL);
        if (buf >= T1_BUFS_CAP) { continue; }
        srvs[T1_HLSL_BUFS_T + T1_BUFS_I + k] = T1_d3d->buf_srvs[buf];
    }
}

static void T1_gpu_d3d11_viewport(u32 width, u32 height) {
    T1D3D11Viewport viewport;
    viewport.top_left_x = 0.0f;
    viewport.top_left_y = 0.0f;
    viewport.width = (f32)width;
    viewport.height = (f32)height;
    viewport.min_depth = 0.0f;
    viewport.max_depth = 1.0f;
    T1_d3d->ctx->vtbl->RSSetViewports(T1_d3d->ctx, 1, &viewport);
}

static void T1_gpu_d3d11_encode_clear_pass(const T1Pass * pass) {
    u32 tex = pass->u32s.color_tex[0];
    if (tex == T1_TEX_NONE) { return; }
    T1D3D11View * rtv = T1_gpu_d3d11_tex_rtv(
        T1_TEX_ARRAY_I(tex), T1_TEX_SLICE_I(tex));
    if (rtv == NULL) { return; }
    T1_d3d->ctx->vtbl->ClearRenderTargetView(T1_d3d->ctx, rtv, pass->clear_rgba);
}

static void T1_gpu_d3d11_encode_draw_pass(const T1Pass * pass) {
    T1D3D11Context * ctx = T1_d3d->ctx;
    s32 pipeline_i = T1_pipeline_id_to_i(pass->pipeline);
    T1_assert(pipeline_i >= 0);
    if (pipeline_i < 0 || pass->u32s.vertex_count < 1) { return; }
    s32 vs_i = T1_d3d->pipeline_units[pipeline_i][0];
    s32 ps_i = T1_d3d->pipeline_units[pipeline_i][1];
    if (vs_i < 0 || ps_i < 0) { return; }
    const T1D3DUnit * vs = &T1_d3d->shaders->units[vs_i];
    const T1D3DUnit * ps = &T1_d3d->shaders->units[ps_i];

    T1D3D11View * rtvs[T1_PASS_COLOR_TEXS_CAP];
    b8 target_arrays[T1_TEXARRAYS_CAP];
    T1_std_memset(target_arrays, 0, sizeof(target_arrays));
    u32 rtvs_size = 0;
    u32 width = 0;
    u32 height = 0;
    for (u32 k = 0; k < T1_PASS_COLOR_TEXS_CAP; k++) {
        rtvs[k] = NULL;
        u32 tex = pass->u32s.color_tex[k];
        if (tex == T1_TEX_NONE) { continue; }
        s32 array_i = T1_TEX_ARRAY_I(tex);
        rtvs[k] = T1_gpu_d3d11_tex_rtv(array_i, T1_TEX_SLICE_I(tex));
        if (rtvs[k] == NULL) { continue; }
        target_arrays[array_i] = 1;
        rtvs_size = k + 1;
        if (width == 0) {
            width = T1_tex_get_width((T1Tex)tex);
            height = T1_tex_get_height((T1Tex)tex);
        }
    }

    // the "depth_tex"'s depth buffer, cleared by its first pass of the frame
    T1D3D11View * dsv = NULL;
    if (pass->u32s.depth_tex != T1_TEX_NONE) {
        u32 depth_w = T1_tex_get_width((T1Tex)pass->u32s.depth_tex);
        u32 depth_h = T1_tex_get_height((T1Tex)pass->u32s.depth_tex);
        T1_assert(width == 0 || (width == depth_w && height == depth_h));
        b8 clear = 0;
        dsv = T1_gpu_d3d11_tex_depth(
            pass->u32s.depth_tex, depth_w, depth_h, &clear);
        if (dsv != NULL && clear) {
            ctx->vtbl->ClearDepthStencilView(
                ctx, dsv, T1D3D11_CLEAR_DEPTH, T1_GLOBAL_CLEARDEPTH, 0);
        }
        if (width == 0) {
            width = depth_w;
            height = depth_h;
        }
    }

    // nothing to draw into
    if (rtvs_size == 0 && dsv == NULL) { return; }

    // targets first: binding them unbinds whatever reads them
    ctx->vtbl->OMSetRenderTargets(ctx, rtvs_size, rtvs, dsv);

    T1D3D11View * srvs[T1_D3D_SRVS_SIZE];
    b8 no_bufs[T1_PASS_BUFS_CAP];
    T1_std_memset(no_bufs, 0, sizeof(no_bufs));
    T1_gpu_d3d11_pass_srvs(
        srvs,
        pass,
        target_arrays,
        vs->reads_textures || ps->reads_textures,
        no_bufs);
    ctx->vtbl->VSSetShaderResources(ctx, 0, T1_D3D_SRVS_SIZE, srvs);
    ctx->vtbl->PSSetShaderResources(ctx, 0, T1_D3D_SRVS_SIZE, srvs);
    ctx->vtbl->VSSetSamplers(ctx, 0, 3, T1_d3d->samplers);
    ctx->vtbl->PSSetSamplers(ctx, 0, 3, T1_d3d->samplers);

    const T1Pipeline * p = &T1_pipelines[pipeline_i];
    ctx->vtbl->VSSetShader(ctx, vs->shader, NULL, 0);
    ctx->vtbl->PSSetShader(ctx, ps->shader, NULL, 0);
    ctx->vtbl->IASetInputLayout(ctx, NULL);
    ctx->vtbl->IASetPrimitiveTopology(
        ctx, T1D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
    ctx->vtbl->RSSetState(
        ctx, T1_d3d->raster_states[p->cull <= T1_CULL_FRONT ? p->cull : 0]);
    f32 blend_factor[4] = {0.0f, 0.0f, 0.0f, 0.0f};
    ctx->vtbl->OMSetBlendState(
        ctx,
        T1_d3d->blend_states[p->blend <= T1_BLEND_STRAIGHT_ALPHA ? p->blend : 0],
        blend_factor,
        0xFFFFFFFF);
    ctx->vtbl->OMSetDepthStencilState(
        ctx, dsv != NULL ? T1_d3d->depth_state : T1_d3d->no_depth_state, 0);
    T1_gpu_d3d11_viewport(width, height);

    ctx->vtbl->Draw(ctx, pass->u32s.vertex_count, pass->u32s.vertex_start);

    T1_gpu_d3d11_unbind_all();
}

static void T1_gpu_d3d11_encode_compute_pass(const T1Pass * pass) {
    T1D3D11Context * ctx = T1_d3d->ctx;
    s32 pipeline_i = T1_pipeline_id_to_i(pass->pipeline);
    T1_assert(pipeline_i >= 0);
    if (pipeline_i < 0) { return; }
    s32 cs_i = T1_d3d->pipeline_units[pipeline_i][0];
    if (cs_i < 0) { return; }
    const T1D3DUnit * cs = &T1_d3d->shaders->units[cs_i];

    // a grid of threads, a texture to write, or both
    b8 has_threads = pass->u32s.threads[0] > 0;
    T1_assert(has_threads || pass->u32s.write_tex != T1_TEX_NONE);

    T1D3D11View * uavs[T1_D3D_UAVS_SIZE];
    T1_std_memset(uavs, 0, sizeof(uavs));
    b8 target_arrays[T1_TEXARRAYS_CAP];
    T1_std_memset(target_arrays, 0, sizeof(target_arrays));
    u32 grid[3];
    if (pass->u32s.write_tex != T1_TEX_NONE) {
        u32 tex = pass->u32s.write_tex;
        s32 array_i = T1_TEX_ARRAY_I(tex);
        uavs[0] = T1_gpu_d3d11_tex_uav(array_i, T1_TEX_SLICE_I(tex));
        if (uavs[0] == NULL) { return; }
        target_arrays[array_i] = 1;
        grid[0] = T1_tex_get_width((T1Tex)tex);
        grid[1] = T1_tex_get_height((T1Tex)tex);
        grid[2] = 1;
    } else if (!has_threads) {
        return;
    }
    if (has_threads) {
        grid[0] = pass->u32s.threads[0];
        grid[1] = pass->u32s.threads[1] > 0 ? pass->u32s.threads[1] : 1;
        grid[2] = pass->u32s.threads[2] > 0 ? pass->u32s.threads[2] : 1;
    }

    // the buffers it writes, the others it reads
    b8 written[T1_PASS_BUFS_CAP];
    T1_std_memset(written, 0, sizeof(written));
    for (u32 slot = 1; slot < T1_D3D_UAVS_SIZE; slot++) {
        u32 n = cs->uav_buf[slot];
        if (n == 0xFF || n < T1_BUFS_I || n >= T1_BUFS_I + T1_PASS_BUFS_CAP) {
            continue;
        }
        u32 k = n - T1_BUFS_I;
        u32 buf = pass->u32s.buf[k];
        if (buf >= T1_BUFS_CAP || T1_d3d->bufs[buf] == NULL) { continue; }
        uavs[slot] = T1_d3d->buf_uavs[buf];
        written[k] = 1;
    }

    T1D3D11View * srvs[T1_D3D_SRVS_SIZE];
    T1_gpu_d3d11_pass_srvs(
        srvs, pass, target_arrays, cs->reads_textures, written);

    u32 grid_cb[4] = {grid[0], grid[1], grid[2], 0};
    ctx->vtbl->UpdateSubresource(ctx, T1_d3d->grid_cb, 0, NULL, grid_cb, 0, 0);

    ctx->vtbl->CSSetShader(ctx, cs->shader, NULL, 0);
    ctx->vtbl->CSSetUnorderedAccessViews(ctx, 0, T1_D3D_UAVS_SIZE, uavs, NULL);
    ctx->vtbl->CSSetShaderResources(ctx, 0, T1_D3D_SRVS_SIZE, srvs);
    ctx->vtbl->CSSetSamplers(ctx, 0, 3, T1_d3d->samplers);
    ctx->vtbl->CSSetConstantBuffers(ctx, 0, 1, &T1_d3d->grid_cb);

    // enough groups to cover the grid, threads past it return right away
    ctx->vtbl->Dispatch(
        ctx,
        (grid[0] + cs->threads[0] - 1) / cs->threads[0],
        (grid[1] + cs->threads[1] - 1) / cs->threads[1],
        (grid[2] + cs->threads[2] - 1) / cs->threads[2]);

    T1_gpu_d3d11_unbind_all();
}

/*
FRAMES
*/

/*
Waits until the frame that last used this slot, T1_FRAMES_CAP frames ago,
is done: this limits the number of frames in flight
*/
u8 T1_os_gpu_begin_frame(void) {
    if (!T1_os_gpu_active()) { return 0; }

    u32 f = T1_d3d->frame_i;
    if (T1_d3d->frame_query_issued[f] && T1_d3d->frame_queries[f] != NULL) {
        // S_FALSE (1) while it isn't done
        while (
            T1_d3d->ctx->vtbl->GetData(
                T1_d3d->ctx, T1_d3d->frame_queries[f], NULL, 0, 0) == 1)
        {
            Sleep(0);
        }
        T1_d3d->frame_query_issued[f] = 0;
    }
    return 1;
}

u8 T1_os_gpu_render_frame(void) {
    if (!T1_os_gpu_active()) { return 0; }

    T1GPUFrameConsts consts = T1_global->frame_consts;
    T1_d3d->ctx->vtbl->UpdateSubresource(
        T1_d3d->ctx, T1_d3d->frame_consts_buf, 0, NULL, &consts, 0, 0);

    for (u32 i = 0; i < T1_D3D_TEX_DEPTHS_CAP; i++) {
        T1_d3d->tex_depths[i].cleared = 0;
    }

    // the client's passes, in the order they were made. Without shaders
    // (e.g. an app that uploads its own pixels) only clear passes run
    for (u32 pass_i = 0; pass_i < T1_passes_size; pass_i++) {
        const T1Pass * pass = T1_passes + pass_i;
        if (!T1_os_gpu_shaders_ready() && pass->kind != T1_PASS_CLEAR) {
            continue;
        }
        switch (pass->kind) {
            case T1_PASS_CLEAR:
                T1_gpu_d3d11_encode_clear_pass(pass);
                break;
            case T1_PASS_DRAW:
                T1_gpu_d3d11_encode_draw_pass(pass);
                break;
            case T1_PASS_COMPUTE:
                T1_gpu_d3d11_encode_compute_pass(pass);
                break;
            default:
                T1_assert(0);
        }
    }

    T1_gpu_d3d11_encode_reads();

    u32 f = T1_d3d->frame_i;
    if (T1_d3d->frame_queries[f] != NULL) {
        T1_d3d->ctx->vtbl->End(T1_d3d->ctx, T1_d3d->frame_queries[f]);
        T1_d3d->frame_query_issued[f] = 1;
    }
    T1_d3d->frames_encoded += 1;
    T1_d3d->frame_i = (T1_d3d->frame_i + 1) % T1_FRAMES_CAP;

    return 1;
}

/*
THE WINDOW
*/

static void T1_gpu_d3d11_release_swap_chain(void) {
    T1_D3D_RELEASE(T1_d3d->back_buffer_rtv);
    T1_D3D_RELEASE(T1_d3d->swap_chain);
    T1_d3d->swap_chain_hwnd = NULL;
}

// The window's swap chain, made with the device's own DXGI factory
static b8 T1_gpu_d3d11_make_swap_chain(T1WinHandle hwnd, u32 w, u32 h) {
    T1DXGIObject * adapter = T1_gpu_d3d11_adapter();
    T1DXGIObject * factory = NULL;
    if (adapter != NULL) {
        adapter->vtbl->GetParent(adapter, &T1_IID_IDXGIFactory, (void **)&factory);
    }
    T1_D3D_RELEASE(adapter);
    if (factory == NULL) { return 0; }

    /*
    The flip model where Windows has it (10, then 8): no copy of each frame
    for the compositor
    */
    static const u32 effects[3] = {
        T1DXGI_SWAP_EFFECT_FLIP_DISCARD,
        T1DXGI_SWAP_EFFECT_FLIP_SEQUENTIAL,
        T1DXGI_SWAP_EFFECT_DISCARD,
    };
    for (u32 e = 0; e < 3 && T1_d3d->swap_chain == NULL; e++) {
        T1DXGISwapChainDesc desc;
        T1_std_memset(&desc, 0, sizeof(T1DXGISwapChainDesc));
        desc.width = w;
        desc.height = h;
        desc.format = T1DXGI_FORMAT_R8G8B8A8_UNORM;
        desc.sample_count = 1;
        desc.buffer_usage = T1DXGI_USAGE_RENDER_TARGET_OUTPUT;
        desc.buffer_count = 2;
        desc.output_window = hwnd;
        desc.windowed = 1;
        desc.swap_effect = effects[e];
        factory->vtbl->CreateSwapChain(
            factory, T1_d3d->device, &desc, &T1_d3d->swap_chain);
    }
    // T1 makes the window fullscreen itself, Alt+Enter would be DXGI's
    if (T1_d3d->swap_chain != NULL) {
        factory->vtbl->Method8(factory, hwnd, T1DXGI_MWA_NO_ALT_ENTER);
    }
    T1_D3D_RELEASE(factory);

    T1_d3d->swap_chain_hwnd = hwnd;
    T1_d3d->back_buffer_w = w;
    T1_d3d->back_buffer_h = h;
    return T1_d3d->swap_chain != NULL;
}

b8 T1_gpu_d3d11_present_tex(void * hwnd, const u32 tex) {
    if (!T1_os_gpu_active() || hwnd == NULL || tex == T1_TEX_NONE) {
        return 0;
    }
    T1D3D11Context * ctx = T1_d3d->ctx;

    // the back buffer is the window's size
    T1WinRect client;
    T1_win32->GetClientRect(hwnd, &client);
    u32 win_w = client.right > 0 ? (u32)client.right : 0;
    u32 win_h = client.bottom > 0 ? (u32)client.bottom : 0;
    if (win_w == 0 || win_h == 0) { return 0; }

    if (T1_d3d->swap_chain != NULL && T1_d3d->swap_chain_hwnd != hwnd) {
        T1_gpu_d3d11_release_swap_chain();
    }
    if (T1_d3d->swap_chain == NULL) {
        if (!T1_gpu_d3d11_make_swap_chain(hwnd, win_w, win_h)) { return 0; }
    } else if (
        T1_d3d->back_buffer_w != win_w ||
        T1_d3d->back_buffer_h != win_h)
    {
        T1_D3D_RELEASE(T1_d3d->back_buffer_rtv);
        ctx->vtbl->OMSetRenderTargets(ctx, 0, NULL, NULL);
        T1_d3d->swap_chain->vtbl->ResizeBuffers(
            T1_d3d->swap_chain, 0, win_w, win_h, T1DXGI_FORMAT_UNKNOWN, 0);
        T1_d3d->back_buffer_w = win_w;
        T1_d3d->back_buffer_h = win_h;
    }
    if (T1_d3d->back_buffer_rtv == NULL) {
        T1D3D11Resource * back_buffer = NULL;
        T1_d3d->swap_chain->vtbl->GetBuffer(
            T1_d3d->swap_chain, 0, &T1_IID_ID3D11Texture2D,
            (void **)&back_buffer);
        if (back_buffer != NULL) {
            T1_d3d->device->vtbl->CreateRenderTargetView(
                T1_d3d->device, back_buffer, NULL, &T1_d3d->back_buffer_rtv);
        }
        T1_D3D_RELEASE(back_buffer);
        if (T1_d3d->back_buffer_rtv == NULL) { return 0; }
    }

    // the T1Tex's slice
    s32 array_i = T1_TEX_ARRAY_I(tex);
    s32 slice_i = T1_TEX_SLICE_I(tex);
    if (
        array_i >= T1_TEXARRAYS_CAP ||
        T1_d3d->texs[array_i] == NULL ||
        (u32)slice_i >= T1_d3d->tex_slices[array_i])
    {
        return 0;
    }
    T1D3D11SRVDesc desc;
    T1_std_memset(&desc, 0, sizeof(T1D3D11SRVDesc));
    desc.format = T1DXGI_FORMAT_R8G8B8A8_UNORM;
    desc.view_dimension = T1D3D11_SRV_DIMENSION_TEXTURE2DARRAY;
    desc.u[0] = 0;
    desc.u[1] = 1;
    desc.u[2] = (u32)slice_i;
    desc.u[3] = 1;
    T1D3D11View * srv = NULL;
    T1_d3d->device->vtbl->CreateShaderResourceView(
        T1_d3d->device, T1_d3d->texs[array_i], &desc, &srv);
    if (srv == NULL) { return 0; }

    // As large as it fits, keeping the aspect ratio, black around it
    u32 tex_w = T1_tex_get_width((T1Tex)tex);
    u32 tex_h = T1_tex_get_height((T1Tex)tex);
    f32 scale_w = (f32)win_w / (f32)tex_w;
    f32 scale_h = (f32)win_h / (f32)tex_h;
    f32 scale = scale_w < scale_h ? scale_w : scale_h;
    T1D3D11Viewport viewport;
    viewport.width = (f32)tex_w * scale;
    viewport.height = (f32)tex_h * scale;
    viewport.top_left_x = ((f32)win_w - viewport.width) * 0.5f;
    viewport.top_left_y = ((f32)win_h - viewport.height) * 0.5f;
    viewport.min_depth = 0.0f;
    viewport.max_depth = 1.0f;

    f32 black[4] = {0.0f, 0.0f, 0.0f, 1.0f};
    ctx->vtbl->ClearRenderTargetView(ctx, T1_d3d->back_buffer_rtv, black);
    ctx->vtbl->OMSetRenderTargets(ctx, 1, &T1_d3d->back_buffer_rtv, NULL);
    ctx->vtbl->RSSetViewports(ctx, 1, &viewport);
    ctx->vtbl->RSSetState(ctx, T1_d3d->raster_states[T1_CULL_NONE]);
    f32 blend_factor[4] = {0.0f, 0.0f, 0.0f, 0.0f};
    ctx->vtbl->OMSetBlendState(
        ctx, T1_d3d->blend_states[T1_BLEND_NONE], blend_factor, 0xFFFFFFFF);
    ctx->vtbl->OMSetDepthStencilState(ctx, T1_d3d->no_depth_state, 0);
    ctx->vtbl->IASetInputLayout(ctx, NULL);
    ctx->vtbl->IASetPrimitiveTopology(ctx, T1D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
    ctx->vtbl->VSSetShader(ctx, T1_d3d->present_vs, NULL, 0);
    ctx->vtbl->PSSetShader(ctx, T1_d3d->present_ps, NULL, 0);
    ctx->vtbl->PSSetShaderResources(ctx, 0, 1, &srv);
    ctx->vtbl->PSSetSamplers(ctx, 0, 1, T1_d3d->samplers);
    ctx->vtbl->Draw(ctx, 3, 0);

    T1_gpu_d3d11_unbind_all();
    T1_D3D_RELEASE(srv);

    // waits for the display's next refresh, which paces the main loop
    s32 presented = T1_d3d->swap_chain->vtbl->Present(T1_d3d->swap_chain, 1, 0);
    return presented >= 0;
}
