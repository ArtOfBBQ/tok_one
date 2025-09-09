#include "T1_uielement.h"

static int32_t currently_sliding_touchable_id = -1;
// static int32_t currently_sliding_zsprite_id = -1;
static int32_t currently_clicking_zsprite_id = -1;

typedef struct ActiveUIElement {
    UIElementPermUserSettings user_set;
    
    int32_t background_zsprite_id;
    int32_t pin_zsprite_id;
    int32_t label_zsprite_id;
    int32_t touchable_id;
    
    float slider_width;
    
    bool32_t clickable;
    bool32_t slideable;
    bool32_t deleted;
    
    void * slider_linked_value;
    void (* clicked_funcptr)(void);
    
    uint8_t has_label;
    uint8_t label_dirty;
} ActiveUIElement;

NextUIElementSettings * next_ui_element_settings = NULL;

#define ACTIVE_UI_ELEMENTS_SIZE 5000
uint32_t active_ui_elements_size = 0;
ActiveUIElement * active_ui_elements = NULL;

static ActiveUIElement * next_active_ui_element(void) {
    for (uint32_t i = 0; i < active_ui_elements_size; i++) {
        if (active_ui_elements[i].deleted) {
            T1_std_memset(&active_ui_elements[i], 0, sizeof(ActiveUIElement));
            return &active_ui_elements[i];
        }
    }
    
    log_assert(active_ui_elements_size < ACTIVE_UI_ELEMENTS_SIZE);
    active_ui_elements[active_ui_elements_size].deleted = true;
    active_ui_elements_size += 1;
    T1_std_memset(&active_ui_elements[active_ui_elements_size - 1], 0, sizeof(ActiveUIElement));
    return &active_ui_elements[active_ui_elements_size - 1];
}

void uielement_init(void) {
    next_ui_element_settings = (NextUIElementSettings *)
        malloc_from_unmanaged(sizeof(NextUIElementSettings));
    T1_std_memset(next_ui_element_settings, 0, sizeof(NextUIElementSettings));
    
    next_ui_element_settings->slider_label                     = NULL;
    next_ui_element_settings->perm.slider_width_screenspace = 100;
    next_ui_element_settings->perm.slider_height_screenspace =  40;
    next_ui_element_settings->perm.ignore_lighting = true;
    next_ui_element_settings->perm.ignore_camera = false;
    next_ui_element_settings->button_background_texturearray_i = -1;
    next_ui_element_settings->button_background_texture_i      = -1;
    next_ui_element_settings->perm.slider_background_tex.array_i = -1;
    next_ui_element_settings->perm.slider_background_tex.slice_i = -1;
    next_ui_element_settings->perm.slider_pin_tex.array_i = -1;
    next_ui_element_settings->perm.slider_pin_tex.slice_i = -1;
    
    active_ui_elements = (ActiveUIElement *)malloc_from_unmanaged(
        sizeof(ActiveUIElement) * ACTIVE_UI_ELEMENTS_SIZE);
    T1_std_memset(
        active_ui_elements,
        0,
        sizeof(ActiveUIElement) * ACTIVE_UI_ELEMENTS_SIZE);
}

