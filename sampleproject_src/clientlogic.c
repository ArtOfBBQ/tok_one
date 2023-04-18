#include "clientlogic.h"

void client_logic_startup(void) {

    log_append("sizeof(zPolygon): ");
    log_append_uint(sizeof(zPolygon));
    log_append_char('\n');
    
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
    
    //    for (uint32_t i = 1; i < 5; i++) {
    //        zPolygon new_quad;
    //
    //        // reminder: higher y is higher on screen
    //        float mid_x = -0.5f + (i == 1 || i == 4 ? 0.5f : 0.0f);
    //        float mid_y = (-0.5f + (i < 3 ? 0.5f : 0.0f)) + 0.5f;
    //        log_append("quad: ");
    //        log_append_uint(i);
    //        printf("at: [%f,%f]\n", mid_x, mid_y);
    //        construct_quad_around(
    //            /* const float mid_x: */
    //                mid_x,
    //            /* const float mid_y: */
    //                mid_y,
    //            /* const float z: */
    //                0.5f,
    //            /* const float width: */
    //                0.2f,
    //            /* const float height: */
    //                0.25f,
    //            /* zPolygon * recipient: */
    //                &new_quad);
    //
    //        new_quad.object_id = i;
    //        new_quad.triangle_materials[0].color[0] = 1.0f - (i * 0.25f);
    //        new_quad.triangle_materials[0].color[1] = i * 0.25f;
    //        new_quad.triangle_materials[0].color[2] = 0.3f;
    //        new_quad.triangle_materials[0].color[3] = 1.0f;
    //        new_quad.triangle_materials[0].texturearray_i = 2;
    //        new_quad.triangle_materials[0].texture_i = 0;
    //        new_quad.ignore_lighting = false;
    //        new_quad.touchable_id = i;
    //        request_zpolygon_to_render(&new_quad);
    //    }
    
    
    char * obj_filenames[3] = {
        "key.obj",
        "teapot.obj",
        "disk.obj"
    };
    
    int32_t key_mesh_id =
        new_mesh_id_from_resource(obj_filenames[0]);
    
    for (uint32_t i = key_mesh_id; i < all_mesh_triangles_size; i++) {
        log_assert(all_mesh_triangles[i].parent_material_i >= 0);
    }
    
    //    zPolygon key;
    //    construct_zpolygon(&key);
    //    key.mesh_head_i = all_mesh_summaries[key_mesh_id].all_meshes_head_i;
    //    key.triangles_size = all_mesh_summaries[key_mesh_id].triangles_size;
    //    key.x =  0.15f;
    //    key.y =  0.0f;
    //    key.z =  0.5f;
    //    key.triangle_materials[0].color[0] = 1.0f;
    //    key.triangle_materials[0].color[1] = 1.0f;
    //    key.triangle_materials[0].color[2] = 1.0f;
    //    key.triangle_materials[0].color[3] = 1.0f;
    //    key.triangle_materials[0].texturearray_i = 2;
    //    key.triangle_materials[0].texture_i = 0;
    //    key.triangle_materials_size = 1;
    //
    //    scale_zpolygon_multipliers_to_height(&key, 0.25f);
    //    request_zpolygon_to_render(&key);
    
    int32_t disk_mesh_id =
        new_mesh_id_from_resource(obj_filenames[2]);
    
    zPolygon disk;
    construct_zpolygon(&disk);
    disk.mesh_id = disk_mesh_id;
    disk.x = 1.0f;
    disk.y = 0.0f;
    disk.z = 0.5f;
    disk.triangle_materials[0].color[0] = 1.0f;
    disk.triangle_materials[0].color[1] = 0.0f;
    disk.triangle_materials[0].color[2] = 0.0f;
    disk.triangle_materials[0].color[3] = 1.0f;
    disk.triangle_materials[0].texturearray_i = -1;
    disk.triangle_materials[0].texture_i = -1;
    disk.triangle_materials[1].color[0] = 1.0f;
    disk.triangle_materials[1].color[1] = 1.0f;
    disk.triangle_materials[1].color[2] = 1.0f;
    disk.triangle_materials[1].color[3] = 1.0f;
    disk.triangle_materials[1].texturearray_i = 2;
    disk.triangle_materials[1].texture_i = 0;
    disk.triangle_materials_size = 2;
    disk.x_angle = 3.14f;
    
    scale_zpolygon_multipliers_to_height(&disk, 0.5f);
    request_zpolygon_to_render(&disk);
    
    font_height = 50;
    font_color[0] = 1.0f;
    font_color[1] = 0.0f;
    font_color[2] = 0.4f;
    font_color[3] = 1.0f;
    request_label_offset_around(
        /* const int32_t with_id: */
            -1,
        /* const char * text_to_draw: */
            "Let's work on particle FX",
        /* const float mid_x_pixelspace: */
            window_globals->window_width / 2,
        /* const float mid_y_pixelspace: */
            window_globals->window_height * 0.15f,
        /* const float pixelspace_x_offset_for_each_character: */
            50.0f,
        /* const float pixelspace_y_offset_for_each_character: */
            50.0f,
        /* const float z: */
            1.0f,
        /* const float max_width: */
            window_globals->window_width * 0.75f,
        /* const uint32_t ignore_camera: */
            false);

    ParticleEffect fountain;
    construct_particle_effect(&fountain);
    fountain.x = 0.0f;
    fountain.y = 0.0f;
    fountain.z = 0.75f;
    fountain.particle_height = screenspace_height_to_height(12, 1.0f);
    fountain.particle_width = screenspace_width_to_width(12, 1.0f);
    fountain.particle_lifespan = 2500000;
    fountain.particle_spawns_per_second = 200;

    fountain.particle_direction[0] = 0.0f;
    fountain.particle_direction[1] = 1.0f;
    fountain.particle_direction[2] = 0.0f;
    fountain.particle_direction_max_x_angle_variance = 0;
    fountain.particle_direction_max_y_angle_variance = 0;
    fountain.particle_direction_max_z_angle_variance = 60;
    fountain.particle_distance_per_second   =  0.3f;

    fountain.squared_direction[0] =  0.0f;
    fountain.squared_direction[1] = -1.0f;
    fountain.squared_direction[2] =  0.0f;
    fountain.squared_direction_max_x_angle_variance = 0;
    fountain.squared_direction_max_y_angle_variance = 0;
    fountain.squared_direction_max_z_angle_variance = 60;
    fountain.squared_distance_per_second    =  0.15f;

    fountain.particle_origin_max_x_variance = 3;
    fountain.particle_origin_max_y_variance = 3;
    fountain.particle_origin_max_z_variance = 10;

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
    fountain.particle_rgba_progression[6][3] = 1.0;

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
