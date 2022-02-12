#include <metal_stdlib>
#include <simd/simd.h>

using namespace metal;

typedef struct
{
    float4 position [[position]];
    float4 color;

} RasterizerData;

struct metal_vertex
{
    float4 position;
    float4 color;
};

vertex RasterizerData
vertexShader(
    uint vertexID [[ vertex_id ]],
    constant metal_vertex * input_array [[ buffer(0) ]])
{
    RasterizerData out;
    
    // To convert from positions in pixel space to positions in
    // clip-space, divide the pixel coordinates by half the size
    // of the viewport.
    // Z is set to 0.0 and w to 1.0 because this is 2D sample.
    out.position =
        vector_float4(
            input_array[vertexID].position[0],
            input_array[vertexID].position[1],
            0.0,
            1.0);
    
    out.color = input_array[vertexID].color;
    
    return out;
}

// Fragment function
fragment float4 fragmentShader(RasterizerData in [[stage_in]])
{
    // Return the interpolated color.
    return in.color;
}

