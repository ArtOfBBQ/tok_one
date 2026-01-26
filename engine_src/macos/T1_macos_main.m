#include "T1_gpu.h"
#include "T1_apple_audio.h"

#include "T1_std.h"
#include "T1_appinit.h"
#include "T1_logger.h"
#include "T1_random.h"
#include "T1_zlight.h"
#include "T1_io.h"
#include "T1_global.h"
#include "T1_simd.h"
#include "T1_clientlogic.h"

void T1_macos_update_window_size(void);

static uint32_t T1_apple_keycode_to_tokone_keycode(const uint32_t apple_key)
{
    #if T1_LOGGER_ASSERTS_ACTIVE == T1_ACTIVE
    char err_msg[128];
    #elif T1_LOGGER_ASSERTS_ACTIVE == T1_INACTIVE
    #else
    #error
    #endif
    
    switch (apple_key) {
        case (24):
            return T1_IO_KEY_HAT;
        case (29):
            return T1_IO_KEY_0;
        case (27):
            return T1_IO_KEY_MINUS;
        case (36):
            return T1_IO_KEY_ENTER;
        case (123):
            return T1_IO_KEY_LEFTARROW;
        case (124):
            return T1_IO_KEY_RIGHTARROW;
        case (125):
            return T1_IO_KEY_DOWNARROW;
        case (126):
            return T1_IO_KEY_UPARROW;
        case (18):
            return T1_IO_KEY_1;
        case (19):
            return T1_IO_KEY_2;
        case (20):
            return T1_IO_KEY_3;
        case (21):
            return T1_IO_KEY_4;
        case (23):
            return T1_IO_KEY_5;
        case (22):
            return T1_IO_KEY_6;
        case (26):
            return T1_IO_KEY_7;
        case (28):
            return T1_IO_KEY_8;
        case (25):
            return T1_IO_KEY_9;
        case (0):
            return T1_IO_KEY_A;
        case (1):
            return T1_IO_KEY_S;
        case (2):
            return T1_IO_KEY_D;
        case (3):
            return T1_IO_KEY_F;
        case (4):
            return T1_IO_KEY_H;
        case (5):
            return T1_IO_KEY_G;
        case (6):
            return T1_IO_KEY_Z;
        case (7):
            return T1_IO_KEY_X;
        case (8):
            return T1_IO_KEY_C;
        case (9):
            return T1_IO_KEY_V;
        case (12):
            return T1_IO_KEY_Q;
        case (13):
            return T1_IO_KEY_W;
        case (14):
            return T1_IO_KEY_E;
        case (15):
            return T1_IO_KEY_R;
        case (17):
            return T1_IO_KEY_T;
        case (16):
            return T1_IO_KEY_Y;
        case (32):
            return T1_IO_KEY_U;
        case (34):
            return T1_IO_KEY_I;
        case (31):
            return T1_IO_KEY_O;
        case (35):
            return T1_IO_KEY_P;
        case (37):
            return T1_IO_KEY_L;
        case (38):
            return T1_IO_KEY_J;
        case (40):
            return T1_IO_KEY_K;
        case (41):
            return T1_IO_KEY_SEMICOLON;
        case (39):
            return T1_IO_KEY_COLON;
        case (30):
            return T1_IO_KEY_OPENSQUAREBRACKET;
        case (33):
            return T1_IO_KEY_AT;
        case (42):
            return T1_IO_KEY_CLOSESQUAREBRACKET;
        case (11):
            return T1_IO_KEY_B;
        case (45):
            return T1_IO_KEY_N;
        case (46):
            return T1_IO_KEY_M;
        case (43):
            return T1_IO_KEY_COMMA;
        case (47):
            // TODO: rename me to period
            return T1_IO_KEY_FULLSTOP;
        case (48):
            return T1_IO_KEY_TAB;
        case (44):
            return T1_IO_KEY_BACKSLASH;
        case (49):
            return T1_IO_KEY_SPACEBAR;
        case (51):
            return T1_IO_KEY_BACKSPACE;
        case (53):
            return T1_IO_KEY_ESCAPE;
        case (93):
            return T1_IO_KEY_YENSIGN;
        case (94):
            return T1_IO_KEY_UNDERSCORE;
        case (122):
            return T1_IO_KEY_F1;
        case (120):
            return T1_IO_KEY_F2;
        case (99):
            return T1_IO_KEY_F3;
        case (118):
            return T1_IO_KEY_F4;
        case (96):
            return T1_IO_KEY_F5;
        case (97):
            return T1_IO_KEY_F6;
        case (98):
            return T1_IO_KEY_F7;
        case (100):
            return T1_IO_KEY_F8;
        case (101):
            return T1_IO_KEY_F9;
        case (109):
            return T1_IO_KEY_F10;
        case (103):
            return T1_IO_KEY_F11;
        case (111):
            return T1_IO_KEY_F12;
        case (102):
            return T1_IO_KEY_ROMAJIBUTTON;
        case (104):
            return T1_IO_KEY_KANABUTTON;
        case (115):
            return T1_IO_KEY_HOME;
        case (116):
            return T1_IO_KEY_PAGEUP;
        case (119):
            return T1_IO_KEY_END;
        case (121):
            return T1_IO_KEY_PAGEDOWN;
        default:
            #if T1_LOGGER_ASSERTS_ACTIVE == T1_ACTIVE
            T1_std_strcpy_cap(err_msg, 128, "unhandled apple keycode: ");
            T1_std_strcat_uint_cap(err_msg, 128, apple_key);
            T1_std_strcat_cap(err_msg, 128, "\n");
            #elif T1_LOGGER_ASSERTS_ACTIVE == T1_INACTIVE
            #else
            #error
            #endif
            break;
    }
    
    #if T1_LOGGER_ASSERTS_ACTIVE == T1_ACTIVE
    log_dump_and_crash(err_msg);
    #elif T1_LOGGER_ASSERTS_ACTIVE == T1_INACTIVE
    #else
    #error
    #endif
    
    return T1_IO_KEY_ESCAPE;
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
    if (T1_global->block_mouse) {
        return;
    }
    
    NSPoint window_location = [event locationInWindow];
    
    T1_io_events[T1_IO_LAST_GPU_DATA].screen_x =
        (float)window_location.x;
    T1_io_events[T1_IO_LAST_GPU_DATA].screen_y =
        (float)window_location.y;
    
    T1_io_event_register(&T1_io_events[T1_IO_LAST_MOUSE_MOVE]);
    T1_io_event_register(&T1_io_events[T1_IO_LAST_MOUSE_OR_TOUCH_MOVE]);
}

