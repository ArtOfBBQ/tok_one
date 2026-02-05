#include "T1_clientlogic.h"

#define TEAPOT 1
#if TEAPOT
static int32_t teapot_mesh_id = -1;
static int32_t teapot_object_ids[2];
static int32_t teapot_touch_ids[2];
#endif

void T1_clientlogic_init(void) {
    return;
}

void T1_clientlogic_early_startup(
    bool32_t * success,
    char * error_message)
{
    uint32_t good;
    T1_texture_files_preregister_png_resource("1001_normal.png", &good);
    T1_texture_files_preregister_png_resource("1001_albedo.png", &good);
    T1_texture_files_preregister_png_resource("blob1.png", &good);
    T1_texture_files_preregister_png_resource("blob2.png", &good);
    T1_texture_files_preregister_png_resource("blob3.png", &good);
    T1_texture_files_preregister_png_resource("phoebus.png", &good);
    T1_texture_files_preregister_png_resource("structuredart1.png", &good);
    T1_texture_files_preregister_png_resource("structuredart2.png", &good);
    T1_texture_files_preregister_png_resource("normalmap_rectangles.png", &good);
    
    assert(good);
    
    #if TEAPOT
    char err_msg[128];
    #if 0
    teapot_mesh_id = T1_objmodel_new_mesh_id_from_resources(
        "crack_1.obj",
        "crack_1.mtl",
        false,
        true,
        &good,
        err_msg);
    #else
    teapot_mesh_id = T1_objmodel_new_mesh_id_from_resources(
        "guitar_simplified.obj",
        "guitar_simplified.mtl",
        /* flip_uv_u: */ false,
        /* flip_uv_v: */ true,
        &good,
        err_msg);
    #endif
    
    if (teapot_mesh_id < 0) {
        #ifndef LOGGER_IGNORE_ASSERTS
        log_dump_and_crash(
            "Failed to register the "
            "guitar_simplified model, maybe the .mtl"
            " or .obj file is missing?");
        #endif
        return;
    }
    #endif
    
    *success = true;
}

static void request_teapots(void) {
    
    #define TEAPOT_X  0.0f
    #define TEAPOT_Y  0.00f
    #define TEAPOT_Z  2.0f
    
    #if TEAPOT
    if (teapot_mesh_id < 0) { return; }
    
    T1_zsprite_delete(teapot_object_ids[0]);
    T1_zsprite_delete(teapot_object_ids[1]);
    
    teapot_object_ids[0] = T1_zspriteid_next_nonui_id();
    teapot_object_ids[1] = T1_zspriteid_next_nonui_id();
    
    for (uint32_t i = 0; i < 1; i++) {
        log_assert(teapot_mesh_id >= 0);
        T1zSpriteRequest teapot_request;
        T1_zsprite_fetch_next(&teapot_request);
        T1_zsprite_construct(&teapot_request);
        teapot_request.cpu_data->mesh_id =
            teapot_mesh_id;
        teapot_request.cpu_data->simd_stats.mul_xyz[0] =
            0.15f;
        teapot_request.cpu_data->simd_stats.mul_xyz[1] =
            0.15f;
        teapot_request.cpu_data->simd_stats.mul_xyz[2] =
            0.15f;
        teapot_request.cpu_data->simd_stats.xyz[0] =
            TEAPOT_X + (i * 0.20f);
        teapot_request.cpu_data->simd_stats.xyz[1] =
            TEAPOT_Y - (i * 1.0f);
        teapot_request.cpu_data->simd_stats.xyz[2] =
            TEAPOT_Z - (i * 0.25f);
        teapot_request.cpu_data->simd_stats.
            angle_xyz[0] = 0.00f;
        teapot_request.cpu_data->simd_stats.
            angle_xyz[1] = 3.2f;
        teapot_request.cpu_data->simd_stats.
            angle_xyz[2] = 0.0f;
        teapot_request.cpu_data->zsprite_id =
            teapot_object_ids[i];
        teapot_request.cpu_data->visible = true;
        teapot_touch_ids[i] =
            T1_zspriteid_next_nonui_id();
        teapot_request.gpu_data->i32.touch_id =
            teapot_touch_ids[i];
        teapot_request.gpu_data->f32.
            ignore_lighting = 0.0f;
        teapot_request.gpu_data->f32.
            ignore_camera =  0.0f;
        T1_zsprite_commit(&teapot_request);
    }
    #endif
    
    font_settings->font_height = 75;
    font_settings->i32.touch_id = -1;
    font_settings->f32.rgba[0] = 0.7f;
    font_settings->f32.rgba[1] = 1.0f;
    font_settings->f32.rgba[2] = 1.0f;
    font_settings->f32.rgba[3] = 1.0f;
    text_request_label_renderable(
        /* int32_t with_object_id: */
            21,
        /* char * text_to_draw: */
            "Welcome",
        /* float left_pixelspace: */
            100.0f,
        /* float top_pixelspace: */
            100.0f,
        /* float z: */
            0.75f,
        /* float max_width: */
            1500.0f);
    font_settings->i32.touch_id = -1;
}

