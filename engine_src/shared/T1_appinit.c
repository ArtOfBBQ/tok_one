#include "T1_appinit.h"

#include <stdlib.h>

#include "decode_png.h"

#include "T1_std.h"
#include "T1_mem.h"
#include "T1_settings.h"
#include "T1_meta.h"
#include "T1_log.h"
#include "T1_tex.h"
#include "T1_tex_files.h"
#include "T1_profiler.h"
#include "T1_rand.h"
#include "T1_objmodel.h"
#include "T1_audio.h"
#include "T1_zlight.h"
#include "T1_text.h"
#include "T1_ui_widget.h"
#include "T1_io.h"
#include "T1_global.h"
#include "T1_term.h"
#include "T1_texquad.h"
#include "T1_zsprite_anim.h"
#include "T1_render.h"
#include "T1_gameloop.h"
#include "T1_token.h"
#include "T1_mtlparser.h"
#include "T1_objparser.h"
#include "T1_particle.h"
#include "T1_texquad_anim.h"
#include "T1_frame_anim.h"
#include "T1_mesh_summary.h"
#include "T1_material.h"
#include "T1_tex_array.h"
#include "T1_render_view.h"

#include "T1_clientlogic.h"
#include "T1_platform_layer.h"




#define IMAGE_DECODING_THREADS_MAX 10
typedef struct InitApplicationState {
    u32 image_decoding_threads;
    u32 all_finished;
    u32 thread_finished[IMAGE_DECODING_THREADS_MAX];
} InitApplicationState;

static InitApplicationState * ias;

#define DPNG_WORKING_MEMORY_SIZE 35000000

#if T1_ENGINE_SAVEFILE_ACTIVE == T1_ACTIVE
typedef struct EngineSaveFile {
    f32 window_left;
    f32 window_width;
    f32 window_bottom;
    f32 window_height;
    f32 music_volume;
    f32 sound_volume;
    b8 window_fullscreen;
} EngineSaveFile;

static EngineSaveFile * engine_save_file = NULL;
#elif T1_ENGINE_SAVEFILE_ACTIVE == T1_INACTIVE
// Pass
#else
#error "T1_ENGINE_SAVEFILE_ACTIVE not set!"
#endif

