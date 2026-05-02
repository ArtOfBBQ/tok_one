#include "T1_clientlogic.h"

static int32_t img_zsprite_id;
static uint8_t img_toggle = 0;
static void draw_test_quad(void) {
    img_zsprite_id = T1_zspriteid_next_nonui_id();;
    
    T1zSpriteRequest img;
    T1_zsprite_fetch_next_noconstruct(&img);
    T1_zsprite_construct_quad(0.2f, 0.2f, 0.50f, 0.25f, 0.25f, &img);
    T1_log_assert(img.cpu_data->mesh_id == T1_BASIC_QUAD_MESH_ID);
    T1Tex tex = T1_tex_array_get_filename_loc("structuredart1.png");
    img.gpu_data->i32.base_mat_i32.texturearray_i = tex.array_i;
    img.gpu_data->i32.base_mat_i32.texture_i = tex.slice_i;
    T1_log_assert(img.gpu_data->i32.base_mat_i32.texturearray_i >= 1);
    T1_log_assert(img.gpu_data->i32.base_mat_i32.texture_i >= 0);
    T1_log_assert(img.gpu_data->f32.base_mat_f32.alpha == 1.0f);
    img.cpu_data->zsprite_id = img_zsprite_id;
    T1_log_assert(img.cpu_data->simd_stats.mul_xyz[0] > 0.02f);
    T1_log_assert(img.cpu_data->simd_stats.mul_xyz[1] > 0.02f);
    T1_log_assert(img.cpu_data->simd_stats.mul_xyz[2] > 0.02f);
    img.gpu_data->f32.ignore_lighting = 1.0f;
    //    img.cpu_data->simd_stats.xyz[0] = 0.0f;
    //    img.cpu_data->simd_stats.xyz[1] = 0.0f;
    //    img.cpu_data->simd_stats.xyz[2] = 0.25f;
    T1_log_assert(img.cpu_data->simd_stats.angle_xyz[0] == 0.0f);
    T1_log_assert(img.cpu_data->simd_stats.angle_xyz[1] == 0.0f);
    T1_log_assert(img.cpu_data->simd_stats.angle_xyz[2] == 0.0f);
    T1_log_assert(img.cpu_data->simd_stats.scale_factor == 1.0f);
    T1_log_assert(img.gpu_data->f32.alpha == 1.0f);
    T1_zsprite_commit(&img);
}

void T1_clientlogic_init(void) {
    return;
}

void T1_clientlogic_early_startup(
    bool32_t * success,
    char * error_message)
{
    uint32_t good;
    T1_tex_files_prereg_png_res(
        "structuredart1.png", &good);
    assert(good);
    T1_tex_files_prereg_png_res(
        "structuredart2.png", &good);
    assert(good);
    
    *success = true;
}

void T1_clientlogic_late_startup(void) {
    
    T1_camera->xyz[0] =  0.00f;
    T1_camera->xyz[1] =  0.00f;
    T1_camera->xyz[2] = -0.50f;
    T1_camera->xyz_angle[0] =  0.0f;
    T1_camera->xyz_angle[1] =  0.0f;
    T1_camera->xyz_angle[2] =  0.0f;
}

void T1_clientlogic_threadmain(int32_t threadmain_id) {
    switch (threadmain_id) {
        default:
            T1_log_append("unhandled threadmain_id: ");
            T1_log_append_int(threadmain_id);
            T1_log_append("\n");
    }
}

