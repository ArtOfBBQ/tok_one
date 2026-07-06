#include "T1_gpu.h"
#include "T1_apple_audio.h"

#include "T1_std.h"
#include "T1_appinit.h"
#include "T1_settings.h"
#include "T1_log.h"
#include "T1_rand.h"
#include "T1_zsprite.h"
#include "T1_zlight.h"
#include "T1_ui_widget.h"
#include "T1_io.h"
#include "T1_global.h"
#include "T1_simd.h"
#include "T1_client.h"
#include "T1_gameloop.h"
#include "T1_particle.h"
#include "T1_platform_layer.h"

#import <GameController/GameController.h>

void T1_macos_update_window_size(void);

static u32 T1_apple_keycode_to_tokone_keycode(
    const u32 apple_key)
{
    #if T1_LOG_ASSERTS_ACTIVE == T1_ACTIVE
    char err_msg[128];
    #elif T1_LOG_ASSERTS_ACTIVE == T1_INACTIVE
    #else
    #error
    #endif
    
    switch (apple_key) {
        case (24):
            return T1_IO_KEYBOARD_HAT;
        case (29):
            return T1_IO_KEYBOARD_0;
        case (27):
            return T1_IO_KEYBOARD_MINUS;
        case (36):
            return T1_IO_KEYBOARD_ENTER;
        case (123):
            return T1_IO_KEYBOARD_LEFTARROW;
        case (124):
            return T1_IO_KEYBOARD_RIGHTARROW;
        case (125):
            return T1_IO_KEYBOARD_DOWNARROW;
        case (126):
            return T1_IO_KEYBOARD_UPARROW;
        case (18):
            return T1_IO_KEYBOARD_1;
        case (19):
            return T1_IO_KEYBOARD_2;
        case (20):
            return T1_IO_KEYBOARD_3;
        case (21):
            return T1_IO_KEYBOARD_4;
        case (23):
            return T1_IO_KEYBOARD_5;
        case (22):
            return T1_IO_KEYBOARD_6;
        case (26):
            return T1_IO_KEYBOARD_7;
        case (28):
            return T1_IO_KEYBOARD_8;
        case (25):
            return T1_IO_KEYBOARD_9;
        case (0):
            return T1_IO_KEYBOARD_A;
        case (1):
            return T1_IO_KEYBOARD_S;
        case (2):
            return T1_IO_KEYBOARD_D;
        case (3):
            return T1_IO_KEYBOARD_F;
        case (4):
            return T1_IO_KEYBOARD_H;
        case (5):
            return T1_IO_KEYBOARD_G;
        case (6):
            return T1_IO_KEYBOARD_Z;
        case (7):
            return T1_IO_KEYBOARD_X;
        case (8):
            return T1_IO_KEYBOARD_C;
        case (9):
            return T1_IO_KEYBOARD_V;
        case (12):
            return T1_IO_KEYBOARD_Q;
        case (13):
            return T1_IO_KEYBOARD_W;
        case (14):
            return T1_IO_KEYBOARD_E;
        case (15):
            return T1_IO_KEYBOARD_R;
        case (17):
            return T1_IO_KEYBOARD_T;
        case (16):
            return T1_IO_KEYBOARD_Y;
        case (32):
            return T1_IO_KEYBOARD_U;
        case (34):
            return T1_IO_KEYBOARD_I;
        case (31):
            return T1_IO_KEYBOARD_O;
        case (35):
            return T1_IO_KEYBOARD_P;
        case (37):
            return T1_IO_KEYBOARD_L;
        case (38):
            return T1_IO_KEYBOARD_J;
        case (40):
            return T1_IO_KEYBOARD_K;
        case (41):
            return T1_IO_KEYBOARD_SEMICOLON;
        case (39):
            return T1_IO_KEYBOARD_COLON;
        case (30):
            return T1_IO_KEYBOARD_OPENSQUAREBRACKET;
        case (33):
            return T1_IO_KEYBOARD_AT;
        case (42):
            return T1_IO_KEYBOARD_CLOSESQUAREBRACKET;
        case (11):
            return T1_IO_KEYBOARD_B;
        case (45):
            return T1_IO_KEYBOARD_N;
        case (46):
            return T1_IO_KEYBOARD_M;
        case (43):
            return T1_IO_KEYBOARD_COMMA;
        case (47):
            // TODO: rename me to period
            return T1_IO_KEYBOARD_FULLSTOP;
        case (48):
            return T1_IO_KEYBOARD_TAB;
        case (44):
            return T1_IO_KEYBOARD_BACKSLASH;
        case (49):
            return T1_IO_KEYBOARD_SPACEBAR;
        case (51):
            return T1_IO_KEYBOARD_BACKSPACE;
        case (53):
            return T1_IO_KEYBOARD_ESCAPE;
        case (93):
            return T1_IO_KEYBOARD_YENSIGN;
        case (94):
            return T1_IO_KEYBOARD_UNDERSCORE;
        case (122):
            return T1_IO_KEYBOARD_F1;
        case (120):
            return T1_IO_KEYBOARD_F2;
        case (99):
            return T1_IO_KEYBOARD_F3;
        case (118):
            return T1_IO_KEYBOARD_F4;
        case (96):
            return T1_IO_KEYBOARD_F5;
        case (97):
            return T1_IO_KEYBOARD_F6;
        case (98):
            return T1_IO_KEYBOARD_F7;
        case (100):
            return T1_IO_KEYBOARD_F8;
        case (101):
            return T1_IO_KEYBOARD_F9;
        case (109):
            return T1_IO_KEYBOARD_F10;
        case (103):
            return T1_IO_KEYBOARD_F11;
        case (111):
            return T1_IO_KEYBOARD_F12;
        case (102):
            return T1_IO_KEYBOARD_ROMAJIBUTTON;
        case (104):
            return T1_IO_KEYBOARD_KANABUTTON;
        case (115):
            return T1_IO_KEYBOARD_HOME;
        case (116):
            return T1_IO_KEYBOARD_PAGEUP;
        case (119):
            return T1_IO_KEYBOARD_END;
        case (121):
            return T1_IO_KEYBOARD_PAGEDOWN;
        default:
            #if T1_LOG_ASSERTS_ACTIVE == T1_ACTIVE
            T1_std_strcpy_cap(err_msg, 128, "unhandled apple keycode: ");
            T1_std_strcat_u32_cap(err_msg, 128, apple_key);
            T1_std_strcat_cap(err_msg, 128, "\n");
            #elif T1_LOG_ASSERTS_ACTIVE == T1_INACTIVE
            #else
            #error
            #endif
            break;
    }
    
    #if T1_LOG_ASSERTS_ACTIVE == T1_ACTIVE
    T1_log_dump_and_crash(err_msg);
    #elif T1_LOG_ASSERTS_ACTIVE == T1_INACTIVE
    #else
    #error
    #endif
    
    return T1_IO_KEYBOARD_ESCAPE;
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
    
    T1_io_register_key_move_to_pos(
        T1_IO_MOUSE,
        (f32)window_location.x,
        (f32)window_location.y);
}

