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

zPolygon load_from_obj_file(char * filename)
{
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

uint32_t label_object_id = 0;
uint32_t teapot_object_id = 1;

static void preregister_assets() {
    
    const char * fontfile;
    fontfile = "font.png";
    register_new_texturearray_by_splitting_file(
        /* filename : */ fontfile,
        /* rows     : */ 10,
        /* columns  : */ 10);
    texture_arrays[0].images_size = 100;
    texture_arrays[0].single_img_width =
        texture_arrays[0].images[0].image->width;
    texture_arrays[0].single_img_height =
        texture_arrays[0].images[0].image->height;
    texture_arrays[0].request_init = true;
    for (uint32_t i = 0; i < texture_arrays[0].images_size; i++) {
        texture_arrays[0].images[i].request_update = true;
        log_assert(
            texture_arrays[0].images[i].image->width ==
                texture_arrays[0].single_img_width);
        log_assert(
            texture_arrays[0].images[i].image->height ==
                texture_arrays[0].single_img_height);
    }

    FileBuffer imgfileheader;
    imgfileheader.size = 50;
    imgfileheader.contents = (char *)malloc_from_managed(imgfileheader.size);
    platform_read_resource_file(
        "structuredart1.png",
        &imgfileheader);
    
    bool32_t header_parsed_succesfully = false;
    get_PNG_width_height(
        (uint8_t *)imgfileheader.contents,
        imgfileheader.size,
        &texture_arrays[1].single_img_width,
        &texture_arrays[1].single_img_height,
        &header_parsed_succesfully);
    log_assert(header_parsed_succesfully);
    
    // 5mb                              5...000
    uint64_t dpng_working_memory_size = 5000000;
    uint8_t * dpng_working_memory =
        malloc_from_managed(dpng_working_memory_size);
    texture_arrays[1].images_size = 2;
    log_assert(texture_arrays[1].single_img_width > 0);
    log_assert(texture_arrays[1].single_img_height > 0);
    texture_arrays[1].request_init = true;
    
    update_texture_slice_from_file_with_memory(
        /* const char * filename: */
            "structuredart1.png",
        /* const int32_t at_texture_array_i: */
            1,
        /* const int32_t at_texture_i: */
            0,
        /* uint8_t * dpng_working_memory: */
            dpng_working_memory,
        /* uint64_t dpng_working_memory_size: */
            dpng_working_memory_size);
    texture_arrays_size++;
    
    update_texture_slice_from_file_with_memory(
        /* const char * filename: */
            "structuredart2.png",
        /* const int32_t at_texture_array_i: */
            1,
        /* const int32_t at_texture_i: */
            1,
        /* uint8_t * dpng_working_memory: */
            dpng_working_memory,
        /* uint64_t dpng_working_memory_size: */
            dpng_working_memory_size);
    
    free_from_managed(dpng_working_memory);
}

static void load_assets(uint32_t start_i, uint32_t last_i)
{
    for (
        int32_t dimension_i = (int32_t)start_i;
        dimension_i <= (int32_t)last_i;
        dimension_i++)
    {
        if (!application_running) { return; }
        
        log_assert(texture_arrays[0].images[52].image->width == 39);
        uint32_t filenames_count = 0;
        while (
            filenames_for_texturearrays[dimension_i][filenames_count][0]
                != '\0')
        {
            log_assert(texture_arrays[0].images[52].image->width == 39);
            filenames_count++;
        }
        log_assert(texture_arrays[0].images[52].image->width == 39);
    }
}

char * client_logic_get_application_name() {
    return (char *)"TOK ONE";
}

void client_logic_startup() {
    
    preregister_assets();
    
    log_assert(texture_arrays[0].images[52].image->width == 39);
    log_assert(texture_arrays_size > 0); 
    load_assets(1, texture_arrays_size - 1);
    log_assert(texture_arrays[0].images[52].image->width == 39);
    
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
    log_assert(texture_arrays[0].images[52].image->width == 39);
    
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
    log_assert(texture_arrays[0].images[52].image->width == 39);
    
    // reminder: threadmain_id 0 calls load_assets() 
    log_append("starting asset-loading thread...\n");
    platform_start_thread(client_logic_threadmain, /* threadmain_id: */ 0);
    
    log_assert(texture_arrays[0].images[52].image->width == 39);
    log_append("finished client_logic_startup()\n");
}

void client_logic_threadmain(int32_t threadmain_id) {
    switch (threadmain_id) {
        case (0):
            load_assets(1, texture_arrays_size - 1);
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
}

uint32_t cur_color_i = 0;
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

static void client_handle_keypresses(
    uint64_t microseconds_elapsed)
{
    float elapsed_mod =
        (float)((double)microseconds_elapsed / (double)16666);
    float cam_speed = 2.0f * elapsed_mod;
    float cam_rotation_speed = 0.05f * elapsed_mod;
    
    if (keypress_map[0] == true)
    {
        // 'A' key is pressed
        camera.x_angle += cam_rotation_speed;
    }
    
    if (keypress_map[6] == true)
    {
        // 'Z' key is pressed
        camera.z_angle -= cam_rotation_speed;
    }
    
    if (keypress_map[7] == true)
    {
       // 'X' key
       camera.z_angle += cam_rotation_speed;
    }

    if (keypress_map[8] == true) {
        // c key is pressed
        log_append("pressed C key, crashing...\n");
        log_assert(0);
    }
    
    if (keypress_map[12] == true)
    {
        // 'Q' key is pressed
        camera.x_angle -= cam_rotation_speed;
    }
    
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
    
    if (keypress_map[30] == true)
    {
        // ] is pressed
    }
    
    if (keypress_map[42] == true)
    {
        // [ is pressed
    }
    
    if (keypress_map[46] == true)
    {
        // m key is pressed
    }
}

bool32_t fading_out = true;
char fps_string[8] = "fps: xx";

void client_logic_update(uint64_t microseconds_elapsed)
{
    // TODO: our timer is weirdly broken on iOS. Fix it!
    uint64_t fps = 1000000 / microseconds_elapsed;
    /*
    float elapsed_mod =
        (float)((double)microseconds_elapsed / (double)16666);
    */
    
    if (fps < 100) {
        fps_string[5] = '0' + ((fps / 10) % 10);
        fps_string[6] = '0' + (fps % 10);
    } else {
        fps_string[5] = '9' + ((fps / 10) % 10);
        fps_string[6] = '9' + (fps % 10);
    }
    
    delete_texquad_object(label_object_id);
    request_label_renderable(
        /* with_id               : */ label_object_id,
        /* char * text_to_draw   : */ fps_string,
        /* float left_pixelspace : */ 20.0f,
        /* float top_pixelspace  : */ 60.0f,
        /* z                     : */ 0.5f,
        /* float max_width       : */ window_width,
        /* bool32_t ignore_camera: */ true);
    
    client_handle_touches_and_leftclicks(microseconds_elapsed);
    client_handle_keypresses(microseconds_elapsed);
}

void client_logic_window_resize(
    const uint32_t new_height,
    const uint32_t new_width)
{
    return;
}

