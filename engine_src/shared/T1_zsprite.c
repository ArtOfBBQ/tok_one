#include "T1_zsprite.h"

T1zSpriteCollection * T1_zsprites_to_render = NULL;

void T1_zsprite_request_next(
    T1zSpriteRequest * stack_recipient)
{
    stack_recipient->cpu_data = NULL;
    stack_recipient->gpu_data = NULL;
    
    for (
        uint32_t zp_i = 0;
        zp_i < T1_zsprites_to_render->size;
        zp_i++)
    {
        if (T1_zsprites_to_render->cpu_data[zp_i].deleted)
        {
            stack_recipient->cpu_data     =
                &T1_zsprites_to_render->cpu_data[zp_i];
            stack_recipient->gpu_data     =
                &T1_zsprites_to_render->gpu_data[zp_i];
            stack_recipient->cpu_data->committed = false;
            break;
        }
    }
    
    if (
        stack_recipient->cpu_data == NULL &&
        stack_recipient->gpu_data == NULL)
    {
        stack_recipient->cpu_data =
            &T1_zsprites_to_render->
                cpu_data[T1_zsprites_to_render->size];
        stack_recipient->gpu_data =
            &T1_zsprites_to_render->
                gpu_data[T1_zsprites_to_render->size];
        stack_recipient->cpu_data[T1_zsprites_to_render->size].
            deleted = false;
        stack_recipient->cpu_data->committed = false;
        
        T1_zsprites_to_render->size += 1;
        log_assert(T1_zsprites_to_render->size + 1 < MAX_ZSPRITES_PER_BUFFER);
    }
    
    return;
}

void T1_zsprite_commit(
    T1zSpriteRequest * to_commit)
{
    log_assert(to_commit->cpu_data->mesh_id >= 0);
    log_assert(to_commit->cpu_data->mesh_id <
        (int32_t)all_mesh_summaries_size);
    log_assert(to_commit->cpu_data->mesh_id < ALL_MESHES_SIZE);
    log_assert(
        all_mesh_summaries[to_commit->cpu_data->mesh_id].
            vertices_size > 0);
    
    #if T1_LOGGER_ASSERTS_ACTIVE == T1_ACTIVE
    uint32_t all_mesh_vertices_tail_i =
        (uint32_t)(
            all_mesh_summaries
                [to_commit->cpu_data->mesh_id].vertices_head_i +
            all_mesh_summaries
                [to_commit->cpu_data->mesh_id].vertices_size -
            1);
    log_assert(all_mesh_vertices_tail_i < all_mesh_vertices->size);
    #elif T1_LOGGER_ASSERTS_ACTIVE == T1_INACTIVE
    #else
    #error
    #endif
    
    to_commit->cpu_data->committed = true;
}

bool32_t T1_zsprite_fetch_by_zsprite_id(
    T1zSpriteRequest * recipient,
    const int32_t object_id)
{
    T1_std_memset(recipient, 0, sizeof(T1zSpriteRequest));
    
    for (
        uint32_t zp_i = 0;
        zp_i < T1_zsprites_to_render->size;
        zp_i++)
    {
        if (
            !T1_zsprites_to_render->cpu_data[zp_i].deleted &&
            T1_zsprites_to_render->cpu_data[zp_i].zsprite_id == object_id)
        {
            recipient->cpu_data = &T1_zsprites_to_render->cpu_data[zp_i];
            recipient->gpu_data = &T1_zsprites_to_render->gpu_data[zp_i];
            return true;
        }
    }
    
    log_assert(recipient->cpu_data == NULL);
    log_assert(recipient->gpu_data == NULL);
    return false;
}

void T1_zsprite_delete(const int32_t with_object_id)
{
    for (uint32_t i = 0; i < T1_zsprites_to_render->size; i++) {
        if (T1_zsprites_to_render->cpu_data[i].zsprite_id == with_object_id)
        {
            T1_zsprites_to_render->cpu_data[i].deleted   = true;
            T1_zsprites_to_render->cpu_data[i].zsprite_id = -1;
        }
    }
}

float T1_zsprite_get_x_multiplier_for_width(
    T1CPUzSprite * for_poly,
    const float for_width)
{
    log_assert(for_poly != NULL);
    #if T1_LOGGER_ASSERTS_ACTIVE == T1_ACTIVE
    if (for_poly == NULL) {
        return 0.0f;
    }
    #elif T1_LOGGER_ASSERTS_ACTIVE == T1_INACTIVE
    #else
    #error
    #endif
    
    log_assert(for_poly->mesh_id >= 0);
    log_assert(for_poly->mesh_id < (int32_t)all_mesh_summaries_size);
    
    log_assert(for_poly->mesh_id >= 0);
    log_assert(for_poly->mesh_id < (int32_t)all_mesh_summaries_size);
    
    float return_value =
        for_width / all_mesh_summaries[for_poly->mesh_id].base_width;
    
    return return_value;
}

