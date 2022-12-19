#include <metal_stdlib>
#include <simd/simd.h>

#include "../shared/vertex_types.h"

using namespace metal;

typedef struct
{
    float4 position [[position]];
    float4 color;
    float2 texture_coordinate;
    float4 lighting;
    uint8_t texturearray_i;
    uint8_t texture_i;
} RasterizerPixel;

vertex RasterizerPixel
vertex_shader(
    uint vertexID [[ vertex_id ]],
    constant Vertex * input_array [[ buffer(0) ]])
{
    RasterizerPixel out;
    
    out.position = vector_float4(
        input_array[vertexID].x,
        input_array[vertexID].y,
        input_array[vertexID].z,
        input_array[vertexID].w);
    
    out.color = vector_float4(
        input_array[vertexID].RGBA[0],
        input_array[vertexID].RGBA[1],
        input_array[vertexID].RGBA[2],
        input_array[vertexID].RGBA[3]);
    
    out.lighting = vector_float4(
        input_array[vertexID].lighting[0],
        input_array[vertexID].lighting[1],
        input_array[vertexID].lighting[2],
        input_array[vertexID].lighting[3]);
    
    if (input_array[vertexID].texturearray_i < 0)
    {
        out.texture_coordinate =
            vector_float2(-1.0f, -1.0f);
    } else {
        out.texturearray_i =
            input_array[vertexID].texturearray_i;
        out.texture_i =
            input_array[vertexID].texture_i;
        out.texture_coordinate =
            vector_float2(
                input_array[vertexID].uv[0],
                input_array[vertexID].uv[1]);
    }
    
    return out;
}

fragment float4
fragment_shader(
    RasterizerPixel in [[stage_in]],
    array<texture2d_array<half>, TEXTUREARRAYS_SIZE> color_textures [[texture(0)]])
{    
    if (
        in.texture_coordinate[0] < 0.0f
        || in.texture_coordinate[1] < 0.0f)
    {
        return in.color * in.lighting;
    }
    
    constexpr sampler textureSampler(
        mag_filter::nearest,
        min_filter::nearest);
    
    // Sample the texture to obtain a color
    const half4 colorSample =
        color_textures[in.texturearray_i].sample(
            textureSampler,
            in.texture_coordinate, 
            in.texture_i);
    float4 texture_sample = float4(colorSample);
    
    float4 out_color = texture_sample * in.color * in.lighting;
    
    // This is reportedly a terrible way to do this, but I'm
    // doing most of my rendering on CPU and using like 5% of gpu
    // resources, and I simply don't know a better way yet
    if (out_color[3] < 0.05f) { discard_fragment(); }
    
    return out_color;
}
