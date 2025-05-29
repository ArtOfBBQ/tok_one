#import <UIKit/UIKit.h>
#import "AppDelegate.h"

#include "init_application.h"
#include "common.h"
#include "logger.h"
#include "tok_random.h"
#include "lightsource.h"
#include "shared_apple/gpu.h"
#include "userinput.h"
#include "window_size.h"
#include "clientlogic.h"

#include "apple_audio.h"

 // definitions of functions we need to implement
#include "platform_layer.h"


int main(int argc, char * argv[]) {
    
    application_running = true;
    has_retina_screen = ([[UIScreen mainScreen]
        respondsToSelector:@selector(displayLinkWithTarget:selector:)]
        && ([UIScreen mainScreen].scale >= 2.0));
    
    uint32_t init_success = 0;
    char err_msg[256];
    init_application_before_gpu_init(&init_success, err_msg);
    
    if (!init_success) {
        return 1;
    }
    
    // These can't be chosen on smartphones, so overwrite our preferred values
    // with the values the phone forces on us
    engine_globals->window_bottom = 0;
    engine_globals->window_left = 0;
    engine_globals->window_width =
        (float)[UIScreen mainScreen].bounds.size.width;
    engine_globals->window_height =
        (float)[UIScreen mainScreen].bounds.size.height;
    
    NSString * appDelegateClassName;
    // Setup code that might create autoreleased objects goes here.
    appDelegateClassName = NSStringFromClass([AppDelegate class]);
    
    return UIApplicationMain(argc, argv, nil, appDelegateClassName);
}
