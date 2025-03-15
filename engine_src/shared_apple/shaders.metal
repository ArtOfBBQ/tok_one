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

float4 project_float3_to_float4_perspective(
    const float3 in_xyz,
    const device GPUProjectionConstants * pjc)
{
    float4 out;
    
    out[0] = in_xyz[0];
    out[1] = in_xyz[1];
    
    out[0] *= pjc->x_multiplier;
    out[1] *= pjc->field_of_view_modifier;
    out[3] = in_xyz[2];  // Positive Z in view space
    
    // clipspace z
    out[2] =
        pjc->zfar *
            (out[3] - pjc->znear) /
                (pjc->zfar - pjc->znear);
    
    return out;
}

vertex float4 shadows_vertex_shader(
    uint vertex_i [[ vertex_id ]],
    const device GPUVertex * vertices [[ buffer(0) ]],
    const device GPUPolygonCollection * polygon_collection [[ buffer(1) ]],
    const device GPULightCollection * light_collection [[ buffer(2) ]],
    const device GPUCamera * camera [[ buffer(3) ]],
    const device GPULockedVertex * locked_vertices [[ buffer(4) ]],
    const device GPUProjectionConstants * projection_constants [[ buffer(5) ]])
{
    float3 out_pos;
    
    uint polygon_i = vertices[vertex_i].polygon_i;
    
    uint locked_vertex_i = vertices[vertex_i].locked_vertex_i;
    
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
    
    
    // translate to world position
    out_pos = z_rotated_vertices + parent_mesh_position;
    
    float3 camera_position = vector_float3(
        light_collection->light_x[light_collection->shadowcaster_i],
        light_collection->light_y[light_collection->shadowcaster_i],
        light_collection->light_z[light_collection->shadowcaster_i]);
    float3 camera_translated_pos = out_pos - camera_position;
    
    // rotate around camera
    float3 cam_x_rotated = x_rotate(
        camera_translated_pos,
        light_collection->angle_x[light_collection->shadowcaster_i]);
    float3 cam_y_rotated = y_rotate(
        cam_x_rotated,
        light_collection->angle_y[light_collection->shadowcaster_i]);
    float3 cam_z_rotated = z_rotate(
        cam_y_rotated,
        light_collection->angle_z[light_collection->shadowcaster_i]);
    
    float ic = clamp(
        polygon_collection->polygons[polygon_i].ignore_camera,
        0.0f,
        1.0f);
    float3 final_pos =
        (out_pos * ic) +
        (cam_z_rotated * (1.0f - ic));
    
    return project_float3_to_float4_perspective(
        final_pos,
        projection_constants);
}

fragment void shadows_fragment_shader() {}

typedef struct
{
    float4 position [[position]];
    float2 texture_coordinate;
    float3 worldpos;
    float3 normal;
    float3 bonus_rgb [[ flat ]];
    int32_t material_i [[ flat ]];
    int32_t touchable_id [[ flat ]];
    float ignore_lighting [[ flat ]];
    float point_size [[ point_size ]];
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
    const device GPUCamera * camera [[ buffer(3) ]],
    const device GPULockedVertex * locked_vertices [[ buffer(4) ]],
    const device GPUProjectionConstants * projection_constants [[ buffer(5) ]])
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
    
    float3 x_rotated_normal  = x_rotate(
        mesh_normals,
        polygon_collection->polygons[polygon_i].xyz_angle[0]);
    float3 y_rotated_normal  = y_rotate(
        x_rotated_normal,
        polygon_collection->polygons[polygon_i].xyz_angle[1]);
    out.normal = z_rotate(
        y_rotated_normal,
        polygon_collection->polygons[polygon_i].xyz_angle[2]);
    
    // translate to world position
    out.worldpos = z_rotated_vertices + parent_mesh_position;
    
    float3 camera_position = vector_float3(
        camera->xyz[0],
        camera->xyz[1],
        camera->xyz[2]);
    float3 camera_translated_pos = out.worldpos - camera_position;
    
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
        (out.worldpos * ic) +
        (cam_z_rotated * (1.0f - ic));
    
    out.bonus_rgb = vector_float3(
    polygon_collection->polygons[polygon_i].bonus_rgb[0],
    polygon_collection->polygons[polygon_i].bonus_rgb[1],
    polygon_collection->polygons[polygon_i].bonus_rgb[2]);
    
    out.material_i = locked_material_i;
    
    out.touchable_id = polygon_collection->polygons[polygon_i].touchable_id;
    
    out.texture_coordinate = vector_float2(
        locked_vertices[locked_vertex_i].uv[0],
        locked_vertices[locked_vertex_i].uv[1]);
    
    out.ignore_lighting =
        polygon_collection->polygons[polygon_i].ignore_lighting;
    
    out.point_size = 40.0f;
    
    out.position = project_float3_to_float4_perspective(
        final_pos,
        projection_constants);
    
    return out;
}

