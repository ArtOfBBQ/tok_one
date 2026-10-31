#ifndef T1_D3D11_H
#define T1_D3D11_H

#include "T1_win32.h"

/*
What T1 uses of Direct3D 11, DXGI and the HLSL compiler, declared here
instead of including <d3d11.h>: d3d11.dll and d3dcompiler_47.dll are loaded
at runtime (see T1_gpu_d3d11.c). Their objects are COM objects, called
through their vtables, which are declared up to the last method T1 calls
(unused ones are void *, they only hold the place).
*/

typedef struct {
    u32 data1;
    u16 data2;
    u16 data3;
    u8  data4[8];
} T1D3DGUID;

#define T1DXGI_FORMAT_UNKNOWN 0
#define T1DXGI_FORMAT_R8G8B8A8_TYPELESS 27
#define T1DXGI_FORMAT_R8G8B8A8_UNORM 28
#define T1DXGI_FORMAT_R32_TYPELESS 39
#define T1DXGI_FORMAT_D32_FLOAT 40
#define T1DXGI_FORMAT_R32_UINT 42

#define T1D3D11_USAGE_DEFAULT 0
#define T1D3D11_USAGE_DYNAMIC 2
#define T1D3D11_USAGE_STAGING 3
#define T1D3D11_BIND_CONSTANT_BUFFER 0x4u
#define T1D3D11_BIND_SHADER_RESOURCE 0x8u
#define T1D3D11_BIND_RENDER_TARGET 0x20u
#define T1D3D11_BIND_DEPTH_STENCIL 0x40u
#define T1D3D11_BIND_UNORDERED_ACCESS 0x80u
#define T1D3D11_CPU_ACCESS_WRITE 0x10000u
#define T1D3D11_CPU_ACCESS_READ 0x20000u
#define T1D3D11_RESOURCE_MISC_GENERATE_MIPS 0x1u
#define T1D3D11_RESOURCE_MISC_BUFFER_ALLOW_RAW_VIEWS 0x20u

#define T1D3D11_SRV_DIMENSION_TEXTURE2DARRAY 5
#define T1D3D11_SRV_DIMENSION_BUFFEREX 11
#define T1D3D11_BUFFEREX_SRV_FLAG_RAW 0x1u
#define T1D3D11_UAV_DIMENSION_BUFFER 1
#define T1D3D11_UAV_DIMENSION_TEXTURE2DARRAY 5
#define T1D3D11_BUFFER_UAV_FLAG_RAW 0x1u
#define T1D3D11_RTV_DIMENSION_TEXTURE2D 4
#define T1D3D11_RTV_DIMENSION_TEXTURE2DARRAY 5
#define T1D3D11_DSV_DIMENSION_TEXTURE2D 3

#define T1D3D11_MAP_READ 1
#define T1D3D11_MAP_WRITE_DISCARD 4
#define T1D3D11_MAP_FLAG_DO_NOT_WAIT 0x100000u
#define T1DXGI_ERROR_WAS_STILL_DRAWING ((s32)0x887A000Au)

#define T1D3D11_BLEND_ZERO 1
#define T1D3D11_BLEND_ONE 2
#define T1D3D11_BLEND_SRC_ALPHA 5
#define T1D3D11_BLEND_INV_SRC_ALPHA 6
#define T1D3D11_BLEND_OP_ADD 1
#define T1D3D11_COLOR_WRITE_ENABLE_ALL 0xF

#define T1D3D11_DEPTH_WRITE_MASK_ALL 1
#define T1D3D11_COMPARISON_NEVER 1
#define T1D3D11_COMPARISON_LESS_EQUAL 4
#define T1D3D11_COMPARISON_ALWAYS 8
#define T1D3D11_STENCIL_OP_KEEP 1
#define T1D3D11_CLEAR_DEPTH 0x1u

#define T1D3D11_FILL_SOLID 3
#define T1D3D11_CULL_NONE 1
#define T1D3D11_CULL_FRONT 2
#define T1D3D11_CULL_BACK 3

