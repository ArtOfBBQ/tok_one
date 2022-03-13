#include <metal_stdlib>
#include <simd/simd.h>

#include "../shared/vertex_types.h"

using namespace metal;

typedef struct
{
    float4 position [[position]];
    float4 color;
    float2 texture_coordinate;
    float lighting;
    uint32_t texture_i;
} RasterizerPixel;

vertex RasterizerPixel
vertex_shader(
    uint vertexID [[ vertex_id ]],
    constant Vertex * input_array [[ buffer(0) ]])
{
    RasterizerPixel out;
    
    out.position =
        vector_float4(
            input_array[vertexID].x,
            input_array[vertexID].y,
            0.0,
            1.0);
    
    out.color =
        vector_float4(
            input_array[vertexID].RGBA[0],
            input_array[vertexID].RGBA[1],
            input_array[vertexID].RGBA[2],
            input_array[vertexID].RGBA[3]);

    out.lighting = input_array[vertexID].lighting;
    
    if (input_array[vertexID].texture_i < 0)
    {
        out.texture_coordinate =
            vector_float2(-1.0f, -1.0f);
    } else
    {
        out.texture_i = input_array[vertexID].texture_i;
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
    array<texture2d<half>, TEXTURES_SIZE> color_textures [[texture(0)]])
{
    if (
        in.texture_coordinate[0] < 0.0f
        || in.texture_coordinate[1] < 0.0f)
    {
        return in.color * in.lighting;
    }
    
    constexpr sampler textureSampler(
        mag_filter::linear,
        min_filter::linear);
    
    // Sample the texture to obtain a color
    const half4 colorSample = color_textures[in.texture_i].sample(
        textureSampler,
        in.texture_coordinate);
    
    // return the color of the texture
    return float4(colorSample) * (0.2 + in.lighting);
}

