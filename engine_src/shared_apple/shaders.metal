#include <metal_stdlib>
#include <simd/simd.h>

#include "T1_stdint.h"
#include "T1_types_gpucpu.h"
#include "T1_types_public_gpucpu.h"

typedef uchar uint8_t;

using namespace metal;

float4 x_rotate(const float4 verts, f32 x_angle) {
    float4 rot_verts = verts;
    f32 cos_angle = cos(x_angle);
    f32 sin_angle = sin(x_angle);
    
    rot_verts[1] = verts[1] * cos_angle - verts[2] * sin_angle;
    rot_verts[2] = verts[1] * sin_angle + verts[2] * cos_angle;
    
    return rot_verts;
}

float4 y_rotate(const float4 verts, f32 y_angle) {
    float4 rot_verts = verts;
    f32 cos_angle = cos(y_angle);
    f32 sin_angle = sin(y_angle);
    
    rot_verts[0] = verts[0] * cos_angle + verts[2] * sin_angle;
    rot_verts[2] = verts[2] * cos_angle - verts[0] * sin_angle;
    
    return rot_verts;
}

float4 z_rotate(const float4 vertices, f32 z_angle) {
    float4 rotated_vertices = vertices;
    f32 cos_angle = cos(z_angle);
    f32 sin_angle = sin(z_angle);
    
    rotated_vertices[0] =
        (vertices[0] * cos_angle) -
        (vertices[1] * sin_angle);
    rotated_vertices[1] =
        (vertices[1] * cos_angle) +
        (vertices[0] * sin_angle);
    
    return rotated_vertices;
}

float4 xyz_rotate(const float4 vertices, const float4 xyz_angle) {
    float4 return_value = x_rotate(vertices,     xyz_angle[0]);
    return_value        = y_rotate(return_value, xyz_angle[1]);
    return_value        = z_rotate(return_value, xyz_angle[2]);
    
    return return_value;
}

typedef struct
{
    float4 projpos [[position]];
    float4 worldpos;
    float4 viewpos;
    float2 texcoord;
    // float4 worldpos;
    float4 normal_viewspace;
    float4 tangent_viewspace;
    float4 bitangent_viewspace;
    u32 locked_vertex_i [[ flat ]];
    u32 polygon_i [[ flat ]];
    s32 touchable_id [[ flat ]];
} RasterizerPixel;

f32 get_distance_f3(
    const float3 a,
    const float3 b)
{
    const float3 squared_diffs = (a-b)*(a-b);
    
    const f32 sum_squares = dot(
        squared_diffs,
        vector_float3(1.0f,1.0f,1.0f));
    
    return sqrt(sum_squares);
}

f32 get_distance(
    float4 a,
    float4 b)
{
    float4 squared_diffs = (a-b)*(a-b);
    
    f32 sum_squares = dot(
        squared_diffs,
        vector_float4(1.0f,1.0f,1.0f,1.0f));
    
    return sqrt(sum_squares);
}

vertex RasterizerPixel
vertex_shader(
    u32 vertex_i [[ vertex_id ]],
    const device T1GPUVertexIndices * vertices [[ buffer(0) ]],
    const device T1GPUzSprite * zsprites [[ buffer(1) ]],
    const device T1GPUzSpriteMatrices * matrices [[ buffer(2) ]],
    const device T1GPURenderView * rv [[ buffer(3) ]],
    constant u32 &rv_i [[buffer(4)]],
    const device T1GPULockedVertex * lverts [[ buffer(5) ]])
{
    RasterizerPixel out;
    
    out.polygon_i = vertices[vertex_i].polygon_i;
    out.locked_vertex_i = vertices[vertex_i].locked_vertex_i;
    
    const device T1GPULockedVertex * lv = &lverts[out.locked_vertex_i];
    const device T1GPUzSprite * zs = &zsprites[out.polygon_i];
    const device T1GPUzSpriteMatrices * m = &matrices[out.polygon_i];
    const device T1GPURenderView * c = rv + rv_i;
    
    if (
        c->write_to_shadow_maps &&
        zs->f32s.shadow_strength < 0.1f)
    {
        out.projpos = vector_float4(
            0.0f, 0.0f, 500.0f, 1.0f);
        return out;
    }
    
    float4 mesh_vertices = vector_float4(
        lv->xyz[0],
        lv->xyz[1],
        lv->xyz[2],
        1.0f);
    
    float3 vertex_normal = vector_float3(
        lv->norm_xyz[0],
        lv->norm_xyz[1],
        lv->norm_xyz[2]);
    
    #if T1_OUTLINES_ACTIVE == T1_ACTIVE
    float3 face_normal = vector_float3(
        lv->face_normal_xyz[0],
        lv->face_normal_xyz[1],
        lv->face_normal_xyz[2]);
    #elif T1_OUTLINES_ACTIVE == T1_INACTIVE
    #else
    #error
    #endif
    
    #if T1_NORMAL_MAPPING_ACTIVE == T1_ACTIVE
    float3 vertex_tangent = vector_float3(
        lv->tan_xyz[0],
        lv->tan_xyz[1],
        lv->tan_xyz[2]);
    
    float3 vertex_bitangent = vector_float3(
        lv->bitan_xyz[0],
        lv->bitan_xyz[1],
        lv->bitan_xyz[2]);
    #elif T1_NORMAL_MAPPING_ACTIVE == T1_INACTIVE
    #else
    #error
    #endif
    
    float4x4 model = matrix_float4x4(
        m->m_4x4[ 0], m->m_4x4[ 1], m->m_4x4[ 2], m->m_4x4[ 3],
        m->m_4x4[ 4], m->m_4x4[ 5], m->m_4x4[ 6], m->m_4x4[ 7],
        m->m_4x4[ 8], m->m_4x4[ 9], m->m_4x4[10], m->m_4x4[11],
        m->m_4x4[12], m->m_4x4[13], m->m_4x4[14], m->m_4x4[15]);
    
    out.worldpos = mesh_vertices * model;
    
    #if 0
    f32 dist = distance(
        mesh_vertices,
        vector_float4(0.0f, 0.0f, 0.0f, 1.0f));
    #endif
    
    float4x4 view = matrix_float4x4(
        c->v_4x4[ 0], c->v_4x4[ 1], c->v_4x4[ 2], c->v_4x4[ 3],
        c->v_4x4[ 4], c->v_4x4[ 5], c->v_4x4[ 6], c->v_4x4[ 7],
        c->v_4x4[ 8], c->v_4x4[ 9], c->v_4x4[10], c->v_4x4[11],
        c->v_4x4[12], c->v_4x4[13], c->v_4x4[14], c->v_4x4[15]);
    
    out.viewpos = out.worldpos * view;
    
    f32 ic = clamp(zs->f32s.no_cam, 0.0f, 1.0f);
    
    out.viewpos =
        (ic * out.worldpos) +
        (1.0f - ic) * out.viewpos;
    
    out.touchable_id = zs->s32s.touch_id;
    
    out.texcoord = vector_float2(lv->uv[0], lv->uv[1]);
    
    float4x4 projection = matrix_float4x4(
        c->p_4x4[ 0], c->p_4x4[ 1], c->p_4x4[ 2], c->p_4x4[ 3],
        c->p_4x4[ 4], c->p_4x4[ 5], c->p_4x4[ 6], c->p_4x4[ 7],
        c->p_4x4[ 8], c->p_4x4[ 9], c->p_4x4[10], c->p_4x4[11],
        c->p_4x4[12], c->p_4x4[13], c->p_4x4[14], c->p_4x4[15]
            );
    
    out.projpos = out.viewpos * projection;
    
    float3x3 normalmodel3x3 = matrix_float3x3(
        m->norm_3x3[0], m->norm_3x3[1], m->norm_3x3[2],
        m->norm_3x3[3], m->norm_3x3[4], m->norm_3x3[5],
        m->norm_3x3[6], m->norm_3x3[7], m->norm_3x3[8]);
    
    float3x3 normalview3x3 = matrix_float3x3(
        c->normv_3x3[0], c->normv_3x3[1], c->normv_3x3[2],
        c->normv_3x3[3], c->normv_3x3[4], c->normv_3x3[5],
        c->normv_3x3[6], c->normv_3x3[7], c->normv_3x3[8]);
    
    out.normal_viewspace = vector_float4(
        vertex_normal *
        normalmodel3x3 *
        normalview3x3,
        0.0f);
    #if T1_NORMAL_MAPPING_ACTIVE == T1_ACTIVE
    out.tangent_viewspace = vector_float4(
        vertex_tangent *
        normalmodel3x3 *
        normalview3x3,
        0.0f);
    out.bitangent_viewspace = vector_float4(
        vertex_bitangent *
        normalmodel3x3 *
        normalview3x3,
        0.0f);
    #elif T1_NORMAL_MAPPING_ACTIVE == T1_INACTIVE
    #else
    #error
    #endif
    
    return out;
}