- (void)mouseDragged:(NSEvent *)event
{
    if (T1_global->block_mouse) {
        return;
    }
    
    NSPoint window_location = [event locationInWindow];
    
    T1_io_events[T1_IO_LAST_GPU_DATA].screen_x =
        (float)window_location.x;
    T1_io_events[T1_IO_LAST_GPU_DATA].screen_y =
        (float)window_location.y;
    
    T1_io_event_register(&T1_io_events[T1_IO_LAST_MOUSE_MOVE]);
    T1_io_event_register(&T1_io_events[T1_IO_LAST_MOUSE_OR_TOUCH_MOVE]);
}

- (void)mouseDown:(NSEvent *)event
{
    if (T1_global->block_mouse) {
        return;
    }
    
    T1_io_event_register(&T1_io_events[T1_IO_LAST_LCLICK_START]);
    T1_io_events[T1_IO_LAST_TOUCH_OR_LCLICK_START] =
        T1_io_events[T1_IO_LAST_LCLICK_START];
}

- (void)mouseUp:(NSEvent *)event
{
    if (T1_global->block_mouse) {
        return;
    }
    
    T1_io_event_register(&T1_io_events[T1_IO_LAST_LCLICK_END]);
    T1_io_events[T1_IO_LAST_TOUCH_OR_LCLICK_END] =
        T1_io_events[T1_IO_LAST_LCLICK_END];
}

- (void)rightMouseDown:(NSEvent *)event
{
    if (T1_global->block_mouse) {
        return;
    }
    
    T1_io_event_register(&T1_io_events[T1_IO_LAST_RCLICK_START]);
}

