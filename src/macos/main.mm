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

bool32_t application_running = true;
bool32_t has_retina_screen = true;

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
    
    buffer_mousedown(
        /* screenspace_x: */
            (float)screenspace_location.x,
        /* screenspace_y: */
            (float)screenspace_location.y);
}

- (void)mouseUp:(NSEvent *)event
{
    NSPoint screenspace_location = [NSEvent mouseLocation];
    
    buffer_mouseup(
        /* screenspace_x: */
            (float)screenspace_location.x,
        /* screenspace_y: */
            (float)screenspace_location.y);
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
    
    buffer_mousemove(
        /* screenspace_x: */
            (float)screenspace_location.x,
        /* screenspace_y: */
            (float)screenspace_location.y);
}

- (void)keyDown:(NSEvent *)event
{
    register_keydown(event.keyCode);
}

- (void)keyUp:(NSEvent *)event
{
    register_keyup(event.keyCode);
}
@end

int main(int argc, const char * argv[])
{
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
    
    NSScreen *screen = [[NSScreen screens] objectAtIndex:0];
    NSRect full_screen_rect = [screen frame]; 
        
    NSWindowWithCustomResponder *window =
        [[NSWindowWithCustomResponder alloc]
            initWithContentRect: full_screen_rect 
            styleMask: /*(NSWindowStyleMaskTitled
                         | NSWindowStyleMaskClosable
                         | */NSWindowStyleMaskFullScreen
            backing: NSBackingStoreBuffered 
            defer: NO];
    
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
    
    MTKView * mtk_view =
        [[MTKView alloc]
            initWithFrame: full_screen_rect
            device: metal_device];
    
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
