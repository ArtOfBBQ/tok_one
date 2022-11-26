#include "../shared/init_application.h"
#include "../shared/common.h"
#include "../shared/logger.h"
#include "../shared/tok_random.h"
#include "../shared/lightsource.h"
#include "../shared_apple/gpu.h"
#include "../shared/userinput.h"
#include "../shared/window_size.h"
#include "../shared/clientlogic.h"

#define SHARED_APPLE_PLATFORM

/*
these variables may not exist on platforms where window resizing is
impossible
*/
float current_window_height = 500;
float current_window_width = 800;
bool32_t startup_complete = false; // dont trigger window resize code at startup

MTKView * mtk_view = NULL; 

@interface
GameWindowDelegate: NSObject<NSWindowDelegate>
@end

@implementation GameWindowDelegate
- (void)windowWillClose:(NSNotification *)notification {
    log_append("window will close, terminating app..\n");
    
    client_logic_shutdown();
    
    add_profiling_stats_to_log();
    bool32_t write_succesful = false;
    log_dump(&write_succesful);
    
    if (!write_succesful) {
        log_append("ERROR - failed to store the log file on app termination..\n");
    }
    
    [NSApp terminate: nil];
}

- (NSSize)
    windowWillResize:(NSWindow *)sender 
    toSize:(NSSize)frameSize
{
    float title_bar_height =
        (float)([sender contentRectForFrameRect: sender.frame].size.height
            - sender.frame.size.height);
    current_window_height = (float)(frameSize.height + title_bar_height);
    current_window_width = (float)frameSize.width;
    
    if (!startup_complete) { return frameSize; }
    
    last_resize_request_at = platform_get_current_time_microsecs();
    
    NSSize new_size = frameSize;
    new_size.height += title_bar_height;
    new_size.height *= 2;
    new_size.width *= 2;
    mtk_view.drawableSize = new_size;
    
    texquads_to_render_size = 0;
    zpolygons_to_render_size = 0; 
    zlights_to_apply_size = 0;
    // [apple_gpu_delegate drawClearScreen: mtk_view];
    
    return frameSize;
}
@end

@interface
NSWindowWithCustomResponder: NSWindow
@end

@implementation NSWindowWithCustomResponder
- (BOOL)canBecomeKeyWindow {
    return YES;
}

- (BOOL)canBecomeMainWindow {
    return YES;
}

- (BOOL)acceptsFirstResponder
{
    return YES;
}

- (void)mouseDown:(NSEvent *)event
{
    NSPoint screenspace_location = [NSEvent mouseLocation];
    
    register_interaction(
        /* interaction : */
            &previous_leftclick_start,
        /* screenspace_x: */
            (float)screenspace_location.x
                - platform_get_current_window_left(),
        /* screenspace_y: */
            (float)screenspace_location.y
                - platform_get_current_window_bottom());
    
    register_interaction(
        /* interaction : */
            &previous_touch_or_leftclick_start,
        /* screenspace_x: */
            (float)screenspace_location.x
                - platform_get_current_window_left(),
        /* screenspace_y: */
            (float)screenspace_location.y
                - platform_get_current_window_bottom());
}

- (void)mouseUp:(NSEvent *)event
{
    NSPoint screenspace_location = [NSEvent mouseLocation];
    
    register_interaction(
        /* interaction : */
            &previous_leftclick_end,
        /* screenspace_x: */
            (float)screenspace_location.x - platform_get_current_window_left(),
        /* screenspace_y: */
            (float)screenspace_location.y
                - platform_get_current_window_bottom());
    
    register_interaction(
        /* interaction : */
            &previous_touch_or_leftclick_end,
        /* screenspace_x: */
            (float)screenspace_location.x
                - platform_get_current_window_left(),
        /* screenspace_y: */
            (float)screenspace_location.y
                - platform_get_current_window_bottom());
}

- (void)rightMouseDown:(NSEvent *)event
{
    log_append("unhandled mouse event\n");
    
    // NSPoint screenspace_location = [NSEvent mouseLocation];
}

- (void)rightMouseUp:(NSEvent *)event
{
    log_append("unhandled mouse event\n");
}

- (void)mouseMoved:(NSEvent *)event {
    NSPoint screenspace_location = [NSEvent mouseLocation];
    
    register_interaction(
        /* interaction : */
            &previous_mouse_move,
        /* screenspace_x: */
            (float)screenspace_location.x
                - platform_get_current_window_left(),
        /* screenspace_y: */
            (float)screenspace_location.y
                - platform_get_current_window_bottom());
    
    register_interaction(
        /* interaction : */
            &previous_mouse_or_touch_move,
        /* screenspace_x: */
            (float)screenspace_location.x
                - platform_get_current_window_left(),
        /* screenspace_y: */
            (float)screenspace_location.y
                - platform_get_current_window_bottom());
}