struct FragmentAndTouchableOut {
    half4 color [[color(0)]];       // Output to screen
    half4 touchable_id [[color(1)]];  // Output to ID buffer
};

/*
We want to output an int32 for 1 of our render targets (touchable_id), but
Metal enforces you use the same data type when you render to multiple targets
simultaneously. That's why we're packing our int32 inside of float16 slots. On
the CPU side we'll retrieve them and put them back together as an int32.
*/
static FragmentAndTouchableOut pack_color_and_touchable_id(
    float4 color,
    int32_t touchable_id)
{
    FragmentAndTouchableOut return_value;
    
    return_value.color = vector_half4(color[0], color[1], color[2], color[3]);
    
    // Interpret signed int32_t as uint32_t (0 to 2^32-1)
    uint uid = as_type<uint>(touchable_id);
    
    // Note: We could just pack our 4-byte int into only 2 channels,
    // and originally that's what I did, but it lead to a nasty bug
    //
    // we're using all 4 channels to avoid NaN values
    // we found out that the GPU flattens all NaN behind our back later,
    // which I guess is considered a risk-free optimization from their POV.
    // (NaN can be represented by many bit patterns, but from their POV it
    // will always have the same effect, so they replace all NaN with their
    // own version of NaN)
    // I think it's 1 of the more nasty traps I've ever run into,
    // and I guess it's the price we pay here for using the graphics API in
    // a way we're not supposed to.
    
    // Split into two 16-bit chunks
    uint16_t fourth_8 = ((uid >> 24) & 0xFF);
    uint16_t third_8 = ((uid >> 16) & 0xFF);
    uint16_t second_8 = ((uid >> 8) & 0xFF);
    uint16_t first_8  = (uid & 0xFF);
    
    // Pack into R and G channels (blue and alpha unused)
    return_value.touchable_id = vector_half4(
        as_type<half>(first_8), /* lowest 8 bits stored in red channel */
        as_type<half>(second_8), /* 2nd lowest 8 bits (green channel) */
        as_type<half>(third_8), /* blue channel */
        as_type<half>(fourth_8)); /* alpha channel */
    
    return return_value;
}