- (void)mouseDragged:(NSEvent *)event
{
    if (T1_global->block_mouse) {
        return;
    }
    
    NSPoint window_location = [event locationInWindow];
    
    T1_io_register_key_move_to_pos(
        T1_IO_MOUSE,
        (f32)window_location.x,
        (f32)window_location.y);
}

- (void)mouseDown:(NSEvent *)event
{
    T1_io_register_keydown(T1_IO_MOUSE_LCLICK);
}

- (void)mouseUp:(NSEvent *)event
{
    T1_io_register_keyup(T1_IO_MOUSE_LCLICK);
}

- (void)rightMouseDown:(NSEvent *)event
{
    T1_io_register_keydown(T1_IO_MOUSE_RCLICK);
}

- (void)rightMouseUp:(NSEvent *)event
{
    T1_io_register_keyup(T1_IO_MOUSE_RCLICK);
}

- (void)otherMouseDown:(NSEvent *)event
{
    s64 button_num = [event buttonNumber];
    
    T1_log_assert(button_num >= 2);
    
    if (button_num < 2) { return; }
    button_num -= 2;
    
    if (button_num >= 4) { return; }
    
    T1_io_register_keydown(
        T1_IO_MOUSE_OTHERCLICK1 + (u32)button_num);
}

- (void)otherMouseUp:(NSEvent *)event
{
    s64 button_num = [event buttonNumber];
    
    T1_log_assert(button_num >= 2);
    
    if (button_num < 2) { return; }
    button_num -= 2;
    
    if (button_num >= 4) { return; }
    
    T1_io_register_keyup(
        T1_IO_MOUSE_OTHERCLICK1 + (u32)button_num);
}

- (void)keyDown:(NSEvent *)event {
    T1_io_register_keydown(T1_apple_keycode_to_tokone_keycode(event.keyCode));
}

- (void)flagsChanged:(NSEvent *)event {
    
    NSEventModifierFlags modifiers = [event modifierFlags];
    
    if (modifiers & NSEventModifierFlagShift) {
        T1_io_register_keydown(T1_IO_KEYBOARD_SHIFT);
    } else if (T1_io_key_is_down(T1_IO_KEYBOARD_SHIFT, -1)) {
        T1_io_register_keyup(T1_IO_KEYBOARD_SHIFT);
    }
}

