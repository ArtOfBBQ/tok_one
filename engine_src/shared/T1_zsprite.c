#include "T1_zsprite.h"

T1zSpriteCollection * T1_zsprite_list = NULL;

void T1_zsprite_fetch_next(
    T1zSpriteRequest * stack_recipient)
{
    stack_recipient->cpu_data = NULL;
    stack_recipient->gpu_data = NULL;
    
    for (
        uint32_t zp_i = 0;
        zp_i < T1_zsprite_list->size;
        zp_i++)
    {
        if (T1_zsprite_list->cpu_data[zp_i].deleted)
        {
            stack_recipient->cpu_data     =
                &T1_zsprite_list->cpu_data[zp_i];
            stack_recipient->gpu_data     =
                &T1_zsprite_list->gpu_data[zp_i];
            stack_recipient->cpu_data->committed = false;
            break;
        }
    }
    
    if (
        stack_recipient->cpu_data == NULL &&
        stack_recipient->gpu_data == NULL)
    {
        stack_recipient->cpu_data =
            &T1_zsprite_list->
                cpu_data[T1_zsprite_list->size];
        stack_recipient->gpu_data =
            &T1_zsprite_list->
                gpu_data[T1_zsprite_list->size];
        stack_recipient->cpu_data[T1_zsprite_list->size].
            deleted = false;
        stack_recipient->cpu_data->committed = false;
        
        T1_zsprite_list->size += 1;
        log_assert(T1_zsprite_list->size + 1 < MAX_ZSPRITES_PER_BUFFER);
    }
    
    return;
}

void T1_zsprite_commit(
    T1zSpriteRequest * to_commit)
{
    log_assert(to_commit->cpu_data->mesh_id >= 0);
    log_assert(to_commit->cpu_data->mesh_id <
        (int32_t)T1_objmodel_mesh_summaries_size);
    log_assert(to_commit->cpu_data->mesh_id < ALL_MESHES_SIZE);
    log_assert(
        T1_objmodel_mesh_summaries[to_commit->cpu_data->mesh_id].
            vertices_size > 0);
    
    #if T1_LOGGER_ASSERTS_ACTIVE == T1_ACTIVE
    uint32_t all_mesh_vertices_tail_i =
        (uint32_t)(
            T1_objmodel_mesh_summaries
                [to_commit->cpu_data->mesh_id].vertices_head_i +
            T1_objmodel_mesh_summaries
                [to_commit->cpu_data->mesh_id].vertices_size -
            1);
    log_assert(all_mesh_vertices_tail_i < T1_objmodel_all_vertices->size);
    #elif T1_LOGGER_ASSERTS_ACTIVE == T1_INACTIVE
    #else
    #error
    #endif
    
    to_commit->cpu_data->committed = true;
}

void T1_zsprite_delete(const int32_t with_object_id)
{
    for (
        uint32_t i = 0;
        i < T1_zsprite_list->size;
        i++)
    {
        if (
            T1_zsprite_list->cpu_data[i].
                zsprite_id == with_object_id)
        {
            T1_zsprite_list->cpu_data[i].
                deleted = true;
            T1_zsprite_list->cpu_data[i].zsprite_id = -1;
        }
    }
}

