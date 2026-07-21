#ifndef UI_WIDGET_H
#define UI_WIDGET_H

#include <stdint.h>

#include "T1_std.h"
#include "T1_tex.h"
#include "T1_meta.h"

/*
These options are applied to every new UI element (like a button or a slider)
you make. They're things like colors/textures/size that I expect to be the
same for most buttons/sliders in the app and don't want to set every time.

If you do want sliders of different sizes, you can set the slider size, request
some sliders, change the slider size, and then request some more sliders
*/
void T1_ui_widget_requester_set_pin_rgba(u8 rgba_i, f32 val);
void T1_ui_widget_requester_set_screenspace_height(u32 height);
void T1_ui_widget_requester_set_screenspace_width(u32 width);
void T1_ui_widget_requester_set_screenspace_pin_height(u32 height);
void T1_ui_widget_requester_set_screenspace_pin_width(u32 width);
void T1_ui_widget_requester_set_sfx_filename(c8 * sfx_fn);
void T1_ui_widget_requester_set_font_height(u32 to_val);
void T1_ui_widget_requester_set_screen_x(s32 x);
void T1_ui_widget_requester_set_screen_y(s32 y);
void T1_ui_widget_requester_set_z(f32 z);
void T1_ui_widget_requester_set_custom_minmax_f32(b8 active, f32 min, f32 max);
void T1_ui_widget_requester_set_linked_type(T1MetaType type);
// allocates memory for state
void T1_ui_widget_init(void);

b8 T1_ui_widget_handle_lclick(void);
#if 0
void T1_ui_widget_handle_touches(u64 ms_elapsed);
#endif

/*
A slider linked to a f32 of your choice, for the user to slide left/right to
get the value of their choosing
*/
void T1_ui_widget_request_slider(
    const u32 background_T1_id,
    const u32 label_T1_id,
    const u32 pin_T1_id,
    void * linked_value_ptr);

void T1_ui_widget_request_button(
    const u32 button_T1_id,
    const u32 button_label_T1_id,
    void (* onclick_funcptr)(s64),
    const s64 clicked_arg);

void T1_ui_widget_delete(u32 with_T1_id);

void T1_ui_widget_delete_all(void);

#endif // !UI_WIDGET_H
