#include <simd/simd.h>

// functions we must implement
#include "../shared/platform_layer.h" 

// shared functionality we can use
#include "../shared_apple/gpu.h"
#include "../shared/common.h"
#include "../shared/userinput.h"
#include "../shared/window_size.h"
#include "../shared/vertex_types.h"
#include "../shared/zpolygon.h"
#include "../shared/software_renderer.h"
#include "../shared/bitmap_renderer.h"

#define SHARED_APPLE_PLATFORM

@interface
GameWindowDelegate: NSObject<NSWindowDelegate>
@end

@implementation GameWindowDelegate
- (void)windowWillClose:(NSNotification *)notification 
{
    [NSApp terminate: nil];
}
@end

@interface
NSWindowWithCustomResponder: NSWindow
@end

@implementation NSWindowWithCustomResponder
- (BOOL)acceptsFirstResponder
{
    return YES;
}

- (void)mouseDown:(NSEvent *)event
{
    printf("mouse down!\n");
}

- (void)mouseUp:(NSEvent *)event
{
    printf("mouse up!\n");
}

- (void)rightMouseDown:(NSEvent *)event
{
    printf("right mouse down!\n");
}

- (void)rightMouseUp:(NSEvent *)event
{
    printf("right mouse up!\n");
}

- (void)mouseMoved:(NSEvent *)event
{
    printf("mouse moved!\n");
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
    NSScreen *screen = [[NSScreen screens] objectAtIndex:0];
    NSRect full_screen_rect = [screen frame]; 
    
    window_height = NSHeight(full_screen_rect);
    window_width = NSWidth(full_screen_rect);
    
    init_projection_constants();
    init_renderer();
    
    NSWindowWithCustomResponder *window =
        [[NSWindowWithCustomResponder alloc]
            initWithContentRect: full_screen_rect 
            styleMask: (NSWindowStyleMaskTitled
                        | NSWindowStyleMaskClosable
                         | NSWindowStyleMaskFullScreen)
            backing: NSBackingStoreBuffered 
            defer: NO];
    
    GameWindowDelegate *window_delegate =
        [[GameWindowDelegate alloc] init];
    
    [window setDelegate: window_delegate];
    [window setTitle: @"Hello, 3dgfx!"];
    [window makeKeyAndOrderFront: nil];
    [window setAcceptsMouseMovedEvents:YES];
    
    id<MTLDevice> metal_device =
        MTLCreateSystemDefaultDevice();
    
    MTKView * mtk_view =
        [[MTKView alloc]
            initWithFrame: full_screen_rect
            device: metal_device];

    [mtk_view setOpaque: NO];
    // mtk_view.opaque = false;
    // mtk_view.preferredFramesPerSecond = 60;
    
    window.contentView = mtk_view;
    
    apple_gpu_delegate =
        [[MetalKitViewDelegate alloc] init];
    [mtk_view setDelegate: apple_gpu_delegate];
    
    NSString * shader_lib_path = @"Shaders.metallib";
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
    
    // find the first responder class like so: 
    // NSLog(@"window first responder: %@", window.firstResponder);
    
    return NSApplicationMain(argc, argv);
}

