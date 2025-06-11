#include <metal_stdlib>
#include <simd/simd.h>

#include "T1_cpu_gpu_shared_types.h"
#include "clientlogic_macro_settings.h"

using namespace metal;

float3 x_rotate(const float3 vertices, const float x_angle) {
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

float3 y_rotate(const float3 vertices, const float y_angle) {
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

float3 z_rotate(const float3 vertices, const float z_angle) {
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

float3 xyz_rotate(const float3 vertices, const float3 xyz_angle) {
    float3 return_value = x_rotate(vertices,     xyz_angle[0]);
    return_value        = y_rotate(return_value, xyz_angle[1]);
    return_value        = z_rotate(return_value, xyz_angle[2]);
    
    return return_value;
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
    const device GPUzSprite * polygons [[ buffer(1) ]],
    const device GPULight * lights [[ buffer(2) ]],
    const device GPUCamera * camera [[ buffer(3) ]],
    const device GPULockedVertex * locked_vertices [[ buffer(4) ]],
    const device GPUProjectionConstants * projection_constants [[ buffer(5) ]],
    const device GPUPostProcessingConstants * updating_globals [[ buffer (6) ]])
{
    float3 out_pos;
    
    uint polygon_i = vertices[vertex_i].polygon_i;
    
    if (polygons[polygon_i].remove_shadow) {
        // early out by failing the depth test
        return vector_float4(
            0.0f,
            0.0f,
            projection_constants->zfar + 10.0f,
            1.0f);
    }
    
    uint locked_vertex_i = vertices[vertex_i].locked_vertex_i;
    
    float3 parent_mesh_position = vector_float3(
        polygons[polygon_i].xyz[0],
        polygons[polygon_i].xyz[1],
        polygons[polygon_i].xyz[2]);
    
    float3 mesh_vertices = vector_float3(
        locked_vertices[locked_vertex_i].xyz[0],
        locked_vertices[locked_vertex_i].xyz[1],
        locked_vertices[locked_vertex_i].xyz[2]);
    
    float3 vertex_multipliers = vector_float3(
        polygons[polygon_i].xyz_multiplier[0],
        polygons[polygon_i].xyz_multiplier[1],
        polygons[polygon_i].xyz_multiplier[2]);
    
    float3 vertex_offsets = vector_float3(
        polygons[polygon_i].xyz_offset[0],
        polygons[polygon_i].xyz_offset[1],
        polygons[polygon_i].xyz_offset[2]);
    
    mesh_vertices *= vertex_multipliers;
    mesh_vertices += vertex_offsets;
    
    mesh_vertices *= polygons[polygon_i].scale_factor;
    
    // rotate vertices
    float3 z_rotated_vertices = xyz_rotate(
        mesh_vertices,
        vector_float3(
            polygons[polygon_i].xyz_angle[0],
            polygons[polygon_i].xyz_angle[1],
            polygons[polygon_i].xyz_angle[2]));
    // translate to world position
    out_pos = z_rotated_vertices + parent_mesh_position;
    
    float3 camera_position = vector_float3(
        lights[updating_globals->shadowcaster_i].xyz[0],
        lights[updating_globals->shadowcaster_i].xyz[1],
        lights[updating_globals->shadowcaster_i].xyz[2]);
    float3 camera_translated_pos = out_pos - camera_position;
    
    // rotate around camera
    float3 cam_z_rotated = xyz_rotate(
        camera_translated_pos,
        vector_float3(
            -lights[updating_globals->shadowcaster_i].angle_xyz[0],
            -lights[updating_globals->shadowcaster_i].angle_xyz[1],
            -lights[updating_globals->shadowcaster_i].angle_xyz[2]));
    
    float ic = clamp(
        polygons[polygon_i].ignore_camera,
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
    float3 tangent;
    float3 bitangent;
    unsigned int locked_vertex_i [[ flat ]];
    unsigned int polygon_i [[ flat ]];
    int32_t touchable_id [[ flat ]];
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
    const device GPUzSprite * polygons [[ buffer(1) ]],
    const device GPUCamera * camera [[ buffer(3) ]],
    const device GPULockedVertex * locked_vertices [[ buffer(4) ]],
    const device GPUProjectionConstants * projection_constants [[ buffer(5) ]])
{
    RasterizerPixel out;
    
    out.polygon_i = vertices[vertex_i].polygon_i;
    out.locked_vertex_i = vertices[vertex_i].locked_vertex_i;
    
    float3 parent_mesh_position = vector_float3(
        polygons[out.polygon_i].xyz[0],
        polygons[out.polygon_i].xyz[1],
        polygons[out.polygon_i].xyz[2]);
    
    float3 mesh_vertices = vector_float3(
        locked_vertices[out.locked_vertex_i].xyz[0],
        locked_vertices[out.locked_vertex_i].xyz[1],
        locked_vertices[out.locked_vertex_i].xyz[2]);
    
    float3 vertex_multipliers = vector_float3(
        polygons[out.polygon_i].xyz_multiplier[0],
        polygons[out.polygon_i].xyz_multiplier[1],
        polygons[out.polygon_i].xyz_multiplier[2]);
    
    float3 vertex_offsets = vector_float3(
        polygons[out.polygon_i].xyz_offset[0],
        polygons[out.polygon_i].xyz_offset[1],
        polygons[out.polygon_i].xyz_offset[2]);
    
    mesh_vertices *= vertex_multipliers;
    mesh_vertices += vertex_offsets;
    
    mesh_vertices *= polygons[out.polygon_i].scale_factor;
    
    float3 mesh_normals = vector_float3(
        locked_vertices[out.locked_vertex_i].normal_xyz[0],
        locked_vertices[out.locked_vertex_i].normal_xyz[1],
        locked_vertices[out.locked_vertex_i].normal_xyz[2]);
    
    float3 mesh_tangent = vector_float3(
        locked_vertices[out.locked_vertex_i].tangent_xyz[0],
        locked_vertices[out.locked_vertex_i].tangent_xyz[1],
        locked_vertices[out.locked_vertex_i].tangent_xyz[2]);
    
    float3 mesh_bitangent = vector_float3(
        locked_vertices[out.locked_vertex_i].bitangent_xyz[0],
        locked_vertices[out.locked_vertex_i].bitangent_xyz[1],
        locked_vertices[out.locked_vertex_i].bitangent_xyz[2]);
    
    float3 parent_xyz_angle = vector_float3(
        polygons[out.polygon_i].xyz_angle[0],
        polygons[out.polygon_i].xyz_angle[1],
        polygons[out.polygon_i].xyz_angle[2]);
    
    float3 z_rotated_vertices = xyz_rotate(
        mesh_vertices,
        parent_xyz_angle);
    
    out.normal = xyz_rotate(
        mesh_normals,
        parent_xyz_angle);
    out.tangent = xyz_rotate(
        mesh_tangent,
        parent_xyz_angle);
    out.bitangent = xyz_rotate(
        mesh_bitangent,
        parent_xyz_angle);
    
    // translate to world position
    out.worldpos = z_rotated_vertices + parent_mesh_position;
    
    float3 camera_position = vector_float3(
        camera->xyz[0],
        camera->xyz[1],
        camera->xyz[2]);
    float3 camera_translated_pos = out.worldpos - camera_position;
    
    // rotate around camera
    float3 cam_z_rotated = xyz_rotate(
        camera_translated_pos,
        vector_float3(
            -camera->xyz_angle[0],
            -camera->xyz_angle[1],
            -camera->xyz_angle[2]));
    
    float ic = clamp(
        polygons[out.polygon_i].ignore_camera,
        0.0f,
        1.0f);
    float3 final_pos =
        (out.worldpos * ic) +
        (cam_z_rotated * (1.0f - ic));
    
    out.touchable_id = polygons[out.polygon_i].touchable_id;
    
    out.texture_coordinate = vector_float2(
        locked_vertices[out.locked_vertex_i].uv[0],
        locked_vertices[out.locked_vertex_i].uv[1]);
    
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
    uint16_t third_8  = ((uid >> 16) & 0xFF);
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

// Gets the color given a material and lighting setup
float4 get_lit(
    #if SHADOWS_ACTIVE
    texture2d<float> shadow_map,
    #endif
    array<texture2d_array<half>, TEXTUREARRAYS_SIZE> color_textures,
    const device GPUCamera * camera,
    const device GPULight * lights,
    const device GPUProjectionConstants * projection_constants,
    const device GPUzSprite * zsprite,
    const device GPULockedMaterial * material,
    const device GPUPostProcessingConstants * updating_globals,
    const RasterizerPixel in)
{
    float4 lit_color = vector_float4(
        0.0f,
        0.0f,
        0.0f,
        1.0f);
    
    float4 ambient_base = vector_float4(
        material->ambient_rgb[0],
        material->ambient_rgb[1],
        material->ambient_rgb[2],
        1.0f);
    float4 diffuse_base = vector_float4(
        material->diffuse_rgb[0],
        material->diffuse_rgb[1],
        material->diffuse_rgb[2],
        1.0f);
    float4 specular_base = vector_float4(
        material->specular_rgb[0],
        material->specular_rgb[1],
        material->specular_rgb[2],
        1.0f);
    
    float4 diffuse_texture_sample = vector_float4(0.75f, 0.75f, 0.75f, 1.0f);
    
    constexpr sampler texture_sampler(
        mag_filter::linear,
        min_filter::linear);
    
    #if TEXTURES_ACTIVE
    if (material->texturearray_i >= 0)
    {
    #else
    if (material->texturearray_i == 0)
    {
    #endif
        // Sample the texture to obtain a color
        const half4 color_sample =
            color_textures[material->texturearray_i].sample(
                texture_sampler,
                in.texture_coordinate,
                material->texture_i);
        diffuse_texture_sample = float4(color_sample);
    }
    
    float4 ignore_lighting_color = diffuse_texture_sample * diffuse_base;
    
    for (
        uint32_t i = 0;
        i < updating_globals->lights_size;
        i++)
    {
        // ambient lighting
        float3 light_pos = vector_float3(
            lights[i].xyz[0],
            lights[i].xyz[1],
            lights[i].xyz[2]);
        float4 light_color = vector_float4(
            lights[i].rgb[0],
            lights[i].rgb[1],
            lights[i].rgb[2],
            1.0f);
        
        float shadow_factor = 1.0f;
        #if SHADOWS_ACTIVE
        float3 light_angle_xyz = vector_float3(
            lights[i].angle_xyz[0],
            lights[i].angle_xyz[1],
            lights[i].angle_xyz[2]);
        
        if (updating_globals->shadowcaster_i == i) {
            constexpr sampler shadow_sampler(
                mag_filter::nearest,
                min_filter::nearest);
            
            float3 light_translated_pos = in.worldpos - light_pos;
            float3 light_z_rotated = xyz_rotate(
                light_translated_pos,
                vector_float3(
                    -light_angle_xyz[0],
                    -light_angle_xyz[1],
                    -light_angle_xyz[2]));
            
            float4 light_clip_pos =
                project_float3_to_float4_perspective(
                    light_z_rotated,
                    projection_constants);
            
            float2 shadow_uv =
                ((light_clip_pos.xy / light_clip_pos.w) * 0.5f) +
                    0.5f;
            
            shadow_uv[1] = 1.0f - shadow_uv[1];
            float shadow_depth = shadow_map.sample(
                shadow_sampler,
                shadow_uv).r;
            
            float frag_depth = light_clip_pos.z / light_clip_pos.w;
            
            shadow_factor =
                (frag_depth <= shadow_depth + SHADOW_BIAS) ?
                    1.0f : 0.25f;
        }
        #endif
        
        float distance = get_distance(
            light_pos,
            in.worldpos);
        
        float distance_overflow = max(
            distance - (lights[i].reach * 0.75f),
            0.0f);
        
        float attenuation = 1.0f - (
            distance_overflow / lights[i].reach);
        
        attenuation = clamp(attenuation, 0.00f, 1.00f);
        
        float4 light_ambient_multiplier =
            attenuation *
            light_color *
            lights[i].ambient *
            shadow_factor;
        
        // if light is at 2,2,2 and rotated_pos is at 1,1,1, we need +1/+1/+1
        // to go from the rotated_pos to the light
        // if the normal also points to the light, we want more diffuse
        // brightness
        float3 object_to_light = normalize(
            (light_pos - in.worldpos));
        
        if (material->normalmap_texturearray_i >= 0) {
            //            half4 normal_map_sample =
            //                color_textures[material->normalmap_texturearray_i].sample(
            //                    texture_sampler,
            //                    in.texture_coordinate,
            //                    material->normalmap_texture_i);
            //
            //            normal_map_sample = normalize((normal_map_sample * 2.0h) - 1.0h);
            
            // TODO: perturb in.normal using TBN (later)
            float4 vector = vector_float4(in.bitangent, 1.0f); // Switch to in.bitangent to visualize
            return vector * 0.5f + 0.5f; // Return for visualization
        }
        
        float diffuse_dot =
            max(
                dot(
                    in.normal,
                    object_to_light),
                0.0f) * 0.85f + 0.15f;
        
        lit_color += (
            ambient_base *
            light_ambient_multiplier);
        
        float4 light_diffuse_multiplier =
            attenuation *
            light_color *
            diffuse_dot *
            lights[i].diffuse *
            shadow_factor;
        
        lit_color += (
            diffuse_base *
            diffuse_texture_sample *
            light_diffuse_multiplier);
        
        // specular lighting
        float3 object_to_view = normalize(
            float3(
                camera->xyz[0],
                camera->xyz[1],
                camera->xyz[2]) - in.worldpos);
        
        float3 reflection_ray = reflect(
            -object_to_light,
            in.normal);
        
        float specular_dot = pow(
            max(
                dot(object_to_view, reflection_ray),
                0.0),
            material->specular_exponent);
        
        lit_color += (
            specular_base *
            attenuation *
            light_color *
            specular_dot *
            lights[i].specular *
            shadow_factor);
    }
    
    lit_color += vector_float4(
        zsprite->bonus_rgb[0],
        zsprite->bonus_rgb[1],
        zsprite->bonus_rgb[2],
        0.0f);
    
    lit_color =
        ((1.0f - zsprite->ignore_lighting) * lit_color) +
        (zsprite->ignore_lighting * ignore_lighting_color);
    
    float4 rgba_cap = vector_float4(
        material->rgb_cap[0],
        material->rgb_cap[1],
        material->rgb_cap[2],
        1.0f);
    
    lit_color = clamp(lit_color, 0.0f, rgba_cap);
    
    return vector_float4(
        lit_color[0],
        lit_color[1],
        lit_color[2],
        material->alpha * ignore_lighting_color[3]);;
}

fragment FragmentAndTouchableOut
fragment_shader(
    const RasterizerPixel in [[stage_in]],
    array<texture2d_array<half>, TEXTUREARRAYS_SIZE>
        color_textures[[ texture(0) ]],
    #if SHADOWS_ACTIVE
    texture2d<float> shadow_map [[texture(SHADOWMAP_TEXTUREARRAY_I)]],
    #endif
    const device GPULockedVertex * locked_vertices [[ buffer(0) ]],
    const device GPUzSprite * polygons [[ buffer(1) ]],
    const device GPULight * lights [[ buffer(2) ]],
    const device GPUCamera * camera [[ buffer(3) ]],
    const device GPUProjectionConstants * projection_constants [[ buffer(4) ]],
    const device GPULockedMaterial * locked_materials [[ buffer(6) ]],
    const device GPUPostProcessingConstants * updating_globals [[ buffer(7) ]])
{
    unsigned int material_i =
        locked_vertices[in.locked_vertex_i].parent_material_i;
    const device GPULockedMaterial * material =
        material_i == PARENT_MATERIAL_BASE ?
            &polygons[in.polygon_i].base_material :
            &locked_materials[locked_vertices[in.locked_vertex_i].locked_materials_head_i + material_i];
    
    float4 lit_color = get_lit(
        #if SHADOWS_ACTIVE
            shadow_map,
        #endif
            color_textures,
        /* const device GPUCamera * camera: */
            camera,
        /* const device GPULight * lights: */
            lights,
        /* const device GPUProjectionConstants * projection_constants: */
            projection_constants,
        /* const device zSprite * zsprite: */
            &polygons[in.polygon_i],
        /* const device GPUPolygonMaterial * fragment_material: */
            material,
        /* const device GPUPostProcessingConstants * updating_globals: */
            updating_globals,
        /* const device RasterizerPixel * in: */
            in);
    
    int diamond_size = 35.0f;
    int neghalfdiamond = -1.0f * (diamond_size / 2);
    
    int alpha_tresh = (int)(lit_color[3] * diamond_size);
    
    if (
        lit_color[3] < 0.05f ||
        (
            lit_color[3] < 0.95f &&
            (
                abs((neghalfdiamond + (int)in.position.x % diamond_size)) +
                abs((neghalfdiamond + (int)in.position.y % diamond_size))
            ) > alpha_tresh
        ))
    {
        discard_fragment();
    }
    
    FragmentAndTouchableOut packed_out =
        pack_color_and_touchable_id(lit_color, in.touchable_id);
    
    return packed_out;
}

fragment FragmentAndTouchableOut
alphablending_fragment_shader(
    RasterizerPixel in [[stage_in]],
    array<texture2d_array<half>, TEXTUREARRAYS_SIZE>
        color_textures[[ texture(0), maybe_unused ]],
    #if SHADOWS_ACTIVE
    texture2d<float> shadow_map [[texture(SHADOWMAP_TEXTUREARRAY_I)]],
    #endif
    const device GPULockedVertex * locked_vertices [[ buffer(0) ]],
    const device GPUzSprite * polygons [[ buffer(1) ]],
    const device GPULight * lights [[ buffer(2) ]],
    const device GPUCamera * camera [[ buffer(3) ]],
    const device GPUProjectionConstants * projection_constants [[ buffer(4) ]],
    const device GPULockedMaterial * locked_materials [[ buffer(6) ]],
    const device GPUPostProcessingConstants * updating_globals [[ buffer(7) ]])
{
    unsigned int material_i =
        locked_vertices[in.locked_vertex_i].parent_material_i;
    
    const device GPULockedMaterial * material =
        material_i == PARENT_MATERIAL_BASE ?
            &polygons[in.polygon_i].base_material :
            &locked_materials[locked_vertices[in.locked_vertex_i].locked_materials_head_i + material_i];
    
    float4 lit_color = get_lit(
        #if SHADOWS_ACTIVE
        /* texture2d<float> shadow_map: */
            shadow_map,
        #endif
            color_textures,
        /* const device GPUCamera * camera: */
            camera,
        /* const device GPULight * lights: */
            lights,
        /* const device GPUProjectionConstants * projection_constants: */
            projection_constants,
        /* const device GPUzSprite zsprite: */
            &polygons[in.polygon_i],
        /* const device GPUPolygonMaterial * fragment_material: */
            material,
        /* const device GPUPostProcessingConstants * updating_globals: */
            updating_globals,
        /* ???: */
            in);
    
    return pack_color_and_touchable_id(lit_color, in.touchable_id);
}

// TODO: just pass this directly to fragment shader
struct PostProcessingFragment
{
    float4 position [[position]];
    half4 bonus_rgb;
    half4 fog_rgb;
    float2 texcoord;
    float2 screen_width_height;
    float blur_pct;
    float nonblur_pct;
    float color_quantization;
    float fog_factor;
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
    
    out.bonus_rgb = vector_half4(
        constants->rgb_add[0],
        constants->rgb_add[1],
        constants->rgb_add[2],
        1.0h);
    
    out.color_quantization = constants->color_quantization;
    
    #if FOG_ACTIVE
    out.fog_factor = constants->fog_factor;
    out.fog_rgb = vector_half4(
        constants->fog_color[0],
        constants->fog_color[1],
        constants->fog_color[2],
        1.0h);
    #endif
    
    return out;
}

fragment half4
single_quad_fragment_shader(
    PostProcessingFragment in [[stage_in]],
    texture2d<half> texture  [[texture(0)]],
    #if BLOOM_ACTIVE
    texture2d<half> downsampled_1  [[texture(1)]],
    texture2d<half> downsampled_2  [[texture(2)]],
    texture2d<half> downsampled_3  [[texture(3)]],
    texture2d<half> downsampled_4  [[texture(4)]],
    texture2d<half> downsampled_5  [[texture(5)]],
    #endif
    texture2d<half> perlin_texture  [[texture(6)]],
    depth2d<float> camera_depth_map
        [[texture(CAMERADEPTH_TEXTUREARRAY_I)]])
{
    constexpr sampler sampler(
        s_address::repeat,
        t_address::repeat,
        mag_filter::linear,
        min_filter::linear);
    
    float2 texcoord = in.texcoord;
    
    half4 color_sample = texture.sample(sampler, texcoord);
    #if BLOOM_ACTIVE
    color_sample += downsampled_1.sample(sampler, texcoord);
    color_sample += downsampled_2.sample(sampler, texcoord);
    color_sample += downsampled_3.sample(sampler, texcoord);
    color_sample += downsampled_4.sample(sampler, texcoord);
    color_sample += downsampled_5.sample(sampler, texcoord);
    #endif
    
    color_sample[3] = 1.0f;
    color_sample += clamp(in.bonus_rgb, 0.0h, 0.25h);
    
    float dist = camera_depth_map.sample(sampler, texcoord);
    dist = clamp(dist - 0.96f, 0.0f, 0.04f) * 50.0f;
    dist = pow(dist, 2.0f);
    dist *= in.fog_factor;
    
    float progress = sin(float(in.curtime) * 0.0000001f);
    progress = (progress + 1.0) * 0.5;
    
    dist *= perlin_texture.sample(
        sampler,
        ((vector_float2(
            (float)progress * 1.1f,
            (float)progress * 0.7f)) * 0.2f) +
        texcoord / 8).r;
    
    color_sample =
        (vector_half4(
            in.fog_rgb[0], in.fog_rgb[1], in.fog_rgb[2], 1.0h) * dist) +
        (color_sample * (1.0h - dist));
    
    // reinhard tone mapping
    #if TONE_MAPPING_ACTIVE
    color_sample = color_sample / (color_sample + 0.30h);
    #endif
    color_sample = clamp(color_sample, 0.0h, 1.0h);
    
    #if COLOR_QUANTIZATION_ACTIVE
    if (in.color_quantization > 1.0h) {
        color_sample = floor(color_sample * in.color_quantization) /
            (in.color_quantization - 1.0h);
    }
    #endif
    
    color_sample[3] = 1.0h;
    
    return vector_half4(color_sample);
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
        camera->xyz[1],
        camera->xyz[2]);
    float3 camera_translated_pos = pos - camera_position;
    
    // rotate around camera
    float3 cam_z_rotated = xyz_rotate(
        camera_translated_pos,
        vector_float3(
            -camera->xyz_angle[0],
            -camera->xyz_angle[1],
            -camera->xyz_angle[2]));
    
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
