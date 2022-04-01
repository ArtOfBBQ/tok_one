#include "clientlogic.h"

TextureArray texture_arrays[TEXTUREARRAYS_SIZE];
zPolygon * zpolygons_to_render[1000];
uint32_t zpolygons_to_render_size = 0;

#define ZLIGHTS_TO_APPLY_ARRAYSIZE 50
zLightSource zlights_to_apply[ZLIGHTS_TO_APPLY_ARRAYSIZE];
uint32_t zlights_to_apply_size;

DecodedImage minimap;

bool32_t handled_minimap_toggle = false;

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

uint32_t img_xy_to_pixel(
    const uint32_t x,
    const uint32_t y,
    DecodedImage * img)
{
    uint32_t return_value = 0;
    
    assert(x > 0);
    assert(x < img->width);
    assert(y > 0);
    assert(y < img->height);
    
    return_value += ((x - 1) * 4);
    return_value += ((y - 1) * 4 * img->width);
    
    assert(return_value >= 0);
    assert(return_value < img->rgba_values_size);
    
    return return_value;
}

void decodedimg_add_pixel(
    DecodedImage * to_modify,
    const float x,
    const float y,
    const uint8_t red,
    const uint8_t green,
    const uint8_t blue)
{
    uint32_t cam_reach = MINIMAP_PIXELS_WIDTH / 2;
    
    int32_t i_x = (uint32_t)(x + cam_reach);
    int32_t i_y = (uint32_t)(y + cam_reach);
    
    if (i_x < 0 || i_y < 0) {
        return;
    }
    
    uint32_t ui_x = (uint32_t)i_x;
    uint32_t ui_y = (uint32_t)i_y;
    
    if (ui_x < 1 || ui_x >= to_modify->width) { return; }
    if (ui_y < 1 || ui_y >= to_modify->height) { return; }
    
    uint32_t location = img_xy_to_pixel(
        /* x: */ ui_x,
        /* y: */ ui_y,
        /* img: */ to_modify);
    
    to_modify->rgba_values[location] = 255;
    to_modify->rgba_values[location + 1] = red;
    to_modify->rgba_values[location + 2] = green;
    to_modify->rgba_values[location + 3] = blue;
}

void decodedimg_add_triangle(
    DecodedImage * to_modify,
    zTriangle * to_add)
{
    if (to_add == NULL) { return; }
    
    float avg_x =
        (to_add->vertices[0].x +
        to_add->vertices[1].x +
        to_add->vertices[2].x) / 3.0f;
    float avg_z =
        (to_add->vertices[0].z +
        to_add->vertices[1].z +
        to_add->vertices[2].z) / 3.0f;
    
    decodedimg_add_pixel(
        to_modify,
        avg_x,
        avg_z,
        255,
        255,
        255);
}

void decodedimg_add_zpolygon(
    DecodedImage * to_modify,
    zPolygon * to_add)
{
    if (to_add == NULL) { return; }
    
    decodedimg_add_pixel(
        to_modify,
        to_add->x,
        to_add->z,
        255,
        255,
        255);
}

void decodedimg_add_camera(
    DecodedImage * to_modify,
    zCamera * to_add)
{
    decodedimg_add_pixel(
        to_modify,
        to_add->x,
        to_add->z,
        255,
        0,
        255);
    
    // draw field of view somehow
    //
    //       cam looking striaght down (Y angle is 0)
    //        *
    //       **
    //      * *
    //     P***   P2
    //
    // angle_right is just cam's y angle + (3.14 / 4)
    // we know the angle and the distance (the hypotenuse)
    // we want to know the opposite and the adjacent
    // we can get adjacent with CAH and opposite with SOH
    //
    // if C = A/H then A = C * H
    // if S = O/H then O = S * H
    for (
        float hypotenuse = 5.0f;
        hypotenuse < 41.0f;
        hypotenuse += 5.0f)
    {
        float angle_right =
            to_add->y_angle + (
                projection_constants.field_of_view_rad / 2);
        float angle_left =
            to_add->y_angle + (
                projection_constants.field_of_view_rad / 2);
        
        float adjacent = cosf(angle_right) * hypotenuse;
        float opposite = sinf(angle_right) * hypotenuse;
        
        decodedimg_add_pixel(
            to_modify,
            to_add->x + opposite,
            to_add->z + adjacent,
            20,
            20,
            250);
        
        angle_right =
            to_add->y_angle - (
                projection_constants.field_of_view_rad / 2);
        angle_left =
            to_add->y_angle - (
                projection_constants.field_of_view_rad / 2);
        
        adjacent = cosf(angle_right) * hypotenuse;
        opposite = sinf(angle_right) * hypotenuse;
        decodedimg_add_pixel(
            to_modify,
            to_add->x + opposite,
            to_add->z + adjacent,
            20,
            20,
            250);
    }
}

