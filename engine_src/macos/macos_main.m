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

#if __cplusplus
extern "C" {
#endif

void macos_update_window_size(void);

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
            // TODO: rename me to period
            return TOK_KEY_FULLSTOP;
        case (48):
            return TOK_KEY_TAB;
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
        case (122):
            return TOK_KEY_F1;
        case (120):
            return TOK_KEY_F2;
        case (99):
            return TOK_KEY_F3;
        case (118):
            return TOK_KEY_F4;
        case (96):
            return TOK_KEY_F5;
        case (97):
            return TOK_KEY_F6;
        case (98):
            return TOK_KEY_F7;
        case (100):
            return TOK_KEY_F8;
        case (101):
            return TOK_KEY_F9;
        case (109):
            return TOK_KEY_F10;
        case (103):
            return TOK_KEY_F11;
        case (111):
            return TOK_KEY_F12;
        case (102):
            return TOK_KEY_ROMAJIBUTTON;
        case (104):
            return TOK_KEY_KANABUTTON;
        case (115):
            return TOK_KEY_HOME;
        case (116):
            return TOK_KEY_PAGEUP;
        case (119):
            return TOK_KEY_END;
        case (121):
            return TOK_KEY_PAGEDOWN;
        default:
            #ifndef LOGGER_IGNORE_ASSERTS
            common_strcpy_capped(err_msg, 128, "unhandled apple keycode: ");
            common_strcat_uint_capped(err_msg, 128, apple_key);
            common_strcat_capped(err_msg, 128, "\n");
            #endif
            break;
    }
    
    #ifndef LOGGER_IGNORE_ASSERTS
    log_dump_and_crash(err_msg);
    #endif
    
    return TOK_KEY_ESCAPE;
}

@interface NSWindowWithCustomResponder: NSWindow
// @property (nonatomic, readwrite, retain) NSWindow * baseclass_window;
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

- (void)mouseMoved:(NSEvent *)event
{
    NSPoint window_location = [event locationInWindow];
    
    register_interaction(&user_interactions[INTR_PREVIOUS_MOUSE_OR_TOUCH_MOVE]);
    user_interactions[INTR_PREVIOUS_MOUSE_OR_TOUCH_MOVE].screen_x =
        (float)window_location.x;
    user_interactions[INTR_PREVIOUS_MOUSE_OR_TOUCH_MOVE].screen_y =
        (float)window_location.y;
    
    register_interaction(&user_interactions[INTR_PREVIOUS_MOUSE_MOVE]);
    user_interactions[INTR_PREVIOUS_MOUSE_MOVE].screen_x =
        (float)window_location.x;
    user_interactions[INTR_PREVIOUS_MOUSE_MOVE].screen_y =
        (float)window_location.y;
}

- (void)mouseDragged:(NSEvent *)event
{
    NSPoint window_location = [event locationInWindow];
    
    register_interaction(&user_interactions[INTR_PREVIOUS_MOUSE_OR_TOUCH_MOVE]);
    user_interactions[INTR_PREVIOUS_MOUSE_OR_TOUCH_MOVE].screen_x =
        (float)window_location.x;
    user_interactions[INTR_PREVIOUS_MOUSE_OR_TOUCH_MOVE].screen_y =
        (float)window_location.y;
    
    register_interaction(&user_interactions[INTR_PREVIOUS_MOUSE_MOVE]);
    user_interactions[INTR_PREVIOUS_MOUSE_MOVE].screen_x =
        (float)window_location.x;
    user_interactions[INTR_PREVIOUS_MOUSE_MOVE].screen_y =
        (float)window_location.y;
}

- (void)mouseDown:(NSEvent *)event
{
    register_interaction(&user_interactions[INTR_PREVIOUS_LEFTCLICK_START]);
    user_interactions[INTR_PREVIOUS_TOUCH_OR_LEFTCLICK_START] =
        user_interactions[INTR_PREVIOUS_LEFTCLICK_START];
}

- (void)mouseUp:(NSEvent *)event
{
    register_interaction(&user_interactions[INTR_PREVIOUS_LEFTCLICK_END]);
    user_interactions[INTR_PREVIOUS_TOUCH_OR_LEFTCLICK_END] =
        user_interactions[INTR_PREVIOUS_LEFTCLICK_END];
}

- (void)rightMouseDown:(NSEvent *)event
{
    register_interaction(&user_interactions[INTR_PREVIOUS_RIGHTCLICK_START]);
}

- (void)rightMouseUp:(NSEvent *)event
{
    register_interaction(&user_interactions[INTR_PREVIOUS_RIGHTCLICK_END]);
}

- (void)update_mouse_location {
    // Currently happens in mouseMoved, so ignore this
}

