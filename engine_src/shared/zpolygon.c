#include "zpolygon.h"

zPolygonCollection * zpolygons_to_render = NULL;

void request_next_zpolygon(PolygonRequest * stack_recipient)
{
    stack_recipient->materials_size = MAX_MATERIALS_PER_POLYGON;
    
    for (
        uint32_t zp_i = 0;
        zp_i < zpolygons_to_render->size;
        zp_i++)
    {
        if (zpolygons_to_render->cpu_data[zp_i].deleted)
        {
            stack_recipient->cpu_data     =
                &zpolygons_to_render->cpu_data[zp_i];
            stack_recipient->gpu_data     =
                &zpolygons_to_render->gpu_data[zp_i];
            stack_recipient->gpu_materials =
                zpolygons_to_render->gpu_materials +
                (zp_i * MAX_MATERIALS_PER_POLYGON);
            stack_recipient->cpu_data->committed = false;
            return;
        }
    }
    
    log_assert(zpolygons_to_render->size + 1 < MAX_POLYGONS_PER_BUFFER);
    stack_recipient->cpu_data     =
        &zpolygons_to_render->cpu_data[zpolygons_to_render->size];
    stack_recipient->gpu_data     =
        &zpolygons_to_render->gpu_data[zpolygons_to_render->size];
    stack_recipient->gpu_materials =
        zpolygons_to_render->gpu_materials +
        (zpolygons_to_render->size * MAX_MATERIALS_PER_POLYGON);
    stack_recipient->cpu_data[zpolygons_to_render->size].deleted = false;
    stack_recipient->cpu_data->committed = false;
    
    zpolygons_to_render->size += 1;
    
    return;
}

void commit_zpolygon_to_render(
    PolygonRequest * to_commit)
{
    log_assert(to_commit->cpu_data->mesh_id >= 0);
    log_assert(to_commit->cpu_data->mesh_id < (int32_t)all_mesh_summaries_size);
    log_assert(to_commit->cpu_data->mesh_id < ALL_MESHES_SIZE);
    log_assert(
        all_mesh_summaries[to_commit->cpu_data->mesh_id].vertices_size > 0);
    
    uint32_t all_mesh_vertices_tail_i =
        (uint32_t)(
            all_mesh_summaries[to_commit->cpu_data->mesh_id].vertices_head_i +
            all_mesh_summaries[to_commit->cpu_data->mesh_id].vertices_size -
            1);
    log_assert(all_mesh_vertices_tail_i < all_mesh_vertices->size);
    for (
        uint32_t vert_i = (uint32_t)
            all_mesh_summaries[to_commit->cpu_data->mesh_id].vertices_head_i;
        vert_i <= all_mesh_vertices_tail_i;
        vert_i++)
    {
        log_assert(all_mesh_vertices->gpu_data[vert_i].parent_material_i >= 0);
        log_assert(all_mesh_vertices->gpu_data[vert_i].parent_material_i  <
            MAX_MATERIALS_PER_POLYGON);
        log_assert(all_mesh_vertices->gpu_data[vert_i].parent_material_i  <
            all_mesh_summaries[to_commit->cpu_data->mesh_id].materials_size);
    }
    
    for (
        int32_t mat_i = 0;
        mat_i < MAX_MATERIALS_PER_POLYGON;
        mat_i++)
    {
        if (to_commit->gpu_materials[mat_i].texturearray_i >= 0) {
            log_assert(to_commit->gpu_materials[mat_i].texture_i >= 0);
            register_high_priority_if_unloaded(
                to_commit->gpu_materials[mat_i].texturearray_i,
                to_commit->gpu_materials[mat_i].texture_i);
        }
        
        log_assert(
            to_commit->gpu_materials[mat_i].texture_i < 5000);
        log_assert(
            to_commit->gpu_materials[mat_i].texturearray_i < TEXTUREARRAYS_SIZE);
        
        for (uint32_t rgba_i = 0; rgba_i < 4; rgba_i++) {
            log_assert(to_commit->gpu_materials[mat_i].rgba[rgba_i] >= - 0.1f);
            log_assert(to_commit->gpu_materials[mat_i].rgba[rgba_i] <=  15.0f);
        }
    }
    
    to_commit->cpu_data->committed = true;
}

