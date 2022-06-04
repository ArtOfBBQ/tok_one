#include "texture_array.h"

TextureArray texture_arrays[TEXTUREARRAYS_SIZE];
uint32_t texture_arrays_size = 0;

void update_texturearray_from_0terminated_files(
    const int32_t texturearray_i,
    const char filenames
        [MAX_FILES_IN_SINGLE_TEXARRAY]
        [MAX_ASSET_FILENAME_SIZE])
{
    assert(
        texturearray_i < TEXTUREARRAYS_SIZE);
    
    uint32_t filenames_size = 0;
    uint32_t t_i = 0;
    while (filenames[t_i][0] != '\0')
    {
        filenames_size += 1;
        t_i++;
    }
    
    if (filenames_size == 0) {
        log_append("WARNING: requested a texture update at ");
        log_append_uint(texturearray_i);
        log_append(" but 0 textures were passed!\n");
        return;
    }
    
    DecodedImage * decoded_images[filenames_size];
    
    for (t_i = 0; t_i < filenames_size; t_i++)
    {
        const char * filename = filenames[t_i];
        
        DecodedImage * new_image = malloc_img_from_filename(
            filename);
        
        decoded_images[t_i] = new_image;
    }
    
    texture_arrays[texturearray_i].image =
        (DecodedImage *)malloc(sizeof(DecodedImage));
    
    *(texture_arrays[texturearray_i].image) =
        concatenate_images(
            /* const DecodedImage ** images_to_concat: */
                (DecodedImage **)decoded_images,
            /* images_to_concat_size: */
                t_i,
            /* out_sprite_rows: */
                &texture_arrays[texturearray_i].sprite_rows,
            /* out_sprite_columns: */
                &texture_arrays[texturearray_i].sprite_columns);
    
    if (texture_arrays[texturearray_i].image->pixel_count < 5) {
        texture_arrays[texturearray_i].sprite_rows = 1;
        texture_arrays[texturearray_i].sprite_columns = 1;
    }
    
    assert(texture_arrays[texturearray_i].image->width > 0);
    assert(texture_arrays[texturearray_i].image->height > 0);
    texture_arrays[texturearray_i].request_update = true;
}

// returns new_texture_array_i (index in texture_arrays)
void register_new_texturearray_from_files(
    const char ** filenames,
    const uint32_t filenames_size)
{
    uint32_t decoded_images_size = filenames_size;
    DecodedImage * decoded_images[decoded_images_size];   
    
    for (
        uint32_t i = 0;
        i < filenames_size;
        i++)
    {
        const char * filename = filenames[i];
        
        DecodedImage * new_image =
            malloc_img_from_filename(filename);
        
        decoded_images[i] = new_image;
    }
    
    DecodedImage ** decoded_images_dblptr =
        decoded_images;
    register_new_texturearray_from_images(
        /* DecodedImage ** new_images : */
            decoded_images_dblptr,
        /* new_images_size: */
            decoded_images_size);
}

