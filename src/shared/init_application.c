#include "init_application.h"

void init_application() {
    init_memory_store();
    
    setup_log();
    
    init_scheduled_animations();
    init_texture_arrays();
    
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
    aspect_ratio = window_height / window_width;
    
    init_projection_constants();
    
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
    
    client_logic_startup();
    
    block_drawinmtkview = false;
}
