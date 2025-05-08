#include "texture_array.h"

static uint32_t texture_arrays_mutex_ids[TEXTUREARRAYS_SIZE];

#define MAX_ASSET_FILENAME_SIZE 30
#define MAX_ASSET_FILES 1500

#define HAS_ALPHA_NO 0
#define HAS_ALPHA_YES 1
#define HAS_ALPHA_UNCHECKED 2

#define TEXTUREARRAY_FILENAME_SIZE 128
typedef struct TextureArrayImage {
    DecodedImage * image;
    char filename[TEXTUREARRAY_FILENAME_SIZE];
    bool32_t has_alpha_channel;
    bool32_t request_update;
    bool32_t prioritize_asset_load;
} TextureArrayImage;

typedef struct TextureArray {
    TextureArrayImage images[MAX_FILES_IN_SINGLE_TEXARRAY];
    uint32_t images_size;
    uint32_t single_img_width;
    uint32_t single_img_height;
    bool32_t gpu_initted;
    bool32_t request_init;
    bool32_t bc1_compressed;
} TextureArray;

static TextureArray * texture_arrays = NULL;
static uint32_t texture_arrays_size = 0;

/* returns texturearray_i */
int32_t texture_array_preinit_new_with_known_dimensions(
    const uint32_t single_img_width,
    const uint32_t single_img_height,
    const uint32_t image_count,
    const bool32_t use_bc1_compression)
{
    int32_t retval = (int32_t)texture_arrays_size;
    
    texture_arrays[retval].images_size = image_count;
    texture_arrays[retval].single_img_width = single_img_width;
    texture_arrays[retval].single_img_height = single_img_height;
    texture_arrays[retval].gpu_initted = false;
    texture_arrays[retval].request_init = false;
    texture_arrays[retval].bc1_compressed = true;
    
    for (uint32_t i = 0; i < image_count; i++) {
        texture_arrays[retval].images[i].image = NULL;
        texture_arrays[retval].images[i].filename[0] = '\0';
        texture_arrays[retval].images[i].has_alpha_channel = false;
        texture_arrays[retval].images[i].request_update = false;
        texture_arrays[retval].images[i].prioritize_asset_load = false;
    }
    
    texture_arrays_size += 1;
    
    #if TEXTURES_ACTIVE
    platform_gpu_init_texture_array(
        /* const int32_t texture_array_i: */
            retval,
        /* const uint32_t num_images: */
            image_count,
        /* const uint32_t single_image_width: */
            single_img_width,
        /* const uint32_t single_image_height: */
            single_img_height,
        /* const uint32_t use_bc1_compression: */
            use_bc1_compression);
    #endif
    
    return retval;
}

void texture_array_push_dds_image_to_preinitted(
    const int32_t to_texturearray_i,
    const int32_t to_texture_i,
    const char * filename)
{
    log_assert(to_texturearray_i < (int32_t)texture_arrays_size);
    
    log_assert(
        texture_arrays[to_texturearray_i].images[to_texture_i].image == NULL);
    log_assert(filename != NULL);
    log_assert(filename[0] != '\0');
    log_assert(
        texture_arrays[to_texturearray_i].images[to_texture_i].
            filename[0] == '\0');
    
    common_strcpy_capped(
        texture_arrays[to_texturearray_i].images[to_texture_i].filename,
        TEXTUREARRAY_FILENAME_SIZE,
        filename);
    
    FileBuffer file_buffer;
    file_buffer.size_without_terminator = platform_get_resource_size(filename);
    
    file_buffer.contents =
        (char *)malloc_from_managed(sizeof(char)
            * file_buffer.size_without_terminator + 1);
    
    platform_read_resource_file(
        filename,
        &file_buffer);
    
    log_assert(file_buffer.good);
    
    #if TEXTURES_ACTIVE
    platform_gpu_push_bc1_texture_slice_and_free_bc1_values(
        /* texture_array_i: */
            to_texturearray_i,
        /* texture_i: */
            to_texture_i,
        /* const uint32_t parent_texture_array_images_size: */
            texture_arrays[to_texturearray_i].images_size,
        /* const uint32_t image_width: */
            texture_arrays[to_texturearray_i].single_img_width,
        /* const uint32_t image_height: */
            texture_arrays[to_texturearray_i].single_img_height,
        /* const uint8_t * bc1_values: */
            (uint8_t *)file_buffer.contents);
    #endif
    
    texture_arrays[to_texturearray_i].images[to_texture_i].
        request_update = false;
    texture_arrays[to_texturearray_i].images[to_texture_i].
        prioritize_asset_load = false;
}

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
        
        to_replace->rgba_values_page_aligned[(pixel_i * 4) + 0] =
            black ? 0 : 255;
        to_replace->rgba_values_page_aligned[(pixel_i * 4) + 1] = 0;
        to_replace->rgba_values_page_aligned[(pixel_i * 4) + 2] = 0;
        to_replace->rgba_values_page_aligned[(pixel_i * 4) + 3] = 255;
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
    log_assert(to_replace->rgba_values_freeable == NULL);
    log_assert(to_replace->rgba_values_page_aligned == NULL);
    
    to_replace->width = 30;
    to_replace->height = 30;
    to_replace->pixel_count =
        to_replace->width * to_replace->height;
    to_replace->rgba_values_size = to_replace->pixel_count * 4;
    
    malloc_from_managed_page_aligned(
        /* void * base_pointer_for_freeing: */
            (void *)&to_replace->rgba_values_freeable,
        /* void * aligned_subptr: */
            (void *)&to_replace->rgba_values_page_aligned,
        /* const size_t subptr_size: */
            to_replace->rgba_values_size);
    
    set_allocated_to_error_image(to_replace);
}