void T1_zsprite_delete_all(void) {
    T1_zsprite_list->size = 0;
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
    log_assert(for_poly->mesh_id < (int32_t)T1_objmodel_mesh_summaries_size);
    
    log_assert(for_poly->mesh_id >= 0);
    log_assert(for_poly->mesh_id < (int32_t)T1_objmodel_mesh_summaries_size);
    
    float return_value =
        for_width / T1_objmodel_mesh_summaries[for_poly->mesh_id].base_width;
    
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
    log_assert(for_poly->mesh_id < (int32_t)T1_objmodel_mesh_summaries_size);
    
    float return_value =
        for_depth / T1_objmodel_mesh_summaries[for_poly->mesh_id].base_depth;
    
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
    log_assert(for_poly->mesh_id < (int32_t)T1_objmodel_mesh_summaries_size);
    
    float return_value =
        for_height / T1_objmodel_mesh_summaries[for_poly->mesh_id].base_height;
    
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
        T1_objmodel_mesh_summaries[mesh_id].locked_material_base_offset != UINT32_MAX)
    {
        uint32_t base_mat_i =
            T1_objmodel_mesh_summaries[mesh_id].locked_material_head_i +
            T1_objmodel_mesh_summaries[mesh_id].locked_material_base_offset;
        
        to_construct->gpu_data->base_mat =
            all_mesh_materials->gpu_data[base_mat_i];
        
        T1_material_construct(&to_construct->gpu_data->base_mat);
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
    to_construct->gpu_data->touch_id = -1;
    to_construct->gpu_data->alpha = 1.0f;
    
    to_construct->cpu_data->simd_stats.scale_factor = 1.0f;
    to_construct->cpu_data->mesh_id = -1;
    to_construct->cpu_data->zsprite_id = -1;
    to_construct->cpu_data->visible = true;
    
    #if T1_OUTLINES_ACTIVE == T1_ACTIVE
    to_construct->gpu_data->outline_alpha = -1.0f;
    #elif T1_OUTLINES_ACTIVE == T1_INACTIVE
    #else
    #error
    #endif
    
    to_construct->gpu_data->mix_project_rv_i = -1;
    to_construct->gpu_data->mix_project_array_i = -1;
    to_construct->gpu_data->mix_project_slice_i = -1;
    
    T1_material_construct(
        &to_construct->gpu_data->base_mat);
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
    if (
        stack_recipient->cpu_data->simd_stats.
            mul_xyz[0] < THRESH)
    {
        stack_recipient->cpu_data->simd_stats.mul_xyz[0] = THRESH;
    }
    if (
        stack_recipient->cpu_data->simd_stats.
            mul_xyz[1] < THRESH)
    {
        stack_recipient->cpu_data->simd_stats.mul_xyz[1] = THRESH;
    }
    if (stack_recipient->cpu_data->simd_stats.
        mul_xyz[2] < THRESH)
    {
        stack_recipient->cpu_data->simd_stats.mul_xyz[2] = THRESH;
    }
    
    stack_recipient->cpu_data->mesh_id =
        BASIC_QUAD_MESH_ID;
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

void T1_zsprite_anim_apply_effects_at_t(
    const float t_applied,
    const float t_now,
    const float * anim_gpu_vals,
    const float * anim_cpu_vals,
    T1GPUzSprite * recip_gpu,
    T1CPUzSpriteSimdStats * recip_cpu)
{
    float * target_vals_ptr = (float *)recip_gpu;
    
    SIMD_FLOAT simd_t_now = simd_set1_float(t_now);
    SIMD_FLOAT simd_t_b4  = simd_set1_float(t_applied);
    
    log_assert((sizeof(T1GPUzSprite) / 4) % SIMD_FLOAT_LANES == 0);
    log_assert((sizeof(T1CPUzSpriteSimdStats) / 4) % SIMD_FLOAT_LANES == 0);
    
    for (
        uint32_t simd_step_i = 0;
        (simd_step_i * sizeof(float)) < sizeof(T1GPUzSprite);
        simd_step_i += SIMD_FLOAT_LANES)
    {
        SIMD_FLOAT simd_anim_vals =
            simd_load_floats((anim_gpu_vals + simd_step_i));
        SIMD_FLOAT simd_target_vals =
            simd_load_floats((target_vals_ptr + simd_step_i));
        
        SIMD_FLOAT simd_t_now_deltas =
            simd_mul_floats(
                simd_anim_vals,
                simd_t_now);
        SIMD_FLOAT simd_t_previous_deltas =
            simd_mul_floats(
                simd_anim_vals,
                simd_t_b4);
        
        simd_t_now_deltas = simd_sub_floats(
            simd_t_now_deltas,
            simd_t_previous_deltas);
        
        simd_store_floats(
            (target_vals_ptr + simd_step_i),
            simd_add_floats(
                simd_target_vals,
                simd_t_now_deltas));
    }
    
    target_vals_ptr = (float *)recip_cpu;
    
    for (
        uint32_t simd_step_i = 0;
        (simd_step_i * sizeof(float)) < sizeof(T1CPUzSpriteSimdStats);
        simd_step_i += SIMD_FLOAT_LANES)
    {
        SIMD_FLOAT simd_anim_vals =
            simd_load_floats((anim_cpu_vals + simd_step_i));
        SIMD_FLOAT simd_target_vals =
            simd_load_floats((target_vals_ptr + simd_step_i));
        
        SIMD_FLOAT simd_t_now_deltas =
            simd_mul_floats(
                simd_anim_vals,
                simd_t_now);
        SIMD_FLOAT simd_t_previous_deltas =
            simd_mul_floats(
                simd_anim_vals,
                simd_t_b4);
        
        simd_t_now_deltas = simd_sub_floats(
            simd_t_now_deltas,
            simd_t_previous_deltas);
        
        simd_store_floats(
            (target_vals_ptr + simd_step_i),
            simd_add_floats(
                simd_target_vals,
                simd_t_now_deltas));
    }
    
    #if T1_LOGGER_ASSERTS_ACTIVE == T1_ACTIVE
    log_assert(recip_gpu->ignore_camera >= -0.05f);
    log_assert(recip_gpu->ignore_camera <= 1.05f);
    log_assert(recip_gpu->ignore_lighting >= -0.05f);
    log_assert(recip_gpu->ignore_lighting <= 1.05f);
    log_assert(recip_gpu->remove_shadow >= 0);
    log_assert(recip_gpu->remove_shadow <= 1);
    // log_assert(recip->alpha >= -0.1f);
    // log_assert(recip->alpha <=  1.1f);
    log_assert(recip_cpu->scale_factor >    0.0f);
    log_assert(recip_cpu->scale_factor < 1000.0f);
    // log_assert(recip->xyz_multiplier[0] > 0.0f);
    // log_assert(recip->xyz_multiplier[1] > 0.0f);
    // log_assert(recip->xyz_multiplier[2] > 0.0f);
    log_assert(recip_gpu->bonus_rgb[0] < 3.0f);
    log_assert(recip_gpu->bonus_rgb[1] < 3.0f);
    log_assert(recip_gpu->bonus_rgb[2] < 3.0f);
    #elif T1_LOGGER_ASSERTS_ACTIVE == T1_INACTIVE
    #else
    #error
    #endif
}

void T1_zsprite_apply_anim_effects_to_id(
    const int32_t zsprite_id,
    const int32_t touch_id,
    const float t_applied,
    const float t_now,
    const float * anim_gpu_vals,
    const float * anim_cpu_vals)
{
    for (
        int32_t zp_i = 0;
        zp_i < (int32_t)T1_zsprite_list->size;
        zp_i++)
    {
        if (
            (zsprite_id >= 0 &&
            T1_zsprite_list->cpu_data[zp_i].
                zsprite_id != zsprite_id) ||
            (touch_id >= 0 &&
            T1_zsprite_list->gpu_data[zp_i].
                touch_id != touch_id) ||
            T1_zsprite_list->cpu_data[zp_i].deleted)
        {
            continue;
        }
        
        T1_zsprite_anim_apply_effects_at_t(
            /* const float t_applied: */
                t_applied,
            /* const float t_now: */
                t_now,
            /* const float * anim_gpu_vals: */
                anim_gpu_vals,
            /* const float * anim_cpu_vals: */
                anim_cpu_vals,
            /* T1GPUzSprite * recip_gpu: */
                &T1_zsprite_list->gpu_data[zp_i],
            /* T1CPUzSpriteSimdStats * recip_cpu: */
                &T1_zsprite_list->cpu_data[zp_i].
                    simd_stats);
    }
}

void T1_zsprite_apply_endpoint_anim(
    const int32_t zsprite_id,
    const int32_t touch_id,
    const float t_applied,
    const float t_now,
    const float * goal_gpu_vals,
    const float * goal_cpu_vals)
{
    // When t is 1.0f, all of our stats will
    // be exactly equal to target_delta
    const float was_left_t = 1.0f - t_applied;
    const float did_now_t = t_now - t_applied;
    const float t_mult = did_now_t / was_left_t;
    SIMD_FLOAT simd_t = simd_set1_float(t_mult);
    
    float no_effect = T1_ZSPRITEANIM_NO_EFFECT;
    SIMD_FLOAT simd_noeffect =
        simd_set1_float(no_effect);
    
    for (
        int32_t zp_i = 0;
        zp_i < (int32_t)T1_zsprite_list->size;
        zp_i++)
    {
        if (
            (zsprite_id >= 0 &&
            T1_zsprite_list->cpu_data[zp_i].
                zsprite_id != zsprite_id) ||
            (touch_id >= 0 &&
            T1_zsprite_list->gpu_data[zp_i].
                touch_id != touch_id) ||
            T1_zsprite_list->cpu_data[zp_i].deleted)
        {
            continue;
        }
        
        float * recip_vals_cpu = (float *)
            &T1_zsprite_list->cpu_data[zp_i].
                simd_stats;
        
        for (
            uint32_t simd_step_i = 0;
            (simd_step_i * sizeof(float)) < sizeof(T1CPUzSpriteSimdStats);
            simd_step_i += SIMD_FLOAT_LANES)
        {
            SIMD_FLOAT simd_goal_vals =
                simd_load_floats((goal_cpu_vals +
                    simd_step_i));
            
            SIMD_FLOAT simd_cur_vals =
                simd_load_floats((recip_vals_cpu +
                    simd_step_i));
            
            SIMD_FLOAT delta_to_goal =
                simd_sub_floats(simd_goal_vals,
                    simd_cur_vals);
            
            delta_to_goal = simd_mul_floats(delta_to_goal, simd_t);
            
            SIMD_FLOAT flags = simd_not_floats(
                simd_cmpeq_floats(
                    simd_goal_vals,
                    simd_noeffect));
            
            delta_to_goal = simd_and_floats(
                delta_to_goal, flags);
            
            simd_cur_vals = simd_add_floats(
                simd_cur_vals,
                delta_to_goal);
            
            simd_store_floats(
                recip_vals_cpu + simd_step_i,
                simd_cur_vals);
        }
        
        float * recip_vals_gpu = (float *)
            &T1_zsprite_list->gpu_data[zp_i];
        
        for (
            uint32_t simd_step_i = 0;
            (simd_step_i * sizeof(float)) < sizeof(T1GPUzSprite);
            simd_step_i += SIMD_FLOAT_LANES)
        {
            SIMD_FLOAT simd_goal_vals =
                simd_load_floats((goal_gpu_vals +
                    simd_step_i));
            
            SIMD_FLOAT simd_cur_vals =
                simd_load_floats(
                    (recip_vals_gpu + simd_step_i));
            
            SIMD_FLOAT delta_to_goal =
                simd_sub_floats(
                    simd_goal_vals, simd_cur_vals);
            
            delta_to_goal = simd_mul_floats(
                delta_to_goal,
                simd_t);
            
            SIMD_FLOAT flags = simd_not_floats(
                simd_cmpeq_floats(
                    simd_goal_vals,
                    simd_noeffect));
            
            delta_to_goal = simd_and_floats(
                delta_to_goal, flags);
            
            simd_cur_vals = simd_add_floats(
                simd_cur_vals,
                delta_to_goal);
            
            simd_store_floats(
                recip_vals_gpu + simd_step_i,
                simd_cur_vals);
        }
    }
}

void T1_zsprite_handle_timed_occlusion(void)
{
    for (
        uint32_t zs_i = 0;
        zs_i < T1_zsprite_list->size;
        zs_i++)
    {
        if (
            T1_zsprite_list->cpu_data[zs_i].
                next_occlusion_in_us >
                    T1_global->elapsed)
        {
            T1_zsprite_list->cpu_data[zs_i].
                next_occlusion_in_us -=
                    T1_global->elapsed;
        } else if (
            T1_zsprite_list->cpu_data[zs_i].
                next_occlusion_in_us > 0)
        {
            T1_zsprite_list->cpu_data[zs_i].
                next_occlusion_in_us = 0;
            T1_zsprite_list->cpu_data[zs_i].
                visible = 0;
        }
    }
}

void T1_zsprite_anim_set_ignore_camera_but_retain_screenspace_pos(
    const int32_t zsprite_id,
    const float new_ignore_camera)
{
    T1GPUzSprite * zs = NULL;
    T1CPUzSprite * zs_cpu = NULL;
    
    for (uint32_t i = 0; i < T1_zsprite_list->size; i++)
    {
        if (T1_zsprite_list->cpu_data[i].zsprite_id == zsprite_id) {
            zs = T1_zsprite_list->gpu_data + i;
            zs_cpu = T1_zsprite_list->cpu_data + i;
        }
    }
    
    if (zs->ignore_camera == new_ignore_camera)
    {
        return;
    }
    
    // For now we're only supporting the easy case of a full toggle
    bool32_t is_near_zero =
        zs->ignore_camera > -0.01f &&
        zs->ignore_camera <  0.01f;
    #if T1_LOGGER_ASSERTS_ACTIVE
    bool32_t is_near_one =
        zs->ignore_camera >  0.99f &&
        zs->ignore_camera <  1.01f;
    #endif
    log_assert(is_near_zero || is_near_one);
    
    if (is_near_zero) {
        log_assert(new_ignore_camera == 1.0f);
        
        zs_cpu->simd_stats.xyz[0] -= T1_camera->xyz[0];
        zs_cpu->simd_stats.xyz[1] -= T1_camera->xyz[1];
        zs_cpu->simd_stats.xyz[2] -= T1_camera->xyz[2];
        x_rotate_f3(zs_cpu->simd_stats.xyz, -T1_camera->xyz_angle[0]);
        y_rotate_f3(zs_cpu->simd_stats.xyz, -T1_camera->xyz_angle[1]);
        z_rotate_f3(zs_cpu->simd_stats.xyz, -T1_camera->xyz_angle[2]);
        
        #if 1
        // This is a hack, an approximation
        zs_cpu->simd_stats.angle_xyz[0] -= T1_camera->xyz_angle[0];
        zs_cpu->simd_stats.angle_xyz[1] -= T1_camera->xyz_angle[1];
        zs_cpu->simd_stats.angle_xyz[2] -= T1_camera->xyz_angle[2];
        #endif
        
        zs->ignore_camera = 1.0f;
    } else {
        log_assert(is_near_one);
        
        z_rotate_f3(zs_cpu->simd_stats.xyz, T1_camera->xyz_angle[2]);
        y_rotate_f3(zs_cpu->simd_stats.xyz, T1_camera->xyz_angle[1]);
        x_rotate_f3(zs_cpu->simd_stats.xyz, T1_camera->xyz_angle[0]);
        
        zs_cpu->simd_stats.xyz[0] += T1_camera->xyz[0];
        zs_cpu->simd_stats.xyz[1] += T1_camera->xyz[1];
        zs_cpu->simd_stats.xyz[2] += T1_camera->xyz[2];
        
        #if 1
        // This is a hack, an approximation
        zs_cpu->simd_stats.angle_xyz[0] += T1_camera->xyz_angle[0];
        zs_cpu->simd_stats.angle_xyz[1] += T1_camera->xyz_angle[1];
        zs_cpu->simd_stats.angle_xyz[2] += T1_camera->xyz_angle[2];
        #endif
        
        zs->ignore_camera = 0.0f;
    }
}

void add_opaque_zpolygons_to_workload(
    T1GPUFrame * frame_data)
{
    // for now we assume this always comes 1st
    log_assert(frame_data->verts_size == 0);
    
    int32_t first_opaq_i = (int32_t)frame_data->
        verts_size;
    
    int32_t cur_vals[4];
    int32_t incr_vals[4];
    incr_vals[0] = 2;
    incr_vals[1] = 0;
    incr_vals[2] = 2;
    incr_vals[3] = 0;
    SIMD_VEC4I incr = simd_load_vec4i(incr_vals);
    
    for (
        int32_t cpu_zp_i = 0;
        cpu_zp_i < (int32_t)T1_zsprite_list->size;
        cpu_zp_i++)
    {
        if (
            T1_zsprite_list->cpu_data[cpu_zp_i].
                deleted ||
            !T1_zsprite_list->cpu_data[cpu_zp_i].
                visible ||
            !T1_zsprite_list->cpu_data[cpu_zp_i].
                committed ||
            T1_zsprite_list->cpu_data[cpu_zp_i].
                alpha_blending_on ||
            T1_zsprite_list->cpu_data[cpu_zp_i].
                bloom_on)
        {
            continue;
        }
        
        int32_t mesh_id = T1_zsprite_list->
            cpu_data[cpu_zp_i].mesh_id;
        log_assert(mesh_id >= 0);
        log_assert(mesh_id < (int32_t)T1_objmodel_mesh_summaries_size);
        
        int32_t vert_tail_i =
            T1_objmodel_mesh_summaries[mesh_id].
                vertices_head_i +
                    T1_objmodel_mesh_summaries[mesh_id].
                        vertices_size;
        log_assert(
            vert_tail_i < MAX_VERTICES_PER_BUFFER);
        
        /*
        We are free to overflow the vertices buffer, since its end is not
        in use yet anyway.
        */
        int32_t vert_i =
            T1_objmodel_mesh_summaries[mesh_id].
                vertices_head_i;
        cur_vals[0] = vert_i-2;
        cur_vals[1] = cpu_zp_i;
        cur_vals[2] = vert_i-1;
        cur_vals[3] = cpu_zp_i;
        SIMD_VEC4I cur  = simd_load_vec4i(cur_vals);
        
        int32_t verts_to_copy = vert_tail_i - vert_i;
        
        #if T1_LOGGER_ASSERTS_ACTIVE == T1_ACTIVE
        uint32_t previous_verts_size = frame_data->verts_size;
        #elif T1_LOGGER_ASSERTS_ACTIVE == T1_INACTIVE
        // Pass
        #else
        #error "T1_LOGGER_ASSERTS_ACTIVE undefined"
        #endif
        
        for (int32_t i = 0; i < verts_to_copy; i += 2) {
            cur = simd_add_vec4i(cur, incr);
            simd_store_vec4i(
                (frame_data->verts + frame_data->verts_size),
                cur);
            frame_data->verts_size += 2;
            
            #if T1_LOGGER_ASSERTS_ACTIVE == T1_ACTIVE
            log_assert(frame_data->verts_size < MAX_VERTICES_PER_BUFFER);
            log_assert(frame_data->verts[frame_data->verts_size-2].
                locked_vertex_i == (vert_i + i));
            log_assert(frame_data->verts[frame_data->verts_size-1].
                locked_vertex_i == (vert_i + i + 1));
            #elif T1_LOGGER_ASSERTS_ACTIVE == T1_INACTIVE
            // Pass
            #else
            #error "T1_LOGGER_ASSERTS_ACTIVE undefined"
            #endif
        }
        
        if (verts_to_copy % 2 == 1) {
            frame_data->verts_size -= 1;
        }
        
        #if T1_LOGGER_ASSERTS_ACTIVE == T1_ACTIVE
        log_assert(frame_data->verts_size ==
            (previous_verts_size + (uint32_t)verts_to_copy));
        #elif T1_LOGGER_ASSERTS_ACTIVE == T1_INACTIVE
        // Pass
        #else
        #error "T1_LOGGER_ASSERTS_ACTIVE undefined"
        #endif
    }
    
    for (
        uint32_t cam_i = 0;
        cam_i < T1_render_views->size;
        cam_i++)
    {
        for (
            int32_t pass_i = 0;
            pass_i < T1_render_views->cpu[cam_i].
                passes_size;
            pass_i++)
        {
            if (
                T1_render_views->cpu[cam_i].passes[pass_i].type ==
                        T1RENDERPASS_DIAMOND_ALPHA)
            {
                T1_render_views->cpu[cam_i].
                    passes[pass_i].vert_i =
                        first_opaq_i;
                T1_render_views->cpu[cam_i].
                    passes[pass_i].verts_size =
                        (int32_t)frame_data->
                            verts_size;
            }
        }
    }
}
