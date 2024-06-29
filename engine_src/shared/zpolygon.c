#include "zpolygon.h"

zPolygonCollection * zpolygons_to_render = NULL;

void set_zpolygon_hitbox(
    zPolygonCPU * mesh_cpu, GPUPolygon * mesh_gpu)
{
    log_assert(all_mesh_summaries[mesh_cpu->mesh_id].vertices_size > 0);
    
    float top    = 0.0f;
    float bottom = 0.0f;
    float left   = 0.0f;
    float right  = 0.0f;
    float back   = 0.0f;
    float front  = 0.0f;
    
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
        
        if (cur_vertex_x < left) {
            left = cur_vertex_x;
        } else if (cur_vertex_x > right) {
            right = cur_vertex_x;
        }
        
        if (cur_vertex_y < bottom) {
            bottom = cur_vertex_y;
        } else if (cur_vertex_y > top) {
            top = cur_vertex_y;
        }
        
        if (cur_vertex_z < front) {
            front = cur_vertex_z;
        } else if (cur_vertex_z > back) {
            back = cur_vertex_z;
        }
    }
    
    log_assert(bottom <= 0.0f);
    log_assert(top    >= 0.0f);
    
    log_assert(left   <= 0.0f);
    log_assert(right  >= 0.0f);
    
    log_assert(back   >= 0.0f);
    log_assert(front  <= 0.0f);
    log_assert(back   >= front);
    
    mesh_cpu->hitbox_height = (top - bottom) + 0.001f;
    mesh_cpu->hitbox_width  = (right - left) + 0.001f;
    mesh_cpu->hitbox_depth  = (back - front) + 0.001f;
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

zTriangle
x_rotate_ztriangle(
    const zTriangle * input,
    const float angle)
{
    zTriangle return_value = *input;
    
    if (angle == 0.0f) {
        return return_value;
    }
    
    for (
        uint32_t i = 0;
        i < 3;
        i++)
    {
        return_value.vertices[i] = x_rotate_zvertex(
            &return_value.vertices[i],
            angle);
        
        return_value.normal = x_rotate_zvertex(
            &return_value.normal,
            angle);
    }
    
    return return_value;
}


zTriangle
z_rotate_ztriangle(
    const zTriangle * input,
    const float angle)
{
    zTriangle return_value = *input;
    
    if (angle == 0.0f) {
        return return_value;
    }
    
    for (
        uint32_t i = 0;
        i < 3;
        i++)
    {
        return_value.vertices[i] = z_rotate_zvertex(
            &return_value.vertices[i],
            angle);
        
        return_value.normal = z_rotate_zvertex(
            &return_value.normal,
            angle);
    }
    
    return return_value;
}

zTriangle
y_rotate_ztriangle(
    const zTriangle * input,
    const float angle)
{
    zTriangle return_value = *input;
    
    if (angle == 0.0f) {
        return return_value;
    }
    
    for (
        uint32_t i = 0;
        i < 3;
        i++)
    {
        return_value.vertices[i] = y_rotate_zvertex(
            &return_value.vertices[i],
            angle);
        
        return_value.normal = y_rotate_zvertex(
            &return_value.normal,
            angle);
    }
    
    return return_value;
}

zTriangle translate_ztriangle(
    const zTriangle * input,
    const float by_x,
    const float by_y,
    const float by_z)
{
    zTriangle return_value = *input;
    
    for (uint32_t i = 0; i < 3; i++) {
        return_value.vertices[i].x += by_x;
        return_value.vertices[i].y += by_y;
        return_value.vertices[i].z += by_z;
    }
    
    return return_value;
}

float get_avg_z(
    const zTriangle * of_triangle)
{
    return (
        of_triangle->vertices[0].z +
        of_triangle->vertices[1].z +
        of_triangle->vertices[2].z) / 3.0f;
}

int sorter_cmpr_lowest_z(
    const void * a,
    const void * b)
{
    return get_avg_z((zTriangle *)a) < get_avg_z((zTriangle *)b) ? -1 : 1;
}

float get_distance(
    const zVertex p1,
    const zVertex p2)
{
    return sqrtf(
        ((p1.x - p2.x) * (p1.x - p2.x))
        + ((p1.y - p2.y) * (p1.y - p2.y))
        + ((p1.z - p2.z) * (p1.z - p2.z)));
}

float distance_to_ztriangle(
    const zVertex p1,
    const zTriangle p2)
{
    return (
        get_distance(p1, p2.vertices[0]) +
        get_distance(p1, p2.vertices[1]) +
        get_distance(p1, p2.vertices[2])) / 3.0f;
}

