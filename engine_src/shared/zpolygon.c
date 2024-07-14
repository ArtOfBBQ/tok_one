#include "zpolygon.h"

zPolygonCollection * zpolygons_to_render = NULL;

void set_zpolygon_hitbox(
    zPolygonCPU * mesh_cpu,
    GPUPolygon * mesh_gpu)
{
    log_assert(all_mesh_summaries[mesh_cpu->mesh_id].vertices_size > 0);
    
    int32_t vertices_tail_i =
        all_mesh_summaries[mesh_cpu->mesh_id].vertices_head_i +
        all_mesh_summaries[mesh_cpu->mesh_id].vertices_size -
        1;
    
    for (
        int32_t vert_i = all_mesh_summaries[mesh_cpu->mesh_id].vertices_head_i;
        vert_i <= vertices_tail_i;
        vert_i++)
    {
        float cur_vertex_x =
            all_mesh_vertices->gpu_data[vert_i].xyz[0] *
            mesh_gpu->xyz_multiplier[0];
        float cur_vertex_y =
            all_mesh_vertices->gpu_data[vert_i].xyz[1] *
            mesh_gpu->xyz_multiplier[1];
        float cur_vertex_z =
            all_mesh_vertices->gpu_data[vert_i].xyz[2] *
            mesh_gpu->xyz_multiplier[2];
        
        if (cur_vertex_x < mesh_cpu->boundbox_leftbottomfront[0]) {
            mesh_cpu->boundbox_leftbottomfront[0] = cur_vertex_x;
        } else if (cur_vertex_x > mesh_cpu->boundbox_righttopback[0]) {
            mesh_cpu->boundbox_righttopback[0] = cur_vertex_x;
        }
        
        if (cur_vertex_y < mesh_cpu->boundbox_leftbottomfront[1]) {
            mesh_cpu->boundbox_leftbottomfront[1] = cur_vertex_y;
        } else if (cur_vertex_y > mesh_cpu->boundbox_righttopback[1]) {
            mesh_cpu->boundbox_righttopback[1] = cur_vertex_y;
        }
        
        if (cur_vertex_z < mesh_cpu->boundbox_leftbottomfront[2]) {
            mesh_cpu->boundbox_leftbottomfront[2] = cur_vertex_z;
        } else if (cur_vertex_z > mesh_cpu->boundbox_righttopback[2]) {
            mesh_cpu->boundbox_righttopback[2] = cur_vertex_z;
        }
    }
    
    log_assert(mesh_cpu->boundbox_leftbottomfront[0] <= 0.0f);
    log_assert(mesh_cpu->boundbox_righttopback[0] >= 0.0f);
    log_assert(
        mesh_cpu->boundbox_leftbottomfront[0] <
            mesh_cpu->boundbox_righttopback[0]);
    
    log_assert(mesh_cpu->boundbox_leftbottomfront[1] <= 0.0f);
    log_assert(mesh_cpu->boundbox_righttopback[1] >= 0.0f);
    log_assert(
        mesh_cpu->boundbox_leftbottomfront[1] <
            mesh_cpu->boundbox_righttopback[1]);
    
    log_assert(mesh_cpu->boundbox_righttopback[2] >= 0.0f);
    log_assert(mesh_cpu->boundbox_leftbottomfront[2] <= 0.0f);
    log_assert(
        mesh_cpu->boundbox_righttopback[2] >=
            mesh_cpu->boundbox_leftbottomfront[2]);
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
    set_zpolygon_hitbox(to_commit->cpu_data, to_commit->gpu_data);
    
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

static bool32_t ray_to_right_from_origin_intersects_zpolygon_boundingbox(
    const float bb_leftbottomfront[3],
    const float bb_righttopback[3],
    float * distance_t)
{
    /*
    We are checking if a ray starting at the origin [0, 0, 0] and moving
    straight to the right direction [1, 0, 0] will intersect with given
    GPUPolygon's bounding box.
    */
    
    float left =
        ((bb_leftbottomfront[0] < bb_righttopback[0]) * bb_leftbottomfront[0]) +
        ((bb_righttopback[0] < bb_leftbottomfront[0]) * bb_righttopback[0]);
    float right =
        ((bb_leftbottomfront[0] > bb_righttopback[0]) * bb_leftbottomfront[0]) +
        ((bb_righttopback[0] > bb_leftbottomfront[0]) * bb_righttopback[0]);
    float bottom =
        ((bb_leftbottomfront[1] < bb_righttopback[1]) * bb_leftbottomfront[1]) +
        ((bb_righttopback[1] < bb_leftbottomfront[1]) * bb_righttopback[1]);
    float top =
        ((bb_leftbottomfront[1] > bb_righttopback[1]) * bb_leftbottomfront[1]) +
        ((bb_righttopback[1] > bb_leftbottomfront[1]) * bb_righttopback[1]);
    float front =
        ((bb_leftbottomfront[2] < bb_righttopback[2]) * bb_leftbottomfront[2]) +
        ((bb_righttopback[2] < bb_leftbottomfront[2]) * bb_righttopback[2]);
    float back =
        ((bb_leftbottomfront[2] > bb_righttopback[2]) * bb_leftbottomfront[2]) +
        ((bb_righttopback[2] > bb_leftbottomfront[2]) * bb_righttopback[2]);
    
    log_assert(left < right);
    log_assert(bottom < top);
    log_assert(front < back);
    
    if (
        right < 0.0f ||
        front > 0.0f ||
        back < 0.0f ||
        top < 0.0f ||
        bottom > 0.0f)
    {
        /*
        This can't possibly be a hit anymore
        */
        return false;
    }
    
    /*
    This can't possibly be a miss anymore, but it becomes difficult to calculate
    the exact distance.
    
    TODO: calculate real distance or abandon this method
    */
    *distance_t = left;
    
    // set to 0.0 if < 0.0
    *distance_t += ((*distance_t < 0.0f) * -*distance_t);
    
    return true;
}

#if 1
bool32_t ray_intersects_zpolygon_hitbox(
    const float ray_origin[3],
    const float ray_direction[3],
    const zPolygonCPU * cpu_data,
    const GPUPolygon  * gpu_data,
    float * recipient_hit_point)
{
    // Step 1: translate everything so that the origin becomes [0. 0, 0]
    float translated_ray_dir[3];
    memcpy(translated_ray_dir, ray_direction, sizeof(float) * 3);
    
    float bb_leftbottomfront[3];
    memcpy(
        bb_leftbottomfront,
        cpu_data->boundbox_leftbottomfront,
        sizeof(float) * 3);
    
    float bb_righttopback[3];
    memcpy(
        bb_righttopback,
        cpu_data->boundbox_righttopback,
        sizeof(float) * 3);
    
    bb_leftbottomfront[0] += gpu_data->xyz[0];
    bb_leftbottomfront[1] += gpu_data->xyz[0];
    bb_leftbottomfront[2] += gpu_data->xyz[0];
    bb_righttopback[0] += gpu_data->xyz[0];
    bb_righttopback[1] += gpu_data->xyz[0];
    bb_righttopback[2] += gpu_data->xyz[0];
    
    translated_ray_dir[0] -= ray_origin[0];
    translated_ray_dir[1] -= ray_origin[1];
    translated_ray_dir[2] -= ray_origin[2];
    
    bb_leftbottomfront[0] -= ray_origin[0];
    bb_leftbottomfront[1] -= ray_origin[1];
    bb_leftbottomfront[2] -= ray_origin[2];
    bb_righttopback[0] -= ray_origin[0];
    bb_righttopback[1] -= ray_origin[1];
    bb_righttopback[2] -= ray_origin[2];
    
    // Step 2: rotate everything around the origin so that the direction
    // becomes [1, 0, 0]
    
    /*
    y
    |
   6|  /
   4| /
   1|/
    ----------------x
     1234
    
    In this example, we show a vector [4, 6, 0]. We want to know the angle
    between this vector and the x-axis.
    
    We will then z-rotate (rotate around z-axis) for negative that amount and
    our new vector's Y will be 0.
    
    Depending on which quadrant our ray points in, the math will be different:
              y
          ¥  8|   /
           ¥ 6|  /
            ¥4| /
             ¥|/
     ---------|---------------x
             ¥|¥
            ¥ | ¥
           ¥-6|  ¥
          ¥ -8|   ¥
             -y
         -4321 1234
    
    For the top right scenario, we make a TOA triangle. The opposite (8) and the
    adjacent (4) means tan(angle) = 8 / 4
    that means angle = arctan(2.0f) = 1.107 radians
    When we rotate our ray by 1.107 radians, it will be aligned to the x-axis
    
    For the bottom right scenario, if we blindly do the same thing again:
    atan(-8.0 / 4.0f) = atan(-2.0f) = -1.107.
    If we rotate our ray by -1.107, we again nicely align to the x-axis
    
    For the top left scenario, if we calculate the same thing, we're are
    indeed aligning to the x-axis again, but this time we'll be pointing left,
    in the wrong direction
    
    For the bottom left scenario, again we are finding the angle that rotates
    us to the x-axis, but again pointing in the left direction
    
    So basically we can calculate by:
    Step 1:
    atan(ray y / ray x)
    
    Step 2:
    add 3.14159 if and only if the ray was pointing left to begin with
    
    
    Next, we want to repeat the entire process, except we're now rotating
    around the y-axis. Our goal this time is to set the Z value to 0.
    
             +z
              |   /
              |  /
              | /
              |/
   -x---------|---------------+x
              |¥
              | ¥
            -6|  ¥
            -8|   ¥
             -z
         -4321 1234
    
    You can see on this picture that the opposite and adjacent this time are:
    opposite -> z -> 8
    adjacent -> x -> 4
    
    So it's exactly the same as the previous step, except we replace y values
    with z values and we replace z_rotate with y_rotate
    
    edit: Actually it's different in that we need to add in the angle, not
    subtract it. I don't really understand why
    
    Note also that I didn't draw the left side, because we forced the vector
    to point to the right (positive x) in the previous step
    
    Since z will be 0 in the end, I don't think it matters if we align to the
    top or to the bottom
    */
    
    // log_assert(translated_ray_dir[1] > 0.0f); // otherwise it's not arctan!!
    float angle_around_z_axis_to_x_axis = atanf(
        translated_ray_dir[1] / translated_ray_dir[0]);
    angle_around_z_axis_to_x_axis +=
        (translated_ray_dir[0] < 0.0f) * 3.14159f;
    z_rotate_zvertex_f3(bb_leftbottomfront, -angle_around_z_axis_to_x_axis);
    z_rotate_zvertex_f3(bb_righttopback,    -angle_around_z_axis_to_x_axis);
    z_rotate_zvertex_f3(
        translated_ray_dir,
        -angle_around_z_axis_to_x_axis);
    
    log_assert(translated_ray_dir[0] >  0.01f); // pointing right
    log_assert(translated_ray_dir[1] <  0.01f); // y is 0, aligned to x-axis
    log_assert(translated_ray_dir[1] > -0.01f); // y is 0, aligned to x-axis
    
    float angle_around_y_axis_to_x_axis = atanf(
        translated_ray_dir[2] / translated_ray_dir[0]);
    // y rotation will leave y at 0
    y_rotate_zvertex_f3(bb_leftbottomfront, angle_around_y_axis_to_x_axis);
    y_rotate_zvertex_f3(bb_righttopback,    angle_around_y_axis_to_x_axis);
    y_rotate_zvertex_f3(
        translated_ray_dir,
        angle_around_y_axis_to_x_axis);
    log_assert(translated_ray_dir[0] >  0.01f); // pointing right
    log_assert(translated_ray_dir[1] <  0.01f); // aligned to x-axis
    log_assert(translated_ray_dir[1] > -0.01f); // aligned to x-axis
    log_assert(translated_ray_dir[2] <  0.01f); // aligned to x-axis
    log_assert(translated_ray_dir[2] > -0.01f); // aligned to x-axis
    
    // Step 2: check for collision
    float distance_t;
    bool32_t return_value =
        ray_to_right_from_origin_intersects_zpolygon_boundingbox(
            /* const float bb_leftbottomfront[3]: */
                bb_leftbottomfront,
            /* const float bb_righttopback[3]: */
                bb_righttopback,
            /* float * distance_t: */
                &distance_t);
    
    return return_value;
}
#else
bool32_t ray_intersects_zpolygon_hitbox(
    const float ray_origin[3],
    const float ray_direction[3],
    const zPolygonCPU * cpu_data,
    const GPUPolygon  * gpu_data,
    float * recipient_hit_point)
{
    /*
    Reminder: The plane offset is named 'D' in this explanation:
    "...and D is the distance from the origin (0, 0, 0) to the
    plane (if we trace a line from the origin to the plane,
    parallel to the plane's normal)."
    "..we know the plane's normal and that the three triangle's
    vertices (V0, V1, V2) lie in the plane. It is, therefore,
    possible to compute  D. Any of the three vertices can be
    chosen. Let's choose V0:
    float D = -dotProduct(N, v0);"
    (source https://www.scratchapixel.com
    */
    float mesh_center[3];
    memcpy(mesh_center, gpu_data->xyz, sizeof(float) * 3);
    mesh_center[0] += gpu_data->xyz_offset[0];
    mesh_center[1] += gpu_data->xyz_offset[1];
    mesh_center[2] += gpu_data->xyz_offset[2];
    
    float plane_normals[6][3];
    float plane_offsets[6];
    float t_values[6];
    bool32_t hit_plane[6];
    float hit_points[6][3];
    
    // These are normals, but they're also offsets to turn the mesh center
    // into the plane for that box face
    // Plane facing towards camera
    plane_normals[0][0] = 0.0f;
    plane_normals[0][1] = 0.0f;
    plane_normals[0][2] = cpu_data->boundbox_leftbottomfront[2];
    // Plane facing away from camera
    plane_normals[1][0] = 0.0f;
    plane_normals[1][1] = 0.0f;
    plane_normals[1][2] = cpu_data->boundbox_righttopback[2];
    // Plane facing up
    plane_normals[2][0] = 0.0f;
    plane_normals[2][1] = cpu_data->boundbox_righttopback[1];
    plane_normals[2][2] = 0.0f;
    // Plane facing down
    plane_normals[3][0] = 0.0f;
    plane_normals[3][1] = cpu_data->boundbox_leftbottomfront[1];
    plane_normals[3][2] = 0.0f;
    // Plane facing left
    plane_normals[4][0] = cpu_data->boundbox_leftbottomfront[0];
    plane_normals[4][1] = 0.0f;
    plane_normals[4][2] = 0.0f;
    // Plane facing right
    plane_normals[5][0] = cpu_data->boundbox_righttopback[0];
    plane_normals[5][1] = 0.0f;
    plane_normals[5][2] = 0.0f;
    
    #define PLANES_TO_CHECK 6
    for (uint32_t p = 0; p < PLANES_TO_CHECK; p++) {
        log_assert(plane_normals[p][0] == plane_normals[p][0]);
        log_assert(plane_normals[p][1] == plane_normals[p][1]);
        log_assert(plane_normals[p][2] == plane_normals[p][2]);
        
        x_rotate_zvertex_f3(plane_normals[p], gpu_data->xyz_angle[0]);
        log_assert(plane_normals[p][0] == plane_normals[p][0]);
        log_assert(plane_normals[p][1] == plane_normals[p][1]);
        log_assert(plane_normals[p][2] == plane_normals[p][2]);
        y_rotate_zvertex_f3(plane_normals[p], gpu_data->xyz_angle[1]);
        log_assert(plane_normals[p][0] == plane_normals[p][0]);
        log_assert(plane_normals[p][1] == plane_normals[p][1]);
        log_assert(plane_normals[p][2] == plane_normals[p][2]);
        z_rotate_zvertex_f3(plane_normals[p], gpu_data->xyz_angle[2]);
    }
    
    for (uint32_t p = 0; p < PLANES_TO_CHECK; p++) {
        
        hit_plane[p] = false;
        
        // before normalizing, offset to find the plane we're checking
        float current_plane_origin[3];
        memcpy(current_plane_origin, gpu_data->xyz, sizeof(float) * 3);
        current_plane_origin[0] += plane_normals[p][0];
        current_plane_origin[1] += plane_normals[p][1];
        current_plane_origin[2] += plane_normals[p][2];
        
        // now we can normalize the offsets and use them as our normal value
        float normalized_plane_normal[3];
        memcpy(normalized_plane_normal, plane_normals, sizeof(float) * 3);
        normalize_vertex(normalized_plane_normal);
        log_assert(normalized_plane_normal[0] == normalized_plane_normal[0]);
        log_assert(normalized_plane_normal[1] == normalized_plane_normal[1]);
        log_assert(normalized_plane_normal[2] == normalized_plane_normal[2]);
        
        /*
        A plane is defined as:
        A*x + B*x + C*z + D = 0
        or:
        D = -(A*x + B*y + C*z)
        
        and since the normal is pointing to the plane, its xyz values are
        literally the same as ABC
        
        for xyz we can use any point inside the plane, so let's use the plane
        origin
        
        Distance from Origin
        "If the normal vector is normalized (unit length), then the constant
        term of the plane equation, d becomes the distance from the origin."
        */
        plane_offsets[p] = -dot_of_vertices_f3(
            normalized_plane_normal,
            current_plane_origin);
        
        /*
        We also know that point P is the intersection point of the ray, and the 
        point lies in the plane. Consequently, we can substitute (x, y, z)
        for P or O + tR that P is equal to and solve for t
        
        after shuffling around you can get:
        t = (p0 - lo) dot N / l dot N
        
        where l is the ray (lo is ray origin, l is ray direction) and
        N is the plane's normal and p0 is the plane origin
        
        or from another source on triangle intersection (I don't understand
        the difference because the triangle solution uses a plane first):
        float t = -(dot(N, orig) + D) / dot(N, dir);
        
        these give the same results so that part must be accurate!
        
          ___
        q(t.t)p
          ( )
          d.b
        */
        float denominator = dot_of_vertices_f3(
            normalized_plane_normal,
            ray_direction);
        
        if (
            denominator <  0.0001f &&
            denominator > -0.0001f)
        {
            // the ray doesn't intersect with the triangle's plane,
            // I think this is always because the ray travels in parallel with
            // the triangle
            continue;
        }
        
        float plane_offset_minus_ray_origin[3];
        memcpy(
            plane_offset_minus_ray_origin,
            current_plane_origin,
            sizeof(float) * 3);
        plane_offset_minus_ray_origin[0] -= ray_origin[0];
        plane_offset_minus_ray_origin[1] -= ray_origin[1];
        plane_offset_minus_ray_origin[2] -= ray_origin[2];
        
        t_values[p] = -(
            dot_of_vertices_f3(normalized_plane_normal, ray_origin) +
                plane_offsets[p]);
        t_values[p] /= denominator;
        
        #ifndef LOGGER_IGNORE_ASSERTS
        // This should give about the same result:
        float t_values_alternative = dot_of_vertices_f3(
            normalized_plane_normal,
            plane_offset_minus_ray_origin) /
                denominator;
        
        float diff = t_values[p] - t_values_alternative;
        log_assert(diff <  0.1f);
        log_assert(diff > -0.1f);
        #endif
        
        // if t is < 0, the triangle's plane must be behind us which counts as
        // a miss
        if (t_values[p] < 0.0f) {
            continue;
        }
        
        // We now have computed t, which we can use to calculate the position
        // of P:
        // Vec3f Phit = orig + t * dir;
        float raw_collision_point[3];
        raw_collision_point[0] = ray_origin[0] +
            (t_values[p] * ray_direction[0]);
        raw_collision_point[1] = ray_origin[1] +
            (t_values[p] * ray_direction[1]);
        raw_collision_point[2] = ray_origin[2] +
            (t_values[p] * ray_direction[2]);
        
        memcpy(hit_points[p], raw_collision_point, sizeof(float) * 3);
        
        float center_to_hit_ray[3];
        center_to_hit_ray[0] = raw_collision_point[0] - gpu_data->xyz[0];
        center_to_hit_ray[1] = raw_collision_point[1] - gpu_data->xyz[1];
        center_to_hit_ray[2] = raw_collision_point[2] - gpu_data->xyz[2];
        
        float reverse_rotated_center_to_hit[3];
        memcpy(
            reverse_rotated_center_to_hit,
            center_to_hit_ray,
            sizeof(float) * 3);
        x_rotate_zvertex_f3(
            reverse_rotated_center_to_hit,
            -gpu_data->xyz_angle[0]);
        y_rotate_zvertex_f3(
            reverse_rotated_center_to_hit,
            -gpu_data->xyz_angle[1]);
        z_rotate_zvertex_f3(
            reverse_rotated_center_to_hit,
            -gpu_data->xyz_angle[2]);
        
        // Now I just want to check if this hit is within the zpolygon's
        // hitbox
        // it's now an 'axis aligned' hitbox (since we rotated the collision
        // point and not the hitbox itself), so we can just check the bounds
        float tolerance = 0.002f;
        if (
            (reverse_rotated_center_to_hit[0] - tolerance) <=
                 cpu_data->boundbox_righttopback[0] &&
            (reverse_rotated_center_to_hit[0] + tolerance) >=
                cpu_data->boundbox_leftbottomfront[0] &&
            (reverse_rotated_center_to_hit[1] - tolerance) <=
                cpu_data->boundbox_righttopback[1] &&
            (reverse_rotated_center_to_hit[1] + tolerance) >=
                cpu_data->boundbox_leftbottomfront[1] &&
            (reverse_rotated_center_to_hit[2] - tolerance) <=
                 cpu_data->boundbox_righttopback[2] &&
            (reverse_rotated_center_to_hit[2] + tolerance) >=
                cpu_data->boundbox_leftbottomfront[2])
        {
            hit_plane[p] = true;
        }
    }
    
    bool32_t return_value = false;
    float closest_t = FLOAT32_MAX;
    
    for (uint32_t p = 0; p < PLANES_TO_CHECK; p++) {
        if (
            hit_plane[p] &&
            t_values[p] < closest_t)
        {
            return_value = true;
            memcpy(recipient_hit_point, hit_points[p], sizeof(float) * 3);
            closest_t = t_values[p];
        }
    }
    
    return return_value;
}
#endif

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
    stack_recipient->gpu_data->xyz_multiplier[2] = 1.0f;
    
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