fragment float4
z_prepass_fragment_shader(
    const RasterizerPixel in [[stage_in]],
    const device T1GPULockedVertex * locked_vertices [[ buffer(0) ]],
    const device T1GPUzSprite * polygons [[ buffer(1) ]],
    const device T1GPURenderView * camera [[ buffer(3) ]],
    constant u32 &camera_i [[buffer(4)]],
    const device T1GPUMatf32 * const_mats [[ buffer(6) ]],
    const device T1GPUPostProcConsts * updating_globals [[ buffer(7) ]])
{
    #if 0
    u32 mat_i =
        locked_vertices[in.locked_vertex_i].
            parent_material_i;
    
    const device T1GPUMat * material =
        mat_i == PARENT_MATERIAL_BASE ?
            &polygons[in.polygon_i].base_mat :
            &const_mats[locked_vertices
                [in.locked_vertex_i].
                    locked_materials_head_i + mat_i];
    #endif
    
    float4 lit_color = vector_float4(
        1.0f,
        1.0f,
        1.0f,
        1.0f);
    
    return lit_color;
}



struct FragmentAndTouchableOut {
    half4 color [[color(0)]];
    half4 touchable_id [[color(1)]];
};

/*
We want to output an s32 for 1 of our render targets (touchable_id), but
Metal enforces you use the same data type when you render to multiple targets
simultaneously. That's why we're packing our s32 inside of RGBA8Unorm slots. On
the CPU side we'll retrieve them and put them back together as an s32.
*/
static FragmentAndTouchableOut pack_color_and_touchable_id(
    float4 color,
    s32 touchable_id)
{
    FragmentAndTouchableOut out;
    
    out.color = (half4)color;
    
    u32 uid = as_type<u32>(touchable_id);
    
    out.touchable_id = half4(
        f16((uid      ) & 0xFFu) / 255.0h,
        f16((uid >>  8) & 0xFFu) / 255.0h,
        f16((uid >> 16) & 0xFFu) / 255.0h,
        f16((uid >> 24) & 0xFFu) / 255.0h
    );
    
    return out;
}

float4 worldspace_to_clipspace(
    float4 worldpos,
    const device T1GPURenderView * cam)
{
    float4x4 light_v = matrix_float4x4(
        cam->v_4x4[ 0],
        cam->v_4x4[ 1],
        cam->v_4x4[ 2],
        cam->v_4x4[ 3],
        cam->v_4x4[ 4],
        cam->v_4x4[ 5],
        cam->v_4x4[ 6],
        cam->v_4x4[ 7],
        cam->v_4x4[ 8],
        cam->v_4x4[ 9],
        cam->v_4x4[10],
        cam->v_4x4[11],
        cam->v_4x4[12],
        cam->v_4x4[13],
        cam->v_4x4[14],
        cam->v_4x4[15]);
    
    float4x4 light_p = matrix_float4x4(
        cam->p_4x4[ 0],
        cam->p_4x4[ 1],
        cam->p_4x4[ 2],
        cam->p_4x4[ 3],
        cam->p_4x4[ 4],
        cam->p_4x4[ 5],
        cam->p_4x4[ 6],
        cam->p_4x4[ 7],
        cam->p_4x4[ 8],
        cam->p_4x4[ 9],
        cam->p_4x4[10],
        cam->p_4x4[11],
        cam->p_4x4[12],
        cam->p_4x4[13],
        cam->p_4x4[14],
        cam->p_4x4[15]);
    
    return worldpos * light_v * light_p;
}

