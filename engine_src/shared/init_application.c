#include "init_application.h"

typedef struct EngineSaveFile {
    bool32_t window_fullscreen;
    float window_left;
    float window_width;
    float window_bottom;
    float window_height;
    float music_volume;
    float sound_volume;
} EngineSaveFile;

static EngineSaveFile * engine_save_file = NULL;

#ifndef LOGGER_IGNORE_ASSERTS
typedef struct SimdTestStruct {
    float imafloat[16];
} SimdTestStruct;
static void test_simd_functions(void) {
    log_assert(sizeof(zLightSource) % (SIMD_FLOAT_LANES * 4) == 0);
    log_assert(sizeof(GPUPolygon)   % (SIMD_FLOAT_LANES * 4) == 0);
    
    log_assert(sizeof(SimdTestStruct) % (SIMD_FLOAT_LANES * 4) == 0);
    SimdTestStruct * structs = malloc_from_managed(
        sizeof(SimdTestStruct) * 10);
    float * sets = malloc_from_managed(
        sizeof(float) * 10);
    SimdTestStruct * muls = malloc_from_managed(
        sizeof(SimdTestStruct) * 10);
    SimdTestStruct * divs = malloc_from_managed(
        sizeof(SimdTestStruct) * 10);
    SimdTestStruct * adds = malloc_from_managed(
        sizeof(SimdTestStruct) * 10);
    SimdTestStruct * maxs = malloc_from_managed(
        sizeof(SimdTestStruct) * 10);
    SimdTestStruct * double_checks = malloc_from_managed(
        sizeof(SimdTestStruct) * 10);
    
    for (uint32_t i = 0; i < 10; i++) {
        memset(structs + i, 0, sizeof(SimdTestStruct));
        memset(double_checks + i, 0, sizeof(SimdTestStruct));
        memset(sets + i, 0, sizeof(float));
        memset(adds + i, 0, sizeof(SimdTestStruct));
        memset(maxs + i, 0, sizeof(SimdTestStruct));
        memset(muls + i, 0, sizeof(SimdTestStruct));
        memset(divs + i, 0, sizeof(SimdTestStruct));
    }
    
    for (uint32_t i = 0; i < 10; i++) {
        for (uint32_t j = 0; j < sizeof(SimdTestStruct) / sizeof(float); j++) {
            sets[i]             = (float)i;
            maxs[i].imafloat[j] = (float)((j % 2) * (i * 2));
            muls[i].imafloat[j] = (float)(i % 4);
            divs[i].imafloat[j] = (float)((i % 2) + 1);
            adds[i].imafloat[j] = (float)((i + 1) % 4);
        }
    }
    
    for (uint32_t j = 0; j < 10; j++) {
        for (uint32_t i = 0; i < sizeof(SimdTestStruct) / sizeof(float); i++) {
            double_checks[j].imafloat[i]  = sets[j];
            double_checks[j].imafloat[i] *= muls[j].imafloat[i];
            double_checks[j].imafloat[i] += adds[j].imafloat[i];
            double_checks[j].imafloat[i] /= divs[j].imafloat[i];
            double_checks[j].imafloat[i] =
                double_checks[j].imafloat[i] > maxs[j].imafloat[i] ?
                    double_checks[j].imafloat[i] :
                    maxs[j].imafloat[i];
        }
        
        float * structs_at = (float *)&structs[j];
        float * muls_at = (float *)&muls[j];
        float * adds_at = (float *)&adds[j];
        float * divs_at = (float *)&divs[j];
        float * maxs_at = (float *)&maxs[j];
        
        for (
            uint32_t i = 0;
            i < sizeof(SimdTestStruct) / sizeof(float);
            i += SIMD_FLOAT_LANES)
        {
            SIMD_FLOAT cur  = simd_load_floats(structs_at + i);
            cur = simd_set_float(sets[j]);
            SIMD_FLOAT mul  = simd_load_floats(muls_at + i);
            SIMD_FLOAT add  = simd_load_floats(adds_at + i);
            SIMD_FLOAT div  = simd_load_floats(divs_at + i);
            SIMD_FLOAT max  = simd_load_floats(maxs_at + i);
            
            cur = simd_mul_floats(cur, mul);
            cur = simd_add_floats(cur, add);
            cur = simd_div_floats(cur, div);
            cur = simd_max_floats(cur, max);
            
            simd_store_floats(structs_at + i, cur);
        }
    }
    
    for (uint32_t i = 0; i < 10; i++) {
        for (uint32_t j = 0; j < 10; j++) {
            log_assert(structs[i].imafloat[j] == double_checks[i].imafloat[j]);
        }
    }
    

    free_from_managed(double_checks);
    free_from_managed(maxs);
    free_from_managed(adds);
    free_from_managed(divs);
    free_from_managed(muls);
    free_from_managed(sets);
    free_from_managed(structs);
}
#endif

