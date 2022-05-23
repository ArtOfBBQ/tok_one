#include "texture_array.h"

TextureArray texture_arrays[TEXTUREARRAYS_SIZE];
uint32_t texture_arrays_size = 0;

void update_texturearray_from_0terminated_files(
    const int32_t texturearray_i,
    const char filenames
        [MAX_FILES_IN_SINGLE_TEXARRAY]
        [MAX_ASSET_FILENAME_SIZE])
{
    printf(
        "update_texarray_from_0terminated_files for ta_i %i\n",
        texturearray_i);
    assert(texturearray_i < texture_arrays_size);
    
    uint32_t filenames_size = 0;
    uint32_t t_i = 0;
    while (filenames[t_i][0] != '\0')
    {
        filenames_size += 1;
        t_i++;
    }
    
    DecodedImage * decoded_images[filenames_size];
    
    t_i = 0;
    while (
        t_i < MAX_FILES_IN_SINGLE_TEXARRAY
        && filenames[t_i][0] != '\0')
    {
        printf("t_i: %i\n", t_i);
        printf("the filename is %s\n", filenames[t_i]);
        const char * filename = filenames[t_i];
        
        FileBuffer file_buffer;
        file_buffer.size = platform_get_filesize(filename) + 1;
        printf("expecting file size: %lu\n", file_buffer.size);
        
        assert(file_buffer.size > 1);
        file_buffer.contents = 
            (char *)malloc(file_buffer.size);
        printf("malloc of file buffer contents succesful\n");
        
        platform_read_file(
            filename,
            &file_buffer);
        
        printf("file read succesful\n");
        
        DecodedImage * new_image =
            (DecodedImage *)malloc(sizeof(DecodedImage));
        new_image->good = false;
        
        get_PNG_width_height(
            /* uint8_t * compressed_bytes: */
                (uint8_t *)file_buffer.contents,
            /* uint32_t compressed_bytes_size: */
                50,
            /* uint32_t * width_out: */
                &new_image->width,
            /* uint32_t * height_out: */
                &new_image->height);
        
        assert(new_image->width > 0);
        assert(new_image->height > 0);
        printf(
            "new_image has dimensions: [%u,%u]\n",
            new_image->width,
            new_image->height);
        
        new_image->rgba_values_size =
            new_image->width * new_image->height * 4;
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
        
        printf(
            "decode_PNG returned with good: %u\n",
            new_image->good);
        
        assert(new_image->good);
        if (new_image->pixel_count * 4 !=
            new_image->rgba_values_size)
        {
            printf(
                "ERR: we loaded an image with with pixel_count of %u (so *4 = %u rgba values), and rgba_values_size of %u. Image dimensions were [%u,%u], so width*height*4 would have been %u\n",
                new_image->pixel_count,
                new_image->pixel_count * 4,
                new_image->rgba_values_size,
                new_image->width,
                new_image->height,
                new_image->width * new_image->height * 4);
            assert(0);
        }
        
        decoded_images[t_i] = new_image;
        printf("free file_buffer.conents..\n");
        free(file_buffer.contents);
        printf(
            "free succesful, new_img dimensions now; [%u,%u]\n",
            new_image->width,
            new_image->height);
        
        t_i++;
    }
    
    printf("finished decoding png's, allocate img mem...\n"); 
    fflush(stdout);
    texture_arrays[texturearray_i].image =
        (DecodedImage *)malloc(sizeof(DecodedImage));
    
    printf("concatenate %u images...\n", t_i); 
    fflush(stdout);
    for (uint32_t print_i = 0; print_i < t_i; print_i++) {
        printf(
            "[%u,%u],",
            decoded_images[print_i]->width,
            decoded_images[print_i]->height);
        fflush(stdout);
    }
    printf("\n");
    fflush(stdout);
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
    assert(texture_arrays[texturearray_i].image->width > 0);
    assert(texture_arrays[texturearray_i].image->height > 0);
    texture_arrays[texturearray_i].request_update = true;
}