zVertex get_ztriangle_normal(
    const zTriangle * input)
{
    uint32_t vertex_0 = 0;
    uint32_t vertex_1 = 1;
    uint32_t vertex_2 = 2;
    
    zVertex normal;
    zVertex vector1;
    zVertex vector2;
    
    vector1.x = input->vertices[vertex_1].x - input->vertices[vertex_0].x;
    vector1.y = input->vertices[vertex_1].y - input->vertices[vertex_0].y;
    vector1.z = input->vertices[vertex_1].z - input->vertices[vertex_0].z;
    
    vector2.x = input->vertices[vertex_2].x - input->vertices[vertex_0].x;
    vector2.y = input->vertices[vertex_2].y - input->vertices[vertex_0].y;
    vector2.z = input->vertices[vertex_2].z - input->vertices[vertex_0].z;
    
    normal.x = (vector1.y * vector2.z) - (vector1.z * vector2.y);
    normal.y = (vector1.z * vector2.x) - (vector1.x * vector2.z);
    normal.z = (vector1.x * vector2.y) - (vector1.y * vector2.x);
    
    return normal;
}

float dot_of_zvertices(
    const zVertex * a,
    const zVertex * b)
{
    float x =
        (
            a->x *
            b->x
        );
    x = (isnan(x) || !isfinite(x)) ? FLOAT32_MAX : x;
    
    float y = (a->y * b->y);
    y = (isnan(y) || !isfinite(y)) ? FLOAT32_MAX : y;
    
    float z = (a->z * b->z);
    z = (isnan(z) || !isfinite(z)) ? FLOAT32_MAX : z;
    
    float return_value = x + y + z;
    
    log_assert(!isnan(return_value));
    
    return return_value;
}

void zcamera_move_forward(
    GPUCamera * to_move,
    const float distance)
{
    // pick a point that would be in front of the camera
    // if it was not angled in any way, and if it was at
    // the origin
    zVertex forward_if_camera_was_unrotated_at_origin;
    forward_if_camera_was_unrotated_at_origin.x = 0.0f;
    forward_if_camera_was_unrotated_at_origin.y = 0.0f;
    forward_if_camera_was_unrotated_at_origin.z = distance;
    
    zVertex x_rotated = x_rotate_zvertex(
        &forward_if_camera_was_unrotated_at_origin,
        camera.xyz_angle[0]);
    zVertex y_rotated = y_rotate_zvertex(
        &x_rotated,
        camera.xyz_angle[1]);
    zVertex final = z_rotate_zvertex(
        &y_rotated,
        camera.xyz_angle[2]);
    
    // add to the camera's current position
    to_move->xyz[0] += final.x;
    to_move->xyz[1] += final.y;
    to_move->xyz[2] += final.z;
}

