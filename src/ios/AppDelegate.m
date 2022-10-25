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
    application_running = false;
    client_logic_shutdown();
    
    add_profiling_stats_to_log();
    bool32_t write_succesful = false;
    log_dump(&write_succesful);
    log_append("ERROR - failed to store the log file on app termination..\n");
}

@end
