#include "T1_clientlogic.h"

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
    T1_tex_files_prereg_png_res(
        "phoebus.png", &good);
    assert(good);
    
    *success = true;
}

static void redraw_quads(void) {
    #if 1
    T1Tex quad_texs[2];
    quad_texs[0] = T1_tex_array_get_filename_loc("phoebus.png");
    
    uint32_t tex_w = T1_tex_arrays[quad_texs[0].array_i].
        single_img_width;
    uint32_t tex_h = T1_tex_arrays[quad_texs[0].array_i].single_img_height;
    uint32_t rgba_size = tex_w * tex_h * 4;
    uint8_t rgba[rgba_size];
    
    
    uint8_t bonus = (T1_global->this_frame_timestamp_us / 10000) % 255;
    for (uint32_t x = 0; x < tex_w; x++) {
        for (uint32_t y = 0; y < tex_h; y++) {
            
            uint32_t i = ((y * tex_w)+x)*4;
            
            rgba[i+0] = 255;
            rgba[i+1] = (x + bonus) % 255;
            rgba[i+2] = (y + bonus) % 255;
            rgba[i+3] = 255;
        }
    }
    
    quad_texs[1] = T1_tex_array_get_filename_loc(
        "structuredart_remixed");
    
    if (quad_texs[1].array_i < 0) {
        quad_texs[1] = T1_tex_array_reg_img(
            "structuredart_remixed",
            tex_w,
            tex_h,
            false,
            false);
    }
    
    T1_tex_array_update_rgba(
        quad_texs[1].array_i,
        quad_texs[1].slice_i,
        rgba,
        rgba_size);
    
    T1_platform_gpu_push_tex_slice_and_free_rgba(
        quad_texs[1].array_i,
        quad_texs[1].slice_i);
    
    float min_screen_dim = T1_render_views->cpu[0].width > T1_render_views->cpu[0].height ?
        T1_render_views->cpu[0].height :
        T1_render_views->cpu[0].width;
    
    float whitespace = min_screen_dim / 20.0f;
    
    float quad_size = (min_screen_dim - (whitespace * 3.0f)) / 2.0f;
    
    float quad_x[2];
    quad_x[0] = whitespace + quad_size*0.5f;
    quad_x[1] = whitespace*2.0f + quad_size*1.5f;
    
    for (uint32_t i = 0; i < 2; i++) {
        T1FlatTexQuadRequest quad;
        T1_texquad_fetch_next(&quad);
        quad.gpu->f32.xyz[0] =
            T1_render_view_screen_x_to_x_noz(quad_x[i]);
        quad.gpu->f32.xyz[1] =
            T1_render_view_screen_y_to_y_noz(whitespace + quad_size*0.5f);
        quad.gpu->f32.xyz[2] = 0.5f;
        quad.gpu->f32.size_xy[0] =
            T1_render_view_screen_width_to_width_noz(quad_size);
        quad.gpu->f32.size_xy[1] =
            T1_render_view_screen_height_to_height_noz(quad_size);
        quad.gpu->i32.tex_array_i = quad_texs[i].array_i;
        quad.gpu->i32.tex_slice_i = quad_texs[i].slice_i;
        quad.gpu->f32.rgba[0] = 1.0f;
        quad.gpu->f32.rgba[1] = 1.0f;
        quad.gpu->f32.rgba[2] = 1.0f;
        quad.gpu->f32.rgba[3] = 1.0f;
        quad.gpu->f32.offset_xy[0] = 0.0f;
        quad.gpu->f32.offset_xy[1] = 0.0f;
        
        quad.cpu->zsprite_id = -1;
        quad.gpu->i32.touch_id = -1;
        quad.cpu->visible = 1;
        quad.cpu->one_frame_only = true;
        
        T1_texquad_commit(&quad);
    }
    #endif
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
    
    if (T1_io_keymap[T1_IO_KEY_S] == true)
    {
        T1_io_keymap[T1_IO_KEY_S] = false;
        
        #if TEAPOT
        #if T1_ZSPRITE_ANIM_ACTIVE == T1_ACTIVE
        T1_zsprite_anim_shatter_and_destroy(
            /* const int32_t object_id: */
                teapot_object_ids[1],
            /* const uint64_t duration_microseconds: */
                750000);
        #elif T1_SCHEDULED_ANIMS_ACTIVE == T1_INACTIVE
        #else
        #error
        #endif
        #endif
    }
    
    if (T1_io_keymap[T1_IO_KEY_P] == true)
    {
        T1_zsprite_list->cpu_data[0].simd_stats.xyz[2] += 0.001f;
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
    
    #if TEAPOT
    if (T1_io_keymap[T1_IO_KEY_M] == true) {
        T1_io_keymap[T1_IO_KEY_M] = false;
        
        testswitch = !testswitch;
        T1zSpriteAnim * anim =
            T1_zsprite_anim_request_next(true);
        anim->cpu_vals.xyz[0] =
            testswitch ? 1.25f : -1.25f;
        anim->cpu_vals.angle_xyz[2] =
            testswitch ? 1.80f : 0.00f;
        anim->gpu_vals.i32.touch_id =
            testswitch ? 12542209 : teapot_touch_ids[0];
        anim->affected_zsprite_id =
            teapot_object_ids[0];
        anim->easing_type = EASINGTYPE_NONE;
        anim->duration_us = 1;
        anim->cpu_vals_active = true;
        anim->gpu_vals_i32_active = true;
        T1_zsprite_anim_commit_and_instarun(anim);
    }
    #else
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
    #endif
    
    #if TEAPOT
    if (T1_io_keymap[T1_IO_KEY_T] == true) {
        T1_io_keymap[T1_IO_KEY_T] = false;
        
        if (testswitch) {
            #if T1_ZSPRITE_ANIM_ACTIVE == T1_ACTIVE
            T1_zsprite_anim_evaporate_and_destroy(
                teapot_object_ids[0],
                900000);
            #elif T1_ZSPRITE_ANIM_ACTIVE == T1_INACTIVE
            #else
            #error
            #endif
        } else {
            request_teapots();
        }
        testswitch = !testswitch;
    }
    #endif
    
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
    redraw_quads();
    
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
    
    if (T1_io_keymap[T1_IO_KEY_R]) {
        for (uint32_t i = 0; i < T1_zsprite_list->size; i++) {
            if (T1_zsprite_list->cpu_data[i].zsprite_id == 20)
            {
                T1_zsprite_list->cpu_data[i].simd_stats.
                    angle_xyz[0] += 0.014f;
                T1_zsprite_list->cpu_data[i].simd_stats.
                    angle_xyz[1] += 0.01f;
                T1_zsprite_list->cpu_data[i].simd_stats.
                    angle_xyz[2] += 0.003f;
            }
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
    
    #if TEAPOT
    for (uint32_t i = 0; i < 2; i++) {
    if (
        !T1_io_events[T1_IO_LAST_LCLICK_START].
            handled &&
        T1_io_events[T1_IO_LAST_LCLICK_START].
            touch_id_top == teapot_touch_ids[i])
    {
        T1_io_events[T1_IO_LAST_LCLICK_START].handled = true;
        
        #if T1_ZSPRITE_ANIM_ACTIVE == T1_ACTIVE
        T1_zsprite_anim_bump(
            teapot_object_ids[0],
            0.0f);
        #elif T1_ZSPRITE_ANIM_ACTIVE == T1_INACTIVE
        #else
        #error
        #endif
    }
    }
    
    if (T1_io_keymap[T1_IO_KEY_R]) {
        for (uint32_t i = 0; i < T1_zsprite_list->size; i++) {
            if (T1_zsprite_list->cpu_data[i].zsprite_id ==
                teapot_object_ids[0])
            {
                T1_zsprite_list->cpu_data[i].simd_stats.
                    angle_xyz[1] += 0.01f;
            }
        }
    }
    #endif
    
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
}

void T1_clientlogic_shutdown(void) {
    // You're notified that your application is about to shut down
}
