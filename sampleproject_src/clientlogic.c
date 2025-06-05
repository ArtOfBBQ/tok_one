#include "T1_clientlogic.h"

#define TEAPOT 1
#if TEAPOT
static int32_t teapot_mesh_id = -1;
static int32_t teapot_object_ids[2];
static int32_t teapot_touchable_ids[2];
#endif

void client_logic_init(void) {
    
}

void client_logic_early_startup(
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
    
    assert(good);
    
    #if TEAPOT
    teapot_mesh_id = T1_objmodel_new_mesh_id_from_resources(
        "guitar_simplified.obj",
        "guitar_simplified.mtl");
    if (teapot_mesh_id < 0) {
        log_dump_and_crash(
            "Failed to register the guitar_simplified model, "
            " maybe the .mtl or .obj file is missing?");
        return;
    }
    #endif
}

void client_logic_late_startup(void) {
    
    #define TEAPOT_X  0.2f
    #define TEAPOT_Y  0.1f
    #define TEAPOT_Z  0.32f
    float teapot_xyz[3];
    teapot_xyz[0] = TEAPOT_X;
    teapot_xyz[1] = TEAPOT_Y;
    teapot_xyz[2] = TEAPOT_Z;
    
    shadowcaster_light_i = 0;
    zLightSource * light = next_zlight();
    light->RGBA[0]       =  1.0f;
    light->RGBA[1]       =  1.0f;
    light->RGBA[2]       =  1.0f;
    light->RGBA[3]       =  0.30f;
    light->reach         =  100.0f;
    light->xyz[0]        =  TEAPOT_X - 1.20f;
    light->xyz[1]        =  TEAPOT_Y + 0.50f;
    light->xyz[2]        =  TEAPOT_Z - 0.15f;
    zlight_point_light_to_location(
        light->xyz_angle,
        light->xyz,
        teapot_xyz);
    commit_zlight(light);
    
    zSpriteRequest lightcube_request;
    zsprite_request_next(&lightcube_request);
    zsprite_construct(&lightcube_request);
    lightcube_request.cpu_data->mesh_id = BASIC_CUBE_MESH_ID;
    lightcube_request.gpu_data->xyz_multiplier[0] = 0.05f;
    lightcube_request.gpu_data->xyz_multiplier[1] = 0.05f;
    lightcube_request.gpu_data->xyz_multiplier[2] = 0.05f;
    lightcube_request.gpu_data->xyz[0]            = light->xyz[0];
    lightcube_request.gpu_data->xyz[1]            = light->xyz[1];
    lightcube_request.gpu_data->xyz[2]            = light->xyz[2];
    lightcube_request.gpu_data->xyz_angle[0]      = 0.0f;
    lightcube_request.gpu_data->xyz_angle[1]      = 0.0f;
    lightcube_request.gpu_data->xyz_angle[2]      = 0.0f;
    lightcube_request.cpu_data->zsprite_id        = light->object_id;
    lightcube_request.cpu_data->visible           = true;
    lightcube_request.gpu_data->ignore_lighting   = 1.0f;
    lightcube_request.gpu_data->ignore_camera     = 0.0f;
    lightcube_request.gpu_data->base_material.alpha = 1.0f;
    lightcube_request.gpu_data->base_material.diffuse_rgb[0] =
        light->RGBA[0] * 2.5f;
    lightcube_request.gpu_data->base_material.diffuse_rgb[1] =
        light->RGBA[1] * 2.5f;
    lightcube_request.gpu_data->base_material.diffuse_rgb[2] =
        light->RGBA[2] * 2.5f;
    lightcube_request.gpu_data->base_material.rgb_cap[0] = 5.0f;
    lightcube_request.gpu_data->base_material.rgb_cap[1] = 5.0f;
    lightcube_request.gpu_data->base_material.rgb_cap[2] = 5.0f;
    lightcube_request.gpu_data->remove_shadow = true;
    zsprite_commit(&lightcube_request);
    
    camera.xyz[0] =  0.5f;
    camera.xyz[1] =  1.6f;
    camera.xyz[2] = -1.3f;
    camera.xyz_angle[0] =  0.7f;
    camera.xyz_angle[1] =  0.0f;
    camera.xyz_angle[2] =  0.0f;
    
    #if TEAPOT
    teapot_object_ids[0] = next_nonui_object_id();
    teapot_object_ids[1] = next_nonui_object_id();
    
    for (uint32_t i = 0; i < 1; i++) {
        log_assert(teapot_mesh_id >= 0);
        zSpriteRequest teapot_request;
        zsprite_request_next(&teapot_request);
        zsprite_construct(&teapot_request);
        teapot_request.cpu_data->mesh_id = teapot_mesh_id;
        teapot_request.gpu_data->xyz_multiplier[0] = 0.15f;
        teapot_request.gpu_data->xyz_multiplier[1] = 0.15f;
        teapot_request.gpu_data->xyz_multiplier[2] = 0.15f;
        teapot_request.gpu_data->xyz[0]            = TEAPOT_X + (i * 0.20f);
        teapot_request.gpu_data->xyz[1]            = TEAPOT_Y - (i * 1.0f);
        teapot_request.gpu_data->xyz[2]            = TEAPOT_Z - (i * 0.25f);
        teapot_request.gpu_data->xyz_angle[0]      = 0.00f;
        teapot_request.gpu_data->xyz_angle[1]      = 3.2f;
        teapot_request.gpu_data->xyz_angle[2]      = 0.0f;
        teapot_request.cpu_data->zsprite_id         = teapot_object_ids[i];
        teapot_request.cpu_data->visible           = true;
        teapot_touchable_ids[i]                    = next_nonui_touchable_id();
        teapot_request.gpu_data->touchable_id      = teapot_touchable_ids[i];
        teapot_request.gpu_data->ignore_lighting =  0.0f;
        teapot_request.gpu_data->ignore_camera =  0.0f;
        
        log_assert(teapot_request.gpu_data->xyz_offset[0] == 0.0f);
        log_assert(teapot_request.gpu_data->xyz_offset[1] == 0.0f);
        log_assert(teapot_request.gpu_data->xyz_offset[2] == 0.0f);
        zsprite_commit(&teapot_request);
    }
    #endif
    
    int32_t quad_texture_array_i = -1;
    int32_t quad_texture_i = -1;
    //    T1_texture_array_get_filename_location(
    //        "structuredart1.png",
    //        &quad_texture_array_i,
    //        &quad_texture_i);
    
    zSpriteRequest quad;
    zsprite_request_next(&quad);
    zsprite_construct_quad(
        /* const float left_x: */
            TEAPOT_X + 0.75f,
        /* const float bottom_y: */
            TEAPOT_Y - 1.25f,
        /* const float z: */
            TEAPOT_Z + 0.2f,
        /* const float width: */
            engineglobals_screenspace_width_to_width(
                engine_globals->window_width * 2, 1.0f),
        /* const float height: */
            engineglobals_screenspace_height_to_height(
                engine_globals->window_height * 2, 1.0f),
        /* PolygonRequest * stack_recipient: */
            &quad);
    quad.gpu_data->base_material.texturearray_i = quad_texture_array_i;
    quad.gpu_data->base_material.texture_i      = quad_texture_i;
    quad.cpu_data->zsprite_id                   = -1;
    quad.gpu_data->touchable_id                 = -1;
    quad.cpu_data->alpha_blending_enabled       = false;
    
    quad.gpu_data->xyz_offset[0]          = 0.0f;
    quad.gpu_data->xyz_offset[1]          = 0.0f;
    quad.gpu_data->xyz_offset[2]          = 0.0f;
    quad.gpu_data->scale_factor           = 1.0f;
    quad.gpu_data->xyz_angle[0]           = 1.8f;
    quad.gpu_data->xyz_angle[1]           = 0.0f;
    quad.gpu_data->xyz_angle[2]           = 0.65f;
    quad.gpu_data->ignore_camera          = 0.0f;
    quad.gpu_data->ignore_lighting        = 0.0f;
    
    quad.gpu_data->base_material.ambient_rgb[0]  = 0.05f;
    quad.gpu_data->base_material.ambient_rgb[1]  = 0.05f;
    quad.gpu_data->base_material.ambient_rgb[2]  = 0.50f;
    quad.gpu_data->base_material.alpha           = 1.0f;
    
    zsprite_commit(&quad);
    
    font_settings->font_height = 50;
    font_settings->touchable_id = -1;
    font_settings->mat.ambient_rgb[0] =  0.1f;
    font_settings->mat.ambient_rgb[1] =  0.1f;
    font_settings->mat.ambient_rgb[2] =  0.1f;
    font_settings->mat.diffuse_rgb[0] =  2.2f;
    font_settings->mat.diffuse_rgb[1] =  2.9f;
    font_settings->mat.diffuse_rgb[2] =  0.8f;
    font_settings->mat.alpha =  1.0f;
    font_settings->ignore_camera = false;
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
    font_settings->touchable_id = -1;
    log_assert(
        zsprites_to_render->cpu_data[zsprites_to_render->size-1].
            zsprite_id == 21);
}

