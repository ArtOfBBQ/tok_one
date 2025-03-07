#include "uielement.h"

static int32_t currently_sliding_touchable_id = -1;
static int32_t currently_sliding_object_id = -1;
static int32_t currently_clicking_object_id = -1;

typedef struct ActiveUIElement {
    int32_t touchable_id;
    int32_t object_id;
    int32_t object_id_2;
    bool32_t clickable;
    bool32_t slideable;
    bool32_t deleted;
    bool32_t is_float;
    float slider_width;
    union {
        float slider_min_float;
        int32_t slider_min_int;
    };
    union {
        float slider_max_float;
        int32_t slider_max_int;
    };
    union {
        float * slider_linked_float;
        int32_t * slider_linked_int;
    };
    char interaction_sound_filename[128];
    void (* clicked_funcptr)(void);
    void (* slid_funcptr)(void);
} ActiveUIElement;

NextUIElementSettings * next_ui_element_settings = NULL;

#define ACTIVE_UI_ELEMENTS_SIZE 5000
uint32_t active_ui_elements_size = 0;
ActiveUIElement * active_ui_elements = NULL;

static ActiveUIElement * next_active_ui_element(void) {
    for (uint32_t i = 0; i < active_ui_elements_size; i++) {
        if (active_ui_elements[i].deleted) {
            common_memset_char(&active_ui_elements[i], 0, sizeof(ActiveUIElement));
            return &active_ui_elements[i];
        }
    }
    
    log_assert(active_ui_elements_size < ACTIVE_UI_ELEMENTS_SIZE);
    active_ui_elements[active_ui_elements_size].deleted = true;
    active_ui_elements_size += 1;
    common_memset_char(&active_ui_elements[active_ui_elements_size - 1], 0, sizeof(ActiveUIElement));
    return &active_ui_elements[active_ui_elements_size - 1];
}

void uielement_init(void) {
    next_ui_element_settings = (NextUIElementSettings *)
        malloc_from_unmanaged(sizeof(NextUIElementSettings));
    common_memset_char(next_ui_element_settings, 0, sizeof(NextUIElementSettings));
    
    next_ui_element_settings->slider_width_screenspace         = 100;
    next_ui_element_settings->slider_height_screenspace        =  40;
    next_ui_element_settings->ignore_lighting                  = true;
    next_ui_element_settings->ignore_camera                    = false;
    next_ui_element_settings->button_background_texturearray_i = -1;
    next_ui_element_settings->button_background_texture_i      = -1;
    next_ui_element_settings->slider_background_texturearray_i = -1;
    next_ui_element_settings->slider_background_texture_i      = -1;
    next_ui_element_settings->slider_pin_texturearray_i        = -1;
    next_ui_element_settings->slider_pin_texture_i             = -1;
    
    active_ui_elements = (ActiveUIElement *)malloc_from_unmanaged(
        sizeof(ActiveUIElement) * ACTIVE_UI_ELEMENTS_SIZE);
    common_memset_char(
        active_ui_elements,
        0,
        sizeof(ActiveUIElement) * ACTIVE_UI_ELEMENTS_SIZE);
}