float3 get_lighting(
    texture2d<float> shadow_map,
    const device GPUCamera * camera,
    const device GPULightCollection * light_collection,
    const device GPUProjectionConstants * projection_constants,
    const device GPUPolygonMaterial * fragment_material,
    float3 fragment_worldpos,
    float3 fragment_normal,
    float ignore_lighting)
{
    float3 return_value = float3(0.0f, 0.0f, 0.0f);
    
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
        float3 light_angle_xyz = vector_float3(
            light_collection->angle_x[i],
            light_collection->angle_y[i],
            light_collection->angle_z[i]);
        
        float shadow_factor = 1.0f;
        if (light_collection->shadowcaster_i == i) {
            constexpr sampler shadow_sampler(
                mag_filter::nearest,
                min_filter::nearest);
            
            float3 light_translated_pos = fragment_worldpos - light_pos;
            float3 light_x_rotated = x_rotate(
                light_translated_pos,
                light_angle_xyz[0]);
            float3 light_y_rotated = y_rotate(
                light_x_rotated,
                light_angle_xyz[1]);
            float3 light_z_rotated = z_rotate(
                light_y_rotated,
                light_angle_xyz[2]); // [0, 0, 1]
            
            float4 light_clip_pos = project_float3_to_float4_perspective(
                light_z_rotated,
                projection_constants);
            
            float2 shadow_uv =
                ((light_clip_pos.xy / light_clip_pos.w) * 0.5f) + 0.5f;
            shadow_uv[1] = 1.0f - shadow_uv[1];
            float shadow_depth = shadow_map.sample(
                shadow_sampler,
                shadow_uv).r;
            
            float frag_depth = light_clip_pos.z / light_clip_pos.w;
            
            shadow_factor = (frag_depth <= shadow_depth + 0.00002f) ?
                1.0f : 0.25f;
        }
        
        float distance = get_distance(
            light_pos,
            fragment_worldpos);
        
        float distance_overflow = max(
            distance - (light_collection->reach[i] * 0.75f),
            0.0f);
        
        float attenuation = 1.0f - (
            distance_overflow /
                light_collection->reach[i]);
        
        attenuation = clamp(attenuation, 0.00f, 1.00f);
        
        return_value += (
            attenuation *
            light_color *
            light_collection->ambient[i] *
            shadow_factor);
        
        // if light is at 2,2,2 and rotated_pos is at 1,1,1, we need +1/+1/+1
        // to go from the rotated_pos to the light
        // if the normal also points to the light, we want more diffuse
        // brightness
        float3 object_to_light = normalize(
            (light_pos - fragment_worldpos));
        
        float diffuse_dot =
            max(
                dot(
                    fragment_normal,
                    object_to_light),
                0.0f);
        
        return_value += (
            light_color *
            attenuation *
            light_collection->diffuse[i] *
            fragment_material->diffuse *
            diffuse_dot *
            shadow_factor);
        
        // specular lighting
        float3 object_to_view = normalize(
            float3(
                camera->xyz[0],
                camera->xyz[1],
                camera->xyz[2]) - fragment_worldpos);
        
        float3 reflection_ray = reflect(
            -object_to_light,
            fragment_normal);
        
        float specular_dot = pow(
            max(
                dot(object_to_view, reflection_ray),
                0.0),
            32);
        return_value += (
            fragment_material->specular *
            specular_dot *
            light_color *
            light_collection->specular[i] *
            attenuation *
            shadow_factor);
    }
    
    return_value = clamp(return_value, 0.05f, 7.5f);
    
    float3 all_ones = vector_float3(1.0f, 1.0f, 1.0f);
    return_value =
        ((1.0f - ignore_lighting) * return_value) +
        (ignore_lighting * all_ones);
    
    return return_value;
}

