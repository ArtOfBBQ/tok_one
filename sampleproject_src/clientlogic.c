#include "clientlogic.h"

/*
Call this example function in client_logic_startup to make a fountain
*/
static void request_particle_fountain() {
    ParticleEffect fountain;
    construct_particle_effect(&fountain);
    fountain.x = 0.0f;
    fountain.y = 0.0f;
    fountain.z = 0.75f;
    fountain.mesh_id_to_spawn = 1;
    fountain.particle_y_multiplier = 0.007f;
    fountain.particle_x_multiplier = 0.007f;
    fountain.particle_z_multiplier = 0.007f;
    fountain.particle_lifespan = 2300000;
    fountain.particle_spawns_per_second = 650;
    fountain.pause_between_spawns = 2300000;
    
    fountain.particle_direction[0] = 0.0f;
    fountain.particle_direction[1] = 1.0f;
    fountain.particle_direction[2] = 0.0f;
    fountain.particle_direction_max_x_angle_variance = 60;
    fountain.particle_direction_max_y_angle_variance = 0;
    fountain.particle_direction_max_z_angle_variance = 60;
    fountain.particle_distance_per_second   =  0.6f;
    
    fountain.squared_direction[0] =  0.0f;
    fountain.squared_direction[1] = -1.0f;
    fountain.squared_direction[2] =  0.0f;
    fountain.squared_direction_max_x_angle_variance = 60;
    fountain.squared_direction_max_y_angle_variance = 0;
    fountain.squared_direction_max_z_angle_variance = 60;
    fountain.squared_distance_per_second    =  0.26f;
    
    fountain.particle_origin_max_x_variance = 3;
    fountain.particle_origin_max_y_variance = 3;
    fountain.particle_origin_max_z_variance = 3;

    fountain.particle_rgba_progression[0][0] = 1.0f;
    fountain.particle_rgba_progression[0][1] = 1.0f;
    fountain.particle_rgba_progression[0][2] = 1.0f;
    fountain.particle_rgba_progression[0][3] = 1.0f;
    fountain.particle_rgba_progression[1][0] = 0.6f;
    fountain.particle_rgba_progression[1][1] = 0.6f;
    fountain.particle_rgba_progression[1][2] = 1.0f;
    fountain.particle_rgba_progression[1][3] = 1.0f;
    fountain.particle_rgba_progression[2][0] = 0.4f;
    fountain.particle_rgba_progression[2][1] = 0.4f;
    fountain.particle_rgba_progression[2][2] = 1.0f;
    fountain.particle_rgba_progression[2][3] = 1.0f;
    fountain.particle_rgba_progression[3][0] = 0.3f;
    fountain.particle_rgba_progression[3][1] = 0.2f;
    fountain.particle_rgba_progression[3][2] = 1.0f;
    fountain.particle_rgba_progression[3][3] = 1.0f;
    fountain.particle_rgba_progression[4][0] = 0.6f;
    fountain.particle_rgba_progression[4][1] = 0.6f;
    fountain.particle_rgba_progression[4][2] = 1.0f;
    fountain.particle_rgba_progression[4][3] = 1.0f;
    fountain.particle_rgba_progression[5][0] = 0.8f;
    fountain.particle_rgba_progression[5][1] = 0.8f;
    fountain.particle_rgba_progression[5][2] = 1.0f;
    fountain.particle_rgba_progression[5][3] = 1.0;
    fountain.particle_rgba_progression[6][0] = 1.0f;
    fountain.particle_rgba_progression[6][1] = 1.0f;
    fountain.particle_rgba_progression[6][2] = 1.0f;
    fountain.particle_rgba_progression[6][3] = 0.0;
    
    fountain.particle_rgba_progression_size = 7;

    fountain.random_texturearray_i[0] = 1;
    fountain.random_texture_i[0] = 0;
    fountain.random_texturearray_i[1] = 1;
    fountain.random_texture_i[1] = 1;
    fountain.random_texturearray_i[2] = 1;
    fountain.random_texture_i[2] = 2;

    fountain.random_textures_size = 3;
    
    request_particle_effect(&fountain);
}

static float slider_value = 0.0f;
void client_logic_startup(void) {
    
    const char * fontfile;
        fontfile = "font.png";
    register_new_texturearray_by_splitting_file(
        /* filename : */ fontfile,
        /* rows     : */ 10,
        /* columns  : */ 10);
    
    char * filenames[3] = {
       "blob1.png",
       "blob2.png",
       "blob3.png",
    };
    register_new_texturearray_from_files(
        (const char **)filenames,
        3);
    
    char * filenames_2[1] = {
       "structuredart2.png",
    };
    register_new_texturearray_from_files((const char **)filenames_2, 1);
    
    char * obj_filenames[3] = {
        "xmas_tree.obj",
        "tree.obj",
        "disk.obj",
    };
    
    // all sliders use this image as the background 
    get_texture_location(
	/* char * for_filename: */
	    "structuredart2.png",
	/* int32_t * texture_array_i_recipient: */
	    &next_ui_element_settings.slider_background_texturearray_i,
	/* int32_t * texture_i_recipient: */
	    &next_ui_element_settings.slider_background_texture_i);
    // all sliders use this image as the pin you slide left/right
    get_texture_location(
	/* char * for_filename: */
	    "structuredart2.png",
	/* int32_t * texture_array_i_recipient: */
	    &next_ui_element_settings.slider_background_texturearray_i,
	/* int32_t * texture_i_recipient: */
	    &next_ui_element_settings.slider_background_texture_i);

    // all sliders use this width/height
    next_ui_element_settings.slider_width_screenspace = 300;
    next_ui_element_settings.slider_height_screenspace = 20;
    
    request_float_slider(
        /* const float x_screenspace: */
            250,
        /* const float y_screenspace: */
            250,
        /* const float z: */
            0.75f,
        /* const float min_value: */
            0.0f,
        /* const float max_value: */
            1.0f,
        /* const float * linked_value: */
            &slider_value);
}

