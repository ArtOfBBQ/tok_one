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
#include "box.h"

/*
This functionality must be provided by the platform because
of iOS, where reading your own app's files is a security
ordeal
*/
FileBuffer * platform_read_file(char * filename) {
    
    NSString * ns_filename =
        [NSString stringWithUTF8String:filename];
    printf(
        "ns_filename: %s\n",
        [ns_filename cStringUsingEncoding:NSUTF8StringEncoding]);
    
    NSString * file_part = [[ns_filename lastPathComponent] stringByDeletingPathExtension];
    NSString * extension_part = [ns_filename pathExtension];
    printf("filepath split: %s /// %s\n",
        [file_part cStringUsingEncoding:NSUTF8StringEncoding],
        [extension_part cStringUsingEncoding:NSUTF8StringEncoding]);
    NSString * filepath =
        [[NSBundle mainBundle]
            pathForResource:file_part
            ofType:extension_part];
    
    printf(
        "hardcoded filepath: %s\n",
        [[[NSBundle mainBundle]
            pathForResource:@"teddybear"
            ofType: @"obj"]
                cStringUsingEncoding: NSUTF8StringEncoding]);
    
    printf(
        "filepath: %s\n",
        [filepath cStringUsingEncoding:NSUTF8StringEncoding]);
    
    FileBuffer * return_value =
        malloc(sizeof(FileBuffer));
    
    FILE * modelfile = fopen(
        [filepath cStringUsingEncoding:NSUTF8StringEncoding],
        "rb");
    
    assert(modelfile != NULL);
    
    fseek(modelfile, 0, SEEK_END);
    unsigned long fsize = (unsigned long)ftell(modelfile);              
    fseek(modelfile, 0, SEEK_SET);
    
    char * buffer = malloc(fsize);
    
    size_t bytes_read = fread(
        /* ptr: */
            buffer,
       /* size of each element to be read: */
            1,
       /* nmemb (no of members) to read: */
           fsize,
        /* stream: */
           modelfile);
   
    fclose(modelfile);
        if (bytes_read != fsize) {
            printf("Error - expected bytes read equal to fsize\n");
            return NULL;
        }
        
        return_value->contents = buffer;
        return_value->size = bytes_read;
        
        return return_value;
}

int main(int argc, char * argv[]) {
    printf("running int main()...\n");
    
    NSString * appDelegateClassName;
    @autoreleasepool {
        // Setup code that might create autoreleased objects goes here.
        appDelegateClassName = NSStringFromClass([AppDelegate class]);
        
        return UIApplicationMain(argc, argv, nil, appDelegateClassName);
    }
}
