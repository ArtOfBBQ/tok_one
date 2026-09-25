#include "T1_platform_layer.h"

#include <pthread.h>
#include <dirent.h>
#include <errno.h> // for pthreads error codes
#include <sys/time.h>
#include <sys/sysctl.h> // for sysctl to get clock frequency

#include "T1_std.h"
#include "T1_mem.h"
#include "T1_log.h"
#include "T1_apple_audio.h"
#include "T1_objc.h"

#define T1NSDataReadingUncached 2

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
    
    T1_objc_open_framework_and_link_perma_good_val(
        "/System/Library/Frameworks/Foundation.framework/Foundation",
        &T1_os_apple_s->framework_good);
    
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
    T1_os_apple_s->sel_count = T1_objc_reg_sel("count:");
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
    T1_objc_close_current_framework();
}

f32 T1_os_get_screen_backing_scale_factor(void) {
    return 1.0f;
}

u32 T1_os_get_dir_separator_size(void) {
    return 1;
}

void T1_os_get_dir_separator(char * recipient) {
    recipient[0] = '/';
    recipient[1] = '\0';
}

u64 T1_os_get_current_time_us(void)
{
    struct timeval tv;
    gettimeofday(&tv,NULL);
    u64 result =
        1000000 *
            (u64)tv.tv_sec +
            (u64)tv.tv_usec;
    
    return result;
}

u64 T1_os_get_clock_frequency(void) {
    //    int mib[2];
    //    size_t len;
    //    mib[0] = CTL_KERN;
    //    mib[1] = KERN_CLOCKRATE;
    //    struct clockinfo clockinfo;
    //    len = sizeof(clockinfo);
    //    int result = sysctl(mib, 2, &clockinfo, &len, NULL, 0);
    //    assert(result != -1);
    // log_trace("clockinfo.hz: %d\n", clockinfo.hz);
    // log_trace("clockinfo.tick: %d\n", clockinfo.tick);
    // return (u64)clockinfo.tick;
    return 3600000000; // my pc's clock frequency
}

#if T1_AUDIO_ACTIVE == T1_ACTIVE
void T1_platform_audio_init(void) {
    T1_apple_audio_init();
}

void T1_platform_audio_start_loop(void)
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
u64 T1_os_get_filesize(
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
        T1_log_dump_and_crash("ERROR - failed to get default NSFileManager\n");
        return 0;
    }
    
    // [file_manager attributesOfItemAtPath:nsstrfilepath error:&error_value]
    void * attrib_dict = (void *)T1_objc_msg_2arg(
        file_manager,
        T1_os_apple_s->sel_attributes_of_item_at_path_error,
        (uintptr_t)nsstrfilepath,
        (uintptr_t)&error_value
    );

    if (attrib_dict == NULL || error_value != NULL) {
        T1_log_append("ERROR - failed to get size of file - ");
        T1_log_append(filepath);
        T1_log_append("\n");
        return 0;
    }

    // [attrib_dict fileSize] returns unsigned long long (u64)
    u64 file_size = T1_objc_msg(
        attrib_dict, T1_os_apple_s->sel_file_size);
    
    return file_size;
}

void T1_os_read_file(
    const char * filepath,
    char * recip,
    u32 * recip_size,
    const u64 recip_cap,
    u8 * good)
{
    //@autoreleasepool {
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
        T1_log_append("Error - failed [NSData initWithContentsOfFile:]\n");
        *recip_size = 0;
        *good = false;
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
    
    *good = true;
}

u8 T1_os_file_exists(
    const char * filepath)
{
    void * nsfilepath = T1_objc_nsstring_construct(
        filepath);
    
    uintptr_t is_directory = false;
    
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
            T1_log_append("warning filepath: ");
            T1_log_append(filepath);
            T1_log_append(" is a directory, returnin FALSE for existence\n");
                return false;
        }
        
        return true;
    }
    
    T1_log_append("filepath: ");
    T1_log_append(filepath);
    T1_log_append(" does not exist, returning FALSE...\n");
    return false;
}

