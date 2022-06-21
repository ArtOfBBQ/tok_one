#define PLATFORM_NS_FILEMANAGER

#include "../shared/platform_layer.h"

char * platform_get_cwd()
{
    NSString * cwd = [[NSFileManager defaultManager] currentDirectoryPath];
    
    char * return_value = (char *)[cwd UTF8String];
    
    return return_value;
}
