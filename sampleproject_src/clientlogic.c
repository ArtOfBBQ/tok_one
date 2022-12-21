#include "clientlogic.h"

#define TEXTURE_FILENAMES_SIZE 6
DecodedImage * decoded_pngs[TEXTURE_FILENAMES_SIZE];

int32_t latest_object_id = 72;

typedef struct TextureArrayLocation {
    int32_t texturearray_i;
    int32_t texture_i;
} TextureArrayLocation;


static zPolygon card_model;
zPolygon load_from_obj_file_expecting_materials(
    char * filepath,
    ExpectedObjMaterials * expected_materials,
    const uint32_t expected_materials_size,
    const bool32_t flip_winding)
{
    log_append("loading obj file: ");
    log_append(filepath);
    log_append_char('\n');
    
    FileBuffer buffer;
    buffer.size = (uint64_t)platform_get_resource_size(filepath) + 1;
    buffer.contents = (char *)malloc_from_managed(buffer.size);
    platform_read_resource_file(
        filepath,
        &buffer);
    
    assert(buffer.size > 1);
    
    zPolygon return_value = parse_obj_expecting_materials(
        /* rawdata     : */ buffer.contents,
        /* rawdata_size: */ buffer.size,
        expected_materials,
        expected_materials_size,
        flip_winding);
    
    free_from_managed((uint8_t *)buffer.contents);
    
    return return_value;
}

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
    
    zlights_to_apply[0].deleted = false;
    zlights_to_apply[0].object_id = -1;
    zlights_to_apply[0].x = -1.0f;
    zlights_to_apply[0].y = -1.0f;
    zlights_to_apply[0].z = 2.0f;
    zlights_to_apply[0].RGBA[0] = 0.5f;
    zlights_to_apply[0].RGBA[1] = 0.5f;
    zlights_to_apply[0].RGBA[2] = 0.0f;
    zlights_to_apply[0].RGBA[3] = 1.0f;
    zlights_to_apply[0].reach = 4.0f;
    zlights_to_apply[0].ambient = 1.0f;
    zlights_to_apply[0].diffuse = 0.0f;
    zlights_to_apply_size++;
    log_assert(zlights_to_apply_size == 1);
    
    ExpectedObjMaterials key_materials;
    strcpy_capped(
        key_materials.material_name,
        16,
        "Metal");
    key_materials.texturearray_i = -1;
    key_materials.texture_i = -1;
    key_materials.rgba[0] = 0.6f;
    key_materials.rgba[1] = 0.6f;
    key_materials.rgba[2] = 0.25f;
    key_materials.rgba[3] = 1.0f;
    zPolygon key = load_from_obj_file_expecting_materials(
        "key.obj",
        &key_materials,
        1,
        false);
    
    key.object_id = 123;
    key.touchable_id = 123;
    log_assert(key.triangles_size > 0);
    scale_zpolygon(
        /* to_scale: */ &key,
        /* new_height: */ 0.5f);
    key.x = 0.5f;
    key.y = 0.5f;
    key.z = 1.0f;
    request_zpolygon_to_render(&key);
    
    zPolygon anotherkey = key;
    anotherkey.object_id = 124;
    anotherkey.touchable_id = 124;
    anotherkey.x = key.x - 0.3f;
    anotherkey.y = key.y - 0.3f;
    anotherkey.z = key.z - 0.3f;
    request_zpolygon_to_render(&anotherkey);
    
    ScheduledAnimation rotate_key;
    construct_scheduled_animation(&rotate_key);
    rotate_key.affected_object_id = 123;
    rotate_key.z_rotation_per_second = 0.2f;
    rotate_key.y_rotation_per_second = 0.5f;
    rotate_key.x_rotation_per_second = 0.9f;
    rotate_key.delta_z_per_second = 0.05f;
    rotate_key.duration_microseconds = 1000000;
    rotate_key.runs = 0;
    request_scheduled_animation(&rotate_key);
    
    TexQuad texture;
    construct_texquad(&texture);
    texture.RGBA[0] = 0.3f;
    texture.RGBA[1] = 1.0f;
    texture.RGBA[2] = 0.3f;
    texture.RGBA[3] = 1.0f;
    texture.object_id = 125;
    texture.touchable_id = 125;
    texture.top_y = 0.2f;
    texture.left_x = 0.2f;
    texture.width = 0.2f;
    texture.height = 0.2f;
    texture.ignore_camera = true;
    request_texquad_renderable(&texture);
    
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
        if (previous_touch_or_leftclick_end.touchable_id >= 0) {
            ScheduledAnimation bump;
            construct_scheduled_animation(&bump);
            bump.affected_object_id = previous_touch_or_leftclick_end.touchable_id;
            bump.final_scale_known = true;
            bump.final_scale = 1.2f;
            bump.duration_microseconds = 100000;
            request_scheduled_animation(&bump);
            
            ScheduledAnimation unbump;
            construct_scheduled_animation(&unbump);
            unbump.affected_object_id = previous_touch_or_leftclick_end.touchable_id;
            unbump.final_scale_known = true;
            unbump.final_scale = 1.0f;
            unbump.remaining_wait_before_next_run  = 100000;
            unbump.duration_microseconds = 100000;
            request_scheduled_animation(&unbump);
        }
        previous_touch_or_leftclick_end.handled = true;
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
