#include "T1_std.h"
#include "T1_mem.h"
#include "T1_log.h"
#include "T1_objc.h"
#include "T1_io.h"
#include "T1_global.h"
#include "T1_ui_widget.h"
#include "T1_zsprite.h"
#include "T1_client.h"
#include "T1_os.h"
#include "T1_gpu.h"
#include "T1_gameloop.h"

#include <sys/mman.h>

#define T1NSUserDomainMask 1
#define T1NSUTF8StringEncoding 4
#define T1NSModalPanelWindowLevel 8
#define T1NSApplicationSupportDirectory 14
#define T1NSEventModifierFlagShift (1 << 17)
#define T1NSWindowStyleMaskFullScreen (1 << 14)
#define T1NSWindowAnimationBehaviorNone 2
#define T1NSBackingStoreBuffered 2
#define T1NSWindowStyleMaskTitled (1 << 0)
#define T1NSWindowStyleMaskClosable (1 << 1)
#define T1NSWindowStyleMaskResizable (1 << 3)
#define T1MTLPixelFormatDepth32Float 252

typedef struct {
    void * class_T1_ns_window; // T1NSWindow
    void * class_T1_ns_window_delegate; // T1NSWindowDelegate
    void * class_T1_mtk_view_delegate; // T1MTKViewDelegate
    void * class_ns_app; // NSApp
    void * class_ns_url; // NSURL;
    void * class_ns_alert; // NSAlert
    void * class_ns_file_manager; // NSFileManager
    void * class_ns_workspace; // NSWorkspace
    void * class_mtk_view; // MTKView
    void * sel_new; // new
    void * sel_window; // window
    void * sel_set_message_text; // setMessageText:
    void * sel_set_level; // setLevel:
    void * sel_make_key_and_order_front; // makeKeyAndOrderFront:
    void * sel_run_modal; // runModal
    void * sel_length; // length;
    void * sel_get_bytes_length; // getBytes:length:
    void * sel_object_at_index; // objectAtIndex:
    void * sel_shared_application; // sharedApplication
    void * sel_terminate; // terminate:
    void * sel_default_manager; // defaultManager
    void * sel_current_dir_path; // currentDirPath
    void * sel_location_in_window; // locationInWindow:
    void * sel_button_number; // buttonNumber:
    void * sel_modifier_flags; // modifierFlags;
    void * sel_file_url_with_path; // fileURLWithPath:
    void * sel_shared_workspace; // sharedWorkspace
    void * func_ns_search_path_dirs_in_domains; // NSSearchPathForDirectoriesInDomains
    void * func_mtl_create_system_default_device; // MTLCreateSystemDefaultDevice()
    void * sel_delta_y; // deltaY
    void * sel_open_url; // openURL:
    void * sel_content_view; // contentView
    void * sel_frame; // frame
    void * sel_get_key_code; // keyCode;
    void * sel_get_style_mask; // getStyleMask
    void * sel_toggle_full_screen; // toggleFullScreen:
    void * sel_set_animation_behavior; // setAnimationBehavior:
    void * sel_set_delegate; // setDelegate:
    void * sel_set_title; // setTitle:
    void * sel_make_main_window; // makeMainWindow
    void * sel_set_accepts_mouse_events;
    void * sel_set_ordered_index;
    void * sel_close;
    void * sel_set_content_view; // setContentView:
    void * sel_alloc;
    void * sel_init_with_content_rect_style_mask; // initWithContentRect:styleMask:backing:defer:
    void * sel_init_with_frame_device; // initWithFrame:device:
    void * sel_set_auto_resize_drawable; // setAutoResizeDrawable:
    void * sel_set_preferred_frames_per_second; // setPreferredFramesPerSecond:
    void * sel_set_enable_set_needs_display; // setEnableSetNeedsDisplay:
    void * sel_set_depth_stencil_pixel_format; // setDepthStencilPixelFormat:
    void * sel_set_clear_depth; // setClearDepth:
    void * sel_set_paused;
    void * sel_set_needs_display; 
    b8 framework_good;
} T1OSMacosState;

static T1OSMacosState * T1_os_macos_s = NULL;

static void T1_mtkviewdel_update_final_window_size(
    void * selfptr,
    void * selectptr)
{
    (void)selfptr; (void)selectptr;
    
    T1_gpu_update_final_window_size();
}

static void T1_mtkviewdel_update_render_view_size(
    void * selfptr,
    void * selectptr,
    s32 at_i)
{
    (void)selfptr; (void)selectptr;
    
    T1_gpu_update_render_view_size(at_i);
}

static void T1_mtkviewdel_draw_in_mtk_view(
    void * selfptr,
    void * selectptr,
    void * mtk_view)
{
    (void)selfptr; (void)selectptr;
    
    T1_gpu_draw_in_mtk_view(mtk_view);
}

static void T1_mtkviewdel_mtkview_drawable_size_will_change(
    void * selfptr,
    void * selectptr,
    void * mtk_view,
    T1ObjcPairf64 size)
{
    (void)selfptr; (void)selectptr;
    (void)mtk_view;
    (void)size;
    
    // pass
}

