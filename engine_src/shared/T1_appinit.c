#include "T1_appinit.h"
#include "T1_pipeline.h"
#include "T1_pass.h"
#include "T1_buf.h"


#include "T1_std.h"
#include "T1_mem.h"
#include "T1_objc.h"
#include "T1_embedded_data.h"
#include "T1_assert.h"
#include "T1_shader.h"
#include "T1_audio.h"
#include "T1_io.h"
#include "T1_global.h"
#include "T1_gameloop.h"
#include "T1_tex_array.h"
#include "T1_fatal_error.h"

#include "T1_os.h"

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
}

void T1_gpu_try_shader_source(
    const char * shader_source,
    u8 * good,
    char * error_recip,
    const u32 error_recip_cap,
    char ** const fatal_error)
{
    if (*fatal_error != 0) { return; }
    
    #if T1_SHADER_CHECK_ACTIVE == T1_ACTIVE
    if (shader_source != NULL && shader_source[0] != '\0') {
        T1_shader_check(shader_source, good, error_recip, error_recip_cap);
        if (!*good) { return; }
    }
    #elif T1_SHADER_CHECK_ACTIVE == T1_INACTIVE
    #else
    #error
    #endif
    
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