void client_logic_threadmain(int32_t threadmain_id) {
    switch (threadmain_id) {
        case (0):
            // load_assets(1, texture_arrays_size - 1);
            break;
        default:
            log_append("unhandled threadmain_id: ");
            log_append_int(threadmain_id);
            log_append("\n");
    }
}

void client_logic_animation_callback(int32_t callback_id)
{
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
}

static void  client_handle_touches_and_leftclicks(
    uint64_t microseconds_elapsed)
{
    if (!user_interactions[INTR_PREVIOUS_LEFTCLICK_START].handled) {
        user_interactions[INTR_PREVIOUS_LEFTCLICK_START].handled = true;
        
        if (user_interactions[INTR_PREVIOUS_LEFTCLICK_START].touchable_id < 5) {
            request_bump_animation(
                user_interactions[INTR_PREVIOUS_LEFTCLICK_START].touchable_id,
                0.0f);
        }
    }
}

static void client_handle_keypresses(
    uint64_t microseconds_elapsed)
{
    float elapsed_mod =
        (float)((double)microseconds_elapsed / (double)16666);
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
    
    if (keypress_map[TOK_KEY_BACKSLASH] == true) {                                             
        // / key                                           
        camera.z -= 0.01f;                                                      
    }
    
    if (keypress_map[TOK_KEY_UNDERSCORE] == true) {                                             
        camera.z += 0.01f;                                                      
    }
    
    if (keypress_map[TOK_KEY_SPACEBAR] == true) {                                             
        // spacebar key is pressed
        keypress_map[TOK_KEY_SPACEBAR] = false;
        float bullet_size = 0.1f;     
        
        zVertex camera_direction;
        camera_direction.x = 0.0f;
        camera_direction.y = 0.0f;
        camera_direction.z = 1.0f;
        
        camera_direction = x_rotate_zvertex(&camera_direction, camera.x_angle);
        camera_direction = y_rotate_zvertex(&camera_direction, camera.y_angle);
        
        zPolygon bullet;
        construct_quad(                                            
            /* left: */
                camera.x - (bullet_size / 2),                                   
            /* top:  */                                   
                camera.y - (bullet_size / 2),      
            /* z:    */
                1.0f,
            /* width: */
                bullet_size,                                                  
            /* height: */
                bullet_size,
            /* recipient: */
                &bullet);
        bullet.object_id = 54321 + (tok_rand() % 1000);
        bullet.z = camera.z;
        bullet.triangle_materials[0].color[0] =
            ((float)(tok_rand() % 100)) / 100.0f;
        bullet.triangle_materials[0].color[1] =
            ((float)(tok_rand() % 100)) / 100.0f;
        bullet.triangle_materials[0].color[2] =
            ((float)(tok_rand() % 100)) / 100.0f;
        bullet.ignore_lighting = true;
        bullet.x_multiplier = 0.02f;
        bullet.y_multiplier = 0.02f;
        
        zLightSource bullet_light;
        bullet_light.object_id = bullet.object_id;
        bullet_light.RGBA[0] = 1.0f;
        bullet_light.RGBA[1] = 0.5f;
        bullet_light.RGBA[2] = 0.0f;
        bullet_light.RGBA[3] = 1.0f;
        bullet_light.reach = 1.5f;
        bullet_light.ambient = 0.2f;
        bullet_light.diffuse = 2.0f;
        bullet_light.x = bullet.x;
        bullet_light.y = bullet.y;
        bullet_light.z = bullet.z;
        bullet_light.deleted = false;
        
        ScheduledAnimation * move_bullet = next_scheduled_animation();
        move_bullet->affected_object_id = bullet.object_id;
        move_bullet->delta_x_per_second = camera_direction.x;
        move_bullet->delta_y_per_second = camera_direction.y;
        move_bullet->delta_z_per_second = camera_direction.z * 3.0f;
        move_bullet->delete_object_when_finished = true;
        move_bullet->duration_microseconds = 2500000;
        move_bullet->runs = 1;
        
        request_zpolygon_to_render(&bullet);
        request_zlightsource(&bullet_light);
        
        commit_scheduled_animation(move_bullet);
    }
}

bool32_t fading_out = true;

void client_logic_update(uint64_t microseconds_elapsed)
{
    request_fps_counter(microseconds_elapsed);
    
    client_handle_touches_and_leftclicks(microseconds_elapsed);
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
    
    strcpy_capped(response, response_cap, "Unrecognized command");
}

void client_logic_window_resize(
    const uint32_t new_height,
    const uint32_t new_width)
{
    char unhandled_rs_msg[256];
    strcpy_capped(
        unhandled_rs_msg,
        256,
        "Error: unhandled client_logic_window_resize() to height/width: of ");
    strcat_uint_capped(unhandled_rs_msg, 256, new_height);
    strcat_capped(unhandled_rs_msg, 256, ", ");
    strcat_uint_capped(unhandled_rs_msg, 256, new_width);
    strcat_capped(
        unhandled_rs_msg,
        256,
        ".\nEither prevent app resizing or handle in clientlogic.c\n");
    log_append(unhandled_rs_msg);
    log_dump_and_crash(unhandled_rs_msg);
}

void client_logic_shutdown(void) {
    // Your application shutdown code goes here!
}