//static DecodedImage * malloc_img_from_filename_with_working_memory(
//    const char * filename)
//{
//}

static DecodedImage * malloc_img_from_filename(
    const char * filename)
{
    FileBuffer file_buffer;
    file_buffer.size_without_terminator = platform_get_resource_size(filename);
    
    log_assert(file_buffer.size_without_terminator > 1);
    file_buffer.contents =
        (char *)malloc_from_managed(sizeof(char) *
            file_buffer.size_without_terminator + 1);
    common_memset_char(
        file_buffer.contents,
        0,
        file_buffer.size_without_terminator + 1);
    
    platform_read_resource_file(
        filename,
        &file_buffer);
    
    if (!file_buffer.good) {
        log_append("platform failed to read file: ");
        log_append(filename);
        log_append("\n");
        free_from_managed((uint8_t *)file_buffer.contents);
        return NULL;
    }
    log_assert(file_buffer.good);
    
    DecodedImage * new_image = malloc_from_unmanaged(sizeof(DecodedImage));
    
    if (
        file_buffer.contents[1] == 'P' &&
        file_buffer.contents[2] == 'N' &&
        file_buffer.contents[3] == 'G')
    {
        get_PNG_width_height(
            /* const uint8_t * compressed_input: */
                (uint8_t *)file_buffer.contents,
            /* const uint64_t compressed_input_size: */
                file_buffer.size_without_terminator - 1,
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
        malloc_from_managed_page_aligned(
            /* void *base_pointer_for_freeing: */
                (void *)&new_image->rgba_values_freeable,
            /* void *aligned_subptr: */
                (void *)&new_image->rgba_values_page_aligned,
            /* const size_t subptr_size: */
                new_image->rgba_values_size);
        
        common_memset_char(
            new_image->rgba_values_page_aligned,
            0,
            new_image->rgba_values_size);
        
        decode_PNG(
            /* const uint8_t * compressed_input: */
                (uint8_t *)file_buffer.contents,
            /* const uint64_t compressed_input_size: */
                file_buffer.size_without_terminator - 1,
            /* out_rgba_values: */
                new_image->rgba_values_page_aligned,
            /* rgba_values_size: */
                new_image->rgba_values_size,
            /* uint32_t * out_good: */
                &new_image->good);
    } else if (
        file_buffer.contents[0] == 'B' &&
        file_buffer.contents[1] == 'M')
    {
        get_BMP_width_height(
            /* const uint8_t * compressed_input: */
                (uint8_t *)file_buffer.contents,
            /* const uint64_t compressed_input_size: */
                file_buffer.size_without_terminator - 1,
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
        malloc_from_managed_page_aligned(
            /* void * base_pointer_for_freeing: */
                (void *)&new_image->rgba_values_freeable,
            /* void * aligned_subptr: */
                (void *)&new_image->rgba_values_page_aligned,
            /* const size_t subptr_size: */
                new_image->rgba_values_size);
        common_memset_char(
            new_image->rgba_values_page_aligned,
            0,
            new_image->rgba_values_size);
        
        decode_BMP(
            /* raw_input: */ (uint8_t *)file_buffer.contents,
            /* raw_input_size: */ file_buffer.size_without_terminator - 1,
            /* out_rgba_values: */ new_image->rgba_values_page_aligned,
            /* out_rgba_values_size: */ new_image->rgba_values_size,
            /* out_good: */ &new_image->good);
    } else {
        log_append("unrecognized file format in: ");
        log_append(filename);
        log_append_char('\n');
        new_image->good = false;
    }
    free_from_managed(file_buffer.contents);
    
    if (!new_image->good) {
        set_allocated_to_error_image(new_image);
        return new_image;
    }
    
    log_assert(new_image->pixel_count * 4 == new_image->rgba_values_size);
    log_assert(new_image->pixel_count == new_image->width * new_image->height);
        
    log_assert(new_image->good);
    return new_image;
}

static DecodedImage * extract_image(
    const DecodedImage * original,
    const uint32_t sprite_columns,
    const uint32_t sprite_rows,
    const uint32_t x,
    const uint32_t y)
{
    log_assert(x > 0);
    log_assert(y > 0);
    log_assert(original != NULL);
    log_assert(original->rgba_values_freeable != NULL);
    log_assert(original->rgba_values_page_aligned != NULL);
    
    if (!application_running) { return NULL; }
    log_assert(sprite_columns > 0);
    log_assert(sprite_rows > 0);
    log_assert(x <= sprite_columns);
    log_assert(y <= sprite_rows);
    if (!application_running) { return NULL; }
    
    DecodedImage * new_image = malloc_from_unmanaged(sizeof(DecodedImage));
    log_assert(new_image != NULL);
    
    #ifndef LOGGER_IGNORE_ASSERTS
    if (!application_running) { return NULL; }
    #endif
    
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
    malloc_from_managed_page_aligned(
        /* void * base_pointer_for_freeing: */
            (void *)&new_image->rgba_values_freeable,
        /* void * aligned_subptr: */
            (void *)&new_image->rgba_values_page_aligned,
        /* const size_t subptr_size: */
            new_image->rgba_values_size);
    common_memset_char(
        new_image->rgba_values_page_aligned,
        0,
        new_image->rgba_values_size);
    
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
            log_assert((rgba_value_i + _) < original->rgba_values_size);
            log_assert(i < new_image->rgba_values_size);
            new_image->rgba_values_page_aligned[i] =
                original->rgba_values_page_aligned[rgba_value_i + _];
            i++;
        }
    }
    
    new_image->good = true;
    return new_image;
}

