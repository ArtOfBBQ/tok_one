#include "texture_array.h"

TextureArray * texture_arrays = NULL;
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
    
    log_assert(new_image->pixel_count * 4 == new_image->rgba_values_size);
    log_assert(get_sum_rgba(new_image) > 0);
    
    log_assert(new_image->pixel_count == new_image->width * new_image->height);
    
    free_from_managed((uint8_t *)file_buffer.contents);
    
    log_assert(new_image->good);
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
        [MAX_ASSET_FILENAME_SIZE],
    const uint32_t expected_width,
    const uint32_t expected_height,
    const uint32_t filenames_size)
{
    uint64_t dpng_working_memory_size = 10000000;
    uint8_t * dpng_working_memory =
        malloc_from_managed(dpng_working_memory_size);
    
    update_texturearray_from_0terminated_files_with_memory(
        texturearray_i,
        filenames,
        expected_width,
        expected_height,
        filenames_size,
        dpng_working_memory,
        dpng_working_memory_size);
    
    free_from_managed(dpng_working_memory);
}

void update_texturearray_from_0terminated_files_with_memory(
    const int32_t texturearray_i,
    const char filenames
        [MAX_FILES_IN_SINGLE_TEXARRAY]
        [MAX_ASSET_FILENAME_SIZE],
    const uint32_t expected_width,
    const uint32_t expected_height,
    const uint32_t filenames_size,
    uint8_t * dpng_working_memory,
    uint64_t dpng_working_memory_size)
{
    log_assert(texturearray_i < TEXTUREARRAYS_SIZE);
    log_assert(filenames_size > 0);
    log_assert(expected_width > 0);
    log_assert(expected_height > 0);
    
    texture_arrays[texturearray_i].single_img_width = expected_width;
    texture_arrays[texturearray_i].single_img_height = expected_height;
    
    texture_arrays[texturearray_i].images_size = filenames_size;
    texture_arrays[texturearray_i].request_init = true;
    log_assert(texture_arrays[texturearray_i].images != NULL);
    
    for (uint32_t t_i = 0; t_i < filenames_size; t_i++)
    {
        log_assert(t_i < MAX_IMAGES_IN_TEXARRAY);
        const char * filename = filenames[t_i];
        
        DecodedImage * new_image =
            malloc_img_from_filename_with_working_memory(
                filename,
                dpng_working_memory,
                dpng_working_memory_size);
        
        log_assert(new_image->good);
        log_assert(new_image->width ==
            texture_arrays[texturearray_i].single_img_width);
        log_assert(new_image->height ==
            texture_arrays[texturearray_i].single_img_height);
        texture_arrays[texturearray_i].images[t_i].image = new_image;
        texture_arrays[texturearray_i].images[t_i].request_update = true;
    }
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
    log_assert(current_width > 0);
    log_assert(current_height > 0);
    log_assert(current_width < 100000);
    log_assert(current_height < 100000);
    
    // set up a new texturearray that's big enough to hold
    // x images
    int32_t new_i = (int32_t)texture_arrays_size;
    log_assert(new_i < TEXTUREARRAYS_SIZE);
    texture_arrays[new_i].images_size = new_images_size;
    texture_arrays[new_i].request_init = true;
    texture_arrays_size += 1;
    
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
        texture_arrays[new_i].images[i].image = new_images[i];
        texture_arrays[new_i].images[i].request_update = true;
    }
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
    const DecodedImage * original,
    const uint32_t sprite_columns,
    const uint32_t sprite_rows,
    const uint32_t x,
    const uint32_t y)
{
    log_assert(x > 0);
    log_assert(y > 0);
    log_assert(original != NULL);
    log_assert(original->rgba_values != NULL);
    
    if (!application_running) { return NULL; }
    log_assert(sprite_columns > 0);
    log_assert(sprite_rows > 0);
    log_assert(x <= sprite_columns);
    log_assert(y <= sprite_rows);
    if (!application_running) { return NULL; }
    
    DecodedImage * new_image = malloc_struct_from_unmanaged(DecodedImage);
    log_assert(new_image != NULL);
    
    uint32_t slice_size_bytes =
        original->rgba_values_size
            / sprite_columns
            / sprite_rows;
    uint32_t slice_width_pixels =
        original->width / sprite_columns;
    uint32_t slice_height_pixels = original->height / sprite_rows;
    log_assert(slice_size_bytes > 0);
    log_assert(slice_width_pixels > 0);
    log_assert(slice_height_pixels > 0);
    log_assert(
        slice_size_bytes == slice_width_pixels * slice_height_pixels * 4);
    log_assert(
        slice_size_bytes *
            sprite_columns *
            sprite_rows ==
                original->rgba_values_size);
    
    new_image->rgba_values_size = slice_size_bytes;
    new_image->rgba_values = malloc_from_unmanaged(slice_size_bytes);
    log_assert(new_image->rgba_values != NULL);
    
    new_image->width = slice_width_pixels;
    new_image->height = slice_height_pixels;
    
    uint32_t start_x_pixels = 1 + ((x - 1) * slice_width_pixels);
    uint32_t start_y_pixels = 1 + ((y - 1) * slice_height_pixels);
    uint32_t end_y_pixels = start_y_pixels + slice_height_pixels - 1;
    
    uint32_t i = 0;
    for (
        uint32_t cur_y_pixels = start_y_pixels;
        cur_y_pixels <= end_y_pixels;
        cur_y_pixels++)
    {
        // get the pixel that's at [start_x_pixels, cur_y_pixels]
        // copcur_y slice_width pixels
        uint64_t rgba_value_i =
            ((start_x_pixels - 1) * 4)
                + ((cur_y_pixels - 1) * original->width * 4);
        
        if (!application_running) {
            new_image->good = false;
            break;
        }
        
        for (
            uint32_t _ = 0;
            _ < (slice_width_pixels * 4);
            _++)
        {
            log_assert(
                (rgba_value_i + _)
                    < original->rgba_values_size);
            log_assert(i < new_image->rgba_values_size);
            new_image->rgba_values[i] =
                original->rgba_values[rgba_value_i + _];
            i++;
        }
    }
    
    new_image->good = true;
    return new_image;
}