bool32_t fetch_zpolygon_by_object_id(
    PolygonRequest * recipient,
    const int32_t object_id)
{
    common_memset_char(recipient, 0, sizeof(PolygonRequest));
    
    for (
        uint32_t zp_i = 0;
        zp_i < zpolygons_to_render->size;
        zp_i++)
    {
        if (
            !zpolygons_to_render->cpu_data[zp_i].deleted &&
            zpolygons_to_render->cpu_data[zp_i].object_id == object_id)
        {
            recipient->cpu_data = &zpolygons_to_render->cpu_data[zp_i];
            recipient->gpu_data = &zpolygons_to_render->gpu_data[zp_i];
            return true;
        }
    }
    
    log_assert(recipient->cpu_data == NULL);
    log_assert(recipient->gpu_data == NULL);
    return false;
}

void delete_zpolygon_object(const int32_t with_object_id)
{
    for (uint32_t i = 0; i < zpolygons_to_render->size; i++) {
        if (zpolygons_to_render->cpu_data[i].object_id == with_object_id)
        {
            zpolygons_to_render->cpu_data[i].deleted   = true;
            zpolygons_to_render->cpu_data[i].object_id = -1;
        }
    }
}

float get_x_multiplier_for_width(
    zPolygonCPU * for_poly,
    const float for_width)
{
    log_assert(for_poly != NULL);
    #ifndef LOGGER_IGNORE_ASSERTS
    if (for_poly == NULL) {
        return 0.0f;
    }
    #endif
    
    log_assert(for_poly->mesh_id >= 0);
    log_assert(for_poly->mesh_id < (int32_t)all_mesh_summaries_size);
    
    log_assert(for_poly->mesh_id >= 0);
    log_assert(for_poly->mesh_id < (int32_t)all_mesh_summaries_size);
    
    float return_value =
        for_width / all_mesh_summaries[for_poly->mesh_id].base_width;
    
    return return_value;
}

float get_y_multiplier_for_height(
    zPolygonCPU * for_poly,
    const float for_height)
{
    log_assert(for_poly != NULL);
    #ifndef LOGGER_IGNORE_ASSERTS
    if (for_poly == NULL) {
        return 0.0f;
    }
    #endif
    
    log_assert(for_poly->mesh_id >= 0);
    log_assert(for_poly->mesh_id < (int32_t)all_mesh_summaries_size);
    
    float return_value =
        for_height / all_mesh_summaries[for_poly->mesh_id].base_height;
    
    return return_value;
}

void scale_zpolygon_multipliers_to_height(
    zPolygonCPU * cpu_data,
    GPUPolygon * gpu_data,
    const float new_height)
{
    float new_multiplier = get_y_multiplier_for_height(
        /* zPolygonCPU * for_poly: */
            cpu_data,
        /* const float for_height: */
            new_height);
    
    gpu_data->xyz_multiplier[0] = new_multiplier;
    gpu_data->xyz_multiplier[1] = new_multiplier;
    gpu_data->xyz_multiplier[2] = new_multiplier;
}

