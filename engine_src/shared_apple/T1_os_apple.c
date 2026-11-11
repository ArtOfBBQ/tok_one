#include "T1_os.h"

#include <pthread.h>
#include <errno.h> // for pthreads error codes
#include <mach/mach_time.h> // the clock (mach, not libc)

// the kernel's write() (libsystem_kernel, not libc), see
// T1_os_write_error()
long write(int fd, const void * bytes, unsigned long size);

#include "T1_std.h"
#include "T1_mem.h"
#include "T1_assert.h"

#include "T1_fatal_error.h"
#include "T1_os_apple_audio.h"
#include "T1_objc.h"

#include <dlfcn.h> // dlsym(), for a function that may be missing

#define T1NSDataReadingUncached 2

// T1_os_init_mutex_and_return_id()'s, in static memory: T1_os_init() makes
// the first one, before anything else is set up
static pthread_mutex_t T1_apple_mutexes[T1_MUTEXES_CAP];
static u32 T1_apple_mutexes_size = 0;

u32 T1_os_init_mutex_and_return_id(void) {
    T1_assert(T1_apple_mutexes_size + 1 < T1_MUTEXES_CAP);
    
    s32 mutex_init_error_value = pthread_mutex_init(
        &T1_apple_mutexes[T1_apple_mutexes_size],
        NULL);
    
    #if T1_ASSERTS_ACTIVE == T1_ACTIVE
    T1_assert(mutex_init_error_value == 0);
    #elif T1_ASSERTS_ACTIVE == T1_INACTIVE
    (void)mutex_init_error_value;
    #else
    #error
    #endif
    
    T1_apple_mutexes_size++;
    return T1_apple_mutexes_size - 1;
}

/*
Attempt to lock a mutex and return True if succesful
*/
u8 T1_os_mutex_trylock(const u32 mutex_id) {
    T1_assert(mutex_id < T1_apple_mutexes_size);
    
    /*
    If successful, pthread_mutex_trylock() will return zero.
    Otherwise, an error number will be returned to indicate the error.
    */
    s32 return_val = pthread_mutex_trylock(&T1_apple_mutexes[mutex_id]);
    
    #if T1_ASSERTS_ACTIVE == T1_ACTIVE
    if (return_val != 0) {
        // EINVAL = The value specified by mutex is invalid
        T1_assert(return_val != EINVAL);
        
        // EPERM = Operation not permitted
        T1_assert(return_val != EPERM);
        
        // EBUSY = Mutex is already locked
        T1_assert(return_val == EBUSY);
    }
    #elif T1_ASSERTS_ACTIVE == T1_INACTIVE
    #else
    #error
    #endif
    
    return return_val == 0;
}

void T1_os_mutex_lock(u32 mutex_id) {
    T1_assert(mutex_id < T1_apple_mutexes_size);
    s32 return_value = pthread_mutex_lock(&T1_apple_mutexes[mutex_id]);
    
    #if T1_ASSERTS_ACTIVE == T1_ACTIVE
    T1_assert(return_value == 0);
    #elif T1_ASSERTS_ACTIVE == T1_INACTIVE
    (void)return_value;
    #else
    #error
    #endif
}

void T1_os_mutex_unlock(u32 mutex_id) {
    T1_assert(mutex_id < T1_apple_mutexes_size);
    s32 return_value = pthread_mutex_unlock(&T1_apple_mutexes[mutex_id]);
    
    #if T1_ASSERTS_ACTIVE == T1_ACTIVE
    T1_assert(return_value == 0);
    #elif T1_ASSERTS_ACTIVE == T1_INACTIVE
    (void)return_value;
    #else
    #error
    #endif
}

typedef struct {
    void * class_ns_filemanager; // NSFileManager
    void * class_ns_data; // NSData
    void * class_ns_url; // NSURL
    void * class_ns_bundle; // NSBundle
    void * class_ns_process_info; // NSProcessInfo
    void * sel_default_manager; // defaultManager
    void * sel_attributes_of_item_at_path_error; // attributesOfItemAtPath:error:
    void * sel_file_size; // fileSize
    void * sel_data_with_contents_of_file; // dataWithContentsOfFile:options:error:
    void * sel_length; // length
    void * sel_file_exists_at_path_is_directory; // fileExistsAtPath:isDirectory:
    void * sel_file_url_with_path_is_directory; // fileURLWithPath:isDirectory:
    void * sel_create_diectory_at_path; // createDirectoryAtPath:withIntermediateDirectories:attributes:error:
    void * sel_contents_of_directory_at_path_error; // contentsOfDirectoryAtPath:error:
    void * sel_count; // count:
    void * sel_get_bytes_length; // getBytes:length:
    void * sel_last_path_component; // lastPathComponent
    void * sel_object_at_index; // objectAtIndex:
    void * sel_main_bundle; // mainBundle
    void * sel_resource_path; // resourcePath
    void * sel_data_with_bytes_length; // dataWithBytes:length:
    void * sel_create_file_at_path_contents_attribs; // createFileAtPath:contents:attributes:
    void * sel_bundle_path; // bundlePath:
    void * sel_process_info; // processInfo
    void * sel_active_processor_count; // activeProcessorCount
    void * sel_remove_item_at_path_error; // removeItemAtPath:error:
    void * sel_copy_item_at_path_to_path_error; // copyItemAtPath:toPath:error:
    b8 framework_good;
} T1OSAppleState;

