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

 // definitions of functions we need to implement
#include "platform_layer.h"

bool32_t application_running = true;
bool32_t has_retina_screen = false;


int main(int argc, char * argv[]) {
    
    has_retina_screen = ([[UIScreen mainScreen]
        respondsToSelector:@selector(displayLinkWithTarget:selector:)]
        && ([UIScreen mainScreen].scale >= 2.0));
    
    init_application();
    
    // These can't be chosen on smartphones, so overwrite our preferred values
    // with the values the phone forces on us
    window_globals->window_bottom = 0;
    window_globals->window_left = 0;
    window_globals->window_width =
        (float)[UIScreen mainScreen].bounds.size.width;
    window_globals->window_height =
        (float)[UIScreen mainScreen].bounds.size.height;
    
    NSString * appDelegateClassName;
    // Setup code that might create autoreleased objects goes here.
    appDelegateClassName = NSStringFromClass([AppDelegate class]);
    
    return UIApplicationMain(argc, argv, nil, appDelegateClassName);
}
