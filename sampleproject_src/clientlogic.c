#include "clientlogic.h"

static int32_t teapot_mesh_id = -1;
static int32_t teapot_object_id = -1;
static int32_t teapot_touchable_id = -1;

void client_logic_early_startup(void) {
    
    init_PNG_decoder(
        malloc_from_managed_infoless,
        free_from_managed,
        memset,
        memcpy);
    
    const char * fontfile = "font.png";
    if (platform_resource_exists("font.png")) {
        register_new_texturearray_by_splitting_file(
            /* filename : */ fontfile,
            /* rows     : */ 10,
            /* columns  : */ 10);
    }
    
    char * textures[1];
    textures[0] = "blob1.png";
    register_new_texturearray_from_files(
        /* const char ** filenames: */
            (const char **)textures,
        /* const uint32_t filenames_size: */
            1);
    
    // teapot_mesh_id = BASIC_CUBE_MESH_ID;
    teapot_mesh_id = new_mesh_id_from_resource("teapot_smooth.obj");
}

void client_logic_late_startup(void) {
    PolygonRequest stack_recipient;
    request_next_zpolygon(&stack_recipient);
    construct_quad_around(
            0.0f,
            0.0f,
            1.0f,
            0.25f,
            0.25f,
        /* PolygonRequest *stack_recipient: */
            &stack_recipient);
    stack_recipient.cpu_data->object_id = next_nonui_object_id();
    stack_recipient.cpu_data->alpha_blending_enabled = true;
    stack_recipient.gpu_data->ignore_camera = true;
    stack_recipient.gpu_data->ignore_lighting = true;
    stack_recipient.gpu_materials[0].rgba[0] = 0.0f;
    stack_recipient.gpu_materials[0].rgba[1] = 0.0f;
    stack_recipient.gpu_materials[0].rgba[2] = 0.0f;
    stack_recipient.gpu_materials[0].rgba[3] = 0.5f;
    commit_zpolygon_to_render(&stack_recipient);
    
    zLightSource * light = next_zlight();
    light->RGBA[0]       =  0.70f;
    light->RGBA[1]       =  0.25f;
    light->RGBA[2]       =  0.25f;
    light->RGBA[3]       =  1.00f;
    light->ambient       =  0.0f;
    light->diffuse       =  1.50f;
    light->reach         =  3.00f;
    light->xyz[0]        = -1.25f;
    light->xyz[1]        =  1.00f;
    light->xyz[2]        =  0.10f;
    commit_zlight(light);
    
    light = next_zlight();
    light->RGBA[0]       =  0.05f;
    light->RGBA[1]       =  0.55f;
    light->RGBA[2]       =  0.05f;
    light->RGBA[3]       =  1.00f;
    light->ambient       =  0.0f;
    light->diffuse       =  1.50f;
    light->reach         =  3.00f;
    light->xyz[0]        =  1.55f;
    light->xyz[1]        =  0.60f;
    light->xyz[2]        =  0.10f;
    commit_zlight(light);
    
    teapot_object_id = next_nonui_object_id();
    
    PolygonRequest teapot_request;
    teapot_request.materials_size = 1;
    request_next_zpolygon(&teapot_request);
    construct_zpolygon(&teapot_request);
    teapot_request.cpu_data->mesh_id = teapot_mesh_id;
    scale_zpolygon_multipliers_to_height(
        teapot_request.cpu_data,
        teapot_request.gpu_data,
        0.25f);
    teapot_request.gpu_data->xyz[0]                = 0.00f;
    teapot_request.gpu_data->xyz[1]                = 0.00f;
    teapot_request.gpu_data->xyz[2]                = 0.75f;
    teapot_request.cpu_data->object_id             = teapot_object_id;
    teapot_request.cpu_data->visible               = true;
    teapot_touchable_id = next_nonui_touchable_id();
    teapot_request.cpu_data->touchable_id          = teapot_touchable_id;
    teapot_request.gpu_materials[0].rgba[0]        = 0.5f;
    teapot_request.gpu_materials[0].rgba[1]        = 0.5f;
    teapot_request.gpu_materials[0].rgba[2]        = 0.5f;
    teapot_request.gpu_materials[0].rgba[3]        = 1.0f;
    teapot_request.gpu_materials[0].texturearray_i =   -1;
    teapot_request.gpu_materials[0].texture_i      =   -1;
    teapot_request.gpu_materials[0].specular       = 1.0f;
    teapot_request.gpu_materials[0].diffuse        = 1.0f;
    teapot_request.gpu_data->ignore_lighting       = 0.0f;
    teapot_request.gpu_data->ignore_camera         = 0.0f;
    commit_zpolygon_to_render(&teapot_request);
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
    strcpy_capped(
        unhandled_callback_id,
        256,
        "unhandled client_logic_animation_callback: ");
    strcat_int_capped(
        unhandled_callback_id,
        256,
        callback_id);
    strcat_capped(
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
        
        request_shatter_and_destroy(
            /* const int32_t object_id: */
                teapot_object_id,
            /* const uint64_t duration_microseconds: */
                750000);
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
    request_fps_counter(microseconds_elapsed);
    
    if (keypress_map[TOK_KEY_R]) {
        for (uint32_t i = 0; i < zpolygons_to_render->size; i++) {
            if (zpolygons_to_render->cpu_data[i].object_id ==
                teapot_object_id)
            {
                zpolygons_to_render->gpu_data[i].xyz_angle[1] += 0.01f;
            }
        }
    }
    
    client_handle_keypresses(microseconds_elapsed);
}

void client_logic_evaluate_terminal_command(
    char * command,
    char * response,
    const uint32_t response_cap)
{
    if (are_equal_strings(command, "EXAMPLE COMMAND")) {
        strcpy_capped(response, response_cap, "Hello from clientlogic!");
        return;
    }
    
    strcpy_capped(
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

