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
    
    uint32_t expected_materials_size = 3; 
    ExpectedObjMaterials * expected_materials = (ExpectedObjMaterials *)
        malloc_from_unmanaged(
            sizeof(ExpectedObjMaterials) * expected_materials_size);
    strcpy_capped(
        expected_materials[0].material_name,
        16,
        "Frontface");
    expected_materials[0].texturearray_i = 3;
    expected_materials[0].texture_i = 0;
    expected_materials[0].rgba[0] = 1.0f;
    expected_materials[0].rgba[1] = 1.0f;
    expected_materials[0].rgba[2] = 1.0f;
    expected_materials[0].rgba[3] = 1.0f;
    
    strcpy_capped(
        expected_materials[1].material_name,
        16,
        "Backface");
    expected_materials[1].texturearray_i = 3;
    expected_materials[1].texture_i = 1;
    expected_materials[1].rgba[0] = 0.0f;
    expected_materials[1].rgba[1] = 1.0f;
    expected_materials[1].rgba[2] = 1.0f;
    expected_materials[1].rgba[3] = 1.0f;
    
    strcpy_capped(
        expected_materials[2].material_name,
        16,
        "Sideface");
    expected_materials[2].texturearray_i = -1;
    expected_materials[2].texture_i = -1;
    expected_materials[2].rgba[0] = 1.0f;
    expected_materials[2].rgba[1] = 1.0f;
    expected_materials[2].rgba[2] = 1.0f;
    expected_materials[2].rgba[3] = 1.0f;
    
    card_model = load_from_obj_file_expecting_materials(
        "cardwithuvcoords.obj",
        expected_materials,
        expected_materials_size,
        false);
    scale_zpolygon(
        /* to_scale: */ &card_model,
        /* new_height: */ 0.5f);
    center_zpolygon_offsets(&card_model);
    
    zPolygon card_1 = card_model;
    card_1.object_id = 234;
    card_1.touchable_id = 0;
    card_1.x = 0.25f;
    card_1.y = 0.25f;
    card_1.z = 1.0f;
    card_1.x_angle = 3.18f;
    card_1.y_angle = 3.2f;
    card_1.z_angle = 0.0f;
    zpolygons_to_render[zpolygons_to_render_size++] = card_1;
    
    zPolygon card_2 = card_1;
    card_2.object_id = 235;
    card_2.x = 0.3f;
    card_2.y = 0.75f;
    card_2.touchable_id = 1;
    //    zpolygons_to_render[zpolygons_to_render_size++] = card_2;
    
    TexQuad purplewheeltexture;
    construct_texquad(&purplewheeltexture);
    purplewheeltexture.object_id = 12321;
    purplewheeltexture.left_x = screenspace_x_to_x((window_width / 2) - 50);
    purplewheeltexture.top_y = screenspace_y_to_y((window_height / 2) + 50);
    purplewheeltexture.z = 1.0f;
    purplewheeltexture.width = screenspace_width_to_width(100);
    purplewheeltexture.height = screenspace_height_to_height(100);;
    purplewheeltexture.texturearray_i = 2;
    purplewheeltexture.texture_i = 0;
    purplewheeltexture.touchable_id = 12345;
    purplewheeltexture.ignore_camera = true;
    //    request_texquad_renderable(&purplewheeltexture);
    
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