#define T1D3D11_FILTER_MIN_MAG_MIP_POINT 0x0
#define T1D3D11_FILTER_MIN_MAG_LINEAR_MIP_POINT 0x14
#define T1D3D11_TEXTURE_ADDRESS_WRAP 1
#define T1D3D11_TEXTURE_ADDRESS_CLAMP 3

#define T1D3D11_QUERY_EVENT 0
#define T1D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST 4

#define T1D3D_DRIVER_TYPE_HARDWARE 1
#define T1D3D_DRIVER_TYPE_WARP 5
#define T1D3D11_SDK_VERSION 7
#define T1D3D_FEATURE_LEVEL_11_0 0xb000
#define T1D3D_FEATURE_LEVEL_11_1 0xb100

#define T1DXGI_SWAP_EFFECT_DISCARD 0
#define T1DXGI_SWAP_EFFECT_FLIP_SEQUENTIAL 3
#define T1DXGI_SWAP_EFFECT_FLIP_DISCARD 4
#define T1DXGI_USAGE_RENDER_TARGET_OUTPUT 0x20u
#define T1DXGI_MWA_NO_ALT_ENTER 0x2u

#define T1D3DCOMPILE_OPTIMIZATION_LEVEL1 0u

typedef struct {
    u32 byte_width;
    u32 usage;
    u32 bind_flags;
    u32 cpu_access_flags;
    u32 misc_flags;
    u32 structure_byte_stride;
} T1D3D11BufferDesc;

typedef struct {
    const void * sys_mem;
    u32 sys_mem_pitch;
    u32 sys_mem_slice_pitch;
} T1D3D11SubresourceData;

typedef struct {
    u32 width;
    u32 height;
    u32 mip_levels;
    u32 array_size;
    u32 format;
    u32 sample_count;
    u32 sample_quality;
    u32 usage;
    u32 bind_flags;
    u32 cpu_access_flags;
    u32 misc_flags;
} T1D3D11Texture2DDesc;

// view descs: the union after the dimension is u32s, see their use
typedef struct {
    u32 format;
    u32 view_dimension;
    u32 u[4];
} T1D3D11SRVDesc;

typedef struct {
    u32 format;
    u32 view_dimension;
    u32 u[3];
} T1D3D11UAVDesc;

typedef struct {
    u32 format;
    u32 view_dimension;
    u32 u[3];
} T1D3D11RTVDesc;

typedef struct {
    u32 format;
    u32 view_dimension;
    u32 flags;
    u32 u[3];
} T1D3D11DSVDesc;

typedef struct {
    s32 blend_enable;
    u32 src_blend;
    u32 dest_blend;
    u32 blend_op;
    u32 src_blend_alpha;
    u32 dest_blend_alpha;
    u32 blend_op_alpha;
    u8  write_mask;
} T1D3D11RenderTargetBlendDesc;

typedef struct {
    s32 alpha_to_coverage_enable;
    s32 independent_blend_enable;
    T1D3D11RenderTargetBlendDesc render_target[8];
} T1D3D11BlendDesc;

typedef struct {
    u32 fail_op;
    u32 depth_fail_op;
    u32 pass_op;
    u32 func;
} T1D3D11DepthStencilOpDesc;

typedef struct {
    s32 depth_enable;
    u32 depth_write_mask;
    u32 depth_func;
    s32 stencil_enable;
    u8  stencil_read_mask;
    u8  stencil_write_mask;
    T1D3D11DepthStencilOpDesc front_face;
    T1D3D11DepthStencilOpDesc back_face;
} T1D3D11DepthStencilDesc;

typedef struct {
    u32 fill_mode;
    u32 cull_mode;
    s32 front_counter_clockwise;
    s32 depth_bias;
    f32 depth_bias_clamp;
    f32 slope_scaled_depth_bias;
    s32 depth_clip_enable;
    s32 scissor_enable;
    s32 multisample_enable;
    s32 antialiased_line_enable;
} T1D3D11RasterizerDesc;

