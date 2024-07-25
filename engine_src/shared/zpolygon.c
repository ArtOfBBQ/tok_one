#include "zpolygon.h"

zPolygonCollection * zpolygons_to_render = NULL;

void set_zpolygon_hitbox(
    zPolygonCPU * mesh_cpu,
    GPUPolygon * mesh_gpu)
{
    log_assert(all_mesh_summaries[mesh_cpu->mesh_id].vertices_size > 0);
    
    int32_t vertices_tail_i =
        all_mesh_summaries[mesh_cpu->mesh_id].vertices_head_i +
        all_mesh_summaries[mesh_cpu->mesh_id].vertices_size;
    
    mesh_cpu->furthest_vertex_xyz[0] = 0.0f;
    mesh_cpu->furthest_vertex_xyz[1] = 0.0f;
    mesh_cpu->furthest_vertex_xyz[2] = 0.0f;
    float furthest_squares = 0.0f;
    
    for (
        int32_t vert_i = all_mesh_summaries[mesh_cpu->mesh_id].vertices_head_i;
        vert_i < vertices_tail_i;
        vert_i++)
    {
        for (uint32_t m = 0; m < 3; m++) {
            float squares =
                (all_mesh_vertices->gpu_data[vert_i].xyz[0] *
                    all_mesh_vertices->gpu_data[vert_i].xyz[0]) +
                (all_mesh_vertices->gpu_data[vert_i].xyz[1] *
                    all_mesh_vertices->gpu_data[vert_i].xyz[1]) +
                (all_mesh_vertices->gpu_data[vert_i].xyz[2] *
                    all_mesh_vertices->gpu_data[vert_i].xyz[2]);
            
            if (squares > furthest_squares) {
                furthest_squares = squares;
                mesh_cpu->furthest_vertex_xyz[0] =
                    all_mesh_vertices->gpu_data[vert_i].xyz[0];
                mesh_cpu->furthest_vertex_xyz[1] =
                    all_mesh_vertices->gpu_data[vert_i].xyz[1];
                mesh_cpu->furthest_vertex_xyz[2] =
                    all_mesh_vertices->gpu_data[vert_i].xyz[2];
                
                #ifndef LOGGER_IGNORE_ASSERTS
                if (fabs(mesh_cpu->furthest_vertex_xyz[0]) > 250.0f ||
                    fabs(mesh_cpu->furthest_vertex_xyz[1]) > 250.0f ||
                    fabs(mesh_cpu->furthest_vertex_xyz[2]) > 250.0f)
                {
                    log_assert(0);
                }
                #endif
            }
        }
    }
}

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

void commit_zpolygon_to_render(PolygonRequest * to_commit)
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
            log_assert(to_commit->gpu_materials[mat_i].rgba[rgba_i] >= -0.1f);
            log_assert(to_commit->gpu_materials[mat_i].rgba[rgba_i] <=  1.1f);
        }
    }
    
    // set the hitbox height, width, and depth
    if (to_commit->cpu_data->touchable_id >= 0) {
        set_zpolygon_hitbox(to_commit->cpu_data, to_commit->gpu_data);
    }
    
    to_commit->cpu_data->committed = true;
}