static T1OSAppleState * T1_os_apple_s = NULL;

void T1_apple_os_init(void) {
    T1_os_apple_s = T1_mem_malloc_unmanaged(
        sizeof(T1OSAppleState));
    T1_std_memset(T1_os_apple_s, 0, sizeof(T1OSAppleState));
    
    // classes
    T1_os_apple_s->class_ns_filemanager = T1_objc_get_class(
        "NSFileManager");
    T1_os_apple_s->class_ns_data = T1_objc_get_class(
        "NSData");
    T1_os_apple_s->class_ns_url =
        T1_objc_get_class("NSURL");
    T1_os_apple_s->class_ns_bundle =
        T1_objc_get_class("NSBundle");
    T1_os_apple_s->class_ns_process_info =
        T1_objc_get_class("NSProcessInfo");
    
    // selectors
    T1_os_apple_s->sel_default_manager = T1_objc_reg_sel(
        "defaultManager");
    T1_os_apple_s->sel_attributes_of_item_at_path_error = T1_objc_reg_sel(
        "attributesOfItemAtPath:error:");
    T1_os_apple_s->sel_file_size = T1_objc_reg_sel(
        "fileSize");
    T1_os_apple_s->sel_data_with_contents_of_file =
        T1_objc_reg_sel(
            "dataWithContentsOfFile:options:error:");
    T1_os_apple_s->sel_length = T1_objc_reg_sel("length");
    T1_os_apple_s->sel_file_exists_at_path_is_directory =
        T1_objc_reg_sel("fileExistsAtPath:isDirectory:");
    T1_os_apple_s->sel_file_url_with_path_is_directory =
        T1_objc_reg_sel("fileURLWithPath:isDirectory:");
    T1_os_apple_s->sel_create_diectory_at_path = T1_objc_reg_sel(
        "createDirectoryAtPath:withIntermediateDirectories:attributes:error:");
    T1_os_apple_s->sel_contents_of_directory_at_path_error =
        T1_objc_reg_sel(
            "contentsOfDirectoryAtPath:error:");
    T1_os_apple_s->sel_count = T1_objc_reg_sel("count");
    T1_os_apple_s->sel_get_bytes_length = T1_objc_reg_sel("getBytes:length:");
    T1_os_apple_s->sel_last_path_component =
        T1_objc_reg_sel("lastPathComponent"); 
    T1_os_apple_s->sel_object_at_index =
        T1_objc_reg_sel("objectAtIndex:");
    T1_os_apple_s->sel_main_bundle = 
        T1_objc_reg_sel("mainBundle");
    T1_os_apple_s->sel_resource_path = 
        T1_objc_reg_sel("resourcePath");
    T1_os_apple_s->sel_data_with_bytes_length = T1_objc_reg_sel(
        "dataWithBytes:length:");
    T1_os_apple_s->sel_create_file_at_path_contents_attribs =
        T1_objc_reg_sel("createFileAtPath:contents:attributes:");
    T1_os_apple_s->sel_bundle_path = T1_objc_reg_sel(
        "bundlePath:");
    T1_os_apple_s->sel_process_info = T1_objc_reg_sel(
        "processInfo");
    T1_os_apple_s->sel_active_processor_count = T1_objc_reg_sel(    
        "activeProcessorCount");
    T1_os_apple_s->sel_remove_item_at_path_error = T1_objc_reg_sel(
        "removeItemAtPath:error:");
    T1_os_apple_s->sel_copy_item_at_path_to_path_error = T1_objc_reg_sel(
        "copyItemAtPath:toPath:error:");
}

void T1_os_get_dir_separator(char * recipient) {
    recipient[0] = '/';
    recipient[1] = '\0';
}

/*
Microseconds since boot, the time asleep too (mach's continuous clock, in
its ticks: times numer / denom is nanoseconds)
*/
u64 T1_os_get_current_time_us(void)
{
    static mach_timebase_info_data_t timebase = { 0, 0 };
    if (timebase.denom == 0) { mach_timebase_info(&timebase); }
    const u64 ticks = mach_continuous_time();
    // (in two steps: ticks * numer alone overflows after a few days)
    const u64 ns =
        (ticks / timebase.denom) * timebase.numer +
        (ticks % timebase.denom) * timebase.numer / timebase.denom;
    return ns / 1000;
}