static uint32_t cur_color_i = 0;
static int32_t cur_texture_i = 0;
static void request_fading_lightsquare(
    const float location_x,
    const float location_y)
{
    TexQuad touch_highlight;
    construct_texquad(&touch_highlight);
    touch_highlight.object_id = latest_object_id++;
    touch_highlight.left_x  = screenspace_x_to_x(location_x);
    touch_highlight.top_y   = screenspace_y_to_y(location_y);
    touch_highlight.z       = 1.0f;
    touch_highlight.height  = screenspace_height_to_height(30.0f);
    touch_highlight.width   = screenspace_width_to_width(30.0f);
    touch_highlight.RGBA[0] = 1.0f;
    touch_highlight.RGBA[0] = 0.0f;
    touch_highlight.RGBA[0] = 1.0f;
    touch_highlight.RGBA[0] = 1.0f;
    touch_highlight.texturearray_i = 0;
    touch_highlight.texture_i      = cur_texture_i++;
    if (cur_texture_i > 92) { cur_texture_i = 0; }
    request_texquad_renderable(&touch_highlight);
    
    uint32_t new_zlight_i;
    for (
        new_zlight_i = 0;
        new_zlight_i <= zlights_to_apply_size;
        new_zlight_i++)
    {
        if (zlights_to_apply[new_zlight_i].deleted
            || new_zlight_i == zlights_to_apply_size)
        {
            break;
        }
    }
    
    zlights_to_apply[new_zlight_i].deleted = false;
    zlights_to_apply[new_zlight_i].object_id =
        touch_highlight.object_id;
    zlights_to_apply[new_zlight_i].x = screenspace_x_to_x(location_x);
    zlights_to_apply[new_zlight_i].y = screenspace_y_to_y(location_y);
    zlights_to_apply[new_zlight_i].z = 1.0f;
    
    if (cur_color_i == 0) {
        zlights_to_apply[new_zlight_i].RGBA[0] = 1.0f;
        zlights_to_apply[new_zlight_i].RGBA[1] = 0.0f;
        zlights_to_apply[new_zlight_i].RGBA[2] = 0.0f;
        zlights_to_apply[new_zlight_i].RGBA[3] = 1.0f;
    } else if (cur_color_i == 1) {
        zlights_to_apply[new_zlight_i].RGBA[0] = 0.0f;
        zlights_to_apply[new_zlight_i].RGBA[1] = 1.0f;
        zlights_to_apply[new_zlight_i].RGBA[2] = 0.0f;
        zlights_to_apply[new_zlight_i].RGBA[3] = 1.0f;
    } else if (cur_color_i == 2) {
        zlights_to_apply[new_zlight_i].RGBA[0] = 0.0f;
        zlights_to_apply[new_zlight_i].RGBA[1] = 0.0f;
        zlights_to_apply[new_zlight_i].RGBA[2] = 1.0f;
        zlights_to_apply[new_zlight_i].RGBA[3] = 1.0f;
    } else {
        zlights_to_apply[new_zlight_i].RGBA[0] =
            0.1f * (float)(tok_rand() % 10);
        zlights_to_apply[new_zlight_i].RGBA[1] =
            0.1f * (float)(tok_rand() % 10);
        zlights_to_apply[new_zlight_i].RGBA[2] =
            0.1f * (float)(tok_rand() % 10);
        zlights_to_apply[new_zlight_i].RGBA[3] = 1.0f;
    }
    log_assert(zlights_to_apply[new_zlight_i].RGBA[0] >= 0.0f);
    log_assert(zlights_to_apply[new_zlight_i].RGBA[1] >= 0.0f);
    log_assert(zlights_to_apply[new_zlight_i].RGBA[2] >= 0.0f);
    log_assert(zlights_to_apply[new_zlight_i].RGBA[3] >= 0.0f);
    
    cur_color_i += 1;
    if (cur_color_i > 2) { cur_color_i = 0; }
    
    zlights_to_apply[new_zlight_i].ambient = 0.0f;
    zlights_to_apply[new_zlight_i].diffuse = 1.0f;
    zlights_to_apply[new_zlight_i].reach = 10.0f;
    if (new_zlight_i == zlights_to_apply_size) {
         zlights_to_apply_size += 1;
    }
    
    ScheduledAnimation moveupandright;
    construct_scheduled_animation(&moveupandright);
    moveupandright.affected_object_id = touch_highlight.object_id;
    moveupandright.duration_microseconds = 6000000;
    moveupandright.delta_x_per_second = 0.3f;
    moveupandright.delta_y_per_second = 0.3f;
    request_scheduled_animation(&moveupandright);
    
    ScheduledAnimation vanish;
    construct_scheduled_animation(&vanish);
    vanish.affected_object_id = touch_highlight.object_id;
    vanish.remaining_wait_before_next_run = 4000000;
    vanish.duration_microseconds = 4000000;
    vanish.delete_object_when_finished = true;
    request_scheduled_animation(&vanish);
}

