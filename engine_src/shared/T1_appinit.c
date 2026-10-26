#include "T1_appinit.h"
#include "T1_pipeline.h"
#include "T1_pass.h"
#include "T1_buf.h"


#include "T1_std.h"
#include "T1_mem.h"
#include "T1_objc.h"
#include "T1_embedded_data.h"
#include "T1_assert.h"
#include "T1_audio.h"
#include "T1_io.h"
#include "T1_global.h"
#include "T1_gameloop.h"
#include "T1_tex_array.h"
#include "T1_fatal_error.h"

#include "T1_os.h"

#if T1_ASSERTS_ACTIVE == T1_ACTIVE
typedef struct {
    f32 imanf32[16];
} T1SimdTestStruct;
 
static void test_simd_struct(
    char ** const fatal_error,
    uint32_t struct_size,
    char * struct_size_str)
{
    s32 extra_padding = (struct_size %
        (SIMD_FLOAT_LANES * 4));
    if (extra_padding != 0) {
        T1_fatal_error_new(fatal_error, struct_size_str);
        T1_std_strcat_cap(*fatal_error, 512, " needs ");
        T1_std_strcat_s32_cap(*fatal_error, 512, extra_padding);
        T1_std_strcat_cap(*fatal_error, 512, " bytes of padding");
        return; 
    }
}
#define test_simd_struct_macro(fatal_error, struct) test_simd_struct(fatal_error, sizeof(struct), #struct)

