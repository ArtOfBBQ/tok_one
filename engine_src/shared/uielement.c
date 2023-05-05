#include "uielement.h"

static int32_t currently_sliding_touchable_id = -1;
static int32_t currently_sliding_object_id = -1;

typedef struct ActiveUIElement {
    int32_t touchable_id;
    int32_t object_id;
    bool32_t clickable;
    bool32_t slideable;
    bool32_t deleted;
    float slider_min;
    float slider_max;
} ActiveUIElement;

NextUIElementSettings * next_ui_element_settings = NULL;

#define ACTIVE_UI_ELEMENTS_SIZE 300
uint32_t active_ui_elements_size = 0;
ActiveUIElement * active_ui_elements = NULL;

static ActiveUIElement * next_active_ui_element(void) {
    for (uint32_t i = 0; i < active_ui_elements_size; i++) {
        if (active_ui_elements[i].deleted) {
            return &active_ui_elements[i];
        }
    }
    
    active_ui_elements[active_ui_elements_size].deleted = true;
    active_ui_elements_size += 1;
    return &active_ui_elements[active_ui_elements_size - 1];
}

void init_ui_elements(void) {
    next_ui_element_settings = (NextUIElementSettings *)
        malloc_from_unmanaged(sizeof(NextUIElementSettings));

    next_ui_element_settings->ignore_lighting = true;

    active_ui_elements = (ActiveUIElement *)malloc_from_unmanaged(
        sizeof(ActiveUIElement) * ACTIVE_UI_ELEMENTS_SIZE);
}

void ui_elements_handle_touches(uint64_t ms_elapsed)
{
    if (
        !user_interactions[INTR_PREVIOUS_TOUCH_OR_LEFTCLICK_START].handled)
    {
        if (
            user_interactions[INTR_PREVIOUS_TOUCH_OR_LEFTCLICK_START].
                touchable_id >= 0)
        {
            for (uint32_t i = 0; i < active_ui_elements_size; i++) {
                if (
                    !active_ui_elements[i].deleted &&
                    user_interactions[INTR_PREVIOUS_TOUCH_OR_LEFTCLICK_START].
                        touchable_id ==
                    active_ui_elements[i].touchable_id)
                {
                    currently_sliding_object_id =
                        active_ui_elements[i].object_id;
                    currently_sliding_touchable_id =
                        active_ui_elements[i].touchable_id;
                    
                    printf(
                        "started sliding on object: %i - %i\n",
                        currently_sliding_object_id,
                        currently_sliding_touchable_id);
                    user_interactions[INTR_PREVIOUS_TOUCH_OR_LEFTCLICK_START].
                        handled = true;
                }
            }
        }
    }
        
    if (
        currently_sliding_touchable_id >= 0 &&
        currently_sliding_object_id >= 0)
    {
        int32_t ui_elem_i = -1;
        for (ui_elem_i = 0; ui_elem_i < active_ui_elements_size; ui_elem_i++) {
            if (
                !active_ui_elements[ui_elem_i].deleted &&
                user_interactions[INTR_PREVIOUS_TOUCH_OR_LEFTCLICK_START].
                    touchable_id ==
                active_ui_elements[ui_elem_i].touchable_id)
            {
                break;
            }
        }
        
        if (ui_elem_i >= 0) {
        
            for (int32_t zp_i = 0; zp_i < zpolygons_to_render_size; zp_i++) {
                if (
                    zpolygons_to_render[zp_i].object_id ==
                        currently_sliding_object_id)
                {
                    // set slider value
                    zpolygons_to_render[zp_i].x_offset =
                        screenspace_x_to_x(
                            user_interactions[INTR_PREVIOUS_MOUSE_OR_TOUCH_MOVE].
                                screen_x,
                            zpolygons_to_render[zp_i].z) -
                        zpolygons_to_render[zp_i].x;
                    printf("updated x-offset\n");
                }
            }
        }
    }
    
    if (
        !user_interactions[INTR_PREVIOUS_TOUCH_OR_LEFTCLICK_END].handled &&
        currently_sliding_touchable_id >= 0 &&
        currently_sliding_object_id >= 0)
    {
        printf("stopped sliding\n");
        
        user_interactions[INTR_PREVIOUS_TOUCH_OR_LEFTCLICK_END].handled = true;
        
        currently_sliding_object_id = -1;
        currently_sliding_touchable_id = -1;
    }
}

void request_float_slider(
    const float x_screenspace,
    const float y_screenspace,
    const float z,
    const float min_value,
    const float max_value,
    const float * linked_value)
{
    zPolygon slider_back;
    construct_zpolygon(
        /* zPolygon * to_construct: */
            &slider_back);
    
    construct_quad_around(
        /* const float mid_x: */
                screenspace_x_to_x(x_screenspace, z),
            /* const float mid_y: */
                screenspace_y_to_y(y_screenspace, z),
        /* const float z: */
            z,
        /* const float width: */
                screenspace_width_to_width(
                    next_ui_element_settings->slider_width_screenspace,
                    z),
        /* const float height: */
                screenspace_height_to_height(
                    next_ui_element_settings->slider_height_screenspace,
                    z),
        /* zPolygon * recipient: */
            &slider_back);
    
    slider_back.triangle_materials[0].texturearray_i =
        next_ui_element_settings->slider_background_texturearray_i;
    slider_back.triangle_materials[0].texture_i =
        next_ui_element_settings->slider_background_texture_i;
    
    slider_back.ignore_lighting = next_ui_element_settings->ignore_lighting;
    
    request_zpolygon_to_render(&slider_back);

    zPolygon slider_pin;
    construct_zpolygon(
    /* zPolygon * to_construct: */
        &slider_pin);
    
    float pin_z = z - 0.001f;
    construct_quad_around(
        /* const float mid_x: */
                screenspace_x_to_x(x_screenspace, pin_z),
            /* const float mid_y: */
                screenspace_y_to_y(y_screenspace, pin_z),
        /* const float z: */
            pin_z,
        /* const float width: */
                screenspace_width_to_width(
                    next_ui_element_settings->pin_width_screenspace,
                    pin_z),
        /* const float height: */
                screenspace_height_to_height(
                    next_ui_element_settings->pin_height_screenspace,
                    pin_z),
        /* zPolygon * recipient: */
            &slider_pin);
    
    slider_pin.x_offset = 0.0f;
    slider_pin.y_offset = 0.0f;
    
    slider_pin.triangle_materials[0].texturearray_i =
        next_ui_element_settings->slider_pin_texturearray_i;
    slider_pin.triangle_materials[0].texture_i =
        next_ui_element_settings->slider_pin_texture_i;
    
    slider_pin.ignore_lighting = next_ui_element_settings->ignore_lighting;
    slider_pin.object_id = next_object_id();
    slider_pin.touchable_id = next_touchable_id();
    
    ActiveUIElement * next_active_element = next_active_ui_element();
    next_active_element->object_id = slider_pin.object_id;
    next_active_element->touchable_id = slider_pin.touchable_id;
    next_active_element->slider_min = min_value;
    next_active_element->slider_max = max_value;
    next_active_element->slideable = true;
    next_active_element->deleted = false;
    
    request_zpolygon_to_render(&slider_pin);
}

