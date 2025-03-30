#include "clientlogic.h"

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
    const char * fontfile = "font.png";
    if (platform_resource_exists("font.png")) {
        register_new_texturearray_by_splitting_file(
            /* filename : */ fontfile,
            /* rows     : */ 10,
            /* columns  : */ 10);
    } else {
        log_assert(0);
    }
    
    #if TEAPOT
    // teapot_mesh_id = BASIC_CUBE_MESH_ID;
    teapot_mesh_id = new_mesh_id_from_resource("teapot.obj");
    #endif
}

void client_logic_late_startup(void) {
    
    #define TEAPOT_X -0.25f
    #define TEAPOT_Y  0.1f
    #define TEAPOT_Z  0.32f
    
    zLightSource * light = next_zlight();
    light->RGBA[0]       =  0.45f;
    light->RGBA[1]       =  0.35f;
    light->RGBA[2]       =  0.35f;
    light->RGBA[3]       =  1.00f;
    light->ambient       =  0.25f;
    light->diffuse       =  1.50f;
    light->reach         =  5.00f;
    light->xyz[0]        =  TEAPOT_X;
    light->xyz[1]        =  TEAPOT_Y + 4.0f;
    light->xyz[2]        =  TEAPOT_Z;
    light->xyz_angle[0]  =  -1.5708f; // rotate around the x axis because light is looking at +z right now but needs to look down
    light->xyz_angle[1]  =  0.00f;
    light->xyz_angle[2]  =  0.00f;
    commit_zlight(light);
    
    shadowcaster_light_i = 0;
    camera.xyz[0] = 0.0f;
    camera.xyz[1] = 0.0f;
    camera.xyz[2] = TEAPOT_Z - 0.45f;
    camera.xyz_angle[0] = 0.0f;
    camera.xyz_angle[1] = 0.0f;
    camera.xyz_angle[2] = 0.0f;
    
    //    light = next_zlight();
    //    light->RGBA[0]       =  0.25f;
    //    light->RGBA[1]       =  0.35f;
    //    light->RGBA[2]       =  0.25f;
    //    light->RGBA[3]       =  1.00f;
    //    light->ambient       =  0.25f;
    //    light->diffuse       =  1.50f;
    //    light->reach         =  5.00f;
    //    light->xyz[0]        =  1.55f;
    //    light->xyz[1]        =  0.60f;
    //    light->xyz[2]        =  1.00f;
    //    light->xyz_angle[0]  = -1.00f;
    //    light->xyz_angle[1]  =  0.00f;
    //    light->xyz_angle[2]  =  0.00f;
    //    commit_zlight(light);
    
    #if TEAPOT
    // register_new_texturearray_from_files("giant_head_texture.png", 1);
    
    teapot_object_ids[0] = next_nonui_object_id();
    teapot_object_ids[1] = next_nonui_object_id();
    
    for (uint32_t i = 0; i < 1; i++) {
        PolygonRequest teapot_request;
        teapot_request.materials_size = 1;
        request_next_zpolygon(&teapot_request);
        construct_zpolygon(&teapot_request);
        teapot_request.cpu_data->mesh_id = teapot_mesh_id;
        //    scale_zpolygon_multipliers_to_height(
        //        teapot_request.cpu_data,
        //        teapot_request.gpu_data,
        //        0.25f);
        teapot_request.gpu_data->xyz_multiplier[0] = 0.15f;
        teapot_request.gpu_data->xyz_multiplier[1] = 0.15f;
        teapot_request.gpu_data->xyz_multiplier[2] = 0.15f;
        teapot_request.gpu_data->xyz[0]            = TEAPOT_X + (i * 0.20f);
        teapot_request.gpu_data->xyz[1]            = TEAPOT_Y + (i * 0.10f);
        teapot_request.gpu_data->xyz[2]            = TEAPOT_Z - (i * 0.25f);
        teapot_request.gpu_data->xyz_angle[0]      = 0.00f;
        teapot_request.gpu_data->xyz_angle[1]      = 3.2f;
        teapot_request.gpu_data->xyz_angle[2]      = 0.0f;
        teapot_request.cpu_data->sprite_id         = teapot_object_ids[i];
        teapot_request.cpu_data->visible           = true;
        teapot_touchable_ids[i]                    = next_nonui_touchable_id();
        teapot_request.gpu_data->touchable_id      = teapot_touchable_ids[i];
        for (uint32_t mat_i = 0; mat_i < MAX_MATERIALS_PER_POLYGON; mat_i++) {
            teapot_request.gpu_materials[mat_i].rgba[0]        =  1.3f;
            teapot_request.gpu_materials[mat_i].rgba[1]        =  1.3f;
            teapot_request.gpu_materials[mat_i].rgba[2]        =  1.3f;
            teapot_request.gpu_materials[mat_i].rgba[3]        =  1.0f;
            teapot_request.gpu_materials[mat_i].texturearray_i = -1.0f;
            teapot_request.gpu_materials[mat_i].texture_i      = -1.0f;
            teapot_request.gpu_materials[mat_i].specular       =  1.0f;
            teapot_request.gpu_materials[mat_i].diffuse        =  1.0f;
            teapot_request.gpu_materials[mat_i].texturearray_i =    -1;
            teapot_request.gpu_materials[mat_i].texture_i      =    -1;
        }
        teapot_request.gpu_data->ignore_lighting =  0.0f;
        teapot_request.gpu_data->ignore_camera =  0.0f;
        log_assert(teapot_request.gpu_data->xyz_offset[0] == 0.0f);
        log_assert(teapot_request.gpu_data->xyz_offset[1] == 0.0f);
        log_assert(teapot_request.gpu_data->xyz_offset[2] == 0.0f);
        commit_zpolygon_to_render(&teapot_request);
    }
    #endif
    
    #if 1
    PolygonRequest quad[1];
    for (uint32_t i = 0; i < 1; i++) {
        request_next_zpolygon(&quad[i]);
        construct_quad(
            /* const float left_x: */
                TEAPOT_X - 0.75f,
            /* const float bottom_y: */
                TEAPOT_Y - 1.25f,
            /* const float z: */
                TEAPOT_Z - 0.08f,
            /* const float width: */
                windowsize_screenspace_width_to_width(
                    window_globals->window_width, 1.0f),
            /* const float height: */
                windowsize_screenspace_height_to_height(
                    window_globals->window_height, 1.0f),
            /* PolygonRequest * stack_recipient: */
                &quad[i]);
        quad[i].gpu_materials->texturearray_i    = -1;
        quad[i].gpu_materials->texture_i         = -1;
        quad[i].cpu_data->sprite_id              = -1;
        quad[i].gpu_data->touchable_id           = -1;
        quad[i].cpu_data->alpha_blending_enabled = false;
        
        quad[i].gpu_data->xyz_offset[0]          = 0.0f;
        quad[i].gpu_data->xyz_offset[1]          = 0.0f;
        quad[i].gpu_data->xyz_offset[2]          = 0.0f;
        quad[i].gpu_data->scale_factor           = 1.0f;
        quad[i].gpu_data->xyz_angle[0]           = 1.8f;
        quad[i].gpu_data->xyz_angle[1]           = 0.0f;
        quad[i].gpu_data->xyz_angle[2]           = 0.0f;
        quad[i].gpu_data->ignore_camera          = 0.0f;
        quad[i].gpu_data->ignore_lighting        = 0.0f;
        
        quad[i].gpu_materials[0].rgba[0]         = 0.6f;
        quad[i].gpu_materials[0].rgba[1]         = 0.4f;
        quad[i].gpu_materials[0].rgba[2]         = 0.3f;
        quad[i].gpu_materials[0].rgba[3]         = 1.0f;
        
        commit_zpolygon_to_render(&quad[i]);
    }
    #endif
    
    #if 1
    #define BOOM_LABEL_TOCUHABLE_ID 1441003
    font_settings->font_height = 280;
    font_settings->font_touchable_id = BOOM_LABEL_TOCUHABLE_ID;
    font_settings->font_color[0] =  2.2f;
    font_settings->font_color[1] =  2.9f;
    font_settings->font_color[2] =  1.8f;
    font_settings->font_color[3] =  1.0f;
    font_settings->ignore_camera = false;
    font_settings->font_ignore_lighting = 1.0f;
    text_request_label_renderable(
        /* const int32_t with_object_id: */
            21,
        /* const char * text_to_draw: */
            "Bloom",
        /* const float left_pixelspace: */
            50.0f,
        /* const float top_pixelspace: */
            300.0f,
        /* const float z: */
            3.5f,
        /* const float max_width: */
            1500.0f);
    font_settings->font_touchable_id = -1;
    log_assert(
        zpolygons_to_render->cpu_data[zpolygons_to_render->size-1].
            sprite_id == 21);
    #endif
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
    
    if (
        keypress_map[TOK_KEY_ENTER] &&
        keypress_map[TOK_KEY_CONTROL])
    {
        keypress_map[TOK_KEY_ENTER] = false;
        platform_toggle_fullscreen();
    }
    
    if (keypress_map[TOK_KEY_S] == true)
    {
        keypress_map[TOK_KEY_S] = false;
        
        #if TEAPOT
        request_shatter_and_destroy(
            /* const int32_t object_id: */
                teapot_object_ids[1],
            /* const uint64_t duration_microseconds: */
                750000);
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
            request_bump_animation(
                /* const int32_t object_id: */
                    20,
                /* const uint32_t wait: */
                    0);
        }
        
        if (
            user_interactions[INTR_PREVIOUS_TOUCH_OR_LEFTCLICK_START].
                touchable_id_top == BOOM_LABEL_TOCUHABLE_ID ||
            user_interactions[INTR_PREVIOUS_TOUCH_OR_LEFTCLICK_START].
                touchable_id_pierce == 6)
        {
            request_bump_animation(
                /* const int32_t object_id: */
                    21,
                /* const uint32_t wait: */
                    0);
        }
    }
    
    if (keypress_map[TOK_KEY_T]) {
        #if TEAPOT
        for (uint32_t i = 0; i < zpolygons_to_render->size; i++) {
            if (
                zpolygons_to_render->cpu_data[i].sprite_id ==
                    teapot_object_ids[0])
            {
                zpolygons_to_render->gpu_data[i].xyz[1] -= 0.01f;
            }
        }
        #endif
    }
    
    if (keypress_map[TOK_KEY_E]) {
        #if TEAPOT
        for (uint32_t i = 0; i < zpolygons_to_render->size; i++) {
            if (
                zpolygons_to_render->cpu_data[i].sprite_id ==
                    teapot_object_ids[0])
            {
                zpolygons_to_render->gpu_data[i].xyz[1] += 0.01f;
            }
        }
        #endif
    }
    
    if (keypress_map[TOK_KEY_R]) {
        for (uint32_t i = 0; i < zpolygons_to_render->size; i++) {
            if (zpolygons_to_render->cpu_data[i].sprite_id == 20)
            {
                zpolygons_to_render->gpu_data[i].xyz_angle[0] += 0.014f;
                zpolygons_to_render->gpu_data[i].xyz_angle[1] += 0.01f;
                zpolygons_to_render->gpu_data[i].xyz_angle[2] += 0.003f;
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
        
        ScheduledAnimation * anim = next_scheduled_animation(true);
        anim->affected_sprite_id = teapot_object_ids[i];
        anim->gpu_polygon_vals.scale_factor = 1.2f;
        anim->duration_microseconds = 100000;
        anim->runs = 1;
        commit_scheduled_animation(anim);
        
        anim = next_scheduled_animation(true);
        anim->affected_sprite_id = teapot_object_ids[i];
        anim->gpu_polygon_vals.scale_factor = 1.0f;
        anim->duration_microseconds = 200000;
        anim->wait_before_each_run = 100000;
        anim->runs = 1;
        commit_scheduled_animation(anim);
    }
    }
    
    if (keypress_map[TOK_KEY_R]) {
        for (uint32_t i = 0; i < zpolygons_to_render->size; i++) {
            if (zpolygons_to_render->cpu_data[i].sprite_id ==
                teapot_object_ids[0])
            {
                zpolygons_to_render->gpu_data[i].xyz_angle[1] += 0.01f;
            }
        }
    }
    #endif
    
    client_handle_keypresses(microseconds_elapsed);
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
    // Your application shutdown code goes here!
}