// returns new_texture_array_i (index in texture_arrays)
void register_new_texturearray_from_files(
    const char ** filenames,
    const uint32_t filenames_size)
{
    printf(
        "register_new_texturearray_from_files (%u files)\n",
        filenames_size);
    uint32_t decoded_images_size = filenames_size;
    DecodedImage * decoded_images[decoded_images_size];   
    
    for (
        uint32_t i = 0;
        i < filenames_size;
        i++)
    {
        const char * filename = filenames[i];
        
        FileBuffer file_buffer;
        file_buffer.size = platform_get_filesize(filename) + 1;
        char filebuffer_contents[file_buffer.size];
        assert(file_buffer.size > 1);
        file_buffer.contents =
            (char *)&filebuffer_contents;
        platform_read_file(
            filename,
            &file_buffer);
        
        DecodedImage * new_image =
            (DecodedImage *)malloc(sizeof(DecodedImage *));
        new_image->good = false;
        
        get_PNG_width_height(
            /* uint8_t * compressed_bytes: */
                (uint8_t *)file_buffer.contents,
            /* uint32_t compressed_bytes_size: */
                50,
            /* uint32_t * width_out: */
                &new_image->width,
            /* uint32_t * height_out: */
                &new_image->height);

        assert(new_image->width > 0);
        assert(new_image->height > 0);
        
        new_image->rgba_values = (uint8_t *)malloc(
            new_image->width * new_image->height * 4);
        
        decode_PNG(
            /* compressed_bytes: */
                (uint8_t *)file_buffer.contents,
            /* compressed_bytes_size: */
                (uint32_t)(file_buffer.size - 1),
            /* DecodedImage * out_preallocated_png: */
                new_image);
    }
    
    register_new_texturearray_from_images(
        /* DecodedImage ** new_images : */
            (const DecodedImage **)&decoded_images[0],
        /* new_images_size: */
            decoded_images_size);

    printf(
        "finished register_new_texturearray_from_files (%u files)\n",
        filenames_size);
}

void register_new_texturearray_from_images(
    const DecodedImage ** new_images,
    const uint32_t new_images_size)
{
    printf(
        "register_new_texturearray_from_images (%u images)\n",
        new_images_size);
    
    assert(new_images_size > 0);
    
    uint32_t current_width = new_images[0]->width;
    uint32_t current_height = new_images[0]->height;
    if (
        current_width == 0
        || current_height == 0)
    {
        printf("ERR - register images with width/height 0\n");
        assert(0);
    }

    if (current_width > 100000 || current_height > 100000) {
        printf("ERR - register images with big width/height\n");
        assert(0);
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
        printf(
            "texture_arrays[%u] was in use, free memory...\n",
            new_i);
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
    
    printf(
        "finished register_new_texturearray_from_images (%u images)\n",
        new_images_size);
}

void register_new_texturearray(
    const DecodedImage * new_image)
{
    printf("register_new_texturearray\n");
    
    assert(new_image != NULL);
    assert(new_image->width > 0);
    assert(new_image->height > 0);
    const DecodedImage * images[1];
    images[0] = new_image;
    register_new_texturearray_from_images(
        (const DecodedImage **)&images[0],
        1);

    printf("finished register_new_texturearray\n");
}

DecodedImage * extract_image(
    TextureArray * texture_array,
    uint32_t x,
    uint32_t y)
{
    printf(
        "extract_image at position [%u,%u]\n",
        x,
        y); 
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
    printf(
        "new_image->rgba_values_size: %u\n",
        slice_size); 
    printf(
        "because texture_array->sprite_columns was: %u\n",
        texture_array->sprite_columns);
    printf(
        "because texture_array->sprite_rows was: %u\n",
        texture_array->sprite_rows);
    printf(
        "because texture_array->image->width was %u\n",
        texture_array->image->width);
    printf(
        "because texture_array->image->height was %u\n",
        texture_array->image->height);
    printf(
        "because texture_array->rgba_values_size was: %u\n",
        texture_array->image->rgba_values_size);
    
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
    
    printf(
        "finished extract_image, new image has dims[%u,%u] and rgba_values_size of %u\n",
        new_image->width,
        new_image->height,
        new_image->rgba_values_size); 
    return new_image;
}

