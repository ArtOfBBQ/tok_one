#define PLATFORM_NS_FILEMANAGER

#import "Appkit/Appkit.h"

#include "T1_platform_layer.h"

void platform_get_writables_path(
    char * recipient,
    const uint32_t recipient_size)
{
    #ifdef COMMON_IGNORE_ASSERTS
    (void)recipient_size;
    #endif
    
    NSArray * paths = NSSearchPathForDirectoriesInDomains(
        NSApplicationSupportDirectory,
        NSUserDomainMask,
        YES);
    
    NSString * libraryDirectory = [paths objectAtIndex:0];
    
    char * library_dir =
        (char *)[libraryDirectory
            cStringUsingEncoding: NSUTF8StringEncoding];
    
    T1_std_strcpy_cap(recipient, recipient_size, library_dir);
    T1_std_strcat_cap(recipient, recipient_size, "/");
    T1_std_strcat_cap(recipient, recipient_size, APPLICATION_NAME);
    
    platform_mkdir_if_not_exist(recipient);
}

void * platform_malloc_unaligned_block(
    const uint64_t size)
{
    void * return_value = mmap(
        /* void *: */
            NULL,
        /* size_t: */
            size,
        /* int prot: */
            PROT_READ | PROT_WRITE,
        /* int: */
            MAP_SHARED | MAP_ANONYMOUS,
        /* int: */
            -1,
        /* off_t: */
            0);
    
    if (return_value == MAP_FAILED) {
        return NULL;
    }
    
    return return_value;
}

void platform_close_application(void) {
    [NSApp terminate: nil];
}

void platform_get_cwd(char * recipient, const uint32_t recipient_size) {
    
    #ifdef COMMON_IGNORE_ASSERTS
    (void)recipient_size;
    #endif
    
    NSString * cwd = [[NSFileManager defaultManager] currentDirectoryPath];
    
    char * return_value = (char *)[cwd UTF8String];
    
    T1_std_strcpy_cap(recipient, recipient_size, return_value);
}

float platform_x_to_x(const float x) {
    return x;
}

float platform_y_to_y(const float y) {
    return y;
}

void platform_open_folder_in_window_if_possible(
    const char * folderpath)
{
    log_append("Trying to open folder: ");
    log_append(folderpath);
    log_append_char('\n');
    
    if (folderpath == NULL || folderpath[0] == '\0') {
        return;
    }
    
    NSString * folderpath_ns = [NSString stringWithUTF8String:folderpath];
    NSURL * folderURL = [NSURL fileURLWithPath: folderpath_ns];
    [[NSWorkspace sharedWorkspace] openURL: folderURL];
}

