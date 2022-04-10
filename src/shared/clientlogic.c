#include "clientlogic.h"

// If you want to texture polygons or just draw bitmaps
// to the screen you need to store your images in this array
TextureArray texture_arrays[TEXTUREARRAYS_SIZE];

// If you want to draw 3D objects to the screen, you need
// to set them up here
zPolygon * zpolygons_to_render[ZPOLYGONS_TO_RENDER_ARRAYSIZE];
uint32_t zpolygons_to_render_size = 0;

// You need lights to make your objects visible
zLightSource zlights_to_apply[ZLIGHTS_TO_APPLY_ARRAYSIZE];
uint32_t zlights_to_apply_size = 0;

zPolygon * load_from_obj_file(char * filename)
{
    FileBuffer * buffer = platform_read_file(filename);
    
    zPolygon * return_value = parse_obj(
        /* rawdata     : */ buffer->contents,
        /* rawdata_size: */ buffer->size);
    
    free(buffer->contents);
    free(buffer);
    
    return return_value;
}

uint64_t label_object_id = 0;

void client_logic_startup() {
    printf("client_logic_startup()\n");    
    // These are some example texture atlases we're using for
    // texture mapping on cards and cubes
    assert(TEXTUREARRAYS_SIZE > 0);
    
    // an example of a font texture in font.png
    // Note: I generally keep my font in slot 0 and only
    // use 1 font
    // if you want to change the texturearray slot used
    // as the font, you need to change font_texturearray_i
    // variable in text.c
    texture_arrays[0].sprite_columns = 9;
    texture_arrays[0].sprite_rows = 9;
    texture_arrays[0].request_update = false;
    
    // 16x16 sample sprites in phoebus.png
    texture_arrays[1].sprite_columns = 16;
    texture_arrays[1].sprite_rows = 16;
    texture_arrays[1].request_update = false;
    
    // 5 lore seeker cards and a debug texture 
    // in sampletexture.png
    texture_arrays[2].sprite_columns = 3;
    texture_arrays[2].sprite_rows = 2;
    texture_arrays[2].request_update = false;
    
    FileBuffer * file_buffer;
    
    #define TEXTURE_FILENAMES_SIZE 3
    assert(TEXTURE_FILENAMES_SIZE <= TEXTUREARRAYS_SIZE);
    char * texture_filenames[TEXTURE_FILENAMES_SIZE] = {
        "font.png",
        "phoebus.png",
        "sampletexture.png"};
    
    for (
        uint32_t i = 0;
        i < TEXTURE_FILENAMES_SIZE;
        i++)
    {
        assert(i < TEXTUREARRAYS_SIZE);
        printf(
            "trying to read file: %s\n",
            texture_filenames[i]);
        file_buffer = platform_read_file(
            texture_filenames[i]);
        if (file_buffer == NULL) {
            printf(
                "ERROR: failed to read file from disk: %s\n",
                texture_filenames[i]);
            assert(false);
        }
        texture_arrays[i].image = decode_PNG(
            (uint8_t *)file_buffer->contents,
            file_buffer->size);
        assert(texture_arrays[i].image->good);
        assert(texture_arrays[i].image->rgba_values_size > 0);
        
        free(file_buffer->contents);
        free(file_buffer);
        printf(
            "loaded texture %s with width %u from disk\n",
            texture_filenames[i],
            texture_arrays[i].image->width);
    }
    
    // get a zpolygon object
    zpolygons_to_render_size += 1;
    zpolygons_to_render[0] = load_from_obj_file("teapot.obj");
    zpolygons_to_render[0]->x = 0.0f;
    zpolygons_to_render[0]->y = 0.0f;
    zpolygons_to_render[0]->z = 250.0f;
    zpolygons_to_render[0]->x_angle = 0.0f;
    zpolygons_to_render[0]->y_angle = 0.0f;
    zpolygons_to_render[0]->z_angle = 0.0f;
    assert(zpolygons_to_render[0]->triangles_size > 0);
    scale_zpolygon(
        /* to_scale   : */
            zpolygons_to_render[0],
        /* new_height : */
            50.0f); 

    for (
        uint32_t t = 0;
        t < zpolygons_to_render[0]->triangles_size;
        t++)
    {
        zpolygons_to_render[0]->triangles[t].color[0] = 1.0f;
        zpolygons_to_render[0]->triangles[t].color[1] = 1.0f;
        zpolygons_to_render[0]->triangles[t].color[2] = 1.0f;
        zpolygons_to_render[0]->triangles[t].color[3] = 1.0f;
    }
    
    // initialize global zLightSource objects, to set up
    // our lighting for the scene
    zlights_to_apply[0].x = 50.0f;
    zlights_to_apply[0].y = 10.0f;
    zlights_to_apply[0].z = 40.0f;
    zlights_to_apply[0].RGBA[0] = 1.0f;
    zlights_to_apply[0].RGBA[1] = 0.0f;
    zlights_to_apply[0].RGBA[2] = 1.0f;
    zlights_to_apply[0].RGBA[3] = 1.0f;
    zlights_to_apply[0].reach = 15.0f;
    zlights_to_apply[0].ambient = 0.0;
    zlights_to_apply[0].diffuse = 8.0;
    zlights_to_apply_size += 1;
    assert(zlights_to_apply_size <= ZLIGHTS_TO_APPLY_ARRAYSIZE);
    
    zlights_to_apply[1].x = -50.0f;
    zlights_to_apply[1].y = -10.0f;
    zlights_to_apply[1].z = 40.0f;
    zlights_to_apply[1].RGBA[0] = 0.0f;
    zlights_to_apply[1].RGBA[1] = 1.0f;
    zlights_to_apply[1].RGBA[2] = 0.0f;
    zlights_to_apply[1].RGBA[3] = 1.0f;
    zlights_to_apply[1].reach = 15.0f;
    zlights_to_apply[1].ambient = 0.0;
    zlights_to_apply[1].diffuse = 8.0;
    zlights_to_apply_size += 1;
    assert(zlights_to_apply_size <= ZLIGHTS_TO_APPLY_ARRAYSIZE);
   
    font_height = 0.025f;
    
    TexQuad sample_pic;
    sample_pic.texturearray_i = 2;
    sample_pic.texture_i = 0;
    sample_pic.left = 0.75;
    sample_pic.top = 0.5f;
    sample_pic.width = 0.5f;
    sample_pic.height = 0.5f;
    for (uint32_t c = 0; c < 4; c++) {
        sample_pic.RGBA[c] = 1.0f;
    }
    sample_pic.visible = true;
    request_texquad_renderable(&sample_pic);
    
    printf("finished client_logic_startup()\n");    
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
    float elapsed_mod =
        (float)((double)microseconds_elapsed / (double)16666);
    float cam_speed = 0.25f * elapsed_mod;
    float cam_rotation_speed = 0.05f * elapsed_mod;
    
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
char fps_string[7] = "fps: xx";
void client_logic_update(
    uint64_t microseconds_elapsed)
{
    // TODO: microseconds_elapsed is overridden here because
    // TODO: our timer is weirdly broken on iOS. Fix it!
    // microseconds_elapsed = 16666;
    uint64_t fps = 1000000 / microseconds_elapsed;
    // printf("fps: %u\n", fps);
    float elapsed_mod =
        (float)((double)microseconds_elapsed / (double)16666);
   
    // move_texquad_object(
    //     /* with_object_id : */ label_object_id,
    //     /* float delta_x  : */ 0.003f * elapsed_mod,
    //     /* float delta_y  : */ 0.0f * elapsed_mod);
    delete_texquad_object(label_object_id);

    if (fps < 100) {
        fps_string[5] = '0' + ((fps / 10) % 10);
        fps_string[6] = '0' + (fps % 10);
    } else {
        fps_string[5] = '9' + ((fps / 10) % 10);
        fps_string[6] = '9' + (fps % 10);
    }
    request_label_renderable(
        /* with_id              : */ label_object_id,
        /* char * text_to_draw  : */ &fps_string,
        /* text_to_draw_size    : */ 7,
        /* float left           : */ -0.95f,
        /* float top            : */ -0.85,
        /* float max_width      : */ 0.5f);
    
    client_handle_keypresses(
        microseconds_elapsed);
    client_handle_touches(
        microseconds_elapsed);
}

