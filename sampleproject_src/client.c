#include "T1_client.h"

#include "T1.h"
#include "T1_settings.h"

static int32_t img_T1_id;
static uint8_t img_toggle = 0;
static void draw_test_quad(void) {
    img_T1_id = T1_id_next_nonui_id();;
    
    T1zSpriteRequest img;
    T1_zsprite_fetch_next_noconstruct(&img);
    T1_zsprite_construct_quad(0.2f, 0.2f, 0.50f, 0.25f, 0.25f, &img);
    T1_log_assert(img.cpu_data->mesh_id == T1_BASIC_QUAD_MESH_ID);
    T1Tex tex = T1_tex_array_get_filename_loc("structuredart1.png");
    img.gpu_data->s32.base_mat_s32.normalmap_tex_and_tex = tex;
    T1_log_assert(img.gpu_data->f32s.base_mat_f32.alpha == 1.0f);
    img.cpu_data->T1_id = img_T1_id;
    T1_log_assert(img.cpu_data->simd_stats.mul_xyz[0] > 0.02f);
    T1_log_assert(img.cpu_data->simd_stats.mul_xyz[1] > 0.02f);
    T1_log_assert(img.cpu_data->simd_stats.mul_xyz[2] > 0.02f);
    img.gpu_data->f32s.no_lighting = 1.0f;
    //    img.cpu_data->simd_stats.xyz[0] = 0.0f;
    //    img.cpu_data->simd_stats.xyz[1] = 0.0f;
    //    img.cpu_data->simd_stats.xyz[2] = 0.25f;
    T1_log_assert(img.cpu_data->simd_stats.angle_xyz[0] == 0.0f);
    T1_log_assert(img.cpu_data->simd_stats.angle_xyz[1] == 0.0f);
    T1_log_assert(img.cpu_data->simd_stats.angle_xyz[2] == 0.0f);
    T1_log_assert(img.cpu_data->simd_stats.scale_factor == 1.0f);
    T1_log_assert(img.gpu_data->f32s.alpha == 1.0f);
    T1_zsprite_commit(&img);
}

void T1_client_init(void) {
    return;
}

static s32 mainwindow_scene_id = -1;
void T1_client_early_startup(
    b8 * success,
    char * error_message)
{
    b8 good;
    T1_tex_files_prereg_png_res(
        "structuredart1.png", &good);
    assert(good);
    T1_tex_files_prereg_png_res(
        "structuredart2.png", &good);
    assert(good);
    
    mainwindow_scene_id = T1_io_create_scene_and_return_id();
    
    *success = true;
}

void T1_client_late_startup(void) {
    
    T1_cam->xyz[0] =  0.00f;
    T1_cam->xyz[1] =  0.00f;
    T1_cam->xyz[2] = -0.50f;
    T1_cam->angle_xyz[0] =  0.0f;
    T1_cam->angle_xyz[1] =  0.0f;
    T1_cam->angle_xyz[2] =  0.0f;
}

void T1_client_threadmain(int32_t threadmain_id) {
    switch (threadmain_id) {
        default:
            T1_log_append("unhandled threadmain_id: ");
            T1_log_append_int(threadmain_id);
            T1_log_append("\n");
    }
}

