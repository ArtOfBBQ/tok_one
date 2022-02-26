#include <metal_stdlib>
#include <simd/simd.h>

#include "../shared/vertex_types.h"

using namespace metal;

typedef struct
{
    float4 position [[position]];
    float4 color;
} RasterizerPixel;

typedef struct
{
    float4 position [[position]];
    float2 texture_coordinate;
} RasterizerTexturedPixel;

vertex RasterizerPixel
vertex_shader(
    uint vertexID [[ vertex_id ]],
    constant ColoredVertex * input_array [[ buffer(0) ]])
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
    
    return out;
}

vertex RasterizerTexturedPixel
texture_vertex_shader(
    uint vertexID [[ vertex_id ]],
    constant TexturedVertex * input_array [[ buffer(0) ]])
{
    RasterizerTexturedPixel out;
    
    out.position =
        vector_float4(
            input_array[vertexID].x,
            input_array[vertexID].y,
            0.0,
            1.0);
    
    out.texture_coordinate =
        vector_float2(
            input_array[vertexID].texture_coordinates[0],
            input_array[vertexID].texture_coordinates[1]);
    
    return out;
}

fragment float4
fragment_shader(
    RasterizerPixel in [[stage_in]])
{
    // Return the interpolated color.
    return in.color;
}

fragment float4
texture_fragment_shader(
    RasterizerTexturedPixel in [[stage_in]],
    texture2d<half> color_texture [[]])
{
    return vector_float4(0.0f, 0.0f, 0.5f, 1.0f);
    
    // constexpr sampler textureSampler(
    //     mag_filter::linear,
    //     min_filter::linear);
    
    // Sample the texture to obtain a color
    // const half4 colorSample = color_texture.sample(
    //     textureSampler,
    //     in.texture_coordinate);
    
    // return the color of the texture
    // return float4(colorSample);
}

