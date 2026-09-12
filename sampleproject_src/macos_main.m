#include "T1.h"

static int main_errorbox_then_exit(
    const char * error_message,
    int argc,
    const char * argv[])
{
    // T1_os_destroy_main_window_if_possible();
    T1_os_request_messagebox(error_message);
    
    @autoreleasepool {
        return NSApplicationMain(argc, argv);
    }
}

static u32 img_T1_ids[2];
static  b8 img_T1_ids_set = 0;
static f32 img_x, img_y;
static void redraw_test_quads(f32 x, f32 y) {
    if (!img_T1_ids_set) {
        img_T1_ids[0] = T1_id_next_nonui_id();
        img_T1_ids[1] = T1_id_next_nonui_id();
    }
    
    T1_zsprite_delete(img_T1_ids[0]);
    T1_texquad_delete(img_T1_ids[1]);
    
    T1zSpriteRequest img;
    T1_zsprite_fetch_next_noconstruct(&img);
    T1_zsprite_construct_quad(x, y, 0.50f, 0.25f, 0.25f, &img);
    T1_log_assert(img.cpu_data->mesh_id == T1_BASIC_QUAD_MESH_ID);
    T1Tex tex = T1_tex_array_get_filename_loc("structuredart1.png");
    img.gpu_data->base_mat_u32.normalmap_tex_and_tex = tex;
    T1_log_assert(img.gpu_data->base_mat_f32.alpha == 1.0f);
    img.cpu_data->T1_id = img_T1_ids[0];
    T1_log_assert(img.cpu_data->zs_cpu_f32s.mul_xyz[0] > 0.02f);
    T1_log_assert(img.cpu_data->zs_cpu_f32s.mul_xyz[1] > 0.02f);
    T1_log_assert(img.cpu_data->zs_cpu_f32s.mul_xyz[2] > 0.02f);
    img.gpu_data->f32s.no_light = 1.0f;
    //    img.cpu_data->simd_stats.xyz[0] = 0.0f;
    //    img.cpu_data->simd_stats.xyz[1] = 0.0f;
    //    img.cpu_data->simd_stats.xyz[2] = 0.25f;
    T1_log_assert(img.cpu_data->zs_cpu_f32s.angle_xyz[0] == 0.0f);
    T1_log_assert(img.cpu_data->zs_cpu_f32s.angle_xyz[1] == 0.0f);
    T1_log_assert(img.cpu_data->zs_cpu_f32s.angle_xyz[2] == 0.0f);
    T1_log_assert(img.gpu_data->f32s.alpha == 1.0f);
    T1_zsprite_commit(&img);
    
    T1TexQuadRequest tq_req;
    T1_texquad_fetch_next(&tq_req);
    tq_req.cpu->T1_id = img_T1_ids[1];
    tq_req.cpu->one_frame_only = 0;
    tq_req.gpu->f32s.xyz[0] = x - 0.2f;
    tq_req.gpu->f32s.xyz[1] = y + 0.1f;
    tq_req.gpu->f32s.xyz[2] = 0.5f;
    tq_req.gpu->f32s.wh[0] = 0.5f;
    tq_req.gpu->f32s.wh[1] = 0.5f;
    tq_req.gpu->f32s.rgba[3] = 0.75f;
    tq_req.gpu->u32s.reserved_and_tex = tex;
    T1_texquad_commit(&tq_req);
}


static void example_callback_windowwillresize(void)
{    
    T1_texquad_delete_all();
    T1_anim_delete_all();
    
    T1_cam_delete_all();
    T1_cam_create_main_view(
        T1_settings_get_render_width(),
        T1_settings_get_render_height());
    
    redraw_test_quads(img_x, img_y);
}

static void example_callback_threadstart(s32 threadmain_id) {
    switch (threadmain_id) {
        default:
            T1_log_append("unhandled threadmain_id: ");
            T1_log_append_s32(threadmain_id);
            T1_log_append("\n");
    }
}

static s32 mainwindow_scene_id = -1;
static u32 testswitch = 0;

