#include <metal_stdlib>
#include <simd/simd.h>

#include "../shared/cpu_gpu_shared_types.h"

using namespace metal;

float3 x_rotate(float3 vertices, float x_angle) {
    float3 rotated_vertices = vertices;
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

float3 y_rotate(float3 vertices, float y_angle) {
    float3 rotated_vertices = vertices;
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

float3 z_rotate(float3 vertices, float z_angle) {
    float3 rotated_vertices = vertices;
    float cos_angle = cos(z_angle);
    float sin_angle = sin(z_angle);
    
    rotated_vertices[0] =
        (vertices[0] * cos_angle) -
        (vertices[1] * sin_angle);
    rotated_vertices[1] =
        (vertices[1] * cos_angle) +
        (vertices[0] * sin_angle);
    
    return rotated_vertices;
}

typedef struct {
    float4 position [[position]];
    float point_size [[point_size]];
    float color;
} RawFragment;

vertex RawFragment
raw_vertex_shader(
    uint vertex_i [[ vertex_id ]],
    const device GPURawVertex * vertices [[ buffer(0) ]],
    const device GPUCamera * camera [[ buffer(3) ]],
    const device GPUProjectionConstants * projection_constants [[ buffer(5) ]])
{
    RawFragment out;
    
    out.point_size = 20.0;
    
    float3 pos = vector_float3(
        vertices[vertex_i].xyz[0],
        vertices[vertex_i].xyz[1],
        vertices[vertex_i].xyz[2]);
    
    float3 camera_position = vector_float3(
        camera->xyz[0],
        camera->xyz[1], camera->xyz[2]);
    float3 camera_translated_pos = pos - camera_position;
    
    // rotate around camera
    float3 cam_x_rotated = x_rotate(
        camera_translated_pos,
        -camera->xyz_angle[0]);
    float3 cam_y_rotated = y_rotate(
        cam_x_rotated,
        -camera->xyz_angle[1]);
    float3 cam_z_rotated = z_rotate(
        cam_y_rotated,
        -camera->xyz_angle[2]);
    
    out.position = vector_float4(cam_z_rotated, 1.0f);
    
    // projection
    out.position[0] *= projection_constants->x_multiplier;
    out.position[1] *= projection_constants->field_of_view_modifier;
    out.position[3]  = out.position[2];
    out.position[2]  =
        (out.position[2] * projection_constants->q) -
        (projection_constants->znear * projection_constants->q);
    
    out.color = vertices[vertex_i].color;
    
    return out;
}

fragment float4
raw_fragment_shader(RawFragment in [[stage_in]])
{
    float4 out_color = vector_float4(in.color, in.color / 3.0f, 1.0f, 1.0f);
    
    return out_color;
}

typedef struct
{
    float4 position [[position]];
    float4 color;
    float2 texture_coordinate;
    float3 lighting;
    int texturearray_i;
    int texture_i;
    float point_size [[point_size]];
} RasterizerPixel;

float get_distance(
    float3 a,
    float3 b)
{
    float3 squared_diffs = (a-b)*(a-b);
    
    float sum_squares = dot(
        squared_diffs,
        vector_float3(1.0f,1.0f,1.0f));
    
    return sqrt(sum_squares);
}

vertex RasterizerPixel
vertex_shader(
    uint vertex_i [[ vertex_id ]],
    const device GPUVertex * vertices [[ buffer(0) ]],
    const device GPUPolygonCollection * polygon_collection [[ buffer(1) ]],
    const device GPULightCollection * light_collection [[ buffer(2) ]],
    const device GPUCamera * camera [[ buffer(3) ]],
    const device GPULockedVertex * locked_vertices [[ buffer(4) ]],
    const device GPUProjectionConstants * projection_constants [[ buffer(5) ]],
    const device GPUPolygonMaterial * polygon_materials [[ buffer(6) ]])
{
    RasterizerPixel out;
    
    uint polygon_i = vertices[vertex_i].polygon_i;
    uint locked_vertex_i = vertices[vertex_i].locked_vertex_i;
    uint locked_material_i = (polygon_i * MAX_MATERIALS_PER_POLYGON) +
        locked_vertices[locked_vertex_i].parent_material_i;
    
    float3 parent_mesh_position = vector_float3(
        polygon_collection->polygons[polygon_i].xyz[0],
        polygon_collection->polygons[polygon_i].xyz[1],
        polygon_collection->polygons[polygon_i].xyz[2]);
    
    float3 mesh_vertices = vector_float3(
        locked_vertices[locked_vertex_i].xyz[0],
        locked_vertices[locked_vertex_i].xyz[1],
        locked_vertices[locked_vertex_i].xyz[2]);
    
    float3 vertex_multipliers = vector_float3(
        polygon_collection->polygons[polygon_i].xyz_multiplier[0],
        polygon_collection->polygons[polygon_i].xyz_multiplier[1],
        polygon_collection->polygons[polygon_i].xyz_multiplier[2]);
    
    float3 vertex_offsets = vector_float3(
        polygon_collection->polygons[polygon_i].xyz_offset[0],
        polygon_collection->polygons[polygon_i].xyz_offset[1],
        polygon_collection->polygons[polygon_i].xyz_offset[2]);
    
    mesh_vertices *= vertex_multipliers;
    mesh_vertices += vertex_offsets;
    
    mesh_vertices *= polygon_collection->polygons[polygon_i].scale_factor;
    
    float3 mesh_normals = vector_float3(
        locked_vertices[locked_vertex_i].normal_xyz[0],
        locked_vertices[locked_vertex_i].normal_xyz[1],
        locked_vertices[locked_vertex_i].normal_xyz[2]);
    
    // rotate vertices
    float3 x_rotated_vertices = x_rotate(
        mesh_vertices,
        polygon_collection->polygons[polygon_i].xyz_angle[0]);
    float3 y_rotated_vertices = y_rotate(
        x_rotated_vertices,
        polygon_collection->polygons[polygon_i].xyz_angle[1]);
    float3 z_rotated_vertices = z_rotate(
        y_rotated_vertices,
        polygon_collection->polygons[polygon_i].xyz_angle[2]);
    
    float3 x_rotated_normals  = x_rotate(
        mesh_normals,
        polygon_collection->polygons[polygon_i].xyz_angle[0]);
    float3 y_rotated_normals  = y_rotate(
        x_rotated_normals,
        polygon_collection->polygons[polygon_i].xyz_angle[1]);
    float3 z_rotated_normals  = z_rotate(
        y_rotated_normals,
        polygon_collection->polygons[polygon_i].xyz_angle[2]);
    
    // translate to world position
    float3 rotated_pos = z_rotated_vertices + parent_mesh_position;
    
    float3 camera_position = vector_float3(
        camera->xyz[0],
        camera->xyz[1],
        camera->xyz[2]);
    float3 camera_translated_pos = rotated_pos - camera_position;
    
    // rotate around camera
    float3 cam_x_rotated = x_rotate(
        camera_translated_pos,
        -camera->xyz_angle[0]);
    float3 cam_y_rotated = y_rotate(
        cam_x_rotated,
        -camera->xyz_angle[1]);
    float3 cam_z_rotated = z_rotate(
        cam_y_rotated,
        -camera->xyz_angle[2]);
    
    float ic = clamp(
        polygon_collection->polygons[polygon_i].ignore_camera,
        0.0f,
        1.0f);
    float3 final_pos =
        (rotated_pos * ic) +
        (cam_z_rotated * (1.0f - ic));
    
    out.position[0] = final_pos[0];
    out.position[1] = final_pos[1];
    out.position[2] = final_pos[2];
    out.position[3] = 1.0f;
    
    // projection
    out.position[0] *= projection_constants->x_multiplier;
    out.position[1] *= projection_constants->field_of_view_modifier;
    out.position[3]  = out.position[2];
    out.position[2]  =
        (out.position[2] * projection_constants->q) -
        (projection_constants->znear * projection_constants->q);
    
    out.color = vector_float4(
        polygon_materials[locked_material_i].rgba[0],
        polygon_materials[locked_material_i].rgba[1],
        polygon_materials[locked_material_i].rgba[2],
        polygon_materials[locked_material_i].rgba[3]);
    
    float4 bonus_rgb = vector_float4(
        polygon_collection->polygons[polygon_i].bonus_rgb[0],
        polygon_collection->polygons[polygon_i].bonus_rgb[1],
        polygon_collection->polygons[polygon_i].bonus_rgb[2],
        0.0f);
    
    out.color += bonus_rgb;
    
    /* out.texturearray_i = vertices[vertex_i].texturearray_i; */
    out.texturearray_i = polygon_materials[locked_material_i].texturearray_i;
    /* out.texture_i = vertices[vertex_i].texture_i; */
    out.texture_i = polygon_materials[locked_material_i].texture_i;
    
    out.texture_coordinate = vector_float2(
        locked_vertices[locked_vertex_i].uv[0],
        locked_vertices[locked_vertex_i].uv[1]);
    
    out.lighting = float3(0.0f, 0.0f, 0.0f);
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
        float3 light_color = vector_float3(
            light_collection->red[i],
            light_collection->green[i],
            light_collection->blue[i]);
        float distance = get_distance(
            light_pos,
            rotated_pos);
        
        float distance_overflow = max(
            distance - (light_collection->reach[i] * 0.75f),
            0.0f);
        
        float attenuation = 1.0f - (
            distance_overflow /
                light_collection->reach[i]);
        
        attenuation = clamp(attenuation, 0.00f, 1.00f);
        
        out.lighting += (
            attenuation *
            light_color *
            light_collection->ambient[i]);
        
        // diffuse lighting
        z_rotated_normals = normalize(z_rotated_normals);
        
        // if light is at 2,2,2 and rotated_pos is at 1,1,1, we need +1/+1/+1
        // to go from the rotated_pos to the light
        // if the normal also points to the light, we want more diffuse
        // brightness
        float3 object_to_light = normalize(
            (light_pos - rotated_pos));
        
        float diffuse_dot =
            max(
                dot(
                    z_rotated_normals,
                    object_to_light),
                0.0f);
        
        out.lighting += (
            light_color *
            attenuation *
            light_collection->diffuse[i] *
            polygon_materials[locked_material_i].diffuse *
            diffuse_dot);
        
        // specular lighting
        float3 object_to_view = normalize(
            camera_position - rotated_pos);
        
        float3 reflection_ray = reflect(
            -object_to_light,
            z_rotated_normals);
        
        float specular_dot = pow(
            max(
                dot(object_to_view, reflection_ray),
                0.0),
            32);
        out.lighting += (
            polygon_materials[locked_material_i].specular *
            specular_dot *
            light_color *
            light_collection->specular[i] *
            attenuation);
    }
    
    out.lighting = clamp(out.lighting, 0.05f, 7.5f);
    
    float ignore_light =
        polygon_collection->polygons[polygon_i].ignore_lighting;
    
    float3 all_ones = vector_float3(1.0f, 1.0f, 1.0f);
    out.lighting =
        ((1.0f - ignore_light) * out.lighting) +
        (ignore_light * all_ones);
    
    out.point_size = 40.0f;
    
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
        out_color *= vector_float4(in.lighting, 1.0f);
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
        
        out_color *= texture_sample * vector_float4(in.lighting, 1.0f);;
    }
    
    int diamond_size = 35.0f;
    int neghalfdiamond = -1.0f * (diamond_size / 2);
    
    int alpha_tresh = (int)(out_color[3] * diamond_size);
    
    if (
        out_color[3] < 0.05f ||
        (
            out_color[3] < 0.95f &&
            (
                abs((neghalfdiamond + (int)in.position.x % diamond_size)) +
                abs((neghalfdiamond + (int)in.position.y % diamond_size))
            ) > alpha_tresh
        ))
    {
        discard_fragment();
        return out_color;
    }
    
    out_color[3] = 1.0f;
    return out_color;
}