- (void)keyUp:(NSEvent *)event {
    T1_io_register_keyup(T1_apple_keycode_to_tokone_keycode(event.keyCode));
}

- (void)scrollWheel:(NSEvent *)event {
    f32 delta = (float)[event deltaY];
    f32 step = 0.1f;
    
    if (delta > 0.0f) {
        while (delta > step) {
            T1_io_register_keyup_force_up_short(
                T1_IO_MOUSE_WHEEL_UP);
            delta -= step;
        }
    } else {
        while (delta < -step) {
            T1_io_register_keyup_force_up_short(
                T1_IO_MOUSE_WHEEL_DOWN);
            delta += step;
        }
    }
}

- (float)getWidth {
    return (float)[[self contentView] frame].size.width;
}

- (float)getHeight {
    return (float)[[self contentView] frame].size.height;
}

@end

NSWindowWithCustomResponder * window = NULL;

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
    T1_log_append("window will close, terminating app..\n");
    
    T1_appinit_shutdown();
    
    T1_client_shutdown();
    
    uint8_t write_succesful = false;
    T1_log_dump(&write_succesful);
    
    if (!write_succesful) {
        T1_log_append(
            "ERROR - failed to store "
            " the log file on app "
            " close..\n");
    }
    
    T1_os_close_app();
}

- (void)
    windowWillEnterFullScreen:(NSNotification *)notification
{
    T1_ui_widget_delete_all();
    T1_zsprite_delete_all();
    T1_global->fullscreen = true;
}

- (void)
    windowWillExitFullScreen:(NSNotification *)notification
{
    T1_ui_widget_delete_all();
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
    T1_os_layer_start_window_resize(
        T1_os_get_current_time_us());
    
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
        /* u64 at_timestamp_us: */
            T1_os_get_current_time_us());
    
    [apple_gpu_delegate updateFinalWindowSize];
}
@end