fragment FragmentAndTouchableOut
fragment_shader(
    RasterizerPixel in [[stage_in]],
    array<texture2d_array<half>, TEXTUREARRAYS_SIZE>
        color_textures[[ texture(0) ]],
    texture2d<float> shadow_map [[texture(31)]],
    const device GPULightCollection * light_collection [[ buffer(2) ]],
    const device GPUCamera * camera [[ buffer(3) ]],
    const device GPUProjectionConstants * projection_constants [[ buffer(4) ]],
    const device GPUPolygonMaterial * polygon_materials [[ buffer(6) ]])
{
    
    if (
        in.material_i < 0 ||
        in.material_i >= MAX_POLYGONS_PER_BUFFER * MAX_MATERIALS_PER_POLYGON)
    {
        discard_fragment();
    }
    
    float3 lighting = get_lighting(
        /* */
            shadow_map,
        /* const device GPUCamera * camera: */
            camera,
        /* const device GPULightCollection * light_collection: */
            light_collection,
        /* const device GPUProjectionConstants * projection_constants: */
            projection_constants,
        /* const device GPUPolygonMaterial * fragment_material: */
            &polygon_materials[in.material_i],
        /* float3 fragment_worldpos: */
            in.worldpos,
        /* float3 fragment_normal: */
            in.normal,
        /* float ignore_lighting: */
            in.ignore_lighting);
    
    float4 out_color = vector_float4(
        polygon_materials[in.material_i].rgba[0],
        polygon_materials[in.material_i].rgba[1],
        polygon_materials[in.material_i].rgba[2],
        polygon_materials[in.material_i].rgba[3]);
    
    float4 texture_sample = vector_float4(1.0f, 1.0f, 1.0f, 1.0f);
    
    if (
        polygon_materials[in.material_i].texturearray_i >= 0)
    {
        if (
            polygon_materials[in.material_i].texturearray_i >= 31 ||
            polygon_materials[in.material_i].texture_i < 0 ||
            polygon_materials[in.material_i].texture_i > 100)
        {
            discard_fragment();
        }
        
        constexpr sampler textureSampler(
            mag_filter::linear,
            min_filter::linear);
        
        // Sample the texture to obtain a color
        const half4 color_sample =
        color_textures[polygon_materials[in.material_i].texturearray_i].sample(
            textureSampler,
            in.texture_coordinate,
            polygon_materials[in.material_i].texture_i);
        texture_sample = float4(color_sample);
    }
    
    out_color = out_color * texture_sample * vector_float4(lighting, 1.0f);
    
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
    }
    
    out_color[3] = 1.0f;
    float4 rgba_cap = vector_float4(
        polygon_materials[in.material_i].rgb_cap[0],
        polygon_materials[in.material_i].rgb_cap[1],
        polygon_materials[in.material_i].rgb_cap[2],
        1.0f);
    out_color = clamp(out_color, 0.0f, rgba_cap);
    
    FragmentAndTouchableOut packed_out =
        pack_color_and_touchable_id(out_color, in.touchable_id);
    
    return packed_out;
}

fragment FragmentAndTouchableOut
alphablending_fragment_shader(
    RasterizerPixel in [[stage_in]],
    array<texture2d_array<half>, TEXTUREARRAYS_SIZE>
        color_textures[[ texture(0) ]],
    texture2d<float> shadow_map [[texture(31)]],
    const device GPULightCollection * light_collection [[ buffer(2) ]],
    const device GPUCamera * camera [[ buffer(3) ]],
    const device GPUProjectionConstants * projection_constants [[ buffer(4) ]],
    const device GPUPolygonMaterial * polygon_materials [[ buffer(6) ]])
{
    float3 lighting = get_lighting(
        /* texture2d<float> shadow_map: */
            shadow_map,
        /* const device GPUCamera * camera: */
            camera,
        /* const device GPULightCollection * light_collection: */
            light_collection,
        /* const device GPUProjectionConstants * projection_constants: */
            projection_constants,
        /* const device GPUPolygonMaterial * fragment_material: */
            &polygon_materials[in.material_i],
        /* float3 fragment_worldpos: */
            in.worldpos,
        /* float3 fragment_normal: */
            in.normal,
        /* float ignore_lighting: */
            in.ignore_lighting);
    
    float4 out_color = vector_float4(
        polygon_materials[in.material_i].rgba[0],
        polygon_materials[in.material_i].rgba[1],
        polygon_materials[in.material_i].rgba[2],
        polygon_materials[in.material_i].rgba[3]);
    
    if (
        polygon_materials[in.material_i].texturearray_i < 0 ||
        polygon_materials[in.material_i].texture_i < 0)
    {
        
    } else {
        constexpr sampler textureSampler(
            mag_filter::nearest,
            min_filter::nearest);
        
        // Sample the texture to obtain a color
        const half4 color_sample =
        color_textures[polygon_materials[in.material_i].texturearray_i].sample(
            textureSampler,
            in.texture_coordinate,
            polygon_materials[in.material_i].texture_i);
        float4 texture_sample = float4(color_sample);
        
        out_color *= texture_sample;
    }
    
    out_color *= vector_float4(lighting, 1.0f);
    
    float4 rgba_cap = vector_float4(
        polygon_materials[in.material_i].rgb_cap[0],
        polygon_materials[in.material_i].rgb_cap[1],
        polygon_materials[in.material_i].rgb_cap[2],
        1.0f);
    out_color = clamp(out_color, 0.0f, rgba_cap);
    
    return pack_color_and_touchable_id(out_color, in.touchable_id);
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
    texture2d<half> texture  [[texture(0)]],
    texture2d<half> downsampled_1  [[texture(1)]],
    texture2d<half> downsampled_2  [[texture(2)]],
    texture2d<half> downsampled_3  [[texture(3)]],
    texture2d<half> downsampled_4  [[texture(4)]],
    texture2d<half> downsampled_5  [[texture(5)]])
{
    constexpr sampler sampler(
        mag_filter::linear,
        min_filter::linear);
    
    float2 texcoord = in.texcoord;
    
    half4 color_sample = texture.sample(sampler, texcoord);
    color_sample += downsampled_1.sample(sampler, texcoord);
    color_sample += downsampled_2.sample(sampler, texcoord);
    color_sample += downsampled_3.sample(sampler, texcoord);
    color_sample += downsampled_4.sample(sampler, texcoord);
    color_sample += downsampled_5.sample(sampler, texcoord);
    color_sample[3] = 1.0f;
    
    // reinhard tone mapping
    color_sample = color_sample / (color_sample + 0.5f);
    color_sample = clamp(color_sample, 0.0f, 1.0f);
    color_sample[3] = 1.0f;
    
    return vector_float4(color_sample);
}