void T1_os_write_error(const char * text) {
    write(2, text, (unsigned long)T1_std_strlen(text));
}

#if T1_AUDIO_ACTIVE == T1_ACTIVE
void T1_os_audio_init(u8 * good) {
    T1_apple_audio_init(good);
}

void T1_os_audio_start_loop(void)
{
    T1_apple_audio_start_loop();
}
#elif T1_AUDIO_ACTIVE == T1_INACTIVE
#else
#error
#endif

/*
Get a file's size. Returns 0 if no such file
*/
static u64 T1_os_apple_get_filesize_unpooled(
    const char * filepath)
{
    void * nsstrfilepath = T1_objc_nsstring_construct(
        filepath);
    void * error_value = NULL;
    
    void * file_manager = (void *)T1_objc_msg(
        T1_os_apple_s->class_ns_filemanager,
        T1_os_apple_s->sel_default_manager
    );
    
    if (file_manager == NULL) {
        T1_crash("ERROR - failed to get default NSFileManager\n");
        return 0;
    }
    
    // [file_manager attributesOfItemAtPath:nsstrfilepath error:&error_value]
    void * attrib_dict = (void *)T1_objc_msg_2arg(
        file_manager,
        T1_os_apple_s->sel_attributes_of_item_at_path_error,
        (uintptr_t)nsstrfilepath,
        (uintptr_t)&error_value);
    
    if (attrib_dict == NULL || error_value != NULL) {
        return 0;
    }
    
    // [attrib_dict fileSize] returns unsigned long long (u64)
    u64 file_size = T1_objc_msg(
        attrib_dict, T1_os_apple_s->sel_file_size);
    
    return file_size;
}

u64 T1_os_get_filesize(
    const char * filepath)
{
    // Frees the NSStrings etc. this autoreleases right away, also when
    // called outside the main loop (e.g. at startup), where nothing would
    void * pool = T1_objc_autorelease_pool_push();
    
    u64 return_value = T1_os_apple_get_filesize_unpooled(filepath);
    
    T1_objc_autorelease_pool_pop(pool);
    
    return return_value;
}

static void T1_os_apple_read_file_unpooled(
    const char * filepath,
    char * recip,
    u32 * recip_size,
    const u64 recip_cap,
    char ** const fatal_error)
{
    if (*fatal_error != 0) { return; }
    
    if (filepath == NULL || filepath[0] == '\0') {
        *fatal_error = "T1_os_read_file() was passed an empty filepath param";
        return;
    }
    
    void * nsfilepath = T1_objc_nsstring_construct(filepath);
    
    void * error = NULL;
    void * file_data = (void *)T1_objc_msg_3arg(
        T1_os_apple_s->class_ns_data,
        T1_os_apple_s->sel_data_with_contents_of_file,
        /* dataWithContentsOfFile: */ (uintptr_t)nsfilepath,
        /* options: */ T1NSDataReadingUncached,
        /* error: */ (uintptr_t)error);
    
    if (
        error ||
        file_data == NULL ||
        recip_cap < 1 ||
        recip_cap >= UINT32_MAX ||
        T1_objc_msg(file_data, T1_os_apple_s->sel_length) >= UINT32_MAX)
    {
        T1_fatal_error_new(
            fatal_error,  
            "T1_os_read_file() Error - failed [NSData initWithContentsOfFile: ");
        if (T1_std_strlen(filepath) < 450) {
            T1_std_strcat_cap(*fatal_error, 512, filepath);
        } else {
            T1_std_strcat_cap(*fatal_error, 512, "450+ bytes filepath");
        }
        T1_std_strcat_cap(*fatal_error, 512, "]");
        *recip_size = 0;
        return;
    }
    
    *recip_size = (u32)T1_objc_msg(file_data, T1_os_apple_s->sel_length);
    if (*recip_size > recip_cap) { *recip_size = (u32)recip_cap; }
    
    // getBytes:length:
    T1_objc_msg_2arg(
        file_data,
        T1_os_apple_s->sel_get_bytes_length,
        (uintptr_t)recip,
        *recip_size);
    
    recip[*recip_size] = '\0';
}

void T1_os_read_file(
    const char * filepath,
    char * recip,
    u32 * recip_size,
    const u64 recip_cap,
    char ** const fatal_error)
{
    // Frees the NSStrings etc. this autoreleases right away, also when
    // called outside the main loop (e.g. at startup), where nothing would
    void * pool = T1_objc_autorelease_pool_push();
    
    T1_os_apple_read_file_unpooled(
        filepath,
        recip,
        recip_size,
        recip_cap,
        fatal_error);
    
    T1_objc_autorelease_pool_pop(pool);
}