static uint32_t testswitch = 0;
static void client_handle_keypresses(
    uint64_t microseconds_elapsed)
{
    float elapsed_mod = (float)(
        (double)microseconds_elapsed / (double)16666);
    float cam_speed = 0.1f * elapsed_mod;
    float cam_rotation_speed = 0.05f * elapsed_mod;
    
    if (T1_io_key_consume_short_tap_this_frame(
        T1_IO_KEYBOARD_D, mainwindow_scene_id))
    {
        draw_test_quad();
    }
    
    if (T1_io_key_consume_short_tap_this_frame(
        T1_IO_KEYBOARD_R, mainwindow_scene_id))
    {
        T1zSpriteAnim * rot = T1_zsprite_anim_request_next(true);
        rot->affected_T1_id = img_T1_id;
        rot->cpu_vals.xyz[0] = -0.25f;
        rot->cpu_vals.xyz[1] =  0.00f;
        rot->cpu_vals.xyz[2] =  0.35f;
        rot->duration_us = 250000;
        rot->cpu_vals.angle_xyz[2] = img_toggle ? 3.14159f : 0.0f;
        rot->cpu_vals_active = true;
        T1_zsprite_anim_commit(rot, "get rid of this");
        
        img_toggle = !img_toggle;
    }
    
    if (T1_io_key_is_down(
        T1_IO_KEYBOARD_LEFTARROW,
        mainwindow_scene_id))
    {
        T1_cam->xyz[0] -= cam_speed;
    }
    
    if (T1_io_key_is_down(
        T1_IO_KEYBOARD_RIGHTARROW,
        mainwindow_scene_id))
    {
        T1_cam->xyz[0] += cam_speed;
    }
    
    if (T1_io_key_is_down(
        T1_IO_KEYBOARD_DOWNARROW,
        mainwindow_scene_id))
    {
        T1_cam->xyz[1] -= cam_speed;
    }
    
    if (T1_io_key_is_down(
        T1_IO_KEYBOARD_UPARROW,
        mainwindow_scene_id))
    {
        T1_cam->xyz[1] += cam_speed;
    }
    
    if (T1_io_key_is_down(
        T1_IO_KEYBOARD_A,
        mainwindow_scene_id))
    {
        T1_cam->angle_xyz[0] +=
            cam_rotation_speed;
    }
    
    if (T1_io_key_is_down(
        T1_IO_KEYBOARD_Z,
        mainwindow_scene_id))
    {
        T1_cam->angle_xyz[2] -= cam_rotation_speed;
    }
    
    if (T1_io_key_is_down(
        T1_IO_KEYBOARD_X,
        mainwindow_scene_id))
    {
        T1_cam->angle_xyz[2] += cam_rotation_speed;
    }
    
    if (T1_io_key_is_down(
        T1_IO_KEYBOARD_Q,
        mainwindow_scene_id))
    {
        T1_cam->angle_xyz[0] -= cam_rotation_speed;
    }
    
    if (T1_io_key_is_down(
        T1_IO_KEYBOARD_W,
        mainwindow_scene_id))
    {
        T1_cam->angle_xyz[1] -=
            cam_rotation_speed;
    }
    
    if (T1_io_key_is_down(
        T1_IO_KEYBOARD_S,
        mainwindow_scene_id))
    {
        T1_cam->angle_xyz[1] +=
            cam_rotation_speed;
    }
    
    if (T1_io_key_consume_short_tap_this_frame(
        T1_IO_KEYBOARD_M,
        mainwindow_scene_id))
    {
        testswitch = !testswitch;
        
        T1TexQuadAnim * anim =
            T1_texquad_anim_request_next(true);
        anim->gpu_vals.f32s.xyz[0] =
            testswitch ? -0.50f : 0.5f;
        anim->gpu_vals.f32s.rgba[0] =
            testswitch ? 1.0f : 0.25f;
        anim->gpu_vals.f32s.rgba[1] =
            testswitch ? 0.0f : 0.25f;
        anim->gpu_vals.f32s.rgba[2] =
            testswitch ? 0.25f : 1.0f;
        anim->affect_T1_id = 21;
        anim->easing_type =
            EASINGTYPE_EASEOUT_ELASTIC_ZERO_TO_ONE;
        anim->duration_us = 250000;
        anim->gpu_f32_active = true;
        anim->gpu_s32_active = false;
        anim->del_conflict_anims = true;
        T1_texquad_anim_commit(anim);
    }
    
    if (T1_io_key_is_down(
        T1_IO_KEYBOARD_BACKSLASH,
        mainwindow_scene_id))
    {
        // / key
        T1_cam->xyz[2] -= 0.01f;
    }
    
    if (T1_io_key_is_down(
        T1_IO_KEYBOARD_FULLSTOP,
        mainwindow_scene_id))
    {
        T1_cam->xyz[2] += 0.01f;
    }
}

void T1_client_update(uint64_t microseconds_elapsed)
{
    client_handle_keypresses(microseconds_elapsed);
}

void T1_client_update_after_render_pass(void) {
    // you can make edits after the objects are copied to the framebuffer
    // and rendered
}

void T1_client_evaluate_terminal_command(
    char * command,
    char * response,
    const uint32_t response_cap)
{
    if (T1_std_are_equal_strings(command, "EXAMPLE COMMAND")) {
        T1_std_strcpy_cap(response, response_cap, "Hello from client!");
        return;
    }
    
    if (T1_std_are_equal_strings(
        command,
        "TEMPDEBUG"))
    {
        T1_cam->xyz[0] = -6.09f;
        T1_cam->xyz[1] = 11.547f;
        T1_cam->xyz[2] = 0.0999f;
        T1_cam->angle_xyz[0] = 1.053f;
        T1_cam->angle_xyz[1] = 0.20f;
        T1_cam->angle_xyz[2] = -102.92f;
        T1_std_strcpy_cap(response, response_cap, "Set camera to the debug scene");
        return;
    }
    
    T1_std_strcpy_cap(
        response,
        response_cap,
        "Unrecognized command - see client_logic_evaluate_terminal_command() "
        "in client.c");
}

void T1_client_window_resize(void)
{    
    T1_texquad_delete_all();
    T1_zsprite_anim_delete_all();
    
    T1_cam_delete_all();
    T1_cam_create_main_view(
        T1_settings_get_render_width(),
        T1_settings_get_render_height());
    
    draw_test_quad();
}

void T1_client_shutdown(void) {
    // You're notified that your application is about to shut down
}
