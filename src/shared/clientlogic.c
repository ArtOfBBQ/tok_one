#include "clientlogic.h"

#define TEXTURE_FILENAMES_SIZE 6
DecodedImage * decoded_pngs[TEXTURE_FILENAMES_SIZE];

zPolygon load_from_obj_file(char * filename)
{
    FileBuffer buffer;
    buffer.size = platform_get_filesize(filename) + 1;
    char buffer_contents[buffer.size];
    buffer.contents = (char *)&buffer_contents;
    platform_read_file(filename, &buffer);
    
    zPolygon return_value =
        parse_obj(
            /* rawdata     : */ buffer.contents,
            /* rawdata_size: */ buffer.size);
    
    return return_value;
}

uint32_t label_object_id = 0;
uint32_t teapot_object_id = 1;

// reminder: this will be run in the background, a
// thread is called in client_logic_startup
void load_assets(void) {
    printf("load_assets()\n");
    
    // char filenames_for_texturearrays
    //     [TEXTUREARRAYS_SIZE]
    //     [MAX_FILES_IN_SINGLE_TEXARRAY]
    //     [MAX_ASSET_FILENAME_SIZE];
    // uint32_t filenames_for_texturearrays_size = 0;
    
    // an example of a font texture in font.png
    // Note: I generally keep my font in slot 0 and only
    // use 1 font
    // if you want to change the texturearray slot used
    // as the font, you need to change font_texturearray_i
    // variable in text.c
    texture_arrays[0].sprite_columns = 9;
    texture_arrays[0].sprite_rows = 9;
    texture_arrays_size++;
    
    char centered_text[145] =
        "I'm a text\nMy purpose is to test centered text, possibly long sentences that don't necessarily make any sense like this.\nOr small sentences.";
    
    float centered_text_color[4];
    centered_text_color[0] = 0.8f;
    centered_text_color[1] = 0.2f;
    centered_text_color[2] = 0.8f;
    centered_text_color[3] = 1.0f;
    
    font_height = 14.0f;
    request_label_around(
        /* with_id:          : */ 50,
        /* text              : */ centered_text,
        /* text_color[4]     : */ centered_text_color,
        /* text_to_draw_size : */ 140,
        /* mid_x_pixelspace  : */ window_width * 0.5,
        /* mid_y_pixelspace  : */ window_height * 0.5,
        /* z                 : */ 0.6f,
        /* max_width         : */ window_width * 0.25,
        /* ignore_camera     : */ false);
    
    // 16x16 sample sprites in phoebus.png
    texture_arrays[1].sprite_columns = 16;
    texture_arrays[1].sprite_rows = 16;
    texture_arrays_size++;
    
    // 5 lore seeker cards and a debug texture
    // in sampletexture.png
    texture_arrays[2].sprite_columns = 3;
    texture_arrays[2].sprite_rows = 2;
    texture_arrays_size++;
     
    FileBuffer file_buffer;
    
    assert(TEXTURE_FILENAMES_SIZE <= TEXTUREARRAYS_SIZE);
    char * texture_filenames[TEXTURE_FILENAMES_SIZE] = {
        (char *)"font.png",
        (char *)"phoebus.png",
        (char *)"sampletexture.png",
        (char *)"structuredart1.png",
        (char *)"structuredart2.png",
        (char *)"structuredart3.png"};
    
    for (
        uint32_t i = 0;
        i < TEXTURE_FILENAMES_SIZE;
        i++)
    {
        printf("*** texture_arrays update at i: %u \n", i);
        assert(i < TEXTUREARRAYS_SIZE);
        
        file_buffer.size =
            platform_get_filesize(texture_filenames[i]) + 1;
        file_buffer.contents =
            (char *)malloc(file_buffer.size);
        printf(
            "loaded file_buffer with size: " FUINT64 "\n",
            file_buffer.size);
        
        if (file_buffer.size < 1)
        {
            printf(
                "ERROR: failed to read file from disk: %s\n",
                texture_filenames[i]);
            assert(0);
        }
        
        platform_read_file(
            texture_filenames[i], &file_buffer);
        printf(
            "read file %s into file buffer\n",
            texture_filenames[i]);
        
        DecodedImage * new_image =
            (DecodedImage *)malloc(sizeof(DecodedImage));
        new_image->good = false;
        
        printf("get width & height...\n");
        
        const uint8_t * start_of_filebuf = (const uint8_t *)file_buffer.contents;
        
        get_PNG_width_height(
            /* uint8_t * compressed_bytes: */
                start_of_filebuf,
            /* uint32_t compressed_bytes_size: */
                29,
            /* uint32_t * width_out: */
                &new_image->width,
            /* uint32_t * height_out: */
                &new_image->height);
        
        printf(
            "will alloc PNG with width/height: [%u,%u]\n",
            new_image->width,
            new_image->height);
        
        assert(new_image->width > 0);
        assert(new_image->height > 0);
        
        new_image->rgba_values_size =
            new_image->width * new_image->height * 4;
        new_image->rgba_values =
            (uint8_t *)malloc(new_image->rgba_values_size);
        printf(
            "allocated new_image with size: %u bytes\n",
            new_image->rgba_values_size);
        
        decode_PNG(
            /* compressed_bytes: */
                start_of_filebuf,
            /* compressed_bytes_size: */
                file_buffer.size - 1,
            /* DecodedImage * out_preallocated_png: */
                new_image);
        printf(
            "decoded_png with size: [%u,%u] (%u bytes)\n",
            new_image->width,
            new_image->height,
            new_image->rgba_values_size);
        
        decoded_pngs[i] = new_image;
        
        texture_arrays[i].image = decoded_pngs[i];
        texture_arrays[i].request_update = true;
        
        free(file_buffer.contents);
    }
    
    // 3 images with the same heigth/width
    // (structuredart1.png, structuredart2.png,
    // structuredart3.png)
    DecodedImage * concatenated =
        (DecodedImage *)malloc(sizeof(DecodedImage));
    assert(decoded_pngs[3]->width == 10);
    assert(decoded_pngs[3]->height == 10);
    assert(decoded_pngs[4]->width == 10);
    assert(decoded_pngs[4]->height == 10);
    assert(decoded_pngs[5]->width == 10);
    assert(decoded_pngs[5]->height == 10);
    *concatenated = concatenate_images(
        /* const DecodedImage ** to_concat: */
            (DecodedImage **)&(decoded_pngs[3]),
        /* to_concat_size: */
            3,
        /* out_sprite_rows: */
            &texture_arrays[3].sprite_rows,
        /* out_sprite_columns: */
            &texture_arrays[3].sprite_columns);
    assert(concatenated->width == 20);
    assert(concatenated->height == 20);
    texture_arrays[3].image = concatenated;
    texture_arrays[3].request_update = true;
    texture_arrays_size++;
    
    // get a zpolygon object
    zpolygons_to_render_size += 1;
    zpolygons_to_render[0] = load_from_obj_file(
        (char *)"teapot.obj");
    zpolygons_to_render[0].x = 0.0f;
    zpolygons_to_render[0].y = 0.0f;
    zpolygons_to_render[0].z = 250.0f;
    zpolygons_to_render[0].x_angle = 0.0f;
    zpolygons_to_render[0].y_angle = 0.0f;
    zpolygons_to_render[0].z_angle = 0.0f;
    assert(zpolygons_to_render[0].triangles_size > 0);
    scale_zpolygon(
        /* to_scale   : */
            &zpolygons_to_render[0],
        /* new_height : */
            50.0f);
    
    for (
        uint32_t t = 0;
        t < zpolygons_to_render[0].triangles_size;
        t++)
    {
        zpolygons_to_render[0].triangles[t].color[0] = 1.0f;
        zpolygons_to_render[0].triangles[t].color[1] = 1.0f;
        zpolygons_to_render[0].triangles[t].color[2] = 1.0f;
        zpolygons_to_render[0].triangles[t].color[3] = 1.0f;
    }
    
    // initialize global zLightSource objects, to set up
    // our lighting for the scene
    zlights_to_apply[0].x = 0;
    zlights_to_apply[0].y = window_height * 0.5f;
    zlights_to_apply[0].z = -0.5f;
    zlights_to_apply[0].RGBA[0] = 1.0f;
    zlights_to_apply[0].RGBA[1] = 0.0f;
    zlights_to_apply[0].RGBA[2] = 1.0f;
    zlights_to_apply[0].RGBA[3] = 1.0f;
    zlights_to_apply[0].reach = 75.0f;
    zlights_to_apply[0].ambient = 8.0f;
    zlights_to_apply[0].diffuse = 1.0f;
    zlights_to_apply_size += 1;
    assert(zlights_to_apply_size <= ZLIGHTS_TO_APPLY_ARRAYSIZE);
    
    zlights_to_apply[1].x = window_width;
    zlights_to_apply[1].y = window_height * 0.5f;
    zlights_to_apply[1].z = -0.5f;
    zlights_to_apply[1].RGBA[0] = 0.0f;
    zlights_to_apply[1].RGBA[1] = 1.0f;
    zlights_to_apply[1].RGBA[2] = 0.0f;
    zlights_to_apply[1].RGBA[3] = 1.0f;
    zlights_to_apply[1].reach = 75.0f;
    zlights_to_apply[1].ambient = 8.0f;
    zlights_to_apply[1].diffuse = 1.0f;
    zlights_to_apply_size += 1;
    assert(zlights_to_apply_size <= ZLIGHTS_TO_APPLY_ARRAYSIZE);
    
    font_height = 40.0f;
    
    TexQuad sample_pic;
    construct_texquad(&sample_pic);
    sample_pic.object_id = 4;
    sample_pic.touchable_id = 88;
    sample_pic.texturearray_i = 2;
    sample_pic.texture_i = 0;
    sample_pic.width_pixels =
        texture_arrays[2].image->width * 0.1f;
    sample_pic.height_pixels =
        texture_arrays[2].image->height * 0.1f;
    sample_pic.left_pixels =
        0.0f
            - (sample_pic.width_pixels * 0.5f);
    sample_pic.top_pixels = (window_height * 0.9f);
    request_texquad_renderable(&sample_pic);
    
    // test our home-concatenated image
    // (we concatenated structuredart1.png, structedart2.png,
    // structuredart3.png)
    TexQuad sample_pic_2;
    construct_texquad(&sample_pic_2);
    sample_pic_2.object_id = 5;
    sample_pic_2.touchable_id = 89;
    sample_pic_2.texturearray_i = 3;  // from concatenated
    sample_pic_2.texture_i = 2; // 2nd in concatenated
    sample_pic_2.width_pixels = 200.0f;
    sample_pic_2.height_pixels = 200.0f;
    sample_pic_2.left_pixels = 20.0f;
    sample_pic_2.top_pixels = (window_height * 0.95f);
    request_texquad_renderable(&sample_pic_2);
    
    TexQuad replacable_pic;
    construct_texquad(&replacable_pic);
    replacable_pic.object_id = 6;
    replacable_pic.touchable_id = 1;
    replacable_pic.texturearray_i = 1;
    replacable_pic.texture_i = 0;
    replacable_pic.width_pixels = 200.0f;
    replacable_pic.height_pixels = 200.0f;
    replacable_pic.left_pixels =
        (window_width * 0.9)
            - (replacable_pic.width_pixels * 0.5f);
    replacable_pic.top_pixels = (window_height * 0.8f);
    request_texquad_renderable(&replacable_pic);
    
    ScheduledAnimation move_sprite_left;
    construct_scheduled_animation(&move_sprite_left);
    move_sprite_left.affected_object_id = 4;
    move_sprite_left.final_x_known = true;
    move_sprite_left.final_mid_x = (window_width * 0.5f);
    move_sprite_left.final_y_known = true;
    move_sprite_left.final_mid_y = (window_height * 0.5f);
    move_sprite_left.wait_first_microseconds = 1750000;
    move_sprite_left.remaining_microseconds = 3000000;
    request_scheduled_animation(&move_sprite_left);
    
    ScheduledAnimation downsize_concatenated_img;
    construct_scheduled_animation(&downsize_concatenated_img);
    downsize_concatenated_img.affected_object_id = 5;
    downsize_concatenated_img.final_xscale_known = true;
    downsize_concatenated_img.final_xscale = 0.5f;
    downsize_concatenated_img.final_yscale_known = true;
    downsize_concatenated_img.final_yscale = 0.5f;
    downsize_concatenated_img.wait_first_microseconds = 500000;
    downsize_concatenated_img.remaining_microseconds = 5000000;
    request_scheduled_animation(&downsize_concatenated_img);
}