// Gets the color given a material and lighting setup
float4 get_lit(
    #if T1_SHADOWS_ACTIVE == T1_ACTIVE
    array<texture2d<f32>, T1_RENDER_VIEW_CAP> shadow_maps,
    #elif T1_SHADOWS_ACTIVE == T1_INACTIVE
    #else
    #error
    #endif
    array<texture2d_array<f16>, T1_TEXARRAYS_CAP> color_textures,
    const u32 cam_i,
    const device T1GPURenderView * render_views,
    const device T1GPULight * lights,
    const device T1GPUzSprite * zsprite,
    const device T1GPUMatf32 * matf32,
    const device T1GPUMats32 * mati32,
    const device T1GPUPostProcConsts * globals,
    const RasterizerPixel in,
    const bool is_base_mtl)
{
    #if T1_AMBIENT_LIGHTING_ACTIVE == T1_ACTIVE
    float4 lit_color = vector_float4(
        matf32->ambient_rgb[0],
        matf32->ambient_rgb[1],
        matf32->ambient_rgb[2],
        10.0f);
    lit_color *= 0.25f;
    #elif T1_AMBIENT_LIGHTING_ACTIVE == T1_INACTIVE
    float4 lit_color = vector_float4(
        0.0f, 0.0f, 0.0f, 1.0f);
    #else
    #error
    #endif
    
    float4 diffuse_base = vector_float4(
        matf32->diffuse_rgb[0],
        matf32->diffuse_rgb[1],
        matf32->diffuse_rgb[2],
        1.0f);
    
    #if T1_SPECULAR_LIGHTING_ACTIVE == T1_ACTIVE
    float4 specular_base = vector_float4(
        matf32->specular_rgb[0],
        matf32->specular_rgb[1],
        matf32->specular_rgb[2],
        1.0f);
    #elif T1_SPECULAR_LIGHTING_ACTIVE == T1_INACTIVE
    #else
    #error
    #endif
    
    float2 uv_orig =
        vector_float2(
            zsprite->f32s.base_mat_uv_offsets[0],
            zsprite->f32s.base_mat_uv_offsets[1]);
    
    float2 uv_adjusted =
        in.texcoord + (
        is_base_mtl *
        uv_orig);
    
    float2 uv_scroll = vector_float2(
        matf32->uv_scroll[0],
        matf32->uv_scroll[1]);
    
    uv_adjusted += globals->timestamp * 0.000001f * uv_scroll;
    
    uv_adjusted = fmod(uv_adjusted, 1.0f);
    
    constexpr sampler texture_sampler(
        mag_filter::linear,
        min_filter::linear
        #if T1_MIPMAPS_ACTIVE == T1_ACTIVE
        ,mip_filter::nearest
        #elif T1_MIPMAPS_ACTIVE == T1_INACTIVE
        #else
        #error
        #endif
        );
    
    float4 texture_base = vector_float4(
        1.0f, 1.0f, 1.0f, 1.0f);
    ushort tex = (mati32->normalmap_tex_and_tex & 0x0000FFFF);
    #if T1_TEXTURES_ACTIVE == T1_ACTIVE 
    if (tex != T1_TEX_NONE)
    {
        int texarray_i = tex >> 11;
        int texslice_i = tex & 0x07FF;
    #elif T1_TEXTURES_ACTIVE == T1_INACTIVE
    if (tex >> 11 == 0)
    {
        int texarray_i = tex >> 11;
        int texslice_i = tex & 0x07FF;
    #else
    #error
    #endif
        // Sample the texture to obtain a color
        #if T1_LOG_ASSERTS_ACTIVE == T1_ACTIVE
        if (texarray_i > 30) {
            return float4(1.0f, 1.0f, 1.0f, 1.0f);
        }
        #elif T1_LOG_ASSERST_ACTIVE == T1_INACTIVE
        #else
        #error
        #endif
        
        const half4 color_sample =
            color_textures[texarray_i].sample(
                texture_sampler,
                uv_adjusted,
                texslice_i);
        texture_base = float4(color_sample);
    }
    
    #if T1_REFLECTION_ACTIVE == T1_ACTIVE
    int mix_rv_i = zsprite->s32s.mix_rv_and_mix_tex >> 16;
    int mix_array_i =
        (zsprite->s32s.mix_rv_and_mix_tex & 0x0000F800) >> 11;
    int mix_slice_i = zsprite->s32s.mix_rv_and_mix_tex & 0x000007FF;
    if (
        mix_rv_i >= 0 &&
        (zsprite->s32s.mix_rv_and_mix_tex & 0x0000FFFF) != T1_TEX_NONE &&
        mix_array_i >= 0 &&
        mix_slice_i >= 0)
    {
        const device T1GPURenderView * rfv = &render_views[mix_rv_i];
        
        if (
            mix_rv_i < 0 ||
            mix_array_i < 0 ||
            mix_slice_i < 0)
        {
            return vector_float4(0.0f, 1.0f, 0.0f, 1.0f);
        }
        
        float4 refl_clipspace =
            worldspace_to_clipspace(
                in.worldpos,
                rfv);
        
        float2 refl_uv =
                ((refl_clipspace.xy /
                    refl_clipspace.w) * 0.5f) +
                        0.5f;
        
        refl_uv = clamp(refl_uv, 0.0f, 1.0f);
        
        refl_uv[1] = 1.0 - refl_uv[1];
        
        f32 mix_strength = 0.95f;
        
        f32 fade_x = refl_uv.x * (1.0f - refl_uv.x) * 8.0f;
        f32 fade_y = refl_uv.y * (1.0f - refl_uv.y) * 8.0f;
        f32 fade = clamp(min(fade_x, fade_y), 0.0f, 1.0f);
        mix_strength *= fade;
        
        const half4 color_sample =
            color_textures[mix_array_i].
                sample(
                    texture_sampler,
                    refl_uv,
                    mix_slice_i);
        
        texture_base =
            (texture_base * (1.0h - mix_strength)) +
            (float4(color_sample) * mix_strength);
    }
    #elif T1_REFLECTION_ACTIVE == T1_INACTIVE
    #else
    #error
    #endif
    
    float4 no_lighting_color = diffuse_base * texture_base;
    
    for (
        u32 i = 0;
        i < globals->lights_size;
        i++)
    {
        float4 light_color = vector_float4(
            lights[i].rgb[0],
            lights[i].rgb[1],
            lights[i].rgb[2],
            1.0f);
        
        float4 shadow_factors = vector_float4(
            1.0f, 1.0f, 1.0f, 1.0f);
        
        #if T1_SHADOWS_ACTIVE == T1_ACTIVE
        if (
            lights[i].shadow_map_render_view_i >= 0 &&
            lights[i].shadow_map_depth_tex_i >= 0)
        {
            float4 light_clip_pos =
                worldspace_to_clipspace(
                    in.worldpos,
                    &render_views[
                        lights[i].shadow_map_render_view_i]);
            
            float2 shadow_uv =
                ((light_clip_pos.xy /
                    light_clip_pos.w) * 0.5f) + 0.5f;
            
            shadow_uv[1] = 1.0f - shadow_uv[1];
            
            constexpr sampler shadow_sampler(
                mag_filter::nearest,
                min_filter::nearest,
                address::clamp_to_edge);
            
            int shadowmap_i = lights[i].shadow_map_depth_tex_i;
            
            f32 shadow_depth = shadow_maps[shadowmap_i].sample(shadow_sampler,shadow_uv).r;
            
            f32 frag_depth =
                light_clip_pos.z / light_clip_pos.w;
            
            shadow_factors =
                (frag_depth <= shadow_depth + SHADOW_BIAS) ?
                1.0f :
                vector_float4(
                    globals->in_shadow_multipliers[0],
                    globals->in_shadow_multipliers[1],
                    globals->in_shadow_multipliers[2],
                    1.0f);
        }
        #elif T1_SHADOWS_ACTIVE == T1_INACTIVE
        #else
        #error
        #endif
        
        float4 light_pos_world = vector_float4(
            lights[i].xyz[0],
            lights[i].xyz[1],
            lights[i].xyz[2],
            1.0f);
        
        float4x4 cam_v_4x4 = matrix_float4x4(
            render_views[cam_i].v_4x4[ 0],
            render_views[cam_i].v_4x4[ 1],
            render_views[cam_i].v_4x4[ 2],
            render_views[cam_i].v_4x4[ 3],
            render_views[cam_i].v_4x4[ 4],
            render_views[cam_i].v_4x4[ 5],
            render_views[cam_i].v_4x4[ 6],
            render_views[cam_i].v_4x4[ 7],
            render_views[cam_i].v_4x4[ 8],
            render_views[cam_i].v_4x4[ 9],
            render_views[cam_i].v_4x4[10],
            render_views[cam_i].v_4x4[11],
            render_views[cam_i].v_4x4[12],
            render_views[cam_i].v_4x4[13],
            render_views[cam_i].v_4x4[14],
            render_views[cam_i].v_4x4[15]);
        
        float4 light_viewspace = light_pos_world * cam_v_4x4;
        
        f32 distance = get_distance_f3(
            (float3)light_viewspace,
            (float3)in.viewpos);
        f32 distance_overflow = max(
            distance - (lights[i].reach * 0.75f),
            0.0f);
        f32 attenuation = 1.0f - (
            distance_overflow / lights[i].reach);
        attenuation = clamp(attenuation, 0.00f, 1.00f);
        
        float3 normal_viewspace = normalize(
            vector_float3(
                in.normal_viewspace[0],
                in.normal_viewspace[1],
                in.normal_viewspace[2]));
        
        float3 object_to_light_viewspace = normalize(
            (float3)light_viewspace -
            (float3)(in.viewpos));
        
        #if T1_NORMAL_MAPPING_ACTIVE == T1_ACTIVE
        // TODO: normal mapping in viewspace
        #if 0
        if (material->normalmap_texturearray_i >= 0) {
            half4 normal_map_sample =
                color_textures[material->normalmap_texturearray_i].sample(
                    texture_sampler,
                    uv_adjusted,
                    material->normalmap_texture_i);
            
            float4 normal_map_sample_f4 = vector_float4(
                normal_map_sample[0],
                normal_map_sample[1],
                normal_map_sample[2],
                0.0f);
            
            normal_map_sample_f4 = (normal_map_sample_f4 * 2.0f) - 1.0f;
            
            float4 normal_viewspace_f4 =
                normal_map_sample[0] *
                    in.tangent_viewspace +
                normal_map_sample[1] *
                    in.bitangent_viewspace +
                normal_map_sample[2] * in.normal_viewspace;
            normal_viewspace = (vector_float3)normal_viewspace_f4;
            
            // normalize again
            normal_viewspace = normalize(
                normal_viewspace);
        }
        #endif
        #elif T1_NORMAL_MAPPING_ACTIVE == T1_INACTIVE
        #else
        #error
        #endif
        
        #if T1_DIFFUSE_LIGHTING_ACTIVE == T1_ACTIVE
        f32 diffuse_dot = max(
            dot(
                normal_viewspace,
                object_to_light_viewspace),
            0.0f) * 0.85f + 0.15f;
        
        float4 light_diffuse_multiplier =
             attenuation *
             light_color *
             diffuse_dot *
             lights[i].diffuse *
             shadow_factors;
        
        lit_color += (diffuse_base * light_diffuse_multiplier);
        #elif T1_DIFFUSE_LIGHTING_ACTIVE == T1_INACTIVE
        #else
        #error
        #endif
        
        #if T1_SPECULAR_LIGHTING_ACTIVE == T1_ACTIVE
        // specular lighting
        float3 fragment_to_cam_viewspace =
            normalize(-(float3)in.viewpos);
        
        float3 half_lightdir_half_view =
            normalize(
                object_to_light_viewspace +
                fragment_to_cam_viewspace);
        
        f32 specular_dot = pow(
            max(
                dot(
                    normal_viewspace,
                    half_lightdir_half_view),
                0.0),
            matf32->specular_exponent);
        
        lit_color += (
            shadow_factors *
            specular_base *
            attenuation *
            light_color *
            lights[i].specular *
            specular_dot);
        #elif T1_SPECULAR_LIGHTING_ACTIVE == T1_INACTIVE
        #else
        #error
        #endif
    }
    
    if (globals->lights_size == 0) {
        lit_color[0] = 0.0f;
        lit_color[1] = 1.0f;
        lit_color[2] = 1.0f;
        lit_color[3] = 1.0f;
    }
    
    lit_color *= texture_base;
    
    f32 no_lighting = clamp(zsprite->f32s.no_light, 0.0f, 1.0f);
    lit_color =
        ((1.0f - no_lighting) * lit_color) +
        (no_lighting * no_lighting_color);
    
    lit_color[3] *= zsprite->f32s.alpha * matf32->alpha;
    
    lit_color += vector_float4(
        zsprite->f32s.bonus_rgb[0],
        zsprite->f32s.bonus_rgb[1],
        zsprite->f32s.bonus_rgb[2],
        0.0f);
    
    lit_color = clamp(lit_color, 0.1f, 1.0f);
    
    return lit_color;
}

fragment FragmentAndTouchableOut
fragment_shader(
    const RasterizerPixel in [[stage_in]],
    array<texture2d_array<f16>, T1_TEXARRAYS_CAP>
        color_textures[[ texture(0) ]],
    #if T1_SHADOWS_ACTIVE == T1_ACTIVE
    array<texture2d<f32>, T1_RENDER_VIEW_CAP> shadow_map [[texture(T1_SHADOW_MAPS_1ST_FRAGARG_I)]],
    #elif T1_SHADOWS_ACTIVE == T1_INACTIVE
    #else
    #error
    #endif
    const device T1GPULockedVertex *
        locked_vertices [[ buffer(0) ]],
    const device T1GPUzSprite *
        polygons [[ buffer(1) ]],
    const device T1GPULight *
        lights [[ buffer(2) ]],
    const device T1GPURenderView *
        render_views [[ buffer(3) ]],
    constant u32 &camera_i [[buffer(4)]],
    const device T1GPUMatf32 * const_mats_f32 [[ buffer(6) ]],
    const device T1GPUMats32 * const_mats_i32 [[ buffer(8) ]],
    const device T1GPUPostProcConsts * updating_globals [[ buffer(7) ]])
{
    if (
        polygons[in.polygon_i].f32s.no_cam < 0.05f &&
        (
            in.worldpos.xyz[2] >=
                render_views[camera_i].cull_above_z ||
            in.worldpos.xyz[2] <=
                render_views[camera_i].cull_below_z))
    {
        discard_fragment();
    }
    
    u32 mat_i =
        locked_vertices[in.locked_vertex_i].parent_material_i;
    const device T1GPUMatf32 * matf32 =
        mat_i == PARENT_MATERIAL_BASE ?
            &polygons[in.polygon_i].base_mat_f32 :
            &const_mats_f32[locked_vertices[in.locked_vertex_i].
                locked_materials_head_i + mat_i];
    const device T1GPUMats32 * mati32 =
        mat_i == PARENT_MATERIAL_BASE ?
            &polygons[in.polygon_i].base_mat_s32 :
            &const_mats_i32[locked_vertices[in.locked_vertex_i].
                locked_materials_head_i + mat_i];
    
    float4 lit_color = get_lit(
        #if T1_SHADOWS_ACTIVE == T1_ACTIVE
            shadow_map,
        #elif T1_SHADOWS_ACTIVE == T1_INACTIVE
        #else
        #error
        #endif
            color_textures,
            camera_i,
        /* const device T1GPURenderView * rvs: */
            render_views,
        /* const device GPULight * lights: */
            lights,
        /* const device zSprite * zsprite: */
            &polygons[in.polygon_i],
        /* const device GPUPolygonMaterialf32 * fragment_material: */
            matf32,
            mati32,
        /* const device GPUPostProcessingConstants * updating_globals: */
            updating_globals,
        /* const device RasterizerPixel * in: */
            in,
        /* const bool is_base_mtl: */
            mat_i == PARENT_MATERIAL_BASE);
    
    s32 diamond_size = 35.0f;
    s32 neghalfdiamond = -1.0f * (diamond_size / 2);
    
    s32 alpha_tresh = (s32)(lit_color[3] * diamond_size);
    
    if (
        lit_color[3] < 0.05f ||
        (
            lit_color[3] < 0.95f &&
            (
                abs((neghalfdiamond + (s32)in.projpos.x % diamond_size)) +
                abs((neghalfdiamond + (s32)in.projpos.y % diamond_size))
            ) > alpha_tresh
        ))
    {
        discard_fragment();
    }
    
    lit_color[3] = clamp(lit_color[3], 0.0f, 1.0f);
    
    FragmentAndTouchableOut packed_out =
        pack_color_and_touchable_id(lit_color, in.touchable_id);
    
    return packed_out;
}

fragment FragmentAndTouchableOut
alphablending_fragment_shader(
    RasterizerPixel in [[stage_in]],
    array<texture2d_array<f16>, T1_TEXARRAYS_CAP>
        color_textures[[ texture(0), maybe_unused ]],
    #if T1_SHADOWS_ACTIVE == T1_ACTIVE
    array<texture2d<f32>, T1_RENDER_VIEW_CAP>
        shadow_maps[[ texture(T1_SHADOW_MAPS_1ST_FRAGARG_I), maybe_unused ]],
    #elif T1_SHADOWS_ACTIVE == T1_INACTIVE
    #else
    #error
    #endif
    const device T1GPULockedVertex * locked_vertices [[ buffer(0) ]],
    const device T1GPUzSprite * polygons [[ buffer(1) ]],
    const device T1GPULight * lights [[ buffer(2) ]],
    const device T1GPURenderView * render_views [[ buffer(3) ]],
    constant u32 &camera_i [[buffer(4)]],
    const device T1GPUMatf32 * locked_mats_f32 [[ buffer(6) ]],
    const device T1GPUMats32 * locked_mats_i32 [[ buffer(8) ]],
    const device T1GPUPostProcConsts * updating_globals [[ buffer(7) ]])
{
    if (
        polygons[in.polygon_i].f32s.no_cam < 0.05f &&
        (
            in.worldpos.xyz[2] >=
                render_views[camera_i].cull_above_z ||
            in.worldpos.xyz[2] <=
                render_views[camera_i].cull_below_z))
    {
        discard_fragment();
    }
    
    u32 mat_i = locked_vertices[in.locked_vertex_i].
        parent_material_i;
    
    const device T1GPUMatf32 * matf32 =
        mat_i == PARENT_MATERIAL_BASE ?
            &polygons[in.polygon_i].base_mat_f32 :
            &locked_mats_f32[locked_vertices[in.locked_vertex_i].
                locked_materials_head_i + mat_i];
    const device T1GPUMats32 * mati32 =
        mat_i == PARENT_MATERIAL_BASE ?
            &polygons[in.polygon_i].base_mat_s32 :
            &locked_mats_i32[locked_vertices[in.locked_vertex_i].locked_materials_head_i + mat_i];
    
    float4 lit_color = get_lit(
        #if T1_SHADOWS_ACTIVE == T1_ACTIVE
        /* texture2d<f32> shadow_map: */
            shadow_maps,
        #elif T1_SHADOWS_ACTIVE == T1_INACTIVE
        #else
        #error
        #endif
            color_textures,
            camera_i,
        /* const device T1GPURenderView * rvs: */
            render_views,
        /* const device GPULight * lights: */
            lights,
        /* const device GPUzSprite zsprite: */
            &polygons[in.polygon_i],
        /* const device GPUPolygonMaterial * fragment_material: */
            matf32,
            mati32,
        /* const device GPUPostProcessingConstants * updating_globals: */
            updating_globals,
        /* const RasterizerPixel in: */
            in,
        /* const bool is_base_mtl: */
            mat_i == PARENT_MATERIAL_BASE);
    
    if (lit_color[3] < 0.05f)
    {
        discard_fragment();
    }
    
    lit_color[3] = clamp(lit_color[3], 0.0f, 1.0f);
    return pack_color_and_touchable_id(lit_color, in.touchable_id);
}

// TODO: just pass this directly to fragment shader
struct PostProcessingFragment
{
    float4 position [[position]];
    half4 bonus_rgb;
    half4 fog_rgb;
    float2 texcoord;
    f32 blur_pct;
    f32 nonblur_pct;
    f32 color_quantization;
    f32 fog_factor;
    u32 curtime;
};

vertex PostProcessingFragment
single_quad_vertex_shader(
    const u32 vertexID [[ vertex_id ]],
    const device T1PostProcessingVertex * vertices [[ buffer(0) ]],
    const constant T1GPUPostProcConsts * constants [[ buffer(1) ]])
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
    
    out.curtime = constants->timestamp;
    
    out.blur_pct = constants->blur_pct;
    
    out.nonblur_pct = constants->nonblur_pct;
    
    out.bonus_rgb = vector_half4(
        constants->rgb_add[0],
        constants->rgb_add[1],
        constants->rgb_add[2],
        0.0h);
    
    out.color_quantization = constants->color_quantization;
    
    #if T1_FOG_ACTIVE == T1_ACTIVE
    out.fog_factor = constants->fog_factor;
    out.fog_rgb = vector_half4(
        constants->fog_color[0],
        constants->fog_color[1],
        constants->fog_color[2],
        1.0h);
    #elif T1_FOG_ACTIVE == T1_INACTIVE
    #else
    #error
    #endif
    
    return out;
}

