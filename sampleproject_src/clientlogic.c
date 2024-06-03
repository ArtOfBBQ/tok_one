#include "clientlogic.h"

static int32_t teapot_object_id = -1;
void client_logic_startup(void) {
    
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
    light->RGBA[0]       =  0.50f;
    light->RGBA[1]       =  0.15f;
    light->RGBA[2]       =  0.15f;
    light->RGBA[3]       =  1.00f;
    light->ambient       =  1.0f;
    light->diffuse       =  1.00f;
    light->reach         =  2.00f;
    light->xyz[0]        = -0.50f;
    light->xyz[1]        =  0.50f;
    light->xyz[2]        =  1.25f;
    commit_zlight(light);
    
    int32_t teapot_mesh_id = new_mesh_id_from_resource("teapot.obj");
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
    teapot_request.gpu_data->xyz[2]                = 0.15f;
    teapot_request.cpu_data->object_id             = teapot_object_id;
    teapot_request.cpu_data->visible               = true;
    teapot_request.cpu_data->touchable_id          = -1;
    teapot_request.gpu_materials[0].rgba[0]        = 0.5f;
    teapot_request.gpu_materials[0].rgba[1]        = 0.5f;
    teapot_request.gpu_materials[0].rgba[2]        = 0.5f;
    teapot_request.gpu_materials[0].rgba[3]        = 1.0f;
    teapot_request.gpu_materials[0].texturearray_i =   -1;
    teapot_request.gpu_materials[0].texture_i      =   -1;
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
    
    if (keypress_map[TOK_KEY_T] == true)
    {
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
    
    if (keypress_map[TOK_KEY_L] == true) {
        keypress_map[TOK_KEY_L] = false;
        LineParticle * lines = next_lineparticle_effect();
        PolygonRequest lines_polygon;
        lines_polygon.cpu_data = &lines->zpolygon_cpu;
        lines_polygon.gpu_data = &lines->zpolygon_gpu;
        lines_polygon.gpu_materials = &lines->zpolygon_material;
        construct_quad(
            /* const float left_x: */
                0.0f,
            /* const float bottom_y: */
                0.0f,
            /* const float z: */
                0.5f,
            /* const float width: */
                screenspace_width_to_width(75.0f, 0.5f),
            /* const float height: */
                screenspace_height_to_height(75.0f, 0.5f),
            /* PolygonRequest * stack_recipient: */
                &lines_polygon);
        lines_polygon.gpu_data->ignore_camera = false;
        lines_polygon.gpu_data->ignore_lighting = true;
        
        lines->zpolygon_material.texturearray_i = 1;
        lines->zpolygon_material.texture_i = 0;
        
        lines_polygon.cpu_data->committed = true;
        lines->waypoint_duration[0] = 1250000;
        lines->waypoint_x[0] = screenspace_x_to_x(
            /* const float screenspace_x: */
                0,
            /* const float given_z: */
                0.5f);
        lines->waypoint_y[0] = screenspace_y_to_y(
            /* const float screenspace_y: */
                0,
            /* const float given_z: */
                0.5f);
        lines->waypoint_z[0] = 0.5f;
        lines->waypoint_r[0] = 0.8f;
        lines->waypoint_g[0] = 0.1f;
        lines->waypoint_b[0] = 0.1f;
        lines->waypoint_a[0] = 1.0f;
        lines->waypoint_scalefactor[0] = 1.0f;
        lines->waypoint_duration[0] = 350000;
        
        lines->waypoint_x[1] = screenspace_x_to_x(
            /* const float screenspace_x: */
                window_globals->window_width,
            /* const float given_z: */
                0.5f);
        lines->waypoint_y[1] = screenspace_y_to_y(
            /* const float screenspace_y: */
                0,
            /* const float given_z: */
                0.5f);
        lines->waypoint_z[1] = 0.5f;
        
        lines->waypoint_r[1] = 0.4f;
        lines->waypoint_g[1] = 0.8f;
        lines->waypoint_b[1] = 0.2f;
        lines->waypoint_a[1] = 1.0f;
        lines->waypoint_scalefactor[1] = 0.85f;
        lines->waypoint_duration[1] = 350000;
                
        lines->trail_delay = 500000;
        lines->waypoints_size = 2;
        lines->particle_count = 50;
        lines->particle_zangle_variance_pct = 15;
        lines->particle_rgb_variance_pct = 15;
        lines->particle_scalefactor_variance_pct = 35;
        commit_lineparticle_effect(lines);
    }
    
    if (keypress_map[TOK_KEY_BACKSLASH] == true) {
        // / key
        camera.xyz[2] -= 0.01f;
    }
    
    if (keypress_map[TOK_KEY_UNDERSCORE] == true) {
        camera.xyz[2] += 0.01f;
    }
}

void client_logic_update(uint64_t microseconds_elapsed)
{
    request_fps_counter(microseconds_elapsed);
    
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