void client_logic_startup() {
    printf("client_logic_startup()\n");    
    // These are some example texture atlases we're using for
    // texture mapping on cards and cubes
    assert(TEXTUREARRAYS_SIZE > 0);
   
    // reminder: threadmain_id 0 calls load_assets() 
    platform_start_thread(/* threadmain_id: */ 0);
    
    printf("finished client_logic_startup()\n");    
}

void client_logic_threadmain(int32_t threadmain_id) {
    printf(
        "client_logic_threadmain(%i)\n",
        threadmain_id);
    
    switch (threadmain_id) {
        case (0):
            load_assets();
            break;
        default:
            printf(
                "unhandled threadmain_id: %i\n",
                threadmain_id);
    }
}

void client_logic_animation_callback(int32_t callback_id)
{
    printf(
        "client_logic_animation_callback(%i)\n",
        callback_id);
}

void client_handle_mouseevents(
    uint64_t microseconds_elapsed)
{
    uint32_t touched_object_id = 999999;
    
    if (!last_mouse_down.handled) {
        last_mouse_down.handled = true;
        if (
            touchable_id_to_texquad_object_id(
                /* const int32_t touchable_id : */
                    last_mouse_down.touchable_id,
                /* uint32_t * object_id_out : */
                    &touched_object_id))
        {
            assert(touched_object_id != 999999);
            
            ScheduledAnimation brighten;
            construct_scheduled_animation(&brighten);
            ScheduledAnimation dim;
            construct_scheduled_animation(&dim);
            
            brighten.affected_object_id = touched_object_id;
            dim.affected_object_id = touched_object_id;
            
            brighten.remaining_microseconds = 150000;
            dim.wait_first_microseconds =
                brighten.remaining_microseconds;
            dim.remaining_microseconds =
                brighten.remaining_microseconds;
            
            for (uint32_t i = 0; i < 3; i++) {
                brighten.rgba_delta_per_second[i] = 0.9;
                dim.rgba_delta_per_second[i] = -0.9;
            }
            brighten.z_rotation_per_second = -0.13;
            dim.z_rotation_per_second = 0.13;
            
            request_scheduled_animation(&brighten);
            request_scheduled_animation(&dim);
        } else {
            printf("touched screen at: [%f,%f]\n",
                last_mouse_down.screenspace_x,
                last_mouse_down.screenspace_y);
            TexQuad touch_highlight;
            construct_texquad(&touch_highlight);
            touch_highlight.object_id = 72;
            touch_highlight.left_pixels = last_mouse_down.screenspace_x;
            touch_highlight.top_pixels = last_mouse_down.screenspace_y;
            touch_highlight.z = 50;
            touch_highlight.height_pixels = 20.0f;
            touch_highlight.width_pixels = 20.0f;
            touch_highlight.RGBA[0] = 1.0f;
            touch_highlight.RGBA[0] = 0.0f;
            touch_highlight.RGBA[0] = 1.0f;
            touch_highlight.RGBA[0] = 1.0f;
            request_texquad_renderable(&touch_highlight);
            
            ScheduledAnimation vanish;
            construct_scheduled_animation(&vanish);
            vanish.affected_object_id = touch_highlight.object_id;
            vanish.remaining_microseconds = 400000;
            vanish.delete_object_when_finished = true;
            vanish.rgba_delta_per_second[3] = -2.5f;
            request_scheduled_animation(&vanish);
        }
    }
}