fragment half4
single_quad_fragment_shader(
    PostProcessingFragment in [[stage_in]],
    texture2d<f16> texture  [[texture(0)]],
    #if T1_BLOOM_ACTIVE == T1_ACTIVE
    texture2d<f16> downsampled_1  [[texture(1)]],
    texture2d<f16> downsampled_2  [[texture(2)]],
    texture2d<f16> downsampled_3  [[texture(3)]],
    texture2d<f16> downsampled_4  [[texture(4)]],
    texture2d<f16> downsampled_5  [[texture(5)]],
    #elif T1_BLOOM_ACTIVE == T1_INACTIVE
    #else
    #error
    #endif
    #if T1_FOG_ACTIVE == T1_ACTIVE
    texture2d_array<f16> perlin_texture [[ texture(6) ]],
    #elif T1_FOG_ACTIVE == T1_INACTIVE
    #else
    #error
    #endif
    depth2d<f32> camera_depth_map
        [[texture(T1_CAM_DEPTH_FRAGARG_I)]])
{
    constexpr sampler sampler(
        s_address::repeat,
        t_address::repeat,
        mag_filter::linear,
        min_filter::linear);
    
    float2 texcoord = in.texcoord;
    
    half4 color_sample = texture. sample(
        sampler, texcoord);
    
    #if T1_BLOOM_ACTIVE == T1_ACTIVE
    color_sample += downsampled_1.sample(
        sampler, texcoord);
    color_sample += downsampled_2.sample(
        sampler, texcoord);
    color_sample += downsampled_3.sample(
        sampler, texcoord);
    color_sample += downsampled_4.sample(
        sampler, texcoord);
    color_sample += downsampled_5.sample(
        sampler, texcoord);
    #elif T1_BLOOM_ACTIVE == T1_INACTIVE
    #else
    #error
    #endif
    
    color_sample[3] = 1.0f;
    color_sample += clamp(in.bonus_rgb, 0.0h, 0.25h);
    
    #if T1_FOG_ACTIVE == T1_ACTIVE
    f32 dist = camera_depth_map.sample(sampler, texcoord);
    dist = clamp(dist - 0.96f, 0.0f, 0.04f) * 50.0f;
    dist = pow(dist, 2.0f);
    dist *= in.fog_factor;
    
    f32 progress = sin(float(in.curtime) * 0.0000001f);
    progress = (progress + 1.0f) * 0.5f;
    
    dist *= perlin_texture.sample(
        sampler,
        ((vector_float2(
            (float)progress * 1.1f,
            (float)progress * 0.7f)) * 0.2f) +
        texcoord / 8,
        /* texture index: */ 0).r;
    
    color_sample =
        (vector_half4(
            in.fog_rgb[0],
            in.fog_rgb[1],
            in.fog_rgb[2],
            1.0h) * dist) +
        (color_sample * (1.0h - dist));
    #elif T1_FOG_ACTIVE == T1_INACTIVE
    #else
    #error
    #endif
    
    // reinhard tone mapping
    #if T1_TONE_MAPPING_ACTIVE == T1_ACTIVE
    color_sample = clamp(color_sample, 0.0h, 1.10h);
    color_sample = color_sample / 1.10h;
    #elif T1_TONE_MAPPING_ACTIVE == T1_INACTIVE
    #else
    #error
    #endif
    color_sample = clamp(color_sample, 0.0h, 1.0h);
    
    #if T1_COLOR_QUANTIZATION_ACTIVE == T1_ACTIVE
    if (in.color_quantization > 1.0h) {
        color_sample = floor(
            color_sample * in.color_quantization) /
            (in.color_quantization - 1.0h);
    }
    #elif T1_COLOR_QUANTIZATION_ACTIVE == T1_INACTIVE
    #else
    #error
    #endif
    
    color_sample[3] = 1.0h;
    
    return vector_half4(color_sample);
}

