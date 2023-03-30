#include "shared/init_application.h"
#include "shared/common.h"
#include "shared/logger.h"
#include "shared/tok_random.h"
#include "shared/lightsource.h"
#include "shared_apple/gpu.h"
#include "shared/userinput.h"
#include "shared/window_size.h"
#include "shared/simd.h"

#include "clientlogic.h"


#define SHARED_APPLE_PLATFORM

static uint32_t apple_keycode_to_tokone_keycode(const uint32_t apple_key)
{
    char err_msg[128];
    
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
            strcpy_capped(err_msg, 128, "unhandled apple keycode: ");
            strcat_uint_capped(err_msg, 128, apple_key);
            strcat_capped(err_msg, 128, "\n");
    }
    
    log_dump_and_crash(err_msg);
    
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
NSSize last_nonfull_screen_size;
NSSize last_nonfull_drawable_size;

- (void)windowWillClose:(NSNotification *)notification {
    log_append("window will close, terminating app..\n");
    
    client_logic_shutdown();
    
    bool32_t write_succesful = false;
    log_dump(&write_succesful);
    
    if (!write_succesful) {
        log_append("ERROR - failed to store the log file on app termination..\n");
    }
    
    platform_close_application();
}

// This allows you to override the full screen size
- (NSSize)
    window:(NSWindow *)window
    willUseFullScreenContentSize:(NSSize)proposedSize
{
    log_append("willuseFullScreenContentSize: ");
    log_append_uint((uint32_t)proposedSize.height);
    log_append(", ");
    log_append_uint((uint32_t)proposedSize.width);
    log_append_char('\n');
        
    [self
        handleResizeTo: proposedSize
        withTitleBarHeight: 0];
    
    return proposedSize;
}

// WindowWillResize() also triggers after this 
// the order always seems the same on my computer
- (void)
    windowWillEnterFullScreen:(NSNotification *)notification
{
    log_append("window will enter full screen\n");
    // I'm stashing this info because NSEvents won't give the correct
    // info when entering full screen and then going back to window mode
    last_nonfull_screen_size.width = window_globals->window_width;
    last_nonfull_screen_size.height = window_globals->window_height;
    last_nonfull_drawable_size = mtk_view.drawableSize;
    log_append("cached for next window minimization: ");
    log_append_uint((uint32_t)last_nonfull_screen_size.height);
    log_append_char(',');
    log_append_uint((uint32_t)last_nonfull_screen_size.width);
    log_append_char('\n');
    log_append("cached for next mtkview drawable minimization: ");
    log_append_uint((uint32_t)last_nonfull_drawable_size.height);
    log_append_char(',');
    log_append_uint((uint32_t)last_nonfull_drawable_size.width);
    log_append_char('\n');
    
    zpolygons_to_render_size = 0;
}

- (void)
    handleResizeTo: (NSSize)size
    withTitleBarHeight: (float)title_bar_height
{
    zpolygons_to_render_size = 0;
    
    window_globals->window_height =
        (float)(size.height + title_bar_height);
    window_globals->window_width =
        (float)size.width;
    
    NSSize new_size = size;
    new_size.height += title_bar_height;
    new_size.height *= 2;
    new_size.width *= 2;
    mtk_view.drawableSize = new_size;
    
    window_globals->last_resize_request_at =
        platform_get_current_time_microsecs();
}

// WindowWillResize() doesn't seem to trigger after this consistently
- (void)
    windowWillExitFullScreen:(NSNotification *)notification
{
    log_append("window will exit full screen\n");
        
    zpolygons_to_render_size = 0;
}

- (NSSize)
    windowWillResize:(NSWindow *)sender
    toSize:(NSSize)frameSize
{
    if (!startup_complete)
    {
        return frameSize;
    }
    
    float title_bar_height =
        (float)([sender contentRectForFrameRect: sender.frame].size.height
            - sender.frame.size.height);
    
    [self
        handleResizeTo: frameSize
        withTitleBarHeight: title_bar_height];
    
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

bool32_t application_running = true;
bool32_t has_retina_screen = true;

NSWindowWithCustomResponder * window = NULL;


int main(int argc, const char * argv[]) {
    
    client_logic_get_application_name_to_recipient(
        /* recipient: */ application_name,
        /* recipient_size: */ 100);
    
    init_application(
        /* const float window_left: */
            INITIAL_WINDOW_LEFT,
        /* const float window_width: */
            INITIAL_WINDOW_WIDTH,
        /* const float window_bottom: */
            INITIAL_WINDOW_BOTTOM,
        /* const float window_height: */
            INITIAL_WINDOW_HEIGHT);
    log_append("initialized application: ");
    log_append(application_name);
    
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
        /* x: */ INITIAL_WINDOW_LEFT,
        /* y: */ INITIAL_WINDOW_BOTTOM,
        /* width: */ INITIAL_WINDOW_WIDTH,
        /* height: */ INITIAL_WINDOW_HEIGHT);
    
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
        [NSString stringWithUTF8String:application_name];  
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
    
    startup_complete = true;
    
    @autoreleasepool {
    return NSApplicationMain(argc, argv);
    }
}