fragment float4
alphablending_fragment_shader(
    RasterizerPixel in [[stage_in]],
    array<texture2d_array<half>, TEXTUREARRAYS_SIZE>
        color_textures[[ texture(0) ]])
{
    float4 out_color = in.color;
    
    if (
        in.texturearray_i < 0 ||
        in.texture_i < 0)
    {
        out_color *= vector_float4(in.lighting, 1.0f);
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
        
        out_color *= texture_sample * vector_float4(in.lighting, 1.0f);;
    }
    
    return out_color;
}

struct PostProcessingFragment
{
    float4 position [[position]];
    float2 texcoord;
    float2 screen_width_height;
    float blur_pct;
    float nonblur_pct;
    unsigned int curtime;
};

vertex PostProcessingFragment
single_quad_vertex_shader(
    const uint vertexID [[ vertex_id ]],
    const device PostProcessingVertex * vertices [[ buffer(0) ]],
    const constant GPUPostProcessingConstants * constants [[ buffer(1) ]])
{
    PostProcessingFragment out;
    
    out.position = vector_float4(
        vertices[vertexID].position[0],
        vertices[vertexID].position[1],
        1.0,
        1.0);
    
    out.texcoord = vector_float2(
        vertices[vertexID].texcoord[0],
        vertices[vertexID].texcoord[1]);
    
    out.screen_width_height = vector_float2(
        constants->screen_width * 2.0f,
        constants->screen_height * 2.0f);
    
    out.curtime = constants->timestamp;
    
    out.blur_pct = constants->blur_pct;
    
    out.nonblur_pct = constants->nonblur_pct;
    
    return out;
}