void construct_zpolygon(
    PolygonRequest * to_construct)
{
    assert(to_construct->cpu_data != NULL);
    assert(to_construct->gpu_data != NULL);
    assert(to_construct->gpu_materials != NULL);
    assert(
        (to_construct->materials_size == 1 ||
        to_construct->materials_size == MAX_MATERIALS_PER_POLYGON));
    
    common_memset_char(
        to_construct->cpu_data,
        0,
        sizeof(zPolygonCPU));
    common_memset_char(
        to_construct->gpu_materials,
        0,
        sizeof(GPUPolygonMaterial) *
            to_construct->materials_size);
    common_memset_char(
        to_construct->gpu_data,
        0,
        sizeof(GPUPolygon));
    
    to_construct->gpu_data->xyz_multiplier[0] = 1.0f;
    to_construct->gpu_data->xyz_multiplier[1] = 1.0f;
    to_construct->gpu_data->xyz_multiplier[2] = 1.0f;
    to_construct->gpu_data->scale_factor = 1.0f;
    to_construct->gpu_data->touchable_id = -1;
    
    to_construct->cpu_data->mesh_id = -1;
    to_construct->cpu_data->object_id = -1;
    to_construct->cpu_data->visible = true;
    
    for (uint32_t i = 0; i < MAX_MATERIALS_PER_POLYGON; i++) {
        to_construct->gpu_materials[i].rgba[0]        = 0.75f;
        to_construct->gpu_materials[i].rgba[1]        = 0.75f;
        to_construct->gpu_materials[i].rgba[2]        = 0.75f;
        to_construct->gpu_materials[i].rgba[3]        = 0.75f;
        to_construct->gpu_materials[i].rgb_cap[0]     = 1.0f;
        to_construct->gpu_materials[i].rgb_cap[1]     = 1.0f;
        to_construct->gpu_materials[i].rgb_cap[2]     = 1.0f;
        to_construct->gpu_materials[i].diffuse        = 0.50f;
        to_construct->gpu_materials[i].specular       = 0.50f;
        to_construct->gpu_materials[i].texture_i      = -1;
        to_construct->gpu_materials[i].texturearray_i = -1;
    }
}

float get_distance_f3(
    const float p1[3],
    const float p2[3])
{
    return sqrtf(
        ((p1[0] - p2[0]) * (p1[0] - p2[0])) +
        ((p1[1] - p2[1]) * (p1[1] - p2[1])) +
        ((p1[2] - p2[2]) * (p1[2] - p2[2])));
}

float dot_of_vertices_f3(
    const float a[3],
    const float b[3])
{
    float x =
        (
            a[0] * b[0]
        );
    x = (isnan(x) || !isfinite(x)) ? FLOAT32_MAX : x;
    
    float y = (a[1] * b[1]);
    y = (isnan(y) || !isfinite(y)) ? FLOAT32_MAX : y;
    
    float z = (a[2] * b[2]);
    z = (isnan(z) || !isfinite(z)) ? FLOAT32_MAX : z;
    
    float return_value = x + y + z;
    
    log_assert(!isnan(return_value));
    
    return return_value;
}

#if 0
static SIMD_VEC4F normal_vec4f_undo_camera_rotation(
    SIMD_VEC4F normal_xyz,
    float ignore_camera)
{
    SIMD_VEC4F ignore_cam_pos = normal_xyz;
    
    ignore_cam_pos = z_rotate_vec4f_known_cossin(
        ignore_cam_pos,
        camera.xyz_cosangle[2],
        camera.xyz_sinangle[2]);
    ignore_cam_pos = y_rotate_vec4f_known_cossin(
        ignore_cam_pos,
        camera.xyz_cosangle[1],
        camera.xyz_sinangle[1]);
    ignore_cam_pos = x_rotate_vec4f_known_cossin(
        ignore_cam_pos,
        camera.xyz_cosangle[0],
        camera.xyz_sinangle[0]);
    
    float one_minus_ignore_camera = 1.0f - ignore_camera;
    return simd_add_vec4f(
        simd_mul_vec4f(simd_set1_vec4f(ignore_camera), ignore_cam_pos),
        simd_mul_vec4f(simd_set1_vec4f(one_minus_ignore_camera), normal_xyz));
}
#endif

