#include "clientlogic.h"

#define TEXTURE_FILENAMES_SIZE 6
DecodedImage * decoded_pngs[TEXTURE_FILENAMES_SIZE];

int32_t latest_object_id = 72;

typedef struct TextureArrayLocation {
    int32_t texturearray_i;
    int32_t texture_i;
} TextureArrayLocation;

zPolygon load_from_obj_file(char * filepath) {
    FileBuffer buffer;
    buffer.size = (uint64_t)platform_get_resource_size(filepath) + 1;
    buffer.contents = (char *)malloc_from_managed(buffer.size);
    platform_read_resource_file(
        filepath,
        &buffer);
    
    assert(buffer.size > 1);
    
    zPolygon return_value = parse_obj(
        /* rawdata     : */ buffer.contents,
        /* rawdata_size: */ buffer.size);
    
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

void client_logic_startup() {
    
    init_rand_with_seed(platform_get_current_time_microsecs());
    
    const char * fontfile;
    fontfile = "font.png";
    register_new_texturearray_by_splitting_file(
        /* filename : */ fontfile,
        /* rows     : */ 10,
        /* columns  : */ 10);
    
    char * filenames[2] = {
        (char *)"structuredart1.png",
        (char *)"structuredart2.png"
    };
    register_new_texturearray_from_files(
        (const char **)filenames,
        2);
    
    zlights_to_apply[0].deleted = false;
    zlights_to_apply[0].object_id = -1;
    zlights_to_apply[0].x = window_width / 2;
    zlights_to_apply[0].y = window_height / 2;
    zlights_to_apply[0].z = 10.0f;
    zlights_to_apply[0].RGBA[0] = 1.0f;
    zlights_to_apply[0].RGBA[1] = 1.0f;
    zlights_to_apply[0].RGBA[2] = 1.0f;
    zlights_to_apply[0].RGBA[3] = 1.0f;
    zlights_to_apply[0].reach = 100;
    zlights_to_apply[0].ambient = 1.0f;
    zlights_to_apply[0].diffuse = 1.0f;
    zlights_to_apply[0].deleted = false;
    zlights_to_apply_size++;
    log_assert(zlights_to_apply_size == 1);
    
    TexQuad foreground_blue;
    construct_texquad(&foreground_blue);
    foreground_blue.object_id = -1;
    foreground_blue.texturearray_i = -1;
    foreground_blue.texture_i = -1;
    foreground_blue.width_pixels = 200;
    foreground_blue.height_pixels = 150;
    foreground_blue.left_pixels = 375;
    foreground_blue.top_pixels = window_height * 0.7f;
    foreground_blue.z = 0.01f;
    foreground_blue.RGBA[0] = 0.0f;
    foreground_blue.RGBA[1] = 0.0f;
    foreground_blue.RGBA[2] = 1.0f;
    foreground_blue.RGBA[3] = 1.0f;
    log_assert(texquads_to_render_size == 0);
    request_texquad_renderable(&foreground_blue);
    log_assert(texquads_to_render_size == 1);
    
    for (uint32_t x = 0; x < 3; x++) {
        TexQuad background;
        construct_texquad(&background);
        background.object_id = -1;
        background.texturearray_i = -1;
        background.texture_i = -1;
        background.width_pixels = 200;
        background.height_pixels = 200;
        background.left_pixels = 50 + (x * 200);
        background.top_pixels = window_height * 0.5f;
        background.z = 0.2f; // 30.0f + (x * 1.0f);
        background.RGBA[0] = 1.0f;
        background.RGBA[1] = 0.2f * x;
        background.RGBA[2] = 0.2f;
        background.RGBA[3] = 1.0f;
        request_texquad_renderable(&background);
    }
    
    
    //    zPolygon teapot = load_from_obj_file("teapot.obj");
    //    zpolygon_scale_to_width_given_z(
    //        /* to_scale: */ &teapot,
    //        /* new_width: */ 100,
    //        /* when_observed_at_z: */ 20.0f);
    //    teapot.object_id = 12345;
    //    teapot.x = -2;
    //    teapot.y = -2;
    //    teapot.z = 40.0f;
    //    zpolygons_to_render[zpolygons_to_render_size++] = teapot;
    //    assert(zpolygons_to_render_size == 1);
    
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
    touch_highlight.left_pixels = location_x;
    touch_highlight.top_pixels = location_y;
    touch_highlight.z = 0.5f;
    touch_highlight.height_pixels = 75.0f;
    touch_highlight.width_pixels = 75.0f;
    touch_highlight.RGBA[0] = 1.0f;
    touch_highlight.RGBA[0] = 0.0f;
    touch_highlight.RGBA[0] = 1.0f;
    touch_highlight.RGBA[0] = 1.0f;
    touch_highlight.texturearray_i = 0;
    touch_highlight.texture_i = cur_texture_i++;
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
    zlights_to_apply[new_zlight_i].x = location_x;
    zlights_to_apply[new_zlight_i].y = location_y;
    zlights_to_apply[new_zlight_i].z = 50.0f;
    
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
    cur_color_i += 1;
    if (cur_color_i > 2) { cur_color_i = 0; }
    
    zlights_to_apply[new_zlight_i].ambient = 0.0f;
    zlights_to_apply[new_zlight_i].diffuse = 1.0f;
    zlights_to_apply[new_zlight_i].reach = 200.0f;
    if (new_zlight_i == zlights_to_apply_size) {
         zlights_to_apply_size += 1;
    }
    
    ScheduledAnimation moveupandright;
    construct_scheduled_animation(&moveupandright);
    moveupandright.affected_object_id = touch_highlight.object_id;
    moveupandright.duration_microseconds = 6000000;
    moveupandright.delta_x_per_second = 150;
    moveupandright.delta_y_per_second = 150;
    request_scheduled_animation(&moveupandright);
    
    ScheduledAnimation vanish;
    construct_scheduled_animation(&vanish);
    vanish.affected_object_id = touch_highlight.object_id;
    vanish.remaining_wait_before_next_run = 4000000;
    vanish.duration_microseconds = 4000000;
    vanish.delete_object_when_finished = true;
    vanish.rgba_delta_per_second[0] = -0.5f;
    vanish.rgba_delta_per_second[1] = -0.5f;
    vanish.rgba_delta_per_second[2] = -0.5f;
    request_scheduled_animation(&vanish);
}

static void  client_handle_touches_and_leftclicks(
    uint64_t microseconds_elapsed)
{
    if (!previous_touch_or_leftclick_end.handled) {
        request_fading_lightsquare(
            previous_touch_or_leftclick_end.screen_x,
            previous_touch_or_leftclick_end.screen_y);
        
        previous_touch_or_leftclick_end.handled = true;
    }
}

static void client_handle_keypresses(uint64_t microseconds_elapsed) {
    float elapsed_mod =
        (float)((double)microseconds_elapsed / (double)16666);
    float cam_speed = 2.0f * elapsed_mod;
    // float cam_rotation_speed = 0.05f * elapsed_mod;
    
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
}

bool32_t fading_out = true;

void client_logic_update(uint64_t microseconds_elapsed)
{
    // TODO: our timer is weirdly broken on iOS. Fix it!
    // request_fps_counter(microseconds_elapsed);
    
    client_handle_touches_and_leftclicks(microseconds_elapsed);
    client_handle_keypresses(microseconds_elapsed); 
    
    // zpolygons_to_render[0].z += 0.05f;
    zpolygons_to_render[0].x += 0.001f;
    zpolygons_to_render[0].y += 0.001f;
    zpolygons_to_render[0].z_angle += 0.01f;
    
    texquads_to_render[0].top_pixels += 0.05f;
    texquads_to_render[0].z += 0.001f;
    printf("blue foreground z: %f\n", texquads_to_render[0].z);
    
    if (texquads_to_render[0].z <= 0.8f &&
        texquads_to_render[0].z >= 0.6f)
    {
        texquads_to_render[0].RGBA[0] = 1.0f;
    }
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