typedef struct {
    u32 filter;
    u32 address_u;
    u32 address_v;
    u32 address_w;
    f32 mip_lod_bias;
    u32 max_anisotropy;
    u32 comparison_func;
    f32 border_color[4];
    f32 min_lod;
    f32 max_lod;
} T1D3D11SamplerDesc;

typedef struct {
    u32 query;
    u32 misc_flags;
} T1D3D11QueryDesc;

typedef struct {
    f32 top_left_x;
    f32 top_left_y;
    f32 width;
    f32 height;
    f32 min_depth;
    f32 max_depth;
} T1D3D11Viewport;

typedef struct {
    u32 left;
    u32 top;
    u32 front;
    u32 right;
    u32 bottom;
    u32 back;
} T1D3D11Box;

typedef struct {
    void * data;
    u32 row_pitch;
    u32 depth_pitch;
} T1D3D11MappedSubresource;

typedef struct {
    u32 width;
    u32 height;
    u32 refresh_numerator;
    u32 refresh_denominator;
    u32 format;
    u32 scanline_ordering;
    u32 scaling;
    u32 sample_count;
    u32 sample_quality;
    u32 buffer_usage;
    u32 buffer_count;
    T1WinHandle output_window;
    s32 windowed;
    u32 swap_effect;
    u32 flags;
} T1DXGISwapChainDesc;

// Any COM object, for Release()
typedef struct T1ComObject T1ComObject;
typedef struct {
    void * QueryInterface;
    void * AddRef;
    u32 (T1_WINAPI * Release)(T1ComObject *);
} T1ComObjectVtbl;
struct T1ComObject {
    const T1ComObjectVtbl * vtbl;
};

typedef struct T1D3D11Device T1D3D11Device;
typedef struct T1D3D11Context T1D3D11Context;
typedef struct T1DXGISwapChain T1DXGISwapChain;
typedef struct T1DXGIObject T1DXGIObject;
typedef struct T1D3DBlob T1D3DBlob;

// resources, views, shaders and states are only passed around
typedef T1ComObject T1D3D11Resource;
typedef T1ComObject T1D3D11View;
typedef T1ComObject T1D3D11Shader;
typedef T1ComObject T1D3D11State;

