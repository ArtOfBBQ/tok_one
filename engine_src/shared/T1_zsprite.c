#include "T1_zsprite.h"

#if T1_LOGGER_ASSERTS_ACTIVE == T1_ACTIVE
static void assert_sanity_check_zsprite_vals(
    T1GPUzSprite * recip_gpu,
    T1CPUzSpriteSimdStats * recip_cpu)
{
    if (recip_gpu) {
        log_assert(recip_gpu->ignore_camera >= -0.05f);
        log_assert(recip_gpu->ignore_camera <= 1.05f);
        log_assert(recip_gpu->ignore_lighting >= -0.05f);
        log_assert(recip_gpu->ignore_lighting <= 1.05f);
        log_assert(recip_gpu->remove_shadow >= 0);
        log_assert(recip_gpu->remove_shadow <= 1);
        log_assert(recip_gpu->alpha >= -0.1f);
        log_assert(recip_gpu->alpha <=  1.1f);
        log_assert(recip_gpu->bonus_rgb[0] < 3.0f);
        log_assert(recip_gpu->bonus_rgb[1] < 3.0f);
        log_assert(recip_gpu->bonus_rgb[2] < 3.0f);
    }
    
    if (recip_cpu) {
        log_assert(recip_cpu->scale_factor >    0.0f);
        log_assert(recip_cpu->scale_factor < 1000.0f);
        log_assert(recip_cpu->mul_xyz[0] > 0.0f);
        log_assert(recip_cpu->mul_xyz[1] > 0.0f);
        log_assert(recip_cpu->mul_xyz[2] > 0.0f);
    }
}

static void assert_sanity_check_zsprite_vals_by_id(
    const int32_t zp_i)
{
    assert_sanity_check_zsprite_vals(
        &T1_zsprite_list->gpu_data[zp_i],
        &T1_zsprite_list->cpu_data[zp_i].
            simd_stats);
}
#elif T1_LOGGER_ASSERTS_ACTIVE == T1_INACTIVE
#define assert_sanity_check_zsprite_vals(x)
#define assert_sanity_check_zsprite_vals_by_id(x)
#else
#error
#endif

T1zSpriteCollection * T1_zsprite_list = NULL;