static void register_to_texturearray_from_images(
    const int32_t target_texture_array_i,
    DecodedImage ** new_images,
    const char ** new_img_filenames,
    const uint32_t new_images_size)
{
    log_assert(target_texture_array_i < TEXTUREARRAYS_SIZE);
    platform_mutex_lock(texture_arrays_mutex_ids[target_texture_array_i]);
    log_assert(new_images_size > 0);
    for (uint32_t i = 0; i < new_images_size; i++) {
        log_assert(new_images[i] != NULL);
        if (new_images[i] == NULL) {
            platform_mutex_unlock(
                texture_arrays_mutex_ids[target_texture_array_i]);
            return;
        }
        log_assert(new_images[i]->width > 0);
        log_assert(new_images[i]->height > 0);
        log_assert(new_images[i]->rgba_values_size > 0);
        log_assert(new_images[i]->rgba_values_freeable != NULL);
        log_assert(new_images[i]->rgba_values_page_aligned != NULL);
    }
    
    uint32_t current_width = new_images[0]->width;
    uint32_t current_height = new_images[0]->height;
    log_assert(current_width > 0);
    log_assert(current_height > 0);
    log_assert(current_width < 100000);
    log_assert(current_height < 100000);
    
    if (texture_arrays[target_texture_array_i].images_size == 0) {
        // set up a new texturearray that's big enough to hold
        // x images
        texture_arrays[target_texture_array_i].single_img_width = current_width;
        texture_arrays[target_texture_array_i].single_img_height = current_height;
        // texture_arrays[target_texture_array_i].images_size = 0;
        texture_arrays[target_texture_array_i].request_init = true;
    } else {
        log_assert(
            texture_arrays[target_texture_array_i].single_img_width ==
                current_width);
        log_assert(
            texture_arrays[target_texture_array_i].single_img_height ==
                current_height);
        log_assert(
            (texture_arrays[target_texture_array_i].images_size +
                new_images_size) < MAX_FILES_IN_SINGLE_TEXARRAY);
    }
    
    for (
        uint32_t i = 0;
        i < new_images_size;
        i++)
    {
        uint32_t texture_i = texture_arrays[target_texture_array_i].images_size;
        
        log_assert(new_images[i] != NULL);
        log_assert(new_images[i]->good);
        log_assert(new_images[i]->rgba_values_size > 0);
        log_assert(new_images[i]->width == current_width);
        log_assert(new_images[i]->height == current_height);
        texture_arrays[target_texture_array_i].images[texture_i].
            has_alpha_channel = HAS_ALPHA_UNCHECKED;
        texture_arrays[target_texture_array_i].images[texture_i].image =
            new_images[i];
        texture_arrays[target_texture_array_i].images[texture_i].
            request_update = true;
        
        if (
            new_img_filenames != NULL &&
            new_img_filenames[i] != NULL)
        {
            log_assert(new_img_filenames[i] != NULL);
            common_strcpy_capped(
                texture_arrays[target_texture_array_i].images[texture_i].
                    filename,
                128,
                new_img_filenames[i]);
        }
        
        texture_arrays[target_texture_array_i].images_size += 1;
    }
    platform_mutex_unlock(texture_arrays_mutex_ids[target_texture_array_i]);
}