- (void)mouseDragged:(NSEvent *)event {
    NSPoint screenspace_location = [NSEvent mouseLocation];
    
    register_interaction(
        /* interaction : */
            &previous_mouse_move,
        /* screenspace_x: */
            (float)screenspace_location.x
                - platform_get_current_window_left(),
        /* screenspace_y: */
            (float)screenspace_location.y
                - platform_get_current_window_bottom());
    
    register_interaction(
        /* interaction : */
            &previous_mouse_or_touch_move,
        /* screenspace_x: */
            (float)screenspace_location.x
                - platform_get_current_window_left(),
        /* screenspace_y: */
            (float)screenspace_location.y
                - platform_get_current_window_bottom());
}


- (void)keyDown:(NSEvent *)event {
    register_keydown(event.keyCode);
}

- (void)keyUp:(NSEvent *)event {
    register_keyup(event.keyCode);
}
@end

bool32_t application_running = true;
bool32_t has_retina_screen = true;

NSWindowWithCustomResponder * window = NULL;

float platform_get_current_window_left() {
    return (float)window.frame.origin.x;
}

float platform_get_current_window_bottom() {
    return (float)window.frame.origin.y;
}

int main(int argc, const char * argv[]) {
    
    client_logic_get_application_name_to_recipient(
        /* recipient: */ application_name,
        /* recipient_size: */ 100);
    
    init_application();
    log_append("initialized application: ");
    log_append(application_name);
    
    log_append("\nconfirming we can save debug info - writing log.txt...\n");
    bool32_t initial_log_dump_succesful = false;
    log_dump(&initial_log_dump_succesful);
    if (!initial_log_dump_succesful) {
        log_append(
            "Error - can't write to log.txt on boot, terminating app...\n");
        return 1;
    }
    
    // NSScreen *screen = [[NSScreen screens] objectAtIndex:0];
    NSRect window_rect = NSMakeRect(
        /* x: */ 200,
        /* y: */ 250,
        /* width: */ platform_get_current_window_width(),
        /* height: */ platform_get_current_window_height());
    
    window =
        [[NSWindowWithCustomResponder alloc]
            initWithContentRect: window_rect 
            styleMask: NSWindowStyleMaskTitled
                         | NSWindowStyleMaskClosable
                         | NSWindowStyleMaskResizable
            backing: NSBackingStoreBuffered 
            defer: NO];
    window.animationBehavior = NSWindowAnimationBehaviorNone;
    
    GameWindowDelegate * window_delegate =
        [[GameWindowDelegate alloc] init];
    
    NSString * nsstring_app_name = [NSString stringWithUTF8String:application_name];  
    [window setDelegate: window_delegate];
    [window setTitle: nsstring_app_name];
    [window makeMainWindow]; 
    [window setAcceptsMouseMovedEvents:YES];
    [window setOrderedIndex:0];
    [window makeKeyAndOrderFront: nil];
    
    id<MTLDevice> metal_device = MTLCreateSystemDefaultDevice();
    
    mtk_view = [[MTKView alloc]
        initWithFrame: window_rect
        device: metal_device];
    
    mtk_view.autoResizeDrawable = false;
    
    // [mtk_view setOpaque: NO];
    // mtk_view.opaque = false;
    // mtk_view.preferredFramesPerSecond = 60;
    mtk_view.enableSetNeedsDisplay = false;
    
    // Indicate that each pixel in the depth buffer is a 32-bit floating point value.
    mtk_view.depthStencilPixelFormat = MTLPixelFormatDepth32Float;
    
    // Indicate that Metal should clear all values in the depth buffer to `1.0` when you create
    // a render command encoder with the MetalKit view's `currentRenderPassDescriptor` property.
    mtk_view.clearDepth = 1.0f;
    [mtk_view setPaused: false];
    
    window.contentView = mtk_view;
    
    apple_gpu_delegate = [[MetalKitViewDelegate alloc] init];
    [mtk_view setDelegate: apple_gpu_delegate];
    
    char shader_lib_path_cstr[1000];
    platform_get_resources_path(shader_lib_path_cstr, 1000);
    strcat_capped(
        shader_lib_path_cstr,
        1000,
        "/Shaders.metallib");
    
    NSString * shader_lib_path =
        [NSString
            stringWithCString:shader_lib_path_cstr
            encoding:NSASCIIStringEncoding];
    
    [apple_gpu_delegate
        configureMetalWithDevice: metal_device
        andPixelFormat: mtk_view.colorPixelFormat
        fromFolder: shader_lib_path];
    
    startup_complete = true;
    
    return NSApplicationMain(argc, argv);
}
