#include "T1_appinit.h"

#define IMAGE_DECODING_THREADS_MAX 10
typedef struct InitApplicationState {
    uint32_t image_decoding_threads;
    uint32_t all_finished;
    uint32_t thread_finished[IMAGE_DECODING_THREADS_MAX];
} InitApplicationState;

static InitApplicationState * ias;

#define DPNG_WORKING_MEMORY_SIZE 35000000

#if T1_ENGINE_SAVEFILE_ACTIVE == T1_ACTIVE
typedef struct EngineSaveFile {
    float window_left;
    float window_width;
    float window_bottom;
    float window_height;
    float music_volume;
    float sound_volume;
    bool8_t window_fullscreen;
} EngineSaveFile;

static EngineSaveFile * engine_save_file = NULL;
#elif T1_ENGINE_SAVEFILE_ACTIVE == T1_INACTIVE
// Pass
#else
#error "T1_ENGINE_SAVEFILE_ACTIVE not set!"
#endif

#if T1_LOGGER_ASSERTS_ACTIVE == T1_ACTIVE
typedef struct SimdTestStruct {
    float imafloat[16];
} SimdTestStruct;
static void test_simd_functions_floats(void) {
    log_assert(sizeof(T1zLight) % (SIMD_FLOAT_LANES * 4) == 0);
    log_assert(sizeof(T1GPUzSprite)   % (SIMD_FLOAT_LANES * 4) == 0);
    
    log_assert(sizeof(SimdTestStruct) % (SIMD_FLOAT_LANES * 4) == 0);
    SimdTestStruct * structs = T1_mem_malloc_from_managed(
        sizeof(SimdTestStruct) * 10);
    float * sets = T1_mem_malloc_from_managed(
        sizeof(float) * 10);
    SimdTestStruct * muls = T1_mem_malloc_from_managed(
        sizeof(SimdTestStruct) * 10);
    SimdTestStruct * divs = T1_mem_malloc_from_managed(
        sizeof(SimdTestStruct) * 10);
    SimdTestStruct * adds = T1_mem_malloc_from_managed(
        sizeof(SimdTestStruct) * 10);
    SimdTestStruct * maxs = T1_mem_malloc_from_managed(
        sizeof(SimdTestStruct) * 10);
    SimdTestStruct * double_checks = T1_mem_malloc_from_managed(
        sizeof(SimdTestStruct) * 10);
    SimdTestStruct * equals = T1_mem_malloc_from_managed(
        sizeof(SimdTestStruct) * 10);
    
    T1_std_memset(structs, 0, sizeof(SimdTestStruct)*10);
    T1_std_memset(double_checks, 0, sizeof(SimdTestStruct)*10);
    T1_std_memset(sets, 0, sizeof(float));
    T1_std_memset(adds, 0, sizeof(SimdTestStruct)*10);
    T1_std_memset(maxs, 0, sizeof(SimdTestStruct)*10);
    T1_std_memset(muls, 0, sizeof(SimdTestStruct)*10);
    T1_std_memset(divs, 0, sizeof(SimdTestStruct)*10);
    T1_std_memset_f32(equals, 2.0f, sizeof(SimdTestStruct)*10);
    
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
    
    T1_mem_free_from_managed(double_checks);
    T1_mem_free_from_managed(maxs);
    T1_mem_free_from_managed(adds);
    T1_mem_free_from_managed(divs);
    T1_mem_free_from_managed(muls);
    T1_mem_free_from_managed(sets);
    T1_mem_free_from_managed(structs);
}
#elif T1_LOGGER_ASSERTS_ACTIVE == T1_INACTIVE
#else
#error "T1_LOGGER_ASSERTS_ACTIVE undefined!"
#endif

static uint32_t pad_to_page_size(uint32_t base_allocation) {
    uint32_t return_value = base_allocation +
        (T1_mem_page_size - (base_allocation % T1_mem_page_size));
    log_assert(return_value % T1_mem_page_size == 0);
    return return_value;
}