kernel void threshold_texture(
    texture2d<half, access::read_write> texture[[texture(0)]],
    uint2 grid_pos [[thread_position_in_grid]])
{
    half4 in_color = texture.read(grid_pos);
    
    half4 thresholded =
        in_color *
        ((in_color[0] + in_color[1] + in_color[2]) > (1.05h * 3.0h));
    
    texture.write(thresholded, grid_pos);
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
    
    uint2 input_pos = out_pos * 2;
    
    half4 in_color = (
        in_texture.read(input_pos) +
        in_texture.read(input_pos + vector_uint2(1, 1)) +
        in_texture.read(input_pos + vector_uint2(1, 0)) +
        in_texture.read(input_pos + vector_uint2(0, 1))) / 4.0h;
    
    out_texture.write(in_color, out_pos);
}

kernel void boxblur_texture(
    texture2d<half, access::read_write> texture[[texture(0)]],
    uint2 pos [[thread_position_in_grid]])
{
    uint2 prev_pos = pos - vector_uint2(1, 1);
    
    half4 in_color = (
        texture.read(pos + vector_uint2(1, 1)) +
        texture.read(pos + vector_uint2(1, 0)) +
        texture.read(pos + vector_uint2(0, 1)) +
        texture.read(pos) +
        texture.read(prev_pos + vector_uint2(1, 1)) +
        texture.read(prev_pos + vector_uint2(1, 0)) +
        texture.read(prev_pos + vector_uint2(0, 1)) +
        texture.read(prev_pos + vector_uint2(0, 2)) +
        texture.read(prev_pos + vector_uint2(2, 0))) / 10.0f;
    
    texture.write(in_color, pos);
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
        projection_constants->zfar *
            (out.position[3] - projection_constants->znear) /
                (projection_constants->zfar - projection_constants->znear);
    
    out.color = vertices[vertex_i].color;
    
    return out;
}

fragment float4
raw_fragment_shader(RawFragment in [[stage_in]])
{
    float4 out_color = vector_float4(in.color, in.color / 3.0f, 1.0f, 1.0f);
    
    return out_color;
}
