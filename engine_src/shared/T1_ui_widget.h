#ifndef UI_WIDGET_H
#define UI_WIDGET_H

#include "T1_std.h"
#include "T1_mem.h"
#include "T1_zspriteid.h"
#include "T1_meta.h"
#include "T1_io.h"
#include "T1_zsprite.h"
#include "T1_tex_array.h"
#include "T1_zsprite_anim.h"
#include "T1_texquad_anim.h"
#include "T1_text.h"
#include "T1_profiler.h"


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
        uint64_t custom_uint_max;
        int64_t  custom_int_max;
        double   custom_float_max;
    };
    union {
        uint64_t custom_uint_min;
        int64_t  custom_int_min;
        double   custom_float_min;
    };
    T1Tex     slider_pin_tex;
    float     width_screen;
    float     height_screen;
    float     pin_width_screen;
    float     pin_height_screen;
    
    float     screen_x;
    float     screen_y;
    float     z;
    
    char      sfx_filename[T1_UI_WIDGET_STR_CAP];
    char      label_prefix[T1_UI_WIDGET_STR_CAP];
    bool8_t   custom_min_max_vals;
    bool8_t   is_meta_enum;
    T1Type    linked_type;
    
    float     pin_rgba[4];
    int32_t   tex_array_i;
    int32_t   tex_slice_i;
    float     slider;
    bool8_t   label_shows_value;
} T1UIWidgetProps;

extern T1UIWidgetProps * T1_ui_widget_next_props;

// allocates memory for state
void T1_ui_widget_init(void);

void T1_ui_widget_handle_touches(uint64_t ms_elapsed);

/*
A slider linked to a float of your choice, for the user to slide left/right to
get the value of their choosing
*/
void T1_ui_widget_request_slider(
    const int32_t background_zsprite_id,
    const int32_t label_zsprite_id,
    const int32_t pin_zsprite_id,
    void * linked_value_ptr);

void T1_ui_widget_request_button(
    const int32_t button_zsprite_id,
    const int32_t button_label_id,
    void (* onclick_funcptr)(int64_t),
    const int64_t clicked_arg);

void T1_ui_widget_delete(
    const int32_t with_zsprite_id);

void T1_ui_widget_delete_all(void);

#endif // !UI_WIDGET_H