void T1_appinit_before_gpu_init(
    bool32_t * success,
    char * error_message)
{
    *success = true;
    error_message[0] = '\0';
    
    void * unmanaged_memory_store = T1_platform_malloc_unaligned_block(
        UNMANAGED_MEMORY_SIZE + 7232);
    
    T1_platform_init(&unmanaged_memory_store, 32);
    
    T1_mem_init(
        unmanaged_memory_store,
        T1_platform_init_mutex_and_return_id,
        T1_platform_mutex_lock,
        T1_platform_mutex_unlock);
    
    T1_meta_init(
        memcpy,
        T1_mem_malloc_from_unmanaged,
        memset,
        strcmp,
        strlen,
        strtoull,
        /* const uint32_t ascii_store_cap: */
            30000,
        /* const uint16_t meta_structs_cap: */
            30,
        /* const uint16_t meta_fields_cap: */
            500,
        /* const uint16_t meta_enums_cap: */
            30,
        /* const uint16_t meta_enum_vals_cap: */
            200);
    
    ias = T1_mem_malloc_from_unmanaged(sizeof(InitApplicationState));
    T1_std_memset(ias, 0, sizeof(InitApplicationState));
    
    // settings_init(malloc_from_unmanaged);
    
    init_PNG_decoder(
        /* void *(*malloc_funcptr)(size_t): */
            T1_mem_malloc_from_managed_infoless,
        /* free_function: */
            T1_mem_free_from_managed,
        /* memset_function: */
            T1_std_memset,
        /* memcpy_function: */
            T1_std_memcpy,
        /* dpng_working_memory_size: */
            DPNG_WORKING_MEMORY_SIZE,
        /* const uint32_t thread_id: */
            0);
    
    #if T1_LOGGER_ASSERTS_ACTIVE == T1_ACTIVE
    test_simd_functions_floats();
    #elif T1_LOGGER_ASSERTS_ACTIVE == T1_INACTIVE
    // Pass
    #else
    #error "T1_LOGGER_ASSERTS_ACTIVE undefined!"
    #endif
    
    uint32_t good = 0;
    T1_token_init(
        T1_std_memset,
        T1_std_strlen,
        T1_mem_malloc_from_managed_infoless,
        &good);
    log_assert(good);
    
    T1_objparser_init(T1_mem_malloc_from_managed_infoless, T1_mem_free_from_managed);
    mtlparser_init(
        T1_std_memset,
        T1_mem_malloc_from_managed_infoless,
        strlcat);
    
    logger_init(
        /* void * arg_malloc_function(size_t size): */
            T1_mem_malloc_from_unmanaged,
        /* uint32_t (* arg_create_mutex_function)(void): */
            T1_platform_init_mutex_and_return_id,
        /* void arg_mutex_lock_function(const uint32_t mutex_id): */
            T1_platform_mutex_lock,
        /* int32_t arg_mutex_unlock_function(const uint32_t mutex_id): */
            T1_platform_mutex_unlock);
    
    #if T1_FRAME_ANIM_ACTIVE == T1_ACTIVE
    T1_frame_anim_init();
    #elif T1_FRAME_ANIM_ACTIVE == T1_INACTIVE
    #else
    #error
    #endif
    
    #if T1_PROFILER_ACTIVE == T1_ACTIVE
    T1_profiler_init(
        T1_platform_get_clock_frequency(),
        T1_mem_malloc_from_unmanaged);
    #elif T1_PROFILER_ACTIVE == T1_INACTIVE
    #else
    #error "T1_PROFILER_ACTIVE not set"
    #endif
    
    #if T1_ENGINE_SAVEFILE_ACTIVE == T1_ACTIVE
    engine_save_file = (EngineSaveFile *)T1_mem_malloc_from_unmanaged(
        sizeof(EngineSaveFile));
    
    char full_writable_pathfile[256];
    T1_platform_writable_filename_to_pathfile(
        "enginestate.dat",
        full_writable_pathfile,
        256);
    T1FileBuffer engine_save;
    engine_save.contents = NULL;
    if (T1_platform_file_exists(full_writable_pathfile)) {
        engine_save.size_without_terminator = T1_platform_get_filesize(
            full_writable_pathfile);
        engine_save.contents = (char *)T1_mem_malloc_from_managed(
            engine_save.size_without_terminator + 1);
        T1_platform_read_file(full_writable_pathfile, &engine_save);
        *engine_save_file = *(EngineSaveFile *)engine_save.contents;
    }
    #elif T1_ENGINE_SAVEFILE_ACTIVE == T1_INACTIVE
    // Pass
    #else
    #error "T1_ENGINE_SAVEFILE_ACTIVE not set"
    #endif
    
    T1_global = (T1Globals *)T1_mem_malloc_from_unmanaged(
        sizeof(T1Globals));
    T1_std_memset(T1_global, 0, sizeof(T1Globals));
    
    #if T1_AUDIO_ACTIVE == T1_ACTIVE
    T1_audio_init(
        /* void *(*arg_malloc_function)(size_t): */
            T1_mem_malloc_from_unmanaged);
    #elif T1_AUDIO_ACTIVE == T1_INACTIVE
    // Pass
    #else
    #error "T1_AUDIO_ACTIVE undefined!"
    #endif
    
    #if T1_ENGINE_SAVEFILE_ACTIVE == T1_ACTIVE
    if (
        engine_save.contents != NULL &&
        engine_save_file->window_height > 20 &&
        engine_save_file->window_height < INITIAL_WINDOW_HEIGHT * 3 &&
        engine_save_file->window_width > 20 &&
        engine_save_file->window_width < INITIAL_WINDOW_WIDTH * 3)
    {
        T1_global->window_height =
            engine_save_file->window_height;
        T1_global->window_width  =
            engine_save_file->window_width;
        T1_global->window_left   = engine_save_file->window_left;
        T1_global->window_bottom = engine_save_file->window_bottom;
        T1_global->fullscreen = engine_save_file->window_fullscreen;
        T1_global->upcoming_fullscreen_request =
            T1_global->fullscreen;
        #if T1_AUDIO_ACTIVE == T1_ACTIVE
        T1_audio_state->music_volume = engine_save_file->music_volume;
        T1_audio_state->sfx_volume = engine_save_file->sound_volume;
        #elif T1_AUDIO_ACTIVE == T1_INACTIVE
        // Pass
        #else
        #error "T1_AUDIO_ACTIVE undefined!"
        #endif // T1_AUDIO_ACTIVE
    } else {
        T1_global->fullscreen = false;
        T1_global->window_height = INITIAL_WINDOW_HEIGHT;
        T1_global->window_width  = INITIAL_WINDOW_WIDTH;
        T1_global->window_left   = INITIAL_WINDOW_LEFT;
        T1_global->window_bottom =
            INITIAL_WINDOW_BOTTOM;
    }
    
    if (engine_save.contents != NULL) {
        T1_mem_free_from_managed(engine_save.contents);
    }
    #elif T1_ENGINE_SAVEFILE_ACTIVE == T1_INACTIVE
    T1_global->fullscreen = false;
    T1_global->window_height = INITIAL_WINDOW_HEIGHT;
    T1_global->window_width  = INITIAL_WINDOW_WIDTH;
    T1_global->window_left   = INITIAL_WINDOW_LEFT;
    T1_global->window_bottom = INITIAL_WINDOW_BOTTOM;
    
    #if T1_AUDIO_ACTIVE == T1_ACTIVE
    T1_audio_state->music_volume  = 0.5f;
    T1_audio_state->sfx_volume    = 0.5f;
    #elif T1_AUDIO_ACTIVE == T1_INACTIVE
    // Pass
    #else
    #error "T1_AUDIO_ACTIVE undefined!"
    #endif
    
    #else
    #error "T1_ENGINE_SAVEFILE_ACTIVE undefined!"
    #endif // T1_ENGINE_SAVEFILE_ACTIVE
    
    T1_global_init();
    
    T1_uielement_init();
    
    T1_flat_texquad_init();
    
    T1_zsprite_init();
    
    T1_material_init(T1_mem_malloc_from_unmanaged);
    
    T1_objmodel_init();
    zlights_to_apply = (T1zLight *)T1_mem_malloc_from_unmanaged(
        sizeof(T1zLight) * MAX_LIGHTS_PER_BUFFER);
    T1_std_memset(
        zlights_to_apply,
        0,
        sizeof(T1zLight) *
            MAX_LIGHTS_PER_BUFFER);
    
    T1_particle_init();
    
    T1_gameloop_init();
    #if T1_TERMINAL_ACTIVE == T1_ACTIVE
    terminal_init(T1_platform_enter_fullscreen);
    #elif T1_TERMINAL_ACTIVE == T1_INACTIVE
    // Pass
    #else
    #error "T1_TERMINAL_ACTIVE undefined!"
    #endif
    
    #if T1_ZSPRITE_ANIM_ACTIVE == T1_ACTIVE
    T1_zsprite_anim_init(
        T1_platform_init_mutex_and_return_id,
        T1_platform_mutex_lock,
        T1_platform_mutex_unlock);
    #elif T1_ZSPRITE_ANIM_ACTIVE == T1_INACTIVE
    // Pass
    #else
    #error "T1_ZSPRITE_ANIM_ACTIVE undefined!"
    #endif
    
    #if T1_TEXQUAD_ANIM_ACTIVE == T1_ACTIVE
    T1_texquad_anim_init(
        T1_platform_init_mutex_and_return_id,
        T1_platform_mutex_lock,
        T1_platform_mutex_unlock);
    #elif T1_TEXQUAD_ANIM_ACTIVE == T1_INACTIVE
    // Pass
    #else
    #error "T1_TEXQUAD_ANIM_ACTIVE undefined!"
    #endif
    
    T1_texture_array_init();
    
    // initialize font with fontmetrics.dat
    T1FileBuffer font_metrics_file;
    font_metrics_file.size_without_terminator = T1_platform_get_resource_size(
        /* filename: */ "fontmetrics.dat");
    
    if (font_metrics_file.size_without_terminator > 0) {
        font_metrics_file.contents = (char *)T1_mem_malloc_from_unmanaged(
            font_metrics_file.size_without_terminator + 1);
        T1_platform_read_resource_file(
            /* const char * filepath: */
                "fontmetrics.dat",
            /* FileBuffer * out_preallocatedbuffer: */
                &font_metrics_file);
        
        if (!font_metrics_file.good) {
            T1_std_strcpy_cap(
                error_message,
                256, "fontmetrics.dat was corrupted\n");
            *success = false;
            return;
        }
        
        text_init(
                T1_mem_malloc_from_unmanaged,
            /* raw_fontmetrics_file_contents: */
                font_metrics_file.contents,
            /* raw_fontmetrics_file_size: */
                font_metrics_file.size_without_terminator);
    } else {
        T1_std_internal_strcpy_cap(
            error_message,
            128,
            "Error - missing font.png at startup");
        *success = 0;
        return;
    }
    
    T1_render_view_init();
        
    T1_io_init(T1_mem_malloc_from_unmanaged);
    
    T1_renderer_init();
    
    T1_clientlogic_init();
    
    gpu_shared_data_collection =
        T1_mem_malloc_from_unmanaged(
            sizeof(T1GPUSharedDataCollection));
    log_assert(gpu_shared_data_collection != NULL);
    
    T1GPUSharedDataCollection * sd = gpu_shared_data_collection;
    log_assert(sd != NULL);
    
    T1_std_memset(
        sd,
        0,
        sizeof(T1GPUSharedDataCollection));
    
    // init the buffers that contain our vertices to send to the GPU
    sd->vertices_alloc_size = pad_to_page_size(
        sizeof(T1GPUVertexIndices) *
            MAX_VERTICES_PER_BUFFER);
    log_assert(sd->vertices_alloc_size > 0);
    
    sd->flat_quads_alloc_size =
        pad_to_page_size(sizeof(T1GPUFlatQuad) *
            MAX_FLATQUADS_PER_BUFFER);
    
    sd->flat_texquads_alloc_size =
        pad_to_page_size(sizeof(T1GPUTexQuad) *
            MAX_TEXQUADS_PER_BUFFER);
    
    sd->polygons_alloc_size =
        pad_to_page_size(
            sizeof(T1GPUzSprite) *
                MAX_ZSPRITES_PER_BUFFER);
    
    sd->matrices_alloc_size =
        pad_to_page_size(
            sizeof(T1GPUzSpriteMatrices) *
                MAX_ZSPRITES_PER_BUFFER);
    
    sd->lights_alloc_size =
        pad_to_page_size(sizeof(T1GPULight) *
            MAX_LIGHTS_PER_BUFFER);
    
    sd->render_views_alloc_size =
        pad_to_page_size(
            sizeof(T1GPURenderView) *
                T1_RENDER_VIEW_CAP);
    
    sd->locked_vertices_alloc_size =
        pad_to_page_size(
            sizeof(T1GPULockedVertex) *
                ALL_LOCKED_VERTICES_SIZE);
    
    sd->const_matsf32_alloc_size =
        pad_to_page_size(
            sizeof(T1GPUConstMatf32) *
                ALL_LOCKED_MATERIALS_SIZE);
    
    sd->const_matsi32_alloc_size =
        pad_to_page_size(
            sizeof(T1GPUConstMati32) *
                ALL_LOCKED_MATERIALS_SIZE);
    
    sd->postprocessing_constants_alloc_size =
        pad_to_page_size(
            sizeof(T1GPUVertexIndices) *
                MAX_VERTICES_PER_BUFFER);
    
    for (
        uint32_t cur_frame_i = 0;
        cur_frame_i < FRAMES_CAP;
        cur_frame_i++)
    {
        T1GPUFrame * f =
            &sd->triple_buffers[cur_frame_i];
        
        f->verts = (T1GPUVertexIndices *)
            T1_mem_malloc_from_unmanaged_aligned(
                sd->vertices_alloc_size,
                T1_mem_page_size);
        
        f->flat_bb_quads = (T1GPUFlatQuad *)
            T1_mem_malloc_from_unmanaged_aligned(
                sd->flat_quads_alloc_size,
                T1_mem_page_size);
        
        f->flat_tex_quads = (T1GPUTexQuad *)
            T1_mem_malloc_from_unmanaged_aligned(
                sd->flat_texquads_alloc_size,
                T1_mem_page_size);
        
        f->zsprite_list = (T1GPUzSpriteList *)
            T1_mem_malloc_from_unmanaged_aligned(
                sd->polygons_alloc_size,
                T1_mem_page_size);
        
        log_assert(sd->lights_alloc_size > 0);
        f->lights = (T1GPULight *)
            T1_mem_malloc_from_unmanaged_aligned(
                sd->lights_alloc_size,
                T1_mem_page_size);
        log_assert(f->lights != NULL);
        
        f->render_views = (T1GPURenderView *)
            T1_mem_malloc_from_unmanaged_aligned(
                sd->render_views_alloc_size,
                T1_mem_page_size);
        
        T1_std_memset_f32(
            f->render_views,
            0.0f,
            sizeof(T1GPURenderView) *
                T1_RENDER_VIEW_CAP);
        
        f->postproc_consts = (T1GPUPostProcConsts *)
            T1_mem_malloc_from_unmanaged_aligned(
                sd->postprocessing_constants_alloc_size,
                T1_mem_page_size);
    }
    
    sd->locked_vertices =
        (T1GPULockedVertex *)T1_mem_malloc_from_unmanaged_aligned(
            sd->locked_vertices_alloc_size,
            T1_mem_page_size);
    
    sd->const_mats_f32 = (T1GPUConstMatf32 *)
        T1_mem_malloc_from_unmanaged_aligned(
            sd->const_matsf32_alloc_size,
            T1_mem_page_size);
    
    sd->const_mats_i32 = (T1GPUConstMati32 *)
        T1_mem_malloc_from_unmanaged_aligned(
            sd->const_matsi32_alloc_size,
            T1_mem_page_size);
    
    bool32_t initial_log_dump_succesful = false;
    log_dump(&initial_log_dump_succesful);
    if (!initial_log_dump_succesful) {
        log_dump_and_crash("initial log dump unsuccesful, exiting app");
        T1_std_internal_strcpy_cap(
            error_message,
            128,
            "Error - couldn't write the log file to "
            "disk at startup");
        *success = 0;
        return;
    }
}