- (void)rightMouseUp:(NSEvent *)event
{
    if (T1_global->block_mouse) {
        return;
    }
    
    T1_io_event_register(&T1_io_events[T1_IO_LAST_RCLICK_END]);
}

- (void)update_mouse_location {
    // Currently happens in mouseMoved, so ignore this
}

- (void)keyDown:(NSEvent *)event {
    T1_io_register_keydown(T1_apple_keycode_to_tokone_keycode(event.keyCode));
}

- (void)keyUp:(NSEvent *)event {
    T1_io_register_keyup(T1_apple_keycode_to_tokone_keycode(event.keyCode));
}

- (void)scrollWheel:(NSEvent *)event {
    T1_io_register_mousescroll((float)[event deltaY]);
}

- (float)getWidth {
    return (float)[[self contentView] frame].size.width;
}

- (float)getHeight {
    return (float)[[self contentView] frame].size.height;
}

@end

NSWindowWithCustomResponder * window = NULL;

void T1_platform_update_mouse_location(void) {
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
    
    T1_appinit_shutdown();
    
    T1_clientlogic_shutdown();
    
    bool32_t write_succesful = false;
    log_dump(&write_succesful);
    
    if (!write_succesful) {
        log_append("ERROR - failed to store the log file on app close..\n");
    }
    
    T1_platform_close_app();
}

- (void)
    windowWillEnterFullScreen:(NSNotification *)notification
{
    T1_uielement_delete_all();
    T1_zsprite_delete_all();
    T1_global->fullscreen = true;
}

- (void)
    windowWillExitFullScreen:(NSNotification *)notification
{
    T1_uielement_delete_all();
    T1_zsprite_delete_all();
    #if T1_PARTICLES_ACTIVE == T1_ACTIVE
    T1_particle_effects_delete_all();
    #elif T1_PARTICLES_ACTIVE == T1_INACTIVE
    #else
    #error
    #endif
    T1_global->fullscreen = false;
}

- (void)windowDidMove:(NSNotification *)notification
{
    T1_global_update_window_pos(
        (float)(((NSWindow *)[notification object]).frame.origin.x),
        (float)(((NSWindow *)[notification object]).frame.origin.y));
}

- (NSSize)
    windowWillResize:(NSWindow *)sender
    toSize:(NSSize)frameSize
{
    T1_platform_layer_start_window_resize(
        T1_platform_get_current_time_us());
    
    return frameSize;
}

- (void)windowDidResize:
    (NSNotification *)notification
{
    T1_global_update_window_size(
        /* float width: */
            [window getWidth],
        /* float height */
            [window getHeight],
        /* uint64_t at_timestamp_us: */
            T1_platform_get_current_time_us());
    
    [apple_gpu_delegate updateFinalWindowSize];
}
@end