#if T1_LOG_ASSERTS_ACTIVE == T1_ACTIVE
typedef struct SimdTestStruct {
    f32 imanf32[16];
} SimdTestStruct;
static void 
test_simd_functions_f32s(void) {
    T1_log_assert(sizeof(T1zLight) % (SIMD_FLOAT_LANES * 4) == 0);
    T1_log_assert(sizeof(T1GPUzSprite)   % (SIMD_FLOAT_LANES * 4) == 0);
    
    T1_log_assert(sizeof(T1GPUTexQuadf32)   % (SIMD_FLOAT_LANES * 4) == 0);
    T1_log_assert(sizeof(T1GPUTexQuads32)   % (SIMD_INT32_LANES * 4) == 0);
    
    T1_log_assert(sizeof(SimdTestStruct) % (SIMD_FLOAT_LANES * 4) == 0);
    SimdTestStruct * structs = T1_mem_malloc_managed(
        sizeof(SimdTestStruct) * 10);
    f32 * sets = T1_mem_malloc_managed(
        sizeof(f32) * 10);
    SimdTestStruct * muls = T1_mem_malloc_managed(
        sizeof(SimdTestStruct) * 10);
    SimdTestStruct * divs = T1_mem_malloc_managed(
        sizeof(SimdTestStruct) * 10);
    SimdTestStruct * adds = T1_mem_malloc_managed(
        sizeof(SimdTestStruct) * 10);
    SimdTestStruct * maxs = T1_mem_malloc_managed(
        sizeof(SimdTestStruct) * 10);
    SimdTestStruct * check_values = T1_mem_malloc_managed(
        sizeof(SimdTestStruct) * 10);
    SimdTestStruct * equals = T1_mem_malloc_managed(
        sizeof(SimdTestStruct) * 10);
    
    T1_std_memset(structs, 0, sizeof(SimdTestStruct)*10);
    T1_std_memset(check_values, 0, sizeof(SimdTestStruct)*10);
    T1_std_memset(sets, 0, sizeof(f32));
    T1_std_memset(adds, 0, sizeof(SimdTestStruct)*10);
    T1_std_memset(maxs, 0, sizeof(SimdTestStruct)*10);
    T1_std_memset(muls, 0, sizeof(SimdTestStruct)*10);
    T1_std_memset(divs, 0, sizeof(SimdTestStruct)*10);
    T1_std_memset_f32(equals, 2.0f, sizeof(SimdTestStruct)*10);
    
    for (u32 i = 0; i < 10; i++) {
        sets[i] = (f32)i;
        for (u32 j = 0; j < sizeof(SimdTestStruct) / sizeof(f32); j++) {
            maxs[i].imanf32[j] = (f32)((j % 2) * (i * 2));
            muls[i].imanf32[j] = (f32)(i % 4);
            divs[i].imanf32[j] = (f32)((i % 2) + 1);
            adds[i].imanf32[j] = (f32)((i + 1) % 4);
        }
    }
    
    for (u32 j = 0; j < 10; j++) {
        for (u32 i = 0; i < sizeof(SimdTestStruct) / sizeof(f32); i++) {
            check_values[j].imanf32[i]  = sets[j];
            check_values[j].imanf32[i] *= muls[j].imanf32[i];
            check_values[j].imanf32[i] += adds[j].imanf32[i];
            check_values[j].imanf32[i] /= divs[j].imanf32[i];
            check_values[j].imanf32[i] =
                check_values[j].imanf32[i] > maxs[j].imanf32[i] ?
                    check_values[j].imanf32[i] :
                    maxs[j].imanf32[i];
        }
    }
    
    for (u32 j = 0; j < 10; j++) {
        f32 * structs_at = (f32 *)&structs[j];
        f32 * muls_at    = (f32 *)&muls[j];
        f32 * adds_at    = (f32 *)&adds[j];
        f32 * divs_at    = (f32 *)&divs[j];
        f32 * maxs_at    = (f32 *)&maxs[j];
        // f32 * equals_at  = (f32 *)&equals[j];
        
        // f32 one = 1.0f;
        // SIMD_FLOAT all_ones   = simd_set1_f32(one);
        for (
            u32 i = 0;
            i < sizeof(SimdTestStruct) / sizeof(f32);
            i += SIMD_FLOAT_LANES)
        {
            SIMD_FLOAT cur  = simd_load_f32s(structs_at + i);
            cur = simd_set1_f32(sets[j]);
            SIMD_FLOAT mul    = simd_load_f32s(muls_at + i);
            SIMD_FLOAT add    = simd_load_f32s(adds_at + i);
            SIMD_FLOAT div    = simd_load_f32s(divs_at + i);
            SIMD_FLOAT max    = simd_load_f32s(maxs_at + i);
            // SIMD_FLOAT eq     = simd_load_f32s(equals_at + i);
            
            
            cur = simd_mul_f32s(cur, mul);
            cur = simd_add_f32s(cur, add);
            cur = simd_div_f32s(cur, div);
            cur = simd_max_f32s(cur, max);
            
            //            SIMD_FLOAT equals_bonuses =
            //                simd_and_f32s(all_ones, simd_cmpeq_f32s(cur, eq));
            //            cur = simd_add_f32s(cur, equals_bonuses);
            
            simd_store_f32s(structs_at + i, cur);
        }
    }
    
    for (u32 i = 0; i < 10; i++) {
        for (u32 j = 0; j < 16; j++) {
            T1_log_assert(
                (structs[i].imanf32[j] - check_values[i].imanf32[j]) <  0.01f &&
                (structs[i].imanf32[j] - check_values[i].imanf32[j]) > -0.01f);
        }
    }
    
    T1_mem_free_managed(check_values);
    T1_mem_free_managed(maxs);
    T1_mem_free_managed(adds);
    T1_mem_free_managed(divs);
    T1_mem_free_managed(muls);
    T1_mem_free_managed(sets);
    T1_mem_free_managed(structs);
}
#elif T1_LOG_ASSERTS_ACTIVE == T1_INACTIVE
#else
#error
#endif