void init_application_before_gpu_init(void)
{
    void * unmanaged_memory_store = platform_malloc_unaligned_block(
        UNMANAGED_MEMORY_SIZE);
    void * managed_memory_store = platform_malloc_unaligned_block(
        MANAGED_MEMORY_SIZE);
    init_memory_store(
        unmanaged_memory_store,
        managed_memory_store,
        platform_init_mutex_and_return_id,
        platform_mutex_lock,
        platform_mutex_unlock);
    
    #ifndef LOGGER_IGNORE_ASSERTS
    test_simd_functions();
    #endif
    
    init_obj_parser(malloc_from_managed_infoless, free_from_managed);
    
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
        engine_save.size_without_terminator = platform_get_filesize(
            full_writable_pathfile);
        engine_save.contents = (char *)malloc_from_managed(
            engine_save.size_without_terminator + 1);
        platform_read_file(full_writable_pathfile, &engine_save);
        *engine_save_file = *(EngineSaveFile *)engine_save.contents;
    }
    
    window_globals = (WindowGlobals *)malloc_from_unmanaged(
        sizeof(WindowGlobals));
    window_globals->visual_debug_mode = false;
    window_globals->wireframe_mode = false;
    
    init_audio(
        /* void *(*arg_malloc_function)(size_t): */
            malloc_from_unmanaged);
    
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
        window_globals->fullscreen = engine_save_file->window_fullscreen;
        sound_settings->music_volume = engine_save_file->music_volume;
        sound_settings->sfx_volume = engine_save_file->sound_volume;
        window_globals->last_resize_request_at =
            platform_get_current_time_microsecs();
    } else {
        window_globals->fullscreen = false;
        window_globals->window_height = INITIAL_WINDOW_HEIGHT;
        window_globals->window_width  = INITIAL_WINDOW_WIDTH;
        window_globals->window_left   = INITIAL_WINDOW_LEFT;
        window_globals->window_bottom = INITIAL_WINDOW_BOTTOM;
    }
    
    if (engine_save.contents != NULL) {
        free_from_managed(engine_save.contents);
    }
    
    window_globals->aspect_ratio =
        window_globals->window_height / window_globals->window_width;
    
    init_projection_constants();
        
    init_ui_elements();
    
    zpolygons_to_render = (zPolygonCollection *)malloc_from_unmanaged(
        sizeof(zPolygonCollection));
    zpolygons_to_render->size = 0;
    
    init_all_meshes();
    zlights_to_apply = (zLightSource *)malloc_from_unmanaged(
        sizeof(zLightSource) * MAX_LIGHTS_PER_BUFFER);
    lineparticle_effects = (LineParticle *)malloc_from_unmanaged(
        sizeof(LineParticle) * LINEPARTICLE_EFFECTS_SIZE);
    particle_effects = (ParticleEffect *)malloc_from_unmanaged(
        sizeof(ParticleEffect) * PARTICLE_EFFECTS_SIZE);
    
    shared_gameloop_init();
    terminal_init();
    init_scheduled_animations(client_logic_animation_callback);
    init_texture_arrays();
    
    // initialize font with fontmetrics.dat
    FileBuffer font_metrics_file;
    font_metrics_file.size_without_terminator = platform_get_resource_size(
        /* filename: */ "fontmetrics.dat");
    
    if (font_metrics_file.size_without_terminator > 0) {
        font_metrics_file.contents = (char *)malloc_from_unmanaged(
            font_metrics_file.size_without_terminator + 1);
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
                font_metrics_file.size_without_terminator);
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
    
    gpu_shared_data_collection.polygon_materials_allocation_size =
        sizeof(GPUPolygonMaterial) *
        MAX_MATERIALS_PER_POLYGON *
        MAX_POLYGONS_PER_BUFFER;
    gpu_shared_data_collection.polygon_materials_allocation_size +=
        (4096 - (gpu_shared_data_collection.polygon_materials_allocation_size %
            4096));
    assert(gpu_shared_data_collection.polygon_materials_allocation_size
        % 4096 == 0);
    
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
    
    gpu_shared_data_collection.locked_vertices_allocation_size =
        (sizeof(GPULockedVertex) * ALL_LOCKED_VERTICES_SIZE);
    gpu_shared_data_collection.locked_vertices_allocation_size +=
        (4096 - (gpu_shared_data_collection.
            locked_vertices_allocation_size % 4096));
    assert(gpu_shared_data_collection.locked_vertices_allocation_size > 0);
    assert(gpu_shared_data_collection.locked_vertices_allocation_size %
        4096 == 0);
    
    gpu_shared_data_collection.projection_constants_allocation_size =
        sizeof(GPUProjectionConstants);
    gpu_shared_data_collection.projection_constants_allocation_size +=
        (4096 - (gpu_shared_data_collection.
            projection_constants_allocation_size % 4096));
    assert(gpu_shared_data_collection.projection_constants_allocation_size > 0);
    assert(gpu_shared_data_collection.projection_constants_allocation_size %
        4096 == 0);
    
    for (
        uint32_t frame_i = 0;
        frame_i < 3;
        frame_i++)
    {
        gpu_shared_data_collection.triple_buffers[frame_i].vertices =
            (GPUVertex *)malloc_from_unmanaged_aligned(
                gpu_shared_data_collection.vertices_allocation_size,
                4096);
        
        gpu_shared_data_collection.triple_buffers[frame_i].polygon_collection =
            (GPUPolygonCollection *)malloc_from_unmanaged_aligned(
                gpu_shared_data_collection.polygons_allocation_size,
                4096);
        
        gpu_shared_data_collection.triple_buffers[frame_i].polygon_materials =
            (GPUPolygonMaterial *)malloc_from_unmanaged_aligned(
                gpu_shared_data_collection.polygon_materials_allocation_size,
                4096);
        
        assert(gpu_shared_data_collection.lights_allocation_size > 0);
        gpu_shared_data_collection.triple_buffers[frame_i].light_collection =
            (GPULightCollection *)malloc_from_unmanaged_aligned(
                gpu_shared_data_collection.lights_allocation_size,
                4096);
        
        gpu_shared_data_collection.triple_buffers[frame_i].camera =
        (GPUCamera *)malloc_from_unmanaged_aligned(
            gpu_shared_data_collection.camera_allocation_size,
            4096);
        
        gpu_shared_data_collection.triple_buffers[frame_i].camera->xyz[0] = 0.0f;
        gpu_shared_data_collection.triple_buffers[frame_i].camera->xyz[1] = 0.0f;
        gpu_shared_data_collection.triple_buffers[frame_i].camera->xyz[2] = 0.0f;
        gpu_shared_data_collection.
            triple_buffers[frame_i].camera->xyz_angle[0] = 0.0f;
        gpu_shared_data_collection.
            triple_buffers[frame_i].camera->xyz_angle[1] = 0.0f;
        gpu_shared_data_collection.
            triple_buffers[frame_i].camera->xyz_angle[2] = 0.0f;
    }
    
    gpu_shared_data_collection.locked_vertices =
        (GPULockedVertex *)malloc_from_unmanaged_aligned(
            gpu_shared_data_collection.locked_vertices_allocation_size,
            4096);
    
    gpu_shared_data_collection.locked_pjc =
        (GPUProjectionConstants *)malloc_from_unmanaged_aligned(
            gpu_shared_data_collection.projection_constants_allocation_size,
            4096);
    
    client_logic_early_startup();
}