void T1_os_mkdir_if_not_exist(
    const char * dirname)
{
    T1_log_append("make directory if it doesn't exist: ");
    T1_log_append(dirname);
    T1_log_append("\n");
    
    void * ns_dirname = T1_objc_nsstring_construct(
        dirname);
    
    #if T1_LOG_ASSERTS_ACTIVE == T1_ACTIVE
    void * directory_url = (void *)T1_objc_msg_2arg(
        T1_os_apple_s->class_ns_url,
        T1_os_apple_s->sel_file_url_with_path_is_directory,
        (uintptr_t)ns_dirname,
        true);
    T1_log_assert(directory_url != NULL);
    #elif T1_LOG_ASSERTS_ACTIVE == T1_INACTIVE
    #else
    #error
    #endif
    
    void * manager = (void *)T1_objc_msg(
        T1_os_apple_s->class_ns_filemanager,
        T1_os_apple_s->sel_default_manager);
    
    if (
        !T1_objc_msg_1arg(
            manager,
            T1_os_apple_s->sel_file_exists_at_path_is_directory,
            (uintptr_t)ns_dirname))
    {
        void * error = NULL;
        
        uintptr_t success = T1_objc_msg_4arg(
            manager,
            T1_os_apple_s->sel_create_diectory_at_path,
            (uintptr_t)ns_dirname,
            (uintptr_t)true,
            (uintptr_t)NULL,
            (uintptr_t)&error);
        
        if (!success) {
            T1_log_dump_and_crash("ERROR - tried to create a directory and failed\n");
            return;
        }
    }
    
    return;
}

void T1_os_del_file(
    const char * filepath)
{
    T1_log_append(
        "trying to delete a file with NSFileManager: ");
    T1_log_append(filepath);
    T1_log_append("\n");
    
    void * nsfilepath = T1_objc_nsstring_construct(
        filepath); 
    
    void * manager = (void *)T1_objc_msg(
        T1_os_apple_s->class_ns_filemanager,
        T1_os_apple_s->sel_default_manager);
    
    if (!manager) { return; }
    
    T1_objc_msg_2arg(
        manager,
        T1_os_apple_s->sel_remove_item_at_path_error,
        /* removeItemAtPath: */ (uintptr_t)nsfilepath,
        /* error: */ 0);
}

void T1_os_copy_file(
    const char * filepath_source,
    const char * filepath_destination)
{
    T1_log_assert(filepath_source != NULL);
    T1_log_assert(filepath_source[0] != '\0');   
    T1_log_assert(filepath_destination != NULL);
    T1_log_assert(filepath_destination[0] != '\0');
    
    T1_log_append("trying to copy from: ");
    T1_log_append(filepath_source);
    T1_log_append(", to: ");
    T1_log_append(filepath_destination);
    T1_log_append_c8('\n');
    
    void * nsfilepath_source = T1_objc_nsstring_construct(
        filepath_source);
    
    void * nsfilepath_dest = T1_objc_nsstring_construct(
        filepath_destination);
    
    void * manager = (void *)T1_objc_msg(
        T1_os_apple_s->class_ns_filemanager,
        T1_os_apple_s->sel_default_manager);
    if (!manager) { return; }
    
    T1_objc_msg_3arg(
        manager,
        T1_os_apple_s->sel_copy_item_at_path_to_path_error,
        /* copyItemAtPath: */ (uintptr_t)nsfilepath_source,
        /* toPath: */ (uintptr_t)nsfilepath_dest,
        /* error: */ 0);
}