void texture_array_init(void) {
    
    // initialize texture arrays
    texture_arrays = (TextureArray *)malloc_from_unmanaged(
        sizeof(TextureArray) * TEXTUREARRAYS_SIZE);
    
    for (uint32_t i = 0; i < TEXTUREARRAYS_SIZE; i++) {
        texture_arrays_mutex_ids[i] = platform_init_mutex_and_return_id();
        texture_arrays[i].images_size = 0;
        texture_arrays[i].single_img_width = 0;
        texture_arrays[i].single_img_height = 0;
        texture_arrays[i].request_init = false;
        texture_arrays[i].gpu_initted = false;
        for (uint32_t j = 0; j < MAX_FILES_IN_SINGLE_TEXARRAY; j++) {
            texture_arrays[i].images[j].request_update = false;
            texture_arrays[i].images[j].prioritize_asset_load = false;
            texture_arrays[i].images[j].filename[0] = '\0';
            texture_arrays[i].images[j].has_alpha_channel =
                HAS_ALPHA_UNCHECKED;
            texture_arrays[i].images[j].image = NULL;
        }
    }
}

void texture_array_gpu_try_push(void) {
    for (
        int32_t i = 0;
        (uint32_t)i < texture_arrays_size;
        i++)
    {
        if (platform_mutex_trylock(texture_arrays_mutex_ids[i])) {
            if (texture_arrays[i].request_init)
            {
                log_assert(texture_arrays[i].images_size < 1240);
                
                log_assert(!texture_arrays[i].gpu_initted);
                texture_arrays[i].request_init = false;
                texture_arrays[i].gpu_initted = true;
                
                #if TEXTURES_ACTIVE
                platform_gpu_init_texture_array(
                    i,
                    texture_arrays[i].images_size,
                    texture_arrays[i].single_img_width,
                    texture_arrays[i].single_img_height,
                    /* bc1_compression: */ false);
                #endif
            }
            
            if (texture_arrays[i].gpu_initted) {
                log_assert(texture_arrays[i].images_size < 1240);
                for (
                    int32_t j = 0;
                    (uint32_t)j < texture_arrays[i].images_size;
                    j++)
                {
                    if (texture_arrays[i].images[j].request_update) {
                        texture_arrays[i].images[j].request_update = false;
                        #ifndef LOGGER_IGNORE_ASSERTS
                        log_assert(texture_arrays[i]
                                .images[j]
                                .image != NULL);
                        log_assert(
                            texture_arrays[i]
                                    .images[j]
                                    .image->rgba_values_freeable != NULL);
                        log_assert(
                            texture_arrays[i]
                                    .images[j]
                                    .image->rgba_values_page_aligned != NULL);
                        #endif
                        
                        if (!application_running) {
                            platform_mutex_unlock(texture_arrays_mutex_ids[i]);
                            return;
                        }
                        
                        #if TEXTURES_ACTIVE
                        platform_gpu_push_texture_slice_and_free_rgba_values(
                            i,
                            j,
                            /* parent_texture_array_images_size: */
                                texture_arrays[i].images_size,
                            /* image_width: */
                                texture_arrays[i].single_img_width,
                            /* image_height: */
                                texture_arrays[i].single_img_height,
                            /* uint8_t * rgba_values_freeable: */
                                texture_arrays[i]
                                    .images[j]
                                    .image->rgba_values_freeable,
                            /* uint8_t * rgba_values_page_aligned: */
                                texture_arrays[i]
                                    .images[j]
                                    .image->rgba_values_page_aligned);
                        #endif
                        
                        texture_arrays[i].images[j].image->
                            rgba_values_page_aligned = NULL;
                    }
                }
            }
            platform_mutex_unlock(texture_arrays_mutex_ids[i]);
        }
    }
}