u8 T1_os_file_exists(
    const char * filepath)
{
    void * pool = T1_objc_autorelease_pool_push();
    
    void * nsfilepath = T1_objc_nsstring_construct(
        filepath);
    
    uintptr_t is_directory = 0;
    
    void * manager = (void *)T1_objc_msg(
        T1_os_apple_s->class_ns_filemanager,
        T1_os_apple_s->sel_default_manager);
    
    if (T1_objc_msg_2arg(
        manager,
        T1_os_apple_s->sel_file_exists_at_path_is_directory,
        (uintptr_t)nsfilepath,
        (uintptr_t)&is_directory))
    {
        if (is_directory) {
            T1_objc_autorelease_pool_pop(pool);
            return 0;
        }
        
        T1_objc_autorelease_pool_pop(pool);
        return 1;
    }
    
    T1_objc_autorelease_pool_pop(pool);
    return 0;
}

static b8 T1_os_apple_mkdir_if_not_exist_unpooled(
    const char * dirname)
{
    if (dirname == NULL || dirname[0] == '\0') {
        return 0;
    }
    
    void * ns_dirname = T1_objc_nsstring_construct(
        dirname);
    if (!ns_dirname) {
        return 0;
    }
    
    #if T1_ASSERTS_ACTIVE == T1_ACTIVE
    void * directory_url = (void *)T1_objc_msg_2arg(
        T1_os_apple_s->class_ns_url,
        T1_os_apple_s->sel_file_url_with_path_is_directory,
        (uintptr_t)ns_dirname,
        1);
    if (directory_url == NULL) {
        return 0;
    }
    #elif T1_ASSERTS_ACTIVE == T1_INACTIVE
    #else
    #error
    #endif
    
    void * manager = (void *)T1_objc_msg(
        T1_os_apple_s->class_ns_filemanager,
        T1_os_apple_s->sel_default_manager);
    if (!manager) {
        return 0;
    }
    
    // fileExistsAtPath:isDirectory: takes the path and where to put whether
    // it's a directory (2 arguments)
    u8 is_directory = 0;
    if (
        !T1_objc_msg_2arg(
            manager,
            T1_os_apple_s->sel_file_exists_at_path_is_directory,
            (uintptr_t)ns_dirname,
            (uintptr_t)&is_directory) ||
        !is_directory)
    {
        void * error = NULL;
        
        uintptr_t success = T1_objc_msg_4arg(
            manager,
            T1_os_apple_s->sel_create_diectory_at_path,
            (uintptr_t)ns_dirname,
            (uintptr_t)1,
            (uintptr_t)NULL,
            (uintptr_t)&error);
        
        if (!success) {
            return 0;
        }
    }
    
    return 1;
}

b8 T1_os_mkdir_if_not_exist(
    const char * dirname)
{
    // Frees the NSStrings etc. this autoreleases right away, also when
    // called outside the main loop (e.g. at startup), where nothing would
    void * pool = T1_objc_autorelease_pool_push();
    
    b8 return_value = T1_os_apple_mkdir_if_not_exist_unpooled(dirname);
    
    T1_objc_autorelease_pool_pop(pool);
    
    return return_value;
}

void T1_os_del_file(
    const char * filepath)
{
    void * pool = T1_objc_autorelease_pool_push();
    
    void * nsfilepath = T1_objc_nsstring_construct(
        filepath); 
    
    void * manager = (void *)T1_objc_msg(
        T1_os_apple_s->class_ns_filemanager,
        T1_os_apple_s->sel_default_manager);
    
    if (!manager) {
        T1_objc_autorelease_pool_pop(pool);
        return;
    }
    
    T1_objc_msg_2arg(
        manager,
        T1_os_apple_s->sel_remove_item_at_path_error,
        /* removeItemAtPath: */ (uintptr_t)nsfilepath,
        /* error: */ 0);
    
    T1_objc_autorelease_pool_pop(pool);
}

void T1_os_copy_file(
    const char * filepath_source,
    const char * filepath_destination)
{
    T1_assert(filepath_source != NULL);
    T1_assert(filepath_source[0] != '\0');   
    T1_assert(filepath_destination != NULL);
    T1_assert(filepath_destination[0] != '\0');
    
    
    void * pool = T1_objc_autorelease_pool_push();
    
    void * nsfilepath_source = T1_objc_nsstring_construct(
        filepath_source);
    
    void * nsfilepath_dest = T1_objc_nsstring_construct(
        filepath_destination);
    
    void * manager = (void *)T1_objc_msg(
        T1_os_apple_s->class_ns_filemanager,
        T1_os_apple_s->sel_default_manager);
    if (!manager) {
        T1_objc_autorelease_pool_pop(pool);
        return;
    }
    
    T1_objc_msg_3arg(
        manager,
        T1_os_apple_s->sel_copy_item_at_path_to_path_error,
        /* copyItemAtPath: */ (uintptr_t)nsfilepath_source,
        /* toPath: */ (uintptr_t)nsfilepath_dest,
        /* error: */ 0);
    
    T1_objc_autorelease_pool_pop(pool);
}