void T1_clientlogic_late_startup(void) {
    
    float teapot_xyz[3];
    teapot_xyz[0] = TEAPOT_X;
    teapot_xyz[1] = TEAPOT_Y;
    teapot_xyz[2] = TEAPOT_Z;
    
    T1zLight * light = T1_zlight_next();
    light->RGBA[0]       =  1.0f;
    light->RGBA[1]       =  1.0f;
    light->RGBA[2]       =  1.0f;
    light->RGBA[3]       =  1.0f;
    light->diffuse       =  1.0f;
    light->specular      =  1.0f;
    light->reach         =  100.0f;
    light->xyz[0]        =  TEAPOT_X - 1.20f;
    light->xyz[1]        =  TEAPOT_Y + 0.50f;
    light->xyz[2]        =  TEAPOT_Z;
    T1_zlight_point_light_to_location(
        light->xyz_angle,
        light->xyz,
        teapot_xyz);
    T1_zlight_commit(light);
    
    T1zSpriteRequest lightcube_request;
    T1_zsprite_fetch_next(&lightcube_request);
    T1_zsprite_construct(&lightcube_request);
    lightcube_request.cpu_data->mesh_id = T1_BASIC_CUBE_MESH_ID;
    lightcube_request.cpu_data->simd_stats.mul_xyz[0] = 0.05f;
    lightcube_request.cpu_data->simd_stats.mul_xyz[1] = 0.05f;
    lightcube_request.cpu_data->simd_stats.mul_xyz[2] = 0.05f;
    lightcube_request.cpu_data->simd_stats.xyz[0] = light->xyz[0];
    lightcube_request.cpu_data->simd_stats.xyz[1] = light->xyz[1];
    lightcube_request.cpu_data->simd_stats.xyz[2] = light->xyz[2];
    lightcube_request.cpu_data->simd_stats.angle_xyz[0] = 0.0f;
    lightcube_request.cpu_data->simd_stats.angle_xyz[1] = 0.0f;
    lightcube_request.cpu_data->simd_stats.angle_xyz[2] = 0.0f;
    lightcube_request.cpu_data->zsprite_id = light->object_id;
    lightcube_request.cpu_data->visible = true;
    lightcube_request.gpu_data->f32.
        ignore_lighting = 1.0f;
    lightcube_request.gpu_data->f32.
        ignore_camera = 0.0f;
    lightcube_request.gpu_data->f32.alpha = 1.0f;
    lightcube_request.gpu_data->f32.base_mat_f32.diffuse_rgb[0] = light->RGBA[0] * 2.15f;
    lightcube_request.gpu_data->f32.base_mat_f32.diffuse_rgb[1] = light->RGBA[1] * 2.15f;
    lightcube_request.gpu_data->f32.base_mat_f32.diffuse_rgb[2] = light->RGBA[2] * 2.15f;
    lightcube_request.cpu_data->simd_stats.bloom_on = true;
    lightcube_request.gpu_data->f32.shadow_strength = 0.0f;
    T1_zsprite_commit(&lightcube_request);
    
    T1_camera->xyz[0] =  0.00f;
    T1_camera->xyz[1] =  0.00f;
    T1_camera->xyz[2] = -0.50f;
    T1_camera->xyz_angle[0] =  0.0f;
    T1_camera->xyz_angle[1] =  0.0f;
    T1_camera->xyz_angle[2] =  0.0f;
    
    request_teapots();
    
    T1Tex quad_tex =
        T1_texture_array_get_filename_location(
            "structuredart1.png");
    
    
    T1zSpriteRequest quad;
    T1_zsprite_fetch_next(&quad);
    T1_zsprite_construct_quad(
        /* const float left_x: */
            TEAPOT_X + 0.50f,
        /* const float bottom_y: */
            TEAPOT_Y - 0.75f,
        /* const float z: */
            TEAPOT_Z + 0.2f,
        /* const float width: */
            T1_render_view_screen_width_to_width(
                T1_global->window_width * 2, 1.0f),
        /* const float height: */
            T1_render_view_screen_height_to_height(
                T1_global->window_height * 2, 1.0f),
        /* PolygonRequest * stack_recipient: */
            &quad);
    quad.gpu_data->i32.base_mat_i32.texturearray_i = quad_tex.array_i;
    quad.gpu_data->i32.base_mat_i32.texture_i = quad_tex.slice_i;
    quad.cpu_data->zsprite_id = -1;
    quad.gpu_data->i32.touch_id = -1;
    
    quad.cpu_data->simd_stats.alpha_blending_on = 1.0f;
    quad.cpu_data->simd_stats.mul_xyz[0] = 0.0f;
    quad.cpu_data->simd_stats.mul_xyz[1] = 0.0f;
    quad.cpu_data->simd_stats.mul_xyz[2] = 0.0f;
    quad.cpu_data->simd_stats.scale_factor = 1.0f;
    quad.cpu_data->simd_stats.angle_xyz[0] = 1.8f;
    quad.cpu_data->simd_stats.angle_xyz[1] = 0.0f;
    quad.cpu_data->simd_stats.angle_xyz[2] = 0.65f;
    quad.gpu_data->f32.ignore_camera = 0.0f;
    quad.gpu_data->f32.ignore_lighting = 0.0f;
    
    quad.gpu_data->f32.base_mat_f32.ambient_rgb[0] = 0.05f;
    quad.gpu_data->f32.base_mat_f32.ambient_rgb[1] = 0.05f;
    quad.gpu_data->f32.base_mat_f32.ambient_rgb[2] = 0.50f;
    quad.gpu_data->f32.alpha = 1.0f;
    
    T1_zsprite_commit(&quad);
    
    for (uint32_t i = 0; i < 3; i++) {
        T1_zsprite_fetch_next(&quad);
        T1_zsprite_construct_quad(
            /* const float left_x: */
                TEAPOT_X - 1.0f,
            /* const float bottom_y: */
                TEAPOT_Y - 0.50f,
            /* const float z: */
                TEAPOT_Z + 0.2f + (i * 0.75f),
            /* const float width: */
                T1_render_view_screen_width_to_width(
                    T1_global->window_width * 2, 1.0f),
            /* const float height: */
                T1_render_view_screen_height_to_height(
                    T1_global->window_height * 2, 1.0f),
            /* PolygonRequest * stack_recipient: */
                &quad);
        
        T1Tex phoebus_tex = T1_texture_array_get_filename_location(
            /* const char * for_filename: */
                "phoebus.png");
        quad.gpu_data->i32.base_mat_i32.texturearray_i = phoebus_tex.array_i;
        quad.gpu_data->i32.base_mat_i32.texture_i = phoebus_tex.slice_i;
        quad.cpu_data->zsprite_id = -1;
        quad.gpu_data->i32.touch_id = -1;
        quad.cpu_data->simd_stats.alpha_blending_on = true;
        
        quad.cpu_data->simd_stats.mul_xyz[0] = 0.0f;
        quad.cpu_data->simd_stats.mul_xyz[1] = 0.0f;
        quad.cpu_data->simd_stats.mul_xyz[2] = 0.0f;
        quad.cpu_data->simd_stats.scale_factor = 1.0f;
        quad.cpu_data->simd_stats.angle_xyz[0] = 1.8f;
        quad.cpu_data->simd_stats.angle_xyz[1] = 0.0f;
        quad.cpu_data->simd_stats.angle_xyz[2] = 0.65f;
        quad.gpu_data->f32.ignore_camera = 0.0f;
        quad.gpu_data->f32.ignore_lighting = 0.0f;
        
        quad.gpu_data->f32.base_mat_f32.ambient_rgb[0] = 0.05f;
        quad.gpu_data->f32.base_mat_f32.ambient_rgb[1] = 0.05f;
        quad.gpu_data->f32.base_mat_f32.ambient_rgb[2] = 0.50f;
        quad.gpu_data->f32.alpha = 0.25f;
        
        T1_zsprite_commit(&quad);
    }
}

void T1_clientlogic_threadmain(int32_t threadmain_id) {
    switch (threadmain_id) {
        default:
            log_append("unhandled threadmain_id: ");
            log_append_int(threadmain_id);
            log_append("\n");
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
    T1_texture_array_delete_slice(
        T1_render_views->cpu[0].write_array_i,
        T1_render_views->cpu[0].write_slice_i);
    T1_render_views->cpu[0].write_array_i = -1;
    T1_render_views->cpu[0].write_slice_i = -1;
    
    T1_render_view_delete(0);
    
    int32_t rv_i =
        T1_texture_array_create_new_render_view(
            new_width,
            new_height);
    
    T1_render_views->cpu[rv_i].passes_size = 6;
    log_assert(
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
    
    // You're notified that the window is resized!
    request_teapots();
}

void T1_clientlogic_shutdown(void) {
    // You're notified that your application is about to shut down
}