void ui_elements_handle_touches(uint64_t ms_elapsed)
{
    #if PROFILER_ACTIVE
    profiler_start("ui_elements_handle_touches()");
    #endif
    
    (void)ms_elapsed;
    
    if (
        currently_sliding_touchable_id >= 0)
    {
        int32_t ui_elem_i = -1;
        for (
            int32_t elem_i = 0;
            elem_i < (int32_t)active_ui_elements_size;
            elem_i++)
        {
            if (
                !active_ui_elements[elem_i].deleted &&
                active_ui_elements[elem_i].slideable &&
                user_interactions
                    [INTR_PREVIOUS_TOUCH_OR_LEFTCLICK_START].
                        touchable_id_top ==
                active_ui_elements[elem_i].touchable_id)
            {
                user_interactions
                    [INTR_PREVIOUS_TOUCH_OR_LEFTCLICK_START].
                        handled = true;
                ui_elem_i = elem_i;
                break;
            }
        }
        
        if (ui_elem_i >= 0) {
            
            for (
                uint32_t zp_i = 0;
                zp_i < zsprites_to_render->size;
                zp_i++)
            {
                if (
                    active_ui_elements[ui_elem_i].slideable &&
                    zsprites_to_render->gpu_data[zp_i].
                        touchable_id ==
                            currently_sliding_touchable_id)
                {
                    // set slider value
                    float new_x_offset =
                        engineglobals_screenspace_x_to_x(
                            user_interactions[
                                INTR_PREVIOUS_MOUSE_OR_TOUCH_MOVE].
                                    screen_x,
                            zsprites_to_render->gpu_data[zp_i].
                                xyz[2]) -
                        zsprites_to_render->gpu_data[zp_i].xyz[0];
                    
                    if (
                        new_x_offset <
                            -active_ui_elements[ui_elem_i].
                                slider_width / 2)
                    {
                        new_x_offset =
                            -active_ui_elements[ui_elem_i].
                                slider_width / 2;
                    }
                    
                    if (
                        new_x_offset >
                            active_ui_elements[ui_elem_i].
                                slider_width / 2)
                    {
                        new_x_offset =
                            active_ui_elements[ui_elem_i].
                                slider_width / 2;
                    }
                    
                    zsprites_to_render->gpu_data[zp_i].xyz_offset[0] =
                        new_x_offset;
                        
                    active_ui_elements[ui_elem_i].label_dirty = true;
                    
                    double slider_pct = (new_x_offset /
                        active_ui_elements[ui_elem_i].
                            slider_width) + 0.5;
                    
                    log_assert(
                        active_ui_elements[ui_elem_i].
                            slider_linked_value != NULL);
                    
                    double new_val_f64 =
                        active_ui_elements[ui_elem_i].user_set.
                            custom_float_min +
                        ((active_ui_elements[ui_elem_i].user_set.
                            custom_float_max -
                            active_ui_elements[ui_elem_i].user_set.
                                custom_float_min) *
                                    slider_pct);
                    
                    uint64_t new_val_u64 =
                        active_ui_elements[ui_elem_i].user_set.
                            custom_uint_min +
                        (uint64_t)((double)(active_ui_elements[ui_elem_i].
                            user_set.custom_uint_max -
                            active_ui_elements[ui_elem_i].user_set.
                                custom_uint_min) *
                                    slider_pct);
                    
                    int64_t new_val_i64 =
                        active_ui_elements[ui_elem_i].user_set.
                            custom_int_min +
                        (int64_t)((double)(active_ui_elements[ui_elem_i].
                            user_set.custom_int_max -
                            active_ui_elements[ui_elem_i].
                                user_set.custom_int_min) *
                                    slider_pct);
                    
                    switch (active_ui_elements[ui_elem_i].user_set.linked_type)
                    {
                        case T1_TYPE_I64:
                            *((int64_t *)active_ui_elements[ui_elem_i].
                                slider_linked_value) = new_val_i64;
                        break;
                        case T1_TYPE_I32:
                            *((int32_t *)active_ui_elements[ui_elem_i].
                                slider_linked_value) = (int32_t)new_val_i64;
                        break;
                        case T1_TYPE_I16:
                            *((int16_t *)active_ui_elements[ui_elem_i].
                                slider_linked_value) = (int16_t)new_val_i64;
                        break;
                        case T1_TYPE_I8:
                            *((int8_t *)active_ui_elements[ui_elem_i].
                                slider_linked_value) = (int8_t)new_val_i64;
                        break;
                        case T1_TYPE_U64:
                            *((uint64_t *)active_ui_elements[ui_elem_i].
                                slider_linked_value) = new_val_u64;
                        break;
                        case T1_TYPE_U32:
                            *((uint32_t *)active_ui_elements[ui_elem_i].
                                slider_linked_value) =
                                    (uint32_t)new_val_u64;
                        break;
                        case T1_TYPE_U16:
                            *((uint16_t *)active_ui_elements[ui_elem_i].
                                slider_linked_value) =
                                    (uint16_t)new_val_u64;
                        break;
                        case T1_TYPE_U8:
                            *((uint8_t *)active_ui_elements[ui_elem_i].
                                slider_linked_value) =
                                    (uint8_t)new_val_u64;
                        break;
                        case T1_TYPE_F32:
                            *((float *)active_ui_elements[ui_elem_i].
                                slider_linked_value) = (float)new_val_f64;
                        break;
                        default:
                            log_dump_and_crash(
                                "Unhandled slider data type");
                    }
                }
            }
        }
    }
    
    if (
        !user_interactions[INTR_PREVIOUS_TOUCH_OR_LEFTCLICK_END].
            handled)
    {
        if (
            currently_sliding_touchable_id >= 0)
        {
            currently_sliding_touchable_id = -1;
        }
        
        if (
            currently_clicking_zsprite_id >= 0)
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
                    user_interactions
                        [INTR_PREVIOUS_TOUCH_OR_LEFTCLICK_END].
                            touchable_id_top ==
                    active_ui_elements[ui_elem_i].touchable_id)
                {
                    user_interactions
                        [INTR_PREVIOUS_TOUCH_OR_LEFTCLICK_END].
                            handled = true;
                    active_ui_elements[ui_elem_i].
                        clicked_funcptr();
                    break;
                }
            }
            
            currently_sliding_touchable_id = -1;
        }
    }
    
    if (
        !user_interactions[INTR_PREVIOUS_TOUCH_OR_LEFTCLICK_START].
            handled)
    {
        if (
            user_interactions
                [INTR_PREVIOUS_TOUCH_OR_LEFTCLICK_START].
                    touchable_id_top >= 0 &&
            user_interactions
                [INTR_PREVIOUS_TOUCH_OR_LEFTCLICK_START].
                    touchable_id_top < LAST_UI_TOUCHABLE_ID)
        {
            for (
                uint32_t i = 0;
                i < active_ui_elements_size;
                i++)
            {
                if (
                    active_ui_elements[i].deleted ||
                    user_interactions
                        [INTR_PREVIOUS_TOUCH_OR_LEFTCLICK_START].
                            touchable_id_top !=
                                active_ui_elements[i].touchable_id)
                {
                    continue;
                }
                
                if (
                    active_ui_elements[i].slideable)
                {
                    currently_sliding_touchable_id =
                        active_ui_elements[i].touchable_id;
                    user_interactions
                        [INTR_PREVIOUS_TOUCH_OR_LEFTCLICK_END].
                            handled = true;
                    
                    T1ScheduledAnimation * bump_pin =
                        T1_scheduled_animations_request_next(true);
                    bump_pin->easing_type =
                        EASINGTYPE_SINGLE_BOUNCE_ZERO_TO_ZERO;
                    bump_pin->affected_zsprite_id = active_ui_elements[i].pin_zsprite_id;
                    bump_pin->gpu_polygon_vals.scale_factor = 1.20f;
                    bump_pin->duration_us = 120000;
                    T1_scheduled_animations_commit(bump_pin);
                }
                
                if (active_ui_elements[i].clickable) {
                    currently_clicking_zsprite_id =
                        active_ui_elements[i].background_zsprite_id;
                    
                    T1ScheduledAnimation * bump =
                        T1_scheduled_animations_request_next(true);
                    bump->affected_zsprite_id =
                        currently_clicking_zsprite_id;
                    bump->easing_type =
                        EASINGTYPE_SINGLE_BOUNCE_ZERO_TO_ZERO;
                    bump->gpu_polygon_vals.scale_factor = 1.25f;
                    bump->duration_us = 140000;
                    T1_scheduled_animations_commit(bump);
                }
                
                if (
                    active_ui_elements[i].user_set.
                        interaction_sound_filename[0] != '\0')
                {
                     #if AUDIO_ACTIVE
                     /*
                     add_audio();
                     platform_play_sound_resource(
                        active_ui_elements[i].
                            interaction_sound_filename);
                     */
                     #endif
                }
                
                user_interactions[INTR_PREVIOUS_TOUCH_OR_LEFTCLICK_START].
                    handled = true;
            }
        }
    }
    
    for (uint32_t i = 0; i < active_ui_elements_size; i++) {
        if (
            active_ui_elements[i].has_label &&
            active_ui_elements[i].label_dirty)
        {
            active_ui_elements[i].label_dirty = false;
            
            char full_label[256];
            full_label[0] = '\0';
            
            if (active_ui_elements[i].user_set.label_prefix[0] != '\0') {
                T1_std_strcpy_cap(
                    full_label,
                    256,
                    active_ui_elements[i].user_set.label_prefix);
                T1_std_strcat_cap(
                    full_label,
                    256,
                    ": ");
            }
            
            float xy_screenspace[3];
            float z;
            uint8_t ignore_camera =
                active_ui_elements[i].user_set.ignore_camera;
            xy_screenspace[0] =
                active_ui_elements[i].user_set.screenspace_x;
            xy_screenspace[1] =
                active_ui_elements[i].user_set.screenspace_y;
            z = active_ui_elements[i].user_set.z;
            
            for (
                int32_t zs_i = 0;
                zs_i < (int32_t)zsprites_to_render->size;
                zs_i++)
            {
                if (
                    zsprites_to_render->cpu_data[zs_i].zsprite_id ==
                        active_ui_elements[i].label_zsprite_id)
                {
                    xy_screenspace[0] =
                        engineglobals_x_to_screenspace_x(
                            zsprites_to_render->gpu_data[zs_i].xyz[0],
                            zsprites_to_render->gpu_data[zs_i].xyz[2]);
                    xy_screenspace[1] =
                        engineglobals_y_to_screenspace_y(
                            zsprites_to_render->gpu_data[zs_i].xyz[1],
                            zsprites_to_render->gpu_data[zs_i].xyz[2]);
                    z = zsprites_to_render->gpu_data[zs_i].xyz[2];
                    ignore_camera = (uint8_t)zsprites_to_render->gpu_data[zs_i].ignore_camera;
                    zsprites_to_render->cpu_data[zs_i].deleted = true;
                }
            }
            
            font_settings->font_height = (active_ui_elements[i].
                user_set.slider_height_screenspace * 2) / 3;
            font_settings->mat.diffuse_rgb[0] = 0.5f;
            font_settings->mat.diffuse_rgb[1] = 1.0f;
            font_settings->mat.diffuse_rgb[2] = 0.5f;
            font_settings->mat.ambient_rgb[0] = 0.5f;
            font_settings->mat.ambient_rgb[1] = 1.0f;
            font_settings->mat.ambient_rgb[2] = 1.0f;
            font_settings->mat.rgb_cap[0] = 1.0f;
            font_settings->mat.rgb_cap[1] = 1.0f;
            font_settings->mat.rgb_cap[2] = 1.0f;
            font_settings->mat.alpha = 1.0f;
            font_settings->ignore_lighting =
                active_ui_elements[i].user_set.ignore_lighting;
            font_settings->ignore_camera = ignore_camera;
            font_settings->alpha = 1.0f;
            
            switch (active_ui_elements[i].user_set.linked_type) {
            case T1_TYPE_F32:
                T1_std_strcat_float_cap(
                    full_label,
                    256,
                    *(float *)active_ui_elements[i].
                        slider_linked_value);
                break;
            case T1_TYPE_U64:
            T1_std_strcat_uint_cap(
                    full_label,
                    256,
                    (uint32_t)*(uint64_t *)active_ui_elements[i].
                        slider_linked_value);
            break;
            case T1_TYPE_U32:
            T1_std_strcat_uint_cap(
                    full_label,
                    256,
                    *(uint32_t *)active_ui_elements[i].
                        slider_linked_value);
            break;
            case T1_TYPE_U16:
                T1_std_strcat_uint_cap(
                    full_label,
                    256,
                    *(uint16_t *)active_ui_elements[i].
                        slider_linked_value);
            break;
            case T1_TYPE_U8:
                T1_std_strcat_uint_cap(
                    full_label,
                    256,
                    *(uint8_t *)active_ui_elements[i].
                        slider_linked_value);
            break;
            default:
                T1_std_strcat_cap(
                    full_label,
                    256,
                    "?DATA");
            break;
            }
            
            text_request_label_around(
                /* const int32_t with_object_id: */
                    active_ui_elements[i].label_zsprite_id,
                /* const char * text_to_draw: */
                    full_label,
                /* const float mid_x_pixelspace: */
                    xy_screenspace[0],
                /* const float mid_y_pixelspace: */
                    xy_screenspace[1],
                /* const float z: */
                    z,
                /* const float max_width: */
                    active_ui_elements[i].user_set.
                        slider_width_screenspace);
        }
    }
    
    #if PROFILER_ACTIVE
    profiler_end("ui_elements_handle_touches()");
    #endif
}

