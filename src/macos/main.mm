#include <simd/simd.h>

// functions we must implement
#include "../shared/platform_layer.h" 

// shared functionality we can use
#include "../shared/common.h"
#include "../shared_apple/gpu.h"
#include "../shared/window_size.h"
#include "../shared/vertex_types.h"
#include "../shared/zpolygon.h"
#include "../shared/software_renderer.h"


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

- (void)keyDown:(NSEvent *)event
{
    switch (event.keyCode) {
        case 123:
            // left arrow key
            camera.x -= 2.5f;
            printf("new camera.x: %f\n", camera.x);
            break;
        case 124:
            // right arrow key
            camera.x += 2.5f;
            printf("new camera.x: %f\n", camera.x);
            break;
        case 125:
            // down arrow key
            camera.z -= 2.5f;
            printf("new camera.z: %f\n", camera.z);
            break;
        case 126:
            // up arrow kez
            camera.z += 2.5f;
            printf("new camera.z: %f\n", camera.z);
            break;
        default:
            NSLog(@"%hu",event.keyCode); 
            printf("unrecognized code\n");
            break;
    }
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
    
    id<MTLDevice> metal_device =
        MTLCreateSystemDefaultDevice();
    
    MTKView * mtk_view =
        [[MTKView alloc]
            initWithFrame: full_screen_rect
            device: metal_device];
    
    window.contentView = mtk_view;
    
    MetalKitViewDelegate *ViewDelegate =
        [[MetalKitViewDelegate alloc] init];
    [mtk_view setDelegate: ViewDelegate];
    
    NSString * shader_lib_path = @"Shaders.metallib";
    [ViewDelegate
        configureMetalWithDevice: metal_device
        andPixelFormat: mtk_view.colorPixelFormat
        fromFolder: shader_lib_path];
    
   
    // this cruft makes the app a "foreground application"
    // capable of accepting key events 
    ProcessSerialNumber psn = {0, kCurrentProcess};
    OSStatus status =
        TransformProcessType(
            &psn,
            kProcessTransformToForegroundApplication);

    // find the first responder class like so: 
    // NSLog(@"window first responder: %@", window.firstResponder);
    
    return NSApplicationMain(argc, argv);
}