static void register_to_texturearray_by_splitting_image(
    DecodedImage * new_image,
    const char * filename_prefix,
    const int32_t texture_array_i,
    const uint32_t rows,
    const uint32_t columns)
{
    log_assert(new_image != NULL);
    if (new_image == NULL) { return; }
    log_assert(new_image->rgba_values_freeable != NULL);
    log_assert(new_image->rgba_values_size > 0);
    log_assert(rows >= 1);
    log_assert(columns >= 1);
    
    if (new_image->rgba_values_size < 1) { return; }
    
    DecodedImage ** subimages = (DecodedImage **)
        malloc_from_unmanaged(sizeof(DecodedImage *) * rows * columns);
    
    char * filenames[256];
    for (uint32_t i = 0; i < 256; i++) {
        filenames[i] = malloc_from_managed(256);
        common_memset_char(filenames[i], 0, sizeof(rows * columns * 256));
    }
    
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
            
            if (new_img == NULL) {
                log_assert(0);
                continue;
            }
            log_assert(new_img->good);
            
            if (expected_width == 0 || expected_height == 0) {
                expected_width  = new_img->width;
                expected_height = new_img->height;
            } else {
                log_assert(new_img->width == expected_width);
                log_assert(new_img->height == expected_height);
            }
            subimages[(row_i * columns) + col_i] = new_img;
            
            common_strcpy_capped(
                filenames[(row_i*columns)+col_i],
                256,
                filename_prefix);
            common_strcat_capped(
                filenames[(row_i*columns)+col_i],
                256,
                "_");
            common_strcat_uint_capped(
                filenames[(row_i*columns)+col_i],
                256,
                col_i);
            common_strcat_capped(
                filenames[(row_i*columns)+col_i],
                256,
                "_");
            common_strcat_uint_capped(
                filenames[(row_i*columns)+col_i],
                256,
                row_i);
        }
    }
    
    DecodedImage ** subimages_dblptr = subimages;
    register_to_texturearray_from_images(
        /* target_texture_array_i: */
            texture_array_i,
        /* DecodedImage ** new_images : */
            subimages_dblptr,
        /* new_img_filenames: */
            (const char **)filenames,
        /* new_images_size: */
            rows * columns);
    
    for (uint32_t i = 0; i < 256; i++) {
        free(filenames[i]);
    }
}

static void register_new_texturearray_by_splitting_image(
    DecodedImage * new_image,
    const char * filename_prefix,
    const uint32_t rows,
    const uint32_t columns)
{
    int32_t new_texture_array_i = -1;
    
    uint32_t expected_width = new_image->width / columns;
    uint32_t expected_height = new_image->height / rows;
    
    for (int32_t i = 0; i < (int32_t)texture_arrays_size; i++) {
        if (
            texture_arrays[i].single_img_width == expected_width &&
            texture_arrays[i].single_img_height == expected_height &&
            texture_arrays[i].images_size + (rows * columns) <
                MAX_FILES_IN_SINGLE_TEXARRAY)
        {
            new_texture_array_i = i;
            break;
        }
    }
    
    if (new_texture_array_i < 0) {
        new_texture_array_i = (int)texture_arrays_size;
        texture_arrays[new_texture_array_i].request_init = true;
        texture_arrays_size++;
        log_assert(texture_arrays_size <= TEXTUREARRAYS_SIZE);
    }
    
    register_to_texturearray_by_splitting_image(
        /* DecodedImage * new_image: */
            new_image,
        /* filename_prefix: */
            filename_prefix,
        /* const int32_t texture_array_i: */
            new_texture_array_i,
        /* const uint32_t rows: */
            rows,
        /* const uint32_t columns: */
            columns);
}

