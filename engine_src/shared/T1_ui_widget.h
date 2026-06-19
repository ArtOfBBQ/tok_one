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
#define T1_UI_WIDGET_STR_CAP 128
typedef struct {
    char * meta_struct_name;
    union {
        u64 custom_umax;
        s64 custom_smax;
        f64 custom_fmax;
    };
    union {
        u64 custom_umin;
        s64 custom_smin;
        f64 custom_fmin;
    };
    T1Tex       slider_pin_tex;
    f32       width_screen;
    f32       height_screen;
    f32       pin_width_screen;
    f32       pin_height_screen;
    
    f32       screen_x;
    f32       screen_y;
    f32       z;
    
    char        sfx_filename[T1_UI_WIDGET_STR_CAP];
    char        label_prefix[T1_UI_WIDGET_STR_CAP];
    b8     custom_min_max_vals;
    b8     is_meta_enum;
    T1MetaType  linked_type;
    
    f32       pin_rgba[4];
    s32     tex_array_i;
    s32     tex_slice_i;
    f32       slider;
    b8          label_shows_value;
} T1UIWidgetProps;

extern T1UIWidgetProps * T1_ui_widget_next_props;

// allocates memory for state
void T1_ui_widget_init(void);

void T1_ui_widget_handle_touches(u64 ms_elapsed);

/*
A slider linked to a f32 of your choice, for the user to slide left/right to
get the value of their choosing
*/
void T1_ui_widget_request_slider(
    const s32 background_T1_id,
    const s32 label_T1_id,
    const s32 pin_T1_id,
    void * linked_value_ptr);

void T1_ui_widget_request_button(
    const s32 button_T1_id,
    const s32 button_label_T1_id,
    void (* onclick_funcptr)(s64),
    const s64 clicked_arg);

void T1_ui_widget_delete(const s32 with_T1_id);

void T1_ui_widget_delete_all(void);

#endif // !UI_WIDGET_H
