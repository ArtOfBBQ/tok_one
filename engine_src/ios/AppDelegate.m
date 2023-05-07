#import "AppDelegate.h"

@interface AppDelegate ()

@end

@implementation AppDelegate
- (BOOL)
    application:(UIApplication *)application
    didFinishLaunchingWithOptions:(NSDictionary *)launchOptions
{
    return YES;
}

- (void)
    applicationWillTerminate:(UIApplication *)application
{
    shared_shutdown_application();
    
    client_logic_shutdown();
    
    bool32_t write_succesful = false;
    log_dump(&write_succesful);
    log_append("ERROR - failed to store the log file on app termination..\n");
}

@end