static u32 T1_apple_keycode_to_tokone_keycode(
    u32 apple_key)
{
    #if T1_LOG_ASSERTS_ACTIVE == T1_ACTIVE
    char err_msg[128];
    #elif T1_LOG_ASSERTS_ACTIVE == T1_INACTIVE
    #else
    #error
    #endif
    
    switch (apple_key) {
        case   0: return T1_IO_KEYBOARD_A;
        case   1: return T1_IO_KEYBOARD_S;
        case   2: return T1_IO_KEYBOARD_D;
        case   3: return T1_IO_KEYBOARD_F;
        case   4: return T1_IO_KEYBOARD_H;
        case   5: return T1_IO_KEYBOARD_G;
        case   6: return T1_IO_KEYBOARD_Z;
        case   7: return T1_IO_KEYBOARD_X;
        case   8: return T1_IO_KEYBOARD_C;
        case   9: return T1_IO_KEYBOARD_V;
        case  10: return T1_IO_KEYBOARD_UNKNOWNBTN;
        case  11: return T1_IO_KEYBOARD_B;
        case  12: return T1_IO_KEYBOARD_Q;
        case  13: return T1_IO_KEYBOARD_W;
        case  14: return T1_IO_KEYBOARD_E;
        case  15: return T1_IO_KEYBOARD_R;
        case  16: return T1_IO_KEYBOARD_Y;
        case  17: return T1_IO_KEYBOARD_T;
        case  18: return T1_IO_KEYBOARD_1;
        case  19: return T1_IO_KEYBOARD_2;
        case  20: return T1_IO_KEYBOARD_3;
        case  21: return T1_IO_KEYBOARD_4;
        case  22: return T1_IO_KEYBOARD_6;
        case  23: return T1_IO_KEYBOARD_5;
        case  24: return T1_IO_KEYBOARD_HAT;
        case  25: return T1_IO_KEYBOARD_9;
        case  26: return T1_IO_KEYBOARD_7;
        case  27: return T1_IO_KEYBOARD_MINUS;
        case  28: return T1_IO_KEYBOARD_8;
        case  29: return T1_IO_KEYBOARD_0;
        case  30: return T1_IO_KEYBOARD_OPENSQUARE;
        case  31: return T1_IO_KEYBOARD_O;
        case  32: return T1_IO_KEYBOARD_U;
        case  33: return T1_IO_KEYBOARD_AT;
        case  34: return T1_IO_KEYBOARD_I;
        case  35: return T1_IO_KEYBOARD_P;
        case  36: return T1_IO_KEYBOARD_ENTER;
        case  37: return T1_IO_KEYBOARD_L;
        case  38: return T1_IO_KEYBOARD_J;
        case  39: return T1_IO_KEYBOARD_COLON;
        case  40: return T1_IO_KEYBOARD_K;
        case  41: return T1_IO_KEYBOARD_SEMICOLON;
        case  42: return T1_IO_KEYBOARD_CLOSESQUARE;
        case  45: return T1_IO_KEYBOARD_N;
        case  46: return T1_IO_KEYBOARD_M;
        case  43: return T1_IO_KEYBOARD_COMMA;
        case  47: return T1_IO_KEYBOARD_FULLSTOP;
        case  48: return T1_IO_KEYBOARD_TAB;
        case  44: return T1_IO_KEYBOARD_BACKSLASH;
        case  49: return T1_IO_KEYBOARD_SPACEBAR;
        case  50: return T1_IO_KEYBOARD_TILDE;
        case  51: return T1_IO_KEYBOARD_BACKSPACE;
        case  53: return T1_IO_KEYBOARD_ESCAPE;
        // NUMPAD
        case  65: return T1_IO_KEYBOARD_FULLSTOP;
        case  67: return T1_IO_KEYBOARD_ASTERISK;
        case  69: return T1_IO_KEYBOARD_PLUS;	
        case  71: return T1_IO_KEYBOARD_NUMPADCLEAR;
        case  75: return T1_IO_KEYBOARD_YENSIGN;
        case  76: return T1_IO_KEYBOARD_ENTER;
        case  78: return T1_IO_KEYBOARD_MINUS;
        // NUMPAD NUMBERS
        case  82: return T1_IO_KEYBOARD_0;
        case  83: return T1_IO_KEYBOARD_1;
        case  84: return T1_IO_KEYBOARD_2;
        case  85: return T1_IO_KEYBOARD_3;
        case  86: return T1_IO_KEYBOARD_4;
        case  87: return T1_IO_KEYBOARD_5;
        case  88: return T1_IO_KEYBOARD_6;
        case  89: return T1_IO_KEYBOARD_7;
        case  91: return T1_IO_KEYBOARD_8;
        case  92: return T1_IO_KEYBOARD_9;
        case  93: return T1_IO_KEYBOARD_YENSIGN;
        case  94: return T1_IO_KEYBOARD_UNDERSCORE;
        case  99: return T1_IO_KEYBOARD_F3;
        case  96: return T1_IO_KEYBOARD_F5;
        case  97: return T1_IO_KEYBOARD_F6;
        case  98: return T1_IO_KEYBOARD_F7;
        case 100: return T1_IO_KEYBOARD_F8;
        case 101: return T1_IO_KEYBOARD_F9;
        case 103: return T1_IO_KEYBOARD_F11;
        case 102: return T1_IO_KEYBOARD_ROMAJI;
        case 104: return T1_IO_KEYBOARD_KANA;
        case 109: return T1_IO_KEYBOARD_F10;
        case 111: return T1_IO_KEYBOARD_F12;
        case 114: return T1_IO_KEYBOARD_INSERT;
        case 115: return T1_IO_KEYBOARD_HOME;
        case 116: return T1_IO_KEYBOARD_PAGEUP;
        case 118: return T1_IO_KEYBOARD_F4;
        case 119: return T1_IO_KEYBOARD_END;
        case 120: return T1_IO_KEYBOARD_F2;
        case 121: return T1_IO_KEYBOARD_PAGEDOWN;
        case 122: return T1_IO_KEYBOARD_F1;
        case 123: return T1_IO_KEYBOARD_LEFTARROW;
        case 124: return T1_IO_KEYBOARD_RIGHTARROW;
        case 125: return T1_IO_KEYBOARD_DOWNARROW;
        case 126: return T1_IO_KEYBOARD_UPARROW;
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

static void T1_os_macos_ns_window_key_down(
    void * selfptr,
    void * selectorptr,
    void * event)
{
    (void)selfptr;
    (void)selectorptr;
    
    uintptr_t apple_keycode = T1_objc_msg(
        event,
        T1_os_macos_s->sel_get_key_code);
    
    T1_io_register_keydown(
        T1_apple_keycode_to_tokone_keycode((u32)apple_keycode),
        /* debounces: */ 0);
}

static void T1_os_macos_ns_window_key_up(
    void * selfptr,
    void * selectorptr,
    void * event)
{
    (void)selfptr;
    (void)selectorptr;
    
    uintptr_t apple_keycode = T1_objc_msg(
        event,
        T1_os_macos_s->sel_get_key_code);
    
    T1_io_register_keyup(
        T1_apple_keycode_to_tokone_keycode(
            (u32)apple_keycode),
        /* debounces: */ 0);
}

static void T1_os_macos_ns_window_mouse_moved(
    void * selfptr,
    void * selectorptr,
    void * event)
{
    (void)selfptr;
    (void)selectorptr;
    
    // event is an NSEvent *
    if (T1_global->block_mouse) {
        return;
    }
    
    T1ObjcPairf64 window_location =
        T1_objc_msg_get_pairf64(
            event,
            T1_os_macos_s->sel_location_in_window);
    
    T1_io_register_key_move_to_pos(
        T1_IO_MOUSE,
        (f32)window_location.a,
        (f32)window_location.b);
}

static void T1_os_macos_ns_window_mouse_down(
    void * selfptr,
    void * selectorptr,
    void * event)
{
    (void)selfptr;
    (void)selectorptr;
    (void)event;
    
    T1_io_register_keydown(T1_IO_MOUSE_LCLICK, 0);
}

static void T1_os_macos_ns_window_mouse_up(
    void * selfptr,
    void * selectorptr,
    void * event)
{
    (void)selfptr;
    (void)selectorptr;
    (void)event;
    
    T1_io_register_keyup(
        T1_IO_MOUSE_LCLICK,
        /* debounces: */ 0);
}

static void T1_os_macos_ns_window_right_mouse_down(
    void * selfptr,
    void * selectorptr,
    void * event)
{
    (void)selfptr;
    (void)selectorptr;
    (void)event;
    
    T1_io_register_keydown(T1_IO_MOUSE_RCLICK, 0);
}

static void T1_os_macos_ns_window_right_mouse_up(
    void * selfptr,
    void * selectorptr,
    void * event)
{
    (void)selfptr;
    (void)selectorptr;
    (void)event;
    
    T1_io_register_keyup(
        T1_IO_MOUSE_RCLICK,
            /* debounces: */ 0);
}

static void T1_os_macos_ns_window_other_mouse_down(
    void * selfptr,
    void * selectorptr,
    void * event)
{
    (void)selfptr;
    (void)selectorptr;
    (void)event;
    
    uintptr_t button_num = T1_objc_msg(
        event,
        T1_os_macos_s->sel_button_number);
    
    T1_log_assert(button_num >= 2);
    
    if (button_num < 2) { return; }
    button_num -= 2;
    
    if (button_num >= 4) { return; }
    
    T1_io_register_keydown(
        T1_IO_MOUSE_OTHERCLICK1 + (u32)button_num, 0);
}

static void T1_os_macos_ns_window_other_mouse_up(
    void * selfptr,
    void * selectorptr,
    void * event)
{
    (void)selfptr;
    (void)selectorptr;
    (void)event;
    
    uintptr_t button_num = T1_objc_msg(
        event,
        T1_os_macos_s->sel_button_number);
    
    T1_log_assert(button_num >= 2);
    
    if (button_num < 2) { return; }
    button_num -= 2;
    
    if (button_num >= 4) { return; }
    
    T1_io_register_keyup(
        T1_IO_MOUSE_OTHERCLICK1 + (u32)button_num,
        /* debounces: */ 0);
}

static void T1_os_macos_ns_window_flags_changed(
    void * selfptr,
    void * selectorptr,
    void * event)
{
    (void)selfptr;
    (void)selectorptr;
    (void)event;
    
    // [event modifierFlags];
    uintptr_t modifiers = T1_objc_msg(
        event,
        T1_os_macos_s->sel_modifier_flags);
    
    if (modifiers & T1NSEventModifierFlagShift) {
        T1_io_register_keydown(T1_IO_KEYBOARD_SHIFT, 0);
    } else if (T1_io_key_is_down(T1_IO_KEYBOARD_SHIFT, -1)) {
        T1_io_register_keyup(
            T1_IO_KEYBOARD_SHIFT,
            /* debounces: */ 0);
    }
}

static void T1_os_macos_ns_window_scroll_wheel(
    void * eventptr,
    void * selectorptr,
    void * event)
{
    (void)eventptr;
    (void)selectorptr;
    
    f32 delta = (f32)T1_objc_msg_get_f64(
        event,
        T1_os_macos_s->sel_delta_y);
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

static T1ObjcQuadf64 T1_os_macos_ns_window_get_content_frame(
    void * self_ptr)
{
    T1ObjcQuadf64 out;
    out.a = 0.0;
    out.b = 0.0;
    out.c = 0.0;
    out.d = 0.0;
    
    // 1. Get contentView
    void * content_view = (void *)T1_objc_msg(
        self_ptr,
        T1_os_macos_s->sel_content_view);
    
    if (content_view) {
        // 2. Call [contentView frame] (returns NSRect: 4 doubles / 32 bytes)
        out = T1_objc_msg_get_quadf64(
            content_view,
            T1_os_macos_s->sel_frame);    
    }
    
    return out;
}

static f32 T1_os_macos_ns_window_get_width(
    void * self_ptr,
    void * selector)
{
    (void)selector;
    
    T1ObjcQuadf64 frame = T1_os_macos_ns_window_get_content_frame(self_ptr);
    
    return (f32)frame.c;
}

static f32 T1_os_macos_ns_window_get_height(
    void * self_ptr,
    void * selector)
{
    (void)selector;
    
    T1ObjcQuadf64 frame = T1_os_macos_ns_window_get_content_frame(self_ptr);
    
    return (f32)frame.d;
}

static u8 T1_os_macos_ns_window_always_true(
    void * self_ptr,
    void * selector)
{
    (void)self_ptr;
    (void)selector;
    
    return 1;
}

static void * window = NULL; // id<T1NSWindow>
static void * mtk_view = NULL; // id<MTKView>
static void * apple_gpu_delegate = NULL; // id<T1MTKViewDelegate>

static void T1_os_macos_ns_window_delegate_window_will_close(
    void * self_ptr,
    void * selector,
    void * notification)
{
    (void)self_ptr; (void)selector; (void)notification;
    
    T1_os_shutdown();
    
    uint8_t write_succesful = false;
    T1_log_dump(&write_succesful);
    
    T1_os_close_app();
}

static void T1_os_macos_ns_window_delegate_window_will_enter_full_screen(
    void * self_ptr,
    void * selector,
    void * notification)
{
    (void)self_ptr; (void)selector; (void)notification;
    
    T1_ui_widget_delete_all();
    T1_zsprite_delete_all();
    T1_global->fullscreen = true;
}

static void T1_os_macos_ns_window_delegate_window_will_exit_full_screen(
    void * self_ptr,
    void * selector,
    void * notification)
{
    (void)self_ptr; (void)selector; (void)notification;
    
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

#if 0
static void T1_os_macos_ns_window_delegate_window_did_move(
    void * self_ptr,
    void * selector,
    void * notification)
{
    (void)self_ptr; (void)selector; (void)notification;
    
    T1ObjcQuadf64 frame = T1_objc_msg_get_quadf64(
        window,
        T1_os_macos_s->sel_frame);
    
    T1_global_update_window_pos(
        (f32)frame.a,
        (f32)frame.b);
}
#endif

static T1ObjcPairf64
T1_os_macos_ns_window_delegate_window_will_resize_to_size(
    void * self_ptr,
    void * selector,
    void * sender,
    T1ObjcPairf64 frame_size)
{
    (void)self_ptr; (void)selector; (void)sender;
    
    T1_os_layer_start_window_resize(
        T1_os_get_current_time_us());
    
    return frame_size;
}

static void T1_os_macos_ns_window_delegate_window_did_resize(
    void * self_ptr,
    void * selector,
    void * notification)
{
    (void)self_ptr; (void)selector; (void)notification;
    
    T1_global_update_window_size(
        /* float width: */
            T1_os_macos_ns_window_get_width(
                window, NULL),
        /* float height */
            T1_os_macos_ns_window_get_height(
                window, NULL),
        /* u64 at_timestamp_us: */
            T1_os_get_current_time_us());
    
    T1_gpu_update_final_window_size();
}

void T1_os_macos_init(void) {
    T1_os_macos_s = T1_mem_malloc_unmanaged(sizeof(T1OSMacosState));
    T1_std_memset(T1_os_macos_s, 0, sizeof(T1OSMacosState));
    
    T1_objc_open_framework_and_link_perma_good_val(
        "/System/Library/Frameworks/Foundation.framework/Foundation",
        &T1_os_macos_s->framework_good);
    T1_objc_open_framework_and_link_perma_good_val(
        "/System/Library/Frameworks/AppKit.framework/AppKit",
        &T1_os_macos_s->framework_good);
    
    T1_os_macos_s->sel_alloc = T1_objc_reg_sel("alloc");
    T1_log_assert(T1_os_macos_s->sel_alloc != NULL);
    
    T1_os_macos_s->sel_init_with_content_rect_style_mask =
        T1_objc_reg_sel("initWithContentRect:styleMask:backing:defer:");
    T1_log_assert(T1_os_macos_s->sel_init_with_content_rect_style_mask != NULL);
    T1_os_macos_s->sel_set_animation_behavior = T1_objc_reg_sel(
        "setAnimationBehavior:");
    T1_log_assert(T1_os_macos_s->sel_set_animation_behavior != NULL);
    
    // classes
    T1_os_macos_s->class_T1_ns_window =
        T1_objc_inherit_from_class(
            /* const char * base_class_name: */
                "NSWindow",
            /* const char * new_class_name: */
                "T1NSWindow");
    T1_objc_add_method(
        T1_os_macos_s->class_T1_ns_window,
        "canBecomeKeyWindow",
        (void *)T1_os_macos_ns_window_always_true,
        "B@:");
    T1_objc_add_method(
        T1_os_macos_s->class_T1_ns_window,
        "canBecomeMainWindow",
        (void *)T1_os_macos_ns_window_always_true,
        "B@:");
    T1_objc_add_method(
        T1_os_macos_s->class_T1_ns_window,
        "acceptsFirstResponder",
        (void *)T1_os_macos_ns_window_always_true,
        "B@:");
    T1_objc_add_method(
        T1_os_macos_s->class_T1_ns_window,
        "mouseMoved:",
        (void *)T1_os_macos_ns_window_mouse_moved,
        "v@:@");
    T1_objc_add_method(
        T1_os_macos_s->class_T1_ns_window,
        "mouseDragged:",
        (void *)T1_os_macos_ns_window_mouse_moved,
        "v@:@");
    T1_objc_add_method(
        T1_os_macos_s->class_T1_ns_window,
        "mouseDown:",
        (void *)T1_os_macos_ns_window_mouse_down,
        "v@:@");
    T1_objc_add_method(
        T1_os_macos_s->class_T1_ns_window,
        "mouseUp:",
        (void *)T1_os_macos_ns_window_mouse_up,
        "v@:@");
    T1_objc_add_method(
        T1_os_macos_s->class_T1_ns_window,
        "rightMouseDown:",
        (void *)T1_os_macos_ns_window_right_mouse_down,
        "v@:@");
    T1_objc_add_method(
        T1_os_macos_s->class_T1_ns_window,
        "rightMouseUp:",
        (void *)T1_os_macos_ns_window_right_mouse_up,
        "v@:@");
    T1_objc_add_method(
        T1_os_macos_s->class_T1_ns_window,
        "otherMouseDown:",
        (void *)T1_os_macos_ns_window_other_mouse_down,
        "v@:@");
    T1_objc_add_method(
        T1_os_macos_s->class_T1_ns_window,
        "otherMouseUp:",
        (void *)T1_os_macos_ns_window_other_mouse_up,
        "v@:@");
    T1_objc_add_method(
        T1_os_macos_s->class_T1_ns_window,
        "keyDown:",
        (void *)T1_os_macos_ns_window_key_down,
        "v@:@");
    T1_objc_add_method(
        T1_os_macos_s->class_T1_ns_window,
        "keyUp:",
        (void *)T1_os_macos_ns_window_key_up,
        "v@:@");
    T1_objc_add_method(
        T1_os_macos_s->class_T1_ns_window,
        "flagsChanged:",
        (void *)T1_os_macos_ns_window_flags_changed,
        "v@:@");
    T1_objc_add_method(
        T1_os_macos_s->class_T1_ns_window,
        "scrollWheel:",
        (void *)T1_os_macos_ns_window_scroll_wheel,
        "v@:@");
    T1_objc_commit_child_class(
        T1_os_macos_s->class_T1_ns_window);
    
    T1_os_macos_s->class_T1_ns_window_delegate =
        T1_objc_inherit_from_class(
            /* const char * base_class_name: */
                "NSObject",
            /* const char * new_class_name: */
                "T1NSWindowDelegate");
    T1_objc_add_method(
        T1_os_macos_s->class_T1_ns_window_delegate,
        "windowWillClose:",
        (void *)T1_os_macos_ns_window_delegate_window_will_close,
        "v@:@");
    T1_objc_add_method(
        T1_os_macos_s->class_T1_ns_window_delegate,
        "windowWillEnterFullScreen:",
        (void *)T1_os_macos_ns_window_delegate_window_will_enter_full_screen,
        "v@:@");
    T1_objc_add_method(
        T1_os_macos_s->class_T1_ns_window_delegate,
        "windowWillExitFullScreen:",
        (void *)T1_os_macos_ns_window_delegate_window_will_exit_full_screen,
        "v@:@");
    T1_objc_add_method(
        T1_os_macos_s->class_T1_ns_window_delegate,
        "windowWillResize:toSize:",
        (void *)T1_os_macos_ns_window_delegate_window_will_resize_to_size,
        "{CGSize=dd}@:@{CGSize=dd}");
    T1_objc_add_method(
        T1_os_macos_s->class_T1_ns_window_delegate,
        "windowDidResize:",
        (void *)T1_os_macos_ns_window_delegate_window_did_resize,
        "v@:@");
    T1_objc_commit_child_class(
        T1_os_macos_s->class_T1_ns_window_delegate);
    
    T1_os_macos_s->class_T1_mtk_view_delegate =
        T1_objc_inherit_from_class(
            /* const char * base_class_name: */
                "NSObject",
            /* const char * new_class_name: */
                "T1MTKViewDelegate");
    T1_objc_add_method(
        T1_os_macos_s->class_T1_mtk_view_delegate,
        "updateFinalWindowSize",
        (void *)T1_mtkviewdel_update_final_window_size,
        "v@:");
    T1_objc_add_method(
        T1_os_macos_s->class_T1_mtk_view_delegate,
        "updateRenderViewSize:",
        (void *)T1_mtkviewdel_update_render_view_size,
        "v@:i");
    T1_objc_add_method(
        T1_os_macos_s->class_T1_mtk_view_delegate,
        "drawInMTKView:",
        (void *)T1_mtkviewdel_draw_in_mtk_view,
        "v@:@");
    T1_objc_add_method(
        T1_os_macos_s->class_T1_mtk_view_delegate,
        "mtkView:drawableSizeWillChange:",
        (void *)T1_mtkviewdel_mtkview_drawable_size_will_change,
        "v@:@{CGSize=dd}");
    T1_objc_commit_child_class(
        T1_os_macos_s->class_T1_mtk_view_delegate);
    
    T1_os_macos_s->class_ns_app = T1_objc_get_class(
        "NSApplication");
    T1_os_macos_s->class_ns_url = T1_objc_get_class(
        "NSURL"); // NSURL;
    T1_log_assert(T1_os_macos_s->class_ns_app != NULL);
    T1_os_macos_s->class_ns_alert = T1_objc_get_class(
        "NSAlert");
    T1_log_assert(T1_os_macos_s->class_ns_alert != NULL);
    T1_os_macos_s->class_ns_workspace = T1_objc_get_class(
        "NSWorkspace");
    T1_os_macos_s->class_mtk_view = T1_objc_get_class(
        "MTKView");
    T1_log_assert(T1_os_macos_s->class_mtk_view != NULL);
    
    // selectors
    T1_os_macos_s->sel_new = T1_objc_reg_sel(
        "new");
    T1_log_assert(T1_os_macos_s->sel_new != NULL);
    T1_os_macos_s->sel_window = T1_objc_reg_sel(
        "window");
    T1_log_assert(T1_os_macos_s->sel_window != NULL);
    T1_os_macos_s->sel_set_message_text = T1_objc_reg_sel(
        "setMessageText:");
    T1_log_assert(T1_os_macos_s->
        sel_set_message_text != NULL);
    T1_os_macos_s->sel_set_level = T1_objc_reg_sel(
        "setLevel:");
    T1_log_assert(T1_os_macos_s->sel_set_level != NULL);
    T1_os_macos_s->sel_make_key_and_order_front = T1_objc_reg_sel(
        "makeKeyAndOrderFront:");
    T1_log_assert(T1_os_macos_s->
        sel_make_key_and_order_front != NULL);
    T1_os_macos_s->sel_run_modal = T1_objc_reg_sel(
        "runModal");
    T1_log_assert(T1_os_macos_s->sel_run_modal != NULL);
    T1_os_macos_s->sel_length = T1_objc_reg_sel(
        "length");
    T1_log_assert(T1_os_macos_s->sel_length != NULL);
    T1_os_macos_s->sel_get_bytes_length = T1_objc_reg_sel(
        "getBytes:length:");
    T1_log_assert(T1_os_macos_s->sel_get_bytes_length != NULL);
    T1_os_macos_s->sel_object_at_index = T1_objc_reg_sel(
        "objectAtIndex:");
    T1_os_macos_s->sel_shared_application = T1_objc_reg_sel(
        "sharedApplication");
    T1_log_assert(T1_os_macos_s->sel_object_at_index != NULL);
    T1_os_macos_s->sel_terminate = T1_objc_reg_sel(
        "terminate:");
    T1_log_assert(T1_os_macos_s->sel_terminate != NULL);
    T1_os_macos_s->sel_default_manager =
        T1_objc_reg_sel("defaultManager");
    T1_log_assert(T1_os_macos_s->
        sel_default_manager != NULL);
    T1_os_macos_s->sel_current_dir_path = T1_objc_reg_sel(
        "currentDirPath");
    T1_os_macos_s->sel_location_in_window = T1_objc_reg_sel(
        "locationInWindow");
    T1_os_macos_s->sel_button_number = T1_objc_reg_sel(
        "buttonNumber:");
    T1_os_macos_s->sel_modifier_flags = T1_objc_reg_sel(
        "modifierFlags");
    T1_os_macos_s->sel_file_url_with_path = T1_objc_reg_sel(
        "fileURLWithPath:");
    T1_os_macos_s->sel_shared_workspace = T1_objc_reg_sel(
        "sharedWorkspace");
    T1_log_assert(T1_os_macos_s->
        sel_current_dir_path != NULL);
    
    T1_os_macos_s->func_ns_search_path_dirs_in_domains =
        T1_objc_get_func(
            "NSSearchPathForDirectoriesInDomains");
    
    T1_os_macos_s->func_mtl_create_system_default_device =
        T1_objc_get_func(
            "MTLCreateSystemDefaultDevice");
    
    T1_os_macos_s->sel_delta_y = T1_objc_reg_sel(
        "deltaY");
    T1_os_macos_s->sel_open_url = T1_objc_reg_sel(
        "openURL:");
    T1_os_macos_s->sel_content_view = T1_objc_reg_sel(
        "contentView");
    T1_os_macos_s->sel_frame = T1_objc_reg_sel(
        "frame");
    T1_os_macos_s->sel_get_key_code = T1_objc_reg_sel(
        "keyCode");
    T1_os_macos_s->sel_get_style_mask = T1_objc_reg_sel(
        "styleMask");
    T1_os_macos_s->sel_toggle_full_screen = T1_objc_reg_sel(
        "toggleFullScreen:");
    
    T1_os_macos_s->sel_set_delegate = T1_objc_reg_sel(
        "setDelegate:");
    T1_os_macos_s->sel_set_title = T1_objc_reg_sel(
        "setTitle:");
    T1_os_macos_s->sel_make_main_window = T1_objc_reg_sel(
        "makeMainWindow");
    T1_os_macos_s->sel_set_accepts_mouse_events = T1_objc_reg_sel(
        "setAcceptsMouseMovedEvents:");
    T1_os_macos_s->sel_set_ordered_index = T1_objc_reg_sel(
        "setOrderedIndex:");
    T1_os_macos_s->sel_close = T1_objc_reg_sel("close");
    T1_os_macos_s->sel_set_content_view =
        T1_objc_reg_sel("setContentView:");
    
    
    T1_os_macos_s->sel_init_with_frame_device = T1_objc_reg_sel(
        "initWithFrame:device:");
    T1_os_macos_s->sel_set_auto_resize_drawable = T1_objc_reg_sel(
        "setAutoResizeDrawable:");
    T1_os_macos_s->sel_set_preferred_frames_per_second = T1_objc_reg_sel(
        "setPreferredFramesPerSecond:");
    T1_os_macos_s->sel_set_enable_set_needs_display = T1_objc_reg_sel(
        "setEnableSetNeedsDisplay:");
    T1_os_macos_s->sel_set_depth_stencil_pixel_format = T1_objc_reg_sel(
        "setDepthStencilPixelFormat:");
    T1_os_macos_s->sel_set_clear_depth = T1_objc_reg_sel(
        "setClearDepth:");
    T1_os_macos_s->sel_set_paused = T1_objc_reg_sel(
        "setPaused:");
    T1_os_macos_s->sel_set_needs_display = T1_objc_reg_sel(
        "setNeedsDisplay:");
    
    T1_log_assert(T1_os_macos_s->
        func_ns_search_path_dirs_in_domains != NULL);
}

void T1_os_request_messagebox(
    const char * message)
{
    void * alert = (void *)T1_objc_msg(
        T1_os_macos_s->class_ns_alert,
        T1_os_macos_s->sel_new);
    void * ns_msg = T1_objc_nsstring_construct(message);
    T1_objc_msg_1arg(
        alert,
        T1_os_macos_s->sel_set_message_text,
        (uintptr_t)ns_msg);
    //    void * window = (void *)T1_objc_msg(
    //        alert, T1_os_macos_s->sel_window);
    
    T1_objc_msg_1arg(
        window,
        T1_os_macos_s->sel_set_level,
        T1NSModalPanelWindowLevel);
    
    T1_objc_msg_1arg(
        window,
        T1_os_macos_s->sel_make_key_and_order_front,
        0);
    
    T1_objc_msg(alert, T1_os_macos_s->sel_run_modal);
}

#if T1_GAMEPAD_ACTIVE == T1_ACTIVE
static void update_simple_key(
    u8 ispressed,
    T1IOKey T1_io_key)
{
    if (ispressed) {
        T1_io_register_keydown(
            T1_io_key,
            /* debounces: */ 1);
    } else {
        T1_io_register_keyup(
            T1_io_key,
            /* debounces: */ 1);
    }
}

typedef struct {
    void * class_GCController;
    void * sel_current;
    void * sel_extendedgamepad;
    void * sel_left_shoulder;
    void * sel_right_shoulder;
    void * sel_left_trigger;
    void * sel_right_trigger;
    void * sel_left_thumbstick_button;
    void * sel_right_thumbstick_button;
    void * sel_ispressed;
    void * sel_dpad;
    void * sel_left;
    void * sel_right;
    void * sel_up;
    void * sel_down;
    void * sel_button_a;
    void * sel_button_b;
    void * sel_button_x;
    void * sel_button_y;
    void * sel_button_home;
    void * sel_button_menu;
    void * sel_button_options;
    void * sel_left_thumbstick;
    void * sel_right_thumbstick;
    void * sel_xaxis;
    void * sel_yaxis;
    void * sel_value;
    u8 good;
} ObjCFrameworkGCC;

static ObjCFrameworkGCC * T1_mpl_objc = NULL;

static void T1_os_setup_obj_frameworks(void) {
    // we're assuming no one calls this a 2nd time
    T1_log_assert(!T1_mpl_objc);
    
    T1_mpl_objc = T1_mem_malloc_unmanaged(
        sizeof(ObjCFrameworkGCC));
    if (!T1_mpl_objc) { return; }
    T1_std_memset(T1_mpl_objc, 0, sizeof(ObjCFrameworkGCC));
    
    T1_objc_open_framework_and_link_perma_good_val(
        "/System/Library/Frameworks/GameController.framework/GameController",
        &T1_mpl_objc->good);
    
    T1_mpl_objc->class_GCController = T1_objc_get_class("GCController");
    T1_mpl_objc->sel_current = T1_objc_reg_sel("current");
    T1_mpl_objc->sel_extendedgamepad = T1_objc_reg_sel("extendedGamepad");
    T1_mpl_objc->sel_dpad = T1_objc_reg_sel("dpad");
    T1_mpl_objc->sel_left_shoulder = T1_objc_reg_sel("leftShoulder");
    T1_mpl_objc->sel_right_shoulder = T1_objc_reg_sel("rightShoulder");
    T1_mpl_objc->sel_left_trigger = T1_objc_reg_sel("leftTrigger");
    T1_mpl_objc->sel_right_trigger = T1_objc_reg_sel("rightTrigger");
    T1_mpl_objc->sel_left_thumbstick_button = T1_objc_reg_sel("leftThumbstickButton");
    T1_mpl_objc->sel_right_thumbstick_button = T1_objc_reg_sel("rightThumbstickButton");
    T1_mpl_objc->sel_left = T1_objc_reg_sel("left");
    T1_mpl_objc->sel_right = T1_objc_reg_sel("right");
    T1_mpl_objc->sel_up = T1_objc_reg_sel("up");
    T1_mpl_objc->sel_down = T1_objc_reg_sel("down");
    T1_mpl_objc->sel_button_a = T1_objc_reg_sel("buttonA");
    T1_mpl_objc->sel_button_b = T1_objc_reg_sel("buttonB");
    T1_mpl_objc->sel_button_x = T1_objc_reg_sel("buttonX");
    T1_mpl_objc->sel_button_y = T1_objc_reg_sel("buttonY");
    T1_mpl_objc->sel_button_home = T1_objc_reg_sel("buttonHome");
    T1_mpl_objc->sel_button_menu = T1_objc_reg_sel("buttonMenu");
    T1_mpl_objc->sel_button_options = T1_objc_reg_sel("buttonOptions");
    T1_mpl_objc->sel_ispressed = T1_objc_reg_sel("isPressed");
    T1_mpl_objc->sel_left_thumbstick = T1_objc_reg_sel("leftThumbstick");
    T1_mpl_objc->sel_right_thumbstick = T1_objc_reg_sel("rightThumbstick");
    T1_mpl_objc->sel_xaxis = T1_objc_reg_sel("xAxis");
    T1_mpl_objc->sel_yaxis = T1_objc_reg_sel("yAxis");
    T1_mpl_objc->sel_value = T1_objc_reg_sel("value");
}

static void update_chain_key(
    void * objc_parent,
    void * objc_sel,
    T1IOKey T1_io_key)
{
    if (!objc_parent) { return; }
    
    void * objc_sub = (void *)T1_objc_msg(
        objc_parent,
        objc_sel);
    
    if (!objc_sub) { return; }
    
    uintptr_t isdown_u32 = T1_objc_msg(
        objc_sub,
        T1_mpl_objc->sel_ispressed);
    
    update_simple_key(isdown_u32 & 1, T1_io_key);
}

void T1_os_poll_gamepad_events(void) {
    
    if (!T1_mpl_objc) {
        T1_os_setup_obj_frameworks();
        return;
    }
    
    if (!T1_mpl_objc->good) {
        return;
    }
    
    // Grab the primary controller (like your EasySMX X05PRO)
    // GCController * c = [GCController current];
    void * c = (void *)T1_objc_msg(
        T1_mpl_objc->class_GCController,
        T1_mpl_objc->sel_current);
    
    if (!c) { return; }
    
    void * g = (void *)T1_objc_msg(
        c,
        T1_mpl_objc->sel_extendedgamepad);
    
    if (!g) { return; }
    
    void * dpad = (void *)T1_objc_msg(
        g,
        T1_mpl_objc->sel_dpad);
    
    if (dpad) {
        update_chain_key(dpad, T1_mpl_objc->sel_left, T1_IO_GAMEPAD_DPAD_LEFT);
        update_chain_key(dpad, T1_mpl_objc->sel_right, T1_IO_GAMEPAD_DPAD_RIGHT);
        update_chain_key(dpad, T1_mpl_objc->sel_up, T1_IO_GAMEPAD_DPAD_UP); 
        update_chain_key(dpad, T1_mpl_objc->sel_down, T1_IO_GAMEPAD_DPAD_DOWN);
    }
    
    update_chain_key(g, T1_mpl_objc->sel_left_shoulder, T1_IO_GAMEPAD_LSHOULDER);
    update_chain_key(g, T1_mpl_objc->sel_right_shoulder, T1_IO_GAMEPAD_RSHOULDER);
    update_chain_key(g, T1_mpl_objc->sel_left_trigger, T1_IO_GAMEPAD_LTRIGGER);
    update_chain_key(g, T1_mpl_objc->sel_right_trigger, T1_IO_GAMEPAD_RTRIGGER);
    update_chain_key(g, T1_mpl_objc->sel_left_thumbstick_button, T1_IO_GAMEPAD_LTHUMBSTICKBTN);
    update_chain_key(g, T1_mpl_objc->sel_right_thumbstick_button, T1_IO_GAMEPAD_RTHUMBSTICKBTN);
    
    update_chain_key(g, T1_mpl_objc->sel_button_a, T1_IO_GAMEPAD_A);
    update_chain_key(g, T1_mpl_objc->sel_button_b, T1_IO_GAMEPAD_B);
    update_chain_key(g, T1_mpl_objc->sel_button_x, T1_IO_GAMEPAD_X);
    update_chain_key(g, T1_mpl_objc->sel_button_y, T1_IO_GAMEPAD_Y);
    update_chain_key(g, T1_mpl_objc->sel_button_home, T1_IO_GAMEPAD_HOME);
    update_chain_key(g, T1_mpl_objc->sel_button_menu, T1_IO_GAMEPAD_MENU);
    update_chain_key(g, T1_mpl_objc->sel_button_options, T1_IO_GAMEPAD_OPTIONS);
    
    // thumbsticks
    void * xaxis = T1_objc_msgx2_get_ptr(
        g,
        T1_mpl_objc->sel_left_thumbstick,
        T1_mpl_objc->sel_xaxis);
    f32 xval = (f32)T1_objc_msg_get_f64(xaxis, T1_mpl_objc->sel_value);
    void * yaxis = T1_objc_msgx2_get_ptr(
        g,
        T1_mpl_objc->sel_left_thumbstick,
        T1_mpl_objc->sel_yaxis);
    f32 yval = (f32)T1_objc_msg_get_f64(yaxis, T1_mpl_objc->sel_value);
    T1_io_register_key_move_to_pos(
        T1_IO_GAMEPAD_LTHUMBSTICK,
        xval,
        yval);
    
    xaxis = T1_objc_msgx2_get_ptr(
        g,
        T1_mpl_objc->sel_right_thumbstick,
        T1_mpl_objc->sel_xaxis);
    yaxis = T1_objc_msgx2_get_ptr(
        g,
        T1_mpl_objc->sel_right_thumbstick,
        T1_mpl_objc->sel_yaxis);
    xval = (f32)T1_objc_msg_get_f64(xaxis, T1_mpl_objc->sel_value);
    yval = (f32)T1_objc_msg_get_f64(yaxis, T1_mpl_objc->sel_value);
    T1_io_register_key_move_to_pos(
        T1_IO_GAMEPAD_RTHUMBSTICK,
        xval,
        yval);
}
#elif T1_GAMEPAD_ACTIVE == T1_INACTIVE
#else
#error
#endif


void T1_os_get_writables_dir(
    char * recipient,
    const u32 recipient_size)
{
    typedef void *(* search_fn)(
        uintptr_t,
        uintptr_t,
        u8);
    search_fn fn = (search_fn)T1_os_macos_s->
        func_ns_search_path_dirs_in_domains;
    
    if (fn == NULL) { return; }
    
    void * paths = fn(
        T1NSApplicationSupportDirectory,
        T1NSUserDomainMask,
        1);
    
    void * lib_dir = (void *)T1_objc_msg_1arg(
        paths,
        T1_os_macos_s->sel_object_at_index,
        0);
    
    char * lib_dir_cstr = T1_objc_nsstring_to_cstring(lib_dir);
    
    T1_std_strcpy_cap(recipient, recipient_size, lib_dir_cstr);
    T1_std_strcat_cap(recipient, recipient_size, "/");
    T1_std_strcat_cap(recipient, recipient_size, T1_APP_NAME);
    
    T1_os_mkdir_if_not_exist(recipient);
}

void * T1_os_malloc_unaligned_block(
    const u64 size)
{
    void * return_value = mmap(
        /* void *: */
            NULL,
        /* size_t: */
            size,
        /* int prot: */
            PROT_READ | PROT_WRITE,
        /* int: */
            MAP_SHARED | MAP_ANONYMOUS,
        /* int: */
            -1,
        /* off_t: */
            0);
    
    if (return_value == MAP_FAILED) {
        return NULL;
    }
    
    return return_value;
}

void T1_os_close_app(void) {
    void * ns_app = (void *)T1_objc_msg(
        T1_os_macos_s->class_ns_app,
        T1_os_macos_s->sel_shared_application);
    
    T1_objc_msg_1arg(
        ns_app,
        T1_os_macos_s->sel_terminate,
        0);
}

void T1_os_get_cwd(char * recip, u32 recip_size) {
    
    void * manager = (void *)T1_objc_msg(
        T1_os_macos_s->class_ns_file_manager,
        T1_os_macos_s->sel_default_manager);
    void * nsstr_cwd = (void *)T1_objc_msg(
        manager,
        T1_os_macos_s->sel_current_dir_path);
    
    char * return_value = T1_objc_nsstring_to_cstring(
        nsstr_cwd);
    
    T1_std_strcpy_cap(recip, recip_size, return_value);
}

float T1_os_x_to_x(const float x) {
    return x;
}

float T1_os_y_to_y(const float y) {
    return y;
}

void T1_os_open_dir_in_file_explorer_window_if_possible(
    const char * folderpath)
{
    T1_log_append("Trying to open folder: ");
    T1_log_append(folderpath);
    T1_log_append_c8('\n');
    
    if (folderpath == NULL || folderpath[0] == '\0') {
        return;
    }
    
    void * nsstr_folderpath =
        T1_objc_nsstring_construct(folderpath);
    
    void * folder_url = (void *)T1_objc_msg_1arg(
        T1_os_macos_s->class_ns_url,
        T1_os_macos_s->sel_file_url_with_path,
        (uintptr_t)nsstr_folderpath);
    
    // [[NSWorkspace sharedWorkspace] openURL: folderURL];
    void * shared_ws = (void *)T1_objc_msg(
        T1_os_macos_s->class_ns_workspace,
        T1_os_macos_s->sel_shared_workspace);
    
    T1_objc_msg_1arg(
        shared_ws,
        T1_os_macos_s->sel_open_url,
        (uintptr_t)folder_url);
}

void T1_os_enter_fullscreen(void) {
    uintptr_t stylemask = T1_objc_msg(
        window,
        T1_os_macos_s->sel_get_style_mask);
    if ((stylemask & T1NSWindowStyleMaskFullScreen) == 0) {
        T1_objc_msg_1arg(
            window,
            T1_os_macos_s->sel_toggle_full_screen,
            (uintptr_t)window);
    }
}

void T1_os_toggle_fullscreen(void) {
    T1_objc_msg_1arg(
        window,
        T1_os_macos_s->sel_toggle_full_screen,
        (uintptr_t)window);
}

__attribute__((no_sanitize("address")))
void T1_os_create_main_window(
    b8 * good)
{
    *good = 0;
    
    if (!T1_global) { return; }
    
    // NSScreen *screen = [[NSScreen screens] objectAtIndex:0];
    T1ObjcQuadf64 window_rect = T1_objc_quadf64_construct(
        /* x: */ T1_global->window_left,
        /* y: */ T1_global->window_bottom,
        /* width: */ T1_global->window_wh[0],
        /* height: */ T1_global->window_wh[1]);
    
    void * window_alloc = (void *)T1_objc_msg(
        T1_os_macos_s->class_T1_ns_window,
        T1_os_macos_s->sel_alloc);
    
    window = (void *)T1_objc_msg_1quadf64_3arg(
            window_alloc,
            T1_os_macos_s->sel_init_with_content_rect_style_mask,
        /* initWithContentRect: */
            window_rect,
        /* styleMask: */ 
            T1NSWindowStyleMaskTitled    |
            T1NSWindowStyleMaskClosable  |
            T1NSWindowStyleMaskResizable,
        /* backing: */
            T1NSBackingStoreBuffered,
        /* defer: */ 0);
    
    T1_objc_msg_1arg(
        window,
        T1_os_macos_s->sel_set_animation_behavior,
        T1NSWindowAnimationBehaviorNone);
    
    void * window_delegate = (void *)T1_objc_msg(
        T1_os_macos_s->class_T1_ns_window_delegate,
        T1_os_macos_s->sel_new);
    
    #ifdef T1_APP_NAME
    void * nsstring_app_name = T1_objc_nsstring_construct(T1_APP_NAME);
    #else
    #error
    #endif
    
    T1_objc_msg_1arg(
        window,
        T1_os_macos_s->sel_set_delegate,
        (uintptr_t)window_delegate);
    T1_objc_msg_1arg(
        window,
        T1_os_macos_s->sel_set_title,
        (uintptr_t)nsstring_app_name);
    T1_objc_msg(
        window,
        T1_os_macos_s->sel_make_main_window);
    
    T1_objc_msg_1arg(
        window,
        T1_os_macos_s->sel_set_accepts_mouse_events,
        1);
    
    T1_objc_msg_1arg(
        window,
        T1_os_macos_s->sel_set_ordered_index,
        0);
    
    T1_objc_msg_1arg(
        window,
        T1_os_macos_s->sel_make_key_and_order_front,
        0);
    
    *good = 1;
}

void T1_os_destroy_main_window_if_possible(void) {
    if (window) {
        T1_objc_msg(window, T1_os_macos_s->sel_close);
    }
}

void T1_os_link_gpu_to_main_window(
    c8 * errmsg,
    u32 errmsg_cap,
    b8 * good)
{
    *good = 0;
    
    typedef void *(* mtl_fn)(void);
    mtl_fn fn = (mtl_fn)T1_os_macos_s->
        func_mtl_create_system_default_device;
    
    if (fn == NULL) { return; }
    void * metal_device_for_window = fn();
    
    T1ObjcQuadf64 window_rect = T1_objc_quadf64_construct(
        /* x: */ T1_global->window_left,
        /* y: */ T1_global->window_bottom,
        /* width: */ T1_global->window_wh[0],
        /* height: */ T1_global->window_wh[1]);
    
    mtk_view = (void *)T1_objc_msg(
        T1_os_macos_s->class_mtk_view,
        T1_os_macos_s->sel_alloc);
    
    mtk_view = (void *)T1_objc_msg_1quadf64_1arg(
        mtk_view,
        T1_os_macos_s->sel_init_with_frame_device,
        window_rect,
        (uintptr_t)metal_device_for_window);
    
    T1_objc_msg_1arg(mtk_view,
        T1_os_macos_s->sel_set_auto_resize_drawable, true);
    T1_objc_msg_1arg(mtk_view,
        T1_os_macos_s->sel_set_preferred_frames_per_second, 60);
    T1_objc_msg_1arg(mtk_view,
        T1_os_macos_s->sel_set_enable_set_needs_display, false);
    
    T1_objc_msg_1arg(mtk_view,
        T1_os_macos_s->sel_set_depth_stencil_pixel_format,
        T1MTLPixelFormatDepth32Float);
    
    T1_objc_msg_1arg(mtk_view,
        T1_os_macos_s->sel_set_clear_depth,
        T1_GLOBAL_CLEARDEPTH);
    T1_objc_msg_1arg(mtk_view,
        T1_os_macos_s->sel_set_paused, false);
    T1_objc_msg_1arg(mtk_view,
        T1_os_macos_s->sel_set_needs_display, false);
    
    T1_objc_msg_1arg(
        window,
        T1_os_macos_s->sel_set_content_view,
        (uintptr_t)mtk_view);
    
    apple_gpu_delegate = (void *)T1_objc_msg(
        T1_os_macos_s->class_T1_mtk_view_delegate,
        T1_os_macos_s->sel_new);
    
    T1_objc_msg_1arg(mtk_view,
        T1_os_macos_s->sel_set_delegate,
        (uintptr_t)apple_gpu_delegate);
    
    char shader_lib_path_cstr[512];
    T1_os_get_res_dir(
        shader_lib_path_cstr,
        512);
    
    T1_std_strcat_cap(
        shader_lib_path_cstr,
        512,
        "/Shaders.metallib");
    
    b8 result = T1_apple_gpu_init(
        /* void (* arg_funcptr_shared_gameloop_update)(GPUDataForSingleFrame *): */
            T1_gameloop_update_before_render_pass,
            T1_gameloop_update_after_render_pass,
        /* id<MTLDevice> with_metal_device: */
            metal_device_for_window,
        /* NSString *shader_lib_filepath: */
            shader_lib_path_cstr,
        /* bool32_t has_retina_screen: */
            T1_os_get_screen_backing_scale_factor(),
            // (float)[[window screen] backingScaleFactor],
        /* char * error_msg_string: */
            errmsg);
    
    if (!result || !T1_log_app_running) {
        if (errmsg[0] == '\0') {
            T1_std_strcpy_cap(
                errmsg,
                errmsg_cap,
                "Failed Metal init (unhandled)"); 
        }
        return;
    }
    
    *good = 1;
}
