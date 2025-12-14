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
    #if 1
    teapot_mesh_id = T1_objmodel_new_mesh_id_from_resources(
        "crack_1.obj",
        "crack_1.mtl",
        false,
        true);
    #else
    teapot_mesh_id = T1_objmodel_new_mesh_id_from_resources(
        "guitar_simplified.obj",
        "guitar_simplified.mtl",
        /* flip_uv_u: */ false,
        /* flip_uv_v: */ true);
    #endif
    
    if (teapot_mesh_id < 0) {
        #ifndef LOGGER_IGNORE_ASSERTS
        log_dump_and_crash(
            "Failed to register the guitar_simplified model, "
            " maybe the .mtl or .obj file is missing?");
        #endif
        return;
    }
    #endif
    
    *success = true;
}

static void request_teapots(void) {
        
    #define TEAPOT_X  -5.0f
    #define TEAPOT_Y  10.0f
    #define TEAPOT_Z  1.0f
    
    #if TEAPOT
    teapot_object_ids[0] = T1_zspriteid_next_nonui_id();
    teapot_object_ids[1] = T1_zspriteid_next_nonui_id();
    
    for (uint32_t i = 0; i < 1; i++) {
        log_assert(teapot_mesh_id >= 0);
        T1zSpriteRequest teapot_request;
        T1_zsprite_request_next(&teapot_request);
        T1_zsprite_construct(&teapot_request);
        teapot_request.cpu_data->mesh_id = teapot_mesh_id;
        teapot_request.cpu_data->simd_stats.mul_xyz[0] = 0.15f;
        teapot_request.cpu_data->simd_stats.mul_xyz[1] = 0.15f;
        teapot_request.cpu_data->simd_stats.mul_xyz[2] = 0.15f;
        teapot_request.cpu_data->simd_stats.xyz[0] = TEAPOT_X + (i * 0.20f);
        teapot_request.cpu_data->simd_stats.xyz[1] = TEAPOT_Y - (i * 1.0f);
        teapot_request.cpu_data->simd_stats.xyz[2] = TEAPOT_Z - (i * 0.25f);
        teapot_request.cpu_data->simd_stats.angle_xyz[0] = 0.00f;
        teapot_request.cpu_data->simd_stats.angle_xyz[1] = 3.2f;
        teapot_request.cpu_data->simd_stats.angle_xyz[2] = 0.0f;
        teapot_request.cpu_data->zsprite_id = teapot_object_ids[i];
        teapot_request.cpu_data->visible = true;
        teapot_touch_ids[i] = T1_zspriteid_next_nonui_id();
        teapot_request.gpu_data->touch_id = teapot_touch_ids[i];
        teapot_request.gpu_data->ignore_lighting =  0.0f;
        teapot_request.cpu_data->simd_stats.ignore_camera =  0.0f;
        T1_zsprite_commit(&teapot_request);
    }
    #endif
}