void T1_os_write_file(
    const char * filepath,
    const char * output,
    u32 output_size,
    u8 * good)
{
    *good = 0;
    
    void * pool = T1_objc_autorelease_pool_push();
    
    void * nsfilepath = T1_objc_nsstring_construct(filepath);
    if (!nsfilepath) {
        T1_objc_autorelease_pool_pop(pool);
        return;
    }
    
    void * nsdata = (void *)T1_objc_msg_2arg(
        T1_os_apple_s->class_ns_data,
        T1_os_apple_s->sel_data_with_bytes_length,
        /* dataWithBytes: */ (uintptr_t)output,
        /* length: */ output_size);
    if (!nsdata) {
        T1_objc_autorelease_pool_pop(pool);
        return;
    }
    
    void * manager = (void *)T1_objc_msg(
        T1_os_apple_s->class_ns_filemanager,
        T1_os_apple_s->sel_default_manager);
    if (!manager) {
        T1_objc_autorelease_pool_pop(pool);
        return;
    }
    
    uintptr_t result = T1_objc_msg_3arg(
            manager,
            T1_os_apple_s->
                sel_create_file_at_path_contents_attribs,
        /* createFileAtPath: */
            (uintptr_t)nsfilepath,
        /* contents: */
            (uintptr_t)nsdata,
        /* attributes: */
            0);
    
    *good = result > 0;
    
    T1_objc_autorelease_pool_pop(pool);
}

void T1_os_get_filenames_in(
    const char * directory,
    char * filenames,
    const u32 filenames_cap)
{
    // (NSFileManager: libc's opendir() and readdir() are what T1 doesn't use)
    void * pool = T1_objc_autorelease_pool_push();
    
    void * ns_path = T1_objc_nsstring_construct(
        directory);
    
    void * manager = (void *)T1_objc_msg(
        T1_os_apple_s->class_ns_filemanager,
        T1_os_apple_s->sel_default_manager);
    
    void * results = (void *)T1_objc_msg_2arg(
        manager,
        T1_os_apple_s->sel_contents_of_directory_at_path_error,
        (uintptr_t)ns_path,
        0);
    
    if (results == NULL) {
        T1_objc_autorelease_pool_pop(pool);
        return;
    }
    
    u32 results_count = (u32)T1_objc_msg(
        results,
        T1_os_apple_s->sel_count);
    
    u32 char_i = 0;
    for (
        uintptr_t i = 0;
        i < results_count;
        i++)
    {
        void * results_i = (void *)T1_objc_msg_1arg(
            results,
            T1_os_apple_s->sel_object_at_index,
            i);
        void * nsstr_cur_result = (void *)T1_objc_msg(
            results_i,
            T1_os_apple_s->sel_last_path_component);
        char * cur_result = T1_objc_nsstring_to_cstring(
            nsstr_cur_result);
        // (hidden files too, like .DS_Store: not those)
        if (
            cur_result == NULL ||
            cur_result[0] == '\0' ||
            cur_result[0] == '.')
        {
            continue;
        }
        
        u32 new_len = (u32)T1_std_strlen(cur_result);
        if (char_i + new_len + 1 >= filenames_cap) {
            T1_objc_autorelease_pool_pop(pool);
            return;
        }
        
        T1_std_strcpy_cap(
            filenames + char_i,
            (u32)(filenames_cap - char_i),
            cur_result);
        char_i += new_len;
        char_i += 1;
    }
    T1_objc_autorelease_pool_pop(pool);
}

void T1_os_get_res_dir(
    char * recip,
    u32 recip_cap)
{
    void * pool = T1_objc_autorelease_pool_push();
    
    void * bundle = (void *)T1_objc_msg(
        T1_os_apple_s->class_ns_bundle,
        T1_os_apple_s->sel_main_bundle);
    
    void * nsstr_res_path = (void *)T1_objc_msg(
        bundle,
        T1_os_apple_s->sel_resource_path);
    
    char * cstr = T1_objc_nsstring_to_cstring(
        nsstr_res_path);
    
    T1_std_strcpy_cap(
        recip,
        recip_cap,
        cstr);
    
    T1_objc_autorelease_pool_pop(pool);
}

// What a new thread runs: its own copy, so threads started at the same time
// can't overwrite each other's (the thread frees it)
typedef struct {
    void *(* function_to_run)(void *);
    void * argument;
} T1OSThreadStart;

