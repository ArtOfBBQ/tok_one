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
    teapot = teapot;
    
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
        for (uint32_t tri_i = 0; tri_i < anotherteapot.triangles_size; tri_i++) {
            anotherteapot.triangles[tri_i].color[0] = 0.4f;
            anotherteapot.triangles[tri_i].color[1] = 0.4f;
            anotherteapot.triangles[tri_i].color[2] = 0.6f;
        }
        
        request_zpolygon_to_render(&anotherteapot);
        
        ScheduledAnimation rotate_teapot;
        construct_scheduled_animation(&rotate_teapot);
        rotate_teapot.affected_object_id    = 124 + _;
        rotate_teapot.x_rotation_per_second = 0.2f + (0.9f / (_ + 1));
        rotate_teapot.y_rotation_per_second = -0.3f + (0.05f * _);
        rotate_teapot.z_rotation_per_second = 0.4f + (0.04f * _);
        rotate_teapot.duration_microseconds = 10000000;
        rotate_teapot.runs = 0;
        request_scheduled_animation(&rotate_teapot);
    }
    
    #define NUM_LIGHTS 4
    float light_size = 0.05f;
    float light_xs[NUM_LIGHTS];
    light_xs[0] = -0.3f;
    light_xs[1] = -0.3f;
    light_xs[2] = 0.3f;
    light_xs[3] = 0.3f;
    float light_zs[NUM_LIGHTS];
    light_zs[0] = 1.0f;
    light_zs[1] = 0.7f;
    light_zs[2] = 1.0f;
    light_zs[3] = 0.7f;
    for (uint32_t i = 0; i < NUM_LIGHTS; i++) {
        
        zlights_to_apply[i].deleted    =   false;
        zlights_to_apply[i].object_id  =      -1;
        zlights_to_apply[i].x          =    light_xs[i];
        zlights_to_apply[i].y          =    0.2f;
        zlights_to_apply[i].z          =    light_zs[i];
        zlights_to_apply[i].RGBA[0]    =    i % 3 == 0 ? 1.0f : 0.0f; 
        zlights_to_apply[i].RGBA[1]    =    i % 2 == 0 ? 1.0f : 0.0f;
        zlights_to_apply[i].RGBA[2]    =    i % 3 == 1 ? 1.0f : 0.0f;
        zlights_to_apply[i].RGBA[3]    =    1.0f;
        zlights_to_apply[i].reach      =    1.0f;
        zlights_to_apply[i].ambient    =    0.1f;
        zlights_to_apply[i].diffuse    =    1.0f;
        zlights_to_apply_size++;
        
        TexQuad texture;
        construct_texquad(&texture);
        texture.RGBA[0]       = zlights_to_apply[i].RGBA[0] * 5.0f;
        texture.RGBA[1]       = zlights_to_apply[i].RGBA[1] * 5.0f;
        texture.RGBA[2]       = zlights_to_apply[i].RGBA[2] * 5.0f;
        texture.RGBA[3]       = 1.0f;
        texture.object_id     = 123;
        texture.touchable_id  = 123;
        texture.top_y         = zlights_to_apply[i].y - (light_size / 2);
        texture.left_x        = zlights_to_apply[i].x - (light_size / 2);
        texture.z             = zlights_to_apply[i].z;
        texture.width         = light_size;
        texture.height        = light_size;
        texture.z_angle       = 0.0f;
        texture.ignore_camera = false;
        request_texquad_renderable(&texture);
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
        
        // let's make a visualization of the ray we're using to check what
        // we're clicking (because I'm having trouble understanding ray-triangle
        // intersection)
        zVertex ray_origin;
        ray_origin.x = camera.x -1.0f +
            ((previous_touch_or_leftclick_end.screen_x / window_width) * 2.0f);
        ray_origin.y = camera.y -1.0f +
            ((previous_touch_or_leftclick_end.screen_y / window_height) * 2.0f);
        ray_origin.z = camera.z;
        
        zVertex ray_direction;
        ray_direction.x = 0.0f;
        ray_direction.y = 0.0f;
        ray_direction.z = 1.0f;
        ray_direction = x_rotate_zvertex(&ray_direction, camera.x_angle);
        ray_direction = y_rotate_zvertex(&ray_direction, camera.y_angle);
        normalize_zvertex(&ray_direction);
        
        uint32_t ray_obj_id = (tok_rand() % 2000) + 500;
        float ray_size = 0.02f;
        
        for (float t = 0.01f; t < 10.0f; t += 0.05f) {
            TexQuad point_in_ray;
            construct_texquad(&point_in_ray);
            point_in_ray.object_id = ray_obj_id;
            point_in_ray.left_x = ray_origin.x + (ray_direction.x * t);
            point_in_ray.top_y = ray_origin.y + (ray_direction.y * t);
            point_in_ray.z = ray_origin.z + (ray_direction.z * t);
            point_in_ray.width = ray_size;
            point_in_ray.height = ray_size;
            point_in_ray.RGBA[0] = 1.0f - (t / 10.0f);
            point_in_ray.RGBA[1] = t / 10.0f;
            point_in_ray.RGBA[2] = t / 10.0f;
            point_in_ray.RGBA[3] = 1.0f;
            point_in_ray.ignore_lighting = true;
            request_texquad_renderable(&point_in_ray);
        }
        
        ScheduledAnimation fade_ray;
        construct_scheduled_animation(&fade_ray);
        fade_ray.affected_object_id = ray_obj_id;
        fade_ray.duration_microseconds = 1500000;
        fade_ray.final_rgba_known[0] = true;
        fade_ray.final_rgba[0] = 0.0f;
        fade_ray.final_rgba_known[1] = true;
        fade_ray.final_rgba[1] = 0.0f;
        fade_ray.final_rgba_known[2] = true;
        fade_ray.final_rgba[2] = 0.0f;
        fade_ray.delete_object_when_finished = true;
        request_scheduled_animation(&fade_ray);
        
        zVertex collision_point;
        
        for (
            uint32_t zp_i = 0;
            zp_i < zpolygons_to_render_size;
            zp_i++)
        {
            if (
                ray_intersects_zpolygon(
                    /* const zVertex * ray_origin: */
                        &ray_origin,
                    /* const zVertex * ray_direction: */
                        &ray_direction,
                    /* const zPolygon * mesh: */
                        &zpolygons_to_render[zp_i],
                    /* zVertex * recipient_hit_point: */
                        &collision_point))
            {
                ScheduledAnimation bump;
                construct_scheduled_animation(&bump);
                bump.affected_object_id = zpolygons_to_render[zp_i].object_id;
                bump.final_scale_known = true;
                bump.final_scale = 1.3f;
                bump.duration_microseconds = 150000;
                
                ScheduledAnimation unbump;
                construct_scheduled_animation(&unbump);
                unbump.affected_object_id = bump.affected_object_id;
                unbump.final_scale_known = true;
                unbump.final_scale = 1.0f;
                unbump.remaining_wait_before_next_run  = 200000;
                unbump.duration_microseconds = 150000;
                
                request_scheduled_animation(&bump);
                request_scheduled_animation(&unbump);
            }   
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
    
    if (keypress_map[12] == true) {                                             
        // 'Q' key is pressed                                                   
        camera.x_angle -= cam_rotation_speed;                                   
    }
    
    if (keypress_map[44] == true) {                                             
        // / key                                                                
        camera.z -= 0.01f;                                                      
    }
    
    if (keypress_map[94] == true) {                                             
        // _ key is pressed                                                     
        camera.z += 0.01f;                                                      
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
        
        TexQuad bullet;
        construct_texquad(&bullet);
        bullet.object_id = 54321 + (tok_rand() % 1000);
        bullet.left_x = camera.x - (bullet_size / 2);                                                      
        bullet.top_y = camera.y - (bullet_size / 2);
        bullet.width = bullet_size;
        bullet.height = bullet_size;
        bullet.z = camera.z;
        bullet.RGBA[0] = ((float)(tok_rand() % 100)) / 100.0f;
        bullet.RGBA[1] = ((float)(tok_rand() % 100)) / 100.0f;
        bullet.RGBA[2] = ((float)(tok_rand() % 100)) / 100.0f;
        bullet.RGBA[3] = 1.0f;
        
        zLightSource bullet_light;
        bullet_light.object_id = bullet.object_id;
        bullet_light.RGBA[0] = bullet.RGBA[0];
        bullet_light.RGBA[1] = bullet.RGBA[1];
        bullet_light.RGBA[2] = bullet.RGBA[2];
        bullet_light.RGBA[3] = 1.0f;
        bullet_light.reach = 1.5f;
        bullet_light.ambient = 0.2f;
        bullet_light.diffuse = 2.0f;
        bullet_light.x = bullet.left_x + (bullet_size * 0.5f);
        bullet_light.y = bullet.top_y + (bullet_size * 0.5f);
        bullet_light.z = bullet.z;
        bullet_light.deleted = false;
        
        ScheduledAnimation move_bullet;
        construct_scheduled_animation(&move_bullet);
        move_bullet.affected_object_id = bullet.object_id;
        move_bullet.delta_x_per_second = camera_direction.x;
        move_bullet.delta_y_per_second = camera_direction.y;
        move_bullet.delta_z_per_second = camera_direction.z;
        move_bullet.delete_object_when_finished = true;
        move_bullet.duration_microseconds = 5000000;
        move_bullet.runs = 1;
        
        request_texquad_renderable(&bullet);
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