void minimaps_clear(void)
{
    for (
        uint32_t i = 0;
        i < minimap.rgba_values_size;
        i += 4)
    {
        if
        (
            i % (minimap.width * 4) == 0
            ||
            (i % (minimap.width * 4)) == ((minimap.width * 4) - 4)
            ||
            i / (minimap.width * 4) == 0
            ||
            (i / (minimap.width * 4) == (minimap.height - 1))
        )
        {
            minimap.rgba_values[i] = 220;
            minimap.rgba_values[i+1] = 220;
            minimap.rgba_values[i+2] = 220;
            minimap.rgba_values[i+3] = 255;
        } else {
            minimap.rgba_values[i] = 30;
            minimap.rgba_values[i+1] = 30;
            minimap.rgba_values[i+2] = 30;
            minimap.rgba_values[i+3] = 255;
        }
    }
}

void client_logic_startup() {
    
    // These are some example texture atlases we're using for
    // texture mapping on cards and cubes
    assert(TEXTUREARRAYS_SIZE > 0);
    texture_arrays[0].sprite_columns = 16;
    texture_arrays[0].sprite_rows = 16;
    texture_arrays[0].request_update = false;
    texture_arrays[1].sprite_columns = 3;
    texture_arrays[1].sprite_rows = 2;
    texture_arrays[1].request_update = false;
    texture_arrays[2].sprite_columns = 1;
    texture_arrays[2].sprite_rows = 1;
    texture_arrays[2].request_update = false;
    
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
    zlights_to_apply[0].y = -10.0f;
    zlights_to_apply[0].z = 200.0f;
    zlights_to_apply[0].RGBA[0] = 1.0f;
    zlights_to_apply[0].RGBA[1] = 0.05;
    zlights_to_apply[0].RGBA[2] = 1.0f;
    zlights_to_apply[0].RGBA[3] = 1.0f;
    zlights_to_apply[0].reach = 15.0f;
    zlights_to_apply[0].ambient = 0.05;
    zlights_to_apply[0].diffuse = 4.0;
    zlights_to_apply_size += 1;
    
    zlights_to_apply[1].x = -100.0f;
    zlights_to_apply[1].y = -10.0f;
    zlights_to_apply[1].z = 200.0f;
    zlights_to_apply[1].RGBA[0] = 0.05;
    zlights_to_apply[1].RGBA[1] = 1.0f;
    zlights_to_apply[1].RGBA[2] = 0.05;
    zlights_to_apply[1].RGBA[3] = 1.0f;
    zlights_to_apply[1].reach = 15.0f;
    zlights_to_apply[1].ambient = 0.05;
    zlights_to_apply[1].diffuse = 4.0;
    zlights_to_apply_size += 1;
    
    for (uint32_t l = 0; l < 2; l++) {
        zpolygons_to_render_size += 1;
        uint32_t light_i = zpolygons_to_render_size - 1;
        zpolygons_to_render[light_i] = get_box();
        zpolygons_to_render[light_i]->x = zlights_to_apply[0].x;
        zpolygons_to_render[light_i]->y = zlights_to_apply[0].y;
        zpolygons_to_render[light_i]->z = zlights_to_apply[0].z;
        
        // add a cubes to represent the light sources
        for (
            uint32_t j = 0;
            j < zpolygons_to_render[light_i]->triangles_size;
            j++)
        {
            // mimic the color of the associated light source
            zpolygons_to_render[light_i]->triangles[j].color[0] =
                zlights_to_apply[l].RGBA[0] * 7.5f;
            zpolygons_to_render[light_i]->triangles[j].color[1] =
                zlights_to_apply[l].RGBA[1] * 7.5f;
            zpolygons_to_render[light_i]->triangles[j].color[2] =
                zlights_to_apply[l].RGBA[2] * 7.5f;
            zpolygons_to_render[light_i]->triangles[j].color[3] =
                1.0f;
            zpolygons_to_render[light_i]
                ->triangles[j].texturearray_i = -1;
            zpolygons_to_render[light_i]
                ->triangles[j].texture_i = -1;
        }
    }
    
    
    // add 2 2D bitmaps representing minimaps
    minimap.width = MINIMAP_PIXELS_WIDTH;
    minimap.height = MINIMAP_PIXELS_WIDTH;
    minimap.rgba_values_size =
        minimap.height * minimap.width * 4;
    minimap.rgba_values = malloc(minimap.rgba_values_size);
    
    for (
        uint32_t i = 0;
        i < minimap.rgba_values_size;
        i += 4)
    {
        minimap.rgba_values[i+0] = i % 50;
        minimap.rgba_values[i+1] = 25;
        minimap.rgba_values[i+2] = (i + 125) % 75;
        minimap.rgba_values[i+3] = 255;
    }

    float minimap_height = 0.5f;
    float minimap_width =
        projection_constants.aspect_ratio * minimap_height;
    float minimap_offset = 0.05f;
    float minimap_left = 1.0f - minimap_offset - minimap_width;
    float minimap_top = -1.0f + minimap_offset + minimap_height;
    
    texquads_to_render_size += 1;
    texquads_to_render[0].texturearray_i = 2;
    texquads_to_render[0].texture_i = 0;
    texquads_to_render[0].left = minimap_left;
    texquads_to_render[0].width = minimap_width;
    texquads_to_render[0].top = minimap_top;
    texquads_to_render[0].height = minimap_height;
    texquads_to_render[0].visible = true;
}