typedef struct {
    s32 (T1_WINAPI * QueryInterface)(
        T1D3D11Device *, const T1D3DGUID *, void **);
    void * AddRef;
    u32 (T1_WINAPI * Release)(T1D3D11Device *);
    s32 (T1_WINAPI * CreateBuffer)(
        T1D3D11Device *,
        const T1D3D11BufferDesc *,
        const T1D3D11SubresourceData *,
        T1D3D11Resource **);
    void * CreateTexture1D;
    s32 (T1_WINAPI * CreateTexture2D)(
        T1D3D11Device *,
        const T1D3D11Texture2DDesc *,
        const T1D3D11SubresourceData *,
        T1D3D11Resource **);
    void * CreateTexture3D;
    s32 (T1_WINAPI * CreateShaderResourceView)(
        T1D3D11Device *,
        T1D3D11Resource *,
        const T1D3D11SRVDesc *,
        T1D3D11View **);
    s32 (T1_WINAPI * CreateUnorderedAccessView)(
        T1D3D11Device *,
        T1D3D11Resource *,
        const T1D3D11UAVDesc *,
        T1D3D11View **);
    s32 (T1_WINAPI * CreateRenderTargetView)(
        T1D3D11Device *,
        T1D3D11Resource *,
        const T1D3D11RTVDesc *,
        T1D3D11View **);
    s32 (T1_WINAPI * CreateDepthStencilView)(
        T1D3D11Device *,
        T1D3D11Resource *,
        const T1D3D11DSVDesc *,
        T1D3D11View **);
    void * CreateInputLayout;
    s32 (T1_WINAPI * CreateVertexShader)(
        T1D3D11Device *, const void *, size_t, void *, T1D3D11Shader **);
    void * CreateGeometryShader;
    void * CreateGeometryShaderWithStreamOutput;
    s32 (T1_WINAPI * CreatePixelShader)(
        T1D3D11Device *, const void *, size_t, void *, T1D3D11Shader **);
    void * CreateHullShader;
    void * CreateDomainShader;
    s32 (T1_WINAPI * CreateComputeShader)(
        T1D3D11Device *, const void *, size_t, void *, T1D3D11Shader **);
    void * CreateClassLinkage;
    s32 (T1_WINAPI * CreateBlendState)(
        T1D3D11Device *, const T1D3D11BlendDesc *, T1D3D11State **);
    s32 (T1_WINAPI * CreateDepthStencilState)(
        T1D3D11Device *, const T1D3D11DepthStencilDesc *, T1D3D11State **);
    s32 (T1_WINAPI * CreateRasterizerState)(
        T1D3D11Device *, const T1D3D11RasterizerDesc *, T1D3D11State **);
    s32 (T1_WINAPI * CreateSamplerState)(
        T1D3D11Device *, const T1D3D11SamplerDesc *, T1D3D11State **);
    s32 (T1_WINAPI * CreateQuery)(
        T1D3D11Device *, const T1D3D11QueryDesc *, T1ComObject **);
    void * CreatePredicate;
    void * CreateCounter;
    void * CreateDeferredContext;
    void * OpenSharedResource;
    void * CheckFormatSupport;
    void * CheckMultisampleQualityLevels;
    void * CheckCounterInfo;
    void * CheckCounter;
    void * CheckFeatureSupport;
    void * GetPrivateData;
    void * SetPrivateData;
    void * SetPrivateDataInterface;
    u32 (T1_WINAPI * GetFeatureLevel)(T1D3D11Device *);
    void * GetCreationFlags;
    s32 (T1_WINAPI * GetDeviceRemovedReason)(T1D3D11Device *);
    void (T1_WINAPI * GetImmediateContext)(
        T1D3D11Device *, T1D3D11Context **);
} T1D3D11DeviceVtbl;

struct T1D3D11Device {
    const T1D3D11DeviceVtbl * vtbl;
};