void T1_uielement_request_slider(
    const int32_t background_zsprite_id,
    const int32_t label_zsprite_id,
    const int32_t pin_zsprite_id,
    void * linked_value_ptr)
{
    log_assert(next_ui_element_settings != NULL);
    log_assert(background_zsprite_id != pin_zsprite_id);
    log_assert(background_zsprite_id != label_zsprite_id);
    log_assert(next_ui_element_settings->perm.
        slider_width_screenspace > 0);
    log_assert(next_ui_element_settings->perm.
        slider_height_screenspace > 0);
    log_assert(next_ui_element_settings->perm.pin_width_screenspace > 0);
    log_assert(next_ui_element_settings->perm.pin_height_screenspace > 0);
    
    ActiveUIElement * next_active_element = next_active_ui_element();
    next_active_element->user_set = next_ui_element_settings->perm;
    next_active_element->has_label = next_ui_element_settings->slider_label != NULL;
    if (next_active_element->has_label) {
        T1_std_strcpy_cap(
            next_active_element->user_set.label_prefix,
            128,
            next_ui_element_settings->slider_label);
    }
    next_active_element->label_dirty = true;
    
    next_active_element->background_zsprite_id = background_zsprite_id;
    next_active_element->pin_zsprite_id        = pin_zsprite_id;
    next_active_element->label_zsprite_id      = label_zsprite_id;
    
    if (next_ui_element_settings->perm.custom_min_max_vals) {
        next_active_element->user_set.custom_int_min = next_ui_element_settings->perm.custom_int_min;
        next_active_element->user_set.custom_int_max =
            next_ui_element_settings->perm.custom_int_max;
    } else {
        switch (next_ui_element_settings->perm.linked_type) {
            case T1_TYPE_I32:
                next_active_element->user_set.custom_int_min = INT32_MIN;
                next_active_element->user_set.custom_int_max = INT32_MAX;
            break;
            case T1_TYPE_U32:
                next_active_element->
                    user_set.custom_uint_min = 0;
                next_active_element->
                    user_set.custom_uint_max = UINT32_MAX;
            break;
            case T1_TYPE_I64:
                next_active_element->user_set.custom_int_min = INT64_MIN;
                next_active_element->user_set.custom_int_max = INT64_MAX;
            break;
            case T1_TYPE_U64:
                next_active_element->user_set.custom_float_min = 0;
                next_active_element->user_set.custom_float_max = INT64_MAX;
            break;
            case T1_TYPE_F32:
                next_active_element->user_set.custom_float_min = FLOAT32_MIN;
                next_active_element->user_set.custom_float_max = FLOAT32_MAX;
            break;
            default:
                log_assert(0);
        }
    }
    next_active_element->slider_width =
        engineglobals_screenspace_width_to_width(
            next_ui_element_settings->perm.slider_width_screenspace,
            next_ui_element_settings->perm.z);
    next_active_element->slideable = true;
    next_active_element->deleted = false;
    next_active_element->slider_linked_value = linked_value_ptr;
    next_active_element->touchable_id = next_ui_element_touchable_id();
    
    zSpriteRequest slider_back;
    zsprite_request_next(&slider_back);
    zsprite_construct_quad_around(
        /* const float mid_x: */
            engineglobals_screenspace_x_to_x(
                next_active_element->user_set.screenspace_x,
                next_active_element->user_set.z),
        /* const float mid_y: */
            engineglobals_screenspace_y_to_y(
                next_active_element->user_set.screenspace_y,
                next_active_element->user_set.z),
        /* const float z: */
            next_active_element->user_set.z,
        /* const float width: */
            engineglobals_screenspace_width_to_width(
                next_active_element->user_set.
                    slider_width_screenspace,
                next_active_element->user_set.z),
        /* const float height: */
            engineglobals_screenspace_height_to_height(
                next_active_element->user_set.slider_height_screenspace,
                next_active_element->user_set.z),
        /* zPolygon * recipient: */
            &slider_back);
    
    slider_back.cpu_data->zsprite_id = background_zsprite_id;
    
    slider_back.gpu_data->base_mat.texturearray_i =
        next_active_element->user_set.slider_background_tex.array_i;
    slider_back.gpu_data->base_mat.texture_i =
        next_active_element->user_set.slider_background_tex.slice_i;
    
    slider_back.gpu_data->base_mat.diffuse_rgb[0] =
        next_ui_element_settings->slider_background_rgba[0];
    slider_back.gpu_data->base_mat.diffuse_rgb[1] =
        next_ui_element_settings->slider_background_rgba[1];
    slider_back.gpu_data->base_mat.diffuse_rgb[2] =
        next_ui_element_settings->slider_background_rgba[2];
    slider_back.gpu_data->base_mat.alpha =
        next_ui_element_settings->slider_background_rgba[3];
    
    slider_back.gpu_data->ignore_lighting =
        next_active_element->user_set.ignore_lighting;
    slider_back.gpu_data->ignore_camera =
        next_active_element->user_set.ignore_camera;
    log_assert(slider_back.cpu_data->visible);
    log_assert(!slider_back.cpu_data->committed);
    log_assert(!slider_back.cpu_data->deleted);
    zsprite_commit(&slider_back);
    
    zSpriteRequest slider_pin;
    zsprite_request_next(&slider_pin);
    float pin_z = next_active_element->user_set.z - 0.00001f;
    zsprite_construct_quad_around(
        /* const float mid_x: */
            engineglobals_screenspace_x_to_x(
                next_active_element->user_set.screenspace_x,
                next_active_element->user_set.z),
        /* const float mid_y: */
            engineglobals_screenspace_y_to_y(
                next_active_element->user_set.screenspace_y,
                next_active_element->user_set.z),
        /* const float z: */
            pin_z,
        /* const float width: */
            engineglobals_screenspace_width_to_width(
                next_active_element->user_set.pin_width_screenspace,
                next_active_element->user_set.z),
        /* const float height: */
            engineglobals_screenspace_height_to_height(
                next_active_element->user_set.pin_height_screenspace,
                next_active_element->user_set.z),
        /* zPolygon * recipient: */
            &slider_pin);
    
    slider_pin.cpu_data->zsprite_id = pin_zsprite_id;
    slider_pin.gpu_data->xyz_offset[0] = 0.0f;
    slider_pin.gpu_data->xyz_offset[1] = 0.0f;
    
    slider_pin.gpu_data->base_mat.texturearray_i =
        next_active_element->user_set.slider_pin_tex.array_i;
    slider_pin.gpu_data->base_mat.texture_i =
        next_active_element->user_set.slider_pin_tex.slice_i;
    
    slider_pin.gpu_data->base_mat.diffuse_rgb[0] =
        next_ui_element_settings->slider_pin_rgba[0];
    slider_pin.gpu_data->base_mat.diffuse_rgb[1] =
        next_ui_element_settings->slider_pin_rgba[1];
    slider_pin.gpu_data->base_mat.diffuse_rgb[2] =
        next_ui_element_settings->slider_pin_rgba[2];
    slider_pin.gpu_data->base_mat.alpha =
        next_ui_element_settings->slider_pin_rgba[3];
    
    slider_pin.gpu_data->ignore_lighting =
        next_active_element->user_set.ignore_lighting;
    slider_pin.gpu_data->ignore_camera =
        next_active_element->user_set.ignore_camera;
    slider_pin.gpu_data->touchable_id =
        next_active_element->touchable_id;
    zsprite_commit(&slider_pin);
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
    
    zSpriteRequest button_request;
    zsprite_request_next(&button_request);
    zsprite_construct_quad_around(
        /* const float mid_x: */
            engineglobals_screenspace_x_to_x(x_screenspace, z),
        /* const float mid_y: */
            engineglobals_screenspace_y_to_y(y_screenspace, z),
        /* const float z: */
            z,
        /* const float width: */
            engineglobals_screenspace_width_to_width(
                next_ui_element_settings->button_width_screenspace,
                z),
        /* const float height: */
            engineglobals_screenspace_height_to_height(
                next_ui_element_settings->button_height_screenspace,
                z),
        /* PolygonRequest * stack_recipient: */
            &button_request);
    
    button_request.cpu_data->zsprite_id = button_object_id;
    button_request.gpu_data->touchable_id = next_ui_element_touchable_id();
    button_request.gpu_data->ignore_camera =
        next_ui_element_settings->perm.ignore_camera;
    button_request.gpu_data->ignore_lighting =
        next_ui_element_settings->perm.ignore_lighting;
    zsprite_commit(&button_request);
    
    button_request.gpu_data->base_mat.diffuse_rgb[0] =
        next_ui_element_settings->button_background_rgba[0];
    button_request.gpu_data->base_mat.diffuse_rgb[1] =
        next_ui_element_settings->button_background_rgba[1];
    button_request.gpu_data->base_mat.diffuse_rgb[2] =
        next_ui_element_settings->button_background_rgba[2];
    button_request.gpu_data->base_mat.alpha =
        next_ui_element_settings->button_background_rgba[3];
    button_request.gpu_data->base_mat.texturearray_i =
        next_ui_element_settings->button_background_texturearray_i;
    button_request.gpu_data->base_mat.texture_i =
        next_ui_element_settings->button_background_texture_i;
    
    font_settings->ignore_camera = next_ui_element_settings->perm.ignore_camera;
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
    next_element->background_zsprite_id = button_object_id;
    next_element->deleted = false;
}

void unregister_ui_element_with_object_id(
    const int32_t with_object_id)
{
    for (uint32_t i = 0; i < active_ui_elements_size; i++) {
        if (active_ui_elements[i].background_zsprite_id == with_object_id) {
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
