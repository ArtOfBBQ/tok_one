#include "init_application.h"

void init_application() {
    init_memory_store();
    
    setup_log();
    
    window_globals = (WindowGlobals *)malloc_from_unmanaged(
        sizeof(WindowGlobals));
    keypress_map = (bool32_t *)malloc_from_unmanaged(
        sizeof(bool32_t) * KEYPRESS_MAP_SIZE);
    zpolygons_to_render = (zPolygon *)malloc_from_unmanaged(
        sizeof(zPolygon) * ZPOLYGONS_TO_RENDER_ARRAYSIZE);
    zlights_to_apply = (zLightSource *)malloc_from_unmanaged(
        sizeof(zLightSource) * ZLIGHTS_TO_APPLY_ARRAYSIZE);

    
    init_scheduled_animations();
    init_texture_arrays();

    gpu_shared_data_collection.triple_buffers[0].touchable_pixels = NULL;
    gpu_shared_data_collection.triple_buffers[1].touchable_pixels = NULL;
    gpu_shared_data_collection.triple_buffers[2].touchable_pixels = NULL;
    
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
        /* raw_fontmetrics_file_contents: */
            font_metrics_file.contents,
        /* raw_fontmetrics_file_size: */
            font_metrics_file.size);
        
    window_globals->window_height = platform_get_current_window_height();
    window_globals->window_width = platform_get_current_window_width();
    window_globals->aspect_ratio =
        window_globals->window_height / window_globals->window_width;
    
    init_projection_constants();
    
    user_interactions = (Interaction *)
        malloc_from_unmanaged(sizeof(Interaction) * USER_INTERACTIONS_SIZE);
    for (uint32_t m = 0; m < USER_INTERACTIONS_SIZE; m++) {
        construct_interaction(&user_interactions[m]);
    }
    
    init_renderer();
    
    client_logic_startup();
    
    block_drawinmtkview = false;
}
