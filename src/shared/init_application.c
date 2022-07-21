#include "init_application.h"

void init_application() {
    unmanaged_memory = (uint8_t *)malloc(UNMANAGED_MEMORY_SIZE);
    managed_memory = (uint8_t *)malloc(MANAGED_MEMORY_SIZE);
    
    application_name = client_logic_get_application_name();
    
    setup_log();
    
    // initialize texture arrays
    texture_arrays = (TextureArray *)malloc_from_unmanaged(
        sizeof(TextureArray) * TEXTUREARRAYS_SIZE);
    for (uint32_t i = 0; i < TEXTUREARRAYS_SIZE; i++) {
        texture_arrays[i].images_size = 0;
        texture_arrays[i].single_img_width = 0;
        texture_arrays[i].single_img_height = 0;
        texture_arrays[i].request_init = false;
    }
    
    // initialize font with fontmetrics.dat
    FileBuffer font_metrics_file;
    font_metrics_file.size = platform_get_resource_size(
        /* filename: */ "fontmetrics.dat");
    log_assert(font_metrics_file.size > 0);
    
    font_metrics_file.contents = (char *)malloc_from_unmanaged(
        font_metrics_file.size);
    platform_read_resource_file(
        /* const char * filepath: */
            "fontmetrics.dat",
        /* FileBuffer * out_preallocatedbuffer: */
            &font_metrics_file);
    log_assert(font_metrics_file.good);
    init_font(
        /* raw_fontmetrics_file_contents: */ font_metrics_file.contents,
        /* raw_fontmetrics_file_size: */ font_metrics_file.size);
    
    zlights_to_apply = (zLightSource *)malloc_from_unmanaged(
        sizeof(zLightSource) * ZLIGHTS_TO_APPLY_ARRAYSIZE);
    zlights_transformed = (zLightSource *)malloc_from_unmanaged(
        sizeof(zLightSource) * ZLIGHTS_TO_APPLY_ARRAYSIZE);
    texquads_to_render = (TexQuad *)malloc_from_unmanaged(
        sizeof(TexQuad) * TEXQUADS_TO_RENDER_ARRAYSIZE);
    touchable_triangles = (TriangleArea *)malloc_from_unmanaged(
        sizeof(TriangleArea) * TOUCHABLE_TRIANGLES_ARRAYSIZE);
    
    window_height = platform_get_current_window_height();
    window_width = platform_get_current_window_width();
    log_append("window height set to: ");
    log_append_float(window_height);
    log_append(" - window width set to: ");
    log_append_float(window_width);
    log_append("\n");
    
    init_projection_constants();
    
    client_logic_startup();
    
    construct_interaction(&previous_touch_start);
    construct_interaction(&previous_touch_end);
    construct_interaction(&previous_leftclick_start);
    construct_interaction(&previous_leftclick_end);
    construct_interaction(&previous_touch_or_leftclick_start);
    construct_interaction(&previous_touch_or_leftclick_end);
    construct_interaction(&previous_rightclick_start);
    construct_interaction(&previous_rightclick_end);
    construct_interaction(&previous_mouse_move);
    
    init_renderer();
}