void register_new_texturearray_by_splitting_image(
    DecodedImage * new_image,
    const uint32_t rows,
    const uint32_t columns)
{
    log_assert(new_image != NULL);
    log_assert(new_image->rgba_values != NULL);
    log_assert(new_image->rgba_values_size > 0);
    log_assert(rows >= 1);
    log_assert(columns >= 1);
    
    DecodedImage ** subimages = (DecodedImage **)
        malloc_from_unmanaged(sizeof(DecodedImage *) * rows * columns);
    
    // each image should have the exact same dimensions
    uint32_t expected_width = 0;
    uint32_t expected_height = 0;
    
    for (uint32_t col_i = 0; col_i < columns; col_i++) {
        for (uint32_t row_i = 0; row_i < rows; row_i++) {
            DecodedImage * new_img = extract_image(
                /* const DecodedImage * original: */
                    new_image,
                /* const uint32_t sprite_columns: */
                    columns,
                /* const uint32_t sprite_rows: */
                    rows,
                /* const uint32_t x: */
                    col_i + 1,
                /* const uint32_t y: */
                    row_i + 1);
            log_assert(new_img->good);
            
            if (expected_width == 0 || expected_height == 0) {
                expected_width = new_img->width;
                expected_height = new_img->height;
            } else {
                log_assert(new_img->width == expected_width);
                log_assert(new_img->height == expected_height);
            }
            subimages[(row_i * columns) + col_i] = new_img; 
        }
    }
    
    DecodedImage ** subimages_dblptr = subimages;
    register_new_texturearray_from_images(
        /* DecodedImage ** new_images : */
            subimages_dblptr,
        /* new_images_size: */
            rows * columns);
}

void register_new_texturearray_by_splitting_file(
    const char * filename,
    const uint32_t rows,
    const uint32_t columns)
{
    DecodedImage * img = malloc_img_from_filename(filename);
    
    register_new_texturearray_by_splitting_image(
        img,
        rows,
        columns);
}