float T1_zsprite_get_z_multiplier_for_depth(
    T1CPUzSprite * for_poly,
    const float for_depth)
{
    log_assert(for_poly != NULL);
    #if T1_LOGGER_ASSERTS_ACTIVE == T1_ACTIVE
    if (for_poly == NULL) {
        return 0.0f;
    }
    #elif T1_LOGGER_ASSERTS_ACTIVE == T1_INACTIVE
    #else
    #error
    #endif
    
    log_assert(for_poly->mesh_id >= 0);
    log_assert(for_poly->mesh_id < (int32_t)all_mesh_summaries_size);
    
    float return_value =
        for_depth / all_mesh_summaries[for_poly->mesh_id].base_depth;
    
    return return_value;
}

float T1_zsprite_get_y_multiplier_for_height(
    T1CPUzSprite * for_poly,
    const float for_height)
{
    log_assert(for_poly != NULL);
    #if T1_LOGGER_ASSERTS_ACTIVE == T1_ACTIVE
    if (for_poly == NULL) {
        return 0.0f;
    }
    #elif T1_LOGGER_ASSERTS_ACTIVE == T1_INACTIVE
    #else
    #error
    #endif
    
    log_assert(for_poly->mesh_id >= 0);
    log_assert(for_poly->mesh_id < (int32_t)all_mesh_summaries_size);
    
    float return_value =
        for_height / all_mesh_summaries[for_poly->mesh_id].base_height;
    
    return return_value;
}

void T1_zsprite_scale_multipliers_to_width(
    T1CPUzSprite * cpu_data,
    T1GPUzSprite * gpu_data,
    const float new_height)
{
    float new_multiplier = T1_zsprite_get_x_multiplier_for_width(
        /* zPolygonCPU * for_poly: */
            cpu_data,
        /* const float for_height: */
            new_height);
    
    cpu_data->simd_stats.mul_xyz[0] = new_multiplier;
    cpu_data->simd_stats.mul_xyz[1] = new_multiplier;
    cpu_data->simd_stats.mul_xyz[2] = new_multiplier;
}

void T1_zsprite_scale_multipliers_to_height(
    T1CPUzSprite * cpu_data,
    T1GPUzSprite * gpu_data,
    const float new_height)
{
    float new_multiplier = T1_zsprite_get_y_multiplier_for_height(
        /* zPolygonCPU * for_poly: */
            cpu_data,
        /* const float for_height: */
            new_height);
    
    cpu_data->simd_stats.mul_xyz[0] = new_multiplier;
    cpu_data->simd_stats.mul_xyz[1] = new_multiplier;
    cpu_data->simd_stats.mul_xyz[2] = new_multiplier;
}

void T1_zsprite_construct_with_mesh_id(
    T1zSpriteRequest * to_construct,
    const int32_t mesh_id)
{
    T1_zsprite_construct(to_construct);
    
    to_construct->cpu_data->mesh_id = mesh_id;
    
    if (mesh_id >= 0 &&
        all_mesh_summaries[mesh_id].locked_material_base_offset != UINT32_MAX)
    {
        uint32_t base_mat_i =
            all_mesh_summaries[mesh_id].locked_material_head_i +
            all_mesh_summaries[mesh_id].locked_material_base_offset;
        
        to_construct->gpu_data->base_mat =
            all_mesh_materials->gpu_data[base_mat_i];
    }
}

void T1_zsprite_construct(
    T1zSpriteRequest * to_construct)
{
    assert(to_construct->cpu_data != NULL);
    assert(to_construct->gpu_data != NULL);
    
    T1_std_memset(
        to_construct->cpu_data,
        0,
        sizeof(T1CPUzSprite));
    T1_std_memset(
        to_construct->gpu_data,
        0,
        sizeof(T1GPUzSprite));
    
    to_construct->cpu_data->simd_stats.mul_xyz[0] = 1.0f;
    to_construct->cpu_data->simd_stats.mul_xyz[1] = 1.0f;
    to_construct->cpu_data->simd_stats.mul_xyz[2] = 1.0f;
    to_construct->gpu_data->touchable_id = -1;
    to_construct->gpu_data->alpha = 1.0f;
    
    to_construct->cpu_data->simd_stats.scale_factor = 1.0f;
    to_construct->cpu_data->mesh_id = -1;
    to_construct->cpu_data->zsprite_id = -1;
    to_construct->cpu_data->visible = true;
    
    T1_material_construct(&to_construct->gpu_data->base_mat);
}

float T1_zsprite_get_distance_f3(
    const float p1[3],
    const float p2[3])
{
    return sqrtf(
        ((p1[0] - p2[0]) * (p1[0] - p2[0])) +
        ((p1[1] - p2[1]) * (p1[1] - p2[1])) +
        ((p1[2] - p2[2]) * (p1[2] - p2[2])));
}