static void * T1_os_apple_start_thread_with_autorelease(
    void * thread_start)
{
    T1OSThreadStart start = *(T1OSThreadStart *)thread_start;
    T1_mem_free_managed(thread_start);

    char * dud_error = 0;
    u64 T1_thread_id = T1_os_get_T1_thread_id(&dud_error);
    T1_assert(dud_error == 0);
    (void)T1_thread_id;

    void * pool = T1_objc_autorelease_pool_push();

    void * out = start.function_to_run(start.argument);

    T1_objc_autorelease_pool_pop(pool);

    // the thread ends: its T1 thread id goes to the next thread
    uint64_t tid;
    pthread_threadid_np(NULL, &tid);
    T1_os_forget_raw_thread_id(tid);

    return out;
}

void T1_os_start_thread(
    void *(*function_to_run)(void *),
    void * argument)
{
    T1OSThreadStart * start = T1_mem_malloc_managed(sizeof(T1OSThreadStart));
    start->function_to_run = function_to_run;
    start->argument = argument;

    pthread_t thread;
    u32 result = (u32)pthread_create(
        &thread,
        NULL,
        T1_os_apple_start_thread_with_autorelease,
        start);
    T1_assert(result == 0);
    (void)result; // only used by T1_assert()
    // nobody joins our threads: their resources go when they end
    pthread_detach(thread);
}

u64 T1_os_get_T1_thread_id(char ** const fatal_error) {
    if (*fatal_error != 0) { return 0; }
    
    uint64_t tid;
    pthread_threadid_np(NULL, &tid);
    
    return T1_os_raw_thread_id_to_T1_thread_id(tid, fatal_error);
}

u32 T1_os_get_cpu_logical_core_count(void)
{
    void * process_info = (void *)T1_objc_msg(
        T1_os_apple_s->class_ns_process_info,
        T1_os_apple_s->sel_process_info);
    
    uintptr_t core_count = T1_objc_msg(
        process_info,
        T1_os_apple_s->sel_active_processor_count);
    
    return (core_count > 0) ? (unsigned int)core_count : 1;
}

/*
FONTS: CoreText draws the glyph into a CoreGraphics bitmap. Both are plain C
APIs, their functions found by name (dlsym) at the first glyph, their types
declared here: no Objective-C, no headers, nothing linked.
*/
typedef struct { f64 x; f64 y; } T1FontCGPoint;
typedef struct { f64 width; f64 height; } T1FontCGSize;
typedef struct { T1FontCGPoint origin; T1FontCGSize size; } T1FontCGRect;
typedef struct { long location; long length; } T1FontCFRange;

#define T1kCGImageAlphaOnly 7u
#define T1kCTFontOrientationDefault 0u

typedef struct {
    void * (* CFStringCreateWithCharacters)(void *, const u16 *, long);
    void (* CFRelease)(const void *);
    u8 (* CFEqual)(const void *, const void *);
    void * (* CTFontCreateWithName)(void *, f64, const void *);
    void * (* CTFontCreateForString)(void *, void *, T1FontCFRange);
    // (macOS 10.15+: CTFontCreateForString() without it, the system's language)
    void * (* CTFontCreateForStringWithLanguage)(
        void *, void *, T1FontCFRange, void *);
    void * (* CTFontCopyPostScriptName)(void *);
    f64 (* CTFontGetAscent)(void *);
    f64 (* CTFontGetDescent)(void *);
    u8 (* CTFontGetGlyphsForCharacters)(void *, const u16 *, u16 *, long);
    T1FontCGRect (* CTFontGetBoundingRectsForGlyphs)(
        void *, u32, const u16 *, T1FontCGRect *, long);
    f64 (* CTFontGetAdvancesForGlyphs)(
        void *, u32, const u16 *, T1FontCGSize *, long);
    void (* CTFontDrawGlyphs)(
        void *, const u16 *, const T1FontCGPoint *, size_t, void *);
    void * (* CGBitmapContextCreate)(
        void *, size_t, size_t, size_t, size_t, void *, u32);
    void (* CGContextSetGrayFillColor)(void *, f64, f64);
    void (* CGContextRelease)(void *);
    
    void * base_font; // the system's font, at base_size
    f32 base_size;
    // CoreText's font of last resort: a box for every codepoint, no glyph
    void * last_resort_name;
    // T1_font_set_language()'s code, and as a CFString (NULL: the system's)
    char language[T1_FONT_LANGUAGE_CAP];
    void * language_cf;
    u8 language_changed;
    u8 loaded; // 0: not yet, 1: loaded, 2: failed (no glyphs, ever)
} T1AppleFonts;
static T1AppleFonts T1_apple_fonts;

