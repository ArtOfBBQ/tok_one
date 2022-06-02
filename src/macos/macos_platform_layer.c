#define PLATFORM_NS_FILEMANAGER

#ifndef FUINT64 
#define FUINT64 "%llu"
#endif

#include "../shared/platform_layer.h"

char * platform_get_cwd()
{
    NSString * cwd = [[NSFileManager defaultManager] currentDirectoryPath];
    NSLog(@"%@", cwd);
    
    char * return_value = (char *)[cwd UTF8String];
    
    return return_value;
}

