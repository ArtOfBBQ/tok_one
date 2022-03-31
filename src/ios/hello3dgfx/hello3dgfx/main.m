#import <UIKit/UIKit.h>
#import "AppDelegate.h"
#include "stdlib.h" // TODO: do we really need stdlib.h?

 // definitions of functions we need to implement
#include "platform_layer.h"

// functions we can use
#include "window_size.h"
#include "vertex_types.h"
#include "software_renderer.h"
#include "gpu.h"
#include "zpolygon.h"

/*
This functionality must be provided by the platform because
of iOS, where reading your own app's files is a security
ordeal
*/
FileBuffer * platform_read_file(char * filename) {
    printf("platform_read of filename: %s¥n", filename);
    
    NSString * ns_filename =
        [NSString stringWithUTF8String:filename];
    
    NSString * file_part = [[ns_filename lastPathComponent] stringByDeletingPathExtension];
    NSString * extension_part = [ns_filename pathExtension];
    
    NSString * filepath =
        [[NSBundle mainBundle]
            pathForResource:file_part
            ofType:extension_part];
    
    FileBuffer * return_value =
        malloc(sizeof(FileBuffer));
    
    FILE * rawfile = fopen(
        [filepath cStringUsingEncoding:NSUTF8StringEncoding],
        "rb");
    
    assert(rawfile != NULL);
    
    fseek(rawfile, 0, SEEK_END);
    unsigned long fsize = (unsigned long)ftell(rawfile);              
    fseek(rawfile, 0, SEEK_SET);
    
    char * buffer = malloc(fsize);
    
    size_t bytes_read = fread(
        /* ptr: */
            buffer,
       /* size of each element to be read: */
            1,
       /* nmemb (no of members) to read: */
           fsize,
        /* stream: */
           rawfile);
    
    fclose(rawfile);
    if (bytes_read != fsize) {
        printf("Error - expected bytes read equal to fsize\n");
        return NULL;
    }
    
    return_value->contents = buffer;
    return_value->size = (uint32_t)bytes_read;
    
    printf(
        "read file %s (%u bytes) ¥n¥n",
        filename,
        bytes_read);
    
    return return_value;
}

int main(int argc, char * argv[]) {
    NSString * appDelegateClassName;
    @autoreleasepool {
        // Setup code that might create autoreleased objects goes here.
        appDelegateClassName = NSStringFromClass([AppDelegate class]);
        
        return UIApplicationMain(argc, argv, nil, appDelegateClassName);
    }
}