void ui_elements_handle_touches(uint64_t ms_elapsed)
{
    #ifdef PROFILER_ACTIVE
    profiler_start("ui_elements_handle_touches()");
    #endif
    
    (void)ms_elapsed;
    
    if (
        !user_interactions[INTR_PREVIOUS_TOUCH_OR_LEFTCLICK_START].handled)
    {
        if (
            user_interactions[INTR_PREVIOUS_TOUCH_OR_LEFTCLICK_START].
                touchable_id_top >= 0 &&
            user_interactions[INTR_PREVIOUS_TOUCH_OR_LEFTCLICK_START].
                touchable_id_top < LAST_UI_TOUCHABLE_ID)
        {
            for (
                uint32_t i = 0;
                i < active_ui_elements_size;
                i++)
            {
                if (
                    active_ui_elements[i].deleted ||
                    user_interactions[INTR_PREVIOUS_TOUCH_OR_LEFTCLICK_START].
                        touchable_id_top !=
                    active_ui_elements[i].touchable_id)
                {
                    continue;
                }
                
                if (
                    active_ui_elements[i].slideable)
                {
                    currently_sliding_object_id =
                    active_ui_elements[i].object_id_2;
                    currently_sliding_touchable_id =
                        active_ui_elements[i].touchable_id;
                    
                    for (
                        uint32_t zp_i = 0;
                        zp_i < zpolygons_to_render->size;
                        zp_i++)
                    {
                        if (zpolygons_to_render->cpu_data[zp_i].object_id ==
                            currently_sliding_object_id)
                        {
                            zpolygons_to_render->gpu_data[zp_i].scale_factor =
                                1.05f;
                        }
                    }
                    
                    ScheduledAnimation * bump_pin =
                        next_scheduled_animation(true);
                    bump_pin->affected_object_id = currently_sliding_object_id;
                    bump_pin->gpu_polygon_vals.scale_factor = 1.20f;
                    bump_pin->duration_microseconds = 20;
                    commit_scheduled_animation(bump_pin);
                    
                    bump_pin = next_scheduled_animation(true);
                    bump_pin->affected_object_id = currently_sliding_object_id;
                    bump_pin->gpu_polygon_vals.scale_factor = 1.0f;
                    bump_pin->wait_before_each_run = 20;
                    bump_pin->duration_microseconds = 200000;
                    commit_scheduled_animation(bump_pin);
                }
                
                if (active_ui_elements[i].clickable) {
                    currently_clicking_object_id =
                        active_ui_elements[i].object_id;
                    
                    ScheduledAnimation * bump =
                        next_scheduled_animation(true);
                    bump->affected_object_id = currently_clicking_object_id;
                    bump->gpu_polygon_vals.scale_factor = 1.25f;
                    bump->duration_microseconds = 40;
                    commit_scheduled_animation(bump);
                    
                    ScheduledAnimation * flatten =
                        next_scheduled_animation(true);
                    flatten->affected_object_id = currently_clicking_object_id;
                    flatten->gpu_polygon_vals.scale_factor = 1.0f;
                    flatten->wait_before_each_run = 50;
                    flatten->duration_microseconds = 250000;
                    commit_scheduled_animation(flatten);
                }
                
                if (
                    active_ui_elements[i].
                        interaction_sound_filename[0] != '\0')
                {
                    // add_audio();
                    // platform_play_sound_resource(
                    //    active_ui_elements[i].interaction_sound_filename);
                }
                
                user_interactions[INTR_PREVIOUS_TOUCH_OR_LEFTCLICK_START].
                    handled = true;
            }
        }
    }
    
    if (
        currently_sliding_touchable_id >= 0 &&
        currently_sliding_object_id >= 0)
    {
        int32_t ui_elem_i = -1;
        for (
            ui_elem_i = 0;
            ui_elem_i < (int32_t)active_ui_elements_size;
            ui_elem_i++)
        {
            if (
                !active_ui_elements[ui_elem_i].deleted &&
                active_ui_elements[ui_elem_i].slideable &&
                user_interactions[INTR_PREVIOUS_TOUCH_OR_LEFTCLICK_START].
                    touchable_id_top ==
                active_ui_elements[ui_elem_i].touchable_id)
            {
                user_interactions[INTR_PREVIOUS_TOUCH_OR_LEFTCLICK_START].
                    handled = true;
                break;
            }
        }
        
        if (ui_elem_i >= 0) {
            
            for (
                uint32_t zp_i = 0;
                zp_i < zpolygons_to_render->size;
                zp_i++)
            {
                if (
                    active_ui_elements[ui_elem_i].slideable &&
                    zpolygons_to_render->cpu_data[zp_i].object_id ==
                        currently_sliding_object_id)
                {
                    // set slider value
                    float new_x_offset =
                        windowsize_screenspace_x_to_x(
                            user_interactions[
                                INTR_PREVIOUS_MOUSE_OR_TOUCH_MOVE].screen_x,
                            zpolygons_to_render->gpu_data[zp_i].xyz[2]) -
                        zpolygons_to_render->gpu_data[zp_i].xyz[0];
                    
                    if (
                        new_x_offset <
                            -active_ui_elements[ui_elem_i].slider_width / 2)
                    {
                        new_x_offset =
                            -active_ui_elements[ui_elem_i].slider_width / 2;
                    }
                    
                    if (
                        new_x_offset >
                            active_ui_elements[ui_elem_i].slider_width / 2)
                    {
                        new_x_offset =
                            active_ui_elements[ui_elem_i].slider_width / 2;
                    }
                    
                    zpolygons_to_render->gpu_data[zp_i].xyz_offset[0] =
                        new_x_offset;
                    
                    float slider_pct = (new_x_offset /
                        active_ui_elements[ui_elem_i].slider_width) + 0.5f;
                    
                    if (active_ui_elements[ui_elem_i].is_float) {
                        log_assert(
                            active_ui_elements[ui_elem_i].slider_linked_float !=
                                NULL);
                        *(active_ui_elements[ui_elem_i].slider_linked_float) =
                            active_ui_elements[ui_elem_i].slider_min_float +
                                ((active_ui_elements[ui_elem_i].
                                    slider_max_float -
                                    active_ui_elements[ui_elem_i].
                                        slider_min_float) * slider_pct);
                    } else {
                        log_assert(
                            active_ui_elements[ui_elem_i].slider_linked_int !=
                                NULL);
                        *(active_ui_elements[ui_elem_i].slider_linked_int) =
                            active_ui_elements[ui_elem_i].slider_min_int +
                                (int32_t)(
                                    (active_ui_elements[ui_elem_i].
                                        slider_max_int -
                                            active_ui_elements[ui_elem_i].
                                                slider_min_int) * slider_pct);
                    }
                    
                    if (active_ui_elements[ui_elem_i].slid_funcptr != NULL) {
                        active_ui_elements[ui_elem_i].slid_funcptr();
                    }
                }
            }
        }
    }
    
    if (
        !user_interactions[INTR_PREVIOUS_TOUCH_OR_LEFTCLICK_END].handled)
    {
        if (
            currently_sliding_touchable_id >= 0 &&
            currently_sliding_object_id >= 0)
        {
            currently_sliding_object_id = -1;
            currently_sliding_touchable_id = -1;
        }
        
        if (
            currently_clicking_object_id >= 0)
        {
            int32_t ui_elem_i = -1;
            for (
                ui_elem_i = 0;
                ui_elem_i < (int32_t)active_ui_elements_size;
                ui_elem_i++)
            {
                if (
                    !active_ui_elements[ui_elem_i].deleted &&
                    active_ui_elements[ui_elem_i].clickable &&
                    user_interactions[INTR_PREVIOUS_TOUCH_OR_LEFTCLICK_END].
                        touchable_id_top ==
                    active_ui_elements[ui_elem_i].touchable_id)
                {
                    user_interactions[INTR_PREVIOUS_TOUCH_OR_LEFTCLICK_END].
                        handled = true;
                    active_ui_elements[ui_elem_i].clicked_funcptr();
                    break;
                }
            }
            
            currently_sliding_object_id = -1;
        }
    }
    
    #ifdef PROFILER_ACTIVE
    profiler_end("ui_elements_handle_touches()");
    #endif
}

