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
    
    client_logic_get_application_name_to_recipient(
        /* recipient: */ application_name,
        /* recipient_size: */ 100);
    
    init_application();
    
    NSString * appDelegateClassName;
    // Setup code that might create autoreleased objects goes here.
    appDelegateClassName = NSStringFromClass([AppDelegate class]);
        
    return UIApplicationMain(argc, argv, nil, appDelegateClassName);
}
