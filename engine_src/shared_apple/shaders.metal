#include <metal_stdlib>
#include <simd/simd.h>

#include "T1_cpu_gpu_shared_types.h"
#include "clientlogic_macro_settings.h"

using namespace metal;

float4 x_rotate(const float4 vertices, const float x_angle) {
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

float4 y_rotate(const float4 vertices, const float y_angle) {
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

float4 z_rotate(const float4 vertices, const float z_angle) {
    float4 rotated_vertices = vertices;
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
    float2 texture_coordinate;
    // float4 worldpos;
    float4 normal_viewspace;
    float4 tangent_viewspace;
    float4 bitangent_viewspace;
    unsigned int locked_vertex_i [[ flat ]];
    unsigned int polygon_i [[ flat ]];
    int32_t touchable_id [[ flat ]];
} RasterizerPixel;

float get_distance_f3(
    const float3 a,
    const float3 b)
{
    const float3 squared_diffs = (a-b)*(a-b);
    
    const float sum_squares = dot(
        squared_diffs,
        vector_float3(1.0f,1.0f,1.0f));
    
    return sqrt(sum_squares);
}

float get_distance(
    float4 a,
    float4 b)
{
    float4 squared_diffs = (a-b)*(a-b);
    
    float sum_squares = dot(
        squared_diffs,
        vector_float4(1.0f,1.0f,1.0f,1.0f));
    
    return sqrt(sum_squares);
}

vertex RasterizerPixel
vertex_shader(
    uint vertex_i [[ vertex_id ]],
    const device T1GPUVertexIndices * vertices [[ buffer(0) ]],
    const device T1GPUzSprite * zsprites [[ buffer(1) ]],
    const device T1GPURenderView * rv [[ buffer(3) ]],
    constant uint32_t &rv_i [[buffer(4)]],
    const device T1GPULockedVertex * lverts [[ buffer(5) ]])
{
    RasterizerPixel out;
    
    out.polygon_i = vertices[vertex_i].polygon_i;
    out.locked_vertex_i = vertices[vertex_i].locked_vertex_i;
    
    const device T1GPULockedVertex * lv =
        &lverts[out.locked_vertex_i];
    const device T1GPUzSprite * zs =
        &zsprites[out.polygon_i];
    
    const device T1GPURenderView * c = rv + rv_i;
    
    if (c->write_to_shadow_maps && zs->remove_shadow) {
        out.projpos = vector_float4(0.0f, 0.0f, 100.0f, 1.0f);
        return out;
    }
    
    float4 mesh_vertices = vector_float4(
        lv->xyz[0], lv->xyz[1], lv->xyz[2], 1.0f);
    
    float3 vertex_normal = vector_float3(
        lv->norm_xyz[0],
        lv->norm_xyz[1],
        lv->norm_xyz[2]);
    
    float3 vertex_tangent = vector_float3(
        lv->tan_xyz[0],
        lv->tan_xyz[1],
        lv->tan_xyz[2]);
    
    float3 vertex_bitangent = vector_float3(
        lv->bitan_xyz[0],
        lv->bitan_xyz[1],
        lv->bitan_xyz[2]);
    
    float4x4 model = matrix_float4x4(
        zs->m_4x4[ 0], zs->m_4x4[ 1], zs->m_4x4[ 2], zs->m_4x4[ 3],
        zs->m_4x4[ 4], zs->m_4x4[ 5], zs->m_4x4[ 6], zs->m_4x4[ 7],
        zs->m_4x4[ 8], zs->m_4x4[ 9], zs->m_4x4[10], zs->m_4x4[11],
        zs->m_4x4[12], zs->m_4x4[13], zs->m_4x4[14], zs->m_4x4[15]);
    
    out.worldpos = mesh_vertices * model;
    
    float4x4 view = matrix_float4x4(
        c->v_4x4[ 0], c->v_4x4[ 1], c->v_4x4[ 2], c->v_4x4[ 3],
        c->v_4x4[ 4], c->v_4x4[ 5], c->v_4x4[ 6], c->v_4x4[ 7],
        c->v_4x4[ 8], c->v_4x4[ 9], c->v_4x4[10], c->v_4x4[11],
        c->v_4x4[12], c->v_4x4[13], c->v_4x4[14], c->v_4x4[15]);
    
    out.viewpos = out.worldpos * view;
    
    out.viewpos =
        (zs->ignore_camera * out.worldpos) +
        (1.0f - zs->ignore_camera) * out.viewpos;
    
    out.touchable_id = zs->touch_id;
    
    out.texture_coordinate = vector_float2(lv->uv[0], lv->uv[1]);
    
    float4x4 projection = matrix_float4x4(
        c->p_4x4[ 0], c->p_4x4[ 1], c->p_4x4[ 2], c->p_4x4[ 3],
        c->p_4x4[ 4], c->p_4x4[ 5], c->p_4x4[ 6], c->p_4x4[ 7],
        c->p_4x4[ 8], c->p_4x4[ 9], c->p_4x4[10], c->p_4x4[11],
        c->p_4x4[12], c->p_4x4[13], c->p_4x4[14], c->p_4x4[15]
            );
    
    out.projpos = out.viewpos * projection;
    
    float3x3 normalmodel3x3 = matrix_float3x3(
        zs->norm_3x3[0], zs->norm_3x3[1], zs->norm_3x3[2],
        zs->norm_3x3[3], zs->norm_3x3[4], zs->norm_3x3[5],
        zs->norm_3x3[6], zs->norm_3x3[7], zs->norm_3x3[8]);
    
    float3x3 normalview3x3 = matrix_float3x3(
        c->normv_3x3[0], c->normv_3x3[1], c->normv_3x3[2],
        c->normv_3x3[3], c->normv_3x3[4], c->normv_3x3[5],
        c->normv_3x3[6], c->normv_3x3[7], c->normv_3x3[8]);
    
    out.normal_viewspace = vector_float4(
        vertex_normal *
        normalmodel3x3 *
        normalview3x3,
        0.0f);
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
    
    return out;
}

fragment float4
z_prepass_fragment_shader(
    const RasterizerPixel in [[stage_in]],
    const device T1GPULockedVertex * locked_vertices [[ buffer(0) ]],
    const device T1GPUzSprite * polygons [[ buffer(1) ]],
    const device T1GPURenderView * camera [[ buffer(3) ]],
    constant uint32_t &camera_i [[buffer(4)]],
    const device T1GPUConstMat * const_mats [[ buffer(6) ]],
    const device T1GPUPostProcConsts * updating_globals [[ buffer(7) ]])
{
    #if 0
    unsigned int mat_i =
        locked_vertices[in.locked_vertex_i].
            parent_material_i;
    
    const device T1GPUConstMat * material =
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
We want to output an int32 for 1 of our render targets (touchable_id), but
Metal enforces you use the same data type when you render to multiple targets
simultaneously. That's why we're packing our int32 inside of RGBA8Unorm slots. On
the CPU side we'll retrieve them and put them back together as an int32.
*/
static FragmentAndTouchableOut pack_color_and_touchable_id(
    float4 color,
    int32_t touchable_id)
{
    FragmentAndTouchableOut out;
    
    out.color = (half4)color;
    
    uint uid = as_type<uint>(touchable_id);
    
    out.touchable_id = half4(
        half((uid      ) & 0xFFu) / 255.0h,
        half((uid >>  8) & 0xFFu) / 255.0h,
        half((uid >> 16) & 0xFFu) / 255.0h,
        half((uid >> 24) & 0xFFu) / 255.0h
    );
    
    return out;
}

// Gets the color given a material and lighting setup
float4 get_lit(
    #if T1_SHADOWS_ACTIVE == T1_ACTIVE
    array<texture2d<float>, T1_RENDER_VIEW_CAP> shadow_maps,
    #elif T1_SHADOWS_ACTIVE == T1_INACTIVE
    #else
    #error
    #endif
    array<texture2d_array<half>, TEXTUREARRAYS_SIZE> color_textures,
    const uint32_t camera_i,
    const device T1GPURenderView * render_views,
    const device T1GPULight * lights,
    const device T1GPUzSprite * zsprite,
    const device T1GPUConstMat * material,
    const device T1GPUPostProcConsts * globals,
    const RasterizerPixel in,
    const bool is_base_mtl)
{
    #if T1_AMBIENT_LIGHTING_ACTIVE == T1_ACTIVE
    float4 lit_color = vector_float4(
        material->ambient_rgb[0],
        material->ambient_rgb[1],
        material->ambient_rgb[2],
        10.0f);
    lit_color *= 0.10f;
    #elif T1_AMBIENT_LIGHTING_ACTIVE == T1_INACTIVE
    float4 lit_color = vector_float4(
        0.0f, 0.0f, 0.0f, 1.0f);
    #else
    #error
    #endif
    
    float4 diffuse_base = vector_float4(
        material->diffuse_rgb[0],
        material->diffuse_rgb[1],
        material->diffuse_rgb[2],
        1.0f);
    
    #if T1_SPECULAR_LIGHTING_ACTIVE == T1_ACTIVE
    float4 specular_base = vector_float4(
        material->specular_rgb[0],
        material->specular_rgb[1],
        material->specular_rgb[2],
        1.0f);
    #elif T1_SPECULAR_LIGHTING_ACTIVE == T1_INACTIVE
    #else
    #error
    #endif
    
    float2 uv_adjusted =
        in.texture_coordinate + (
        is_base_mtl *
        vector_float2(
            zsprite->base_mat_uv_offsets[0],
            zsprite->base_mat_uv_offsets[1]));
    
    float2 uv_scroll = vector_float2(
        material->uv_scroll[0],
        material->uv_scroll[1]);
    
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
    #if T1_TEXTURES_ACTIVE == T1_ACTIVE
    if (material->texturearray_i >= 0)
    {
    #elif T1_TEXTURES_ACTIVE == T1_INACTIVE
    if (material->texturearray_i == 0)
    {
    #else
    #error
    #endif
        // Sample the texture to obtain a color
        const half4 color_sample =
            color_textures[material->texturearray_i].
                sample(
                    texture_sampler,
                    uv_adjusted,
                    material->texture_i);
        texture_base = float4(color_sample);
    }
    
    float4 ignore_lighting_color =
        diffuse_base * texture_base;
    
    for (
        uint32_t i = 0;
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
            // render_views[camera_i].use_shadow_maps &&
            lights[i].shadow_map_render_view_i >= 0)
        {
            const device T1GPURenderView * light_rv =
                &render_views[
                    lights[i].shadow_map_render_view_i];
            
            float4x4 light_v = matrix_float4x4(
                light_rv->v_4x4[ 0],
                light_rv->v_4x4[ 1],
                light_rv->v_4x4[ 2],
                light_rv->v_4x4[ 3],
                light_rv->v_4x4[ 4],
                light_rv->v_4x4[ 5],
                light_rv->v_4x4[ 6],
                light_rv->v_4x4[ 7],
                light_rv->v_4x4[ 8],
                light_rv->v_4x4[ 9],
                light_rv->v_4x4[10],
                light_rv->v_4x4[11],
                light_rv->v_4x4[12],
                light_rv->v_4x4[13],
                light_rv->v_4x4[14],
                light_rv->v_4x4[15]);
            
            float4x4 light_p = matrix_float4x4(
                light_rv->p_4x4[ 0],
                light_rv->p_4x4[ 1],
                light_rv->p_4x4[ 2],
                light_rv->p_4x4[ 3],
                light_rv->p_4x4[ 4],
                light_rv->p_4x4[ 5],
                light_rv->p_4x4[ 6],
                light_rv->p_4x4[ 7],
                light_rv->p_4x4[ 8],
                light_rv->p_4x4[ 9],
                light_rv->p_4x4[10],
                light_rv->p_4x4[11],
                light_rv->p_4x4[12],
                light_rv->p_4x4[13],
                light_rv->p_4x4[14],
                light_rv->p_4x4[15]);
            
            float4 light_clip_pos =
                in.worldpos *
                light_v *
                light_p;
            
            float2 shadow_uv =
                ((light_clip_pos.xy /
                    light_clip_pos.w) * 0.5f) + 0.5f;
            
            constexpr sampler shadow_sampler(
                mag_filter::nearest,
                min_filter::nearest,
                address::clamp_to_edge);
            
            shadow_uv[1] = 1.0f - shadow_uv[1];
            float shadow_depth =
                shadow_maps[
                    lights[i].shadow_map_depth_tex_i].
                        sample(
                            shadow_sampler,
                            shadow_uv).r;
            
            float frag_depth =
                light_clip_pos.z / light_clip_pos.w;
            
            shadow_factors =
                (frag_depth <= shadow_depth +
                    SHADOW_BIAS) ?
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
        
        float4 light_worldpos =
            vector_float4(
                lights[i].xyz[0],
                lights[i].xyz[1],
                lights[i].xyz[2],
                1.0f);
        
        float4x4 cam_v_4x4 = matrix_float4x4(
            render_views[camera_i].v_4x4[ 0],
            render_views[camera_i].v_4x4[ 1],
            render_views[camera_i].v_4x4[ 2],
            render_views[camera_i].v_4x4[ 3],
            render_views[camera_i].v_4x4[ 4],
            render_views[camera_i].v_4x4[ 5],
            render_views[camera_i].v_4x4[ 6],
            render_views[camera_i].v_4x4[ 7],
            render_views[camera_i].v_4x4[ 8],
            render_views[camera_i].v_4x4[ 9],
            render_views[camera_i].v_4x4[10],
            render_views[camera_i].v_4x4[11],
            render_views[camera_i].v_4x4[12],
            render_views[camera_i].v_4x4[13],
            render_views[camera_i].v_4x4[14],
            render_views[camera_i].v_4x4[15]);
        
        float4 light_viewspace =
            light_worldpos * cam_v_4x4;
        
        float distance = get_distance_f3(
            (float3)light_viewspace,
            (float3)in.viewpos);
        float distance_overflow = max(
            distance - (lights[i].reach * 0.75f),
            0.0f);
        float attenuation = 1.0f - (
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
        #elif T1_NORMAL_MAPPING_ACTIVE == T1_INACTIVE
        #else
        #error
        #endif
        
        #if T1_DIFFUSE_LIGHTING_ACTIVE == T1_ACTIVE
        float diffuse_dot = max(
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
        
        lit_color += (
            diffuse_base *
            light_diffuse_multiplier);
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
        
        float specular_dot = pow(
            max(
                dot(
                    normal_viewspace,
                    half_lightdir_half_view),
                0.0),
            material->specular_exponent);
        
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
    
    lit_color *= texture_base;
    
    lit_color =
        ((1.0f - zsprite->ignore_lighting) *
            lit_color) +
        (zsprite->ignore_lighting *
            ignore_lighting_color);
    
    lit_color[3] *= zsprite->alpha * material->alpha;
    
    lit_color += vector_float4(
        zsprite->bonus_rgb[0],
        zsprite->bonus_rgb[1],
        zsprite->bonus_rgb[2],
        0.0f);
    
    lit_color = clamp(lit_color, 0.0f, 1.0f);
    
    return lit_color;
}

fragment FragmentAndTouchableOut
fragment_shader(
    const RasterizerPixel in [[stage_in]],
    array<texture2d_array<half>, TEXTUREARRAYS_SIZE>
        color_textures[[ texture(0) ]],
    #if T1_SHADOWS_ACTIVE == T1_ACTIVE
    array<texture2d<float>, T1_RENDER_VIEW_CAP> shadow_map [[texture(SHADOW_MAPS_1ST_FRAGARG_I)]],
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
    constant uint32_t &camera_i [[buffer(4)]],
    const device T1GPUConstMat * const_mats [[ buffer(6) ]],
    const device T1GPUPostProcConsts * updating_globals [[ buffer(7) ]])
{
    unsigned int mat_i =
        locked_vertices[in.locked_vertex_i].parent_material_i;
    const device T1GPUConstMat * material =
        mat_i == PARENT_MATERIAL_BASE ?
            &polygons[in.polygon_i].base_mat :
            &const_mats[locked_vertices[in.locked_vertex_i].
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
        /* const device GPUPolygonMaterial * fragment_material: */
            material,
        /* const device GPUPostProcessingConstants * updating_globals: */
            updating_globals,
        /* const device RasterizerPixel * in: */
            in,
        /* const bool is_base_mtl: */
            mat_i == PARENT_MATERIAL_BASE);
    
    int diamond_size = 35.0f;
    int neghalfdiamond = -1.0f * (diamond_size / 2);
    
    int alpha_tresh = (int)(lit_color[3] * diamond_size);
    
    if (
        lit_color[3] < 0.05f ||
        (
            lit_color[3] < 0.95f &&
            (
                abs((neghalfdiamond + (int)in.projpos.x % diamond_size)) +
                abs((neghalfdiamond + (int)in.projpos.y % diamond_size))
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
    array<texture2d_array<half>, TEXTUREARRAYS_SIZE>
        color_textures[[ texture(0), maybe_unused ]],
    #if T1_SHADOWS_ACTIVE == T1_ACTIVE
    array<texture2d<float>, T1_RENDER_VIEW_CAP>
        shadow_maps[[ texture(SHADOW_MAPS_1ST_FRAGARG_I), maybe_unused ]],
    #elif T1_SHADOWS_ACTIVE == T1_INACTIVE
    #else
    #error
    #endif
    const device T1GPULockedVertex * locked_vertices [[ buffer(0) ]],
    const device T1GPUzSprite * polygons [[ buffer(1) ]],
    const device T1GPULight * lights [[ buffer(2) ]],
    const device T1GPURenderView * render_views [[ buffer(3) ]],
    constant uint &camera_i [[buffer(4)]],
    const device T1GPUConstMat * locked_materials [[ buffer(6) ]],
    const device T1GPUPostProcConsts * updating_globals [[ buffer(7) ]])
{
    unsigned int mat_i =
        locked_vertices[in.locked_vertex_i].parent_material_i;
    
    const device T1GPUConstMat * material =
        mat_i == PARENT_MATERIAL_BASE ?
            &polygons[in.polygon_i].base_mat :
            &locked_materials[locked_vertices[in.locked_vertex_i].locked_materials_head_i + mat_i];
    
    float4 lit_color = get_lit(
        #if T1_SHADOWS_ACTIVE == T1_ACTIVE
        /* texture2d<float> shadow_map: */
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
            material,
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
    float blur_pct;
    float nonblur_pct;
    float color_quantization;
    float fog_factor;
    unsigned int curtime;
};

vertex PostProcessingFragment
single_quad_vertex_shader(
    const uint vertexID [[ vertex_id ]],
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
        1.0h);
    
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
    texture2d<half> texture  [[texture(0)]],
    #if T1_BLOOM_ACTIVE == T1_ACTIVE
    texture2d<half> downsampled_1  [[texture(1)]],
    texture2d<half> downsampled_2  [[texture(2)]],
    texture2d<half> downsampled_3  [[texture(3)]],
    texture2d<half> downsampled_4  [[texture(4)]],
    texture2d<half> downsampled_5  [[texture(5)]],
    #elif T1_BLOOM_ACTIVE == T1_INACTIVE
    #else
    #error
    #endif
    #if T1_FOG_ACTIVE == T1_ACTIVE
    texture2d_array<half> perlin_texture [[ texture(6) ]],
    #elif T1_FOG_ACTIVE == T1_INACTIVE
    #else
    #error
    #endif
    depth2d<float> camera_depth_map
        [[texture(CAM_DEPTH_FRAGARG_I)]])
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
    float dist = camera_depth_map.sample(sampler, texcoord);
    dist = clamp(dist - 0.96f, 0.0f, 0.04f) * 50.0f;
    dist = pow(dist, 2.0f);
    dist *= in.fog_factor;
    
    float progress = sin(float(in.curtime) * 0.0000001f);
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
    color_sample = clamp(color_sample, 0.0h, 1.20h);
    color_sample = color_sample / 1.20h;
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
    texture2d<half, access::read_write>
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
        in_texture.read(
            input_pos + vector_uint2(1, 1)) +
        in_texture.read(
            input_pos + vector_uint2(1, 0)) +
        in_texture.read(
            input_pos + vector_uint2(0, 1))) / 4.0h;
    
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

typedef struct
{
    float4 projpos [[ position ]];
    float4 rgba [[ flat ]];
} FlatQuadPixel;

vertex FlatQuadPixel
flat_billboard_quad_vertex_shader(
    uint vertex_i [[ vertex_id ]],
    const device T1GPUFlatQuad * quads [[ buffer(2) ]],
    const device T1GPURenderView * camera [[ buffer(3) ]])
{
    uint quad_i = vertex_i / 6;
    uint corner_id  = vertex_i % 6;
    
    float halfsize = quads[quad_i].size * 0.5f;
    
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

#if T1_OUTLINES_ACTIVE == T1_ACTIVE
typedef struct
{
    float4 pos [[position]];
    float  outline_alpha [[ flat ]];
} OutlinePixel;

vertex OutlinePixel
outlines_vertex_shader(
    uint vertex_i [[ vertex_id ]],
    const device T1GPUVertexIndices * vertices [[ buffer(0) ]],
    const device T1GPUzSprite * polygons [[ buffer(1) ]],
    const device T1GPURenderView * camera [[ buffer(3) ]],
    constant uint &rv_i [[buffer(4)]],
    const device T1GPULockedVertex * lverts [[ buffer(5)]])
{
    OutlinePixel out;
    
    uint polygon_i = vertices[vertex_i].polygon_i;
    uint locked_vertex_i =
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
    
    out.outline_alpha = polygons[polygon_i].
        outline_alpha;
    
    float4x4 m_4x4 = matrix_float4x4(
        polygons[polygon_i].m_4x4[ 0],
        polygons[polygon_i].m_4x4[ 1],
        polygons[polygon_i].m_4x4[ 2],
        polygons[polygon_i].m_4x4[ 3],
        polygons[polygon_i].m_4x4[ 4],
        polygons[polygon_i].m_4x4[ 5],
        polygons[polygon_i].m_4x4[ 6],
        polygons[polygon_i].m_4x4[ 7],
        polygons[polygon_i].m_4x4[ 8],
        polygons[polygon_i].m_4x4[ 9],
        polygons[polygon_i].m_4x4[10],
        polygons[polygon_i].m_4x4[11],
        polygons[polygon_i].m_4x4[12],
        polygons[polygon_i].m_4x4[13],
        polygons[polygon_i].m_4x4[14],
        polygons[polygon_i].m_4x4[15]);
    
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
        polygons[polygon_i].norm_3x3[ 0],
        polygons[polygon_i].norm_3x3[ 1],
        polygons[polygon_i].norm_3x3[ 2],
        polygons[polygon_i].norm_3x3[ 3],
        polygons[polygon_i].norm_3x3[ 4],
        polygons[polygon_i].norm_3x3[ 5],
        polygons[polygon_i].norm_3x3[ 6],
        polygons[polygon_i].norm_3x3[ 7],
        polygons[polygon_i].norm_3x3[ 8]);
    
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
