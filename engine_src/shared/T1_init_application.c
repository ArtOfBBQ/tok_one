#include "T1_init_application.h"

#define IMAGE_DECODING_THREADS_MAX 10
typedef struct InitApplicationState {
    uint32_t image_decoding_threads;
    uint32_t all_finished;
    uint32_t thread_finished[IMAGE_DECODING_THREADS_MAX];
} InitApplicationState;

static InitApplicationState * ias;

#define DPNG_WORKING_MEMORY_SIZE 35000000

#if ENGINE_SAVEFILE_ACTIVE
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
#endif

#ifndef LOGGER_IGNORE_ASSERTS
typedef struct SimdTestStruct {
    float imafloat[16];
} SimdTestStruct;
static void test_simd_functions_floats(void) {
    log_assert(sizeof(zLightSource) % (SIMD_FLOAT_LANES * 4) == 0);
    log_assert(sizeof(GPUzSprite)   % (SIMD_FLOAT_LANES * 4) == 0);
    
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
    SimdTestStruct * equals = malloc_from_managed(
        sizeof(SimdTestStruct) * 10);
    
    common_memset_char(structs, 0, sizeof(SimdTestStruct)*10);
    common_memset_char(double_checks, 0, sizeof(SimdTestStruct)*10);
    common_memset_char(sets, 0, sizeof(float));
    common_memset_char(adds, 0, sizeof(SimdTestStruct)*10);
    common_memset_char(maxs, 0, sizeof(SimdTestStruct)*10);
    common_memset_char(muls, 0, sizeof(SimdTestStruct)*10);
    common_memset_char(divs, 0, sizeof(SimdTestStruct)*10);
    common_memset_float(equals, 2.0f, sizeof(SimdTestStruct)*10);
    
    for (uint32_t i = 0; i < 10; i++) {
        sets[i] = (float)i;
        for (uint32_t j = 0; j < sizeof(SimdTestStruct) / sizeof(float); j++) {
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
            //            float equals_bonus = double_checks[j].imafloat[i] ==
            //                equals[j].imafloat[i];
            //            printf("equals_bonus: %f\n", equals_bonus);
            //            double_checks[j].imafloat[i] += equals_bonus;
        }
    }
    
    for (uint32_t j = 0; j < 10; j++) {
        float * structs_at = (float *)&structs[j];
        float * muls_at    = (float *)&muls[j];
        float * adds_at    = (float *)&adds[j];
        float * divs_at    = (float *)&divs[j];
        float * maxs_at    = (float *)&maxs[j];
        // float * equals_at  = (float *)&equals[j];
        
        // float one = 1.0f;
        // SIMD_FLOAT all_ones   = simd_set1_float(one);
        for (
            uint32_t i = 0;
            i < sizeof(SimdTestStruct) / sizeof(float);
            i += SIMD_FLOAT_LANES)
        {
            SIMD_FLOAT cur  = simd_load_floats(structs_at + i);
            cur = simd_set1_float(sets[j]);
            SIMD_FLOAT mul    = simd_load_floats(muls_at + i);
            SIMD_FLOAT add    = simd_load_floats(adds_at + i);
            SIMD_FLOAT div    = simd_load_floats(divs_at + i);
            SIMD_FLOAT max    = simd_load_floats(maxs_at + i);
            // SIMD_FLOAT eq     = simd_load_floats(equals_at + i);
            
            
            cur = simd_mul_floats(cur, mul);
            cur = simd_add_floats(cur, add);
            cur = simd_div_floats(cur, div);
            cur = simd_max_floats(cur, max);
            
            //            SIMD_FLOAT equals_bonuses =
            //                simd_and_floats(all_ones, simd_cmpeq_floats(cur, eq));
            //            cur = simd_add_floats(cur, equals_bonuses);
            
            simd_store_floats(structs_at + i, cur);
        }
    }
    
    for (uint32_t i = 0; i < 10; i++) {
        for (uint32_t j = 0; j < 16; j++) {
            log_assert(
                (structs[i].imafloat[j] - double_checks[i].imafloat[j]) <  0.01f &&
                (structs[i].imafloat[j] - double_checks[i].imafloat[j]) > -0.01f);
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

static uint32_t pad_to_page_size(uint32_t base_allocation) {
    uint32_t return_value = base_allocation +
        (page_size - (base_allocation % page_size));
    assert(return_value % page_size == 0);
    return return_value;
}

void init_application_before_gpu_init(
    bool32_t * success,
    char * error_message)
{
    *success = true;
    error_message[0] = '\0';
    
    void * unmanaged_memory_store = platform_malloc_unaligned_block(
        UNMANAGED_MEMORY_SIZE + 7232);
    
    platform_layer_init(&unmanaged_memory_store, 32);
    
    memorystore_init(
        unmanaged_memory_store,
        platform_init_mutex_and_return_id,
        platform_mutex_lock,
        platform_mutex_unlock);
    
    ias = malloc_from_unmanaged(sizeof(InitApplicationState));
    
    init_PNG_decoder(
        /* void *(*malloc_funcptr)(size_t): */
            malloc_from_managed_infoless,
        /* free_function: */
            free_from_managed,
        /* memset_function: */
            common_memset_char,
        /* memcpy_function: */
            common_memcpy,
        /* dpng_working_memory_size: */
            DPNG_WORKING_MEMORY_SIZE,
        /* const uint32_t thread_id: */
            0);
    
    #ifndef LOGGER_IGNORE_ASSERTS
    test_simd_functions_floats();
    #endif
    
    uint32_t good = 0;
    toktoken_init(
        common_memset_char,
        malloc_from_managed_infoless,
        &good);
    log_assert(good);
    
    T1_objparser_init(malloc_from_managed_infoless, free_from_managed);
    mtlparser_init(
        common_memset_char,
        malloc_from_managed_infoless,
        strlcat);
    
    keypress_map = (bool32_t *)malloc_from_unmanaged(
        sizeof(bool32_t) * KEYPRESS_MAP_SIZE);
    for (uint32_t i = 0; i < KEYPRESS_MAP_SIZE; i++) {
        keypress_map[i] = false;
    }
    
    logger_init(
        /* void * arg_malloc_function(size_t size): */
            malloc_from_unmanaged,
        /* uint32_t (* arg_create_mutex_function)(void): */
            platform_init_mutex_and_return_id,
        /* void arg_mutex_lock_function(const uint32_t mutex_id): */
            platform_mutex_lock,
        /* int32_t arg_mutex_unlock_function(const uint32_t mutex_id): */
            platform_mutex_unlock);
    
    #if PROFILER_ACTIVE
    profiler_init(platform_get_clock_frequency(), malloc_from_unmanaged);
    #endif
    
    #if ENGINE_SAVEFILE_ACTIVE
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
    #endif
    
    engine_globals = (EngineGlobals *)malloc_from_unmanaged(
        sizeof(EngineGlobals));
    common_memset_char(engine_globals, 0, sizeof(EngineGlobals));
    
    #if AUDIO_ACTIVE
    audio_init(
        /* void *(*arg_malloc_function)(size_t): */
            malloc_from_unmanaged);
    #endif
    
    #if ENGINE_SAVEFILE_ACTIVE
    if (
        engine_save.contents != NULL &&
        engine_save_file->window_height > 20 &&
        engine_save_file->window_height < INITIAL_WINDOW_HEIGHT * 3 &&
        engine_save_file->window_width > 20 &&
        engine_save_file->window_width < INITIAL_WINDOW_WIDTH * 3)
    {
        engine_globals->window_height = engine_save_file->window_height;
        engine_globals->window_width  = engine_save_file->window_width;
        engine_globals->window_left   = engine_save_file->window_left;
        engine_globals->window_bottom = engine_save_file->window_bottom;
        engine_globals->fullscreen = engine_save_file->window_fullscreen;
        #if AUDIO_ACTIVE
        sound_settings->music_volume = engine_save_file->music_volume;
        sound_settings->sfx_volume = engine_save_file->sound_volume;
        #endif
    } else {
        engine_globals->fullscreen = false;
        engine_globals->window_height = INITIAL_WINDOW_HEIGHT;
        engine_globals->window_width  = INITIAL_WINDOW_WIDTH;
        engine_globals->window_left   = INITIAL_WINDOW_LEFT;
        engine_globals->window_bottom = INITIAL_WINDOW_BOTTOM;
    }
    
    if (engine_save.contents != NULL) {
        free_from_managed(engine_save.contents);
    }
    #else
    engine_globals->fullscreen = false;
    engine_globals->window_height = INITIAL_WINDOW_HEIGHT;
    engine_globals->window_width  = INITIAL_WINDOW_WIDTH;
    engine_globals->window_left   = INITIAL_WINDOW_LEFT;
    engine_globals->window_bottom = INITIAL_WINDOW_BOTTOM;
    #if AUDIO_ACTIVE
    sound_settings->music_volume  = 0.5f;
    sound_settings->sfx_volume    = 0.5f;
    #endif
    #endif
    
    engine_globals->aspect_ratio = engine_globals->window_height /
        engine_globals->window_width;
    
    engineglobals_init();
    
    uielement_init();
    
    zsprites_to_render = (zSpriteCollection *)malloc_from_unmanaged(
        sizeof(zSpriteCollection));
    zsprites_to_render->size = 0;
    
    T1_material_init(malloc_from_unmanaged);
    
    T1_objmodel_init();
    zlights_to_apply = (zLightSource *)malloc_from_unmanaged(
        sizeof(zLightSource) * MAX_LIGHTS_PER_BUFFER);
    common_memset_char(
        zlights_to_apply,
        0,
        sizeof(zLightSource) * MAX_LIGHTS_PER_BUFFER);
    
    #if PARTICLES_ACTIVE
    lineparticle_effects = (LineParticle *)malloc_from_unmanaged(
        sizeof(LineParticle) * LINEPARTICLE_EFFECTS_SIZE);
    common_memset_char(
        lineparticle_effects,
        0,
        sizeof(LineParticle) * LINEPARTICLE_EFFECTS_SIZE);
    particle_effects = (ParticleEffect *)malloc_from_unmanaged(
        sizeof(ParticleEffect) * PARTICLE_EFFECTS_SIZE);
    common_memset_char(
        particle_effects,
        0,
        sizeof(ParticleEffect) * PARTICLE_EFFECTS_SIZE);
    #endif
    
    gameloop_init();
    #if TERMINAL_ACTIVE
    terminal_init(platform_enter_fullscreen);
    #endif
    #if SCHEDULED_ANIMS_ACTIVE
    scheduled_animations_init();
    #endif
    T1_texture_array_init();
    
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
        
        if (!font_metrics_file.good) {
            common_strcpy_capped(
                error_message,
                256, "fontmetrics.dat was corrupted\n");
            *success = false;
            return;
        }
        
        text_init(
                malloc_from_unmanaged,
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
    
    renderer_init();
    
    client_logic_init();
    
    gpu_shared_data_collection = malloc_from_unmanaged(
        sizeof(GPUSharedDataCollection));
    common_memset_char(
        gpu_shared_data_collection,
        0,
        sizeof(GPUSharedDataCollection));
    
    // init the buffers that contain our vertices to send to the GPU
    gpu_shared_data_collection->vertices_allocation_size =
        pad_to_page_size(sizeof(GPUVertex) * MAX_VERTICES_PER_BUFFER);
    
    gpu_shared_data_collection->polygons_allocation_size =
        pad_to_page_size(sizeof(GPUzSprite) * MAX_ZSPRITES_PER_BUFFER);
    
    gpu_shared_data_collection->lights_allocation_size =
        pad_to_page_size(sizeof(GPULight) * MAX_LIGHTS_PER_BUFFER);
    
    gpu_shared_data_collection->camera_allocation_size =
        pad_to_page_size(sizeof(GPUCamera));
    
    gpu_shared_data_collection->locked_vertices_allocation_size =
        pad_to_page_size(
            sizeof(GPULockedVertex) * ALL_LOCKED_VERTICES_SIZE);
    
    gpu_shared_data_collection->locked_materials_allocation_size =
        pad_to_page_size(
            sizeof(GPULockedMaterial) * ALL_LOCKED_MATERIALS_SIZE);
    
    gpu_shared_data_collection->projection_constants_allocation_size =
        pad_to_page_size(sizeof(GPUProjectionConstants));
    
    gpu_shared_data_collection->point_vertices_allocation_size =
        pad_to_page_size(sizeof(GPURawVertex) * MAX_POINT_VERTICES);
    
    gpu_shared_data_collection->line_vertices_allocation_size =
        pad_to_page_size(sizeof(GPURawVertex) * MAX_LINE_VERTICES);
    
    gpu_shared_data_collection->postprocessing_constants_allocation_size =
        pad_to_page_size(sizeof(GPUVertex) * MAX_VERTICES_PER_BUFFER);
    
    for (
        uint32_t cur_frame_i = 0;
        cur_frame_i < MAX_RENDERING_FRAME_BUFFERS;
        cur_frame_i++)
    {
        gpu_shared_data_collection->triple_buffers[cur_frame_i].vertices =
            (GPUVertex *)malloc_from_unmanaged_aligned(
                gpu_shared_data_collection->vertices_allocation_size,
                page_size);
        
        gpu_shared_data_collection->triple_buffers[cur_frame_i].
            polygon_collection =
                (GPUSpriteCollection *)malloc_from_unmanaged_aligned(
                    gpu_shared_data_collection->polygons_allocation_size,
                    page_size);
        
        assert(gpu_shared_data_collection->lights_allocation_size > 0);
        gpu_shared_data_collection->triple_buffers[cur_frame_i].lights =
            (GPULight *)malloc_from_unmanaged_aligned(
                gpu_shared_data_collection->lights_allocation_size,
                page_size);
        assert(gpu_shared_data_collection->triple_buffers[cur_frame_i].
            lights != NULL);
        
        gpu_shared_data_collection->triple_buffers[cur_frame_i].camera =
            (GPUCamera *)malloc_from_unmanaged_aligned(
                gpu_shared_data_collection->camera_allocation_size,
                page_size);
        
        #if RAW_SHADER_ACTIVE
        gpu_shared_data_collection->triple_buffers[cur_frame_i].point_vertices =
            (GPURawVertex *)malloc_from_unmanaged_aligned(
                gpu_shared_data_collection->point_vertices_allocation_size,
                page_size);
        
        gpu_shared_data_collection->triple_buffers[cur_frame_i].line_vertices =
            (GPURawVertex *)malloc_from_unmanaged_aligned(
                gpu_shared_data_collection->line_vertices_allocation_size,
                page_size);
        #endif
        
        gpu_shared_data_collection->triple_buffers[cur_frame_i].
            postprocessing_constants =
                (GPUPostProcessingConstants *)malloc_from_unmanaged_aligned(
                    gpu_shared_data_collection->
                        postprocessing_constants_allocation_size,
                    page_size);
        
        common_memset_float(
            gpu_shared_data_collection->triple_buffers[cur_frame_i].camera,
            0.0f,
            sizeof(GPUCamera));
    }
    
    gpu_shared_data_collection->locked_vertices =
        (GPULockedVertex *)malloc_from_unmanaged_aligned(
            gpu_shared_data_collection->locked_vertices_allocation_size,
            page_size);
    
    gpu_shared_data_collection->locked_materials =
        (GPULockedMaterial *)malloc_from_unmanaged_aligned(
            gpu_shared_data_collection->locked_materials_allocation_size,
            page_size);
    
    gpu_shared_data_collection->locked_pjc =
        (GPUProjectionConstants *)malloc_from_unmanaged_aligned(
            gpu_shared_data_collection->projection_constants_allocation_size,
            page_size);
}

#if TEXTURES_ACTIVE
static void asset_loading_thread(int32_t asset_thread_id) {
    if (asset_thread_id > 0) {
        init_PNG_decoder(
            malloc,
            free,
            common_memset_char,
            common_memcpy,
            DPNG_WORKING_MEMORY_SIZE,
            (uint32_t)asset_thread_id);    
    }
    
    T1_texture_files_decode_all_preregistered(
        (uint32_t)asset_thread_id,
        ias->image_decoding_threads);
    
    if (asset_thread_id > 0) {
        deinit_PNG_decoder((uint32_t)asset_thread_id);
    }
    
    ias->thread_finished[asset_thread_id] = 1;
}
#endif

void init_application_after_gpu_init(int32_t throwaway_threadarg) {
    (void)throwaway_threadarg;
    
    T1_texture_array_load_font_images();
    
    uint32_t perlin_good = 0;
    T1_texture_files_preregister_dds_resource(
        "perlin_noise.dds",
        &perlin_good);
    log_assert(perlin_good);
    
    T1_texture_array_get_filename_location(
        "perlin_noise.dds",
        &engine_globals->postprocessing_constants.perlin_texturearray_i,
        &engine_globals->postprocessing_constants.perlin_texture_i);
    
    log_assert(engine_globals->postprocessing_constants.perlin_texturearray_i >= 1);
    log_assert(engine_globals->postprocessing_constants.perlin_texture_i == 0);
    
    platform_gpu_update_viewport(); // kicks off loading screen
        
    bool32_t success = false;
    char errmsg[256];
    
    if (application_running) {
        client_logic_early_startup(&success, errmsg);
        
        uint32_t core_count = platform_get_cpu_logical_core_count();
        log_assert(core_count > 0);
        ias->image_decoding_threads = core_count > 6 ? 6 : core_count;
        
        common_memset_char(
            ias->thread_finished,
            0,
            sizeof(uint32_t) * IMAGE_DECODING_THREADS_MAX);
        ias->thread_finished[0] = true;
        
        log_assert(engine_globals->startup_bytes_to_load == 0);
        log_assert(engine_globals->startup_bytes_loaded == 0);
        
        #if TEXTURES_ACTIVE
        loading_textures = true;
        for (int32_t i = 1; i < (int32_t)ias->image_decoding_threads; i++) {
            platform_start_thread(
                /* void (*function_to_run)(int32_t): */
                    asset_loading_thread,
                /* int32_t argument: */
                    i);
        }
        #endif
    } else {
        return;
    }
    
    /*
    If this was a single-threaded sequence, the next logical step
    would be to run:
        > T1_texture_array_push_all_predecoded();
    
    We'll do a bunch of other work first, because that gives us something
    to do while we wait the other threads to finish.
    */
        
    #define MIN_VERTICES_FOR_SHATTER_EFFECT 400
    for (uint32_t i = 0; i < all_mesh_summaries_size; i++) {
        if (all_mesh_summaries[i].shattered_vertices_head_i < 0) {
            if (
                all_mesh_summaries[i].shattered_vertices_size <
                    MIN_VERTICES_FOR_SHATTER_EFFECT)
            {
                T1_objmodel_create_shattered_version_of_mesh(
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
    
    common_memcpy(
        /* void * dst: */
            gpu_shared_data_collection->locked_vertices,
        /* const void * src: */
            all_mesh_vertices->gpu_data,
        /* size_t n: */
            sizeof(GPULockedVertex) * ALL_LOCKED_VERTICES_SIZE);
    platform_gpu_copy_locked_vertices();
    
    common_memcpy(
        /* void * dst: */
            gpu_shared_data_collection->locked_materials,
        /* const void * src: */
            all_mesh_materials->gpu_data,
        /* size_t n: */
            sizeof(GPULockedMaterial) * ALL_LOCKED_MATERIALS_SIZE);
    platform_gpu_copy_locked_materials();
    
    if (engine_globals->fullscreen) {
        platform_enter_fullscreen();
    }
    
    #if TEXTURES_ACTIVE
    asset_loading_thread(0);
    
    // Wait until all worker threads are finished
    while (!ias->all_finished) {
        ias->all_finished = true;
        for (uint32_t i = 1; i < ias->image_decoding_threads; i++) {
            if (!ias->thread_finished[i]) {
                ias->all_finished = false;
            }
        }
    }
    loading_textures = false;
    #endif
    
    uint64_t longest_taken = 0;
    int32_t longest_ta_i = -1;
    for (int32_t i = 0; i < (int32_t)texture_arrays_size; i++) {
        uint64_t taken =
            texture_arrays[i].ended_decoding -
            texture_arrays[i].started_decoding;
        if (taken > longest_taken) {
            longest_taken = taken;
            longest_ta_i = i;
        }
    }
    log_append("Slowest texture array: ");
    log_append_int(longest_ta_i);
    log_append("\nIncludes images: ");
    log_append(texture_arrays[longest_ta_i].images[0].filename);
    for (
        int32_t t_i = 1;
        t_i < (int32_t)texture_arrays[longest_ta_i].images_size;
        t_i++)
    {
        log_append(", ");
        log_append(texture_arrays[longest_ta_i].images[t_i].filename);
    }
    
    T1_texture_array_push_all_predecoded();
    
    platform_layer_start_window_resize(
        platform_get_current_time_microsecs());
    
    if (application_running) {
        client_logic_late_startup();
    }
    
    #if AUDIO_ACTIVE
    start_audio_loop();
    #endif
    
    gameloop_active = true;
}

void shared_shutdown_application(void)
{
    #if ENGINE_SAVEFILE_ACTIVE
    #if AUDIO_ACTIVE
    engine_save_file->music_volume = sound_settings->music_volume;
    engine_save_file->sound_volume = sound_settings->sfx_volume;
    #else
    engine_save_file->music_volume = 0.5f;
    engine_save_file->sound_volume = 0.5f;
    #endif
    #endif
    
    #if ENGINE_SAVEFILE_ACTIVE
    log_assert(engine_save_file != NULL);
    engine_save_file->window_bottom = engine_globals->window_bottom;
    engine_save_file->window_height = engine_globals->window_height;
    engine_save_file->window_left = engine_globals->window_left;
    engine_save_file->window_width = engine_globals->window_width;
    engine_save_file->window_fullscreen = engine_globals->fullscreen;
    
    uint32_t good = false;
    platform_delete_writable("enginestate.dat");
    
    platform_write_file_to_writables(
        /* const char filepath_inside_writables: */
            "enginestate.dat",
        /* const char * output: */
            (char *)engine_save_file,
        /* output_size: */
            sizeof(EngineSaveFile),
        /* uint32_t good: */
            &good);
    #else
    uint32_t good = true;
    #endif
    
    if (!good) {
        return;
    }
}
