#include "texture_array.h"

DecodedImage * extract_image(
    TextureArray * texture_array,
    uint32_t x,
    uint32_t y)
{
    printf("extracting image from [%u,%u]\n", x, y);
    assert(x > 0);
    assert(y > 0);
    assert(x <= texture_array->sprite_columns);
    assert(y <= texture_array->sprite_rows);
    
    DecodedImage * new_image = malloc(sizeof(DecodedImage));

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
    
    new_image->rgba_values = malloc(sizeof(char) * slice_size);
    
    new_image->rgba_values_size = slice_size;
    new_image->width = slice_width;
    new_image->height = slice_height;
    
    uint32_t start_x = 1 + ((x - 1) * slice_width);
    uint32_t start_y = 1 + ((y - 1) * slice_height);
    uint32_t end_y = start_y + slice_height;
    uint32_t end_x = start_x + slice_width;
    
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

    printf(
        "returning new image of [%u,%u]\n",
        new_image->width,
        new_image->height);
    
    return new_image;
}