void client_logic_threadmain(int32_t threadmain_id) {
    switch (threadmain_id) {
        default:
            log_append("unhandled threadmain_id: ");
            log_append_int(threadmain_id);
            log_append("\n");
    }
}

void client_logic_animation_callback(
    const int32_t callback_id,
    const float arg_1,
    const float arg_2,
    const int32_t arg_3)
{
    #ifndef LOGGER_IGNORE_ASSERTS
    char unhandled_callback_id[256];
    common_strcpy_capped(
        unhandled_callback_id,
        256,
        "unhandled client_logic_animation_callback: ");
    common_strcat_int_capped(
        unhandled_callback_id,
        256,
        callback_id);
    common_strcat_capped(
        unhandled_callback_id,
        256,
        ". Find in clientlogic.c -> client_logic_animation_callback\n");
    log_append(unhandled_callback_id);
    log_dump_and_crash(unhandled_callback_id);
    #endif
}

static void client_handle_keypresses(
    uint64_t microseconds_elapsed)
{
    float elapsed_mod = (float)(
        (double)microseconds_elapsed / (double)16666);
    float cam_speed = 0.1f * elapsed_mod;
    float cam_rotation_speed = 0.05f * elapsed_mod;
    
    if (keypress_map[TOK_KEY_S] == true)
    {
        keypress_map[TOK_KEY_S] = false;
        
        #if TEAPOT
        #if SCHEDULED_ANIMS_ACTIVE
        scheduled_animations_request_shatter_and_destroy(
            /* const int32_t object_id: */
                teapot_object_ids[1],
            /* const uint64_t duration_microseconds: */
                750000);
        #endif
        #endif
    }
    
    if (keypress_map[TOK_KEY_LEFTARROW] == true)
    {
        camera.xyz[0] -= cam_speed;
    }
    
    if (keypress_map[TOK_KEY_RIGHTARROW] == true)
    {
        camera.xyz[0] += cam_speed;
    }
    
    if (keypress_map[TOK_KEY_DOWNARROW] == true)
    {
        camera.xyz[1] -= cam_speed;
    }
    
    if (keypress_map[TOK_KEY_UPARROW] == true)
    {
        camera.xyz[1] += cam_speed;
    }
    
    if (keypress_map[TOK_KEY_A] == true) {
        camera.xyz_angle[0] += cam_rotation_speed;
    }
    
    if (keypress_map[TOK_KEY_Z] == true) {
        camera.xyz_angle[2] -= cam_rotation_speed;
    }
    
    if (keypress_map[TOK_KEY_X] == true) {
        camera.xyz_angle[2] += cam_rotation_speed;
    }
    
    if (keypress_map[TOK_KEY_Q] == true) {
        camera.xyz_angle[0] -= cam_rotation_speed;
    }
    
    if (keypress_map[TOK_KEY_W] == true) {
        camera.xyz_angle[1] -= cam_rotation_speed;
    }
    
    if (keypress_map[TOK_KEY_S] == true) {
        camera.xyz_angle[1] += cam_rotation_speed;
    }
    
    if (keypress_map[TOK_KEY_BACKSLASH] == true) {
        // / key
        camera.xyz[2] -= 0.01f;
    }
    
    if (keypress_map[TOK_KEY_FULLSTOP] == true) {
        camera.xyz[2] += 0.01f;
    }
}