- (void)keyDown:(NSEvent *)event {
    register_keydown(apple_keycode_to_tokone_keycode(event.keyCode));
}

- (void)keyUp:(NSEvent *)event {
    register_keyup(apple_keycode_to_tokone_keycode(event.keyCode));
}

- (void)scrollWheel:(NSEvent *)event {
    register_mousescroll((float)[event deltaY]);
}

- (float)getWidth {
    return (float)[[self contentView] frame].size.width;
}

- (float)getHeight {
    return (float)[[self contentView] frame].size.height;
}

@end

NSWindowWithCustomResponder * window = NULL;

void platform_update_mouse_location(void) {
    [window update_mouse_location];
}

/*
these variables may not exist on platforms where window resizing is
impossible
*/
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
        log_append("ERROR - failed to store the log file on app close..\n");
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
    windowsize_update_window_position(
        (float)(((NSWindow *)[notification object]).frame.origin.x),
        (float)(((NSWindow *)[notification object]).frame.origin.y));
}

- (NSSize)
    windowWillResize:(NSWindow *)sender
    toSize:(NSSize)frameSize
{
    delete_all_ui_elements();
    zpolygons_to_render->size = 0;
    particle_effects_size = 0;
    
    return frameSize;
}

- (void)windowDidResize:(NSNotification *)notification {
    
    windowsize_update_window_size(
        /* float width: */
            [window getWidth],
        /* float height */
            [window getHeight],
        /* uint64_t at_timestamp_microseconds: */
            platform_get_current_time_microsecs());
    
    [apple_gpu_delegate updateViewport];
}
@end

extern bool32_t metal_active;

int main(int argc, const char * argv[]) {
    
    metal_active = false;
    gameloop_active = false;
    application_running = true;
    has_retina_screen = true; // TODO: actually query the machine and find out
    
    assert(sizeof(GPUPolygon) % 32 == 0);
    
    char errmsg[512];
    uint32_t success = 1;
    init_application_before_gpu_init(
        &success,
        errmsg);
    
    bool32_t initial_log_dump_succesful = false;
    log_dump(&initial_log_dump_succesful);
    if (!initial_log_dump_succesful) {
        printf("%s\n", "initial log dump unsuccesful, exiting app");
        return 1;
    }
    
    apple_gpu_init(gameloop_update);
    
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
    
    GameWindowDelegate * window_delegate = [[GameWindowDelegate alloc] init];
    
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
    mtk_view.preferredFramesPerSecond = 120;
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
    
    char shader_lib_path_cstr[2000];
    platform_get_resources_path(shader_lib_path_cstr, 2000);
    common_strcat_capped(
        shader_lib_path_cstr,
        1000,
        "/Shaders.metallib");
    
    NSString * shader_lib_path =
        [NSString
            stringWithCString:shader_lib_path_cstr
            encoding:NSASCIIStringEncoding];
     
    BOOL result = [apple_gpu_delegate
        configureMetalWithDevice: metal_device
        fromFilePath: shader_lib_path
        errMsgCStr: errmsg];
    
    if (!result || !application_running) {
        #ifndef LOGGER_IGNORE_ASSERTS
        log_dump_and_crash("Can't draw anything to the screen...\n");
        #endif
        
        char errmsg2[512];
        common_strcpy_capped(
            errmsg2,
            512,
            "Critical failure: couldn't configure Metal graphics."
            " Looked for shader in: ");
        common_strcat_capped(
            errmsg2,
            512,
            shader_lib_path_cstr);
        common_strcat_capped(
            errmsg2,
            512,
            " Metal error description: ");
        common_strcat_capped(
            errmsg2,
            512,
            errmsg);
        
        platform_request_messagebox(errmsg2);
    } else {
        load_font_images();
        
        client_logic_early_startup(&success, errmsg);
        
        init_application_after_gpu_init();
        
        start_audio_loop();
    }
    
    metal_active = true;
    
    #ifndef LOGGER_IGNORE_ASSERTS
    if (!success && application_running) {
        log_dump_and_crash(errmsg);
    }
    #endif
    
    @autoreleasepool {
        return NSApplicationMain(argc, argv);
    }
}

void platform_request_messagebox(const char * message) {
    NSAlert * alert = [[NSAlert alloc] init];
    NSString * NSmsg = [NSString
        stringWithCString:message
        encoding:NSASCIIStringEncoding];
    [alert setMessageText: NSmsg];
    [alert runModal];
}

void platform_enter_fullscreen(void) {
    if ((window.styleMask & NSWindowStyleMaskFullScreen) == 0) {
        [window toggleFullScreen: window];
    }
}

void platform_toggle_fullscreen(void) {
    [window toggleFullScreen: window];
}

#if __cplusplus
}
#endif
