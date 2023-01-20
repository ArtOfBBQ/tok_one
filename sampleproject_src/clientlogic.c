#include "clientlogic.h"

#define TEXTURE_FILENAMES_SIZE 6
DecodedImage * decoded_pngs[TEXTURE_FILENAMES_SIZE];

int32_t latest_object_id = 72;

typedef struct TextureArrayLocation {
    int32_t texturearray_i;
    int32_t texture_i;
} TextureArrayLocation;

zPolygon load_from_obj_file(char * filepath, const bool32_t flip_winding) {
    FileBuffer buffer;
    buffer.size = (uint64_t)platform_get_resource_size(filepath) + 1;
    buffer.contents = (char *)malloc_from_managed(buffer.size);
    platform_read_resource_file(
        filepath,
        &buffer);
    
    assert(buffer.size > 1);
    
    zPolygon return_value = parse_obj(
        /* rawdata     : */ buffer.contents,
        /* rawdata_size: */ buffer.size,
        flip_winding);
    
    free_from_managed((uint8_t *)buffer.contents);
    
    return return_value;
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

static bool32_t ran_anim[4]; // 1 for each card in the test scene
void client_logic_startup() {
    
    font_height = 40.0f;
    font_color[0] = 1.0f;
    font_color[1] = 1.0f;
    font_color[2] = 1.0f;
    font_color[3] = 1.0f;
    request_label_around(
        /* const int32_t with_id: */
            9999,
        /* const char * text_to_draw: */
            "Press space to shoot a light\nClick a teapot to bump it\npress T to toggle debug\npress C to reset camera",
        /* const float mid_x_pixelspace: */
            window_width * 0.5f,
        /* const float top_y_pixelspace: */
            window_height * 0.75f,
        /* const float z: */
            1.0f,
        /* const float max_width: */
            1000.0f,
        /* const bool32_t ignore_camera: */
            true);
    
    ran_anim[0] = false;
    ran_anim[1] = false;
    ran_anim[2] = false;
    ran_anim[3] = false;
    
    init_rand_with_seed(platform_get_current_time_microsecs());
    
    const char * fontfile;
    fontfile = "font.png";
    register_new_texturearray_by_splitting_file(
        /* filename : */ fontfile,
        /* rows     : */ 10,
        /* columns  : */ 10);
    
    char * filenames[2] = {
        (char *)"structuredart1.png",
        (char *)"structuredart2.png",
    };
    register_new_texturearray_from_files(
        (const char **)filenames,
        2);
    
    char * moar_filenames[1] = {
        (char *)"roundedborder.png",
    };
    register_new_texturearray_from_files(
        (const char **)moar_filenames,
        1);
    
    char * card_filenames[2] = {
        (char *)"fs_justice.bmp",
        (char *)"fs_cardback.bmp"
    };
    register_new_texturearray_from_files(
        (const char **)card_filenames,
        2);
    
    zPolygon teapot = load_from_obj_file("teapot.obj", false);
    center_zpolygon_offsets(&teapot);
    scale_zpolygon(&teapot, 1.0f);
    
    teapot.object_id = 123;
    teapot.touchable_id = 1;
    log_assert(teapot.triangles_size > 0);
    scale_zpolygon(
        /* to_scale: */ &teapot,
        /* new_height: */ 0.15f);
    teapot.x = -0.55f;
    teapot.y = -0.3f;
    teapot.z = 0.7f;
    
    for (uint32_t _ = 0; _ < 20; _++) {
        zPolygon anotherteapot = teapot;
        anotherteapot.object_id = 124 + _;
        anotherteapot.touchable_id = 1 + _;
        anotherteapot.x = teapot.x + ((_ / 5) * 0.4f);
        anotherteapot.y = teapot.y;
        anotherteapot.z = teapot.z + ((_ % 5) * 0.3f);
        anotherteapot.x_angle = (_ * 0.48f);
        anotherteapot.y_angle = (_ * 0.35f);
        anotherteapot.z_angle = (_ * 0.61f);
        
        for (
            uint32_t tri_i = 0;
            tri_i < anotherteapot.triangles_size;
            tri_i++)
        {
            anotherteapot.triangles[tri_i].color[0] = 0.4f;
            anotherteapot.triangles[tri_i].color[1] = 0.4f;
            anotherteapot.triangles[tri_i].color[2] = 0.6f;
        }
        
        request_zpolygon_to_render(&anotherteapot);
        
        ScheduledAnimation rotate_teapot;
        construct_scheduled_animation(&rotate_teapot);
        rotate_teapot.affected_object_id    =   124 + _;
        rotate_teapot.x_rotation_per_second =  0.4f + (1.8f / (_ + 1));
        rotate_teapot.y_rotation_per_second = -0.6f + (0.1f  * _);
        rotate_teapot.z_rotation_per_second =  0.8f + (0.08f * _);
        rotate_teapot.duration_microseconds = 10000000;
        rotate_teapot.runs = 0;
        request_scheduled_animation(&rotate_teapot);
    }
    
    #define NUM_LIGHTS 4
    float light_size = 0.05f;
    float light_xs[NUM_LIGHTS + 3];
    light_xs[0] = -0.3f;
    light_xs[1] = -0.35f;
    light_xs[2] = 0.3f;
    light_xs[3] = 0.35f;
    float light_ys[NUM_LIGHTS + 3];
    light_ys[0] = 0.2f;
    light_ys[1] = 0.21f;
    light_ys[2] = 0.5f;
    light_ys[3] = 0.51f;
    float light_zs[NUM_LIGHTS + 3];
    light_zs[0] = 1.0f;
    light_zs[1] = 0.7f;
    light_zs[2] = 1.3f;
    light_zs[3] = 0.4f;
    
    for (uint32_t i = 0; i < NUM_LIGHTS; i++) {
        zlights_to_apply[i].deleted      =    false;
        zlights_to_apply[i].object_id    =    123 + 250 + i;
        zlights_to_apply[i].x            =    light_xs[i];
        zlights_to_apply[i].y            =    light_ys[i];
        zlights_to_apply[i].z            =    light_zs[i];
        zlights_to_apply[i].RGBA[0]      =    i % 3 == 0 ? 1.0f : 0.0f; 
        zlights_to_apply[i].RGBA[1]      =    i % 2 == 0 ? 1.0f : 0.0f;
        zlights_to_apply[i].RGBA[2]      =    i % 3 == 1 ? 1.0f : 0.0f;
        zlights_to_apply[i].RGBA[3]      =    1.0f;
        zlights_to_apply[i].reach        =    1.0f;
        zlights_to_apply[i].ambient      =    0.1f;
        zlights_to_apply[i].diffuse      =    1.0f;
        zlights_to_apply_size++;
        
        zPolygon quad = construct_quad(
            /* float left_x: */
                zlights_to_apply[i].x - (light_size / 2),
            /* float top_y: */
                zlights_to_apply[i].y - (light_size / 2),
            /* z: */
                light_zs[i],
            /* float width: */
                light_size,
            /* float height: */
                light_size);
        quad.object_id                   = 123 + 250 + i;
        quad.touchable_id                = 250 + i;
        quad.ignore_lighting             = true;
        quad.ignore_camera               = i == 0;
        quad.triangles[0].texturearray_i = 1;
        quad.triangles[0].texture_i      = 1;
        quad.triangles[1].texturearray_i = 1;
        quad.triangles[1].texture_i      = 1;
        quad.x_angle                     = i * 0.3f;
        request_zpolygon_to_render(&quad);
    }
    
    log_append("finished client_logic_startup()\n");
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
    strcpy_capped(unhandled_callback_id, 256, "unhandled client_logic_animation_callback: ");
    strcat_int_capped(unhandled_callback_id, 256, callback_id);
    strcat_capped(unhandled_callback_id, 256, ". You should handle it in clientlogic.c -> client_logic_animation_callback\n");
    log_append(unhandled_callback_id);
    log_dump_and_crash(unhandled_callback_id);
}

static void  client_handle_touches_and_leftclicks(
    uint64_t microseconds_elapsed)
{
    if (!previous_touch_or_leftclick_end.handled) {
        int32_t leftclick_touchable_id =
            previous_touch_or_leftclick_end.touchable_id;
        previous_touch_or_leftclick_end.handled = true;
        
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

static void client_handle_keypresses(uint64_t microseconds_elapsed) {
    float elapsed_mod =
        (float)((double)microseconds_elapsed / (double)16666);
    float cam_speed = 0.1f * elapsed_mod;
    float cam_rotation_speed = 0.05f * elapsed_mod;
    
    if (keypress_map[123] == true)
    {
        // left arrow key
        camera.x -= cam_speed;
    }
    
    if (keypress_map[124] == true)
    {
        // right arrow key
        camera.x += cam_speed;
    }
    
    if (keypress_map[125] == true)
    {
        // down arrow key
        camera.y -= cam_speed;
    }
    
    if (keypress_map[126] == true)
    {
        // up arrow key is pressed
        camera.y += cam_speed;
    }
     
    if (keypress_map[0] == true) {                                              
        // 'A' key is pressed                                                   
        camera.x_angle += cam_rotation_speed;                                   
    }
    
    if (keypress_map[6] == true) {                                              
        // 'Z' key is pressed                                                   
        camera.z_angle -= cam_rotation_speed;                                   
    }
    
    if (keypress_map[7] == true) {                                              
        // 'X' key                                                               
        camera.z_angle += cam_rotation_speed;                                    
    }

    if (keypress_map[8] == true) {                                             
        // C key is pressed, reset camera
        camera.x = 0.0f;
        camera.y = 0.0f;
        camera.z = 0.0f;
        camera.x_angle = 0.0f;
        camera.y_angle = 0.0f;
        camera.z_angle = 0.0f;
    }
    
    if (keypress_map[12] == true) {                                             
        // 'Q' key is pressed                                                   
        camera.x_angle -= cam_rotation_speed;                                   
    }
    
    if (keypress_map[13] == true) {                                             
        // 'W' key is pressed                                                   
        camera.y_angle -= cam_rotation_speed;                              
    }
    
    if (keypress_map[1] == true) {                                             
        // 'S' key is pressed                                                   
        camera.y_angle += cam_rotation_speed;                                   
    }
    
    if (keypress_map[44] == true) {                                             
        // / key                                                                
        camera.z -= 0.01f;                                                      
    }
    
    if (keypress_map[94] == true) {                                             
        // _ key is pressed                                                     
        camera.z += 0.01f;                                                      
    }
    
    if (keypress_map[17] == true) {                                             
        // T key is pressed
        keypress_map[17] = false;
        visual_debug_mode = !visual_debug_mode;
    }
    
    if (keypress_map[49] == true) {                                             
        // spacebar key is pressed
        keypress_map[49] = false;
        float bullet_size = 0.1f;     
        
        zVertex camera_direction;
        camera_direction.x = 0.0f;
        camera_direction.y = 0.0f;
        camera_direction.z = 1.0f;
        
        camera_direction = x_rotate_zvertex(&camera_direction, camera.x_angle);
        camera_direction = y_rotate_zvertex(&camera_direction, camera.y_angle);
        
        zPolygon bullet = construct_quad(                                            
            /* left: */
                camera.x - (bullet_size / 2),                                   
            /* top:  */                                   
                camera.y - (bullet_size / 2),      
            /* z:    */
                1.0f,
            /* width: */
                bullet_size,                                                    
            /* height: */
                bullet_size);
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
    // request_fps_counter(microseconds_elapsed);
    
    client_handle_touches_and_leftclicks(microseconds_elapsed);
    client_handle_keypresses(microseconds_elapsed);  
}

void client_logic_window_resize(
    const uint32_t new_height,
    const uint32_t new_width)
{
    char unhandled_rs_msg[256];
    strcpy_capped(unhandled_rs_msg, 256, "Error: unhandled client_logic_window_resize() to height/width: of ");
    strcat_uint_capped(unhandled_rs_msg, 256, new_height);
    strcat_capped(unhandled_rs_msg, 256, ", ");
    strcat_uint_capped(unhandled_rs_msg, 256, new_width);
    strcat_capped(unhandled_rs_msg, 256, ".\nEither prevent app resizing or handle in clientlogic.c\n");
    log_append(unhandled_rs_msg);
    log_dump_and_crash(unhandled_rs_msg);
}

void client_logic_shutdown(void) {
    // Your application shutdown code goes here!
}