static u32 pad_to_page_size(u32 base_allocation) {
    u32 return_value = base_allocation +
        (T1_mem_page_size - (base_allocation % T1_mem_page_size));
    T1_log_assert(return_value % T1_mem_page_size == 0);
    return return_value;
}

void T1_appinit_before_gpu_init(
    u8 * success,
    char * error_message)
{
    *success = true;
    error_message[0] = '\0';
    
    void * unmanaged_memory_store =
        T1_os_malloc_unaligned_block(
            T1_UNMANAGED_MEM_CAP + 7232);
    
    T1_os_init(&unmanaged_memory_store, 32);
    
    T1_mem_init(
        unmanaged_memory_store,
        T1_platform_init_mutex_and_return_id,
        T1_platform_mutex_lock,
        T1_platform_mutex_unlock);
    
    T1_settings_init(T1_mem_malloc_unmanaged);
    
    T1_meta_init(
        T1_std_memcpy,
        T1_mem_malloc_unmanaged,
        T1_std_memset,
        strcmp,
        T1_std_strlen,
        strtoull,
        /* const u32 ascii_store_cap: */
            30000,
        /* const u16 meta_structs_cap: */
            30,
        /* const u16 meta_fields_cap: */
            500,
        /* const u16 meta_enums_cap: */
            30,
        /* const u16 meta_enum_vals_cap: */
            200);
    
    ias = T1_mem_malloc_unmanaged(sizeof(InitApplicationState));
    T1_std_memset(ias, 0, sizeof(InitApplicationState));
    
    // settings_init(malloc_from_unmanaged);
    
    decode_png_init(
        /* void *(*malloc_funcptr)(u64): */
            T1_mem_malloc_managed_infoless,
        /* free_function: */
            T1_mem_free_managed,
        /* memset_function: */
            T1_std_memset,
        /* memcpy_function: */
            T1_std_memcpy,
        /* dpng_working_memory_size: */
            DPNG_WORKING_MEMORY_SIZE,
        /* const u32 thread_id: */
            0);
    
    #if T1_LOG_ASSERTS_ACTIVE == T1_ACTIVE
    test_simd_functions_f32s();
    #elif T1_LOG_ASSERTS_ACTIVE == T1_INACTIVE
    // Pass
    #else
    #error
    #endif
    
    u8 good = 0;
    T1_token_init(
        T1_std_memset,
        T1_std_strlen,
        T1_mem_malloc_managed_infoless,
        &good);
    T1_log_assert(good);
    
    T1_objparser_init(T1_mem_malloc_managed_infoless, T1_mem_free_managed);
    mtlparser_init(
        T1_std_memset,
        T1_mem_malloc_managed_infoless,
        T1_std_strlcat);
    
    T1_logger_init(
        /* void * arg_malloc_function(u64 size): */
            T1_mem_malloc_unmanaged,
        /* u32 (* arg_create_mutex_function)(void): */
            T1_platform_init_mutex_and_return_id,
        /* void arg_mutex_lock_function(const u32 mutex_id): */
            T1_platform_mutex_lock,
        /* s32 arg_mutex_unlock_function(const u32 mutex_id): */
            T1_platform_mutex_unlock);
    
    #if T1_FRAME_ANIM_ACTIVE == T1_ACTIVE
    T1_frame_anim_init();
    #elif T1_FRAME_ANIM_ACTIVE == T1_INACTIVE
    #else
    #error
    #endif
    
    #if T1_PROFILER_ACTIVE == T1_ACTIVE
    T1_profiler_init(
        T1_os_get_clock_frequency(),
            T1_mem_malloc_unmanaged);
    #elif T1_PROFILER_ACTIVE == T1_INACTIVE
    #else
    #error "T1_PROFILER_ACTIVE not set"
    #endif
    
    #if T1_ENGINE_SAVEFILE_ACTIVE == T1_ACTIVE
    engine_save_file = (EngineSaveFile *)T1_mem_malloc_unmanaged(
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
        engine_save.contents = (char *)T1_mem_malloc_managed(
            engine_save.size_without_terminator + 1);
        T1_platform_read_file(full_writable_pathfile, &engine_save);
        *engine_save_file = *(EngineSaveFile *)engine_save.contents;
    }
    #elif T1_ENGINE_SAVEFILE_ACTIVE == T1_INACTIVE
    // Pass
    #else
    #error "T1_ENGINE_SAVEFILE_ACTIVE not set"
    #endif
    
    T1_global = (T1Globals *)T1_mem_malloc_unmanaged(
        sizeof(T1Globals));
    T1_std_memset(T1_global, 0, sizeof(T1Globals));
    
    #if T1_AUDIO_ACTIVE == T1_ACTIVE
    T1_audio_init(
        /* void *(*arg_malloc_function)(u64): */
            T1_mem_malloc_unmanaged);
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
        T1_mem_free_managed(engine_save.contents);
    }
    #elif T1_ENGINE_SAVEFILE_ACTIVE == T1_INACTIVE
    T1_global->fullscreen = false;
    T1_global->window_wh[0]  = INITIAL_WINDOW_WIDTH;
    T1_global->window_wh[1]  = INITIAL_WINDOW_HEIGHT;
    T1_global->window_left   = INITIAL_WINDOW_LEFT;
    T1_global->window_bottom = INITIAL_WINDOW_BOTTOM;
    
    #if T1_AUDIO_ACTIVE == T1_ACTIVE
    T1_audio_s->music_volume  = 0.5f;
    T1_audio_s->sfx_volume    = 0.5f;
    #elif T1_AUDIO_ACTIVE == T1_INACTIVE
    // Pass
    #else
    #error "T1_AUDIO_ACTIVE undefined!"
    #endif
    
    #else
    #error "T1_ENGINE_SAVEFILE_ACTIVE undefined!"
    #endif // T1_ENGINE_SAVEFILE_ACTIVE
    
    T1_global_init();
    
    T1_ui_widget_init();
    
    T1_texquad_init();
    
    T1_zsprite_init();
    
    T1_material_init(T1_mem_malloc_unmanaged);
    
    T1_objmodel_init();
    T1_zlights = (T1zLight *)T1_mem_malloc_unmanaged(
        sizeof(T1zLight) * T1_ZLIGHTS_CAP);
    T1_std_memset(
        T1_zlights,
        0,
        sizeof(T1zLight) *
            T1_ZLIGHTS_CAP);
    
    T1_particle_init();
    
    T1_gameloop_init();
    #if T1_TERM_ACTIVE == T1_ACTIVE
    T1_term_init(T1_os_enter_fullscreen);
    #elif T1_TERM_ACTIVE == T1_INACTIVE
    // Pass
    #else
    #error "T1_TERM_ACTIVE undefined!"
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
    
    T1_tex_array_init();
    
    // initialize font with fontmetrics.dat
    u64 font_metrics_contents_cap = T1_os_get_resource_size(
        /* filename: */ "fontmetrics.dat");
    char * font_metrics_contents = NULL; 
    u8 font_metrics_good = 0;
    
    if (font_metrics_contents_cap > 0) {
        font_metrics_contents = (char *)T1_mem_malloc_unmanaged(
            font_metrics_contents_cap + 1);
        T1_os_read_resource_file(
            /* const char * filename: */
                "fontmetrics.dat",
            /* char * recip: */
                font_metrics_contents,
            /* const u64 recip_cap: */
                font_metrics_contents_cap,
            /* u8 * good: */
                &font_metrics_good);
        
        if (!font_metrics_good) {
            T1_std_strcpy_cap(
                error_message,
                256, "fontmetrics.dat was corrupted\n");
            *success = false;
            return;
        }
        
        T1_text_init(
                T1_mem_malloc_unmanaged,
            /* raw_fontmetrics_file_contents: */
                font_metrics_contents,
            /* raw_fontmetrics_file_size: */
                font_metrics_contents_cap);
    } else {
        T1_std_internal_strcpy_cap(
            error_message,
            128,
            "Error - missing font.png at startup");
        *success = 0;
        return;
    }
    
    T1_render_view_init();
        
    T1_io_init(T1_mem_malloc_unmanaged);
    
    T1_render_init();
    
    T1_clientlogic_init();
    
    T1_rand_init(
        #if T1_LOG_ASSERTS_ACTIVE == T1_ACTIVE
        T1_os_get_current_time_us() % RANDOM_SEQUENCE_SIZE
        #elif T1_LOG_ASSERTS_ACTIVE == T1_INACTIVE
        0
        #else
        #error
        #endif
        );
    
    T1_cpu_to_gpu_data = T1_mem_malloc_unmanaged(
        sizeof(T1CPUToGPUData));
    T1_log_assert(T1_cpu_to_gpu_data != NULL);
    
    T1CPUToGPUData * sd = T1_cpu_to_gpu_data;
    T1_log_assert(sd != NULL);
    
    T1_std_memset(
        sd,
        0,
        sizeof(T1CPUToGPUData));
    
    // init the buffers that contain our vertices to send to the GPU
    sd->vertices_alloc_size = pad_to_page_size(
        sizeof(T1GPUVertexIndices) *
            MAX_VERTICES_PER_BUFFER);
    T1_log_assert(sd->vertices_alloc_size > 0);
    
    sd->flat_quads_alloc_size =
        pad_to_page_size(sizeof(T1GPUFlatQuad) *
            MAX_FLATQUADS_PER_BUFFER);
    
    sd->flat_texquads_alloc_size =
        pad_to_page_size(sizeof(T1GPUTexQuad) *
            MAX_TEXQUADS_PER_BUFFER);
    
    sd->polygons_alloc_size =
        pad_to_page_size(
            sizeof(T1GPUzSprite) *
                T1_ZSPRITES_CAP);
    
    sd->matrices_alloc_size =
        pad_to_page_size(
            sizeof(T1GPUzSpriteMatrices) *
                T1_ZSPRITES_CAP);
    
    sd->lights_alloc_size =
        pad_to_page_size(sizeof(T1GPULight) *
            T1_ZLIGHTS_CAP);
    
    sd->render_views_alloc_size =
        pad_to_page_size(
            sizeof(T1GPURenderView) *
                T1_RENDER_VIEW_CAP);
    
    sd->locked_vertices_alloc_size =
        pad_to_page_size(
            sizeof(T1GPULockedVertex) *
                T1_LOCKED_VERTEX_CAP);
    
    sd->const_matsf32_alloc_size =
        pad_to_page_size(
            sizeof(T1GPUConstMatf32) *
                T1_ALL_LOCKED_MATERIALS_SIZE);
    
    sd->const_matss32_alloc_size =
        pad_to_page_size(
            sizeof(T1GPUConstMats32) *
                T1_ALL_LOCKED_MATERIALS_SIZE);
    
    sd->postprocessing_constants_alloc_size =
        pad_to_page_size(
            sizeof(T1GPUVertexIndices) *
                MAX_VERTICES_PER_BUFFER);
    
    for (
        u32 cur_frame_i = 0;
        cur_frame_i < T1_FRAMES_CAP;
        cur_frame_i++)
    {
        T1GPUFrame * f =
            &sd->triple_buffers[cur_frame_i];
        
        f->verts = (T1GPUVertexIndices *)
            T1_mem_malloc_unmanaged_aligned(
                sd->vertices_alloc_size,
                T1_mem_page_size);
        
        f->flat_bb_quads = (T1GPUFlatQuad *)
            T1_mem_malloc_unmanaged_aligned(
                sd->flat_quads_alloc_size,
                T1_mem_page_size);
        
        f->flat_tex_quads = (T1GPUTexQuad *)
            T1_mem_malloc_unmanaged_aligned(
                sd->flat_texquads_alloc_size,
                T1_mem_page_size);
        
        f->zsprite_list = (T1GPUzSpriteList *)
            T1_mem_malloc_unmanaged_aligned(
                sd->polygons_alloc_size,
                T1_mem_page_size);
        T1_log_assert(f->zsprite_list != NULL);
        
        T1_log_assert(sd->lights_alloc_size > 0);
        f->lights = (T1GPULight *)
            T1_mem_malloc_unmanaged_aligned(
                sd->lights_alloc_size,
                T1_mem_page_size);
        T1_log_assert(f->lights != NULL);
        
        f->render_views = (T1GPURenderView *)
            T1_mem_malloc_unmanaged_aligned(
                sd->render_views_alloc_size,
                T1_mem_page_size);
        
        T1_std_memset_f32(
            f->render_views,
            0.0f,
            sizeof(T1GPURenderView) *
                T1_RENDER_VIEW_CAP);
        
        f->postproc_consts = (T1GPUPostProcConsts *)
            T1_mem_malloc_unmanaged_aligned(
                sd->postprocessing_constants_alloc_size,
                T1_mem_page_size);
    }
    
    sd->locked_vertices =
        (T1GPULockedVertex *)T1_mem_malloc_unmanaged_aligned(
            sd->locked_vertices_alloc_size,
            T1_mem_page_size);
    
    sd->const_mats_f32 = (T1GPUConstMatf32 *)
        T1_mem_malloc_unmanaged_aligned(
            sd->const_matsf32_alloc_size,
            T1_mem_page_size);
    
    sd->const_mats_s32 = (T1GPUConstMats32 *)
        T1_mem_malloc_unmanaged_aligned(
            sd->const_matss32_alloc_size,
            T1_mem_page_size);
    
    u8 initial_log_dump_succesful = false;
    T1_log_dump(&initial_log_dump_succesful);
    if (!initial_log_dump_succesful) {
        T1_log_dump_and_crash(
            "initial log dump unsuccesful, exiting app");
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
static void
T1_appinit_asset_loading_thread(
    s32 asset_thread_id)
{
    if (asset_thread_id > 0) {
        decode_png_init(
            T1_mem_malloc_managed_infoless,
            T1_mem_free_managed,
            T1_std_memset,
            T1_std_memcpy,
            DPNG_WORKING_MEMORY_SIZE,
            (u32)asset_thread_id);    
    }
    
    T1_tex_files_decode_all_prereg(
        (u32)asset_thread_id,
        ias->image_decoding_threads);
    
    if (asset_thread_id > 0) {
        decode_png_deinit((u32)asset_thread_id);
    }
    
    ias->thread_finished[asset_thread_id] = 1;
}
#elif T1_TEXTURES_ACTIVE == T1_INACTIVE
// Pass
#else
#error "T1_TEXTURES_ACTIVE undefined!"
#endif

void T1_appinit_after_gpu_init_step1(
    u8 * success,
    char * error_message)
{
    *success = 0;
    
    if (!T1_log_app_running) {
        return;
    }
    
    error_message[0] = '\0';
    
    T1_tex_files_load_font_images(
        success,
        error_message);
    
    if (!*success) { return; } else { *success = 0; }
    
    u32 rv_width = T1_settings_get_render_width();
    u32 rv_height = T1_settings_get_render_height();
    
    while (rv_width > 2048 || rv_height > 2048) {
        rv_width  /= 2;
        rv_height /= 2;
    }
    
    T1_tex_array_create_new_render_view(
        rv_width,
        rv_height);
    
    // This needs to happen as early as possible, because we can't show
    // log_dump_and_crash or T1_log_assert() errors before this.
    // It also allows us to draw "loading textures x%".
    T1_os_gpu_update_internal_render_viewport(0);
    T1_os_gpu_update_window_viewport();
    
    // We copy the basic quad vertices immediately, again to show debugging
    // text (see above comment)
    T1_std_memcpy(
        /* void * dst: */
            T1_cpu_to_gpu_data->locked_vertices,
        /* const void * src: */
            T1_mesh_summary_all_vertices->gpu_data,
        /* u64 n: */
            sizeof(T1GPULockedVertex) * T1_LOCKED_VERTEX_CAP);
    T1_os_gpu_copy_locked_vertices();
    
    T1_gameloop_active = true;
    *success = true;
}

void T1_appinit_after_gpu_init_step2(
    s32 throwaway_threadarg)
{
    (void)throwaway_threadarg;
    
    if (!T1_log_app_running) {
        return;
    }
    
    b8 perlin_good = 0;
    T1_tex_files_prereg_dds_res(
        "perlin_noise.dds",
        &perlin_good);
    
    if (!perlin_good) {
        T1_log_dump_and_crash(
            "Missing engine file: "
            "perlin_noise.dds");
        T1_global->postproc_consts.perlin_texturearray_i = 1;
        T1_global->postproc_consts.perlin_texture_i = 0;
    } else {
        T1Tex perlin_tex = T1_tex_array_get_filename_loc(
            "perlin_noise.dds");
        T1_global->postproc_consts.perlin_texturearray_i = 
            T1_tex_to_array_i(perlin_tex);
        T1_global->postproc_consts.perlin_texture_i =
            T1_tex_to_slice_i(perlin_tex);
    }
    
    if (
        T1_global->postproc_consts.perlin_texturearray_i < 1 ||
        T1_global->postproc_consts.perlin_texture_i != 0)
    {
        T1_gameloop_active = true;
        T1_log_dump_and_crash("Failed to read engine file: perlin_noise.dds");
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
    
    u8 success = false;
    char errmsg[256];
    errmsg[0] = '\0';
    
    if (T1_log_app_running) {
        T1_clientlogic_early_startup(&success, errmsg);
        
        if (!success) {
            if (errmsg[0] == '\0') {
                T1_std_strcpy_cap(
                    errmsg,
                    256,
                    "client_logic_early_startup() returned failure "
                    "without an error message");
            }
            T1_log_dump_and_crash(errmsg);
            return;
        }
        
        T1_global->
            clientlogic_early_startup_finished = 1;
        
        u32 core_count = T1_os_get_cpu_logical_core_count();
        T1_log_assert(core_count > 0);
        ias->image_decoding_threads = core_count > 6 ? 6 : core_count;
        
        T1_std_memset(
            ias->thread_finished,
            0,
            sizeof(u32) * IMAGE_DECODING_THREADS_MAX);
        ias->thread_finished[0] = true;
        
        T1_log_assert(T1_global->startup_bytes_to_load == 0);
        T1_log_assert(T1_global->startup_bytes_loaded == 0);
        
        if (!T1_log_app_running) {
            return;
        }
        
        #if T1_TEXTURES_ACTIVE == T1_ACTIVE
        T1_gameloop_loading_texs = true;
        for (
            s32 i = 1;
            i < (s32)ias->
                image_decoding_threads;
            i++)
        {
            T1_os_start_thread(
                /* void (*function_to_run)(s32): */
                    T1_appinit_asset_loading_thread,
                /* s32 argument: */
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
    for (
        u32 i = 0;
        i < T1_mesh_summary_list_size;
        i++)
    {
        if (T1_mesh_summary_list[i].shattered_vertices_head_i < 0) {
            if (
                T1_mesh_summary_list[i].shattered_vertices_size <
                    MIN_VERTICES_FOR_SHATTER_EFFECT)
            {
                // If you land here via debugger:
                // print T1_mesh_summary_list[i].resource_name
                T1_objmodel_create_shattered_version_of_mesh(
                    /* const s32 mesh_id: */
                        T1_mesh_summary_list[i].mesh_id,
                    /* const u32 triangles_mulfiplier: */
                        (MIN_VERTICES_FOR_SHATTER_EFFECT /
                            (u32)T1_mesh_summary_list[i].vertices_size) + 1);
                T1_log_assert(
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
    
    if (!T1_log_app_running) {
        T1_gameloop_active = true;
        return;
    }
    
    T1_std_memcpy(
        /* void * dst: */
            T1_cpu_to_gpu_data->
                locked_vertices,
        /* const void * src: */
            T1_mesh_summary_all_vertices->gpu_data,
        /* u64 n: */
            sizeof(T1GPULockedVertex) * T1_LOCKED_VERTEX_CAP);
    T1_os_gpu_copy_locked_vertices();
    
    T1_std_memcpy(
        /* void * dst: */
            T1_cpu_to_gpu_data->
                const_mats_f32,
        /* const void * src: */
            all_mesh_materials->gpu_f32,
        /* u64 n: */
            sizeof(T1GPUConstMatf32) * T1_ALL_LOCKED_MATERIALS_SIZE);
    T1_std_memcpy(
        /* void * dst: */
            T1_cpu_to_gpu_data->
                const_mats_s32,
        /* const void * src: */
            all_mesh_materials->gpu_s32,
        /* u64 n: */
            sizeof(T1GPUConstMats32) * T1_ALL_LOCKED_MATERIALS_SIZE);
    T1_os_gpu_copy_locked_materials();
    
    if (!T1_log_app_running) {
        return;
    }
    
    #if T1_TEXTURES_ACTIVE == T1_ACTIVE
    T1_appinit_asset_loading_thread(0);
    
    if (!T1_log_app_running) {
        return;
    }
    
    // Wait until all worker threads are finished
    while (!ias->all_finished) {
        ias->all_finished = true;
        for (u32 i = 1; i < ias->image_decoding_threads; i++) {
            if (!ias->thread_finished[i]) {
                ias->all_finished = false;
            }
        }
    }
    T1_gameloop_loading_texs = false;
    #elif T1_TEXTURES_ACTIVE == T1_INACTIVE
    #else
    #error "T1_TEXTURES_ACTIVE undefined!"
    #endif
    
    u64 longest_taken = 0;
    s32 longest_ta_i = -1;
    for (s32 i = 0; i < (s32)T1_tex_arrays_size; i++) {
        u64 taken =
            T1_tex_arrays[i].ended_decoding -
            T1_tex_arrays[i].started_decoding;
        if (taken > longest_taken) {
            longest_taken = taken;
            longest_ta_i = i;
        }
    }
    
    if (longest_ta_i >= 0) {
        T1_log_append("Slowest texture array: ");
        T1_log_append_int(longest_ta_i);
        T1_log_append("\nIncludes images: ");
        T1_log_append(T1_tex_arrays[longest_ta_i].images[0].name);
        for (
            s32 t_i = 1;
            t_i < (s32)T1_tex_arrays[longest_ta_i].images_size;
            t_i++)
        {
            T1_log_append(", ");
            T1_log_append(T1_tex_arrays[longest_ta_i].images[t_i].name);
        }
    }
    
    T1_tex_array_push_all();
    
    #if T1_MIPMAPS_ACTIVE == T1_ACTIVE
    for (
        s32 i = 1;
        i < (s32)T1_texture_arrays_size;
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
    
    if (!T1_log_app_running) {
        return;
    }
    
    T1_platform_layer_start_window_resize(
        T1_os_get_current_time_us());
    
    if (T1_log_app_running) {
        T1_clientlogic_late_startup();
    } else {
        T1_gameloop_active = true;
        return;
    }
    
    if (!T1_log_app_running) {
        return;
    }
    
    #if T1_AUDIO_ACTIVE == T1_ACTIVE
    T1_platform_audio_start_loop();
    #elif T1_AUDIO_ACTIVE == T1_INACTIVE
    // Pass
    #else
    #error "T1_AUDIO_ACTIVE undefined!"
    #endif
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
    T1_log_assert(engine_save_file != NULL);
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
    
    u32 good = false;
    T1_platform_del_writable("enginestate.dat");
    
    T1_platform_write_file_to_writables(
        /* const char filepath_inside_writables: */
            "enginestate.dat",
        /* const char * output: */
            (char *)engine_save_file,
        /* output_size: */
            sizeof(EngineSaveFile),
        /* u32 good: */
            &good);
    #elif T1_ENGINE_SAVEFILE_ACTIVE == T1_INACTIVE
    u32 good = true;
    #else
    #error
    #endif
    
    if (!good) {
        return;
    }
}