// a CFString of ASCII text (the caller releases it)
static void * T1_apple_cfstring(const char * ascii) {
    u16 utf16[64];
    long n = 0;
    while (ascii[n] != '\0' && n < 64) {
        utf16[n] = (u16)(u8)ascii[n];
        n++;
    }
    return T1_apple_fonts.CFStringCreateWithCharacters(NULL, utf16, n);
}

static void T1_apple_fonts_load(void) {
    T1AppleFonts * f = &T1_apple_fonts;
    f->loaded = 2;
    
    if (
        !T1_objc_open_framework(
            "/System/Library/Frameworks/CoreFoundation.framework/"
            "CoreFoundation") ||
        !T1_objc_open_framework(
            "/System/Library/Frameworks/CoreGraphics.framework/"
            "CoreGraphics") ||
        !T1_objc_open_framework(
            "/System/Library/Frameworks/CoreText.framework/CoreText"))
    {
        return;
    }
    
    // Every function pointer has the same size, the field's type is right
    #define T1_APPLE_FONT_FN(name) { \
        void * found = T1_objc_get_func(#name); \
        if (found == NULL) { return; } \
        T1_std_memcpy(&f->name, &found, sizeof(void *)); }
    T1_APPLE_FONT_FN(CFStringCreateWithCharacters);
    T1_APPLE_FONT_FN(CFRelease);
    T1_APPLE_FONT_FN(CFEqual);
    T1_APPLE_FONT_FN(CTFontCreateWithName);
    T1_APPLE_FONT_FN(CTFontCreateForString);
    T1_APPLE_FONT_FN(CTFontCopyPostScriptName);
    T1_APPLE_FONT_FN(CTFontGetAscent);
    T1_APPLE_FONT_FN(CTFontGetDescent);
    T1_APPLE_FONT_FN(CTFontGetGlyphsForCharacters);
    T1_APPLE_FONT_FN(CTFontGetBoundingRectsForGlyphs);
    T1_APPLE_FONT_FN(CTFontGetAdvancesForGlyphs);
    T1_APPLE_FONT_FN(CTFontDrawGlyphs);
    T1_APPLE_FONT_FN(CGBitmapContextCreate);
    T1_APPLE_FONT_FN(CGContextSetGrayFillColor);
    T1_APPLE_FONT_FN(CGContextRelease);
    #undef T1_APPLE_FONT_FN
    
    {
        void * found = dlsym(RTLD_DEFAULT, "CTFontCreateForStringWithLanguage");
        T1_std_memcpy(&f->CTFontCreateForStringWithLanguage, &found,
            sizeof(void *));
    }
    
    f->last_resort_name = T1_apple_cfstring("LastResort");
    if (f->last_resort_name == NULL) { return; }
    f->loaded = 1;
}

/*
The system's font (Helvetica: on every Apple system, CoreText finds the
rest), at the point size that makes its ascent plus descent size pixels
*/
static void * T1_apple_base_font(const f32 size) {
    T1AppleFonts * f = &T1_apple_fonts;
    if (f->base_font != NULL && f->base_size == size) {
        return f->base_font;
    }
    if (f->base_font != NULL) {
        f->CFRelease(f->base_font);
        f->base_font = NULL;
    }
    
    void * name = T1_apple_cfstring("Helvetica");
    if (name == NULL) { return NULL; }
    void * probe = f->CTFontCreateWithName(name, 100.0, NULL);
    if (probe == NULL) {
        f->CFRelease(name);
        return NULL;
    }
    const f64 span = f->CTFontGetAscent(probe) + f->CTFontGetDescent(probe);
    f->CFRelease(probe);
    if (!(span > 0.0)) {
        f->CFRelease(name);
        return NULL;
    }
    f->base_font = f->CTFontCreateWithName(
        name, (f64)size * (100.0 / span), NULL);
    f->CFRelease(name);
    f->base_size = size;
    return f->base_font;
}

void T1_os_font_set_language(const char * code) {
    T1AppleFonts * f = &T1_apple_fonts;
    T1_std_strcpy_cap(f->language, T1_FONT_LANGUAGE_CAP, code);
    // (made at the next glyph: CoreFoundation may not be loaded yet)
    f->language_changed = 1;
}

// T1_font_set_language()'s language as a CFString, NULL: the system's
static void * T1_apple_font_language(void) {
    T1AppleFonts * f = &T1_apple_fonts;
    if (f->language_changed) {
        f->language_changed = 0;
        if (f->language_cf != NULL) {
            f->CFRelease(f->language_cf);
            f->language_cf = NULL;
        }
        if (f->language[0] != '\0') {
            f->language_cf = T1_apple_cfstring(f->language);
        }
    }
    return f->language_cf;
}

static s32 T1_apple_floor(const f64 x) {
    const s32 i = (s32)x;
    return (f64)i > x ? i - 1 : i;
}

static s32 T1_apple_ceil(const f64 x) {
    const s32 i = (s32)x;
    return (f64)i < x ? i + 1 : i;
}

