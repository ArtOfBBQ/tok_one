#include "clientlogic.h"

#define TEXTURE_FILENAMES_SIZE 6
DecodedImage * decoded_pngs[TEXTURE_FILENAMES_SIZE];

int32_t latest_object_id = 72;

typedef struct TextureArrayLocation {
    int32_t texturearray_i;
    int32_t texture_i;
} TextureArrayLocation;

void load_from_obj_file(
    char * filepath,
    const bool32_t flip_winding,
    zPolygon * recipient)
{
    FileBuffer buffer;
    buffer.size = (uint64_t)platform_get_resource_size(filepath) + 1;
    buffer.contents = (char *)malloc_from_managed(buffer.size);
    platform_read_resource_file(
        filepath,
        &buffer);
    
    assert(buffer.size > 1);
    
    parse_obj(
        /* rawdata     : */ buffer.contents,
        /* rawdata_size: */ buffer.size,
        flip_winding,
        recipient);
    
    free_from_managed((uint8_t *)buffer.contents);    
}

void client_logic_get_application_name_to_recipient(
    char * recipient,
    const uint32_t recipient_size)
{
    char * app_name = (char *)"TOK ONE";
    
    strcpy_capped(
        /* recipient: */ recipient,
        /* recipient_size: */ recipient_size,
        /* origin: */ app_name);
}

void client_logic_startup(void) {
    
    const char * fontfile;
        fontfile = "font.png";
        register_new_texturearray_by_splitting_file(
        /* filename : */ fontfile,
        /* rows     : */ 10,
        /* columns  : */ 10);
    
    char * filenames[1] = {
       (char *)"structuredart1.png",
    };
    register_new_texturearray_from_files(
        (const char **)filenames,
        1);
    
    for (uint32_t uint_z = 0; uint_z < 50; uint_z++) {
        
        float z = 0.01f + (0.25f * uint_z);
        
        zPolygon quad;
        construct_quad_around(
            /* const float mid_x: */
                screenspace_x_to_x(300, z),
            /* const float mid_y: */
                screenspace_y_to_y(300, z),
            /* const float z: */
                z,
            /* const float width: */
                0.1f,
            /* const float height: */
                0.1f,
            /* zPolygon * recipient: */
                &quad);
        quad.triangles[0].texturearray_i = 1;
        quad.triangles[0].texture_i = 0;
        quad.triangles[1].texturearray_i = 1;
        quad.triangles[1].texture_i = 0;
        quad.triangles[0].color[0] = z * 0.2f;
        quad.triangles[0].color[1] = z * 0.2f;
        quad.triangles[1].color[0] = z * 0.2f;
        quad.triangles[1].color[1] = z * 0.2f;
        quad.ignore_lighting = true;
        request_zpolygon_to_render(&quad);
    }
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
    if (!user_interactions[INTR_PREVIOUS_TOUCH_OR_LEFTCLICK_END].handled) {
        int32_t leftclick_touchable_id =
            user_interactions[INTR_PREVIOUS_TOUCH_OR_LEFTCLICK_END].
                touchable_id;
        user_interactions[INTR_PREVIOUS_TOUCH_OR_LEFTCLICK_END].handled = true;
        
        if (leftclick_touchable_id >= 0) {
            log_append("leftclick_touchable_id: ");
            log_append_int(leftclick_touchable_id);
            log_append_char('\n');
            
            ScheduledAnimation bump;
            construct_scheduled_animation(&bump);
            int32_t o_id = leftclick_touchable_id + 123;
            bump.affected_object_id    = o_id;
            bump.final_scale_known     = true;
            bump.final_scale           = 1.2f;
            bump.duration_microseconds = 50000;
            
            ScheduledAnimation unbump;
            construct_scheduled_animation(&unbump);
            unbump.affected_object_id             =   o_id;
            unbump.final_scale_known              =   true;
            unbump.final_scale                    =   1.0f;
            unbump.duration_microseconds          = 150000;
            unbump.remaining_wait_before_next_run =  50000;
            
            request_scheduled_animation(&bump);
            request_scheduled_animation(&unbump);
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
    
    if (keypress_map[TOK_KEY_C] == true) {                                             
        camera.x = 0.0f;
        camera.y = 0.0f;
        camera.z = 0.0f;
        camera.x_angle = 0.0f;
        camera.y_angle = 0.0f;
        camera.z_angle = 0.0f;
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
        // T key is pressed
        keypress_map[TOK_KEY_T] = false;
        window_globals->visual_debug_mode =
            !window_globals->visual_debug_mode;
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
        float r_color = ((float)(tok_rand() % 100)) / 100.0f;
        float g_color = ((float)(tok_rand() % 100)) / 100.0f;
        float b_color = ((float)(tok_rand() % 100)) / 100.0f;
        for (uint32_t tri_i = 0; tri_i < bullet.triangles_size; tri_i++) {
            bullet.triangles[tri_i].color[0] = r_color;
            bullet.triangles[tri_i].color[1] = g_color;
            bullet.triangles[tri_i].color[2] = b_color;
            bullet.triangles[tri_i].color[3] = 1.0f;
        }
        bullet.ignore_lighting = true;
        
        zLightSource bullet_light;
        bullet_light.object_id = bullet.object_id;
        bullet_light.RGBA[0] = bullet.triangles[0].color[0];
        bullet_light.RGBA[1] = bullet.triangles[0].color[1];
        bullet_light.RGBA[2] = bullet.triangles[0].color[2];
        bullet_light.RGBA[3] = 1.0f;
        bullet_light.reach = 1.5f;
        bullet_light.ambient = 0.2f;
        bullet_light.diffuse = 2.0f;
        bullet_light.x = bullet.x;
        bullet_light.y = bullet.y;
        bullet_light.z = bullet.z;
        bullet_light.deleted = false;
        
        ScheduledAnimation move_bullet;
        construct_scheduled_animation(&move_bullet);
        move_bullet.affected_object_id = bullet.object_id;
        move_bullet.delta_x_per_second = camera_direction.x;
        move_bullet.delta_y_per_second = camera_direction.y;
        move_bullet.delta_z_per_second = camera_direction.z * 3.0f;
        move_bullet.delete_object_when_finished = true;
        move_bullet.duration_microseconds = 2500000;
        move_bullet.runs = 1;
        
        request_zpolygon_to_render(&bullet);
        request_zlightsource(&bullet_light);
        request_scheduled_animation(&move_bullet);
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
