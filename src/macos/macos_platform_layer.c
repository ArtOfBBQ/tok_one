#define PLATFORM_NS_FILEMANAGER

#include "../shared/platform_layer.h"

NSFileManager * file_manager;

char * platform_get_cwd()
{
    if (file_manager == NULL) {
        file_manager =
            [[NSFileManager alloc] init];
    }
    
    NSString * cwd = [file_manager currentDirectoryPath];
    NSLog(@"%@", cwd);
    
    char * return_value = (char *)[cwd UTF8String];
    
    return return_value;
}