static void request_slider_shared(
    const int32_t background_object_id,
    const int32_t pin_object_id,
    const float x_screenspace,
    const float y_screenspace,
    const float z,
    const float initial_x_offset_screenspace,
    ActiveUIElement * next_active_element)
{
    log_assert(next_ui_element_settings != NULL);
    log_assert(background_object_id != pin_object_id);
    log_assert(next_ui_element_settings->slider_width_screenspace > 0);
    log_assert(next_ui_element_settings->slider_height_screenspace > 0);
    log_assert(next_ui_element_settings->pin_width_screenspace > 0);
    log_assert(next_ui_element_settings->pin_height_screenspace > 0);
    
    PolygonRequest slider_back;
    request_next_zpolygon(&slider_back);
    construct_quad_around(
        /* const float mid_x: */
            windowsize_screenspace_x_to_x(x_screenspace, z),
        /* const float mid_y: */
            windowsize_screenspace_y_to_y(y_screenspace, z),
        /* const float z: */
            z,
        /* const float width: */
            windowsize_screenspace_width_to_width(
                next_ui_element_settings->slider_width_screenspace,
                z),
        /* const float height: */
            windowsize_screenspace_height_to_height(
                next_ui_element_settings->slider_height_screenspace,
                z),
        /* zPolygon * recipient: */
            &slider_back);
    
    slider_back.cpu_data->object_id = background_object_id;
    
    slider_back.gpu_materials[0].texturearray_i =
        next_ui_element_settings->slider_background_texturearray_i;
    slider_back.gpu_materials[0].texture_i =
        next_ui_element_settings->slider_background_texture_i;
    slider_back.gpu_materials[0].rgba[0] =
        next_ui_element_settings->slider_background_rgba[0];
    slider_back.gpu_materials[0].rgba[1] =
        next_ui_element_settings->slider_background_rgba[1];
    slider_back.gpu_materials[0].rgba[2] =
        next_ui_element_settings->slider_background_rgba[2];
    slider_back.gpu_materials[0].rgba[3] =
        next_ui_element_settings->slider_background_rgba[3];
    
    slider_back.gpu_data->ignore_lighting =
        next_ui_element_settings->ignore_lighting;
    slider_back.gpu_data->ignore_camera =
        next_ui_element_settings->ignore_camera;
    log_assert(slider_back.cpu_data->visible);
    log_assert(!slider_back.cpu_data->committed);
    log_assert(!slider_back.cpu_data->deleted);
    commit_zpolygon_to_render(&slider_back);
    
    PolygonRequest slider_pin;
    request_next_zpolygon(&slider_pin);
    float pin_z = z - 0.002f;
    construct_quad_around(
        /* const float mid_x: */
            windowsize_screenspace_x_to_x(x_screenspace, pin_z),
        /* const float mid_y: */
            windowsize_screenspace_y_to_y(y_screenspace, pin_z),
        /* const float z: */
            pin_z,
        /* const float width: */
            windowsize_screenspace_width_to_width(
                next_ui_element_settings->pin_width_screenspace,
                pin_z),
        /* const float height: */
            windowsize_screenspace_height_to_height(
                next_ui_element_settings->pin_height_screenspace,
                pin_z),
        /* zPolygon * recipient: */
            &slider_pin);
    
    slider_pin.cpu_data->object_id = pin_object_id;
    
    slider_pin.gpu_data->xyz_offset[0] =
        windowsize_screenspace_width_to_width(
            initial_x_offset_screenspace,
            pin_z);
    
    slider_pin.gpu_data->xyz_offset[1] = 0.0f;
    
    slider_pin.gpu_materials[0].texturearray_i =
        next_ui_element_settings->slider_pin_texturearray_i;
    slider_pin.gpu_materials[0].texture_i =
        next_ui_element_settings->slider_pin_texture_i;
    slider_pin.gpu_materials[0].rgba[0] =
        next_ui_element_settings->slider_pin_rgba[0];
    slider_pin.gpu_materials[0].rgba[1] =
        next_ui_element_settings->slider_pin_rgba[1];
    slider_pin.gpu_materials[0].rgba[2] =
        next_ui_element_settings->slider_pin_rgba[2];
    slider_pin.gpu_materials[0].rgba[3] =
        next_ui_element_settings->slider_pin_rgba[3];
    
    slider_pin.gpu_data->ignore_lighting =
        next_ui_element_settings->ignore_lighting;
    slider_pin.gpu_data->ignore_camera =
        next_ui_element_settings->ignore_camera;
    slider_pin.gpu_data->touchable_id = next_ui_element_touchable_id();
    commit_zpolygon_to_render(&slider_pin);
    
    next_active_element->object_id = background_object_id;
    next_active_element->object_id_2 = pin_object_id;
    next_active_element->touchable_id = slider_pin.gpu_data->touchable_id;
    next_active_element->slid_funcptr =
        next_ui_element_settings->slider_slid_funcptr;
    
    next_active_element->slider_width =
        windowsize_screenspace_width_to_width(
            next_ui_element_settings->slider_width_screenspace,
            z);
    next_active_element->slideable = true;
    next_active_element->deleted = false;
    common_strcpy_capped(
        next_active_element->interaction_sound_filename,
        128,
        next_ui_element_settings->interaction_sound_filename);
}