void client_logic_update(uint64_t microseconds_elapsed)
{
    if (
        !user_interactions[INTR_PREVIOUS_TOUCH_OR_LEFTCLICK_START].handled)
    {
        user_interactions[INTR_PREVIOUS_TOUCH_OR_LEFTCLICK_START].
            handled = true;
        
        if (
            user_interactions[INTR_PREVIOUS_TOUCH_OR_LEFTCLICK_START].
                touchable_id_top == 5 ||
            user_interactions[INTR_PREVIOUS_TOUCH_OR_LEFTCLICK_START].
                touchable_id_pierce == 5)
        {
            #if SCHEDULED_ANIMS_ACTIVE
            scheduled_animations_request_bump(
                /* const int32_t object_id: */
                    20,
                /* const uint32_t wait: */
                    0);
            #endif
        }
    }
    
    if (keypress_map[TOK_KEY_R]) {
        for (uint32_t i = 0; i < zsprites_to_render->size; i++) {
            if (zsprites_to_render->cpu_data[i].zsprite_id == 20)
            {
                zsprites_to_render->gpu_data[i].xyz_angle[0] += 0.014f;
                zsprites_to_render->gpu_data[i].xyz_angle[1] += 0.01f;
                zsprites_to_render->gpu_data[i].xyz_angle[2] += 0.003f;
            }
        }
    }
    
    if (
        !user_interactions[INTR_PREVIOUS_RIGHTCLICK_START].handled)
    {
        user_interactions[INTR_PREVIOUS_RIGHTCLICK_START].handled = true;
    }
    
    if (
        !user_interactions[INTR_PREVIOUS_MOUSE_OR_TOUCH_MOVE].handled)
    {
        user_interactions[INTR_PREVIOUS_MOUSE_OR_TOUCH_MOVE].handled = true;
    }
    
    #if TEAPOT
    for (uint32_t i = 0; i < 2; i++) {
    if (
        !user_interactions[INTR_PREVIOUS_LEFTCLICK_START].handled &&
        user_interactions[INTR_PREVIOUS_LEFTCLICK_START].touchable_id_top ==
            teapot_touchable_ids[i])
    {
        user_interactions[INTR_PREVIOUS_LEFTCLICK_START].handled = true;
        
        #if SCHEDULED_ANIMS_ACTIVE
        scheduled_animations_request_bump(teapot_object_ids[0], 0.0f);
        #endif
    }
    }
    
    if (keypress_map[TOK_KEY_R]) {
        for (uint32_t i = 0; i < zsprites_to_render->size; i++) {
            if (zsprites_to_render->cpu_data[i].zsprite_id ==
                teapot_object_ids[0])
            {
                zsprites_to_render->gpu_data[i].xyz_angle[1] += 0.01f;
            }
        }
    }
    #endif
    
    client_handle_keypresses(microseconds_elapsed);
}

void client_logic_update_after_render_pass(void) {
    // you can make edits after the objects are copied to the framebuffer
    // and rendered
}

void client_logic_evaluate_terminal_command(
    char * command,
    char * response,
    const uint32_t response_cap)
{
    if (common_are_equal_strings(command, "EXAMPLE COMMAND")) {
        common_strcpy_capped(response, response_cap, "Hello from clientlogic!");
        return;
    }
    
    common_strcpy_capped(
        response,
        response_cap,
        "Unrecognized command - see client_logic_evaluate_terminal_command() "
        "in clientlogic.c");
}

void client_logic_window_resize(
    const uint32_t new_height,
    const uint32_t new_width)
{
    // You're notified that the window is resized!
}

void client_logic_shutdown(void) {
    // You're notified that your application is about to shut down
}