fragment float4
single_quad_fragment_shader(
    PostProcessingFragment in [[stage_in]],
    texture2d<half> texture  [[texture(0)]])
{
    constexpr sampler sampler(
        mag_filter::nearest,
        min_filter::nearest);
    
    float2 texcoord = in.texcoord;
    
    half4 color_sample = texture.sample(sampler, texcoord);
    color_sample[3] = 1.0f;
    
    // reinhard tone mapping
    color_sample = color_sample / (color_sample + 0.5f);
    color_sample = clamp(color_sample, 0.0f, 1.0f);
    color_sample[3] = 1.0f;
    
    return vector_float4(color_sample);
}

kernel void downsample_texture(
    texture2d<half, access::read> in_texture[[texture(0)]],
    texture2d<half, access::write> out_texture[[texture(1)]],
    uint2 out_pos [[thread_position_in_grid]])
{
    if (
        out_pos.x >= out_texture.get_width() ||
        out_pos.y >= out_texture.get_height())
    {
        return;
    }
    
    uint2  input_pos = out_pos * 2;
    half4  in_color  = in_texture.read(input_pos);
    
    out_texture.write(in_color, out_pos);
}

kernel void additive_blend_textures(
    texture2d<half, access::read_write> main_texture[[texture(0)]],
    texture2d<half, access::read>       ds_1 [[texture(1)]],
    texture2d<half, access::read>       ds_2 [[texture(2)]],
    texture2d<half, access::read>       ds_3 [[texture(3)]],
    uint2 main_pos [[thread_position_in_grid]])
{
    if (
        main_pos.x >= main_texture.get_width() ||
        main_pos.y >= main_texture.get_height())
    {
        return;
    }
    
    uint2 ds_pos = main_pos / 2;
    half4 main_color  = main_texture.read(main_pos);
    half4 ds_1_color = ds_1.read(ds_pos);
    ds_1_color = clamp(ds_1_color, 1.0f, 3.0f);
    ds_1_color -= 1.0f;
    
    ds_pos = main_pos / 4;
    half4 ds_2_color = ds_2.read(ds_pos);
    ds_2_color = clamp(ds_2_color, 1.0f, 3.0f);
    ds_2_color -= 1.0f;
    
    ds_pos = main_pos / 8;
    half4 ds_3_color = ds_3.read(ds_pos);
    ds_3_color = clamp(ds_3_color, 1.0f, 3.0f);
    ds_3_color -= 1.0f;
    
    main_texture.write(
        main_color + ds_1_color + ds_2_color + ds_3_color,
        main_pos);
}
