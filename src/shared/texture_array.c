#include "texture_array.h"

TextureArray texture_arrays[TEXTUREARRAYS_SIZE];
uint32_t texture_arrays_size = 0;


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
        
        FileBuffer * file_buffer =
            platform_read_file(filename);
        
        if (file_buffer == NULL) {
            printf(
                "ERR - fail to add texarray - unfound file %s\n",
                filename);
            assert(0);
        }
        
        decoded_images[i] =
            decode_PNG(
                (uint8_t *)file_buffer->contents,
                file_buffer->size);
        
        free(file_buffer->contents);
        free(file_buffer);
    }
    
    register_new_texturearray_from_images(
        /* DecodedImage ** new_iamges : */ &decoded_images,
        /* new_images_size: */ decoded_images_size);
}

// returns new_texture_array_i (index in texture_arrays)
void register_new_texturearray_from_images(
    const DecodedImage ** new_images,
    const uint32_t new_images_size)
{
    for (uint32_t i = 0; i < new_images_size; i++) {
        assert(new_images[i] != NULL);
        assert(new_images[i]->good);
        assert(new_images[i]->rgba_values_size > 0);
        
        const uint32_t current_width = new_images[i]->width;
        const uint32_t current_height = new_images[i]->height;
        
        // find out how many images of this width/height we have 
        uint32_t current_texturearray_images = 0; 
        for (uint32_t j = i; j < new_images_size; j++) {
            if (new_images[j]->width != current_width ||
                new_images[j]->height != current_height)
            {
                continue;
            }
            
            current_texturearray_images += 1;
        }
        
        // set up a new texturearray that's big enough to hold
        // x images
        printf(
            "set up a new texturearray big enough to hold %u images...\n",
            current_texturearray_images);
        int32_t new_i = (int32_t)texture_arrays_size;
        assert(new_i < TEXTUREARRAYS_SIZE);
        texture_arrays_size += 1;
        texture_arrays[new_i].sprite_columns = 1;
        texture_arrays[new_i].sprite_rows = 1;
        
        // fill in the images in a new texturearray
        for (uint32_t j = i; j < new_images_size; j++) {
            if (new_images[j]->width != current_width ||
                new_images[j]->height != current_height)
            {
                continue;
            }
        }
        
        // assign the new image and request an update so that
        // it gets copied to gpu memory by the platform layer
        // texture_arrays[new_i].image = new_image;
        // texture_arrays[new_i].request_update =
        //     true;
    }
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
    
    new_image->rgba_values =
        (uint8_t *)malloc(sizeof(char) * slice_size);
    
    new_image->rgba_values_size = slice_size;
    new_image->width = slice_width;
    new_image->height = slice_height;
    
    uint32_t start_x = 1 + ((x - 1) * slice_width);
    uint32_t start_y = 1 + ((y - 1) * slice_height);
    uint32_t end_y = start_y + slice_height;
    
    uint32_t i = 0;
    for (uint32_t cur_y = start_y; cur_y < end_y; cur_y++)
    {
        // get the pixel that's at [start_x, cur_y]
        // copcur_y slice_width pixels
        uint32_t pixel_i =
            ((start_x - 1) * 4)
                + ((cur_y - 1) * texture_array->image->width * 4);
        assert(i < new_image->rgba_values_size);
        for (uint32_t _ = 0; _ < (slice_width * 4); _++) {

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