int main(int argc, const char * argv[]) {
    
    T1_gameloop_active = false;
    T1_log_app_running = true;
    
    char errmsg[512];
    b8 success = 1;
    T1_appinit_before_gpu_init(
        &success,
        errmsg);
    
    // NSScreen *screen = [[NSScreen screens] objectAtIndex:0];
    NSRect window_rect = NSMakeRect(
        /* x: */ T1_global->window_left,
        /* y: */ T1_global->window_bottom,
        /* width: */ T1_global->window_wh[0],
        /* height: */ T1_global->window_wh[1]);
    
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
        [NSString stringWithUTF8String:
            T1_APP_NAME];
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
    mtk_view.clearDepth = T1_GLOBAL_CLEARDEPTH;
    [mtk_view setPaused: false];
    [mtk_view setNeedsDisplay: false];
    
    window.contentView = mtk_view;
    
    apple_gpu_delegate = [[MetalKitViewDelegate alloc] init];
    [mtk_view setDelegate: apple_gpu_delegate];
    
    char shader_lib_path_cstr[2000];
    T1_os_get_res_dir(
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
    
    b8 result = T1_apple_gpu_init(
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
    
    if (!result || !T1_log_app_running) {
        #if T1_LOG_ASSERTS_ACTIVE == T1_ACTIVE
        T1_log_dump_and_crash("Can't draw anything to the screen...\n");
        #elif T1_LOG_ASSERTS_ACTIVE == T1_INACTIVE
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
        T1_log_app_running = false;
    } else {
        T1_os_start_thread(
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

void T1_os_enter_fullscreen(void) {
    if ((window.styleMask & NSWindowStyleMaskFullScreen) == 0) {
        [window toggleFullScreen: window];
    }
}

void T1_os_toggle_fullscreen(void) {
    [window toggleFullScreen: window];
}

#if T1_GAMEPAD_ACTIVE == T1_ACTIVE
static void update_simple_key(
    GCExtendedGamepad * g,
    u8 ispressed,
    T1IOKey T1_io_key)
{
    if (ispressed) {
        if (!T1_io_key_is_down(T1_io_key, -1)) {
            T1_io_register_keydown(T1_io_key);
        }
    } else {
        if (T1_io_key_is_down(T1_io_key, -1)) {
            T1_io_register_keyup(T1_io_key);
        }
    }
}

void T1_os_vibrate_gamepad(void) {
    NSArray *controllers = [GCController controllers];
    
    if (controllers.count == 0) {
        return; // No gamepads connected
    }
    
    GCController * c = controllers[0];
    
    if (!c || !c.haptics) {
        return;
    }
    
    NSError * error = nil;    
    CHHapticEngine * eng = [c.haptics
        createEngineWithLocality: GCHapticsLocalityDefault];
    
    if (!eng) {
        return;
    }
    
    #if 0
    // Start the haptic engine
    [eng startAndReturnError: &error];
    if (error) {
        NSLog(@"Failed to start haptic engine: %@", error.localizedDescription);
        return;
    }
    
    // 1. Define haptic parameters (Intensity and Sharpness)
    CHHapticEventParameter * inten = [[CHHapticEventParameter alloc] initWithParameterID:CHHapticEventParameterIDHapticIntensity value:1.0];
    CHHapticEventParameter * sharp = [[CHHapticEventParameter alloc] initWithParameterID:CHHapticEventParameterIDHapticSharpness value:0.5];
    
    // 2. Create a continuous or transient haptic event (0.5-second duration)
    CHHapticEvent *event = [[CHHapticEvent alloc]
        initWithEventType:CHHapticEventTypeHapticContinuous
        parameters:@[intensity, sharpness]
        relativeTime:0.0
        duration:0.5];
    
    // 3. Package the event into a pattern
    CHHapticPattern *pattern = [[CHHapticPattern alloc] initWithEvents:@[event] parameters:@[] error:&error];
    if (error) {
        NSLog(@"Failed to create haptic pattern: %@", error.localizedDescription);
        return;
    }
    
    // 4. Create the player and start playback
    id<CHHapticPatternPlayer> player = [eng makePlayerWithPattern:pattern error:&error];
    if (error) {
        NSLog(@"Failed to create haptic player: %@", error.localizedDescription);
        return;
    }
    
    [player startAtTime:0.0 error:&error];
    if (error) {
        NSLog(@"Failed to play haptic pattern: %@", error.localizedDescription);
    }
    #endif
}

void T1_os_poll_gamepad_events(void) {
    NSArray *controllers = [GCController controllers];
    
    if (controllers.count == 0) {
        return; // No gamepads connected
    }
    
    // Grab the primary controller (like your EasySMX X05PRO)
    GCController * c = controllers[0];
    GCExtendedGamepad * g = c.extendedGamepad;
    
    if (g) {
        update_simple_key(g, g.dpad.left.isPressed, T1_IO_GAMEPAD_DPAD_LEFT); 
        update_simple_key(g, g.dpad.right.isPressed, T1_IO_GAMEPAD_DPAD_RIGHT);
        update_simple_key(g, g.dpad.up.isPressed, T1_IO_GAMEPAD_DPAD_UP); 
        update_simple_key(g, g.dpad.down.isPressed, T1_IO_GAMEPAD_DPAD_DOWN);
        update_simple_key(g, g.leftShoulder.isPressed, T1_IO_GAMEPAD_LEFTSHOULDER);
        update_simple_key(g, g.rightShoulder.isPressed, T1_IO_GAMEPAD_RIGHTSHOULDER);
        update_simple_key(g, g.leftTrigger.isPressed, T1_IO_GAMEPAD_LEFTTRIGGER);
        update_simple_key(g, g.rightTrigger.isPressed, T1_IO_GAMEPAD_RIGHTTRIGGER);
        update_simple_key(g, g.leftThumbstickButton.isPressed, T1_IO_GAMEPAD_LEFTTHUMBSTICKBUTTON);
        update_simple_key(g, g.rightThumbstickButton.isPressed, T1_IO_GAMEPAD_RIGHTTHUMBSTICKBUTTON);
        update_simple_key(g, g.buttonA.isPressed, T1_IO_GAMEPAD_A);
        update_simple_key(g, g.buttonB.isPressed, T1_IO_GAMEPAD_B);
        update_simple_key(g, g.buttonX.isPressed, T1_IO_GAMEPAD_X);
        update_simple_key(g, g.buttonY.isPressed, T1_IO_GAMEPAD_Y);
        update_simple_key(g, g.buttonHome.isPressed, T1_IO_GAMEPAD_HOME);
        update_simple_key(g, g.buttonMenu.isPressed, T1_IO_GAMEPAD_MENU);
        update_simple_key(g, g.buttonOptions.isPressed, T1_IO_GAMEPAD_OPTIONS);
        
        T1_io_register_key_move_to_pos(
            T1_IO_GAMEPAD_LEFTTHUMBSTICK,
                g.leftThumbstick.xAxis.value,
                g.leftThumbstick.yAxis.value); 
        T1_io_register_key_move_to_pos(
            T1_IO_GAMEPAD_RIGHTTHUMBSTICK,
                g.rightThumbstick.xAxis.value,
                g.rightThumbstick.yAxis.value);
    }
}
#elif T1_GAMEPAD_ACTIVE == T1_INACTIVE
#else
#error
#endif