void register_new_texturearray_from_images(
    DecodedImage ** new_images,
    const uint32_t new_images_size)
{
    assert(new_images_size > 0);
    for (uint32_t i = 0; i < new_images_size; i++) {
        assert(new_images[i] != NULL);
        assert(new_images[i]->width > 0);
        assert(new_images[i]->height > 0);
        assert(new_images[i]->rgba_values_size > 0);
        assert(new_images[i]->rgba_values != NULL);
    }
    
    uint32_t current_width = new_images[0]->width;
    uint32_t current_height = new_images[0]->height;
    if (
        current_width == 0
        || current_height == 0)
    {
        log_append("ERROR - register images with width/height 0\n");
        log_dump_and_crash();
    }
    
    if (current_width > 100000 || current_height > 100000) {
        log_append("ERROR - tried register images with excessive width/height: ");
        log_append_uint(current_width);
        log_append("x");
        log_append_uint(current_height);
        log_append("\n");
        log_dump_and_crash();
    }
    
    for (
        uint32_t i = 0;
        i < new_images_size;
        i++)
    {
        assert(new_images[i] != NULL);
        assert(new_images[i]->good);
        assert(new_images[i]->rgba_values_size > 0);
        assert(new_images[i]->width == current_width);
        assert(new_images[i]->height == current_height);
    }
    
    // set up a new texturearray that's big enough to hold
    // x images
    int32_t new_i = (int32_t)texture_arrays_size;
    assert(new_i < TEXTUREARRAYS_SIZE);
    texture_arrays_size += 1;
    
    // fill in the images in a new texturearray
    if (texture_arrays[new_i].image != NULL) {
        free(texture_arrays[new_i].image);
    }
    
    texture_arrays[new_i].image =
        (DecodedImage *)malloc(sizeof(DecodedImage));
    *(texture_arrays[new_i].image) =
        concatenate_images(
            /* const DecodedImage ** images_to_concat: */
                (DecodedImage **)new_images,
            /* images_to_concat_size: */
                new_images_size,
            /* out_sprite_rows: */
                &texture_arrays[new_i].sprite_rows,
            /* out_sprite_columns: */
                &texture_arrays[new_i].sprite_columns);
    assert(texture_arrays[new_i].image->width > 0);
    assert(texture_arrays[new_i].image->height > 0);
    texture_arrays[new_i].request_update = true;
}

void register_new_texturearray(
    DecodedImage * new_image)
{
    assert(new_image != NULL);
    assert(new_image->width > 0);
    assert(new_image->height > 0);
    assert(new_image->rgba_values_size > 0);
    DecodedImage * images[1];
    images[0] = new_image;
    DecodedImage ** images_dblptr = images;
    assert(images[0] != NULL);
    assert(images[0]->width > 0);
    assert(images[0]->height > 0);
    assert(images[0]->rgba_values_size > 0);
    assert(images[0]->rgba_values != NULL);
    register_new_texturearray_from_images(
        images_dblptr,
        1);
}

DecodedImage * extract_image(
    TextureArray * texture_array,
    uint32_t x,
    uint32_t y)
{
    assert(x > 0);
    assert(y > 0);
    assert(x <= texture_array->sprite_columns);
    assert(y <= texture_array->sprite_rows);
    
    DecodedImage * new_image =
        (DecodedImage *)malloc(sizeof(DecodedImage));
    
    uint32_t slice_size =
        texture_array->image->rgba_values_size
            / texture_array->sprite_columns
            / texture_array->sprite_rows;
    uint32_t slice_width =
        texture_array->image->width
            / texture_array->sprite_columns;
    uint32_t slice_height =
        texture_array->image->height
            / texture_array->sprite_rows;
    
    new_image->rgba_values_size = slice_size;
    new_image->rgba_values =
        (uint8_t *)malloc(slice_size);
    
    new_image->width = slice_width;
    new_image->height = slice_height;
    
    uint32_t start_x = 1 + ((x - 1) * slice_width);
    uint32_t start_y = 1 + ((y - 1) * slice_height);
    uint32_t end_y = start_y + slice_height;
    
    uint32_t i = 0;
    for (
        uint32_t cur_y = start_y;
        cur_y < end_y;
        cur_y++)
    {
        // get the pixel that's at [start_x, cur_y]
        // copcur_y slice_width pixels
        uint32_t pixel_i =
            ((start_x - 1) * 4)
                + ((cur_y - 1) * texture_array->image->width * 4);
        assert(i < new_image->rgba_values_size);
        for (
            uint32_t _ = 0;
            _ < (slice_width * 4);
            _++)
        {
            assert(
                (pixel_i + _)
                    < texture_array->image->rgba_values_size);
            new_image->rgba_values[i] =
                texture_array->image->rgba_values[pixel_i + _];
            i++;
        }
    }
    
    return new_image;
}