#if T1_TEXTURES_ACTIVE == T1_ACTIVE
static void T1_appinit_asset_loading_thread(int32_t asset_thread_id) {
    if (asset_thread_id > 0) {
        init_PNG_decoder(
            malloc,
            free,
            T1_std_memset,
            T1_std_memcpy,
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
#elif T1_TEXTURES_ACTIVE == T1_INACTIVE
// Pass
#else
#error "T1_TEXTURES_ACTIVE undefined!"
#endif

void T1_appinit_after_gpu_init_step1(
    bool32_t * success,
    char * error_message)
{
    *success = 0;
    
    if (!T1_app_running) {
        return;
    }
    
    error_message[0] = '\0';
    
    T1_texture_files_load_font_images(
        success,
        error_message);
    
    if (!*success) { return; } else { *success = 0; }
    
    uint32_t rv_width = (uint32_t)
        T1_global->window_width;
    uint32_t rv_height = (uint32_t)
        T1_global->window_height;
    
    while (rv_width > 2048 || rv_height > 2048) {
        rv_width  /= 2;
        rv_height /= 2;
    }
    
    T1_texture_array_create_new_render_view(
        rv_width,
        rv_height);
    
    // This needs to happen as early as possible, because we can't show
    // log_dump_and_crash or log_assert() errors before this.
    // It also allows us to draw "loading textures x%".
    T1_platform_gpu_update_internal_render_viewport(0);
    T1_platform_gpu_update_window_viewport();
    
    // We copy the basic quad vertices immediately, again to show debugging
    // text (see above comment)
    T1_std_memcpy(
        /* void * dst: */
            gpu_shared_data_collection->locked_vertices,
        /* const void * src: */
            T1_mesh_summary_all_vertices->gpu_data,
        /* size_t n: */
            sizeof(T1GPULockedVertex) * ALL_LOCKED_VERTICES_SIZE);
    T1_platform_gpu_copy_locked_vertices();
    
    T1_gameloop_active = true;
    *success = true;
}

void T1_appinit_after_gpu_init_step2(
    int32_t throwaway_threadarg)
{
    (void)throwaway_threadarg;
    
    if (!T1_app_running) {
        return;
    }
    
    uint32_t perlin_good = 0;
    T1_texture_files_preregister_dds_resource(
        "perlin_noise.dds",
        &perlin_good);
    
    if (!perlin_good) {
        log_dump_and_crash(
            "Missing engine file: "
            "perlin_noise.dds");
        T1_global->postproc_consts.perlin_texturearray_i = 1;
        T1_global->postproc_consts.perlin_texture_i = 0;
    } else {
        T1Tex perlin_tex = T1_texture_array_get_filename_location(
            "perlin_noise.dds");
        T1_global->postproc_consts.perlin_texturearray_i =
            perlin_tex.array_i;
        T1_global->postproc_consts.perlin_texture_i =
            perlin_tex.slice_i;
    }
    
    if (
        T1_global->postproc_consts.perlin_texturearray_i < 1 ||
        T1_global->postproc_consts.perlin_texture_i != 0)
    {
        T1_gameloop_active = true;
        log_dump_and_crash("Failed to read engine file: perlin_noise.dds");
        return;
    }
    
    #if T1_SHADOWS_ACTIVE == T1_ACTIVE
    T1_global->postproc_consts.in_shadow_multipliers[0] = 0.5f;
    T1_global->postproc_consts.in_shadow_multipliers[1] = 0.5f;
    T1_global->postproc_consts.in_shadow_multipliers[2] = 0.5f;
    #elif T1_SHADOWS_ACTIVE == T1_INACTIVE
    // Pass
    #else
    #error "T1_SHADOWS_ACTIVE undefined"
    #endif
    
    bool32_t success = false;
    char errmsg[256];
    errmsg[0] = '\0';
    
    if (T1_app_running) {
        T1_clientlogic_early_startup(&success, errmsg);
        
        if (!success) {
            if (errmsg[0] == '\0') {
                T1_std_strcpy_cap(
                    errmsg,
                    256,
                    "client_logic_early_startup() returned failure without "
                    "an error message");
            }
            log_dump_and_crash(errmsg);
            return;
        }
        
        T1_global->
            clientlogic_early_startup_finished = 1;
        
        uint32_t core_count = T1_platform_get_cpu_logical_core_count();
        log_assert(core_count > 0);
        ias->image_decoding_threads = core_count > 6 ? 6 : core_count;
        
        T1_std_memset(
            ias->thread_finished,
            0,
            sizeof(uint32_t) * IMAGE_DECODING_THREADS_MAX);
        ias->thread_finished[0] = true;
        
        log_assert(T1_global->startup_bytes_to_load == 0);
        log_assert(T1_global->startup_bytes_loaded == 0);
        
        if (!T1_app_running) {
            return;
        }
        
        #if T1_TEXTURES_ACTIVE == T1_ACTIVE
        T1_loading_textures = true;
        for (
            int32_t i = 1;
            i < (int32_t)ias->
                image_decoding_threads;
            i++)
        {
            T1_platform_start_thread(
                /* void (*function_to_run)(int32_t): */
                    T1_appinit_asset_loading_thread,
                /* int32_t argument: */
                    i);
        }
        #elif T1_TEXTURES_ACTIVE == T1_INACTIVE
        // Pass
        #else
        #error "T1_TEXTURES_ACTIVE undefined"
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
    #if T1_PARTICLES_ACTIVE == T1_ACTIVE
    #define MIN_VERTICES_FOR_SHATTER_EFFECT 250
    for (uint32_t i = 0; i < T1_mesh_summary_list_size; i++) {
        if (T1_mesh_summary_list[i].shattered_vertices_head_i < 0) {
            if (
                T1_mesh_summary_list[i].shattered_vertices_size <
                    MIN_VERTICES_FOR_SHATTER_EFFECT)
            {
                T1_objmodel_create_shattered_version_of_mesh(
                    /* const int32_t mesh_id: */
                        T1_mesh_summary_list[i].mesh_id,
                    /* const uint32_t triangles_mulfiplier: */
                        (MIN_VERTICES_FOR_SHATTER_EFFECT /
                            (uint32_t)T1_mesh_summary_list[i].vertices_size) + 1);
                log_assert(
                    T1_mesh_summary_list[i].shattered_vertices_head_i >= 0);
            } else {
                T1_mesh_summary_list[i].shattered_vertices_head_i =
                    T1_mesh_summary_list[i].vertices_head_i;
                T1_mesh_summary_list[i].shattered_vertices_size =
                    T1_mesh_summary_list[i].vertices_size;
            }
        }
    }
    #elif T1_PARTICLES_ACTIVE == T1_INACTIVE
    // Pass
    #else
    #error "T1_PARTICLES_ACTIVE undefined!"
    #endif
    
    if (!T1_app_running) {
        T1_gameloop_active = true;
        return;
    }
    
    T1_std_memcpy(
        /* void * dst: */
            gpu_shared_data_collection->
                locked_vertices,
        /* const void * src: */
            T1_mesh_summary_all_vertices->gpu_data,
        /* size_t n: */
            sizeof(T1GPULockedVertex) * ALL_LOCKED_VERTICES_SIZE);
    T1_platform_gpu_copy_locked_vertices();
    
    T1_std_memcpy(
        /* void * dst: */
            gpu_shared_data_collection->
                const_mats_f32,
        /* const void * src: */
            all_mesh_materials->gpu_f32,
        /* size_t n: */
            sizeof(T1GPUConstMatf32) * ALL_LOCKED_MATERIALS_SIZE);
    T1_std_memcpy(
        /* void * dst: */
            gpu_shared_data_collection->
                const_mats_i32,
        /* const void * src: */
            all_mesh_materials->gpu_i32,
        /* size_t n: */
            sizeof(T1GPUConstMati32) * ALL_LOCKED_MATERIALS_SIZE);
    T1_platform_gpu_copy_locked_materials();
    
    if (!T1_app_running) {
        return;
    }
    
    #if T1_TEXTURES_ACTIVE == T1_ACTIVE
    T1_appinit_asset_loading_thread(0);
    
    if (!T1_app_running) {
        return;
    }
    
    // Wait until all worker threads are finished
    while (!ias->all_finished) {
        ias->all_finished = true;
        for (uint32_t i = 1; i < ias->image_decoding_threads; i++) {
            if (!ias->thread_finished[i]) {
                ias->all_finished = false;
            }
        }
    }
    T1_loading_textures = false;
    #elif T1_TEXTURES_ACTIVE == T1_INACTIVE
    #else
    #error "T1_TEXTURES_ACTIVE undefined!"
    #endif
    
    uint64_t longest_taken = 0;
    int32_t longest_ta_i = -1;
    for (int32_t i = 0; i < (int32_t)T1_texture_arrays_size; i++) {
        uint64_t taken =
            T1_texture_arrays[i].ended_decoding -
            T1_texture_arrays[i].started_decoding;
        if (taken > longest_taken) {
            longest_taken = taken;
            longest_ta_i = i;
        }
    }
    
    if (longest_ta_i >= 0) {
        log_append("Slowest texture array: ");
        log_append_int(longest_ta_i);
        log_append("\nIncludes images: ");
        log_append(T1_texture_arrays[longest_ta_i].images[0].name);
        for (
            int32_t t_i = 1;
            t_i < (int32_t)T1_texture_arrays[longest_ta_i].images_size;
            t_i++)
        {
            log_append(", ");
            log_append(T1_texture_arrays[longest_ta_i].images[t_i].name);
        }
    }
    
    T1_texture_array_push_all_predecoded();
    
    #if T1_MIPMAPS_ACTIVE == T1_ACTIVE
    for (
        int32_t i = 1;
        i < (int32_t)T1_texture_arrays_size;
        i++)
    {
        if (!T1_texture_arrays[i].bc1_compressed) {
            T1_platform_gpu_generate_mipmaps_for_texture_array(i);
        }
    }
    #elif T1_MIPMAPS_ACTIVE == T1_INACTIVE
    #else
    #error "T1_MIPMAPS_ACTIVE undefined!"
    #endif
    
    if (!T1_app_running) {
        return;
    }
    
    T1_platform_layer_start_window_resize(
        T1_platform_get_current_time_us());
    
    if (T1_app_running) {
        T1_clientlogic_late_startup();
    } else {
        T1_gameloop_active = true;
        return;
    }
    
    if (!T1_app_running) {
        return;
    }
    
    #if T1_AUDIO_ACTIVE == T1_ACTIVE
    T1_platform_audio_start_loop();
    #elif T1_AUDIO_ACTIVE == T1_INACTIVE
    // Pass
    #else
    #error "T1_AUDIO_ACTIVE undefined!"
    #endif
    
    T1_token_deinit(T1_mem_free_from_managed);
}

void T1_appinit_shutdown(void)
{
    #if T1_ENGINE_SAVEFILE_ACTIVE == T1_ACTIVE
    
    #if T1_AUDIO_ACTIVE == T1_ACTIVE
    engine_save_file->music_volume = T1_audio_state->music_volume;
    engine_save_file->sound_volume = T1_audio_state->sfx_volume;
    #elif T1_AUDIO_ACTIVE == T1_INACTIVE
    #else
    #error "T1_AUDIO_ACTIVE undefined!"
    #endif
    
    #elif T1_ENGINE_SAVEFILE_ACTIVE == T1_INACTIVE
    #else
    #error "T1_ENGINE_SAVEFILE_ACTIVE undefined!"
    #endif
    
    #if T1_ENGINE_SAVEFILE_ACTIVE == T1_ACTIVE
    log_assert(engine_save_file != NULL);
    engine_save_file->window_bottom =
        T1_global->window_bottom;
    engine_save_file->window_height =
        T1_global->window_height;
    engine_save_file->window_left =
        T1_global->window_left;
    engine_save_file->window_width =
        T1_global->window_width;
    engine_save_file->window_fullscreen =
        T1_global->fullscreen;
    
    uint32_t good = false;
    T1_platform_delete_writable("enginestate.dat");
    
    T1_platform_write_file_to_writables(
        /* const char filepath_inside_writables: */
            "enginestate.dat",
        /* const char * output: */
            (char *)engine_save_file,
        /* output_size: */
            sizeof(EngineSaveFile),
        /* uint32_t good: */
            &good);
    #elif T1_ENGINE_SAVEFILE_ACTIVE == T1_INACTIVE
    uint32_t good = true;
    #else
    #error
    #endif
    
    if (!good) {
        return;
    }
}