void T1_zsprite_init(void) {
    T1_zsprite_list = (T1zSpriteCollection *)
        T1_mem_malloc_from_unmanaged(
            sizeof(T1zSpriteCollection));
    T1_zsprite_list->size = 0;
    
    
    #if T1_LOGGER_ASSERTS_ACTIVE == T1_ACTIVE
    // This should be the first int32 attribute
    int32_t i32_props_start = offsetof(T1GPUzSprite, base_mat_i32);
    // we expect this to be next
    int32_t next_i32 = offsetof(T1GPUzSprite, remove_shadow);
    log_assert(next_i32 == i32_props_start + (int32_t)sizeof(T1GPUConstMati32));
    // ...and the float values before that
    // should be simd-padded, I think for 8 floats
    log_assert(((i32_props_start / 4) % 8) == 0);
    
    int32_t i32_size =
        (int32_t)sizeof(T1GPUzSprite) - i32_props_start;
    
    // again should be simd-padded, again for 8
    log_assert(((i32_size / 4) % 8) == 0);
    #elif T1_LOGGER_ASSERTS_ACTIVE == T1_INACTIVE
    #else
    #error
    #endif
}

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
        
        to_construct->gpu_data->base_mat_f32 =
            all_mesh_materials->gpu_f32[base_mat_i];
        to_construct->gpu_data->base_mat_i32 =
            all_mesh_materials->gpu_i32[base_mat_i];
        
        T1_material_construct(
            &to_construct->gpu_data->base_mat_f32,
            &to_construct->gpu_data->base_mat_i32);
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
        &to_construct->gpu_data->base_mat_f32,
        &to_construct->gpu_data->base_mat_i32);
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
    const float * anim_gpu_f32s,
    const float * anim_cpu_vals,
    T1GPUzSprite * recip_gpu,
    T1CPUzSpriteSimdStats * recip_cpu)
{
    SIMD_FLOAT simd_t_now = simd_set1_float(t_now);
    SIMD_FLOAT simd_t_b4  = simd_set1_float(t_applied);
    
    log_assert((offsetof(T1GPUzSprite, base_mat_i32) / 4) % SIMD_FLOAT_LANES == 0);
    log_assert((sizeof(T1CPUzSpriteSimdStats) / 4) % SIMD_FLOAT_LANES == 0);
    
    assert_sanity_check_zsprite_vals(
        recip_gpu,
        recip_cpu);
    
    float * target_vals_ptr = (float *)recip_gpu;
    
    // Float values
    for (
        uint32_t simd_step_i = 0;
        (simd_step_i * sizeof(float)) < offsetof(T1GPUzSprite, base_mat_i32);
        simd_step_i += SIMD_FLOAT_LANES)
    {
        SIMD_FLOAT simd_anim_vals =
            simd_load_floats(
                (anim_gpu_f32s + simd_step_i));
        SIMD_FLOAT simd_target_vals =
            simd_load_floats(
                (target_vals_ptr + simd_step_i));
        
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
    
    assert_sanity_check_zsprite_vals(
        recip_gpu,
        recip_cpu);
    
    int32_t * anim_gpu_i32s = (int32_t *)((char *)anim_gpu_f32s + offsetof(T1GPUzSprite, base_mat_i32));
    int32_t * target_i32s_ptr = (int32_t *)(&recip_gpu->base_mat_i32);
    
    // Int32 values
    for (
        uint32_t simd_step_i = 0;
        (simd_step_i * sizeof(int32_t)) < (sizeof(T1GPUzSprite) - offsetof(T1GPUzSprite, base_mat_i32));
        simd_step_i += SIMD_INT32_LANES)
    {
        SIMD_FLOAT simd_anim_vals_f32 =
            simd_cast_int32s_to_floats(
                simd_load_int32s(
                (anim_gpu_i32s + simd_step_i)));
        
        SIMD_FLOAT simd_target_vals_f32 =
            simd_cast_int32s_to_floats(
                simd_load_int32s(
                (target_i32s_ptr + simd_step_i)));
        
        SIMD_FLOAT simd_t_now_deltas =
            simd_mul_floats(
                simd_anim_vals_f32,
                simd_t_now);
        SIMD_FLOAT simd_t_previous_deltas =
            simd_mul_floats(
                simd_anim_vals_f32,
                simd_t_b4);
        
        simd_t_now_deltas = simd_sub_floats(
            simd_t_now_deltas,
            simd_t_previous_deltas);
        
        SIMD_FLOAT result = simd_add_floats(
            simd_target_vals_f32,
            simd_t_now_deltas);
        
        SIMD_INT32 result_i32 =
            simd_cast_floats_to_int32s(result);
        
        simd_store_int32s(
            anim_gpu_i32s + simd_step_i,
            result_i32);
    }
    
    target_vals_ptr = (float *)recip_cpu;
    
    assert_sanity_check_zsprite_vals(
        recip_gpu,
        recip_cpu);
    
    if (!target_vals_ptr) { return; }
    
    for (
        uint32_t simd_step_i = 0;
        (simd_step_i * sizeof(float)) <
            sizeof(T1CPUzSpriteSimdStats);
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
    
    assert_sanity_check_zsprite_vals(
        recip_gpu,
        recip_cpu);
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
        
        assert_sanity_check_zsprite_vals_by_id(zp_i);
        
        float * recip_vals_cpu = (float *)
            &T1_zsprite_list->cpu_data[zp_i].
                simd_stats;
        
        for (
            uint32_t simd_step_i = 0;
            (simd_step_i * sizeof(float)) <
                sizeof(T1CPUzSpriteSimdStats);
            simd_step_i += SIMD_FLOAT_LANES)
        {
            SIMD_FLOAT simd_goal_vals =
                simd_load_floats((goal_cpu_vals +
                    simd_step_i));
            
            SIMD_FLOAT simd_cur_vals =
                simd_load_floats((recip_vals_cpu +
                    simd_step_i));
            
            SIMD_FLOAT delta_to_goal =
                simd_sub_floats(
                    simd_goal_vals,
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
            (simd_step_i * sizeof(float)) < offsetof(T1GPUzSprite, base_mat_i32);
            simd_step_i += SIMD_FLOAT_LANES)
        {
            SIMD_FLOAT simd_goal_vals =
                simd_load_floats(
                    (goal_gpu_vals + simd_step_i));
            
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
        
        assert_sanity_check_zsprite_vals_by_id(zp_i);
        
        int32_t * recip_vals_i32 = (int32_t *)
            (((void *)
                &T1_zsprite_list->gpu_data[zp_i]) +
                    offsetof(
                        T1GPUzSprite,
                        base_mat_i32));
        log_assert(recip_vals_i32[0] ==
            T1_zsprite_list->gpu_data[zp_i].base_mat_i32.texturearray_i);
        int32_t * goal_gpu_i32s = (int32_t *)
            (((void *)goal_gpu_vals) +
                offsetof(
                    T1GPUzSprite,
                    base_mat_i32));
        
        log_assert(goal_gpu_i32s[0] ==
            ((T1GPUzSprite *)goal_gpu_vals)->
                base_mat_i32.texturearray_i);
        
        for (
            uint32_t simd_step_i = 0;
            (simd_step_i * sizeof(float)) <
                (sizeof(T1GPUzSprite) -
                    offsetof(
                        T1GPUzSprite,
                        base_mat_i32));
            simd_step_i += SIMD_FLOAT_LANES)
        {
            SIMD_INT32 simd_goal_i32s =
                simd_load_int32s(
                    (goal_gpu_i32s + simd_step_i));
            
            SIMD_INT32 results_i32;
            
            SIMD_FLOAT simd_flags_f32 =
                simd_load_floats(
                    ((float *)goal_gpu_i32s + simd_step_i));
            
            simd_flags_f32 = simd_cmpeq_floats(
                simd_flags_f32,
                simd_noeffect);
            
            SIMD_INT32 simd_cur_i32s =
                    simd_load_int32s(
                        (recip_vals_i32 + simd_step_i));
            
            if (t_mult != 1.0f) {
                
                // Not every int32 is representable
                // by floats, so this code path
                // only works for very small values
                // use at own risk
                return;
                log_assert(0);
                
                SIMD_FLOAT simd_goal_f32s =
                    simd_cast_int32s_to_floats(
                        simd_goal_i32s);
                
                SIMD_FLOAT simd_cur_f32s =
                    simd_cast_int32s_to_floats(
                        simd_cur_i32s);
                
                SIMD_FLOAT delta_to_goal =
                simd_sub_floats(
                    simd_goal_f32s,
                    simd_cur_f32s);
                
                delta_to_goal = simd_mul_floats(
                    delta_to_goal,
                    simd_t);
                
                delta_to_goal = simd_and_floats(
                    delta_to_goal,
                    simd_not_floats(simd_flags_f32));
                
                simd_cur_f32s = simd_add_floats(
                    simd_cur_f32s,
                    delta_to_goal);
                
                results_i32 =
                    simd_cast_floats_to_int32s(
                        simd_cur_f32s);
            } else {
                SIMD_INT32 simd_flags_i32s;
                T1_std_memcpy(
                    &simd_flags_i32s,
                    &simd_flags_f32,
                    sizeof(SIMD_FLOAT));
                
                results_i32 = simd_add_int32s(
                    simd_and_int32s(
                        simd_goal_i32s,
                        simd_not_int32s(simd_flags_i32s)),
                    simd_and_int32s(
                        simd_cur_i32s,
                        simd_flags_i32s));
            }
            
            simd_store_int32s(
                recip_vals_i32 + simd_step_i,
                results_i32);
        }
        
        assert_sanity_check_zsprite_vals_by_id(zp_i);
    }
}

void T1_zsprite_set_occlusion(
    const int32_t zsprite_id,
    const int32_t new_visible_stat,
    const uint64_t wait_before_invis_us)
{
    for (
        int32_t zp_i = 0;
        zp_i < (int32_t)T1_zsprite_list->size;
        zp_i++)
    {
        if (!new_visible_stat)
        {
            if (wait_before_invis_us > 0)
            {
                if (
                    T1_zsprite_list->cpu_data[zp_i].
                        visible)
                {
                    T1_zsprite_list->cpu_data[zp_i].
                        next_occlusion_in_us =
                            wait_before_invis_us;
                } else {
                    T1_zsprite_list->cpu_data[zp_i].
                        next_occlusion_in_us = 0;
                }
            } else {
                T1_zsprite_list->cpu_data[zp_i].
                    next_occlusion_in_us = 0;
                T1_zsprite_list->cpu_data[zp_i].
                    visible = 0;
            }
        } else {
            T1_zsprite_list->cpu_data[zp_i].
                next_occlusion_in_us = 0;
            T1_zsprite_list->cpu_data[zp_i].
                visible = 1;
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

void T1_zsprite_copy_to_frame_data(
    T1GPUzSprite * recip,
    IdPair * recip_ids,
    uint32_t * recip_size)
{
    T1_std_memcpy(
        /* void * dest: */
            recip,
        /* const void * src: */
            T1_zsprite_list->gpu_data,
        /* size_t n: */
            sizeof(T1GPUzSprite) *
                T1_zsprite_list->size);
    
    for (
        uint32_t i = 0;
        i < T1_zsprite_list->size;
        i++)
    {
        recip_ids[i].zsprite_id = T1_zsprite_list->cpu_data[i].zsprite_id;
        recip_ids[i].touch_id = T1_zsprite_list->gpu_data[i].touch_id;
    }
    
    *recip_size = T1_zsprite_list->size;
}

void T1_add_alphablending_zpolygons_to_workload(
    T1GPUFrame * frame_data)
{
    int32_t first_alpha_i =
        (int32_t)frame_data->verts_size;
    
    // Copy all vertices that do use alpha blending
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
            !T1_zsprite_list->cpu_data[cpu_zp_i].committed ||
            !T1_zsprite_list->cpu_data[cpu_zp_i].
                alpha_blending_on ||
            T1_zsprite_list->cpu_data[cpu_zp_i].
                bloom_on)
        {
            continue;
        }
        
        int32_t mesh_id = T1_zsprite_list->cpu_data[cpu_zp_i].mesh_id;
        log_assert(mesh_id >= 0);
        log_assert(mesh_id < (int32_t)T1_objmodel_mesh_summaries_size);
        
        int32_t vert_tail_i =
            T1_objmodel_mesh_summaries[mesh_id].vertices_head_i +
                T1_objmodel_mesh_summaries[mesh_id].vertices_size;
        assert(vert_tail_i < MAX_VERTICES_PER_BUFFER);
        
        for (
            int32_t vert_i = T1_objmodel_mesh_summaries[mesh_id].vertices_head_i;
            vert_i < vert_tail_i;
            vert_i += 1)
        {
            frame_data->verts[frame_data->verts_size].
                locked_vertex_i = vert_i;
            frame_data->verts[frame_data->verts_size].
                polygon_i = cpu_zp_i;
            frame_data->verts_size += 1;
            log_assert(
                frame_data->verts_size <
                    MAX_VERTICES_PER_BUFFER);
        }
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
                        T1RENDERPASS_ALPHA_BLEND)
            {
                T1_render_views->cpu[cam_i].
                    passes[pass_i].vert_i =
                        first_alpha_i;
                T1_render_views->cpu[cam_i].
                    passes[pass_i].verts_size =
                        (int32_t)frame_data->
                            verts_size - first_alpha_i;
            }
        }
    }
}

void T1_zsprite_add_bloom_zpolygons_to_workload(
    T1GPUFrame * frame_data)
{
    int32_t first_bloom_i = (int32_t)frame_data->
        verts_size;
    
    // Copy all vertices that do use bloom
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
            !T1_zsprite_list->cpu_data[cpu_zp_i].
                bloom_on)
        {
            continue;
        }
        
        log_assert(
            !T1_zsprite_list->cpu_data[cpu_zp_i].
                alpha_blending_on);
        log_assert(
            T1_zsprite_list->cpu_data[cpu_zp_i].
                bloom_on);
        
        int32_t mesh_id =
            T1_zsprite_list->cpu_data[cpu_zp_i].
                mesh_id;
        log_assert(mesh_id >= 0);
        log_assert(mesh_id < (int32_t)T1_objmodel_mesh_summaries_size);
        
        int32_t vert_tail_i =
            T1_objmodel_mesh_summaries[mesh_id].
                vertices_head_i +
                    T1_objmodel_mesh_summaries[mesh_id].
                        vertices_size;
        assert(vert_tail_i < MAX_VERTICES_PER_BUFFER);
        
        for (
            int32_t vert_i =
                T1_objmodel_mesh_summaries[mesh_id].
                    vertices_head_i;
            vert_i < vert_tail_i;
            vert_i += 1)
        {
            frame_data->verts[frame_data->verts_size].
                locked_vertex_i = vert_i;
            frame_data->verts[frame_data->verts_size].
                polygon_i = cpu_zp_i;
            frame_data->verts_size += 1;
            log_assert(
                frame_data->verts_size <
                    MAX_VERTICES_PER_BUFFER);
        }
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
                T1_render_views->cpu[cam_i].
                    passes[pass_i].type ==
                        T1RENDERPASS_BLOOM)
            {
                T1_render_views->cpu[cam_i].
                    passes[pass_i].vert_i =
                        first_bloom_i;
                log_assert(frame_data->
                    verts_size < INT32_MAX);
                T1_render_views->cpu[cam_i].
                    passes[pass_i].verts_size =
                        (int32_t)frame_data->
                            verts_size -
                                first_bloom_i;
            }
        }
    }
}