void texture_array_register_new_by_splitting_file(
    const char * filename,
    const uint32_t rows,
    const uint32_t columns)
{
    DecodedImage * img = malloc_img_from_filename(filename);
    
    char filename_prefix[256];
    common_strcpy_capped(filename_prefix, 256, filename);
    uint32_t i = 0;
    while (filename_prefix[i] != '\0' && filename_prefix[i] != '.') {
        i++;
    }
    filename_prefix[i] = '\0';
    
    register_new_texturearray_by_splitting_image(
        img,
        filename_prefix,
        rows,
        columns);
}

void texture_array_register_high_priority_if_unloaded(
    const int32_t texture_array_i,
    const int32_t texture_i)
{
    log_assert(texture_array_i >= 0 && texture_i >= 0);
    
    if (
        texture_array_i >= 0
        && texture_i >= 0
        && texture_arrays[texture_array_i].images[texture_i].image == NULL
        && !texture_arrays[texture_array_i]
            .images[texture_i].prioritize_asset_load)
    {
        // asset is needed to go onscreen already but not available,
        // mark as high priority
        texture_arrays[texture_array_i].images[texture_i]
            .prioritize_asset_load = true;
    }
}

void texture_array_preregister_null_image(
    char * filename,
    const uint32_t height,
    const uint32_t width)
{
    int32_t new_texturearray_i = -1;
    int32_t new_texture_i = -1;
    
    // *****************
    // find out if same width/height was already used
    // set new_texturearray_i and new_texture_i if so
    for (uint32_t i = 0; i < texture_arrays_size; i++)
    {
        log_assert(i < TEXTUREARRAYS_SIZE);
        if (
            texture_arrays[i].single_img_width == width
            && texture_arrays[i].single_img_height == height)
        {
            new_texturearray_i = (int)i;
            new_texture_i = (int32_t)texture_arrays[i].images_size;
            texture_arrays[i].images_size++;
            log_append("texture_arrays[");
            log_append_uint(i);
            log_append("].images_size is now: ");
            log_append_uint(texture_arrays[i].images_size);
            log_append("\n");
            
            log_assert(new_texture_i >= 1);
            log_assert(new_texture_i < MAX_FILES_IN_SINGLE_TEXARRAY);
            break;
        }
    }
    
    // if this is a new texturearray (we never saw
    // these image dimensions before)
    if (new_texturearray_i < 0) {
        new_texturearray_i = (int32_t)texture_arrays_size;
        texture_arrays_size++;
        log_assert(texture_arrays_size <= TEXTUREARRAYS_SIZE);
        new_texture_i = 0;
        
        texture_arrays[new_texturearray_i].images_size = 1;
        texture_arrays[new_texturearray_i].single_img_width = width;
        texture_arrays[new_texturearray_i].single_img_height = height;
    }
    
    common_strcpy_capped(
        texture_arrays[new_texturearray_i].images[new_texture_i].filename,
        TEXTUREARRAY_FILENAME_SIZE,
        filename);
}

void texture_array_get_filename_location(
    const char * for_filename,
    int32_t * texture_array_i_recipient,
    int32_t * texture_i_recipient)
{
    *texture_array_i_recipient = -1;
    *texture_i_recipient = -1;
    
    for (uint32_t i = 0; i < texture_arrays_size; i++) {
        log_assert(texture_arrays[i].images_size < 2000);
        for (uint32_t j = 0; j < texture_arrays[i].images_size; j++) {
            if (common_are_equal_strings(
                texture_arrays[i].images[j].filename,
                for_filename))
            {
                *texture_array_i_recipient = (int32_t)i;
                *texture_i_recipient = (int32_t)j;
                return;
            }
        }
    }
    
    *texture_array_i_recipient = -1;
    *texture_i_recipient       = -1;
}

