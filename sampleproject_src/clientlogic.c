#include "clientlogic.h"

void client_logic_startup(void) {
    
    init_PNG_decoder(malloc_from_managed, free_from_managed);
    
    const char * fontfile = "font.png";
    if (platform_resource_exists("font.png")) {
        register_new_texturearray_by_splitting_file(
            /* filename : */ fontfile,
            /* rows     : */ 10,
            /* columns  : */ 10);
    }
    
    char * textures[1];
    textures[0] = "structuredart1.png";
    register_new_texturearray_from_files(
        /* const char ** filenames: */
            (const char **)textures,
        /* const uint32_t filenames_size: */
            1);
    
    PolygonRequest quad;
    request_next_zpolygon(&quad);

    construct_quad(
        /* const float left_x: */
            -0.125f,
        /* const float top_y: */
            -0.125f,
        /* const float z: */
            0.75f,
        /* const float width: */
            0.25f,
        /* const float height: */
            0.25f,
        /* zPolygon *recipient: */
            &quad);
    quad.cpu_data->object_id = 321;
    quad.cpu_data->visible   = true;
    quad.gpu_data->ignore_lighting = true;
    quad.gpu_material[0].texture_i      = 0;
    quad.gpu_material[0].texturearray_i = 1;
    quad.gpu_material[0].rgba[0] = 0.50f;
    quad.gpu_material[0].rgba[1] = 0.50f;
    quad.gpu_material[0].rgba[2] = 0.75f;
    quad.gpu_material[0].rgba[3] = 1.00f;
    commit_zpolygon_to_render(&quad);
    
    //    PolygonRequest teapot;
    //    request_next_zpolygon(&teapot);
    //    construct_zpolygon(&teapot);
    //    teapot.cpu_data->object_id            = 321;
    //    teapot.cpu_data->visible              = true;
    //    teapot.gpu_data->xyz[0]               = 0.05f;
    //    teapot.gpu_data->xyz[1]               = 0.05f;
    //    teapot.gpu_data->xyz[2]               = 0.25f;
    //    teapot.gpu_data->xyz_multiplier[0]    = 1.0f;
    //    teapot.gpu_data->xyz_multiplier[1]    = 1.0f;
    //    teapot.gpu_data->xyz_multiplier[2]    = 1.0f;
    //    teapot.gpu_data->xyz_offset[0]        = 0.0f;
    //    teapot.gpu_data->xyz_offset[1]        = 0.0f;
    //    teapot.gpu_data->xyz_offset[2]        = 0.0f;
    //    teapot.gpu_material[0].texture_i      = -1;
    //    teapot.gpu_material[0].texturearray_i = -1;
    //    teapot.gpu_material[0].rgba[0]        = 0.50f;
    //    teapot.gpu_material[0].rgba[1]        = 0.50f;
    //    teapot.gpu_material[0].rgba[2]        = 0.75f;
    //    teapot.gpu_material[0].rgba[3]        = 1.00f;
    //    int32_t teapot_mesh_id = new_mesh_id_from_resource("teapot.obj");
    //    teapot.cpu_data->mesh_id              = teapot_mesh_id;
    //    log_assert(teapot.cpu_data->mesh_id == 3); // after quad, cube, and point
    //    teapot.gpu_data->ignore_lighting      = false;
    //    teapot.gpu_data->ignore_camera        = false;
    //    scale_zpolygon_multipliers_to_height(
    //        /* zPolygonCPU * cpu_data: */
    //            teapot.cpu_data,
    //        /* GPUPolygon *gpu_data: */
    //            teapot.gpu_data,
    //        /* const float new_height: */
    //            0.25f);
    //    center_mesh_offsets(teapot_mesh_id);
    //    commit_zpolygon_to_render(&teapot);
    
    zLightSource * light = next_zlight();
    light->RGBA[0] =  0.50f;
    light->RGBA[1] =  0.15f;
    light->RGBA[2] =  0.15f;
    light->RGBA[3] =  1.00f;
    light->ambient =  1.00f;
    light->diffuse =  1.00f;
    light->reach   =  5.00f;
    light->x       = -2.00f;
    light->y       =  0.50f;
    light->z       =  0.75f;
    commit_zlight(light);
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
    float elapsed_mod = (float)((double)microseconds_elapsed / (double)16666);
    float cam_speed = 0.1f * elapsed_mod;
    float cam_rotation_speed = 0.05f * elapsed_mod;
    
    if (keypress_map[TOK_KEY_LEFTARROW] == true)
    {
        camera.x -= cam_speed;
    }
    
    if (keypress_map[TOK_KEY_RIGHTARROW] == true)
    {
        camera.x += cam_speed;
    }
    
    if (keypress_map[TOK_KEY_DOWNARROW] == true)
    {
        camera.y -= cam_speed;
    }
    
    if (keypress_map[TOK_KEY_UPARROW] == true)
    {
        camera.y += cam_speed;
    }
    
    if (keypress_map[TOK_KEY_A] == true) {
        camera.x_angle += cam_rotation_speed;
    }
    
    if (keypress_map[TOK_KEY_Z] == true) {
        camera.z_angle -= cam_rotation_speed;
    }
    
    if (keypress_map[TOK_KEY_X] == true) {
        camera.z_angle += cam_rotation_speed;
    }
    
    if (keypress_map[TOK_KEY_Q] == true) {
        camera.x_angle -= cam_rotation_speed;
    }
    
    if (keypress_map[TOK_KEY_W] == true) {
        camera.y_angle -= cam_rotation_speed;
    }
    
    if (keypress_map[TOK_KEY_S] == true) {
        camera.y_angle += cam_rotation_speed;
    }
    
    if (keypress_map[TOK_KEY_T] == true) {
        // particle effect for object id 321
        for (uint32_t zp_i = 0; zp_i < zpolygons_to_render->size; zp_i++) {
            if (zpolygons_to_render->cpu_data[zp_i].object_id != 321) {
                continue;
            }
            
            ShatterEffect * shatter = next_shatter_effect();
            shatter->zpolygon_to_shatter_cpu =
                zpolygons_to_render->cpu_data[zp_i];
            zpolygons_to_render->cpu_data[zp_i].deleted = true;
            shatter->zpolygon_to_shatter_gpu =
                zpolygons_to_render->gpu_data[zp_i];
            shatter->zpolygon_to_shatter_material =
                zpolygons_to_render->gpu_materials[zp_i * MAX_MATERIALS_SIZE];
            
            shatter->longest_random_delay_before_launch = 5000000;
            shatter->linear_direction[0] = 0.8f;
            shatter->linear_direction[1] = 0.1f;
            shatter->linear_direction[2] = 0.1f;
            shatter->linear_distance_per_second = 0.05f;
            shatter->exploding_distance_per_second = 0.05f;
            commit_shatter_effect(shatter);
        }
    }
    
    if (keypress_map[TOK_KEY_BACKSLASH] == true) {
        // / key
        camera.z -= 0.01f;
    }
    
    if (keypress_map[TOK_KEY_UNDERSCORE] == true) {
        camera.z += 0.01f;
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