void T1_add_opaque_zpolygons_to_workload(
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
        log_assert(mesh_id < (int32_t)
            T1_objmodel_mesh_summaries_size);
        
        int32_t vert_tail_i =
            T1_objmodel_mesh_summaries[mesh_id].
                vertices_head_i +
                    T1_objmodel_mesh_summaries
                        [mesh_id].vertices_size;
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
        
        for (
            int32_t i = 0;
            i < verts_to_copy;
            i += 2)
        {
            cur = simd_add_vec4i(cur, incr);
            simd_store_vec4i(
                (frame_data->verts +
                    frame_data->verts_size),
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

void
T1_zsprite_construct_model_and_normal_matrices(void)
{
    T1_linal_float4x4 result;
    T1_linal_float4x4 next;
    
    T1_linal_float3x3 model3x3;
    // T1_linal_float3x3 view3x3;
    
    for (
        uint32_t i = 0;
        i < T1_zsprite_list->size;
        i++)
    {
        
        T1CPUzSpriteSimdStats * s =
            &T1_zsprite_list->cpu_data[i].
                simd_stats;
        
        T1_linal_float4x4_construct_identity(&result);
        
        // Translation
        T1_linal_float4x4_construct(
            &next,
            1.0f, 0.0f, 0.0f, s->xyz[0],
            0.0f, 1.0f, 0.0f, s->xyz[1],
            0.0f, 0.0f, 1.0f, s->xyz[2],
            0.0f, 0.0f, 0.0f, 1.0f);
        
        T1_linal_float4x4_mul_float4x4_inplace(
            &result, &next);
        
        T1_linal_float4x4_construct_xyz_rotation(
            &next,
            s->angle_xyz[0],
            s->angle_xyz[1],
            s->angle_xyz[2]);
        
        T1_linal_float4x4_mul_float4x4_inplace(
            &result,
            &next);
        
        T1_linal_float4x4_construct(
            &next,
            1.0f, 0.0f, 0.0f, s->offset_xyz[0],
            0.0f, 1.0f, 0.0f, s->offset_xyz[1],
            0.0f, 0.0f, 1.0f, s->offset_xyz[2],
            0.0f, 0.0f, 0.0f, 1.0f);
        
        T1_linal_float4x4_mul_float4x4_inplace(
            &result, &next);
        
        T1_linal_float4x4_construct(
            &next,
            s->mul_xyz[0], 0.0f, 0.0f, 0.0f,
            0.0f, s->mul_xyz[1], 0.0f, 0.0f,
            0.0f, 0.0f, s->mul_xyz[2], 0.0f,
            0.0f, 0.0f, 0.0f, 1.0f);
        T1_linal_float4x4_mul_float4x4_inplace(
            &result, &next);
        
        T1_std_memcpy(
            T1_zsprite_list->gpu_data[i].
                m_4x4 + 0,
            result.rows[0].data,
            sizeof(float) * 4);
        T1_std_memcpy(
            T1_zsprite_list->gpu_data[i].
                m_4x4 + 4,
            result.rows[1].data,
            sizeof(float) * 4);
        T1_std_memcpy(
            T1_zsprite_list->gpu_data[i].
                m_4x4 + 8,
            result.rows[2].data,
            sizeof(float) * 4);
        T1_std_memcpy(
            T1_zsprite_list->gpu_data[i].
                m_4x4 + 12,
            result.rows[3].data,
            sizeof(float) * 4);
        
        // Next: transforming normals
        // store topleft 3x3 of "model to world"
        // matrix in model3x3
        T1_linal_float4x4_extract_float3x3(
            /* const T1_linal_float4x4 * in: */
                &result,
            /* const int omit_row_i: */
                3,
            /* const int omit_col_i: */
                3,
            /* T1_linal_float3x3 * out: */
                &model3x3);
        
        // inverse transpose the topleft 3x3
        T1_linal_float3x3_inverse_transpose_inplace(&model3x3);
        
        // store as the "normal to world" matrix
        T1_zsprite_list->gpu_data[i].
            norm_3x3[0] = model3x3.rows[0].data[0];
        T1_zsprite_list->gpu_data[i].
            norm_3x3[1] = model3x3.rows[0].data[1];
        T1_zsprite_list->gpu_data[i].
            norm_3x3[2] = model3x3.rows[0].data[2];
        T1_zsprite_list->gpu_data[i].
            norm_3x3[3] = model3x3.rows[1].data[0];
        T1_zsprite_list->gpu_data[i].
            norm_3x3[4] = model3x3.rows[1].data[1];
        T1_zsprite_list->gpu_data[i].
            norm_3x3[5] = model3x3.rows[1].data[2];
        T1_zsprite_list->gpu_data[i].
            norm_3x3[6] = model3x3.rows[2].data[0];
        T1_zsprite_list->gpu_data[i].
            norm_3x3[7] = model3x3.rows[2].data[1];
        T1_zsprite_list->gpu_data[i].
            norm_3x3[8] = model3x3.rows[2].data[2];
    }
}