typedef struct {
    void * QueryInterface;
    void * AddRef;
    u32 (T1_WINAPI * Release)(T1D3D11Context *);
    void * GetDevice;
    void * GetPrivateData;
    void * SetPrivateData;
    void * SetPrivateDataInterface;
    void (T1_WINAPI * VSSetConstantBuffers)(
        T1D3D11Context *, u32, u32, T1D3D11Resource * const *);
    void (T1_WINAPI * PSSetShaderResources)(
        T1D3D11Context *, u32, u32, T1D3D11View * const *);
    void (T1_WINAPI * PSSetShader)(
        T1D3D11Context *, T1D3D11Shader *, void * const *, u32);
    void (T1_WINAPI * PSSetSamplers)(
        T1D3D11Context *, u32, u32, T1D3D11State * const *);
    void (T1_WINAPI * VSSetShader)(
        T1D3D11Context *, T1D3D11Shader *, void * const *, u32);
    void * DrawIndexed;
    void (T1_WINAPI * Draw)(T1D3D11Context *, u32 count, u32 start);
    s32 (T1_WINAPI * Map)(
        T1D3D11Context *,
        T1D3D11Resource *,
        u32 subresource,
        u32 map_type,
        u32 map_flags,
        T1D3D11MappedSubresource *);
    void (T1_WINAPI * Unmap)(T1D3D11Context *, T1D3D11Resource *, u32);
    void (T1_WINAPI * PSSetConstantBuffers)(
        T1D3D11Context *, u32, u32, T1D3D11Resource * const *);
    void (T1_WINAPI * IASetInputLayout)(T1D3D11Context *, T1ComObject *);
    void * IASetVertexBuffers;
    void * IASetIndexBuffer;
    void * DrawIndexedInstanced;
    void * DrawInstanced;
    void * GSSetConstantBuffers;
    void * GSSetShader;
    void (T1_WINAPI * IASetPrimitiveTopology)(T1D3D11Context *, u32);
    void (T1_WINAPI * VSSetShaderResources)(
        T1D3D11Context *, u32, u32, T1D3D11View * const *);
    void (T1_WINAPI * VSSetSamplers)(
        T1D3D11Context *, u32, u32, T1D3D11State * const *);
    void * Begin;
    void (T1_WINAPI * End)(T1D3D11Context *, T1ComObject *);
    s32 (T1_WINAPI * GetData)(
        T1D3D11Context *, T1ComObject *, void *, u32, u32);
    void * SetPredication;
    void * GSSetShaderResources;
    void * GSSetSamplers;
    void (T1_WINAPI * OMSetRenderTargets)(
        T1D3D11Context *, u32, T1D3D11View * const *, T1D3D11View *);
    void * OMSetRenderTargetsAndUnorderedAccessViews;
    void (T1_WINAPI * OMSetBlendState)(
        T1D3D11Context *, T1D3D11State *, const f32 *, u32);
    void (T1_WINAPI * OMSetDepthStencilState)(
        T1D3D11Context *, T1D3D11State *, u32);
    void * SOSetTargets;
    void * DrawAuto;
    void * DrawIndexedInstancedIndirect;
    void * DrawInstancedIndirect;
    void (T1_WINAPI * Dispatch)(T1D3D11Context *, u32, u32, u32);
    void * DispatchIndirect;
    void (T1_WINAPI * RSSetState)(T1D3D11Context *, T1D3D11State *);
    void (T1_WINAPI * RSSetViewports)(
        T1D3D11Context *, u32, const T1D3D11Viewport *);
    void * RSSetScissorRects;
    void (T1_WINAPI * CopySubresourceRegion)(
        T1D3D11Context *,
        T1D3D11Resource * dst,
        u32 dst_subresource,
        u32 dst_x,
        u32 dst_y,
        u32 dst_z,
        T1D3D11Resource * src,
        u32 src_subresource,
        const T1D3D11Box * src_box);
    void (T1_WINAPI * CopyResource)(
        T1D3D11Context *, T1D3D11Resource *, T1D3D11Resource *);
    void (T1_WINAPI * UpdateSubresource)(
        T1D3D11Context *,
        T1D3D11Resource *,
        u32 subresource,
        const T1D3D11Box *,
        const void * data,
        u32 row_pitch,
        u32 depth_pitch);
    void * CopyStructureCount;
    void (T1_WINAPI * ClearRenderTargetView)(
        T1D3D11Context *, T1D3D11View *, const f32 *);
    void * ClearUnorderedAccessViewUint;
    void * ClearUnorderedAccessViewFloat;
    void (T1_WINAPI * ClearDepthStencilView)(
        T1D3D11Context *, T1D3D11View *, u32, f32, u8);
    void (T1_WINAPI * GenerateMips)(T1D3D11Context *, T1D3D11View *);
    void * SetResourceMinLOD;
    void * GetResourceMinLOD;
    void * ResolveSubresource;
    void * ExecuteCommandList;
    void * HSSetShaderResources;
    void * HSSetShader;
    void * HSSetSamplers;
    void * HSSetConstantBuffers;
    void * DSSetShaderResources;
    void * DSSetShader;
    void * DSSetSamplers;
    void * DSSetConstantBuffers;
    void (T1_WINAPI * CSSetShaderResources)(
        T1D3D11Context *, u32, u32, T1D3D11View * const *);
    void (T1_WINAPI * CSSetUnorderedAccessViews)(
        T1D3D11Context *, u32, u32, T1D3D11View * const *, const u32 *);
    void (T1_WINAPI * CSSetShader)(
        T1D3D11Context *, T1D3D11Shader *, void * const *, u32);
    void (T1_WINAPI * CSSetSamplers)(
        T1D3D11Context *, u32, u32, T1D3D11State * const *);
    void (T1_WINAPI * CSSetConstantBuffers)(
        T1D3D11Context *, u32, u32, T1D3D11Resource * const *);
} T1D3D11ContextVtbl;