void init_application_after_gpu_init(void) {
    printf("init_application_after_gpu_init()...\n");
    
    #define MIN_VERTICES_FOR_SHATTER_EFFECT 400
    for (uint32_t i = 0; i < all_mesh_summaries_size; i++) {
        if (all_mesh_summaries[i].shattered_vertices_head_i < 0) {
            if (
                all_mesh_summaries[i].shattered_vertices_size <
                    MIN_VERTICES_FOR_SHATTER_EFFECT)
            {
                create_shattered_version_of_mesh(
                    /* const int32_t mesh_id: */
                        all_mesh_summaries[i].mesh_id,
                    /* const uint32_t triangles_mulfiplier: */
                        (MIN_VERTICES_FOR_SHATTER_EFFECT /
                            (uint32_t)all_mesh_summaries[i].vertices_size) + 1);
                log_assert(
                    all_mesh_summaries[i].shattered_vertices_head_i >= 0);
            } else {
                all_mesh_summaries[i].shattered_vertices_head_i =
                    all_mesh_summaries[i].vertices_head_i;
                all_mesh_summaries[i].shattered_vertices_size =
                    all_mesh_summaries[i].vertices_size;
            }
        }
    }
    
    memcpy(
        /* void * dst: */
            gpu_shared_data_collection.locked_vertices,
        /* const void * src: */
            all_mesh_vertices->gpu_data,
        /* size_t n: */
            sizeof(GPULockedVertex) * ALL_LOCKED_VERTICES_SIZE);
    platform_gpu_copy_locked_vertices();
    platform_gpu_update_viewport();
    
    if (window_globals->fullscreen) {
        // TODO: reimplement me!
        // platform_enter_fullscreen();
    }
    
    client_logic_late_startup();
}

void shared_shutdown_application(void)
{
    log_assert(engine_save_file != NULL);
    
    engine_save_file->music_volume = sound_settings->music_volume;
    engine_save_file->sound_volume = sound_settings->sfx_volume;
    
    engine_save_file->window_bottom = window_globals->window_bottom;
    engine_save_file->window_height = window_globals->window_height;
    engine_save_file->window_left = window_globals->window_left;
    engine_save_file->window_width = window_globals->window_width;
    engine_save_file->window_fullscreen = window_globals->fullscreen;
    
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