int main(int argc, const char * argv[]) {
    
    T1_gameloop_active = false;
    T1_app_running = true;
    
    char errmsg[512];
    uint32_t success = 1;
    T1_appinit_before_gpu_init(
        &success,
        errmsg);
    
    // NSScreen *screen = [[NSScreen screens] objectAtIndex:0];
    NSRect window_rect = NSMakeRect(
        /* x: */ T1_global->window_left,
        /* y: */ T1_global->window_bottom,
        /* width: */ T1_global->window_width,
        /* height: */ T1_global->window_height);
    
    window =
        [[NSWindowWithCustomResponder alloc]
            initWithContentRect: window_rect
            styleMask:
                NSWindowStyleMaskTitled    |
                NSWindowStyleMaskClosable  |
                NSWindowStyleMaskResizable
            backing: NSBackingStoreBuffered 
            defer: NO];
    window.animationBehavior =
        NSWindowAnimationBehaviorNone;
    
    GameWindowDelegate * window_delegate = [[GameWindowDelegate alloc] init];
    
    NSString * nsstring_app_name =
        [NSString stringWithUTF8String:APPLICATION_NAME];
    [window setDelegate: window_delegate];
    [window setTitle: nsstring_app_name];
    [window makeMainWindow];
    [window setAcceptsMouseMovedEvents:YES];
    [window setOrderedIndex:0];
    [window makeKeyAndOrderFront: nil];
    
    [[NSApplication sharedApplication] activateIgnoringOtherApps:YES];
    
    if (!success) {
        T1_platform_request_messagebox(errmsg);
    }
    
    id<MTLDevice> metal_device_for_window = MTLCreateSystemDefaultDevice();
    
    mtk_view = [[MTKView alloc]
        initWithFrame: window_rect
        device: metal_device_for_window];
    
    mtk_view.autoResizeDrawable = true;
    
    mtk_view.preferredFramesPerSecond = 120;
    mtk_view.enableSetNeedsDisplay = false;
    
    // Indicate that each pixel in the depth buffer is a 32-bit floating point
    // value.
    mtk_view.depthStencilPixelFormat = MTLPixelFormatDepth32Float;
    
    // Indicate that Metal should clear all values in the depth buffer to x
    // when you create a render command encoder with the MetalKit view's
    // `currentRenderPassDescriptor` property.
    mtk_view.clearDepth = T1_CLEARDEPTH;
    [mtk_view setPaused: false];
    [mtk_view setNeedsDisplay: false];
    
    window.contentView = mtk_view;
    
    apple_gpu_delegate = [[MetalKitViewDelegate alloc] init];
    [mtk_view setDelegate: apple_gpu_delegate];
    
    char shader_lib_path_cstr[2000];
    T1_platform_get_resources_path(
        shader_lib_path_cstr,
        2000);
    
    T1_std_strcat_cap(
        shader_lib_path_cstr,
        1000,
        "/Shaders.metallib");
    
    NSString * shader_lib_path =
        [NSString
            stringWithCString:shader_lib_path_cstr
            encoding:NSASCIIStringEncoding];
    
    bool32_t result = T1_apple_gpu_init(
        /* void (* arg_funcptr_shared_gameloop_update)(GPUDataForSingleFrame *): */
            T1_gameloop_update_before_render_pass,
            T1_gameloop_update_after_render_pass,
        /* id<MTLDevice> with_metal_device: */
            metal_device_for_window,
        /* NSString *shader_lib_filepath: */
            shader_lib_path,
        /* bool32_t has_retina_screen: */
            (float)[[window screen] backingScaleFactor],
        /* char * error_msg_string: */
            errmsg);
    
    if (!result || !T1_app_running) {
        #if T1_LOGGER_ASSERTS_ACTIVE == T1_ACTIVE
        log_dump_and_crash("Can't draw anything to the screen...\n");
        #elif T1_LOGGER_ASSERTS_ACTIVE == T1_INACTIVE
        #else
        #error
        #endif
        
        char errmsg2[512];
        T1_std_strcpy_cap(
            errmsg2,
            512,
            "Critical failure: couldn't configure Metal graphics."
            " Looked for shader in: ");
        T1_std_strcat_cap(
            errmsg2,
            512,
            shader_lib_path_cstr);
        T1_std_strcat_cap(
            errmsg2,
            512,
            " Metal error description: ");
        T1_std_strcat_cap(
            errmsg2,
            512,
            errmsg);
        
        T1_platform_request_messagebox(errmsg2);
    }
    
    T1_appinit_after_gpu_init_step1(
        &success,
        errmsg);
    
    if (!success) {
        T1_platform_request_messagebox(errmsg);
        T1_app_running = false;
    } else {
        T1_platform_start_thread(
            T1_appinit_after_gpu_init_step2,
            0);
    }
    
    @autoreleasepool {
        return NSApplicationMain(argc, argv);
    }
}

void T1_platform_request_messagebox(const char * message) {
    NSAlert * alert = [[NSAlert alloc] init];
    NSString * NSmsg = [NSString
        stringWithCString:message
        encoding:NSASCIIStringEncoding];
    [alert setMessageText: NSmsg];
    [alert runModal];
}

void T1_platform_enter_fullscreen(void) {
    if ((window.styleMask & NSWindowStyleMaskFullScreen) == 0) {
        [window toggleFullScreen: window];
    }
}

void T1_platform_toggle_fullscreen(void) {
    [window toggleFullScreen: window];
}