kernel void threshold_texture(
    texture2d<f16, access::read_write>
        texture[[texture(0)]],
    uint2 grid_pos [[thread_position_in_grid]])
{
    half4 in_color = texture.read(grid_pos);
    
    half4 thresholded =
        in_color *
        (in_color[3] > 1.0f);
    
    texture.write(thresholded, grid_pos);
}

kernel void downsample_texture(
    texture2d<f16, access::read> in_texture[[texture(0)]],
    texture2d<f16, access::write> out_texture[[texture(1)]],
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
        in_texture.read(
            input_pos + vector_uint2(1, 1)) +
        in_texture.read(
            input_pos + vector_uint2(1, 0)) +
        in_texture.read(
            input_pos + vector_uint2(0, 1))) / 4.0h;
    
    out_texture.write(in_color, out_pos);
}

kernel void boxblur_texture(
    texture2d<f16, access::read_write> texture[[texture(0)]],
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

typedef struct
{
    float4 projpos [[ position ]];
    float4 rgba [[ flat ]];
} FlatQuadPixel;

vertex FlatQuadPixel
flat_billboard_quad_vertex_shader(
    u32 vertex_i [[ vertex_id ]],
    const device T1GPUFlatQuad * quads [[ buffer(2) ]],
    const device T1GPURenderView * camera [[ buffer(3) ]])
{
    u32 quad_i = vertex_i / 6;
    u32 corner_id  = vertex_i % 6;
    
    f32 halfsize = quads[quad_i].size * 0.5f;
    
    constexpr const float2 corners[6] = {
        float2(0.0f, 0.0f),
        float2(1.0f, 0.0f),
        float2(1.0f, 1.0f),
        float2(0.0f, 0.0f),
        float2(1.0f, 1.0f),
        float2(0.0f, 1.0f)
    };
    
    FlatQuadPixel out;
    
    float4 worldpos = vector_float4(
        quads[quad_i].xyz[0],
        quads[quad_i].xyz[1],
        quads[quad_i].xyz[2],
        1.0f);
    
    float4x4 view = matrix_float4x4(
        camera->v_4x4[ 0],
        camera->v_4x4[ 1],
        camera->v_4x4[ 2],
        camera->v_4x4[ 3],
        camera->v_4x4[ 4],
        camera->v_4x4[ 5],
        camera->v_4x4[ 6],
        camera->v_4x4[ 7],
        camera->v_4x4[ 8],
        camera->v_4x4[ 9],
        camera->v_4x4[10],
        camera->v_4x4[11],
        camera->v_4x4[12],
        camera->v_4x4[13],
        camera->v_4x4[14],
        camera->v_4x4[15]);
    
    float4x4 projection = matrix_float4x4(
        camera->p_4x4[ 0],
        camera->p_4x4[ 1],
        camera->p_4x4[ 2],
        camera->p_4x4[ 3],
        camera->p_4x4[ 4],
        camera->p_4x4[ 5],
        camera->p_4x4[ 6],
        camera->p_4x4[ 7],
        camera->p_4x4[ 8],
        camera->p_4x4[ 9],
        camera->p_4x4[10],
        camera->p_4x4[11],
        camera->p_4x4[12],
        camera->p_4x4[13],
        camera->p_4x4[14],
        camera->p_4x4[15]);
    
    out.projpos = worldpos * view * projection;
    
    out.projpos.x += (corners[corner_id].x * halfsize) / worldpos.w;
    out.projpos.y += (corners[corner_id].y * halfsize) / worldpos.w;
    
    out.rgba = vector_float4(
        quads[quad_i].rgba[0],
        quads[quad_i].rgba[1],
        quads[quad_i].rgba[2],
        quads[quad_i].rgba[3]);
    
    return out;
}

fragment float4 flat_billboard_quad_fragment_shader(
    const FlatQuadPixel in [[stage_in]])
{
    return in.rgba * in.rgba[3];
}

typedef struct
{
    float4 screenpos [[ position ]];
    float4 rgba [[ flat ]];
    float2 uv;
    s32 array_i [[ flat ]];
    s32 slice_i [[ flat ]];
    s32 touch_id [[ flat ]];
} FlatTexQuadPixel;

vertex FlatTexQuadPixel
flat_texquad_vertex_shader(
    u32 vertex_i [[ vertex_id ]],
    const device T1GPUTexQuad * quads [[ buffer(0) ]],
    const device T1GPUzSpriteMatrices * mats [[ buffer(2) ]],
    const device T1GPURenderView * cam [[ buffer(3) ]])
{
    u32 quad_i = vertex_i / 6;
    u32 corner_id  = vertex_i % 6;
    
    float2 size_xy = vector_float2(
        quads[quad_i].f32s.wh[0],
        quads[quad_i].f32s.wh[1]);
    
    constexpr const float2 corners[6] = {
        float2(-0.5f, -0.5f),
        float2( 0.5f, -0.5f),
        float2( 0.5f,  0.5f),
        float2(-0.5f, -0.5f),
        float2( 0.5f,  0.5f),
        float2(-0.5f,  0.5f)
    };
    
    constexpr float2 uvs[6] = {
        float2(0.0f, 1.0f),
        float2(1.0f, 1.0f),
        float2(1.0f, 0.0f),
        float2(0.0f, 1.0f),
        float2(1.0f, 0.0f),
        float2(0.0f, 0.0f),
    };
    
    float4 worldpos = vector_float4(
        quads[quad_i].f32s.xyz[0] +
            quads[quad_i].f32s.offset_xy[0],
        quads[quad_i].f32s.xyz[1] +
            quads[quad_i].f32s.offset_xy[1],
        quads[quad_i].f32s.xyz[2],
        1.0f);
    
    FlatTexQuadPixel out;
    
    out.screenpos = worldpos; // * view * projection;
    
    out.screenpos.xy += (corners[corner_id].xy * size_xy);
    
    out.rgba = vector_float4(
        quads[quad_i].f32s.rgba[0],
        quads[quad_i].f32s.rgba[1],
        quads[quad_i].f32s.rgba[2],
        quads[quad_i].f32s.rgba[3]);
    
    out.uv = uvs[corner_id];
    
    ushort tex = quads[quad_i].s32s.reserved_and_tex & 0x0000FFFF;
    
    if (tex == T1_TEX_NONE) {
        out.array_i = -1;
        out.slice_i = -1;
    } else {
        out.array_i = tex >> 11;
        out.slice_i = tex & 0x07FF;    
    }
    
    out.touch_id = quads[quad_i].s32s.touch_id;
    
    return out;
}

fragment FragmentAndTouchableOut flat_texquad_fragment_shader(
    array<texture2d_array<f16>, T1_TEXARRAYS_CAP> color_textures,
    const FlatTexQuadPixel in [[stage_in]])
{
    constexpr sampler texture_sampler(
        mag_filter::linear,
        min_filter::linear);
    
    float4 color_sample = vector_float4(1.0f, 1.0f, 1.0f, 1.0f);
    if (
        in.array_i >= 0 &&
        in.array_i < T1_TEXARRAYS_CAP)
    {
        color_sample = float4(
            color_textures[in.array_i].
                sample(
                    texture_sampler,
                    in.uv,
                    in.slice_i));
    }
    
    if (color_sample[3] < 0.03f) { discard_fragment(); }
    
    FragmentAndTouchableOut packed_out =
        pack_color_and_touchable_id(
            color_sample * in.rgba,
            in.touch_id);
    
    return packed_out;
}

#if T1_OUTLINES_ACTIVE == T1_ACTIVE
typedef struct
{
    float4 pos [[position]];
    f32    outline_alpha [[ flat ]];
} OutlinePixel;

vertex OutlinePixel
outlines_vertex_shader(
    u32 vertex_i [[ vertex_id ]],
    const device T1GPUVertexIndices * vertices [[ buffer(0) ]],
    const device T1GPUzSprite * polygons [[ buffer(1) ]],
    const device T1GPUzSpriteMatrices * matrices [[ buffer(2) ]],
    const device T1GPURenderView * camera [[ buffer(3) ]],
    constant u32 &rv_i [[buffer(4)]],
    const device T1GPULockedVertex * lverts [[ buffer(5)]])
{
    OutlinePixel out;
    
    u32 polygon_i = vertices[vertex_i].polygon_i;
    u32 locked_vertex_i =
        vertices[vertex_i].locked_vertex_i;
    
    float4 vert = vector_float4(
        lverts[locked_vertex_i].xyz[0],
        lverts[locked_vertex_i].xyz[1],
        lverts[locked_vertex_i].xyz[2],
        1.0f);
    
    float3 normal = vector_float3(
        lverts[locked_vertex_i].face_normal_xyz[0],
        lverts[locked_vertex_i].face_normal_xyz[1],
        lverts[locked_vertex_i].face_normal_xyz[2]);
    
    out.outline_alpha = polygons[polygon_i].f32s.outline_alpha;
    
    float4x4 m_4x4 = matrix_float4x4(
        matrices[polygon_i].m_4x4[ 0],
        matrices[polygon_i].m_4x4[ 1],
        matrices[polygon_i].m_4x4[ 2],
        matrices[polygon_i].m_4x4[ 3],
        matrices[polygon_i].m_4x4[ 4],
        matrices[polygon_i].m_4x4[ 5],
        matrices[polygon_i].m_4x4[ 6],
        matrices[polygon_i].m_4x4[ 7],
        matrices[polygon_i].m_4x4[ 8],
        matrices[polygon_i].m_4x4[ 9],
        matrices[polygon_i].m_4x4[10],
        matrices[polygon_i].m_4x4[11],
        matrices[polygon_i].m_4x4[12],
        matrices[polygon_i].m_4x4[13],
        matrices[polygon_i].m_4x4[14],
        matrices[polygon_i].m_4x4[15]);
    
    float4x4 v_4x4 = matrix_float4x4(
        camera->v_4x4[ 0],
        camera->v_4x4[ 1],
        camera->v_4x4[ 2],
        camera->v_4x4[ 3],
        camera->v_4x4[ 4],
        camera->v_4x4[ 5],
        camera->v_4x4[ 6],
        camera->v_4x4[ 7],
        camera->v_4x4[ 8],
        camera->v_4x4[ 9],
        camera->v_4x4[10],
        camera->v_4x4[11],
        camera->v_4x4[12],
        camera->v_4x4[13],
        camera->v_4x4[14],
        camera->v_4x4[15]);
    
    float4x4 p_4x4 = matrix_float4x4(
        camera->p_4x4[ 0],
        camera->p_4x4[ 1],
        camera->p_4x4[ 2],
        camera->p_4x4[ 3],
        camera->p_4x4[ 4],
        camera->p_4x4[ 5],
        camera->p_4x4[ 6],
        camera->p_4x4[ 7],
        camera->p_4x4[ 8],
        camera->p_4x4[ 9],
        camera->p_4x4[10],
        camera->p_4x4[11],
        camera->p_4x4[12],
        camera->p_4x4[13],
        camera->p_4x4[14],
        camera->p_4x4[15]);
    
    out.pos = vert * m_4x4 * v_4x4;
    
    float3x3 n_3x3 = matrix_float3x3(
        matrices[polygon_i].norm_3x3[ 0],
        matrices[polygon_i].norm_3x3[ 1],
        matrices[polygon_i].norm_3x3[ 2],
        matrices[polygon_i].norm_3x3[ 3],
        matrices[polygon_i].norm_3x3[ 4],
        matrices[polygon_i].norm_3x3[ 5],
        matrices[polygon_i].norm_3x3[ 6],
        matrices[polygon_i].norm_3x3[ 7],
        matrices[polygon_i].norm_3x3[ 8]);
    
    normal = normalize(normal * n_3x3);
    
    out.pos -= vector_float4(normal, 0.0f) * 0.002f;
    
    out.pos *= p_4x4;
    
    return out;
}

fragment float4 outlines_fragment_shader(
    const OutlinePixel in [[stage_in]])
{
    if (in.outline_alpha < 0.0f) {
        discard_fragment();
    }
    
    float4 ret = vector_float4(
        0.7f, 0.03f, 1.00f, 1.0f);
    
    return ret * in.outline_alpha;
}
#endif