static uint32_t testswitch = 0;
static void clientlogic_handle_keypresses(
    uint64_t microseconds_elapsed)
{
    float elapsed_mod = (float)(
        (double)microseconds_elapsed / (double)16666);
    float cam_speed = 0.1f * elapsed_mod;
    float cam_rotation_speed = 0.05f * elapsed_mod;
    
    if (T1_io_keymap[T1_IO_KEY_D] == true)
    {
        T1_io_keymap[T1_IO_KEY_D] = false;
        draw_test_quad();
    }
    
    if (T1_io_keymap[T1_IO_KEY_R] == true)
    {
        T1_io_keymap[T1_IO_KEY_R] = false;
        
        T1zSpriteAnim * rot = T1_zsprite_anim_request_next(true);
        rot->affected_zsprite_id = img_zsprite_id;
        rot->cpu_vals.xyz[0] = -0.25f;
        rot->cpu_vals.xyz[1] =  0.00f;
        rot->cpu_vals.xyz[2] =  0.35f;
        rot->duration_us = 250000;
        rot->cpu_vals.angle_xyz[2] = img_toggle ? 3.14159f : 0.0f;
        rot->cpu_vals_active = true;
        T1_zsprite_anim_commit(rot);
        
        img_toggle = !img_toggle;
    }
    
    if (T1_io_keymap[T1_IO_KEY_LEFTARROW] == true)
    {
        T1_camera->xyz[0] -= cam_speed;
    }
    
    if (T1_io_keymap[T1_IO_KEY_RIGHTARROW] == true)
    {
        T1_camera->xyz[0] += cam_speed;
    }
    
    if (T1_io_keymap[T1_IO_KEY_DOWNARROW] == true)
    {
        T1_camera->xyz[1] -= cam_speed;
    }
    
    if (T1_io_keymap[T1_IO_KEY_UPARROW] == true)
    {
        T1_camera->xyz[1] += cam_speed;
    }
    
    if (T1_io_keymap[T1_IO_KEY_A] == true) {
        T1_camera->xyz_angle[0] +=
            cam_rotation_speed;
    }
    
    if (T1_io_keymap[T1_IO_KEY_Z] == true) {
        T1_camera->xyz_angle[2] -= cam_rotation_speed;
    }
    
    if (
        T1_io_keymap[T1_IO_KEY_X] == true)
    {
        T1_camera->xyz_angle[2] += cam_rotation_speed;
    }
    
    if (T1_io_keymap[T1_IO_KEY_Q] == true)
    {
        T1_camera->xyz_angle[0] -= cam_rotation_speed;
    }
    
    if (T1_io_keymap[T1_IO_KEY_W] == true) {
        T1_camera->xyz_angle[1] -=
            cam_rotation_speed;
    }
    
    if (T1_io_keymap[T1_IO_KEY_S] == true) {
        T1_camera->xyz_angle[1] +=
            cam_rotation_speed;
    }
    
    if (T1_io_keymap[T1_IO_KEY_M] == true) {
        T1_io_keymap[T1_IO_KEY_M] = false;
        
        testswitch = !testswitch;
        
        T1TexQuadAnim * anim =
            T1_texquad_anim_request_next(true);
        anim->gpu_vals.f32.xyz[0] =
            testswitch ? -0.50f : 0.5f;
        anim->gpu_vals.f32.rgba[0] =
            testswitch ? 1.0f : 0.25f;
        anim->gpu_vals.f32.rgba[1] =
            testswitch ? 0.0f : 0.25f;
        anim->gpu_vals.f32.rgba[2] =
            testswitch ? 0.25f : 1.0f;
        anim->affect_zsprite_id = 21;
        anim->easing_type =
            EASINGTYPE_EASEOUT_ELASTIC_ZERO_TO_ONE;
        anim->duration_us = 250000;
        anim->gpu_f32_active = true;
        anim->gpu_i32_active = false;
        anim->del_conflict_anims = true;
        T1_texquad_anim_commit(anim);
    }
    
    if (
        T1_io_keymap[T1_IO_KEY_BACKSLASH] == true)
    {
        // / key
        T1_camera->xyz[2] -= 0.01f;
    }
    
    if (T1_io_keymap[T1_IO_KEY_FULLSTOP] == true) {
        T1_camera->xyz[2] += 0.01f;
    }
}