bool32_t fetch_zpolygon_by_object_id(
    PolygonRequest * recipient,
    const int32_t object_id)
{
    memset(recipient, 0, sizeof(PolygonRequest));
    
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
    
    memset(
        to_construct->cpu_data,
        0,
        sizeof(zPolygonCPU));
    memset(
        to_construct->gpu_materials,
        0,
        sizeof(GPUPolygonMaterial) *
            to_construct->materials_size);
    memset(
        to_construct->gpu_data,
        0,
        sizeof(GPUPolygon));
    
    to_construct->gpu_data->xyz_multiplier[0] = 1.0f;
    to_construct->gpu_data->xyz_multiplier[1] = 1.0f;
    to_construct->gpu_data->xyz_multiplier[2] = 1.0f;
    to_construct->gpu_data->scale_factor = 1.0f;
    
    to_construct->cpu_data->mesh_id = -1;
    to_construct->cpu_data->object_id = -1;
    to_construct->cpu_data->touchable_id = -1;
    to_construct->cpu_data->visible = true;
    
    to_construct->gpu_materials[0].rgba[0] = 0.75f;
    to_construct->gpu_materials[0].rgba[1] = 0.75f;
    to_construct->gpu_materials[0].rgba[2] = 0.75f;
    to_construct->gpu_materials[0].rgba[3] = 0.75f;
    to_construct->gpu_materials[0].diffuse = 0.75f;
    to_construct->gpu_materials[0].specular = 0.75f;
    to_construct->gpu_materials[0].texture_i = -1;
    to_construct->gpu_materials[0].texturearray_i = -1;
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

static void normal_undo_camera_rotation(
    float normal_xyz[3],
    float ignore_camera)
{
    log_assert(ignore_camera >= 0.0f);
    log_assert(ignore_camera <= 1.0f);
    
    float ignore_cam_pos[3];
    memcpy(ignore_cam_pos, normal_xyz, sizeof(float) * 3);
    
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

static void undo_camera_translation(
    float xyz[3],
    float ignore_camera)
{
    float ignore_cam_pos[3];
    memcpy(ignore_cam_pos, xyz, sizeof(float) * 3);
    
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
}

void zpolygon_get_transformed_triangle_vertices(
    const zPolygonCPU * cpu_data,
    const GPUPolygon * gpu_data,
    const int32_t locked_vertex_i,
    float * vertices_recipient_9f,
    float * normals_recipient_9f)
{
    for (int32_t i = 0; i < 9; i++) {
        vertices_recipient_9f[i] = all_mesh_vertices->gpu_data[
            locked_vertex_i + (i / 3)].xyz[i % 3];
        vertices_recipient_9f[i] *= gpu_data->xyz_multiplier[i % 3];
        vertices_recipient_9f[i] += gpu_data->xyz_offset[i % 3];
        vertices_recipient_9f[i] *= gpu_data->scale_factor;
        
        normals_recipient_9f[i] = all_mesh_vertices->gpu_data[
            locked_vertex_i + (i / 3)].normal_xyz[i % 3];
    }
    
    normalize_zvertex_f3(normals_recipient_9f);
    normalize_zvertex_f3(normals_recipient_9f+3);
    normalize_zvertex_f3(normals_recipient_9f+6);
    
    x_rotate_f3(vertices_recipient_9f  , gpu_data->xyz_angle[0]);
    y_rotate_f3(vertices_recipient_9f  , gpu_data->xyz_angle[1]);
    z_rotate_f3(vertices_recipient_9f  , gpu_data->xyz_angle[2]);
    x_rotate_f3(vertices_recipient_9f+3, gpu_data->xyz_angle[0]);
    y_rotate_f3(vertices_recipient_9f+3, gpu_data->xyz_angle[1]);
    z_rotate_f3(vertices_recipient_9f+3, gpu_data->xyz_angle[2]);
    x_rotate_f3(vertices_recipient_9f+6, gpu_data->xyz_angle[0]);
    y_rotate_f3(vertices_recipient_9f+6, gpu_data->xyz_angle[1]);
    z_rotate_f3(vertices_recipient_9f+6, gpu_data->xyz_angle[2]);
    
    x_rotate_f3(normals_recipient_9f  , gpu_data->xyz_angle[0]);
    y_rotate_f3(normals_recipient_9f  , gpu_data->xyz_angle[1]);
    z_rotate_f3(normals_recipient_9f  , gpu_data->xyz_angle[2]);
    x_rotate_f3(normals_recipient_9f+3, gpu_data->xyz_angle[0]);
    y_rotate_f3(normals_recipient_9f+3, gpu_data->xyz_angle[1]);
    z_rotate_f3(normals_recipient_9f+3, gpu_data->xyz_angle[2]);
    x_rotate_f3(normals_recipient_9f+6, gpu_data->xyz_angle[0]);
    y_rotate_f3(normals_recipient_9f+6, gpu_data->xyz_angle[1]);
    z_rotate_f3(normals_recipient_9f+6, gpu_data->xyz_angle[2]);
    
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
}

void zpolygon_get_transformed_boundsphere(
    const zPolygonCPU * cpu_data,
    const GPUPolygon * gpu_data,
    float * recipient_center_xyz,
    float * recipient_radius)
{
    memcpy(
        recipient_center_xyz,
        gpu_data->xyz_offset,
        sizeof(float)*3);
    
    recipient_center_xyz[0] *= gpu_data->scale_factor;
    recipient_center_xyz[1] *= gpu_data->scale_factor;
    recipient_center_xyz[2] *= gpu_data->scale_factor;
    
    x_rotate_f3(recipient_center_xyz, gpu_data->xyz_angle[0]);
    y_rotate_f3(recipient_center_xyz, gpu_data->xyz_angle[1]);
    z_rotate_f3(recipient_center_xyz, gpu_data->xyz_angle[2]);
    
    recipient_center_xyz[0] += gpu_data->xyz[0];
    recipient_center_xyz[1] += gpu_data->xyz[1];
    recipient_center_xyz[2] += gpu_data->xyz[2];
    
    // 'ignore camera' can be reframed as undoing the effects of the camera
    undo_camera_translation(recipient_center_xyz, gpu_data->ignore_camera);
    
    float furthest[3];
    memcpy(furthest, cpu_data->furthest_vertex_xyz, sizeof(float)*3);
    
    furthest[0] *= gpu_data->xyz_multiplier[0];
    furthest[1] *= gpu_data->xyz_multiplier[1];
    furthest[2] *= gpu_data->xyz_multiplier[2];
    
    furthest[0] *= gpu_data->scale_factor;
    furthest[1] *= gpu_data->scale_factor;
    furthest[2] *= gpu_data->scale_factor;
    
    *recipient_radius = sqrtf(
        (furthest[0] * furthest[0]) +
        (furthest[1] * furthest[1]) +
        (furthest[2] * furthest[2]));
}

float ray_intersects_zpolygon(
    const float ray_origin[3],
    float ray_direction[3],
    const zPolygonCPU * cpu_data,
    GPUPolygon  * gpu_data,
    float * recipient_hit_point)
{
    memset(recipient_hit_point, 0, sizeof(float) * 3);
    
    gpu_data->last_clicked_locked_vertex_id = -1.0f;
    
    normalize_zvertex_f3(ray_direction);
    
    float sphere_center[3];
    float sphere_radius;
    zpolygon_get_transformed_boundsphere(
        /* const zPolygonCPU * cpu_data: */
            cpu_data,
        /* const GPUPolygon * gpu_data: */
            gpu_data,
        /* float * recipient_center_xyz: */
            sphere_center,
        /* float * recipient_radius: */
            &sphere_radius);
    
    float dist_to_hit = normalized_ray_hits_sphere(
        /* const float * ray_origin: */
            ray_origin,
        /* const float * normalized_ray_direction: */
            ray_direction,
        /* const float * sphere_origin: */
            sphere_center,
        /* const float sphere_radius: */
            sphere_radius,
        /* float * collision_recipient: */
            recipient_hit_point);
    
    if (dist_to_hit > 0.0f && dist_to_hit < (COL_FLT_MAX / 2)) {
        dist_to_hit = COL_FLT_MAX;
        
        float closest_hit_point[3];
        memset(closest_hit_point, 0, sizeof(float)*3);
        
        for (
            int32_t vert_i =
                all_mesh_summaries[cpu_data->mesh_id].vertices_head_i;
            vert_i <
                all_mesh_summaries[cpu_data->mesh_id].vertices_head_i +
                all_mesh_summaries[cpu_data->mesh_id].vertices_size;
            vert_i += 3)
        {
            float transformed_triangle[9];
            float transformed_normals[9];
            
            zpolygon_get_transformed_triangle_vertices(
                /* const zPolygonCPU * cpu_data: */
                    cpu_data,
                /* const GPUPolygon * gpu_data: */
                    gpu_data,
                /* const unsigned int locked_vertex_i: */
                    vert_i,
                /* float * vertices_recipient_f9: */
                    transformed_triangle,
                /* float * normals_recipient_f9: */
                    transformed_normals);
            
            float avg_normal[3];
            avg_normal[0] =
                (transformed_normals[0] +
                    transformed_normals[3] +
                        transformed_normals[6]) /
                    3.0f;
            avg_normal[1] =
                (transformed_normals[1] +
                    transformed_normals[4] +
                        transformed_normals[7]) /
                    3.0f;
            avg_normal[2] =
                (transformed_normals[2] +
                    transformed_normals[5] +
                        transformed_normals[8]) /
                    3.0f;
            normalize_zvertex_f3(avg_normal);
            
            float dist_to_tri = ray_hits_triangle(
                /* const float * ray_origin: */
                    ray_origin,
                /* const float * ray_direction: */
                    ray_direction,
                /* const float * triangle_vertex_1: */
                    transformed_triangle,
                /* const float * triangle_vertex_2: */
                    transformed_triangle + 3,
                /* const float * triangle_vertex_3: */
                    transformed_triangle + 6,
                /* const float * triangle_normal: */
                    avg_normal,
                /* float * collision_recipient: */
                    closest_hit_point);
            
            if (dist_to_tri > 0 && dist_to_tri < dist_to_hit)
            {
                gpu_data->last_clicked_locked_vertex_id = (float)vert_i;
                dist_to_hit = dist_to_tri;
                memcpy(
                    recipient_hit_point,
                    closest_hit_point,
                    sizeof(float)*3);
            }
        }
    }
    
    return dist_to_hit;
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
