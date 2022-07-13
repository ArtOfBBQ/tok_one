#import <UIKit/UIKit.h>
#import "AppDelegate.h"

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
bool32_t has_retina_screen =
    ([[UIScreen mainScreen]
        respondsToSelector:@selector(displayLinkWithTarget:selector:)]
    && ([UIScreen mainScreen].scale >= 2.0));

int main(int argc, char * argv[]) {
    unmanaged_memory = (uint8_t *)malloc(UNMANAGED_MEMORY_SIZE);
    managed_memory = (uint8_t *)malloc(MANAGED_MEMORY_SIZE);
    
    setup_log();
    log_append("Lore seeker for iOS started running\n");
    
    // initialize font with fontmetrics.dat
    FileBuffer font_metrics_file;
    font_metrics_file.size = platform_get_resource_size(
        /* filename: */ "fontmetrics.dat");
    log_assert(font_metrics_file.size > 0);
    
    font_metrics_file.contents = (char *)malloc_from_unmanaged(
        font_metrics_file.size);
    platform_read_resource_file(
        /* const char * filepath: */
            "fontmetrics.dat",
        /* FileBuffer * out_preallocatedbuffer: */
            &font_metrics_file);
    log_assert(font_metrics_file.good);
    init_font(
        /* raw_fontmetrics_file_contents: */ font_metrics_file.contents,
        /* raw_fontmetrics_file_size: */ font_metrics_file.size);
    
    zlights_to_apply = (zLightSource *)malloc_from_unmanaged(
        sizeof(zLightSource) * ZLIGHTS_TO_APPLY_ARRAYSIZE);
    zlights_transformed = (zLightSource *)malloc_from_unmanaged(
        sizeof(zLightSource) * ZLIGHTS_TO_APPLY_ARRAYSIZE);
    texquads_to_render = (TexQuad *)malloc_from_unmanaged(
        sizeof(TexQuad) * TEXQUADS_TO_RENDER_ARRAYSIZE);
    
    NSString * appDelegateClassName;
    @autoreleasepool {
        // Setup code that might create autoreleased objects goes here.
        appDelegateClassName = NSStringFromClass([AppDelegate class]);
        
        return UIApplicationMain(argc, argv, nil, appDelegateClassName);
    }
}