bool32_t ray_intersects_zpolygon_hitbox(
    const zVertex * ray_origin,
    const zVertex * ray_direction,
    const zPolygonCPU * cpu_data,
    const GPUPolygon  * gpu_data,
    zVertex * recipient_hit_point)
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
    zVertex     mesh_center;
    mesh_center.x = gpu_data->xyz[0];
    mesh_center.y = gpu_data->xyz[1];
    mesh_center.z = gpu_data->xyz[2];
    
    zVertex plane_normals[6];
    float plane_offsets[6];
    float t_values[6];
    bool32_t hit_plane[6];
    zVertex hit_points[6];
    
    // These are normals, but they're also offsets to turn the mesh center
    // into the plane for that box face
    // Plane facing towards camera
    plane_normals[0].x =                         0.0f;
    plane_normals[0].y =                         0.0f;
    plane_normals[0].z =  -cpu_data->hitbox_depth / 2;
    // Plane facing away from camera
    plane_normals[1].x =                         0.0f;
    plane_normals[1].y =                         0.0f;
    plane_normals[1].z =   cpu_data->hitbox_depth / 2;
    // Plane facing up
    plane_normals[2].x =                         0.0f;
    plane_normals[2].y =  cpu_data->hitbox_height / 2;
    plane_normals[2].z =                         0.0f;
    // Plane facing down
    plane_normals[3].x =                         0.0f;
    plane_normals[3].y = -cpu_data->hitbox_height / 2;
    plane_normals[3].z =                         0.0f;
    // Plane facing left
    plane_normals[4].x =  -cpu_data->hitbox_width / 2;
    plane_normals[4].y =                         0.0f;
    plane_normals[4].z =                         0.0f;
    // Plane facing right
    plane_normals[5].x =   cpu_data->hitbox_width / 2;
    plane_normals[5].y =                         0.0f;
    plane_normals[5].z =                         0.0f;
    
    #define PLANES_TO_CHECK 6
    for (uint32_t p = 0; p < PLANES_TO_CHECK; p++) {
        log_assert(plane_normals[p].x == plane_normals[p].x);
        log_assert(plane_normals[p].y == plane_normals[p].y);
        log_assert(plane_normals[p].z == plane_normals[p].z);
        
        plane_normals[p]    = x_rotate_zvertex(
            &plane_normals[p], gpu_data->xyz_angle[0]);
        log_assert(plane_normals[p].x == plane_normals[p].x);
        log_assert(plane_normals[p].y == plane_normals[p].y);
        log_assert(plane_normals[p].z == plane_normals[p].z);
        plane_normals[p]    = y_rotate_zvertex(
            &plane_normals[p], gpu_data->xyz_angle[1]);
        log_assert(plane_normals[p].x == plane_normals[p].x);
        log_assert(plane_normals[p].y == plane_normals[p].y);
        log_assert(plane_normals[p].z == plane_normals[p].z);
        plane_normals[p]    = z_rotate_zvertex(
            &plane_normals[p], gpu_data->xyz_angle[2]);
    }
    
    for (uint32_t p = 0; p < PLANES_TO_CHECK; p++) {
        
        hit_plane[p] = false;
        
        // before normalizing, offset to find the plane we're checking
        zVertex current_plane_origin;
        current_plane_origin.x  = gpu_data->xyz[0];
        current_plane_origin.y  = gpu_data->xyz[1];
        current_plane_origin.z  = gpu_data->xyz[2];
        current_plane_origin.x += plane_normals[p].x;
        current_plane_origin.y += plane_normals[p].y;
        current_plane_origin.z += plane_normals[p].z;
        
        // now we can normalize the offsets and use them as our normal value
        zVertex normalized_plane_normal = plane_normals[p];
        normalize_zvertex(&normalized_plane_normal);
        log_assert(normalized_plane_normal.x == normalized_plane_normal.x);
        log_assert(normalized_plane_normal.y == normalized_plane_normal.y);
        log_assert(normalized_plane_normal.z == normalized_plane_normal.z);
        
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
        plane_offsets[p] = -dot_of_zvertices(
            &normalized_plane_normal,
            &current_plane_origin);
        
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
        float denominator = dot_of_zvertices(
            &normalized_plane_normal,
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
        
        zVertex plane_offset_minus_ray_origin = current_plane_origin;
        plane_offset_minus_ray_origin.x -= ray_origin->x;
        plane_offset_minus_ray_origin.y -= ray_origin->y;
        plane_offset_minus_ray_origin.z -= ray_origin->z;
        
        t_values[p] = -(
            dot_of_zvertices(&normalized_plane_normal, ray_origin) +
                plane_offsets[p]);
        t_values[p] /= denominator;
        
        #ifndef LOGGER_IGNORE_ASSERTS
        // This should give about the same result:
        float t_values_alternative = dot_of_zvertices(
            &normalized_plane_normal,
            &plane_offset_minus_ray_origin) /
                denominator;
        
        float diff = t_values[p] - t_values_alternative;
        log_assert(diff <  0.1f);
        log_assert(diff > -0.1f);
        #endif
        
        // end of debug check
        
        // if t is < 0, the triangle's plane must be behind us which counts as
        // a miss
        if (t_values[p] < 0.0f) {
            continue;
        }
        
        // We now have computed t, which we can use to calculate the position of P:
        // Vec3f Phit = orig + t * dir;
        zVertex raw_collision_point;
        raw_collision_point.x = ray_origin->x +
            (t_values[p] * ray_direction->x);
        raw_collision_point.y = ray_origin->y +
            (t_values[p] * ray_direction->y); 
        raw_collision_point.z = ray_origin->z +
            (t_values[p] * ray_direction->z);
        
        hit_points[p] = raw_collision_point;
        
        zVertex center_to_hit_ray;
        center_to_hit_ray.x = raw_collision_point.x - gpu_data->xyz[0];
        center_to_hit_ray.y = raw_collision_point.y - gpu_data->xyz[1];
        center_to_hit_ray.z = raw_collision_point.z - gpu_data->xyz[2];
        
        zVertex reverse_rotated_center_to_hit = center_to_hit_ray;        
        reverse_rotated_center_to_hit =
            x_rotate_zvertex(
                &reverse_rotated_center_to_hit,
                -gpu_data->xyz_angle[0]);
        reverse_rotated_center_to_hit =
            y_rotate_zvertex(
                &reverse_rotated_center_to_hit,
                -gpu_data->xyz_angle[1]);
        reverse_rotated_center_to_hit =
            z_rotate_zvertex(
                &reverse_rotated_center_to_hit,
                -gpu_data->xyz_angle[2]);
        
        // Now I just want to check if this hit is within the zpolygon's
        // hitbox
        // it's now an 'axis aligned' hitbox (since we rotated the collision
        // point and not the hitbox itself), so we can just check the bounds
        float tolerance = 0.002f;
        if (
            (reverse_rotated_center_to_hit.x - tolerance) <= 
                 ( cpu_data->hitbox_width  / 2 ) &&
            (reverse_rotated_center_to_hit.x + tolerance) >=
                -( cpu_data->hitbox_width  / 2 ) &&
            (reverse_rotated_center_to_hit.y - tolerance) <=
                 ( cpu_data->hitbox_height / 2 ) &&
            (reverse_rotated_center_to_hit.y + tolerance) >=
                -( cpu_data->hitbox_height / 2 ) &&
            (reverse_rotated_center_to_hit.z - tolerance) <=
                 ( cpu_data->hitbox_depth  / 2 ) &&
            (reverse_rotated_center_to_hit.z + tolerance) >=
                -( cpu_data->hitbox_depth  / 2 ))
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
            *recipient_hit_point = hit_points[p];
            closest_t = t_values[p];
        }
    }
    
    return return_value;
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