void request_int_slider(
    const int32_t background_object_id,
    const int32_t pin_object_id,
    const float x_screenspace,
    const float y_screenspace,
    const float z,
    const int32_t min_value,
    const int32_t max_value,
    int32_t * linked_value)
{
    if (*linked_value < min_value) {
        *linked_value = min_value;
    }
    if (*linked_value > max_value) {
        *linked_value = max_value;
    }
    
    float initial_slider_progress =
        (*linked_value - min_value) / (max_value - min_value);
    float initial_x_offset_screenspace =
        -(next_ui_element_settings->slider_width_screenspace / 2) +
        (initial_slider_progress *
            next_ui_element_settings->slider_width_screenspace);
    
    ActiveUIElement * next_active_element = next_active_ui_element();
    
    request_slider_shared(
        /* const int32_t background_object_id: */
            background_object_id,
        /* const int32_t pin_object_id: */
            pin_object_id,
        /* const float x_screenspace: */
            x_screenspace,
        /* const float y_screenspace: */
            y_screenspace,
        /* const float z: */
            z,
        /* initial_x_offset_screenspace: */
            initial_x_offset_screenspace,
        /* ActiveUIElement * next_active_element: */
            next_active_element);
    
    next_active_element->is_float = false;
    next_active_element->slider_linked_int = linked_value;
    next_active_element->slider_min_int = min_value;
    next_active_element->slider_max_int = max_value;
}