void client_logic_update()
{
    uint64_t elapsed_since_previous_frame =
    platform_end_timer_get_nanosecs();
    
    platform_start_timer();
    
    // uint64_t fps = 1000000000 / elapsed_since_previous_frame;
    // printf("fps: %llu\n", fps);
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
    
    if (keypress_map[46] == true)
    {
        // m key is pressed

        if (handled_minimap_toggle == false) {
            texquads_to_render[0].visible =
                texquads_to_render[0].visible ? false : true;
            handled_minimap_toggle = true;
        }
    } else {
        handled_minimap_toggle = false;
    }
    
    // animate objects
    for (
        uint32_t i = 0;
        i < zpolygons_to_render_size;
        i++)
    {
        zpolygons_to_render[i]->x -= 0.005f;
        zpolygons_to_render[i]->z_angle += 0.03f;
        zpolygons_to_render[i]->x_angle += 0.021f;
        zpolygons_to_render[i]->y_angle += 0.015f;
    }
    
    // move our light sources
    for (uint32_t i = 0; i < 2; i++) {
        uint32_t assoc_light_i = zpolygons_to_render_size - 2 + i;
        
        assert(assoc_light_i < zpolygons_to_render_size);
        if (
            zlights_to_apply[i].z > -10.0f)
        {
            zlights_to_apply[i].z -= 0.25f;
        }
        
        zpolygons_to_render[assoc_light_i]->x =
            zlights_to_apply[i].x;
        zpolygons_to_render[assoc_light_i]->y =
            zlights_to_apply[i].y;
        zpolygons_to_render[assoc_light_i]->z =
            zlights_to_apply[i].z;
    }
    
    // update minimaps
    minimaps_clear();
    
    decodedimg_add_camera(
        &minimap,
        &camera);
    for (
        uint32_t i = 0;
        i < zpolygons_to_render_size;
        i++)
    {
        decodedimg_add_zpolygon(
            &minimap,
            zpolygons_to_render[i]);
    }
    
    texture_arrays[2].request_update = true;
}