void texture_array_decode_null_image_at(
    const int32_t texture_array_i,
    const int32_t texture_i)
{
    int32_t i = texture_array_i;
    int32_t j = texture_i;
    
    FileBuffer file_buffer;
    file_buffer.size_without_terminator = platform_get_resource_size(
        texture_arrays[i].images[j].filename);
    
    file_buffer.contents =
        (char *)malloc_from_managed(sizeof(char)
            * file_buffer.size_without_terminator + 1);
    
    platform_read_resource_file(
        texture_arrays[i].images[j].filename,
        &file_buffer);
    
    log_assert(file_buffer.good);
    
    DecodedImage * new_image =
        malloc_from_unmanaged(sizeof(DecodedImage));
    new_image->height = texture_arrays[i].single_img_height;
    new_image->width = texture_arrays[i].single_img_width;
    log_assert(new_image->height > 0);
    log_assert(new_image->width > 0);
    new_image->rgba_values_size =
        new_image->height * new_image->width * 4;
    log_assert(new_image->rgba_values_size > 0);
    malloc_from_managed_page_aligned(
        /* void * base_pointer_for_freeing: */
            (void *)&new_image->rgba_values_freeable,
        /* void * aligned_subptr: */
            (void *)&new_image->rgba_values_page_aligned,
        /* const size_t subptr_size: */
            new_image->rgba_values_size);
    
    if (new_image->rgba_values_freeable == NULL || !application_running) {
        application_running = false;
        return;
    }
    
    if (file_buffer.contents[1] == 'P' &&
        file_buffer.contents[2] == 'N')
    {
        decode_PNG(
            /* const uint8_t * compressed_input: */
                (uint8_t *)file_buffer.contents,
            /* const uint64_t compressed_input_size: */
                file_buffer.size_without_terminator,
            /* out_rgba_values: */
                new_image->rgba_values_page_aligned,
            /* rgba_values_size: */
                new_image->rgba_values_size,
            /* uint32_t * out_good: */
                &new_image->good);
    } else if (
        file_buffer.contents[0] == 'B' &&
        file_buffer.contents[1] == 'M')
    {
        decode_BMP(
            /* const uint8_t * raw_input: */
                (uint8_t *)file_buffer.contents,
            /* const uint64_t raw_input_size: */
                file_buffer.size_without_terminator,
            /* out_rgba_values: */
                new_image->rgba_values_page_aligned,
            /* out_rgba_values_size: */
                new_image->rgba_values_size,
            /* uint32_t * out_good: */
                &new_image->good);
    }
    
    // TODO: reinstate this assert
    // log_assert(new_image->good);
    log_assert(new_image->height == texture_arrays[i].single_img_height);
    log_assert(new_image->width == texture_arrays[i].single_img_width);
    new_image->pixel_count = new_image->height * new_image->width;
    
    free_from_managed(file_buffer.contents);
    
    platform_mutex_lock(texture_arrays_mutex_ids[i]);
    texture_arrays[i].images[j].image = new_image;
    texture_arrays[i].images[j].request_update = true;
    texture_arrays[i].images[j].prioritize_asset_load = false;
    platform_mutex_unlock(texture_arrays_mutex_ids[i]);
}

void texture_array_load_font_images(void) {
    
    // Don't call this, the engine should call it
    assert(texture_arrays_size == 0);
    const char * fontfile = "font.png";
    texture_array_register_new_by_splitting_file(
        /* filename : */ fontfile,
        /* rows     : */ 10,
        /* columns  : */ 10);
    
    #if TEXTURES_ACTIVE
    platform_gpu_init_texture_array(
        0,
        texture_arrays[0].images_size,
        texture_arrays[0].single_img_width,
        texture_arrays[0].single_img_height,
        /* bc1_compression: */ false);
    #endif
    
    for (
        int32_t i = 0;
        i < (int32_t)texture_arrays[0].images_size;
        i++)
    {
        #if TEXTURES_ACTIVE
        platform_gpu_push_texture_slice_and_free_rgba_values(
            /* const int32_t texture_array_i: */
                0,
            /* const int32_t texture_i: */
                i,
            /* const uint32_t parent_texture_array_images_size: */
                texture_arrays[0].images_size,
            /* const uint32_t image_width: */
                texture_arrays[0].single_img_width,
            /* const uint32_t image_height: */
                texture_arrays[0].single_img_height,
            /* const uint8_t *rgba_values: */
                texture_arrays[0].images[i].image->rgba_values_freeable,
                texture_arrays[0].images[i].image->rgba_values_page_aligned);
        #endif
        texture_arrays[0].images[i].image->rgba_values_page_aligned = NULL;
        texture_arrays[0].images[i].request_update = false;
    }
    
    texture_arrays[0].request_init = false;
}