static void normal_undo_camera_rotation(
    float normal_xyz[3],
    float ignore_camera)
{
    log_assert(ignore_camera >= 0.0f);
    log_assert(ignore_camera <= 1.0f);
    
    float ignore_cam_pos[3];
    common_memcpy(ignore_cam_pos, normal_xyz, sizeof(float) * 3);
    
    // In the standard shader everything will be rotated by negative the
    // xyz_angle, so do the opposite
    z_rotate_f3(
        ignore_cam_pos,
        camera.xyz_angle[2]);
    y_rotate_f3(
        ignore_cam_pos,
        camera.xyz_angle[1]);
    x_rotate_f3(
        ignore_cam_pos,
        camera.xyz_angle[0]);
    
    normal_xyz[0] =
        (ignore_camera * ignore_cam_pos[0]) +
        ((1.0f - ignore_camera) * normal_xyz[0]);
    normal_xyz[1] =
        (ignore_camera * ignore_cam_pos[1]) +
        ((1.0f - ignore_camera) * normal_xyz[1]);
    normal_xyz[2] =
        (ignore_camera * ignore_cam_pos[2]) +
        ((1.0f - ignore_camera) * normal_xyz[2]);
}

static SIMD_VEC4F undo_camera_translation_vec4f(
    SIMD_VEC4F xyz,
    float ignore_camera)
{
    SIMD_VEC4F ignore_cam_pos = xyz;
    
    ignore_cam_pos = z_rotate_vec4f_known_cossin(
        ignore_cam_pos,
        camera.xyz_cosangle[2],
        camera.xyz_sinangle[2]);
    ignore_cam_pos = y_rotate_vec4f_known_cossin(
        ignore_cam_pos,
        camera.xyz_cosangle[1],
        camera.xyz_sinangle[1]);
    ignore_cam_pos = x_rotate_vec4f_known_cossin(
        ignore_cam_pos,
        camera.xyz_cosangle[0],
        camera.xyz_sinangle[0]);
    
    ignore_cam_pos = simd_add_vec4f(
        ignore_cam_pos,
        simd_load_vec4f(camera.xyz));
    
    float one_minus_ignore_cam = 1.0f - ignore_camera;
    return simd_add_vec4f(
        simd_mul_vec4f(simd_set1_vec4f(ignore_camera), ignore_cam_pos),
        simd_mul_vec4f(simd_set1_vec4f(one_minus_ignore_cam), xyz));
}

static void undo_camera_translation(
    float xyz[3],
    float ignore_camera)
{
    #ifdef PROFILER_ACTIVE
    profiler_start("undo_camera_translation()");
    #endif
    
    float ignore_cam_pos[3];
    common_memcpy(ignore_cam_pos, xyz, sizeof(float) * 3);
    
    // In the standard shader everything will be rotated by negative the
    // xyz_angle, so do the opposite
    z_rotate_f3_known_cossin(
        ignore_cam_pos,
        /* cos_z_angle: */ camera.xyz_cosangle[2],
        /* sin_z_angle: */ camera.xyz_sinangle[2]);
    y_rotate_f3_known_cossin(
        ignore_cam_pos,
        /* cos_y_angle: */ camera.xyz_cosangle[1],
        /* sin_y_angle: */ camera.xyz_sinangle[1]);
    x_rotate_f3_known_cossin(
        ignore_cam_pos,
        /* cos_x_angle: */ camera.xyz_cosangle[0],
        /* sin_x_angle: */ camera.xyz_sinangle[0]);
    
    // In the standard shader camera xyz will be subbed
    ignore_cam_pos[0] += camera.xyz[0];
    ignore_cam_pos[1] += camera.xyz[1];
    ignore_cam_pos[2] += camera.xyz[2];
    
    xyz[0] =
        (ignore_camera * ignore_cam_pos[0]) +
        ((1.0f - ignore_camera) * xyz[0]);
    xyz[1] =
        (ignore_camera * ignore_cam_pos[1]) +
        ((1.0f - ignore_camera) * xyz[1]);
    xyz[2] =
        (ignore_camera * ignore_cam_pos[2]) +
        ((1.0f - ignore_camera) * xyz[2]);
    
    #ifdef PROFILER_ACTIVE
    profiler_end("undo_camera_translation()");
    #endif
}