void T1_clientlogic_late_startup(void) {
    
    float teapot_xyz[3];
    teapot_xyz[0] = TEAPOT_X;
    teapot_xyz[1] = TEAPOT_Y;
    teapot_xyz[2] = TEAPOT_Z;
    
    shadowcaster_light_i = 0;
    zLightSource * light = next_zlight();
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
    zlight_point_light_to_location(
        light->xyz_angle,
        light->xyz,
        teapot_xyz);
    commit_zlight(light);
    
    T1zSpriteRequest lightcube_request;
    T1_zsprite_request_next(&lightcube_request);
    T1_zsprite_construct(&lightcube_request);
    lightcube_request.cpu_data->mesh_id = BASIC_CUBE_MESH_ID;
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
    lightcube_request.gpu_data->ignore_lighting = 1.0f;
    lightcube_request.cpu_data->simd_stats.
        ignore_camera = 0.0f;
    lightcube_request.gpu_data->alpha = 1.0f;
    lightcube_request.gpu_data->base_mat.diffuse_rgb[0] = light->RGBA[0] * 2.15f;
    lightcube_request.gpu_data->base_mat.diffuse_rgb[1] = light->RGBA[1] * 2.15f;
    lightcube_request.gpu_data->base_mat.diffuse_rgb[2] = light->RGBA[2] * 2.15f;
    lightcube_request.gpu_data->base_mat.rgb_cap[0] = 5.0f;
    lightcube_request.gpu_data->base_mat.rgb_cap[1] = 5.0f;
    lightcube_request.gpu_data->base_mat.rgb_cap[2] = 5.0f;
    lightcube_request.gpu_data->remove_shadow = true;
    T1_zsprite_commit(&lightcube_request);
    
    camera.xyz[0] = -4.50f;
    camera.xyz[1] =  9.50f;
    camera.xyz[2] = -0.5f;
    camera.xyz_angle[0] =  0.1f;
    camera.xyz_angle[1] =  0.2f;
    camera.xyz_angle[2] =  0.0f;
    
    request_teapots();
    
    int32_t quad_texture_array_i = -1;
    int32_t quad_texture_i = -1;
    //    T1_texture_array_get_filename_location(
    //        "structuredart1.png",
    //        &quad_texture_array_i,
    //        &quad_texture_i);
    
    T1zSpriteRequest quad;
    T1_zsprite_request_next(&quad);
    zsprite_construct_quad(
        /* const float left_x: */
            TEAPOT_X + 0.75f,
        /* const float bottom_y: */
            TEAPOT_Y - 1.25f,
        /* const float z: */
            TEAPOT_Z + 0.2f,
        /* const float width: */
            T1_engineglobals_screenspace_width_to_width(
                T1_engine_globals->window_width * 2, 1.0f),
        /* const float height: */
            T1_engineglobals_screenspace_height_to_height(
                T1_engine_globals->window_height * 2, 1.0f),
        /* PolygonRequest * stack_recipient: */
            &quad);
    quad.gpu_data->base_mat.texturearray_i = quad_texture_array_i;
    quad.gpu_data->base_mat.texture_i = quad_texture_i;
    quad.cpu_data->zsprite_id = -1;
    quad.gpu_data->touch_id = -1;
    quad.cpu_data->alpha_blending_on       = false;
    
    quad.cpu_data->simd_stats.mul_xyz[0]    = 0.0f;
    quad.cpu_data->simd_stats.mul_xyz[1]    = 0.0f;
    quad.cpu_data->simd_stats.mul_xyz[2]    = 0.0f;
    quad.cpu_data->simd_stats.scale_factor  = 1.0f;
    quad.cpu_data->simd_stats.angle_xyz[0]  = 1.8f;
    quad.cpu_data->simd_stats.angle_xyz[1]  = 0.0f;
    quad.cpu_data->simd_stats.angle_xyz[2]  = 0.65f;
    quad.cpu_data->simd_stats.ignore_camera = 0.0f;
    quad.gpu_data->ignore_lighting          = 0.0f;
    
    quad.gpu_data->base_mat.ambient_rgb[0]  = 0.05f;
    quad.gpu_data->base_mat.ambient_rgb[1]  = 0.05f;
    quad.gpu_data->base_mat.ambient_rgb[2]  = 0.50f;
    quad.gpu_data->alpha = 1.0f;
    
    T1_zsprite_commit(&quad);
    
    font_settings->font_height = 50;
    font_settings->touch_id = -1;
    font_settings->mat.ambient_rgb[0] =  0.1f;
    font_settings->mat.ambient_rgb[1] =  0.1f;
    font_settings->mat.ambient_rgb[2] =  0.1f;
    font_settings->mat.diffuse_rgb[0] =  2.2f;
    font_settings->mat.diffuse_rgb[1] =  2.9f;
    font_settings->mat.diffuse_rgb[2] =  0.8f;
    font_settings->mat.alpha =  1.0f;
    font_settings->alpha = 1.0f;
    font_settings->ignore_camera = false;
    font_settings->alpha_blending_on = false;
    font_settings->ignore_lighting = 1.0f;
    font_settings->mat.rgb_cap[0] = 5.0f;
    font_settings->mat.rgb_cap[1] = 5.0f;
    font_settings->mat.rgb_cap[2] = 5.0f;
    text_request_label_renderable(
        /* const int32_t with_object_id: */
            21,
        /* const char * text_to_draw: */
            "Welcome",
        /* const float left_pixelspace: */
            250.0f,
        /* const float top_pixelspace: */
            300.0f,
        /* const float z: */
            3.5f,
        /* const float max_width: */
            1500.0f);
    font_settings->touch_id = -1;
    log_assert(
        T1_zsprite_list->cpu_data[T1_zsprite_list->size-1].
            zsprite_id == 21);
    
    for (uint32_t i = 0; i < 3; i++) {
        T1_zsprite_request_next(&quad);
        zsprite_construct_quad(
            /* const float left_x: */
                TEAPOT_X - 3.0f,
            /* const float bottom_y: */
                TEAPOT_Y - 1.25f,
            /* const float z: */
                TEAPOT_Z + 0.2f + (i * 0.75f),
            /* const float width: */
                T1_engineglobals_screenspace_width_to_width(
                    T1_engine_globals->window_width * 2, 1.0f),
            /* const float height: */
                T1_engineglobals_screenspace_height_to_height(
                    T1_engine_globals->window_height * 2, 1.0f),
            /* PolygonRequest * stack_recipient: */
                &quad);
        
        T1Tex phoebus_tex = T1_texture_array_get_filename_location(
            /* const char * for_filename: */
                "phoebus.png");
        quad.gpu_data->base_mat.texturearray_i = phoebus_tex.array_i;
        quad.gpu_data->base_mat.texture_i = phoebus_tex.slice_i;
        quad.cpu_data->zsprite_id = -1;
        quad.gpu_data->touch_id = -1;
        quad.cpu_data->alpha_blending_on = true;
        
        quad.cpu_data->simd_stats.mul_xyz[0] = 0.0f;
        quad.cpu_data->simd_stats.mul_xyz[1] = 0.0f;
        quad.cpu_data->simd_stats.mul_xyz[2] = 0.0f;
        quad.cpu_data->simd_stats.scale_factor = 1.0f;
        quad.cpu_data->simd_stats.angle_xyz[0] = 1.8f;
        quad.cpu_data->simd_stats.angle_xyz[1] = 0.0f;
        quad.cpu_data->simd_stats.angle_xyz[2] = 0.65f;
        quad.cpu_data->simd_stats.ignore_camera = 0.0f;
        quad.gpu_data->ignore_lighting         = 0.0f;
        
        quad.gpu_data->base_mat.ambient_rgb[0] = 0.05f;
        quad.gpu_data->base_mat.ambient_rgb[1] = 0.05f;
        quad.gpu_data->base_mat.ambient_rgb[2] = 0.50f;
        quad.gpu_data->alpha = 0.25f;
        
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
        camera.xyz[0] -= cam_speed;
    }
    
    if (T1_io_keymap[T1_IO_KEY_RIGHTARROW] == true)
    {
        camera.xyz[0] += cam_speed;
    }
    
    if (T1_io_keymap[T1_IO_KEY_DOWNARROW] == true)
    {
        camera.xyz[1] -= cam_speed;
    }
    
    if (T1_io_keymap[T1_IO_KEY_UPARROW] == true)
    {
        camera.xyz[1] += cam_speed;
    }
    
    if (T1_io_keymap[T1_IO_KEY_A] == true) {
        camera.xyz_angle[0] += cam_rotation_speed;
    }
    
    if (T1_io_keymap[T1_IO_KEY_Z] == true) {
        camera.xyz_angle[2] -= cam_rotation_speed;
    }
    
    if (T1_io_keymap[T1_IO_KEY_X] == true) {
        camera.xyz_angle[2] += cam_rotation_speed;
    }
    
    if (T1_io_keymap[T1_IO_KEY_Q] == true) {
        camera.xyz_angle[0] -= cam_rotation_speed;
    }
    
    if (T1_io_keymap[T1_IO_KEY_W] == true) {
        camera.xyz_angle[1] -= cam_rotation_speed;
    }
    
    if (T1_io_keymap[T1_IO_KEY_S] == true) {
        camera.xyz_angle[1] += cam_rotation_speed;
    }
    
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
    
    if (T1_io_keymap[T1_IO_KEY_BACKSLASH] == true) {
        // / key
        camera.xyz[2] -= 0.01f;
    }
    
    if (T1_io_keymap[T1_IO_KEY_FULLSTOP] == true) {
        camera.xyz[2] += 0.01f;
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
        !T1_io_events[T1_IO_LAST_LCLICK_START].handled &&
        T1_io_events[T1_IO_LAST_LCLICK_START].touch_id_top ==
            teapot_touch_ids[i])
    {
        T1_io_events[T1_IO_LAST_LCLICK_START].handled = true;
        
        #if T1_ZSPRITE_ANIM_ACTIVE == T1_ACTIVE
        T1_zsprite_anim_bump(teapot_object_ids[0], 0.0f);
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
        camera.xyz[0] = -6.09f;
        camera.xyz[1] = 11.547f;
        camera.xyz[2] = 0.0999f;
        camera.xyz_angle[0] = 1.053f;
        camera.xyz_angle[1] = 0.20f;
        camera.xyz_angle[2] = -102.92f;
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
    const uint32_t new_height,
    const uint32_t new_width)
{
    // You're notified that the window is resized!
}

void T1_clientlogic_shutdown(void) {
    // You're notified that your application is about to shut down
}