void client_handle_keypresses(
    uint64_t microseconds_elapsed)
{
    float elapsed_mod =
        (float)((double)microseconds_elapsed / (double)16666);
    float cam_speed = 0.25f * elapsed_mod;
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
    
    if (keypress_map[12] == true)
    {
        // 'Q' key is pressed
        camera.x_angle -= cam_rotation_speed;
    }
    
    if (keypress_map[123] == true)
    {
        // left arrow key
        camera.y_angle -= cam_rotation_speed;
        // camera.x -= cam_speed;
    }
    
    if (keypress_map[124] == true)
    {
        // right arrow key
        camera.y_angle += cam_rotation_speed;
        // camera.x += cam_speed;
    }
    
    if (keypress_map[125] == true)
    {
        // down arrow key
        camera.z -= cosf(camera.y_angle) * cam_speed;
        camera.x -= sinf(camera.y_angle) * cam_speed;
    }
    
    if (keypress_map[126] == true)
    {
        // up arrow key is pressed
        zcamera_move_forward(
            &camera,
            cam_speed);
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

void client_handle_touches(
    uint64_t microseconds_elapsed)
{
    //float elapsed_mod =
    //    (float)((double)microseconds_elapsed / (double)16666);
    
    // handle tablet & phone touches
    if (!current_touch.handled) {
        if (current_touch.finished) {
            // an unhandled, finished touch
            if (
                (current_touch.finished_at
                    - current_touch.started_at) < 7500)
            {
                if (current_touch.current_y >
                    (window_height * 0.5))
                {
                    camera.z -= 3.0f;
                } else {
                    camera.z += 3.0f;
                }
                current_touch.handled = true;
            } else {
                float delta_x = current_touch.current_x -
                    current_touch.start_x;
                
                if (delta_x < -50.0 || delta_x > 50.0) {
                    camera.y_angle -= (delta_x * 0.001f);
                }
                current_touch.handled = true;
            }
        }
    }
}

bool32_t fading_out = true;
char fps_string[8] = "fps: xx";

void client_logic_update(
    uint64_t microseconds_elapsed)
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
    
    float fps_color[4];
    fps_color[0] = 0.8f;
    fps_color[1] = 0.2f;
    fps_color[2] = 0.8f;
    fps_color[3] = 1.0f;
    delete_texquad_object(label_object_id);
    
    request_label_renderable(
        /* with_id               : */ label_object_id,
        /* char * text_to_draw   : */ fps_string,
        /* float text_color[4]   : */ fps_color,
        /* text_to_draw_size     : */ 7,
        /* float left_pixelspace : */ 20.0f,
        /* float top_pixelspace  : */ 60.0f,
        /* z                     : */ 0.5f,
        /* float max_width       : */ window_width,
        /* bool32_t ignore_camera: */ true);
    
    client_handle_mouseevents(
        microseconds_elapsed);
    client_handle_keypresses(
        microseconds_elapsed);
    client_handle_touches(
        microseconds_elapsed);
}