/*
Error handling function: when we fail to load an image,
use this hardcoded 4-pixel image image as a replacement
rather than crashing the application
*/
static void set_allocated_to_error_image(
    DecodedImage * to_replace)
{
    to_replace->good = true;
    
    assert(
        (to_replace->width * to_replace->height) ==
            to_replace->pixel_count);
    assert(
        to_replace->pixel_count * 4 ==
            to_replace->rgba_values_size);
    
    bool32_t black = true;
    for (
        uint32_t pixel_i = 0;
        pixel_i < to_replace->pixel_count;
        pixel_i++)
    {
        assert(
            ((pixel_i * 4) + 3) < to_replace->rgba_values_size);
        
        to_replace->rgba_values[(pixel_i * 4) + 0] =
            black ? 0 : 255;
        to_replace->rgba_values[(pixel_i * 4) + 1] = 0;
        to_replace->rgba_values[(pixel_i * 4) + 2] = 0;
        to_replace->rgba_values[(pixel_i * 4) + 3] = 255;
        black = !black;
    }
}

/*
Error handling function: when we fail to load an image,
use this hardcoded 4-pixel image image as a replacement
rather than crashing the application
*/
static void set_unalloated_to_error_image(
    DecodedImage * to_replace)
{
    to_replace->pixel_count = 4;
    to_replace->rgba_values_size = 16;
    to_replace->width = 2;
    to_replace->height = 2;
    
    to_replace->rgba_values =
        (uint8_t *)malloc(16 * sizeof(uint8_t));
    
    set_allocated_to_error_image(to_replace);
}

DecodedImage * malloc_img_from_filename(
    const char * filename)
{
    FileBuffer file_buffer;
    file_buffer.size = platform_get_resource_size(filename) + 1;
    
    assert(file_buffer.size > 1);
    file_buffer.contents =
        (char *)malloc(sizeof(char) * file_buffer.size);
    
    platform_read_resource_file(
        filename,
        &file_buffer);
    
    if (!file_buffer.good) {
        log_append("platform failed to read file: ");
        log_append(filename);
        log_append("\n");
        assert(0);
    }
    
    DecodedImage * new_image =
        (DecodedImage *)malloc(sizeof(DecodedImage));
    
    get_PNG_width_height(
        /* uint8_t * compressed_bytes: */
            (uint8_t *)file_buffer.contents,
        /* uint32_t compressed_bytes_size: */
            50,
        /* uint32_t * width_out: */
            &new_image->width,
        /* uint32_t * height_out: */
            &new_image->height);
    
    if (new_image->width == 0 || new_image->height == 0) {
        assert(0);
        set_unalloated_to_error_image(new_image);
        free(file_buffer.contents);
        return new_image;
    }
    
    new_image->pixel_count =
        new_image->width * new_image->height;
    new_image->rgba_values_size = new_image->pixel_count * 4;
    new_image->rgba_values = (uint8_t *)malloc(
        new_image->rgba_values_size);
    new_image->good = false;
    decode_PNG(
        /* compressed_bytes: */
            (uint8_t *)file_buffer.contents,
        /* compressed_bytes_size: */
            (uint32_t)(file_buffer.size - 1),
        /* DecodedImage * out_preallocated_png: */
            new_image);
    
    if (!new_image->good) {
        set_allocated_to_error_image(new_image);
        free(file_buffer.contents);
        return new_image;
    }
    
    if (new_image->pixel_count * 4 !=
        new_image->rgba_values_size)
    {
        log_append(
            "ERROR: loaded an image with with pixel_count of ");
        log_append_uint(new_image->pixel_count);
        log_append(", which should be *4 = ");
        log_append_uint(new_image->pixel_count * 4);
        log_append(", and rgba_values_size of ");
        log_append_uint(new_image->rgba_values_size);
        log_append(". Image dimensions were [");
        log_append_uint(new_image->width);
        log_append(",");
        log_append_uint(new_image->height);
        log_append(", so width*height*4 would have been: ");
        log_append_uint(new_image->width * new_image->height * 4);
        log_append("\n");
        log_dump_and_crash();
    }
    
    if (get_sum_rgba(new_image) < 1) {
        log_append("new_image's summed rgba value was only: ");
        log_append_uint(get_sum_rgba(new_image));
        log_append("\n");
        log_dump_and_crash();
    }
    
    free(file_buffer.contents);
    
    assert(new_image->pixel_count ==
        new_image->width * new_image->height);
    assert(new_image->rgba_values_size ==
        new_image->pixel_count * 4);
    
    return new_image;
}