float T1_zsprite_dot_of_vertices_f3(
    const float a[3],
    const float b[3])
{
    float x =
        (
            a[0] * b[0]
        );
    x = (isnan(x) || !isfinite(x)) ? T1_F32_MAX : x;
    
    float y = (a[1] * b[1]);
    y = (isnan(y) || !isfinite(y)) ? T1_F32_MAX : y;
    
    float z = (a[2] * b[2]);
    z = (isnan(z) || !isfinite(z)) ? T1_F32_MAX : z;
    
    float return_value = x + y + z;
    
    log_assert(!isnan(return_value));
    
    return return_value;
}

void zsprite_construct_quad(
    const float left_x,
    const float bottom_y,
    const float z,
    const float width,
    const float height,
    T1zSpriteRequest * stack_recipient)
{
    log_assert(z > 0.0f);
    
    T1_zsprite_construct(stack_recipient);
    
    const float mid_x =
        left_x + (width  / 2);
    const float mid_y =
        bottom_y  + (height / 2);
    
    stack_recipient->cpu_data->simd_stats.xyz[0] = mid_x;
    stack_recipient->cpu_data->simd_stats.xyz[1] = mid_y;
    stack_recipient->cpu_data->simd_stats.xyz[2] = z;
    stack_recipient->cpu_data->visible = true;
    stack_recipient->gpu_data->ignore_camera = false;
    
    // a quad is hardcoded in objmodel.c's init_all_meshes()
    stack_recipient->cpu_data->mesh_id = 0;
    
    // the hardcoded quad offsets range from -1.0f to 1.0f,
    // so the current width is 2.0f
    float current_width = 2.0f;
    float current_height = 2.0f;
    stack_recipient->cpu_data->simd_stats.mul_xyz[0] = width / current_width;
    stack_recipient->cpu_data->simd_stats.mul_xyz[1] = height / current_height;
    stack_recipient->cpu_data->simd_stats.mul_xyz[2] = 1.0f;
}

void T1_zsprite_construct_quad_around(
    const float mid_x,
    const float mid_y,
    const float z,
    const float width,
    const float height,
    T1zSpriteRequest * stack_recipient)
{
    // log_assert(z > 0.0f);
    
    T1_zsprite_construct(stack_recipient);
    
    stack_recipient->cpu_data->simd_stats.xyz[0]  = mid_x;
    stack_recipient->cpu_data->simd_stats.xyz[1]  = mid_y;
    stack_recipient->cpu_data->simd_stats.xyz[2]  = z;
    stack_recipient->cpu_data->visible = true;
    
    // the hardcoded quad offsets range from -1.0f to 1.0f,
    // so the current width is 2.0f
    float current_width = 2.0f;
    float current_height = 2.0f;
    stack_recipient->cpu_data->simd_stats.mul_xyz[0] =
        width / current_width;
    stack_recipient->cpu_data->simd_stats.mul_xyz[1] =
        height / current_height;
    stack_recipient->cpu_data->simd_stats.mul_xyz[2] =
        stack_recipient->cpu_data->simd_stats.mul_xyz[1] / 20.0f;
    
    #define THRESH 0.00001f
    if (stack_recipient->cpu_data->simd_stats.mul_xyz[0] < THRESH) {
        stack_recipient->cpu_data->simd_stats.mul_xyz[0] = THRESH;
    }
    if (stack_recipient->cpu_data->simd_stats.mul_xyz[1] < THRESH) {
        stack_recipient->cpu_data->simd_stats.mul_xyz[1] = THRESH;
    }
    if (stack_recipient->cpu_data->simd_stats.mul_xyz[2] < THRESH) {
        stack_recipient->cpu_data->simd_stats.mul_xyz[2] = THRESH;
    }
    
    stack_recipient->cpu_data->mesh_id = BASIC_QUAD_MESH_ID;
}

void zsprite_construct_cube_around(
    const float mid_x,
    const float mid_y,
    const float z,
    const float width,
    const float height,
    const float depth,
    T1zSpriteRequest * stack_recipient)
{
    log_assert(z > 0.0f);
    
    T1_zsprite_construct(stack_recipient);
    
    stack_recipient->cpu_data->simd_stats.xyz[0]  = mid_x;
    stack_recipient->cpu_data->simd_stats.xyz[1]  = mid_y;
    stack_recipient->cpu_data->simd_stats.xyz[2]  = z;
    stack_recipient->cpu_data->visible = true;
    
    // the hardcoded quad offsets range from -1.0f to 1.0f,
    // so the current width is 2.0f
    float current_width = 2.0f;
    float current_height = 2.0f;
    float current_depth = 2.0f;
    stack_recipient->cpu_data->simd_stats.mul_xyz[0] = width / current_width;
    stack_recipient->cpu_data->simd_stats.mul_xyz[1] = height / current_height;
    stack_recipient->cpu_data->simd_stats.mul_xyz[2] = depth / current_depth;
    
    stack_recipient->cpu_data->mesh_id = 1;
}
