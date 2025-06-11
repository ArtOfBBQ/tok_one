#include "T1_zsprite.h"

zSpriteCollection * zsprites_to_render = NULL;

void zsprite_request_next(zSpriteRequest * stack_recipient)
{
    for (
        uint32_t zp_i = 0;
        zp_i < zsprites_to_render->size;
        zp_i++)
    {
        if (zsprites_to_render->cpu_data[zp_i].deleted)
        {
            stack_recipient->cpu_data     =
                &zsprites_to_render->cpu_data[zp_i];
            stack_recipient->gpu_data     =
                &zsprites_to_render->gpu_data[zp_i];
            stack_recipient->cpu_data->committed = false;
            return;
        }
    }
    
    log_assert(zsprites_to_render->size + 1 < MAX_ZSPRITES_PER_BUFFER);
    stack_recipient->cpu_data     =
        &zsprites_to_render->cpu_data[zsprites_to_render->size];
    stack_recipient->gpu_data     =
        &zsprites_to_render->gpu_data[zsprites_to_render->size];
    stack_recipient->cpu_data[zsprites_to_render->size].deleted = false;
    stack_recipient->cpu_data->committed = false;
    
    zsprites_to_render->size += 1;
    
    return;
}

void zsprite_commit(
    zSpriteRequest * to_commit)
{
    log_assert(to_commit->cpu_data->mesh_id >= 0);
    log_assert(to_commit->cpu_data->mesh_id < (int32_t)all_mesh_summaries_size);
    log_assert(to_commit->cpu_data->mesh_id < ALL_MESHES_SIZE);
    log_assert(
        all_mesh_summaries[to_commit->cpu_data->mesh_id].vertices_size > 0);
    
    #ifndef LOGGER_IGNORE_ASSERTS
    uint32_t all_mesh_vertices_tail_i =
        (uint32_t)(
            all_mesh_summaries[to_commit->cpu_data->mesh_id].vertices_head_i +
            all_mesh_summaries[to_commit->cpu_data->mesh_id].vertices_size -
            1);
    log_assert(all_mesh_vertices_tail_i < all_mesh_vertices->size);
    #endif
    
    to_commit->cpu_data->committed = true;
}

bool32_t zsprite_fetch_by_zsprite_id(
    zSpriteRequest * recipient,
    const int32_t object_id)
{
    common_memset_char(recipient, 0, sizeof(zSpriteRequest));
    
    for (
        uint32_t zp_i = 0;
        zp_i < zsprites_to_render->size;
        zp_i++)
    {
        if (
            !zsprites_to_render->cpu_data[zp_i].deleted &&
            zsprites_to_render->cpu_data[zp_i].zsprite_id == object_id)
        {
            recipient->cpu_data = &zsprites_to_render->cpu_data[zp_i];
            recipient->gpu_data = &zsprites_to_render->gpu_data[zp_i];
            return true;
        }
    }
    
    log_assert(recipient->cpu_data == NULL);
    log_assert(recipient->gpu_data == NULL);
    return false;
}

void zsprite_delete(const int32_t with_object_id)
{
    for (uint32_t i = 0; i < zsprites_to_render->size; i++) {
        if (zsprites_to_render->cpu_data[i].zsprite_id == with_object_id)
        {
            zsprites_to_render->cpu_data[i].deleted   = true;
            zsprites_to_render->cpu_data[i].zsprite_id = -1;
        }
    }
}

float zsprite_get_x_multiplier_for_width(
    CPUzSprite * for_poly,
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
        engineglobals_screenspace_width_to_width(for_width, 1.0f) /
            all_mesh_summaries[for_poly->mesh_id].base_width;
    
    return return_value;
}

