#include "init_application.h"

typedef struct EngineSaveFile {
    float window_left;
    float window_width;
    float window_bottom;
    float window_height;
    float music_volume;
    float sound_volume;
} EngineSaveFile;

static EngineSaveFile * engine_save_file = NULL;

void init_application(void)
{
    init_memory_store();
    
    keypress_map = (bool32_t *)malloc_from_unmanaged(
        sizeof(bool32_t) * KEYPRESS_MAP_SIZE);
    for (uint32_t i = 0; i < KEYPRESS_MAP_SIZE; i++) {
        keypress_map[i] = false;
    }
    
    init_logger(
        /* void * arg_malloc_function(size_t size): */
            malloc_from_unmanaged,
        /* uint32_t (* arg_create_mutex_function)(void): */
            platform_init_mutex_and_return_id,
        /* void arg_mutex_lock_function(const uint32_t mutex_id): */
            platform_mutex_lock,
        /* void arg_mutex_unlock_function(const uint32_t mutex_id: */
            platform_mutex_unlock);
    
    engine_save_file = (EngineSaveFile *)malloc_from_unmanaged(
        sizeof(EngineSaveFile));
    
    char full_writable_pathfile[256];
    writable_filename_to_pathfile(
        "enginestate.dat",
        full_writable_pathfile,
        256);
    FileBuffer engine_save;
    engine_save.contents = NULL;
    if (platform_file_exists(full_writable_pathfile)) {
        engine_save.size = platform_get_filesize(full_writable_pathfile);
        engine_save.contents = (char *)malloc_from_managed(engine_save.size);
        platform_read_file(full_writable_pathfile, &engine_save);
        *engine_save_file = *(EngineSaveFile *)engine_save.contents;
        printf("engine save contents: %s\n", engine_save.contents);
    }
    
    window_globals = (WindowGlobals *)malloc_from_unmanaged(
        sizeof(WindowGlobals));
    window_globals->visual_debug_mode = false;
    
    if (engine_save.contents != NULL) {
        window_globals->window_height = engine_save_file->window_height;
        window_globals->window_width  = engine_save_file->window_width;
        window_globals->window_left   = engine_save_file->window_left;
        window_globals->window_bottom = engine_save_file->window_bottom;
        window_globals->titlebar_height = 0.0f;
        platform_music_volume = engine_save_file->music_volume;
        platform_sound_volume = engine_save_file->sound_volume;
    } else {
        window_globals->window_height = INITIAL_WINDOW_HEIGHT;
        window_globals->window_width  = INITIAL_WINDOW_WIDTH;
        window_globals->window_left   = INITIAL_WINDOW_LEFT;
        window_globals->window_bottom = INITIAL_WINDOW_BOTTOM;
        window_globals->titlebar_height = 0.0f;
    }
    
    window_globals->aspect_ratio =
        window_globals->window_height / window_globals->window_width;
    
    init_projection_constants();
    
    init_ui_elements();
    
    zpolygons_to_render = (zPolygon *)malloc_from_unmanaged(
        sizeof(zPolygon) * ZPOLYGONS_TO_RENDER_ARRAYSIZE);
    init_all_meshes();
    zlights_to_apply = (zLightSource *)malloc_from_unmanaged(
        sizeof(zLightSource) * ZLIGHTS_TO_APPLY_ARRAYSIZE);
    particle_effects = (ParticleEffect *)malloc_from_unmanaged(
        sizeof(ParticleEffect) * PARTICLE_EFFECTS_SIZE);
    shatter_effects = (ShatterEffect *)malloc_from_unmanaged(
        sizeof(ShatterEffect) * SHATTER_EFFECTS_SIZE);
    
    shared_gameloop_init();
    terminal_init();
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
    
    user_interactions = (Interaction *)
        malloc_from_unmanaged(sizeof(Interaction) * USER_INTERACTIONS_SIZE);
    for (uint32_t m = 0; m < USER_INTERACTIONS_SIZE; m++) {
        construct_interaction(&user_interactions[m]);
    }
    
    init_renderer();
    
    client_logic_startup();
}

void shared_shutdown_application(void)
{
    log_assert(engine_save_file != NULL);
    
    engine_save_file->music_volume = platform_music_volume;
    engine_save_file->sound_volume = platform_sound_volume;
    
    engine_save_file->window_bottom = window_globals->window_bottom;
    engine_save_file->window_height = window_globals->window_height;
    engine_save_file->window_left = window_globals->window_left;
    engine_save_file->window_width = window_globals->window_width;
    
    uint32_t good = false;
    platform_write_file_to_writables(
        /* const char filepath_inside_writables: */
            "enginestate.dat",
        /* const char * output: */
            (char *)engine_save_file,
        /* output_size: */
            sizeof(EngineSaveFile),
        /* uint32_t good: */
            &good);
}
