#include "clientlogic.h"

#define TEXTURE_FILENAMES_SIZE 6
DecodedImage * decoded_pngs[TEXTURE_FILENAMES_SIZE];

int32_t latest_object_id = 72;

char filenames_for_texturearrays
    [TEXTUREARRAYS_SIZE]
    [MAX_FILES_IN_SINGLE_TEXARRAY]
    [MAX_ASSET_FILENAME_SIZE];
uint32_t filenames_for_texturearrays_size = 0;

typedef struct TextureArrayLocation {
    int32_t texturearray_i;
    int32_t texture_i;
} TextureArrayLocation;

zPolygon load_from_obj_file(char * filename) {
    FileBuffer buffer;
    buffer.size = (uint64_t)platform_get_filesize(filename) + 1;
    char buffer_contents[buffer.size];
    buffer.contents = (char *)&buffer_contents;
    platform_read_file(
        filename,
        &buffer);
    
    zPolygon return_value = parse_obj(
        /* rawdata     : */ buffer.contents,
        /* rawdata_size: */ buffer.size);
    
    return return_value;
}

void client_logic_get_application_name(
    char * recipient,
    const uint32_t recipient_size)
{
    char * app_name = (char *)"TOK ONE";
    
    copy_0term_string_to(
        /* recipient: */ recipient,
        /* recipient_size: */ recipient_size,
        /* origin: */ app_name);
}

