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
void platform_read_file(
    const char * filename,
    FileBuffer * out_preallocatedbuffer)
{
    printf("platform_read of filename: %s\n", filename);
    
    NSString * ns_filename =
        [NSString stringWithUTF8String:filename];
    
    NSString * file_part = [[ns_filename lastPathComponent] stringByDeletingPathExtension];
    NSString * extension_part = [ns_filename pathExtension];
    
    NSString * filepath =
        [[NSBundle mainBundle]
            pathForResource:file_part
            ofType:extension_part];
        
    FILE * rawfile = fopen(
        [filepath cStringUsingEncoding:NSUTF8StringEncoding],
        "rb");
    
    assert(rawfile != NULL);
    
    fseek(rawfile, 0, SEEK_END);
    unsigned long fsize = (unsigned long)ftell(rawfile);              
    fseek(rawfile, 0, SEEK_SET);
        
    size_t bytes_read = fread(
        /* ptr: */
            out_preallocatedbuffer->contents,
       /* size of each element to be read: */
            1,
       /* nmemb (no of members) to read: */
           fsize,
        /* stream: */
           rawfile);
    
    fclose(rawfile);
    if (bytes_read != fsize) {
        printf("Error - expected bytes read equal to fsize\n");
        return;
    }
    
    out_preallocatedbuffer->size = (uint32_t)bytes_read;
    
    printf(
        "read file %s (%zu bytes)\n",
        filename,
        bytes_read);
}

int64_t platform_get_filesize(const char * filename)
{
    printf("platform_read of filename: %s\n", filename);
    
    NSString * ns_filename =
        [NSString stringWithUTF8String:filename];
    
    NSString * file_part = [[ns_filename lastPathComponent] stringByDeletingPathExtension];
    NSString * extension_part = [ns_filename pathExtension];
    
    NSString * filepath =
        [[NSBundle mainBundle]
            pathForResource:file_part
            ofType:extension_part];
    
    FILE * rawfile = fopen(
        [filepath cStringUsingEncoding:NSUTF8StringEncoding],
        "rb");
    
    assert(rawfile != NULL);
    
    fseek(rawfile, 0, SEEK_END);
    unsigned long fsize = (unsigned long)ftell(rawfile);
    
    return fsize;
}

int main(int argc, char * argv[]) {
    NSString * appDelegateClassName;
    @autoreleasepool {
        // Setup code that might create autoreleased objects goes here.
        appDelegateClassName = NSStringFromClass([AppDelegate class]);
        
        return UIApplicationMain(argc, argv, nil, appDelegateClassName);
    }
}

