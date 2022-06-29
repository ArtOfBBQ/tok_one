#include "texture_array.h"

TextureArray texture_arrays[TEXTUREARRAYS_SIZE];
uint32_t texture_arrays_size = 0;

/*
Error handling function: when we fail to load an image,
use this hardcoded image as a replacement rather than
crashing the application
*/
static void set_allocated_to_error_image(
    DecodedImage * to_replace)
{
    to_replace->good = true;
    
    log_assert(
        (to_replace->width * to_replace->height) ==
            to_replace->pixel_count);
    log_assert(
        to_replace->pixel_count * 4 ==
            to_replace->rgba_values_size);
    
    bool32_t black = true;
    for (
        uint32_t pixel_i = 0;
        pixel_i < to_replace->pixel_count;
        pixel_i++)
    {
        log_assert(
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
static void set_unallocated_to_error_image(
    DecodedImage * to_replace)
{
    log_assert(to_replace->rgba_values == NULL);
    
    to_replace->width = 30;
    to_replace->height = 30;
    to_replace->pixel_count =
        to_replace->width * to_replace->height;
    to_replace->rgba_values_size = to_replace->pixel_count * 4;
    
    to_replace->rgba_values =
        malloc_from_unmanaged(to_replace->rgba_values_size);
    
    set_allocated_to_error_image(to_replace);
}

static DecodedImage *
malloc_img_from_filename_with_working_memory(
    const char * filename,
    const uint8_t * dpng_working_memory,
    const uint64_t dpng_working_memory_size)
{
    FileBuffer file_buffer;
    file_buffer.size = platform_get_resource_size(filename) + 1;
    
    log_assert(file_buffer.size > 1);
    file_buffer.contents =
        (char *)malloc_from_managed(sizeof(char) * file_buffer.size);
    
    platform_read_resource_file(
        filename,
        &file_buffer);
    
    if (!file_buffer.good) {
        log_append("platform failed to read file: ");
        log_append(filename);
        log_append("\n");
        free_from_managed((uint8_t *)file_buffer.contents);
    }
    log_assert(file_buffer.good);
    
    DecodedImage * new_image = malloc_struct_from_unmanaged(DecodedImage);
    
    get_PNG_width_height(
        /* const uint8_t * compressed_input: */
            (uint8_t *)file_buffer.contents,
        /* const uint64_t compressed_input_size: */
            file_buffer.size - 1,
        /* uint32_t * out_width: */
            &new_image->width,
        /* uint32_t * out_height: */
            &new_image->height,
        /* uint32_t * out_good: */
            &new_image->good);
    
    if (!new_image->good) {
        set_unallocated_to_error_image(new_image);
        free_from_managed((uint8_t *)file_buffer.contents);
        return new_image;
    }
    
    new_image->good = false;
    new_image->pixel_count = new_image->width * new_image->height;
    new_image->rgba_values_size = new_image->pixel_count * 4;
    new_image->rgba_values = malloc_from_unmanaged(new_image->rgba_values_size);
    
    decode_PNG(
        /* const uint8_t * compressed_input: */
            (uint8_t *)file_buffer.contents,
        /* const uint64_t compressed_input_size: */
            file_buffer.size - 1,
        /* out_rgba_values: */
            new_image->rgba_values,
        /* rgba_values_size: */
            new_image->rgba_values_size,
        /* dpng_working_memory: */
            dpng_working_memory,
        /* dpng_working_memory_size: */
            dpng_working_memory_size,
        /* uint32_t * out_good: */
            &new_image->good);
    
    free_from_managed((uint8_t *)file_buffer.contents);
    
    if (!new_image->good) {
        set_allocated_to_error_image(new_image);
        return new_image;
    }
    
    log_append("after decoding PNG - memory_store_size: ");
    log_append_uint((uint32_t)unmanaged_memory_size);
    log_append("\n");
    
    log_assert(new_image->pixel_count * 4 == new_image->rgba_values_size);
    log_assert(get_sum_rgba(new_image) > 0);
    
    log_assert(new_image->pixel_count == new_image->width * new_image->height);
    
    return new_image;
}

DecodedImage * malloc_img_from_filename(
    const char * filename)
{
    uint64_t dpng_working_memory_size = 10000000;
    uint8_t * dpng_working_memory =
        malloc_from_managed(dpng_working_memory_size);
    
    DecodedImage * return_value =
        malloc_img_from_filename_with_working_memory(
            filename,
            dpng_working_memory,
            dpng_working_memory_size);
    
    free_from_managed(dpng_working_memory);
    log_assert(return_value->good);
    
    return return_value;
}

void update_texturearray_from_0terminated_files(
    const int32_t texturearray_i,
    const char filenames
        [MAX_FILES_IN_SINGLE_TEXARRAY]
        [MAX_ASSET_FILENAME_SIZE])
{
    uint64_t dpng_working_memory_size = 10000000;
    uint8_t * dpng_working_memory =
        malloc_from_managed(dpng_working_memory_size);
    
    update_texturearray_from_0terminated_files_with_memory(
        texturearray_i,
        filenames,
        dpng_working_memory,
        dpng_working_memory_size);
    
    free_from_managed(dpng_working_memory);
}

void update_texturearray_from_0terminated_files_with_memory(
    const int32_t texturearray_i,
    const char filenames
        [MAX_FILES_IN_SINGLE_TEXARRAY]
        [MAX_ASSET_FILENAME_SIZE],
    const uint8_t * dpng_working_memory,
    const uint64_t dpng_working_memory_size)
{
    log_assert(
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
        
        DecodedImage * new_image =
            malloc_img_from_filename_with_working_memory(
                filename,
                dpng_working_memory,
                dpng_working_memory_size);
        log_assert(new_image->good);
        decoded_images[t_i] = new_image;
    }
    
    texture_arrays[texturearray_i].image =
        (DecodedImage *)malloc_from_unmanaged(sizeof(DecodedImage));
    
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
    
    log_assert(texture_arrays[texturearray_i].image->width > 0);
    log_assert(texture_arrays[texturearray_i].image->height > 0);
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
        DecodedImage * new_image;
        if (platform_resource_exists(filenames[i])) {
            new_image =
                malloc_img_from_filename(filenames[i]);
        } else {
            new_image = (DecodedImage *)malloc_from_unmanaged(sizeof(DecodedImage));
            new_image->good = false;
            set_unallocated_to_error_image(new_image);
        }
        
        log_assert(new_image->good);
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
    log_assert(new_images_size > 0);
    for (uint32_t i = 0; i < new_images_size; i++) {
        log_assert(new_images[i] != NULL);
        log_assert(new_images[i]->width > 0);
        log_assert(new_images[i]->height > 0);
        log_assert(new_images[i]->rgba_values_size > 0);
        log_assert(new_images[i]->rgba_values != NULL);
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
    
    log_assert(current_width < 100000);
    log_assert(current_height < 100000);
    
    for (
        uint32_t i = 0;
        i < new_images_size;
        i++)
    {
        log_assert(new_images[i] != NULL);
        log_assert(new_images[i]->good);
        log_assert(new_images[i]->rgba_values_size > 0);
        log_assert(new_images[i]->width == current_width);
        log_assert(new_images[i]->height == current_height);
    }
    
    // set up a new texturearray that's big enough to hold
    // x images
    int32_t new_i = (int32_t)texture_arrays_size;
    log_assert(new_i < TEXTUREARRAYS_SIZE);
    texture_arrays_size += 1;
    
    texture_arrays[new_i].request_update = false;
    texture_arrays[new_i].image =
        (DecodedImage *)malloc_from_unmanaged(sizeof(DecodedImage));
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
    log_assert(texture_arrays[new_i].image->width > 0);
    log_assert(texture_arrays[new_i].image->height > 0);
    texture_arrays[new_i].request_update = true;
}

void register_new_texturearray(
    DecodedImage * new_image)
{
    log_assert(new_image != NULL);
    if (new_image == NULL) { return; }
    log_assert(new_image->width > 0);
    log_assert(new_image->height > 0);
    log_assert(new_image->rgba_values_size > 0);
    DecodedImage * images[1];
    images[0] = new_image;
    DecodedImage ** images_dblptr = images;
    log_assert(images[0] != NULL);
    log_assert(images[0]->width > 0);
    log_assert(images[0]->height > 0);
    log_assert(images[0]->rgba_values_size > 0);
    log_assert(images[0]->rgba_values != NULL);
    register_new_texturearray_from_images(
        images_dblptr,
        1);
}

DecodedImage * extract_image(
    TextureArray * texture_array,
    uint32_t x,
    uint32_t y)
{
    log_assert(x > 0);
    log_assert(y > 0);
    log_assert(texture_array != NULL);
    if (!application_running) { return NULL; }
    log_assert(texture_array->sprite_columns > 0);
    log_assert(texture_array->sprite_rows > 0);
    log_assert(x <= texture_array->sprite_columns);
    log_assert(y <= texture_array->sprite_rows);
    if (!application_running) { return NULL; }
    
    DecodedImage * new_image = malloc_struct_from_unmanaged(DecodedImage);
    log_assert(new_image != NULL);
    
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
    log_assert(slice_size > 0);
    log_assert(slice_width > 0);
    log_assert(slice_height > 0);
    log_assert(slice_size == slice_width * slice_height * 4);
    
    new_image->rgba_values_size = slice_size;
    new_image->rgba_values = malloc_from_unmanaged(slice_size);
    log_assert(new_image->rgba_values != NULL);
    
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
        log_assert(i < new_image->rgba_values_size);
        
        if (!application_running) {
            new_image->good = false;
            break;
        }
        
        for (
            uint32_t _ = 0;
            _ < (slice_width * 4);
            _++)
        {
            log_assert(
                (pixel_i + _)
                    < texture_array->image->rgba_values_size);
            new_image->rgba_values[i] =
                texture_array->image->rgba_values[pixel_i + _];
            i++;
        }
    }
    
    return new_image;
}