static void  client_handle_touches_and_leftclicks(
    uint64_t microseconds_elapsed)
{
    if (!previous_touch_or_leftclick_end.handled) {
        int32_t touch_id = previous_touch_or_leftclick_end.touchable_id;
        
        log_append("left touch at: ");
        log_append_float(previous_touch_or_leftclick_end.screen_x);
        log_append_char('/');
        log_append_float(previous_touch_or_leftclick_end.screen_y);
        log_append_char('\n');
        
        if (touch_id == 12345) {
            ScheduledAnimation bump_purple_wheel;
            construct_scheduled_animation(&bump_purple_wheel);
            bump_purple_wheel.affected_object_id = 12321;
            bump_purple_wheel.final_scale_known = true;
            bump_purple_wheel.final_scale = 1.25f;
            bump_purple_wheel.duration_microseconds = 100000;
            request_scheduled_animation(&bump_purple_wheel);
            
            ScheduledAnimation return_pw;
            construct_scheduled_animation(&return_pw);
            return_pw.affected_object_id = 12321;
            return_pw.final_scale_known = true;
            return_pw.final_scale = 1.0f;
            return_pw.remaining_wait_before_next_run = 100000;
            return_pw.duration_microseconds = 100000;
            request_scheduled_animation(&return_pw);
            previous_touch_or_leftclick_end.handled = true;
            return;
        }
        
        if (touch_id >= 0 && touch_id < 4) {
            if (!ran_anim[touch_id]) {
                ran_anim[touch_id] = true;
                ScheduledAnimation flip_card;
                construct_scheduled_animation(&flip_card);
                flip_card.affected_object_id = 234 + touch_id;
                flip_card.final_y_angle_known = true;
                flip_card.final_y_angle = 0.0f;
                flip_card.duration_microseconds = 275000;
                request_scheduled_animation(&flip_card);
                
                ScheduledAnimation move_card_closer;
                construct_scheduled_animation(&move_card_closer);
                move_card_closer.affected_object_id = 234 + touch_id;
                move_card_closer.final_z_known = true;
                move_card_closer.final_mid_z = 1.0f - (touch_id * 0.01f);
                move_card_closer.remaining_wait_before_next_run = 200000;
                move_card_closer.duration_microseconds = 250000;
                request_scheduled_animation(&move_card_closer);
            } else {
                ran_anim[touch_id] = false;
                ScheduledAnimation flip_card;
                construct_scheduled_animation(&flip_card);
                flip_card.affected_object_id = 234 + touch_id;
                flip_card.final_y_angle_known = true;
                flip_card.final_y_angle = 3.2f;
                flip_card.duration_microseconds = 250000;
                request_scheduled_animation(&flip_card);
                
                ScheduledAnimation move_card_back;
                construct_scheduled_animation(&move_card_back);
                move_card_back.affected_object_id = 234 + touch_id;
                move_card_back.final_z_known = true;
                move_card_back.final_mid_z = 3.0f + (touch_id * 0.01f);
                move_card_back.remaining_wait_before_next_run = 200000;
                move_card_back.duration_microseconds = 275000;
                request_scheduled_animation(&move_card_back);
            }
        }
        
        //        request_fading_lightsquare(
        //            previous_touch_or_leftclick_end.screen_x,
        //            previous_touch_or_leftclick_end.screen_y);
        
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
    
    #define BULLET_OBJECT_ID 5555
    if (keypress_map[49] == true) {
        // Spacebar pressed
        keypress_map[49] = false;                          
        zPolygon bullet = card_model;
        bullet.deleted = false;
        
        bullet.object_id = BULLET_OBJECT_ID;
        bullet.x = camera.x;
        bullet.y = camera.y;
        bullet.z = camera.z;
        for (uint32_t _ = 0; _ < bullet.triangles_size; _++) {
            bullet.triangles[_].texture_i = -1;
            bullet.triangles[_].texturearray_i = -1;
            bullet.triangles[_].color[0] = 1.0f;
            bullet.triangles[_].color[1] = 0.1f;
            bullet.triangles[_].color[2] = 0.1f;
            bullet.triangles[_].color[3] = 1.0f;
        }
        request_zpolygon_to_render(&bullet);
        
        zlights_to_apply[zlights_to_apply_size].object_id = BULLET_OBJECT_ID;
        zlights_to_apply[zlights_to_apply_size].x = bullet.x;
        zlights_to_apply[zlights_to_apply_size].y = bullet.y;
        zlights_to_apply[zlights_to_apply_size].z = bullet.z;
        zlights_to_apply[zlights_to_apply_size].reach = 5.0f;
        zlights_to_apply[zlights_to_apply_size].diffuse = 1.0f;
        zlights_to_apply[zlights_to_apply_size].ambient = 0.0f;
        zlights_to_apply[zlights_to_apply_size].deleted = false;
        zlights_to_apply[zlights_to_apply_size].RGBA[0] = bullet.triangles[0].color[0];
        zlights_to_apply[zlights_to_apply_size].RGBA[1] = bullet.triangles[0].color[1];
        zlights_to_apply[zlights_to_apply_size].RGBA[2] = bullet.triangles[0].color[2];
        zlights_to_apply[zlights_to_apply_size].RGBA[3] = bullet.triangles[0].color[3];
        zlights_to_apply_size += 1;
        
        ScheduledAnimation move_bullet;
        construct_scheduled_animation(&move_bullet);
        move_bullet.affected_object_id = bullet.object_id;
        move_bullet.duration_microseconds = 9000000;
        move_bullet.delta_z_per_second = 0.75f;
        move_bullet.delete_object_when_finished = true;
        request_scheduled_animation(&move_bullet);
    }
}

bool32_t fading_out = true;

void client_logic_update(uint64_t microseconds_elapsed)
{
    for (uint32_t i = 0; i < zlights_to_apply_size; i++) {
        if (zlights_to_apply[i].object_id == BULLET_OBJECT_ID) {
            printf("bullet light at {%f,%f,%f}\n",
                zlights_to_apply[i].x,
                zlights_to_apply[i].y,
                zlights_to_apply[i].z);
        }
    }
    // request_fps_counter(microseconds_elapsed);
    
    //    zlights_to_apply[0].x = camera.x;
    //    zlights_to_apply[0].y = camera.y;
    //    zlights_to_apply[0].z = camera.z;
    
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
