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

    #if T1_PLATFORM_APPLE == T1_ACTIVE
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
    #elif T1_PLATFORM_WINDOWS == T1_ACTIVE
    #elif T1_PLATFORM_WEB == T1_ACTIVE
    #else
    #error
    #endif
    
    T1_global = (T1Globals *)T1_mem_malloc_unmanaged(
        sizeof(T1Globals));
    if (!T1_global) { *fatal_error = "Malloc fail"; return; }
    T1_std_memset(T1_global, 0, sizeof(T1Globals));

    // before audio, which needs its DLLs
    #if T1_PLATFORM_WINDOWS == T1_ACTIVE
    T1_os_windows_init(fatal_error);
    if (*fatal_error != 0) { return; }
    #elif T1_PLATFORM_APPLE == T1_ACTIVE
    #elif T1_PLATFORM_WEB == T1_ACTIVE
    #else
    #error
    #endif

    // (audio waits for T1_audio_start(): the client's fill makes it)
    
    #if T1_PLATFORM_APPLE == T1_ACTIVE
    T1_apple_os_init();
    #if TARGET_OS_OSX
    T1_os_macos_init();
    #else
    #error "T1's only Apple layer is macOS's"
    #endif
    #elif T1_PLATFORM_WINDOWS == T1_ACTIVE
    #elif T1_PLATFORM_WEB == T1_ACTIVE
    #else
    #error
    #endif
    
    // The writables dir (saves, caches) has to exist before anything is
    // written to it: nothing else creates it
    {
        char writables_dir[512];
        T1_std_memset(writables_dir, 0, 512);
        T1_os_writable_filename_to_pathfile("", writables_dir, 512);
        T1_os_mkdir_if_not_exist(writables_dir);
    }
    
    T1_global->fullscreen = 0;
    T1_global->window_wh[0]  = T1_INITIAL_WINDOW_WIDTH;
    T1_global->window_wh[1]  = T1_INITIAL_WINDOW_HEIGHT;
    T1_global->window_left   = T1_INITIAL_WINDOW_LEFT;
    T1_global->window_bottom = T1_INITIAL_WINDOW_BOTTOM;
    
    T1_global_init();
    
    T1_io_init(T1_mem_malloc_unmanaged);
    
    T1_gameloop_init(
        /* void *(*arg_malloc_fptr)(size_t): */
            T1_mem_malloc_unmanaged);

    #if T1_PLATFORM_APPLE == T1_ACTIVE
    T1_objc_autorelease_pool_pop(pool);
    #elif T1_PLATFORM_WINDOWS == T1_ACTIVE
    #elif T1_PLATFORM_WEB == T1_ACTIVE
    #else
    #error
    #endif

    T1_gameloop_active = 1;
}



/*
T1's GPU objects: pipelines, passes, buffers and texture arrays. Every app
that opens a window needs them, it shows a T1Tex.
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
    
    #if T1_PLATFORM_APPLE == T1_ACTIVE
    void * pool = T1_objc_autorelease_pool_push();
    T1_appinit_gpu_modules(fatal_error);
    T1_objc_autorelease_pool_pop(pool);
    #elif T1_PLATFORM_WINDOWS == T1_ACTIVE || T1_PLATFORM_WEB == T1_ACTIVE
    T1_appinit_gpu_modules(fatal_error);
    #else
    #error
    #endif
    
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
