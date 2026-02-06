#import "Appkit/Appkit.h"

#include "T1_platform_layer.h"

void T1_platform_get_writables_dir(
    char * recipient,
    const uint32_t recipient_size)
{
    #if T1_STD_ASSERTS_ACTIVE == T1_ACTIVE
    (void)recipient_size;
    #elif T1_STD_ASSERTS_INACTIVE == T1_INACTIVE
    #else
    #error
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
    T1_std_strcat_cap(recipient, recipient_size, T1_APP_NAME);
    
    T1_platform_mkdir_if_not_exist(recipient);
}

void * T1_platform_malloc_unaligned_block(
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

void T1_platform_close_app(void) {
    [NSApp terminate: nil];
}

void T1_platform_get_cwd(char * recipient, const uint32_t recipient_size) {
    
    #if T1_STD_ASSERTS_ACTIVE == T1_ACTIVE
    (void)recipient_size;
    #elif T1_STD_ASSERTS_ACTIVE == T1_INACTIVE
    #else
    #error
    #endif
    
    NSString * cwd = [[NSFileManager defaultManager] currentDirectoryPath];
    
    char * return_value = (char *)[cwd UTF8String];
    
    T1_std_strcpy_cap(recipient, recipient_size, return_value);
}

float T1_platform_x_to_x(const float x) {
    return x;
}

float T1_platform_y_to_y(const float y) {
    return y;
}

void T1_platform_open_dir_in_window_if_possible(
    const char * folderpath)
{
    T1_log_append("Trying to open folder: ");
    T1_log_append(folderpath);
    T1_log_append_char('\n');
    
    if (folderpath == NULL || folderpath[0] == '\0') {
        return;
    }
    
    NSString * folderpath_ns = [NSString stringWithUTF8String:folderpath];
    NSURL * folderURL = [NSURL fileURLWithPath: folderpath_ns];
    [[NSWorkspace sharedWorkspace] openURL: folderURL];
}
