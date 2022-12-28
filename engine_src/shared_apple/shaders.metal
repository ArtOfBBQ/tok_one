#include <metal_stdlib>
#include <simd/simd.h>

#include "../shared/vertex_types.h"

using namespace metal;

typedef struct
{
    float4 position [[position]];
    float4 surface_normal;
    float4 color;
    float2 texture_coordinate;
    float4 lighting;
    uint8_t texturearray_i;
    uint8_t texture_i;
} RasterizerPixel;

//typedef struct
//{
//   float4 position;
//   float4 ambient_lighting;
//   float4 diffuse_lighting;
//   float reach;
//} FragmentShaderLight;

float get_distance(
    float3 a,
    float3 b)
{    
    float3 squared_diffs = (a-b)*(a-b);
    
    float sum_squares = dot(squared_diffs, float3(1.0f,1.0f,1.0f));
    
    return sqrt(sum_squares);
}

vertex RasterizerPixel
vertex_shader(
    uint vertexID [[ vertex_id ]],
    const device Vertex * input_array [[ buffer(0) ]],
    const device LightCollection * light_collection [[ buffer(1) ]])
{
    RasterizerPixel out;
    
    float3 orig_position = vector_float3(
        input_array[vertexID].orig_x,
        input_array[vertexID].orig_y,
        input_array[vertexID].orig_z);
    float3 in_position = vector_float3(
        input_array[vertexID].x,
        input_array[vertexID].y,
        input_array[vertexID].z);
    
    out.color = vector_float4(
        input_array[vertexID].RGBA[0],
        input_array[vertexID].RGBA[1],
        input_array[vertexID].RGBA[2],
        input_array[vertexID].RGBA[3]);
    
    out.surface_normal = vector_float4(
        input_array[vertexID].normal_x,
        input_array[vertexID].normal_y,
        input_array[vertexID].normal_z,
        1.0f);
    
    out.lighting = vector_float4(0.0f, 0.0f, 0.0f, 1.0f); 
    
    for (
        uint32_t i = 0;
        i < light_collection->lights_size;
        i++)
    {
        // ambient lighting
        float3 light_pos = vector_float3(
            light_collection->light_x[i],
            light_collection->light_y[i],
            light_collection->light_z[i]);
        float3 light_untranslated_pos = vector_float3(
            light_collection->orig_x[i],
            light_collection->orig_y[i],
            light_collection->orig_z[i]);
        float4 light_color = vector_float4(
            light_collection->red[i],
            light_collection->green[i],
            light_collection->blue[i],
            1.0f); 
        float distance = get_distance(
            light_pos,
            in_position);
        float distance_mod = light_collection->reach[i] - distance;
        distance_mod = clamp(distance_mod, 0.0f, 10.0f);
        
        out.lighting += (
            distance_mod *
            light_color *
            light_collection->ambient[i]);
        
        distance_mod = light_collection->reach[i] - distance;
        
        // diffuse lighting
        float3 surface_normal = vector_float3(
            input_array[vertexID].normal_x,
            input_array[vertexID].normal_y,
            input_array[vertexID].normal_z);
        normalize(surface_normal);
        
        float3 vec_from_light_to_vertex = normalize(orig_position - light_untranslated_pos);
        float visibility_rating = max(0.0f, dot(surface_normal, vec_from_light_to_vertex));
        
        out.lighting += (
            distance_mod *
            light_color *
            (light_collection->diffuse[i] * 3.0f) *
            visibility_rating);
    }
    
    if (input_array[vertexID].texturearray_i < 0)
    {
        out.texture_coordinate = vector_float2(
            -1.0f, -1.0f);
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
    
    out.position = vector_float4(in_position[0], in_position[1], in_position[2], input_array[vertexID].w);
    out.lighting = clamp(out.lighting, 0.0f, 5.0f);
    out.lighting[3] = 1.0f;
    
    return out;
}

fragment float4
fragment_shader(
    RasterizerPixel in [[stage_in]],
    array<texture2d_array<half>, TEXTUREARRAYS_SIZE> color_textures [[texture(0)]])
{
    float4 out_color = in.color;
    
    if (
        in.texture_coordinate[0] < 0.0f ||
        in.texture_coordinate[1] < 0.0f)
    {
        out_color *= in.lighting;
    } else {
    
        constexpr sampler textureSampler(
                                         mag_filter::nearest,
                                         min_filter::nearest);
        
        // Sample the texture to obtain a color
        const half4 color_sample =
        color_textures[in.texturearray_i].sample(
                                                 textureSampler,
                                                 in.texture_coordinate, 
                                                 in.texture_i);
        float4 texture_sample = float4(color_sample);
        
        out_color *= texture_sample * in.lighting;
    }
    // This is reportedly a terrible way to do this, but I'm
    // doing most of my rendering on CPU and using like 5% of gpu
    // resources, and I simply don't know a better way yet
    if (out_color[3] < 0.05f) { discard_fragment(); }
    
    return out_color;
}