void T1_os_font_glyph(
    u32 codepoint,
    f32 size,
    u8 * alpha,
    u32 alpha_cap,
    u32 * w,
    u32 * h,
    f32 * left,
    f32 * top,
    f32 * advance,
    u8 * good)
{
    T1AppleFonts * f = &T1_apple_fonts;
    if (f->loaded == 0) { T1_apple_fonts_load(); }
    if (f->loaded != 1) { return; }
    
    void * base = T1_apple_base_font(size);
    if (base == NULL) { return; }
    
    // the codepoint in UTF-16, CoreText's text
    u16 utf16[2];
    long units = 1;
    if (codepoint < 0x10000u) {
        utf16[0] = (u16)codepoint;
    } else {
        const u32 above = codepoint - 0x10000u;
        utf16[0] = (u16)(0xD800u + (above >> 10));
        utf16[1] = (u16)(0xDC00u + (above & 0x3FFu));
        units = 2;
    }
    
    // the base font if it has the codepoint, else one that does
    void * text = f->CFStringCreateWithCharacters(NULL, utf16, units);
    if (text == NULL) { return; }
    T1FontCFRange range;
    range.location = 0;
    range.length = units;
    // (the language picks between e.g. Japanese and Chinese fonts)
    void * language = T1_apple_font_language();
    void * font =
        language != NULL && f->CTFontCreateForStringWithLanguage != NULL ?
            f->CTFontCreateForStringWithLanguage(base, text, range, language) :
            f->CTFontCreateForString(base, text, range);
    f->CFRelease(text);
    if (font == NULL) { return; }
    
    u8 is_last_resort = 0;
    void * font_name = f->CTFontCopyPostScriptName(font);
    if (font_name != NULL) {
        is_last_resort = f->CFEqual(font_name, f->last_resort_name) != 0;
        f->CFRelease(font_name);
    }
    u16 glyphs[2] = { 0, 0 };
    const u8 found = f->CTFontGetGlyphsForCharacters(
        font, utf16, glyphs, units);
    if (is_last_resort || !found || glyphs[0] == 0) {
        f->CFRelease(font);
        return;
    }
    
    T1FontCGRect bounds;
    f->CTFontGetBoundingRectsForGlyphs(
        font, T1kCTFontOrientationDefault, glyphs, &bounds, 1);
    T1FontCGSize advance_size;
    f->CTFontGetAdvancesForGlyphs(
        font, T1kCTFontOrientationDefault, glyphs, &advance_size, 1);
    *advance = (f32)advance_size.width;
    
    // nothing to draw (a space)
    if (!(bounds.size.width > 0.0) || !(bounds.size.height > 0.0)) {
        f->CFRelease(font);
        *good = 1;
        return;
    }
    if (
        !(bounds.size.width < 4096.0) ||
        !(bounds.size.height < 4096.0) ||
        !(bounds.origin.x > -4096.0 && bounds.origin.x < 4096.0) ||
        !(bounds.origin.y > -4096.0 && bounds.origin.y < 4096.0))
    {
        f->CFRelease(font);
        return;
    }
    
    // whole pixels around the ink, and one more for its antialiasing
    const s32 x0 = T1_apple_floor(bounds.origin.x) - 1;
    const s32 y0 = T1_apple_floor(bounds.origin.y) - 1;
    const s32 x1 = T1_apple_ceil(bounds.origin.x + bounds.size.width) + 1;
    const s32 y1 = T1_apple_ceil(bounds.origin.y + bounds.size.height) + 1;
    *w = (u32)(x1 - x0);
    *h = (u32)(y1 - y0);
    *left = (f32)x0;
    *top = (f32)y1;
    // (only measured)
    if (alpha == NULL) {
        f->CFRelease(font);
        *good = 1;
        return;
    }
    if ((u64)*w * (u64)*h > (u64)alpha_cap) {
        f->CFRelease(font);
        return;
    }
    
    /*
    An alpha-only bitmap of our memory (its first row the top one), the
    pen at (-x0, -y0) from its bottom left so the ink lands inside
    */
    T1_std_memset(alpha, 0, *w * *h);
    void * context = f->CGBitmapContextCreate(
        alpha, *w, *h, 8, *w, NULL, T1kCGImageAlphaOnly);
    if (context == NULL) {
        *w = 0;
        *h = 0;
        f->CFRelease(font);
        return;
    }
    f->CGContextSetGrayFillColor(context, 1.0, 1.0);
    T1FontCGPoint pen;
    pen.x = (f64)-x0;
    pen.y = (f64)-y0;
    f->CTFontDrawGlyphs(font, glyphs, &pen, 1, context);
    f->CGContextRelease(context);
    f->CFRelease(font);
    *good = 1;
}
