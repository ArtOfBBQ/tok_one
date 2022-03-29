#include "clientlogic.h"

TextureArray texture_arrays[TEXTUREARRAYS_SIZE];
zPolygon * zpolygons_to_render[1000];
uint32_t zpolygons_to_render_size = 0;

zLightSource zlights_to_apply[50];
uint32_t zlights_to_apply_size;

DecodedImage minimap;
DecodedImage minimap2;
bool32_t handled_minimap_toggle = false;
bool32_t minimaps_visible = true;

zPolygon * load_from_obj_file(char * filename)
{
    FileBuffer * buffer = platform_read_file(filename);
    
    zPolygon * return_value = load_from_obj(
        /* rawdata     : */ buffer->contents,
        /* rawdata_size: */ buffer->size);

    free(buffer->contents);
    free(buffer);
    
    return return_value;
}

void client_logic_startup() {
    
    // These are some example texture atlases we're using for
    // texture mapping on cards and cubes
    assert(TEXTUREARRAYS_SIZE > 0);
    texture_arrays[0].sprite_columns = 16;
    texture_arrays[0].sprite_rows = 16;
    texture_arrays[1].sprite_columns = 3;
    texture_arrays[1].sprite_rows = 2;
    texture_arrays[2].sprite_columns = 1;
    texture_arrays[2].sprite_rows = 1;
    texture_arrays[3].sprite_columns = 1;
    texture_arrays[3].sprite_rows = 1;
    
    FileBuffer * file_buffer;
    
    #define TEXTURE_FILENAMES_SIZE 2
    char * texture_filenames[TEXTURE_FILENAMES_SIZE] = {
        "phoebus.png",
        "sampletexture.png"};
    for (
        uint32_t i = 0;
        i < TEXTURE_FILENAMES_SIZE;
        i++)
    {
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
        free(file_buffer->contents);
        free(file_buffer);
        printf(
            "read texture %s with width %u\n",
            texture_filenames[i],
            texture_arrays[i].image->width);
    }
    
    texture_arrays[2].image = &minimap;
    texture_arrays[3].image = &minimap2;
    
    // initialize zPolygon objects, the 3-D objects we're
    // planning to render
    zpolygons_to_render_size = 0;
    
    // objects part 1: load some cards from object file 
    for (uint32_t i = 0; i < 3; i++) {
        uint32_t last_i = zpolygons_to_render_size;
        zpolygons_to_render_size += 1;
        zpolygons_to_render[last_i] =
            i == 2 ?
            load_from_obj_file("teapot.obj")
            : load_from_obj_file("cardwithuvcoords.obj");
        
        float base_y = i % 2 == 0 ? 0.0f : -5.0f;
        zpolygons_to_render[last_i]->x = -2.0f + (4.0f * i);
        zpolygons_to_render[last_i]->y =
            i == 2 ?
            0.0f
            : base_y + (last_i * 7.0f);
        zpolygons_to_render[last_i]->z = i == 2 ? -25.0f : 28.0f;
        
        // change face texture but not the back texture
        for (
            uint32_t t = 0;
            t < zpolygons_to_render[last_i]->triangles_size;
            t++)
        {
            if (
                zpolygons_to_render[last_i]
                    ->triangles[t].texture_i == 2) 
            {
                zpolygons_to_render[last_i]->triangles[t]
                    .texture_i = i;
            }
        }
        
        // scale_zpolygon(
        //     /* to_scale   : */
        //         zpolygons_to_render[last_i],
        //     /* new_height : */
        //         2.0f);
    }
    
    // objects 2: load some hard-coded cubes
    for (uint32_t i = 3; i < 4; i++) {
        uint32_t last_i = zpolygons_to_render_size;
        zpolygons_to_render[last_i] = get_box();
        scale_zpolygon(
            /* to_scale   : */
                zpolygons_to_render[last_i],
            /* new_height : */
                10.0f);
        zpolygons_to_render_size += 1;
    }
    
    // initialize global zLightSource objects, to set up
    // our lighting for the scene
    zlights_to_apply[0].x = 50.0f;
    zlights_to_apply[0].y = 2.5f;
    zlights_to_apply[0].z = 50.0f;
    zlights_to_apply[0].reach = 1.0f;
    zlights_to_apply[0].ambient = 8.0;
    zlights_to_apply[0].diffuse = 8.0;
    zlights_to_apply_size = 1;
    
    // add a white cube to represent the light source
    zpolygons_to_render_size += 1;
    uint32_t light_i = zpolygons_to_render_size - 1;
    zpolygons_to_render[light_i] = get_box();
    zpolygons_to_render[light_i]->x = zlights_to_apply[0].x;
    zpolygons_to_render[light_i]->y = zlights_to_apply[0].y;
    zpolygons_to_render[light_i]->z = zlights_to_apply[0].z;
    for (
        uint32_t j = 0;
        j < zpolygons_to_render[light_i]->triangles_size;
        j++)
    {
        for (uint32_t v = 0; v < 3; v++) {
            // bright white
            zpolygons_to_render[light_i]->triangles[j].color[0] =
                500.0f;
            zpolygons_to_render[light_i]->triangles[j].color[1] =
                500.0f;
            zpolygons_to_render[light_i]->triangles[j].color[2] =
                500.0f;
            zpolygons_to_render[light_i]->triangles[j].color[3] =
                1.0f;
            zpolygons_to_render[light_i]
                ->triangles[j].texturearray_i = -1;
            zpolygons_to_render[light_i]
                ->triangles[j].texture_i = -1;
        }
    }
    // scale_zpolygon(
    //     /* to_scale  : */ zpolygons_to_render[light_i],
    //     /* new_height: */ 0.5f);
}

void client_logic_update()
{
    float cam_speed = 0.25f;
    float cam_rotation_speed = 0.05f;
    
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
        // camera.y_angle -= cam_rotation_speed;
        camera.x -= cam_speed;
    }
    
    if (keypress_map[124] == true)
    {
        // right arrow key
        // camera.y_angle += cam_rotation_speed;
        camera.x += cam_speed;
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
    
    if (keypress_map[46] == true)
    {
        // m key is pressed

        if (handled_minimap_toggle == false) {
            minimaps_visible = minimaps_visible ? false : true;
            handled_minimap_toggle = true;
        }
    } else {
        handled_minimap_toggle = false;
    }
    
}

