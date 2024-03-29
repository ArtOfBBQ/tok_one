#include "gpu.h"
#include "apple_audio.h"

#include "init_application.h"
#include "common.h"
#include "logger.h"
#include "tok_random.h"
#include "lightsource.h"
#include "userinput.h"
#include "window_size.h"
#include "simd.h"
#include "clientlogic.h"

#define SHARED_APPLE_PLATFORM

#if __cplusplus
extern "C" {
#endif

static uint32_t apple_keycode_to_tokone_keycode(const uint32_t apple_key)
{
    #ifndef LOGGER_IGNORE_ASSERTS
    char err_msg[128];
    #endif
    
    switch (apple_key) {
        case (24):
            return TOK_KEY_HAT;
        case (29):
            return TOK_KEY_0;
        case (27):
            return TOK_KEY_MINUS;
        case (36):
            return TOK_KEY_ENTER;
        case (123):
            return TOK_KEY_LEFTARROW;
        case (124):
            return TOK_KEY_RIGHTARROW;
        case (125):
            return TOK_KEY_DOWNARROW;
        case (126):
            return TOK_KEY_UPARROW;
        case (18):
            return TOK_KEY_1;
        case (19):
            return TOK_KEY_2;
        case (20):
            return TOK_KEY_3;
        case (21):
            return TOK_KEY_4;
        case (23):
            return TOK_KEY_5;
        case (22):
            return TOK_KEY_6;
        case (26):
            return TOK_KEY_7;
        case (28):
            return TOK_KEY_8;
        case (25):
            return TOK_KEY_9;
        case (0):
            return TOK_KEY_A;
        case (1):
            return TOK_KEY_S;
        case (2):
            return TOK_KEY_D;
        case (3):
            return TOK_KEY_F;
        case (4):
            return TOK_KEY_H;
        case (5):
            return TOK_KEY_G;
        case (6):
            return TOK_KEY_Z;
        case (7):
            return TOK_KEY_X;
        case (8):
            return TOK_KEY_C;
        case (9):
            return TOK_KEY_V;
        case (12):
            return TOK_KEY_Q;
        case (13):
            return TOK_KEY_W;
        case (14):
            return TOK_KEY_E;
        case (15):
            return TOK_KEY_R;
        case (17):
            return TOK_KEY_T;
        case (16):
            return TOK_KEY_Y;
        case (32):
            return TOK_KEY_U;
        case (34):
            return TOK_KEY_I;
        case (31):
            return TOK_KEY_O;
        case (35):
            return TOK_KEY_P;
        case (37):
            return TOK_KEY_L;
        case (38):
            return TOK_KEY_J;
        case (40):
            return TOK_KEY_K;
        case (41):
            return TOK_KEY_SEMICOLON;
        case (39):
            return TOK_KEY_COLON;
        case (30):
            return TOK_KEY_OPENSQUAREBRACKET;
        case (33):
            return TOK_KEY_AT;
        case (42):
            return TOK_KEY_CLOSESQUAREBRACKET;
        case (11):
            return TOK_KEY_B;
        case (45):
            return TOK_KEY_N;
        case (46):
            return TOK_KEY_M;
        case (43):
            return TOK_KEY_COMMA;
        case (47):
            return TOK_KEY_FULLSTOP;
        case (44):
            return TOK_KEY_BACKSLASH;
        case (49):
            return TOK_KEY_SPACEBAR;
        case (51):
            return TOK_KEY_BACKSPACE;
        case (53):
            return TOK_KEY_ESCAPE;
        case (93):
            return TOK_KEY_YENSIGN;
        case (94):
            return TOK_KEY_UNDERSCORE;
        case (102):
            return TOK_KEY_ROMAJIBUTTON;
        case (104):
            return TOK_KEY_KANABUTTON;
        default:
            #ifndef LOGGER_IGNORE_ASSERTS
            strcpy_capped(err_msg, 128, "unhandled apple keycode: ");
            strcat_uint_capped(err_msg, 128, apple_key);
            strcat_capped(err_msg, 128, "\n");
            #endif
            break;
    }
    
    #ifndef LOGGER_IGNORE_ASSERTS
    log_dump_and_crash(err_msg);
    #endif
    
    return TOK_KEY_ESCAPE;
}

/*
these variables may not exist on platforms where window resizing is
impossible
*/
bool32_t startup_complete = false; // dont trigger window resize code at startup

MTKView * mtk_view = NULL;

@interface
GameWindowDelegate: NSObject<NSWindowDelegate>
@end

@implementation GameWindowDelegate

- (void)windowWillClose:(NSNotification *)notification {
    log_append("window will close, terminating app..\n");
    
    shared_shutdown_application();
    
    client_logic_shutdown();
    
    bool32_t write_succesful = false;
    log_dump(&write_succesful);
    
    if (!write_succesful) {
        log_append("ERROR - failed to store the log file on app termination..\n");
    }
    
    platform_close_application();
}

- (void)
    windowWillEnterFullScreen:(NSNotification *)notification
{
    delete_all_ui_elements();
    zpolygons_to_render->size = 0;
    particle_effects_size = 0;
    window_globals->fullscreen = true;
}

- (void)
    windowWillExitFullScreen:(NSNotification *)notification
{
    delete_all_ui_elements();
    zpolygons_to_render->size = 0;
    particle_effects_size = 0;
    window_globals->fullscreen = false;
}

- (void)windowDidMove:(NSNotification *)notification
{
    update_window_position(
        (float)(((NSWindow *)[notification object]).frame.origin.x),
        (float)(((NSWindow *)[notification object]).frame.origin.y));
}

- (NSSize)
    windowWillResize:(NSWindow *)sender
    toSize:(NSSize)frameSize
{
    if (!startup_complete)
    {
        return frameSize;
    }
    
    delete_all_ui_elements();
    zpolygons_to_render->size = 0;
    particle_effects_size = 0;
    
    return frameSize;
}

- (void)windowDidResize:(NSNotification *)notification {
    if (!startup_complete) {
        return;
    }
    
    NSSize size = [mtk_view frame].size;
    
    update_window_size(
        /* width: */
            (float)size.width,
        /* height: */
            (float)size.height,
        /* at_timestamp_microsecs: */
            platform_get_current_time_microsecs());
    
    [apple_gpu_delegate updateViewport];
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
    @autoreleasepool {
    NSPoint window_location = [event locationInWindow];
        
    register_interaction(
        /* interaction : */
            &user_interactions[INTR_PREVIOUS_LEFTCLICK_START],
        /* screenspace_x: */
            (float)window_location.x,
        /* screenspace_y: */
            (float)window_location.y);
    
    user_interactions[INTR_PREVIOUS_TOUCH_OR_LEFTCLICK_START] =
        user_interactions[INTR_PREVIOUS_LEFTCLICK_START];
    user_interactions[INTR_PREVIOUS_MOUSE_MOVE] =
        user_interactions[INTR_PREVIOUS_LEFTCLICK_START];
    user_interactions[INTR_PREVIOUS_MOUSE_OR_TOUCH_MOVE] =
        user_interactions[INTR_PREVIOUS_LEFTCLICK_START];
    
    user_interactions[INTR_PREVIOUS_TOUCH_END].handled = true;
    user_interactions[INTR_PREVIOUS_LEFTCLICK_END].handled = true;
    user_interactions[INTR_PREVIOUS_TOUCH_OR_LEFTCLICK_END].handled = true;
    }
}

- (void)mouseUp:(NSEvent *)event
{
    @autoreleasepool {
    NSPoint window_location = [event locationInWindow];
    
    register_interaction(
        /* interaction : */
            &user_interactions[INTR_PREVIOUS_LEFTCLICK_END],
        /* screenspace_x: */
            (float)window_location.x,
        /* screenspace_y: */
            (float)window_location.y);
    
    user_interactions[INTR_PREVIOUS_TOUCH_OR_LEFTCLICK_END] =
        user_interactions[INTR_PREVIOUS_LEFTCLICK_END];
    user_interactions[INTR_PREVIOUS_MOUSE_MOVE] =
        user_interactions[INTR_PREVIOUS_LEFTCLICK_END];
    user_interactions[INTR_PREVIOUS_MOUSE_OR_TOUCH_MOVE] =
        user_interactions[INTR_PREVIOUS_LEFTCLICK_END];
    }
}

- (void)rightMouseDown:(NSEvent *)event
{
    log_append("unhandled mouse event\n");    
}

- (void)rightMouseUp:(NSEvent *)event
{
    log_append("unhandled mouse event\n");
}

- (void)mouseMoved:(NSEvent *)event {
    @autoreleasepool {
    NSPoint window_location = [event locationInWindow];
    
    register_interaction(
        /* interaction : */
            &user_interactions[INTR_PREVIOUS_MOUSE_MOVE],
        /* screenspace_x: */
            (float)window_location.x,
        /* screenspace_y: */
            (float)window_location.y);
    
    user_interactions[INTR_PREVIOUS_MOUSE_OR_TOUCH_MOVE] =
        user_interactions[INTR_PREVIOUS_MOUSE_MOVE];
    }
}

- (void)mouseDragged:(NSEvent *)event {
    @autoreleasepool {
    NSPoint window_location = [event locationInWindow];
    
    register_interaction(
        /* interaction : */
            &user_interactions[INTR_PREVIOUS_MOUSE_MOVE],
        /* screenspace_x: */
            (float)window_location.x,
        /* screenspace_y: */
            (float)window_location.y);
    
    user_interactions[INTR_PREVIOUS_MOUSE_OR_TOUCH_MOVE] =
        user_interactions[INTR_PREVIOUS_MOUSE_MOVE];
    }
}


- (void)keyDown:(NSEvent *)event {
    register_keydown(apple_keycode_to_tokone_keycode(event.keyCode));
}

- (void)keyUp:(NSEvent *)event {
    register_keyup(apple_keycode_to_tokone_keycode(event.keyCode));
}
@end

NSWindowWithCustomResponder * window = NULL;

bool32_t application_running = true;
bool32_t has_retina_screen = true;

int main(int argc, const char * argv[]) {
    
    assert(sizeof(GPUPolygon) % 32 == 0);
    
    init_application_before_gpu_init();
    log_append("initialized application: ");
    log_append(APPLICATION_NAME);
    
    log_append(
        "\nconfirming we can save debug info - writing log.txt...\n");
    bool32_t initial_log_dump_succesful = false;
    log_dump(&initial_log_dump_succesful);
    if (!initial_log_dump_succesful) {
        log_append(
            "Error - can't write to log.txt, terminating app...\n");
        return 1;
    }
    
    #ifdef __AVX__
    log_append("Platform CPU supports Intel AVX instructions...");
    #else
    log_append("Platform CPU does not support Intel AVX instructions...");
    #endif
    
    // NSScreen *screen = [[NSScreen screens] objectAtIndex:0];
    NSRect window_rect = NSMakeRect(
        /* x: */ window_globals->window_left,
        /* y: */ window_globals->window_bottom,
        /* width: */ window_globals->window_width,
        /* height: */ window_globals->window_height);
    
    window =
        [[NSWindowWithCustomResponder alloc]
            initWithContentRect: window_rect 
            styleMask:
                NSWindowStyleMaskTitled    |
                NSWindowStyleMaskClosable  |
                NSWindowStyleMaskResizable
            backing: NSBackingStoreBuffered 
            defer: NO];
    window.animationBehavior = NSWindowAnimationBehaviorNone;
    
    GameWindowDelegate * window_delegate =
        [[GameWindowDelegate alloc] init];
    
    NSString * nsstring_app_name =
        [NSString stringWithUTF8String:APPLICATION_NAME];
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
    
    mtk_view.autoResizeDrawable = true;
    
    // [mtk_view setOpaque: NO];
    // mtk_view.opaque = false;
    // mtk_view.preferredFramesPerSecond = 60;
    mtk_view.enableSetNeedsDisplay = false;
    
    // Indicate that each pixel in the depth buffer is a 32-bit floating point
    // value.
    mtk_view.depthStencilPixelFormat = MTLPixelFormatDepth32Float;
    
    // Indicate that Metal should clear all values in the depth buffer to x
    // when you create a render command encoder with the MetalKit view's
    // `currentRenderPassDescriptor` property.
    mtk_view.clearDepth = CLEARDEPTH;
    [mtk_view setPaused: false];
    [mtk_view setNeedsDisplay: false];
    
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
    
    
    init_application_after_gpu_init();
    
    start_audio_loop();
    
    startup_complete = true;
    
    @autoreleasepool {
        return NSApplicationMain(argc, argv);
    }
}

void platform_enter_fullscreen(void) {
    [window toggleFullScreen: window];
}

#if __cplusplus
}
#endif
