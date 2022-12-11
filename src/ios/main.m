#import <UIKit/UIKit.h>
#import "AppDelegate.h"

#include "../shared/init_application.h"
#include "../shared/common.h"
#include "../shared/logger.h"
#include "../shared/tok_random.h"
#include "../shared/lightsource.h"
#include "../shared_apple/gpu.h"
#include "../shared/userinput.h"
#include "../shared/window_size.h"
#include "../shared/clientlogic.h"

 // definitions of functions we need to implement
#include "../shared/platform_layer.h"

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
