#include "clientlogic.h"

/*
Call this example function in client_logic_startup to make a fountain
*/
static void request_particle_fountain() {
    ParticleEffect fountain;
    construct_particle_effect(&fountain);
    fountain.x = -1.5f;
    fountain.y = 0.0f;
    fountain.z = 2.0f;
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

static zPolygon xmastree;
static int32_t xmastree_object_id;
static int32_t label_object_id;
float slider_value = 0.0f;
void client_logic_startup(void) {
    
    init_PNG_decoder(malloc_from_unmanaged);
    
    const char * fontfile = "font.png";
    if (platform_resource_exists("font.png")) {
        register_new_texturearray_by_splitting_file(
            /* filename : */ fontfile,
            /* rows     : */ 10,
            /* columns  : */ 10);
    }
    
    char * filenames[3] = {
       "blob1.png",
       "blob2.png",
       "blob3.png",
    };
    
    register_new_texturearray_from_files(
        (const char **)filenames,
        3);
    
    char * filenames_2[1] = {
        "leaves3.png",
    };
    register_new_texturearray_from_files(
        (const char **)filenames_2, 1);
    
    char * filenames_3[1] = {
        "structuredart2.png",
    };
    register_new_texturearray_from_files(
        (const char **)filenames_3, 1);
    
    char * obj_filenames[3] = {
        "xmas_tree.obj",
        "tree.obj",
        "disk.obj",
    };
    
    int32_t xmastree_mesh_id = new_mesh_id_from_resource(obj_filenames[0]);
    
    center_mesh_offsets(xmastree_mesh_id);
    
    request_particle_fountain();
    
    zLightSource * im_a_light = next_zlight();
    im_a_light->x = -0.75f;
    im_a_light->y = 0.75f;
    im_a_light->z = 0.7f;
    im_a_light->ambient = 0.8f;
    im_a_light->diffuse = 0.8f;
    im_a_light->RGBA[0] = 1.0f;
    im_a_light->RGBA[1] = 1.0f;
    im_a_light->RGBA[2] = 1.0f;
    im_a_light->RGBA[3] = 1.0f;
    im_a_light->reach = 1.5f;
    im_a_light->deleted = false;
    commit_zlight(im_a_light);
    
    // example 1: the 'christams tree' obj file
    for (uint32_t i = 0; i < 2; i++) {
        construct_zpolygon(&xmastree);
        xmastree.mesh_id = xmastree_mesh_id;
        xmastree.x = 0.0f + (0.35f * (i % 10));
        xmastree.y = -0.5f + (0.35f * (i / 10));
        xmastree.z = 1.0f + (0.01 * i);
        xmastree.triangle_materials[0].color[0] = 0.05f; // christmas decorations?
        xmastree.triangle_materials[0].color[1] = 0.05f;
        xmastree.triangle_materials[0].color[2] = 1.0f;
        xmastree.triangle_materials[0].color[3] = 1.0f;
        xmastree.triangle_materials[1].color[0] = 1.0f; // leaves
        xmastree.triangle_materials[1].color[1] = 1.0f;
        xmastree.triangle_materials[1].color[2] = 1.0f;
        xmastree.triangle_materials[1].color[3] = 1.0f;
        xmastree.triangle_materials[1].texturearray_i = 2;
        xmastree.triangle_materials[1].texture_i = 0;
        xmastree.triangle_materials[2].color[0] = 0.3f; // trunk
        xmastree.triangle_materials[2].color[1] = 0.08f;
        xmastree.triangle_materials[2].color[2] = 0.05f;
        xmastree.triangle_materials[2].color[3] = 1.0f;
        xmastree.triangle_materials_size = 3;
        xmastree.x_multiplier = 0.1f;
        xmastree.y_multiplier = 0.1f;
        xmastree.z_multiplier = 0.1f;
        
        // example 2: use a quad instead
        //    construct_quad(
        //        /* const float left_x: */
        //            0.25f,
        //        /* const float bottom_y: */
        //            0.0f,
        //        /* const float z: */
        //            0.75f,
        //        /* const float width: */
        //            0.2f,
        //        /* const float height: */
        //            0.2f,
        //        /* zPolygon * recipient: */
        //            &xmastree);
        //    xmastree.triangle_materials[0].texturearray_i = 3;
        //    xmastree.triangle_materials[0].texture_i = 0;
        
        // this code needs to run regardless of what example mesh we're using
        xmastree_object_id = next_nonui_object_id();
        xmastree.object_id = xmastree_object_id;
        request_zpolygon_to_render(&xmastree);
    }
    
    create_shattered_version_of_mesh(
        /* mesh_id: */
            xmastree.mesh_id,
        /* triangles_multiplier: */
            30);
    
    label_object_id = next_nonui_object_id();
    request_label_around(
        label_object_id,
        "Press T for a shatter effect",
        /* mid_x_pixelspace: */ window_globals->window_width / 2,
        /* y: */ window_globals->window_height / 2,
        /* z: */ 0.75f,
        /* max_width: */ (window_globals->window_width / 4) * 3,
        /* ignore_camera: */ true);
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
    
    if (keypress_map[TOK_KEY_BACKSLASH] == true) {                                             
        // / key                                           
        camera.z -= 0.01f;                                                      
    }
    
    if (keypress_map[TOK_KEY_UNDERSCORE] == true) {                                             
        camera.z += 0.01f;                                                      
    }
    
    if (keypress_map[TOK_KEY_T] == true) {
        keypress_map[TOK_KEY_T] = false;
        
        if (tok_rand() % 2 == 0) {
            float xyz_rot_per_sec[3];
            xyz_rot_per_sec[0] = 0.1f;
            xyz_rot_per_sec[1] = 0.1f;
            xyz_rot_per_sec[2] = 0.1f;
            float lin_direct[3];
            lin_direct[0] = 0.1f;
            lin_direct[1] = 0.0f;
            lin_direct[2] = 0.1f;
            request_shatter_and_destroy(
                /* const int32_t object_id: */
                    label_object_id,
                /* const uint64_t wait_before_first_run: */
                    500,
                /* const uint64_t duration_microseconds: */
                    2000000,
                /* const float exploding_distance_per_second: */
                    0.2f,
                /* const float xyz_rotation_per_second[3]: */
                    xyz_rot_per_sec,
                /* const float linear_distance_per_second: */
                    0.1f,
                /* const float linear_direction[3]: */
                    lin_direct);
        } else {
            delete_zpolygon_object(xmastree_object_id);
            ShatterEffect * dissolving_quad = next_shatter_effect();
            dissolving_quad->zpolygon_to_shatter = xmastree;
            
            //                                                     40.000 0.4s
            dissolving_quad->longest_random_delay_before_launch =  400000;
            dissolving_quad->exploding_distance_per_second = 0.4f;
            dissolving_quad->linear_distance_per_second = 0.3f;
            dissolving_quad->linear_direction[0] =  0.0f;
            dissolving_quad->linear_direction[1] =  1.0f;
            dissolving_quad->linear_direction[2] =  0.0f;
            dissolving_quad->squared_distance_per_second = 0.15f;
            dissolving_quad->squared_direction[0] =  1.0f;
            dissolving_quad->squared_direction[1] =  0.0f;
            dissolving_quad->squared_direction[2] =  0.0f;
            
            //                                             75.000 0.75 seconds
            dissolving_quad->start_fade_out_at_elapsed  =  750000;
            //                                            15..000 1.5 seconds
            dissolving_quad->finish_fade_out_at_elapsed = 1500000;
            commit_shatter_effect(dissolving_quad);
        }
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
            slider_value;
        bullet.triangle_materials[0].color[1] =
            ((float)(tok_rand() % 100)) / 100.0f;
        bullet.triangle_materials[0].color[2] =
            ((float)(tok_rand() % 100)) / 100.0f;
        bullet.ignore_lighting = true;
        bullet.x_multiplier = 0.02f;
        bullet.y_multiplier = 0.02f;
        
        zLightSource * bullet_light = next_zlight();
        bullet_light->object_id = bullet.object_id;
        bullet_light->RGBA[0] = 1.0f;
        bullet_light->RGBA[1] = 0.5f;
        bullet_light->RGBA[2] = 0.0f;
        bullet_light->RGBA[3] = 1.0f;
        bullet_light->reach = 1.5f;
        bullet_light->ambient = 0.2f;
        bullet_light->diffuse = 2.0f;
        bullet_light->x = bullet.x;
        bullet_light->y = bullet.y;
        bullet_light->z = bullet.z;
        
        ScheduledAnimation * move_bullet = next_scheduled_animation();
        move_bullet->affected_object_id = bullet.object_id;
        move_bullet->delta_x_per_second = camera_direction.x;
        move_bullet->delta_y_per_second = camera_direction.y;
        move_bullet->delta_z_per_second = camera_direction.z * 3.0f;
        move_bullet->delete_object_when_finished = true;
        move_bullet->duration_microseconds = 2500000;
        move_bullet->runs = 1;
        
        request_zpolygon_to_render(&bullet);
        commit_zlight(bullet_light);
        
        commit_scheduled_animation(move_bullet);
    }
}

bool32_t fading_out = true;

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
    
    strcpy_capped(response, response_cap, "Unrecognized command");
}

void client_logic_window_resize(
    const uint32_t new_height,
    const uint32_t new_width)
{
    
}

void client_logic_shutdown(void) {
    // Your application shutdown code goes here!
}