void T1_clientlogic_update(uint64_t microseconds_elapsed)
{
    if (
        !T1_io_events[T1_IO_LAST_TOUCH_OR_LCLICK_START].handled)
    {
        T1_io_events[T1_IO_LAST_TOUCH_OR_LCLICK_START].
            handled = true;
        
        if (
            T1_io_events[T1_IO_LAST_TOUCH_OR_LCLICK_START].
                touch_id_top == 5 ||
            T1_io_events[T1_IO_LAST_TOUCH_OR_LCLICK_START].
                touch_id_pierce == 5)
        {
            #if T1_ZSPRITE_ANIM_ACTIVE == T1_ACTIVE
            T1_zsprite_anim_bump(
                /* const int32_t object_id: */
                    20,
                /* const uint32_t wait: */
                    0);
            #elif T1_ZSPRITE_ANIM_ACTIVE == T1_INACTIVE
            #else
            #error
            #endif
        }
    }
    
    if (
        !T1_io_events[T1_IO_LAST_RCLICK_START].handled)
    {
        T1_io_events[T1_IO_LAST_RCLICK_START].handled = true;
    }
    
    if (
        !T1_io_events[T1_IO_LAST_MOUSE_OR_TOUCH_MOVE].handled)
    {
        T1_io_events[T1_IO_LAST_MOUSE_OR_TOUCH_MOVE].handled = true;
    }
    
    clientlogic_handle_keypresses(microseconds_elapsed);
}

void T1_clientlogic_update_after_render_pass(void) {
    // you can make edits after the objects are copied to the framebuffer
    // and rendered
}

void T1_clientlogic_evaluate_terminal_command(
    char * command,
    char * response,
    const uint32_t response_cap)
{
    if (T1_std_are_equal_strings(command, "EXAMPLE COMMAND")) {
        T1_std_strcpy_cap(response, response_cap, "Hello from clientlogic!");
        return;
    }
    
    if (T1_std_are_equal_strings(
        command,
        "TEMPDEBUG"))
    {
        T1_camera->xyz[0] = -6.09f;
        T1_camera->xyz[1] = 11.547f;
        T1_camera->xyz[2] = 0.0999f;
        T1_camera->xyz_angle[0] = 1.053f;
        T1_camera->xyz_angle[1] = 0.20f;
        T1_camera->xyz_angle[2] = -102.92f;
        T1_std_strcpy_cap(response, response_cap, "Set camera to the debug scene");
        return;
    }
    
    T1_std_strcpy_cap(
        response,
        response_cap,
        "Unrecognized command - see client_logic_evaluate_terminal_command() "
        "in clientlogic.c");
}

void T1_clientlogic_window_resize(
    const uint32_t new_width,
    const uint32_t new_height)
{
    T1_tex_array_delete_slice(
        T1_render_views->cpu[0].write_array_i,
        T1_render_views->cpu[0].write_slice_i);
    T1_render_views->cpu[0].write_array_i = -1;
    T1_render_views->cpu[0].write_slice_i = -1;
    
    T1_render_view_delete(0);
    
    int32_t rv_i =
        T1_tex_array_create_new_render_view(
            new_width,
            new_height);
    
    T1_render_views->cpu[rv_i].passes_size = 6;
    T1_log_assert(
        T1_render_views->cpu[rv_i].write_type ==
            T1RENDERVIEW_WRITE_RENDER_TARGET);
    T1_render_views->cpu[rv_i].passes[0].type =
        T1RENDERPASS_DIAMOND_ALPHA;
    T1_render_views->cpu[rv_i].passes[1].type =
        T1RENDERPASS_ALPHA_BLEND;
    T1_render_views->cpu[rv_i].passes[2].type =
        T1RENDERPASS_OUTLINES;
    T1_render_views->cpu[rv_i].passes[3].type =
        T1RENDERPASS_BLOOM;
    T1_render_views->cpu[rv_i].passes[4].type =
        T1RENDERPASS_FLAT_TEXQUADS;
    T1_render_views->cpu[rv_i].passes[5].type =
        T1RENDERPASS_BILLBOARDS;
    
    draw_test_quad();
}

void T1_clientlogic_shutdown(void) {
    // You're notified that your application is about to shut down
}