void request_float_slider(
    const int32_t background_object_id,
    const int32_t pin_object_id,
    const float x_screenspace,
    const float y_screenspace,
    const float z,
    const float min_value,
    const float max_value,
    float * linked_value)
{
    if (*linked_value < min_value) {
        *linked_value = min_value;
    }
    if (*linked_value > max_value) {
        *linked_value = max_value;
    }
    
    float initial_slider_progress =
        (*linked_value - min_value) / (max_value - min_value);
    float initial_x_offset_screenspace =
        -(next_ui_element_settings->slider_width_screenspace / 2) +
        (initial_slider_progress *
            next_ui_element_settings->slider_width_screenspace);
    
    ActiveUIElement * next_active_element = next_active_ui_element();
    
    request_slider_shared(
        /* const int32_t background_object_id: */
            background_object_id,
        /* const int32_t pin_object_id: */
            pin_object_id,
        /* const float x_screenspace: */
            x_screenspace,
        /* const float y_screenspace: */
            y_screenspace,
        /* const float z: */
            z,
        /* initial_x_offset_screenspace: */
            initial_x_offset_screenspace,
        /* ActiveUIElement * next_active_element: */
            next_active_element);
    
    next_active_element->is_float = true;
    next_active_element->slider_linked_float = linked_value;
    next_active_element->slider_min_float = min_value;
    next_active_element->slider_max_float = max_value;
}

void request_button(
    const int32_t button_object_id,
    const char * label,
    const float x_screenspace,
    const float y_screenspace,
    const float z,
    void (* funtion_pointer)(void))
{
    log_assert(next_ui_element_settings->button_width_screenspace  > 5.0f);
    log_assert(next_ui_element_settings->button_height_screenspace > 5.0f);
    
    PolygonRequest button_request;
    button_request.materials_size = 1;
    request_next_zpolygon(&button_request);
    construct_quad_around(
        /* const float mid_x: */
            windowsize_screenspace_x_to_x(x_screenspace, z),
        /* const float mid_y: */
            windowsize_screenspace_y_to_y(y_screenspace, z),
        /* const float z: */
            z,
        /* const float width: */
            windowsize_screenspace_width_to_width(
                next_ui_element_settings->button_width_screenspace,
                z),
        /* const float height: */
            windowsize_screenspace_height_to_height(
                next_ui_element_settings->button_height_screenspace,
                z),
        /* PolygonRequest * stack_recipient: */
            &button_request);
    
    button_request.cpu_data->object_id = button_object_id;
    button_request.gpu_data->touchable_id = next_ui_element_touchable_id();
    button_request.gpu_data->ignore_camera =
        next_ui_element_settings->ignore_camera;
    button_request.gpu_data->ignore_lighting =
        next_ui_element_settings->ignore_lighting;
    commit_zpolygon_to_render(&button_request);
    
    button_request.gpu_materials[0].rgba[0] =
        next_ui_element_settings->button_background_rgba[0];
    button_request.gpu_materials[0].rgba[1] =
        next_ui_element_settings->button_background_rgba[1];
    button_request.gpu_materials[0].rgba[2] =
        next_ui_element_settings->button_background_rgba[2];
    button_request.gpu_materials[0].rgba[3] =
        next_ui_element_settings->button_background_rgba[3];
    button_request.gpu_materials[0].texturearray_i =
        next_ui_element_settings->button_background_texturearray_i;
    button_request.gpu_materials[0].texture_i =
        next_ui_element_settings->button_background_texture_i;
    
    font_settings->ignore_camera = next_ui_element_settings->ignore_camera;
    font_settings->remove_hitbox = true;
    text_request_label_around(
        /* const int32_t with_object_id: */
            button_object_id,
        /* const char * text_to_draw: */
            label,
        /* const float mid_x_pixelspace: */
            x_screenspace,
        /* const float mid_y_pixelspace: */
            y_screenspace,
        /* const float z: */
            z,
        /* const float max_width: */
            next_ui_element_settings->button_width_screenspace);
    
    ActiveUIElement * next_element = next_active_ui_element();
    next_element->clickable = true;
    next_element->clicked_funcptr = funtion_pointer;
    next_element->touchable_id = button_request.gpu_data->touchable_id;
    next_element->object_id = button_object_id;
    next_element->deleted = false;
}

void unregister_ui_element_with_object_id(
    const int32_t with_object_id)
{
    for (uint32_t i = 0; i < active_ui_elements_size; i++) {
        if (active_ui_elements[i].object_id == with_object_id) {
            active_ui_elements[i].deleted = true;
        }
    }
}

void delete_all_ui_elements(void) {
    for (uint32_t i = 0; i < active_ui_elements_size; i++) {
        active_ui_elements[i].deleted = false;
    }
    clear_ui_element_touchable_ids();
    active_ui_elements_size = 0;
}
