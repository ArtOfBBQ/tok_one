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
float current_window_height = 600;
float current_window_width = 300;

MTKView * mtk_view = NULL; 

@interface
GameWindowDelegate: NSObject<NSWindowDelegate>
@end

@implementation GameWindowDelegate
- (void)windowWillClose:(NSNotification *)notification 
{
    log_append("window will close, terminating app..\n");
    add_profiling_stats_to_log();
    log_dump();
    
    [NSApp terminate: nil];
}

- (NSSize)
    windowWillResize:(NSWindow *)sender 
    toSize:(NSSize)frameSize
{
    last_resize_request_at = platform_get_current_time_microsecs();
    float title_bar_height =
        [sender contentRectForFrameRect: sender.frame].size.height
            - sender.frame.size.height;
    
    current_window_height = frameSize.height + title_bar_height;
    current_window_width = frameSize.width;
    
    NSSize new_size = frameSize;
    new_size.height += title_bar_height;
    new_size.height *= 2;
    new_size.width *= 2;
    mtk_view.drawableSize = new_size;
    
    texquads_to_render_size = 0;
    zpolygons_to_render_size = 0; 
    zlights_to_apply_size = 0;
    [apple_gpu_delegate drawClearScreen: mtk_view];
    
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

    printf(
        "mouse down at screenspace: %f, %f\n",
        (float)screenspace_location.x,
        (float)screenspace_location.y);
    
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

- (void)mouseMoved:(NSEvent *)event
{
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
    return window.frame.origin.x;
}

float platform_get_current_window_bottom() {
    return window.frame.origin.y;
}

int main(int argc, const char * argv[]) {
    init_application();
    
    @autoreleasepool {
    log_append("started application: ");
    log_append(application_name);
    log_append("\nallocated unmanaged memory: ");
    log_append_uint(UNMANAGED_MEMORY_SIZE);
    log_append("\nallocated managed memory: ");
    log_append_uint(MANAGED_MEMORY_SIZE);
    log_append("\nconfirming we can save debug info - writing log.txt...\n");
    log_dump();
    
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
    
    GameWindowDelegate *window_delegate =
        [[GameWindowDelegate alloc] init];
    
    [window setDelegate: window_delegate];
    [window setTitle: NSLocalizedString(@"Lore Seeker", @"")];
    [window makeMainWindow]; 
    [window setAcceptsMouseMovedEvents:YES];
    [window setOrderedIndex:0];
    [window makeKeyAndOrderFront: nil];
    
    id<MTLDevice> metal_device =
        MTLCreateSystemDefaultDevice();
    
    mtk_view =
        [[MTKView alloc]
            initWithFrame: window_rect
            device: metal_device];
    
    mtk_view.autoResizeDrawable = false;
    
    // [mtk_view setOpaque: NO];
    // mtk_view.opaque = false;
    // mtk_view.preferredFramesPerSecond = 60;
    
    window.contentView = mtk_view;
    
    apple_gpu_delegate =
        [[MetalKitViewDelegate alloc] init];
    [mtk_view setDelegate: apple_gpu_delegate];
    
    char shader_lib_path_cstr[2000];
    concat_strings(
        /* string_1: */ platform_get_resources_path(),
        /* string_2: */ "/Shaders.metallib",
        /* output: */ shader_lib_path_cstr,
        /* output_size: */ 2000);
    
    NSString * shader_lib_path =
        [NSString
            stringWithCString:shader_lib_path_cstr
            encoding:NSASCIIStringEncoding];
    
    [apple_gpu_delegate
        configureMetalWithDevice: metal_device
        andPixelFormat: mtk_view.colorPixelFormat
        fromFolder: shader_lib_path];
    
    // this cruft makes the app a "foreground application"
    // capable of accepting key events 
    ProcessSerialNumber psn = {0, kCurrentProcess};
    // OSStatus status =
    TransformProcessType(
        &psn,
        kProcessTransformToForegroundApplication);
    }
    
    return NSApplicationMain(argc, argv);
}