void client_logic_startup() {
    
    const char * fontfile;
    fontfile = "font.png";
    register_new_texturearray_by_splitting_file(
        /* filename : */ fontfile,
        /* rows     : */ 10,
        /* columns  : */ 10);
    
    char * filenames[2] = {
        "structuredart1.png",
        "structuredart2.png"
    };
    register_new_texturearray_from_files(
        (const char **)filenames,
        2);
    
    zlights_to_apply[0].deleted = false;
    zlights_to_apply[0].object_id = -1;
    zlights_to_apply[0].x = 0;
    zlights_to_apply[0].y = 0;
    zlights_to_apply[0].z = 0;
    zlights_to_apply[0].RGBA[0] = 1.0f;
    zlights_to_apply[0].RGBA[1] = 0.7f;
    zlights_to_apply[0].RGBA[2] = 0.7f;
    zlights_to_apply[0].RGBA[3] = 1.0f;
    zlights_to_apply[0].reach = 4294967295;
    zlights_to_apply[0].ambient = 1.0;
    zlights_to_apply[0].diffuse = 0.0;
    zlights_to_apply[0].deleted = false;
    zlights_to_apply_size++;
    log_assert(zlights_to_apply_size == 1);
    
    TexQuad sample_pic;
    construct_texquad(&sample_pic);
    sample_pic.object_id = 99999;
    sample_pic.texturearray_i = 1;
    sample_pic.texture_i = 0;
    sample_pic.width_pixels = 300;
    sample_pic.height_pixels = 300;
    sample_pic.left_pixels = 200;
    sample_pic.top_pixels = window_height / 2;
    sample_pic.RGBA[0] = 1.0f;
    sample_pic.RGBA[0] = 0.0f;
    sample_pic.RGBA[0] = 0.5f;
    sample_pic.RGBA[0] = 1.0f;
    request_texquad_renderable(&sample_pic);
    
    ScheduledAnimation linked_rotate_right;
    construct_scheduled_animation(&linked_rotate_right);
    linked_rotate_right.affected_object_id = 99999;
    linked_rotate_right.duration_microseconds = 400000;
    linked_rotate_right.z_rotation_per_second = 1.8f;
    linked_rotate_right.runs = 0;
    linked_rotate_right.wait_before_each_run = 400000;
    request_scheduled_animation(&linked_rotate_right);
    
    ScheduledAnimation linked_rotate_left;
    construct_scheduled_animation(&linked_rotate_left);
    linked_rotate_left.affected_object_id = 99999;
    linked_rotate_left.duration_microseconds = 400000;
    linked_rotate_left.z_rotation_per_second = -1.8f;
    linked_rotate_left.runs = 0;
    linked_rotate_left.remaining_wait_before_next_run = 400000;
    linked_rotate_left.wait_before_each_run = 400000;
    request_scheduled_animation(&linked_rotate_left);
    
    ScheduledAnimation linked_change_texarray;
    construct_scheduled_animation(&linked_change_texarray);
    linked_change_texarray.affected_object_id = 99999;
    linked_change_texarray.duration_microseconds = 1;
    linked_change_texarray.runs = 0;
    linked_change_texarray.set_texture_i = true;
    linked_change_texarray.new_texture_i = 1;
    linked_change_texarray.wait_before_each_run = 800000;
    request_scheduled_animation(&linked_change_texarray);
    
    ScheduledAnimation linked_revert_texarray;
    construct_scheduled_animation(&linked_revert_texarray);
    linked_revert_texarray.affected_object_id = 99999;
    linked_revert_texarray.duration_microseconds = 1;
    linked_revert_texarray.runs = 0;
    linked_revert_texarray.set_texture_i = true;
    linked_revert_texarray.new_texture_i = 0;
    linked_revert_texarray.remaining_wait_before_next_run = 400000;
    linked_revert_texarray.wait_before_each_run = 800000;
    request_scheduled_animation(&linked_revert_texarray);
    
    font_ignore_lighting = false;
    request_label_renderable(
        /* const uint32_t with_id: */
            99,
        /* const char * text_to_draw: */
            "dragonslongestwordcantpossiblyfitonthescreenasdas;dlfkjas;dlfkajs;dflaksjdf;lsakjdf;alskdfjas;ldkfjas;dlfkjas;dlfkajsdgkjsdfhgsdlgkjhslgrkjflksjdfalsdkjfstuff with feet like rabbits! 't is true, I swear!",
        /* const float left_pixelspace: */
            100,
        /* const float top_pixelspace: */
            window_height - 100,
        /* const float z: */
            1.0f,
        /* const float max_width: */
            450,
        /* const bool32_t ignore_camera: */
            false);
    
    // reminder: threadmain_id 0 calls load_assets() 
    log_append("starting asset-loading thread...\n");
    platform_start_thread(
        client_logic_threadmain,
        /* threadmain_id: */ 0);
    
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
    log_append(
        "unhandled client_logic_animation_callback(: ");
    log_append_int(callback_id);
    log_append("\n");
    log_dump_and_crash();
}

static uint32_t cur_color_i = 0;
static void request_fading_lightsquare(
    const float location_x,
    const float location_y)
{ 
    TexQuad touch_highlight;
    construct_texquad(&touch_highlight);
    touch_highlight.object_id = latest_object_id++;
    touch_highlight.left_pixels = location_x;
    touch_highlight.top_pixels = location_y;
    touch_highlight.z = 50;
    touch_highlight.height_pixels = 20.0f;
    touch_highlight.width_pixels = 20.0f;
    touch_highlight.RGBA[0] = 1.0f;
    touch_highlight.RGBA[0] = 0.0f;
    touch_highlight.RGBA[0] = 1.0f;
    touch_highlight.RGBA[0] = 1.0f;
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
    zlights_to_apply[new_zlight_i].z = 50;
    
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
}

bool32_t fading_out = true;

void client_logic_update(uint64_t microseconds_elapsed)
{
    // TODO: our timer is weirdly broken on iOS. Fix it!
    request_fps_counter(microseconds_elapsed);
    
    client_handle_touches_and_leftclicks(microseconds_elapsed);
    client_handle_keypresses(microseconds_elapsed);
}

void client_logic_window_resize(
    const uint32_t new_height,
    const uint32_t new_width)
{
    log_append("unhandled client_logic_window_resize()\n");
    log_dump_and_crash();
}

