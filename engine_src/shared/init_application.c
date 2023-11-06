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
        /* int32_t arg_mutex_unlock_function(const uint32_t mutex_id): */
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
    }
    
    window_globals = (WindowGlobals *)malloc_from_unmanaged(
        sizeof(WindowGlobals));
    window_globals->visual_debug_mode = false;
    
    if (
        engine_save.contents != NULL &&
        engine_save_file->window_height > 10 &&
        engine_save_file->window_height < INITIAL_WINDOW_HEIGHT * 3 &&
        engine_save_file->window_width > 10 &&
        engine_save_file->window_width < INITIAL_WINDOW_WIDTH * 3)
    {
        window_globals->window_height = engine_save_file->window_height;
        window_globals->window_width  = engine_save_file->window_width;
        window_globals->window_left   = engine_save_file->window_left;
        window_globals->window_bottom = engine_save_file->window_bottom;
        platform_music_volume = engine_save_file->music_volume;
        platform_sound_volume = engine_save_file->sound_volume;
        window_globals->last_resize_request_at =
            platform_get_current_time_microsecs();
    } else {
        window_globals->window_height = INITIAL_WINDOW_HEIGHT;
        window_globals->window_width  = INITIAL_WINDOW_WIDTH;
        window_globals->window_left   = INITIAL_WINDOW_LEFT;
        window_globals->window_bottom = INITIAL_WINDOW_BOTTOM;
    }
    
    window_globals->aspect_ratio =
        window_globals->window_height / window_globals->window_width;
    
    init_projection_constants();
    
    init_ui_elements();
    
    zpolygons_to_render = (zPolygon *)malloc_from_unmanaged(
        sizeof(zPolygon) * MAX_POLYGONS_PER_BUFFER);
    init_all_meshes();
    zlights_to_apply = (zLightSource *)malloc_from_unmanaged(
        sizeof(zLightSource) * MAX_LIGHTS_PER_BUFFER);
    particle_effects = (ParticleEffect *)malloc_from_unmanaged(
        sizeof(ParticleEffect) * PARTICLE_EFFECTS_SIZE);
    shatter_effects = (ShatterEffect *)malloc_from_unmanaged(
        sizeof(ShatterEffect) * SHATTER_EFFECTS_SIZE);
    
    shared_gameloop_init();
    terminal_init();
    init_scheduled_animations();
    init_texture_arrays();
    
    // initialize font with fontmetrics.dat
    FileBuffer font_metrics_file;
    font_metrics_file.size = platform_get_resource_size(
        /* filename: */ "fontmetrics.dat");
    
    if (font_metrics_file.size > 0) {
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
    }
    
    user_interactions = (Interaction *)
        malloc_from_unmanaged(sizeof(Interaction) * USER_INTERACTIONS_SIZE);
    for (uint32_t m = 0; m < USER_INTERACTIONS_SIZE; m++) {
        construct_interaction(&user_interactions[m]);
    }
    
    init_renderer();
    
    // init the buffers that contain our vertices to send to the GPU
    gpu_shared_data_collection.vertices_allocation_size =
            sizeof(GPUVertex) * MAX_VERTICES_PER_BUFFER;
    gpu_shared_data_collection.vertices_allocation_size +=
        (4096 - (gpu_shared_data_collection.vertices_allocation_size % 4096));
    assert(gpu_shared_data_collection.vertices_allocation_size % 4096 == 0);
    
    gpu_shared_data_collection.polygons_allocation_size =
        sizeof(GPUPolygonCollection);
    gpu_shared_data_collection.polygons_allocation_size +=
        (4096 - (gpu_shared_data_collection.polygons_allocation_size % 4096));
    assert(gpu_shared_data_collection.polygons_allocation_size > 0);
    assert(gpu_shared_data_collection.polygons_allocation_size % 4096 == 0);
    
    gpu_shared_data_collection.lights_allocation_size =
        sizeof(GPULightCollection);
    gpu_shared_data_collection.lights_allocation_size +=
        (4096 - (gpu_shared_data_collection.lights_allocation_size % 4096));
    assert(gpu_shared_data_collection.lights_allocation_size > 0);
    assert(gpu_shared_data_collection.lights_allocation_size % 4096 == 0);
    
    gpu_shared_data_collection.camera_allocation_size = sizeof(GPUCamera);
    gpu_shared_data_collection.camera_allocation_size +=
        (4096 - (gpu_shared_data_collection.camera_allocation_size % 4096));
    assert(gpu_shared_data_collection.camera_allocation_size % 4096 == 0);
    
    gpu_shared_data_collection.projection_constants_allocation_size =
        sizeof(GPUProjectionConstants);
    gpu_shared_data_collection.projection_constants_allocation_size +=
        (4096 - (gpu_shared_data_collection.
            projection_constants_allocation_size % 4096));
    assert(gpu_shared_data_collection.
        projection_constants_allocation_size % 4096 == 0);
    
    for (
        uint32_t frame_i = 0;
        frame_i < 3;
        frame_i++)
    {
        gpu_shared_data_collection.triple_buffers[frame_i].vertices =
            (GPUVertex *)malloc_from_unmanaged_aligned(
                gpu_shared_data_collection.vertices_allocation_size,
                4096);
        
        #ifndef LOGGER_IGNORE_ASSERTS
        for (uint32_t i = 0; i < MAX_VERTICES_PER_BUFFER; i++) {
            gpu_shared_data_collection.triple_buffers[frame_i].
                vertices[i].texture_i = -1;
            gpu_shared_data_collection.triple_buffers[frame_i].
                vertices[i].texturearray_i = -1;
        }
        #endif
        
        gpu_shared_data_collection.triple_buffers[frame_i].polygon_collection =
            (GPUPolygonCollection *)malloc_from_unmanaged_aligned(
                gpu_shared_data_collection.polygons_allocation_size,
                4096);
        
        gpu_shared_data_collection.triple_buffers[frame_i].light_collection =
            (GPULightCollection *)malloc_from_unmanaged_aligned(
                gpu_shared_data_collection.lights_allocation_size,
                4096);
        
        gpu_shared_data_collection.triple_buffers[frame_i].camera =
            (GPUCamera *)malloc_from_unmanaged_aligned(
                gpu_shared_data_collection.camera_allocation_size,
                4096);
        
        gpu_shared_data_collection.triple_buffers[frame_i].camera->x = 0.0f;
        gpu_shared_data_collection.triple_buffers[frame_i].camera->y = 0.0f;
        gpu_shared_data_collection.triple_buffers[frame_i].camera->z = 0.0f;
        gpu_shared_data_collection.
            triple_buffers[frame_i].camera->x_angle = 0.0f;
        gpu_shared_data_collection.
            triple_buffers[frame_i].camera->y_angle = 0.0f;
        gpu_shared_data_collection.
            triple_buffers[frame_i].camera->z_angle = 0.0f;
        
        gpu_shared_data_collection.triple_buffers[frame_i].
            projection_constants =
                (GPUProjectionConstants *)malloc_from_unmanaged_aligned(
                    gpu_shared_data_collection.
                        projection_constants_allocation_size,
                    4096);
    }
    
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
    
    char memory_usage_desc[512];
    get_memory_usage_summary_string(
        /* char * recipient: */
            memory_usage_desc,
        /* const uint32_t recipient_cap: */
            512);
    log_append(memory_usage_desc);
}