void simd_zpolygon_get_transformed_triangle_vertices(
    const zPolygonCPU * cpu_data,
    const GPUPolygon * gpu_data,
    const int32_t locked_vertex_i,
    float * vertices_recipient_10f)
{
    (void)cpu_data;
    
    #ifdef PROFILER_ACTIVE
    profiler_start("simd_zpolygon_get_transformed_triangle_vertices()");
    #endif
    
    float xyz_cosangle[3];
    xyz_cosangle[0] = cosf(gpu_data->xyz_angle[0]);
    xyz_cosangle[1] = cosf(gpu_data->xyz_angle[1]);
    xyz_cosangle[2] = cosf(gpu_data->xyz_angle[2]);
    float xyz_sinangle[3];
    xyz_sinangle[0] = sinf(gpu_data->xyz_angle[0]);
    xyz_sinangle[1] = sinf(gpu_data->xyz_angle[1]);
    xyz_sinangle[2] = sinf(gpu_data->xyz_angle[2]);
    
    for (int32_t i = 0; i < 3; i++) {
        SIMD_VEC4F vertex = simd_load_vec4f(all_mesh_vertices->gpu_data[
            locked_vertex_i + i].xyz);
        SIMD_VEC4F multipliers = simd_load_vec4f(gpu_data->xyz_multiplier);
        vertex = simd_mul_vec4f(vertex, multipliers);
        vertex = simd_add_vec4f(vertex, simd_load_vec4f(gpu_data->xyz_offset));
        vertex = simd_mul_vec4f(
            vertex,
            simd_set1_vec4f(gpu_data->scale_factor));
        
        vertex = x_rotate_vec4f_known_cossin(
            vertex,
            xyz_cosangle[0],
            xyz_sinangle[0]);
        vertex = y_rotate_vec4f_known_cossin(
            vertex,
            xyz_cosangle[1],
            xyz_sinangle[1]);
        vertex = z_rotate_vec4f_known_cossin(
            vertex,
            xyz_cosangle[2],
            xyz_sinangle[2]);
        
        vertex = simd_add_vec4f(vertex, simd_load_vec4f(gpu_data->xyz));
        
        vertex = undo_camera_translation_vec4f(
            vertex,
            gpu_data->ignore_camera);
        
        simd_store_vec4f(vertices_recipient_10f + (i * 3), vertex);
    }
    
    #ifdef PROFILER_ACTIVE
    profiler_end("simd_zpolygon_get_transformed_triangle_vertices()");
    #endif
}

