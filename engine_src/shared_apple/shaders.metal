#include <metal_stdlib>
#include <simd/simd.h>

#include "../shared/cpu_gpu_shared_types.h"

using namespace metal;

typedef struct
{
    float4 position [[position]];
    float4 color;
    float2 texture_coordinate;
    float4 lighting;
    int texturearray_i;
    int texture_i;
} RasterizerPixel;

float4 x_rotate(float4 vertices, float x_angle) {
    float4 rotated_vertices = vertices;
    float cos_angle = cos(x_angle);
    float sin_angle = sin(x_angle);
    
    rotated_vertices[1] =
        vertices[1] * cos_angle -
        vertices[2] * sin_angle;
    rotated_vertices[2] =
        vertices[1] * sin_angle +
        vertices[2] * cos_angle;
    
    return rotated_vertices;
}

float4 y_rotate(float4 vertices, float y_angle) {
    float4 rotated_vertices = vertices;
    float cos_angle = cos(y_angle);
    float sin_angle = sin(y_angle);
    
    rotated_vertices[0] =
        vertices[0] * cos_angle +
        vertices[2] * sin_angle;
    rotated_vertices[2] =
        vertices[2] * cos_angle -
        vertices[0] * sin_angle;
    
    return rotated_vertices;
}

float4 z_rotate(float4 vertices, float z_angle) {
    float4 rotated_vertices = vertices;
    float cos_angle = cos(z_angle);
    float sin_angle = sin(z_angle);
    
    rotated_vertices[0] =
        vertices[0] * cos_angle -
        vertices[1] * sin_angle;
    rotated_vertices[1] =
        vertices[1] * cos_angle +
        vertices[0] * sin_angle;
    
    return rotated_vertices;
}

float get_distance(
    float4 a,
    float4 b)
{    
    float4 squared_diffs = (a-b)*(a-b);
    
    float sum_squares = dot(squared_diffs, float4(1.0f,1.0f,1.0f,1.0f));
    
    return sqrt(sum_squares);
}

