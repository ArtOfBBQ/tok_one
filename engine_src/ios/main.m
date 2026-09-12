#import <UIKit/UIKit.h>
#import "AppDelegate.h"

// #include "T1.h"

int main(int argc, char * argv[]) {
    
    NSString * appDelegateClassName;
    // Setup code that might create autoreleased objects goes here.
    appDelegateClassName = NSStringFromClass([AppDelegate class]);
    
    return UIApplicationMain(argc, argv, nil, appDelegateClassName);
}
