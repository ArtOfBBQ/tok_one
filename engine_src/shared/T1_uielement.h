#ifndef UI_ELEMENT_H
#define UI_ELEMENT_H

#include "T1_std.h"
#include "T1_mem.h"
#include "T1_zspriteid.h"
#include "T1_userinput.h"
#include "T1_zsprite.h"
#include "T1_texture_array.h"
#include "T1_scheduled_animations.h"
#include "T1_text.h"
#include "T1_profiler.h"
#include "T1_meta.h"


#ifdef __cplusplus
extern "C" {
#endif

/*
These options are applied to every new UI element (like a button or a slider)
you make. They're things like colors/textures/size that I expect to be the
same for most buttons/sliders in the app and don't want to set every time.

If you do want sliders of different sizes, you can set the slider size, request
some sliders, change the slider size, and then request some more sliders
*/
typedef struct {
    T1GPUConstMat back_mat;
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
    float     button_width_screenspace;
    float     button_height_screenspace;
    float     slider_width_screenspace;
    float     slider_height_screenspace;
    float     pin_width_screenspace;
    float     pin_height_screenspace;
    
    float     screenspace_x;
    float     screenspace_y;
    float     z;
    
    char      interaction_sound_filename[128];
    char      label_prefix[128];
    bool8_t   custom_min_max_vals;
    bool8_t   ignore_lighting;
    bool8_t   ignore_camera;
    T1Type    linked_type;
} T1UIElementPermUserSettings;

typedef struct {
    T1UIElementPermUserSettings perm;
    char *   slider_label;
    float    slider_pin_rgba[4];
    int32_t  button_background_texturearray_i;
    int32_t  button_background_texture_i;
    float    slider;
    bool8_t  slider_label_shows_value;
} T1NextUIElementSettings;

extern T1NextUIElementSettings * next_ui_element_settings;

// allocates memory for state
void T1_uielement_init(void);

void T1_uielement_handle_touches(uint64_t ms_elapsed);

/*
A slider linked to a float of your choice, for the user to slide left/right to
get the value of their choosing
*/
void T1_uielement_request_slider(
    const int32_t background_zsprite_id,
    const int32_t label_zsprite_id,
    const int32_t pin_zsprite_id,
    void * linked_value_ptr);

void T1_uielement_request_button(
    const int32_t button_zsprite_id,
    const int32_t button_label_id,
    void (* onclick_funcptr)(int64_t),
    const int64_t clicked_arg);

void T1_uielement_delete(
    const int32_t with_zsprite_id);

void T1_uielement_delete_all(void);

#ifdef __cplusplus
}
#endif

#endif // UI_ELEMENT_H