static void test_simd_functions_f32s(char ** const fatal_error) {
    if (*fatal_error != 0) { return; }
    
    test_simd_struct_macro(fatal_error, T1SimdTestStruct);
    
    T1SimdTestStruct * structs = T1_mem_malloc_managed(
        sizeof(T1SimdTestStruct) * 10);
    f32 * sets = T1_mem_malloc_managed(
        sizeof(f32) * 10);
    T1SimdTestStruct * muls = T1_mem_malloc_managed(
        sizeof(T1SimdTestStruct) * 10);
    T1SimdTestStruct * divs = T1_mem_malloc_managed(
        sizeof(T1SimdTestStruct) * 10);
    T1SimdTestStruct * adds = T1_mem_malloc_managed(
        sizeof(T1SimdTestStruct) * 10);
    T1SimdTestStruct * maxs = T1_mem_malloc_managed(
        sizeof(T1SimdTestStruct) * 10);
    T1SimdTestStruct * check_values = T1_mem_malloc_managed(
        sizeof(T1SimdTestStruct) * 10);
    T1SimdTestStruct * equals = T1_mem_malloc_managed(
        sizeof(T1SimdTestStruct) * 10);
    
    T1_std_memset(structs, 0, sizeof(T1SimdTestStruct)*10);
    T1_std_memset(check_values, 0, sizeof(T1SimdTestStruct)*10);
    T1_std_memset(sets, 0, sizeof(f32));
    T1_std_memset(adds, 0, sizeof(T1SimdTestStruct)*10);
    T1_std_memset(maxs, 0, sizeof(T1SimdTestStruct)*10);
    T1_std_memset(muls, 0, sizeof(T1SimdTestStruct)*10);
    T1_std_memset(divs, 0, sizeof(T1SimdTestStruct)*10);
    T1_std_memset_f32(equals, 2.0f, sizeof(T1SimdTestStruct)*10);
    
    for (u32 i = 0; i < 10; i++) {
        sets[i] = (f32)i;
        for (u32 j = 0; j < sizeof(T1SimdTestStruct) / sizeof(f32); j++) {
            maxs[i].imanf32[j] = (f32)((j % 2) * (i * 2));
            muls[i].imanf32[j] = (f32)(i % 4);
            divs[i].imanf32[j] = (f32)((i % 2) + 1);
            adds[i].imanf32[j] = (f32)((i + 1) % 4);
        }
    }
    
    for (u32 j = 0; j < 10; j++) {
        for (u32 i = 0; i < sizeof(T1SimdTestStruct) / sizeof(f32); i++) {
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
            i < sizeof(T1SimdTestStruct) / sizeof(f32);
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
            T1_assert(
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
#elif T1_ASSERTS_ACTIVE == T1_INACTIVE
#else
#error
#endif

void T1_init(
    void * (* alloc_forever)(size_t size),
    void * (* alloc_temp)(size_t size),
    void   (* free_temp)(void * ptr),
    char ** const fatal_error)
{
    if (*fatal_error != 0) { return; }
    
    if (alloc_forever == NULL || alloc_temp == NULL || free_temp == NULL) {
        *fatal_error = "T1_init() needs all 3 memory functions";
        return;
    }
    
    T1_gameloop_active = 0;
    
    T1_mem_init(
        alloc_forever,
        alloc_temp,
        free_temp,
        T1_os_get_page_size());
    
    T1_os_init();
    
    T1_mem_init_mutex(
        T1_os_init_mutex_and_return_id,
        T1_os_mutex_lock,
        T1_os_mutex_unlock);

    T1_objc_init(
        T1_mem_malloc_unmanaged,
        fatal_error);
    if (*fatal_error != 0) { return; }

    if (
        !T1_objc_open_framework(
            "/System/Library/Frameworks/AppKit.framework/AppKit") ||
        // CAMetalLayer
        !T1_objc_open_framework(
            "/System/Library/Frameworks/QuartzCore.framework/QuartzCore") ||
        !T1_objc_open_framework(
            "/System/Library/Frameworks/Metal.framework/Metal") ||
        !T1_objc_open_framework(
            "/System/Library/Frameworks/GameController.framework/GameController"))
    {
        *fatal_error = "Failed to load a system framework";
        return;
    }

    void * pool = T1_objc_autorelease_pool_push();
    
    
    #if T1_ASSERTS_ACTIVE == T1_ACTIVE
    test_simd_functions_f32s(fatal_error);
    #elif T1_ASSERTS_ACTIVE == T1_INACTIVE
    #else
    #error
    #endif
    
    
    T1_global = (T1Globals *)T1_mem_malloc_unmanaged(
        sizeof(T1Globals));
    if (!T1_global) { *fatal_error = "Malloc fail"; return; }
    T1_std_memset(T1_global, 0, sizeof(T1Globals));
    
    #if T1_AUDIO_ACTIVE == T1_ACTIVE
    T1_audio_init(
        /* void *(*arg_malloc_function)(u64): */
            T1_mem_malloc_unmanaged);
    
    u8 audio_good;
    T1_os_audio_init(&audio_good);
    
    if (audio_good) {
        T1_os_audio_start_loop();
    }
    #elif T1_AUDIO_ACTIVE == T1_INACTIVE
    // Pass
    #else
    #error "T1_AUDIO_ACTIVE undefined!"
    #endif
    
    #ifdef __APPLE__
    T1_apple_os_init();
    #endif
    
    #ifdef TARGET_OS_OSX
    T1_os_macos_init();
    #endif
    
    #if T1_ENGINE_SAVEFILE_ACTIVE == T1_ACTIVE
    engine_save_file = (T1EngineSaveFile *)T1_mem_malloc_unmanaged(
        sizeof(T1EngineSaveFile));
    if (!engine_save_file) {
        *fatal_error = "Malloc fail";
        return;
    }
    T1_std_memset(engine_save_file, 0, sizeof(T1EngineSaveFile));
    
    char full_writable_pathfile[512];
    T1_std_memset(full_writable_pathfile, 0, 512);
    T1_os_writable_filename_to_pathfile(
        "enginestate.dat",
        full_writable_pathfile,
        512);
    
    if (full_writable_pathfile[0] == '\0') {
        *fatal_error = "Failed to find enginestate.dat";
        return;
    }
    
    c8 * engine_save_contents = NULL;
    u32  engine_save_size = 0;
    u64  engine_save_cap_noterm = 0;
    if (T1_os_file_exists(full_writable_pathfile)) {
        engine_save_cap_noterm = T1_os_get_filesize(
            full_writable_pathfile);
        engine_save_contents = (char *)T1_mem_malloc_managed(
            engine_save_cap_noterm + 1);
        T1_std_memset(engine_save_contents, 0, engine_save_cap_noterm + 1); 
        T1_os_read_file(
            full_writable_pathfile,
            engine_save_contents,
            &engine_save_size,
            engine_save_cap_noterm,
            fatal_error);
        if (*fatal_error != 0) { return; }
        *engine_save_file = *(T1EngineSaveFile *)engine_save_contents;
    }
    #elif T1_ENGINE_SAVEFILE_ACTIVE == T1_INACTIVE
    // Pass
    #else
    #error "T1_ENGINE_SAVEFILE_ACTIVE not set"
    #endif
    
    #if T1_ENGINE_SAVEFILE_ACTIVE == T1_ACTIVE
    if (
        engine_save_file->window_height > 20 &&
        engine_save_file->window_height < T1_INITIAL_WINDOW_HEIGHT * 3 &&
        engine_save_file->window_width > 20 &&
        engine_save_file->window_width < T1_INITIAL_WINDOW_WIDTH * 3)
    {
        T1_global_update_window_size(
            engine_save_file->window_width,
            engine_save_file->window_height,
            T1_os_get_current_time_us());
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
        T1_global->fullscreen = 0;
        T1_global_update_window_size(
            T1_INITIAL_WINDOW_WIDTH,
            T1_INITIAL_WINDOW_HEIGHT,
            T1_os_get_current_time_us());
        T1_global->window_left   = T1_INITIAL_WINDOW_LEFT;
        T1_global->window_bottom = T1_INITIAL_WINDOW_BOTTOM;
    }
    
    if (engine_save_contents != NULL) {
        T1_mem_free_managed(engine_save_contents);
    }
    #elif T1_ENGINE_SAVEFILE_ACTIVE == T1_INACTIVE
    T1_global->fullscreen = 0;
    T1_global->window_wh[0]  = T1_INITIAL_WINDOW_WIDTH;
    T1_global->window_wh[1]  = T1_INITIAL_WINDOW_HEIGHT;
    T1_global->window_left   = T1_INITIAL_WINDOW_LEFT;
    T1_global->window_bottom = T1_INITIAL_WINDOW_BOTTOM;
    
    #if T1_AUDIO_ACTIVE == T1_ACTIVE
    T1_audio_state->music_volume  = 0.6f;
    T1_audio_state->sfx_volume    = 0.8f;
    #elif T1_AUDIO_ACTIVE == T1_INACTIVE
    // Pass
    #else
    #error "T1_AUDIO_ACTIVE undefined!"
    #endif
    
    #else
    #error "T1_ENGINE_SAVEFILE_ACTIVE undefined!"
    #endif // T1_ENGINE_SAVEFILE_ACTIVE
    
    T1_global_init();
    
    T1_io_init(T1_mem_malloc_unmanaged);
    
    T1_gameloop_init(
        /* void *(*arg_malloc_fptr)(size_t): */
            T1_mem_malloc_unmanaged);
    
    T1_objc_autorelease_pool_pop(pool);
    
    T1_gameloop_active = 1;
}



/*
What only matters when T1 renders: pipelines, passes, buffers and texture
arrays. Apps that blit their own pixels never pay for these.
*/
static void T1_appinit_gpu_modules(
    char ** const fatal_error)
{
    if (*fatal_error != 0) { return; }
    
    T1_pipeline_init();
    T1_pass_init();
    T1_buf_init();
    T1_tex_array_init();
}

void T1_gpu_init(
    char ** const fatal_error)
{
    if (*fatal_error != 0) { return; }
    
    if (!T1_gameloop_active) {
        *fatal_error = "T1_gpu_init() before T1_init()";
        return;
    }
    
    void * pool = T1_objc_autorelease_pool_push();
    T1_appinit_gpu_modules(fatal_error);
    T1_objc_autorelease_pool_pop(pool);
    
    T1_os_gpu_init(fatal_error);
    if (*fatal_error != 0) { return; }
    
    pool = T1_objc_autorelease_pool_push();
    T1_os_gpu_update_window_viewport();
    T1_objc_autorelease_pool_pop(pool);
}

void T1_gpu_try_shader_source(
    const char * shader_source,
    u8 * good,
    char * error_recip,
    const u32 error_recip_cap,
    char ** const fatal_error)
{
    T1_os_gpu_try_shader_source(
        shader_source,
        good,
        error_recip,
        error_recip_cap,
        fatal_error);
}

void T1_shutdown(void) {
    T1_os_shutdown();
}