vertex RasterizerPixel
vertex_shader(
    uint vertex_i [[ vertex_id ]],
    const device GPU_Vertex * input_array [[ buffer(0) ]],
    const device GPU_LightCollection * light_collection [[ buffer(1) ]],
    const device GPU_Camera * camera [[ buffer(2) ]],
    const device GPU_ProjectionConstants * projection_constants [[ buffer(3) ]])
{
    RasterizerPixel out;
    
    float4 parent_mesh_position = vector_float4(
        input_array[vertex_i].parent_x,
        input_array[vertex_i].parent_y,
        input_array[vertex_i].parent_z,
        0.0f);
    
    float4 mesh_vertices = vector_float4(
        input_array[vertex_i].x,
        input_array[vertex_i].y,
        input_array[vertex_i].z,
        1.0f);
    
    mesh_vertices *= input_array[vertex_i].scale_factor;
    mesh_vertices[3] = 1.0f;
    
    float4 mesh_normals = vector_float4(
        input_array[vertex_i].normal_x,
        input_array[vertex_i].normal_y,
        input_array[vertex_i].normal_z,
        1.0f);
    
    float4 camera_position = vector_float4(
        camera->x,
        camera->y,
        camera->z,
        0.0f);
    
    // rotate vertices
    float4 x_rotated_vertices = x_rotate(
        mesh_vertices,
        input_array[vertex_i].x_angle);
    float4 x_rotated_normals  = x_rotate(
        mesh_normals,
        input_array[vertex_i].x_angle);
    float4 y_rotated_vertices = y_rotate(
        x_rotated_vertices,
        input_array[vertex_i].y_angle);
    float4 y_rotated_normals  = y_rotate(
        x_rotated_normals,
        input_array[vertex_i].y_angle);
    float4 z_rotated_vertices = z_rotate(
        y_rotated_vertices,
        input_array[vertex_i].z_angle);
    float4 z_rotated_normals  = z_rotate(
        y_rotated_normals,
        input_array[vertex_i].z_angle);
    
    // translate to world position
    float4 translated_pos = z_rotated_vertices + parent_mesh_position;
    
    if (input_array[vertex_i].ignore_camera < 1.0f) {
        float4 camera_translated_pos = translated_pos - camera_position;
        
        // rotate around camera
        float4 cam_x_rotated = x_rotate(
            camera_translated_pos,
            -camera->x_angle);
        float4 cam_y_rotated = y_rotate(
            cam_x_rotated,
            -camera->y_angle);
        float4 cam_z_rotated = z_rotate(
            cam_y_rotated,
            -camera->z_angle);
        
        out.position = cam_z_rotated;
    } else {
        out.position = translated_pos;
    }
    
    // projection
    out.position[0] *= projection_constants->x_multiplier;
    out.position[1] *= projection_constants->field_of_view_modifier;
    out.position[3]  = out.position[2];
    out.position[2]  =     
        (out.position[2] * projection_constants->q) -
        (projection_constants->near * projection_constants->q);
    
    out.color = vector_float4(
        input_array[vertex_i].RGBA[0],
        input_array[vertex_i].RGBA[1],
        input_array[vertex_i].RGBA[2],
        input_array[vertex_i].RGBA[3]);
    clamp(out.color, 0.25f, 1.0f);
    
    //    out.surface_normal = vector_float4(
    //        input_array[vertex_i].normal_x,
    //        input_array[vertex_i].normal_y,
    //        input_array[vertex_i].normal_z,
    //        1.0f);
    
    out.lighting = float4(0.0f, 0.0f, 0.0f, 1.0f);
    
    out.texturearray_i = input_array[vertex_i].texturearray_i;
    out.texture_i = input_array[vertex_i].texture_i;
    out.texture_coordinate = vector_float2(
        input_array[vertex_i].uv[0],
        input_array[vertex_i].uv[1]);
    
    if (input_array[vertex_i].ignore_lighting > 0.0f) {
        out.lighting += 1.0f;
        out.lighting[3] = 1.0f;
        return out;
    }
    
    for (
        uint32_t i = 0;
        i < light_collection->lights_size;
        i++)
    {
        // ambient lighting
        float4 light_pos = vector_float4(
            light_collection->light_x[i],
            light_collection->light_y[i],
            light_collection->light_z[i],
            1.0f);
        float4 light_color = vector_float4(
            light_collection->red[i],
            light_collection->green[i],
            light_collection->blue[i],
            1.0f);
        float distance = get_distance(
            light_pos,
            translated_pos);
        float distance_mod = (light_collection->reach[i] + 0.5f)
            - (distance * distance);
        distance_mod = clamp(distance_mod, 0.0f, 10.0f);
        
        out.lighting += (
            distance_mod *
            light_color *
            light_collection->ambient[i]);
        
        // diffuse lighting
        normalize(z_rotated_normals);
        
        float4 vec_from_light_to_vertex = normalize(translated_pos - light_pos);
        float visibility_rating = max(0.0f, -1.0f * dot(z_rotated_normals, vec_from_light_to_vertex));
        
        out.lighting += (
            distance_mod *
            light_color *
            (light_collection->diffuse[i] * 3.0f) *
            visibility_rating);
    }
    
    clamp(out.lighting, 0.05f, 1.0f);
    out.lighting[3] = 1.0f;
    
    return out;
}

fragment float4
fragment_shader(
    RasterizerPixel in [[stage_in]],
    array<texture2d_array<half>, TEXTUREARRAYS_SIZE>
        color_textures[[ texture(0) ]])
{
    float4 out_color = in.color;
    
    if (
        in.texturearray_i < 0 ||
        in.texture_i < 0)
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
    
    int diamond_size = 35.0f;
    int neghalfdiamond = -1.0f * (diamond_size / 2);
        
    int alpha_tresh = (int)(out_color[3] * diamond_size);
    
    if (
        out_color[3] < 0.01f ||
        (
            out_color[3] < 0.99f &&
            (
                abs((neghalfdiamond + (int)in.position.x % diamond_size)) +
                abs((neghalfdiamond + (int)in.position.y % diamond_size))
            ) > alpha_tresh))
    {
        discard_fragment();
    }
    
    out_color[3] = 1.0f;
    return out_color;
}