#if 0
void legacy_simd_zpolygon_get_transformed_triangle_vertices(
    const zPolygonCPU * cpu_data,
    const GPUPolygon * gpu_data,
    const int32_t locked_vertex_i,
    float * vertices_recipient_10f,
    float * normals_recipient_10f)
{
    #ifdef PROFILER_ACTIVE
    profiler_start("simd_zpolygon_get_transformed_triangle_vertices()");
    #endif
    
    float xyz_cosangle[3];
    xyz_cosangle[0] = cosf(gpu_data->xyz_angle[0]);
    xyz_cosangle[1] = cosf(gpu_data->xyz_angle[1]);
    xyz_cosangle[2] = cosf(gpu_data->xyz_angle[2]);
    float xyz_sinangle[3];
    xyz_sinangle[0] = sinf(gpu_data->xyz_angle[0]);
    xyz_sinangle[1] = sinf(gpu_data->xyz_angle[1]);
    xyz_sinangle[2] = sinf(gpu_data->xyz_angle[2]);
    
    for (int32_t i = 0; i < 3; i++) {
        SIMD_VEC4F vertex = simd_load_vec4f(all_mesh_vertices->gpu_data[
            locked_vertex_i + i].xyz);
        vertex = simd_mul_vec4f(
            vertex,
            simd_load_vec4f(gpu_data->xyz_multiplier));
        vertex = simd_add_vec4f(
            vertex,
            simd_load_vec4f(gpu_data->xyz_offset));
        vertex = simd_mul_vec4f(
            vertex,
            simd_set1_vec4f(gpu_data->scale_factor));
        
        SIMD_VEC4F normal = simd_load_vec4f(all_mesh_vertices->gpu_data[
            locked_vertex_i + i].normal_xyz);
        
        normal = normalize_vertex_vec4f(normal);
        
        vertex = x_rotate_vec4f_known_cossin(
            vertex,
            xyz_cosangle[0],
            xyz_sinangle[0]);
        vertex = y_rotate_vec4f_known_cossin(
            vertex,
            xyz_cosangle[1],
            xyz_sinangle[1]);
        vertex = z_rotate_vec4f_known_cossin(
            vertex,
            xyz_cosangle[2],
            xyz_sinangle[2]);
        
        normal = x_rotate_vec4f_known_cossin(
            normal,
            xyz_cosangle[0],
            xyz_sinangle[0]);
        normal = y_rotate_vec4f_known_cossin(
            normal,
            xyz_cosangle[1],
            xyz_sinangle[1]);
        normal = z_rotate_vec4f_known_cossin(
            normal,
            xyz_cosangle[2],
            xyz_sinangle[2]);
        
        vertex = simd_add_vec4f(vertex, simd_load_vec4f(gpu_data->xyz));
        
        vertex = undo_camera_translation_vec4f(
            vertex,
            gpu_data->ignore_camera);
        
        normal = normal_vec4f_undo_camera_rotation(
            normal,
            gpu_data->ignore_camera);
        
        simd_store_vec4f(vertices_recipient_10f + (i * 3), vertex);
        simd_store_vec4f(normals_recipient_10f + (i * 3), normal);
    }
    
    #ifdef PROFILER_ACTIVE
    profiler_end("simd_zpolygon_get_transformed_triangle_vertices()");
    #endif
}
#endif