static void example_callback_update(u64 microseconds_elapsed)
{
    float elapsed_mod = (float)(
        (double)microseconds_elapsed / (double)16666);
    float cam_speed = 0.1f * elapsed_mod;
    float cam_rotation_speed = 0.05f * elapsed_mod;
    
    if (T1_io_key_consume_short_tap_this_frame(
        T1_IO_KEYBOARD_D, mainwindow_scene_id))
    {
        redraw_test_quads(img_x, img_y);
    }
    
    if (T1_io_key_consume_short_tap_this_frame(
        T1_IO_GAMEPAD_DPAD_LEFT,
        mainwindow_scene_id))
    {
        img_x -= 0.10f;
        redraw_test_quads(img_x, img_y);
    }
    
    if (T1_io_key_consume_short_tap_this_frame(
        T1_IO_KEYBOARD_T, mainwindow_scene_id))
    {
        #if T1_ANIM_ACTIVE == T1_ACTIVE  
        testswitch = !testswitch;      
        T1Anim * alpha = T1_anim_request_next(
            /* b8 endps_not_deltas: */ true,
            /* b8 zs_gpu_f32s:      */ true,
            /* b8 zs_cpu_f32s:      */ false,
            /* b8 zs_gpu_s32s:      */ false,
            /* b8 tq_gpu_f32s:      */ true,
            /* b8 tq_gpu_s32s:      */ false,
            /* b8 zl_gpu_f32s:      */ false);
        alpha->zs_gpu_f32s->alpha =
            testswitch ? 1.0f : 0.0f;
        alpha->tq_gpu_f32s->rgba[3] =
            testswitch ? 1.00f : 0.0f;
        alpha->target_T1_id = img_T1_ids[0];
        alpha->duration_us = 225000;
        T1_anim_commit(alpha, "delete me");
        #elif T1_ANIM_ACTIVE == T1_INACTIVE
        #else
        #error
        #endif
    }
    
    if (T1_io_key_consume_short_tap_this_frame(
        T1_IO_KEYBOARD_R, mainwindow_scene_id))
    {
        #if T1_ANIM_ACTIVE == T1_ACTIVE
        T1Anim * rot = T1_anim_request_next(
            /* b8 endpoints_not_delt: */ false,
            /* b8 zs_gpu_f32s:        */ true,
            /* b8 zs_cpu_f32s:        */ true,
            /* b8 zs_gpu_s32s:        */ false,
            /* b8 tq_gpu_f32s:        */ false,
            /* b8 tq_gpu_s32s:        */ false,
            /* b8 zl_gpu_f32s:        */ false);
        rot->target_T1_id = img_T1_ids[0];
        rot->easing_type = T1_EASINGTYPE_OUT_QUADRATIC;
        rot->duration_us = 250000;
        rot->zs_cpu_f32s->angle_xyz[2] = 3.14159f;
        rot->zs_gpu_f32s->bonus_rgb[1] = 0.02f;
        T1_anim_commit(rot, "get rid of this");
        
        T1Anim * rot2 = T1_anim_request_next(
            /* b8 endpoints_not_deltas: */ false,
            /* b8 zs_gpu_f32s: */ false,
            /* b8 zs_cpu_f32s: */ false,
            /* b8 zs_gpu_s32s: */ false,
            /* b8 tq_gpu_f32s: */ true,
            /* b8 tq_gpu_s32s: */ false,
            /* b8 zl_gpu_f32s: */ false);
        rot2->tq_gpu_f32s->xyz[0] = -0.2f;
        rot2->tq_gpu_f32s->xyz[1] = -0.1f;
        rot2->tq_gpu_f32s->rgba[3] = -0.5f;
        rot2->target_T1_id = img_T1_ids[1];
        rot2->easing_type = T1_EASINGTYPE_SINGLE_PULSE_ZERO_TO_ZERO;
        rot2->duration_us = 225000;
        T1_anim_commit(rot2, "delete me");
        #elif T1_ANIM_ACTIVE == T1_INACTIVE
        #else
        #error
        #endif
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
        T1_cam->angle_xyz[1] += cam_rotation_speed;
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

static void example_callback_evaluate_terminal_command(
    char * command,
    char * response,
    u32 response_cap)
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

static void example_callback_onappclose(void) {
    // do stuff!
}

int main(int argc, const char * argv[]) {
    
    {
    char errmsg[512];
    uint8_t success = 0;
    T1_appinit_before_gpu_init(
        example_callback_threadstart,
        example_callback_update,
        example_callback_windowwillresize,
        example_callback_onappclose,
        example_callback_evaluate_terminal_command,
        &success,
        errmsg,
        512);
    
    if (!success) {
        return main_errorbox_then_exit(errmsg, argc, argv);
    }
    
    T1_os_create_main_window(&success);
    if (!success) {
        return main_errorbox_then_exit(errmsg, argc, argv);
    }
    
    if (!success) {
        return main_errorbox_then_exit(errmsg, argc, argv);
    }
    
    T1_os_link_gpu_to_main_window(errmsg, 512, &success);
    if (!success) {
        return main_errorbox_then_exit(errmsg, argc, argv);
    }
    
    T1_appinit_after_gpu_init_step1(
        &success,
        errmsg,
        512);
    
    if (!success) {
        main_errorbox_then_exit(errmsg, argc, argv);
    }
    }
    
    T1_cam->xyz[0] =  0.00f;
    T1_cam->xyz[1] =  0.00f;
    T1_cam->xyz[2] = -0.50f;
    T1_cam->angle_xyz[0] =  0.0f;
    T1_cam->angle_xyz[1] =  0.0f;
    T1_cam->angle_xyz[2] =  0.0f;
    
    b8 good;
    T1_tex_files_prereg_png_res(
        "structuredart1.png", &good);
    assert(good);
    T1_tex_files_prereg_png_res(
        "structuredart2.png", &good);
    assert(good);
    
    mainwindow_scene_id = T1_io_create_scene_and_return_id();
    T1_io_scene_stack_push(mainwindow_scene_id);
    
    T1_os_start_thread(
        T1_appinit_after_gpu_init_step2,
        0);
    
    @autoreleasepool {
        return NSApplicationMain(argc, argv);
    }
}