float zsprite_get_y_multiplier_for_height(
    CPUzSprite * for_poly,
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

void zsprite_scale_multipliers_to_width(
    CPUzSprite * cpu_data,
    GPUzSprite * gpu_data,
    const float new_height)
{
    float new_multiplier = zsprite_get_x_multiplier_for_width(
        /* zPolygonCPU * for_poly: */
            cpu_data,
        /* const float for_height: */
            new_height);
    
    gpu_data->xyz_multiplier[0] = new_multiplier;
    gpu_data->xyz_multiplier[1] = new_multiplier;
    gpu_data->xyz_multiplier[2] = new_multiplier;
}

void zsprite_scale_multipliers_to_height(
    CPUzSprite * cpu_data,
    GPUzSprite * gpu_data,
    const float new_height)
{
    float new_multiplier = zsprite_get_y_multiplier_for_height(
        /* zPolygonCPU * for_poly: */
            cpu_data,
        /* const float for_height: */
            new_height);
    
    gpu_data->xyz_multiplier[0] = new_multiplier;
    gpu_data->xyz_multiplier[1] = new_multiplier;
    gpu_data->xyz_multiplier[2] = new_multiplier;
}

void zsprite_construct(
    zSpriteRequest * to_construct)
{
    assert(to_construct->cpu_data != NULL);
    assert(to_construct->gpu_data != NULL);
    
    common_memset_char(
        to_construct->cpu_data,
        0,
        sizeof(CPUzSprite));
    common_memset_char(
        to_construct->gpu_data,
        0,
        sizeof(GPUzSprite));
    
    to_construct->gpu_data->xyz_multiplier[0] = 1.0f;
    to_construct->gpu_data->xyz_multiplier[1] = 1.0f;
    to_construct->gpu_data->xyz_multiplier[2] = 1.0f;
    to_construct->gpu_data->scale_factor = 1.0f;
    to_construct->gpu_data->touchable_id = -1;
    
    to_construct->cpu_data->mesh_id = -1;
    to_construct->cpu_data->zsprite_id = -1;
    to_construct->cpu_data->visible = true;
    
    T1_material_construct(&to_construct->gpu_data->base_material);
}

float zsprite_get_distance_f3(
    const float p1[3],
    const float p2[3])
{
    return sqrtf(
        ((p1[0] - p2[0]) * (p1[0] - p2[0])) +
        ((p1[1] - p2[1]) * (p1[1] - p2[1])) +
        ((p1[2] - p2[2]) * (p1[2] - p2[2])));
}

float zsprite_dot_of_vertices_f3(
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

void zsprite_construct_quad(
    const float left_x,
    const float bottom_y,
    const float z,
    const float width,
    const float height,
    zSpriteRequest * stack_recipient)
{
    log_assert(z > 0.0f);
    
    zsprite_construct(stack_recipient);
    
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
}

void zsprite_construct_quad_around(
    const float mid_x,
    const float mid_y,
    const float z,
    const float width,
    const float height,
    zSpriteRequest * stack_recipient)
{
    // log_assert(z > 0.0f);
    
    zsprite_construct(stack_recipient);
    
    stack_recipient->gpu_data->xyz[0]  = mid_x;
    stack_recipient->gpu_data->xyz[1]  = mid_y;
    stack_recipient->gpu_data->xyz[2]  = z;
    stack_recipient->cpu_data->visible = true;
    
    // the hardcoded quad offsets range from -1.0f to 1.0f,
    // so the current width is 2.0f
    float current_width = 2.0f;
    float current_height = 2.0f;
    stack_recipient->gpu_data->xyz_multiplier[0] =
        width / current_width;
    stack_recipient->gpu_data->xyz_multiplier[1] =
        height / current_height;
    stack_recipient->gpu_data->xyz_multiplier[2] =
        stack_recipient->gpu_data->xyz_multiplier[1] / 20.0f;
    
    stack_recipient->cpu_data->mesh_id = BASIC_QUAD_MESH_ID;
}

void zsprite_construct_cube_around(
    const float mid_x,
    const float mid_y,
    const float z,
    const float width,
    const float height,
    const float depth,
    zSpriteRequest * stack_recipient)
{
    log_assert(z > 0.0f);
    
    zsprite_construct(stack_recipient);
    
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
}