void zpolygon_get_transformed_triangle_vertices(
    const zPolygonCPU * cpu_data,
    const GPUPolygon * gpu_data,
    const int32_t locked_vertex_i,
    float * vertices_recipient_9f,
    float * normals_recipient_9f)
{
    (void)cpu_data;
    
    #ifdef PROFILER_ACTIVE
    profiler_start("zpolygon_get_transformed_triangle_vertices()");
    #endif
    
    for (int32_t i = 0; i < 9; i++) {
        int32_t imod3 = i % 3;
        vertices_recipient_9f[i] = all_mesh_vertices->gpu_data[
            locked_vertex_i + (i / 3)].xyz[imod3];
        vertices_recipient_9f[i] *= gpu_data->xyz_multiplier[imod3];
        vertices_recipient_9f[i] += gpu_data->xyz_offset[imod3];
        vertices_recipient_9f[i] *= gpu_data->scale_factor;
        
        normals_recipient_9f[i] = all_mesh_vertices->gpu_data[
            locked_vertex_i + (i / 3)].normal_xyz[imod3];
    }
    
    normalize_zvertex_f3(normals_recipient_9f);
    normalize_zvertex_f3(normals_recipient_9f+3);
    normalize_zvertex_f3(normals_recipient_9f+6);
    
    float xyz_cosangle[3];
    xyz_cosangle[0] = cosf(gpu_data->xyz_angle[0]);
    xyz_cosangle[1] = cosf(gpu_data->xyz_angle[1]);
    xyz_cosangle[2] = cosf(gpu_data->xyz_angle[2]);
    float xyz_sinangle[3];
    xyz_sinangle[0] = sinf(gpu_data->xyz_angle[0]);
    xyz_sinangle[1] = sinf(gpu_data->xyz_angle[1]);
    xyz_sinangle[2] = sinf(gpu_data->xyz_angle[2]);
    
    x_rotate_f3_known_cossin(vertices_recipient_9f  , xyz_cosangle[0], xyz_sinangle[0]);
    y_rotate_f3_known_cossin(vertices_recipient_9f  , xyz_cosangle[1], xyz_sinangle[1]);
    z_rotate_f3_known_cossin(vertices_recipient_9f  , xyz_cosangle[2], xyz_sinangle[2]);
    x_rotate_f3_known_cossin(vertices_recipient_9f+3, xyz_cosangle[0], xyz_sinangle[0]);
    y_rotate_f3_known_cossin(vertices_recipient_9f+3, xyz_cosangle[1], xyz_sinangle[1]);
    z_rotate_f3_known_cossin(vertices_recipient_9f+3, xyz_cosangle[2], xyz_sinangle[2]);
    x_rotate_f3_known_cossin(vertices_recipient_9f+6, xyz_cosangle[0], xyz_sinangle[0]);
    y_rotate_f3_known_cossin(vertices_recipient_9f+6, xyz_cosangle[1], xyz_sinangle[1]);
    z_rotate_f3_known_cossin(vertices_recipient_9f+6, xyz_cosangle[2], xyz_sinangle[2]);
    
    x_rotate_f3_known_cossin(normals_recipient_9f  , xyz_cosangle[0], xyz_sinangle[0]);
    y_rotate_f3_known_cossin(normals_recipient_9f  , xyz_cosangle[1], xyz_sinangle[1]);
    z_rotate_f3_known_cossin(normals_recipient_9f  , xyz_cosangle[2], xyz_sinangle[2]);
    x_rotate_f3_known_cossin(normals_recipient_9f+3, xyz_cosangle[0], xyz_sinangle[0]);
    y_rotate_f3_known_cossin(normals_recipient_9f+3, xyz_cosangle[1], xyz_sinangle[1]);
    z_rotate_f3_known_cossin(normals_recipient_9f+3, xyz_cosangle[2], xyz_sinangle[2]);
    x_rotate_f3_known_cossin(normals_recipient_9f+6, xyz_cosangle[0], xyz_sinangle[0]);
    y_rotate_f3_known_cossin(normals_recipient_9f+6, xyz_cosangle[1], xyz_sinangle[1]);
    z_rotate_f3_known_cossin(normals_recipient_9f+6, xyz_cosangle[2], xyz_sinangle[2]);
    
    for (uint32_t i = 0; i < 9; i++) {
        vertices_recipient_9f[i] += gpu_data->xyz[i%3];
    }
    
    undo_camera_translation(vertices_recipient_9f,   gpu_data->ignore_camera);
    undo_camera_translation(vertices_recipient_9f+3, gpu_data->ignore_camera);
    undo_camera_translation(vertices_recipient_9f+6, gpu_data->ignore_camera);
    
    normal_undo_camera_rotation(
        normals_recipient_9f  ,
        gpu_data->ignore_camera);
    normal_undo_camera_rotation(
        normals_recipient_9f+3,
        gpu_data->ignore_camera);
    normal_undo_camera_rotation(
        normals_recipient_9f+6,
        gpu_data->ignore_camera);
    
    #ifdef PROFILER_ACTIVE
    profiler_end("zpolygon_get_transformed_triangle_vertices()");
    #endif
}

