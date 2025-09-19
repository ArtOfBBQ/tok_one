#include "T1_decodedimage.h"

uint64_t T1_decodedimage_get_sum_rgba(
    const T1DecodedImage * input)
{
    assert(input->rgba_values_size > 0);
    
    uint64_t return_value = 0;
    for (uint32_t i = 0; i < input->rgba_values_size; i++) {
        return_value += input->rgba_values_page_aligned[i];
    }
    
    return return_value;
}

uint32_t T1_decodedimage_get_avg_rgba(
    const T1DecodedImage * input)
{
    assert(input->rgba_values_size > 0);
    
    return (uint32_t)(T1_decodedimage_get_sum_rgba(input) / input->rgba_values_size);
}

void overwrite_subregion(
    T1DecodedImage * whole_image,
    const T1DecodedImage * new_image,
    const uint32_t column_count,
    const uint32_t row_count,
    const uint32_t at_column,
    const uint32_t at_row)
{
    assert(at_column > 0);
    assert(at_row > 0);
    
    if (at_column > column_count) {
        #ifndef DECODED_IMAGE_SILENCE
        printf(
            "can't write at [%u,%u], if only %u total columns\n",
            at_column,
            at_row,
            column_count);
        #endif
        return;
    }
    
    if (at_row > row_count) {
        #ifndef DECODED_IMAGE_SILENCE
        printf(
            "can't write at [%u,%u], if only %u total rows\n",
            at_column,
            at_row,
            row_count);
        #endif
        return;
    }
    
    assert(at_row <= row_count);
    uint32_t expected_width = whole_image->width / column_count;
    uint32_t expected_height = whole_image->height / row_count;
    assert(expected_width * column_count == whole_image->width);
    assert(expected_height * row_count == whole_image->height);
    if (
        (new_image->width != expected_width)
        ||
        (new_image->height != expected_height))
    {
        #ifndef DECODED_IMAGE_SILENCE
        printf(
            "Error - can't overwrite chunk [%u,%u] of dimensions [%u,%u] for image (%u x %u) with new subimage sized [%u,%u], expected size [%u,%u]\n",
            at_column,
            at_row,
            column_count,
            row_count,
            whole_image->width,
            whole_image->height,
            new_image->width,
            new_image->height,
            expected_width,
            expected_height);
        #endif
        
        return;
    }
    
    uint32_t slice_width =
        whole_image->width / column_count;
    uint32_t slice_height =
        whole_image->height / row_count;
    uint32_t start_x = 1 + ((at_column - 1) * slice_width);
    uint32_t start_y = 1 + ((at_row - 1) * slice_height);
    uint32_t end_y = start_y + slice_height - 1;
    
    #ifndef NDEBUG
    uint32_t end_x = start_x + slice_width - 1;
    #endif
    
    assert(end_x <= whole_image->width);
    assert(end_y <= whole_image->height);
    
    uint32_t i = 0;
    for (
        uint32_t cur_y = start_y;
        cur_y <= end_y;
        cur_y++)
    {
        uint32_t pixel_i =
            ((start_x - 1) * 4)
                + ((cur_y - 1) * whole_image->width * 4);
        assert(pixel_i < whole_image->rgba_values_size);
        for (uint32_t _ = 0; _ < (slice_width * 4); _++)
        {
            if (i >= whole_image->rgba_values_size) {
                break;
            }
            whole_image->rgba_values_page_aligned[pixel_i + _] =
                new_image->rgba_values_page_aligned[i];
            i++;
        }
    }
}