void T1_os_write_file(
    const char * filepath,
    const char * output,
    u32 output_size,
    u8 * good)
{
    *good = false;
    
    void * nsfilepath = T1_objc_nsstring_construct(filepath);
    if (!nsfilepath) { return; }
    
    void * nsdata = (void *)T1_objc_msg_2arg(
        T1_os_apple_s->class_ns_data,
        T1_os_apple_s->sel_data_with_bytes_length,
        /* dataWithBytes: */ (uintptr_t)output,
        /* length: */ output_size);
    if (!nsdata) { return; }
    
    void * manager = (void *)T1_objc_msg(
        T1_os_apple_s->class_ns_filemanager,
        T1_os_apple_s->sel_default_manager);
    if (!manager) { return; }
    
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
}

__attribute__((no_sanitize("address")))
void T1_os_get_filenames_in(
    const char * directory,
    char * filenames,
    const u32 filenames_cap)
{
    #if 1
    DIR * dir = opendir(directory);
    if (!dir) {
        T1_log_append(
            "ERROR - failed to open directory\n");
        return;
    }
    
    struct dirent * entry;
    u32 i = 0;
    while (
        (entry = readdir(dir)) != NULL)
    {
        if (
            entry->d_name[0] == '\0' ||
            entry->d_name[0] == '.')
        {
            continue;
        }
        
        u32 new_len = (u32)T1_std_strlen(entry->d_name);
        if (i + new_len + 1 >= filenames_cap) {
            return;
        }
        
        T1_std_strcpy_cap(
            filenames + i,
            filenames_cap - i,
            entry->d_name);
        i += new_len;
        i += 1;
    }
    
    closedir(dir);
    #else
    void * ns_path = T1_objc_nsstring_construct(
        directory);
    // NSURL * url = [NSURL URLWithString: path];
    
    // T1_log_assert(url != NULL);
    
    void * manager = (void *)T1_objc_msg(
        T1_apple_os_s->class_ns_filemanager,
        T1_apple_os_s->sel_default_manager);
    
    void * results = (void *)T1_objc_msg_2arg(
        manager,
        T1_apple_os_s->sel_contents_of_directory_at_path_error,
        (uintptr_t)ns_path,
        0);
    
    if (results == NULL) {
        return;
    }
    
    u32 results_count = (u32)T1_objc_msg(
        results,
        T1_apple_os_s->sel_count);
    
    u32 char_i = 0;
    for (
        uintptr_t i = 0;
        i < results_count;
        i++)
    {
        void * results_i = (void *)T1_objc_msg_1arg(
            results,
            T1_apple_os_s->sel_object_at_index,
            i);
        void * nsstr_cur_result = (void *)T1_objc_msg(
            results_i,
            T1_apple_os_s->sel_last_path_component);
        char * cur_result = T1_objc_nsstring_to_cstring(
            nsstr_cur_result);
        
        u32 new_len = (u32)T1_std_strlen(cur_result);
        if (char_i + new_len + 1 >= filenames_cap) {
            return;
        }
        
        T1_std_strcpy_cap(
            filenames + char_i,
            (u32)(filenames_cap - char_i),
            cur_result);
        char_i += new_len;
        char_i += 1;
    }
    #endif
}

void T1_os_get_app_dir(
    char * recipient,
    const u32 recipient_size)
{
    recipient[0] = '\0';
    
    void * main_bundle = (void *)T1_objc_msg(
        T1_os_apple_s->class_ns_bundle,
        T1_os_apple_s->sel_bundle_path);
    
    char * cstr_main_bundle = T1_objc_nsstring_to_cstring(
        main_bundle);
    
    T1_std_strcpy_cap(
        recipient,
        recipient_size,
        cstr_main_bundle);
}

void T1_os_get_res_dir(
    char * recip,
    u32 recip_cap)
{
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
}

void T1_os_start_thread(
    void *(*function_to_run)(void *),
    void * argument)
{
    // TODO: maybe we should just use pthread for threads instead of
    // dispatch_async, since we need pthreads for mutex locks anyway
    // Let's revisit this when we port to other platforms
    pthread_t thread;
    u32 result = (u32)pthread_create(
        &thread,
        NULL,
        function_to_run,
        argument);
    T1_log_assert(result == 0);
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