void texture_array_decode_all_null_images(void)
{
    while (application_running) {
        int32_t priority_png_texturearray_i = -1;
        int32_t priority_png_texture_i = -1;
        int32_t nonpriority_texturearray_i = -1;
        int32_t nonpriority_texture_i = -1;
        
        log_assert(texture_arrays_size <= TEXTUREARRAYS_SIZE);
        
        for (
            int32_t i = 0;
            (uint32_t)i < texture_arrays_size;
            i++)
        {
            // platform_mutex_lock(texture_arrays_mutex_ids[i]);
            log_assert(
                texture_arrays[i].images_size <= MAX_FILES_IN_SINGLE_TEXARRAY);
            for (
                int32_t j = 0;
                (uint32_t)j < texture_arrays[i].images_size;
                j++)
            {
                if (
                    texture_arrays[i].images[j].image == NULL &&
                    texture_arrays[i].images[j].filename[0] != '\0' &&
                    !texture_arrays[i].bc1_compressed)
                {
                    uint32_t strlen = common_get_string_length(
                        texture_arrays[i].images[j].filename);
                    
                    if (texture_arrays[i].images[j].prioritize_asset_load &&
                        strlen > 4) {
                        if (
                            texture_arrays[i].images[j].
                                filename[strlen - 3] == 'b' &&
                            texture_arrays[i].images[j].
                                filename[strlen - 2] == 'm' &&
                            texture_arrays[i].images[j].
                                filename[strlen - 1] == 'p')
                        {
                            log_append("decoding image: ");
                            log_append(texture_arrays[i].images[j].filename);
                            log_append_char('\n');
                            texture_array_decode_null_image_at(
                                /* const int32_t texture_array_i: */ i,
                                /* const int32_t texture_i: */ j);
                            continue;
                        }
                        
                        if (
                            texture_arrays[i].images[j].
                                filename[strlen - 3] == 'p' &&
                            texture_arrays[i].images[j].
                                filename[strlen - 2] == 'n' &&
                            texture_arrays[i].images[j].
                                filename[strlen - 1] == 'g')
                        {
                            priority_png_texturearray_i = (int32_t)i;
                            priority_png_texture_i = (int32_t)j;
                            break;
                        }
                    }
                    
                    if (!texture_arrays[i].images[j].prioritize_asset_load)
                    {
                        nonpriority_texturearray_i = (int32_t)i;
                        nonpriority_texture_i = (int32_t)j;
                        continue;
                    }
                }
            }
        }
        
        int32_t i = -1;
        int32_t j = -1;
        if (
            priority_png_texture_i >= 0 &&
            priority_png_texturearray_i >= 0)
        {
            i = priority_png_texturearray_i;
            j = priority_png_texture_i;
        } else if (
            nonpriority_texture_i >= 0 &&
            nonpriority_texturearray_i >= 0)
        {
            i = nonpriority_texturearray_i;
            j = nonpriority_texture_i;
        }
        
        if (i < 0 || j < 0) {
            break;
        }
        
        log_assert(i < TEXTUREARRAYS_SIZE);
        log_assert(j < MAX_FILES_IN_SINGLE_TEXARRAY);
        
        texture_array_decode_null_image_at(
            /* const int32_t texture_array_i: */
                i,
            /* const int32_t texture_i: */
                j);
        
        log_assert(texture_arrays[i].images[j].image != NULL);
        log_assert(texture_arrays[i].images[j].filename[0] != '\0');
    }
}

void texture_array_flag_all_to_request_gpu_init(void) {
    for (uint32_t i = 1; i < texture_arrays_size; i++) {
        texture_arrays[i].request_init = true;
    }
}
