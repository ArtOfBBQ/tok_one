#import <UIKit/UIKit.h>
#import "AppDelegate.h"

#include "stdlib.h"
#include "window_size.h"
#include "vertex_types.h"
#include "software_renderer.h"
#include "gpu.h"
#include "box.h"

int main(int argc, char * argv[]) {
    printf("running int main()...\n");
    
    NSString * appDelegateClassName;
    @autoreleasepool {
        // Setup code that might create autoreleased objects goes here.
        appDelegateClassName = NSStringFromClass([AppDelegate class]);
        
        return UIApplicationMain(argc, argv, nil, appDelegateClassName);
    }
}