void construct_quad(
    const float left_x,
    const float bottom_y,
    const float z,
    const float width,
    const float height,
    PolygonRequest * stack_recipient)
{
    log_assert(z > 0.0f);
    
    construct_zpolygon(stack_recipient);
    
    const float mid_x =
        left_x + (width  / 2);
    const float mid_y =
        bottom_y  + (height / 2);
    
    stack_recipient->gpu_data->xyz[0] = mid_x;
    stack_recipient->gpu_data->xyz[1] = mid_y;
    stack_recipient->gpu_data->xyz[2] = z;
    stack_recipient->cpu_data->visible = true;
    stack_recipient->gpu_data->ignore_camera = false;
    
    // a quad is hardcoded in objmodel.c's init_all_meshes()
    stack_recipient->cpu_data->mesh_id = 0;
    
    // the hardcoded quad offsets range from -1.0f to 1.0f,
    // so the current width is 2.0f
    float current_width = 2.0f;
    float current_height = 2.0f;
    stack_recipient->gpu_data->xyz_multiplier[0] =
        width / current_width;
    stack_recipient->gpu_data->xyz_multiplier[1] =
        height / current_height;
    stack_recipient->gpu_data->xyz_multiplier[2] = 1.0f;
    
    stack_recipient->gpu_materials[0].rgba[0] = 1.0f;
    stack_recipient->gpu_materials[0].rgba[1] = 1.0f;
    stack_recipient->gpu_materials[0].rgba[2] = 1.0f;
    stack_recipient->gpu_materials[0].rgba[3] = 1.0f;
    stack_recipient->gpu_materials[0].texturearray_i = -1;
    stack_recipient->gpu_materials[0].texture_i = -1;
}

void construct_quad_around(
    const float mid_x,
    const float mid_y,
    const float z,
    const float width,
    const float height,
    PolygonRequest * stack_recipient)
{
    // log_assert(z > 0.0f);
    
    construct_zpolygon(stack_recipient);
    
    stack_recipient->gpu_data->xyz[0]  = mid_x;
    stack_recipient->gpu_data->xyz[1]  = mid_y;
    stack_recipient->gpu_data->xyz[2]  = z;
    stack_recipient->cpu_data->visible = true;
    
    // the hardcoded quad offsets range from -1.0f to 1.0f,
    // so the current width is 2.0f
    float current_width = 2.0f;
    float current_height = 2.0f;
    stack_recipient->gpu_data->xyz_multiplier[0] = width / current_width;
    stack_recipient->gpu_data->xyz_multiplier[1] = height / current_height;
    stack_recipient->gpu_data->xyz_multiplier[2] =
        stack_recipient->gpu_data->xyz_multiplier[1] / 20.0f;
    
    stack_recipient->cpu_data->mesh_id = 0;
    stack_recipient->gpu_materials[0].rgba[0] = 1.0f;
    stack_recipient->gpu_materials[0].rgba[1] = 1.0f;
    stack_recipient->gpu_materials[0].rgba[2] = 1.0f;
    stack_recipient->gpu_materials[0].rgba[3] = 1.0f;
    stack_recipient->gpu_materials[0].texturearray_i = -1;
    stack_recipient->gpu_materials[0].texture_i = -1;
}

void construct_cube_around(
    const float mid_x,
    const float mid_y,
    const float z,
    const float width,
    const float height,
    const float depth,
    PolygonRequest * stack_recipient)
{
    log_assert(z > 0.0f);
    
    construct_zpolygon(stack_recipient);
    
    stack_recipient->gpu_data->xyz[0]  = mid_x;
    stack_recipient->gpu_data->xyz[1]  = mid_y;
    stack_recipient->gpu_data->xyz[2]  = z;
    stack_recipient->cpu_data->visible = true;
    
    // the hardcoded quad offsets range from -1.0f to 1.0f,
    // so the current width is 2.0f
    float current_width = 2.0f;
    float current_height = 2.0f;
    float current_depth = 2.0f;
    stack_recipient->gpu_data->xyz_multiplier[0] = width / current_width;
    stack_recipient->gpu_data->xyz_multiplier[1] = height / current_height;
    stack_recipient->gpu_data->xyz_multiplier[2] = depth / current_depth;
    
    stack_recipient->cpu_data->mesh_id = 1;
    stack_recipient->gpu_materials[0].rgba[0] = 1.0f;
    stack_recipient->gpu_materials[0].rgba[1] = 1.0f;
    stack_recipient->gpu_materials[0].rgba[2] = 1.0f;
    stack_recipient->gpu_materials[0].rgba[3] = 1.0f;
    stack_recipient->gpu_materials[0].texturearray_i = -1;
    stack_recipient->gpu_materials[0].texture_i = -1;
}