struct T1D3D11Context {
    const T1D3D11ContextVtbl * vtbl;
};

// IDXGIObject and the interfaces on it T1 uses: device, adapter, factory
typedef struct {
    s32 (T1_WINAPI * QueryInterface)(
        T1DXGIObject *, const T1D3DGUID *, void **);
    void * AddRef;
    u32 (T1_WINAPI * Release)(T1DXGIObject *);
    void * SetPrivateData;
    void * SetPrivateDataInterface;
    void * GetPrivateData;
    s32 (T1_WINAPI * GetParent)(
        T1DXGIObject *, const T1D3DGUID *, void **);
    // IDXGIDevice: GetAdapter, IDXGIAdapter: EnumOutputs,
    // IDXGIFactory: EnumAdapters
    s32 (T1_WINAPI * Method7)(T1DXGIObject *, void *);
    // IDXGIAdapter: GetDesc, IDXGIFactory: MakeWindowAssociation
    s32 (T1_WINAPI * Method8)(T1DXGIObject *, void *, u32);
    void * Method9;
    // IDXGIFactory: CreateSwapChain
    s32 (T1_WINAPI * CreateSwapChain)(
        T1DXGIObject *,
        void * device,
        T1DXGISwapChainDesc *,
        T1DXGISwapChain **);
} T1DXGIObjectVtbl;

struct T1DXGIObject {
    const T1DXGIObjectVtbl * vtbl;
};

typedef struct {
    void * QueryInterface;
    void * AddRef;
    u32 (T1_WINAPI * Release)(T1DXGISwapChain *);
    void * SetPrivateData;
    void * SetPrivateDataInterface;
    void * GetPrivateData;
    void * GetParent;
    void * GetDevice;
    s32 (T1_WINAPI * Present)(T1DXGISwapChain *, u32 sync_interval, u32);
    s32 (T1_WINAPI * GetBuffer)(
        T1DXGISwapChain *, u32, const T1D3DGUID *, void **);
    void * SetFullscreenState;
    void * GetFullscreenState;
    void * GetDesc;
    s32 (T1_WINAPI * ResizeBuffers)(
        T1DXGISwapChain *, u32 count, u32 w, u32 h, u32 format, u32 flags);
} T1DXGISwapChainVtbl;

struct T1DXGISwapChain {
    const T1DXGISwapChainVtbl * vtbl;
};

typedef struct {
    void * QueryInterface;
    void * AddRef;
    u32 (T1_WINAPI * Release)(T1D3DBlob *);
    void * (T1_WINAPI * GetBufferPointer)(T1D3DBlob *);
    size_t (T1_WINAPI * GetBufferSize)(T1D3DBlob *);
} T1D3DBlobVtbl;

struct T1D3DBlob {
    const T1D3DBlobVtbl * vtbl;
};

typedef s32 (T1_WINAPI * T1D3D11CreateDeviceFn)(
    void * adapter,
    u32 driver_type,
    T1WinHandle software,
    u32 flags,
    const u32 * feature_levels,
    u32 feature_levels_size,
    u32 sdk_version,
    T1D3D11Device ** device,
    u32 * feature_level,
    T1D3D11Context ** context);

typedef s32 (T1_WINAPI * T1D3DCompileFn)(
    const void * source,
    size_t source_size,
    const char * source_name,
    const void * defines,
    void * include,
    const char * entry_point,
    const char * target,
    u32 flags1,
    u32 flags2,
    T1D3DBlob ** code,
    T1D3DBlob ** errors);

#endif // T1_D3D11_H
