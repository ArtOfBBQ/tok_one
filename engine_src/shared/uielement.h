#ifndef UI_ELEMENT_H
#define UI_ELEMENT_H

#include "common.h"
#include "memorystore.h"
#include "scheduled_animations.h"
#include "userinput.h"
#include "objectid.h"
#include "zpolygon.h"

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
typedef struct NextUIElementSettings {
    float    slider_background_rgba[4];
    float    slider_pin_rgba[4];
    bool32_t ignore_lighting;
    bool32_t ignore_camera;
    int32_t  slider_background_texturearray_i;
    int32_t  slider_background_texture_i;
    int32_t  slider_pin_texturearray_i;
    int32_t  slider_pin_texture_i;
    float    slider_width_screenspace;
    float    slider_height_screenspace;
    float    pin_width_screenspace;
    float    pin_height_screenspace;
    float    button_width_screenspace;
    float    button_height_screenspace;
    float    button_background_rgba[4];
    int32_t  button_background_texturearray_i;
    int32_t  button_background_texture_i;
    float    slider;
    char     interaction_sound_filename[128];
} NextUIElementSettings;

extern NextUIElementSettings * next_ui_element_settings;

// allocates memory for state
void init_ui_elements(void);

void ui_elements_handle_touches(uint64_t ms_elapsed);

void request_int_slider(
    const int32_t background_object_id,
    const int32_t pin_object_id,
    const float x_screenspace,
    const float y_screenspace,
    const float z,
    const int32_t min_value,
    const int32_t max_value,
    int32_t * linked_value);

/*
A slider linked to a float of your choice, for the user to slide left/right to
get the value of their choosing
*/
void request_float_slider(
    const int32_t background_object_id,
    const int32_t pin_object_id,
    const float x_screenspace,
    const float y_screenspace,
    const float z,
    const float min_value,
    const float max_value,
    float * linked_value);

void request_button(
    const int32_t button_object_id,
    const char * label,
    const float x_screenspace,
    const float y_screenspace,
    const float z,
    void (* funtion_pointer)(void));

void unregister_ui_element_with_object_id(
    const int32_t with_object_id);

#ifdef __cplusplus
}
#endif

#endif // UI_ELEMENT_H

