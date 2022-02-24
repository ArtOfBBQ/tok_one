#include <metal_stdlib>
#include <simd/simd.h>

using namespace metal;

typedef struct
{
    float4 position [[position]];
    float4 color;
} RasterizerData;

typedef struct MTLColoredVertex {
    float x;
    float y;
    float RGBA[4];
} MTLColoredVertex;

vertex RasterizerData
vertexShader(
    uint vertexID [[ vertex_id ]],
    constant MTLColoredVertex * input_array [[ buffer(0) ]])
{
    RasterizerData out;
    
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

// Fragment function
fragment float4 fragmentShader(RasterizerData in [[stage_in]])
{
    // Return the interpolated color.
    return in.color;
}

