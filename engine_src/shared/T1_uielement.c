#include "T1_uielement.h"

static int32_t currently_sliding_touchable_id = -1;
// static int32_t currently_sliding_zsprite_id = -1;
static int32_t currently_clicking_zsprite_id = -1;

typedef struct ActiveUIElement {
    T1UIElementPermUserSettings user_set;
    
    int64_t clicked_arg;
    
    int32_t background_zsprite_id;
    int32_t pin_zsprite_id;
    int32_t label_zsprite_id;
    int32_t touchable_id;
    
    float slider_width;
    
    bool32_t slideable;
    bool32_t deleted;
    
    void * slider_linked_value;
    void (* clicked_funcptr)(int64_t);
    
    uint8_t has_label;
    uint8_t label_dirty;
} ActiveUIElement;

T1NextUIElementSettings * next_ui_element_settings = NULL;

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

void T1_uielement_init(void) {
    next_ui_element_settings = (T1NextUIElementSettings *)
        T1_mem_malloc_from_unmanaged(sizeof(T1NextUIElementSettings));
    T1_std_memset(next_ui_element_settings, 0, sizeof(T1NextUIElementSettings));
    
    next_ui_element_settings->slider_label                     = NULL;
    next_ui_element_settings->perm.slider_width_screenspace = 100;
    next_ui_element_settings->perm.slider_height_screenspace =  40;
    next_ui_element_settings->perm.ignore_lighting = true;
    next_ui_element_settings->perm.ignore_camera = false;
    next_ui_element_settings->button_background_texturearray_i = -1;
    next_ui_element_settings->button_background_texture_i      = -1;
    next_ui_element_settings->perm.slider_pin_tex.array_i = -1;
    next_ui_element_settings->perm.slider_pin_tex.slice_i = -1;
    
    active_ui_elements = (ActiveUIElement *)T1_mem_malloc_from_unmanaged(
        sizeof(ActiveUIElement) * ACTIVE_UI_ELEMENTS_SIZE);
    T1_std_memset(
        active_ui_elements,
        0,
        sizeof(ActiveUIElement) * ACTIVE_UI_ELEMENTS_SIZE);
}

void T1_uielement_handle_touches(uint64_t ms_elapsed)
{
    #if T1_PROFILER_ACTIVE == T1_ACTIVE
    T1_profiler_start("ui_elements_handle_touches()");
    #elif T1_PROFILER_ACTIVE == T1_INACTIVE
    // Pass
    #else
    #error "T1_PROFILER_ACTIVE undefined"
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
                T1_uiinteractions
                    [T1_INTR_PREVIOUS_TOUCH_OR_LEFTCLICK_START].
                        touchable_id_top ==
                active_ui_elements[elem_i].touchable_id)
            {
                T1_uiinteractions
                    [T1_INTR_PREVIOUS_TOUCH_OR_LEFTCLICK_START].
                        handled = true;
                ui_elem_i = elem_i;
                break;
            }
        }
        
        if (ui_elem_i >= 0) {
            
            for (
                uint32_t zp_i = 0;
                zp_i < T1_zsprites_to_render->size;
                zp_i++)
            {
                if (
                    active_ui_elements[ui_elem_i].slideable &&
                    T1_zsprites_to_render->gpu_data[zp_i].
                        touchable_id ==
                            currently_sliding_touchable_id)
                {
                    // set slider value
                    float new_x_offset =
                        T1_engineglobals_screenspace_x_to_x(
                            T1_uiinteractions[
                                T1_INTR_PREVIOUS_MOUSE_OR_TOUCH_MOVE].
                                    screen_x,
                            T1_zsprites_to_render->cpu_data[zp_i].
                                simd_stats.xyz[2]) -
                        T1_zsprites_to_render->cpu_data[zp_i].simd_stats.xyz[0];
                    
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
                    
                    T1_zsprites_to_render->cpu_data[zp_i].simd_stats.offset_xyz[0] =
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
        !T1_uiinteractions[T1_INTR_PREVIOUS_TOUCH_OR_LEFTCLICK_END].
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
                    active_ui_elements[ui_elem_i].clicked_funcptr != NULL &&
                    T1_uiinteractions
                        [T1_INTR_PREVIOUS_TOUCH_OR_LEFTCLICK_END].
                            touchable_id_top ==
                    active_ui_elements[ui_elem_i].touchable_id)
                {
                    T1_uiinteractions
                        [T1_INTR_PREVIOUS_TOUCH_OR_LEFTCLICK_END].
                            handled = true;
                    active_ui_elements[ui_elem_i].
                        clicked_funcptr(
                            active_ui_elements[ui_elem_i].clicked_arg);
                    break;
                }
            }
            
            currently_sliding_touchable_id = -1;
        }
    }
    
    if (
        !T1_uiinteractions[T1_INTR_PREVIOUS_TOUCH_OR_LEFTCLICK_START].
            handled)
    {
        if (
            T1_uiinteractions
                [T1_INTR_PREVIOUS_TOUCH_OR_LEFTCLICK_START].
                    touchable_id_top >= 0 &&
            T1_uiinteractions
                [T1_INTR_PREVIOUS_TOUCH_OR_LEFTCLICK_START].
                    touchable_id_top < T1_LAST_UI_TOUCHABLE_ID)
        {
            for (
                uint32_t i = 0;
                i < active_ui_elements_size;
                i++)
            {
                if (
                    active_ui_elements[i].deleted ||
                    T1_uiinteractions
                        [T1_INTR_PREVIOUS_TOUCH_OR_LEFTCLICK_START].
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
                    T1_uiinteractions
                        [T1_INTR_PREVIOUS_TOUCH_OR_LEFTCLICK_END].
                            handled = true;
                    
                    #if T1_SCHEDULED_ANIMS_ACTIVE == T1_ACTIVE
                    T1ScheduledAnimation * bump_pin =
                        T1_scheduled_animations_request_next(true);
                    bump_pin->easing_type =
                        EASINGTYPE_SINGLE_BOUNCE_ZERO_TO_ZERO;
                    bump_pin->affected_zsprite_id = active_ui_elements[i].pin_zsprite_id;
                    bump_pin->cpu_vals.scale_factor = 1.20f;
                    bump_pin->duration_us = 120000;
                    T1_scheduled_animations_commit(bump_pin);
                    #elif T1_SCHEDULED_ANIMS_ACTIVE == T1_INACTIVE
                    #else
                    #error
                    #endif
                }
                
                if (active_ui_elements[i].clicked_funcptr != NULL) {
                    currently_clicking_zsprite_id =
                        active_ui_elements[i].background_zsprite_id;
                    
                    #if T1_SCHEDULED_ANIMS_ACTIVE == T1_ACTIVE
                    T1ScheduledAnimation * bump =
                        T1_scheduled_animations_request_next(true);
                    bump->affected_zsprite_id =
                        currently_clicking_zsprite_id;
                    bump->easing_type =
                        EASINGTYPE_SINGLE_BOUNCE_ZERO_TO_ZERO;
                    bump->cpu_vals.scale_factor = 1.25f;
                    bump->duration_us = 140000;
                    T1_scheduled_animations_commit(bump);
                    #elif T1_SCHEDULED_ANIMS_ACTIVE == T1_INACTIVE
                    #else
                    #error
                    #endif
                }
                
                if (
                    active_ui_elements[i].user_set.
                        interaction_sound_filename[0] != '\0')
                {
                     #if T1_AUDIO_ACTIVE == T1_ACTIVE
                     /*
                     add_audio();
                     platform_play_sound_resource(
                        active_ui_elements[i].
                            interaction_sound_filename);
                     */
                     #elif T1_AUDIO_ACTIVE == T1_INACTIVE
                     #else
                     #error
                     #endif
                }
                
                T1_uiinteractions[T1_INTR_PREVIOUS_TOUCH_OR_LEFTCLICK_START].
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
            
            #define FULL_LABEL_CAP 256
            char full_label[FULL_LABEL_CAP];
            T1_std_memset(full_label, 0, FULL_LABEL_CAP);
            
            if (
                active_ui_elements[i].user_set.label_prefix[0] != '\0' &&
                !active_ui_elements[i].user_set.is_meta_enum)
            {
                T1_std_strcpy_cap(
                    full_label,
                    FULL_LABEL_CAP,
                    active_ui_elements[i].user_set.label_prefix);
            }
            
            float xy_screenspace[3];
            float z;
            uint8_t ignore_camera =
                active_ui_elements[i].user_set.ignore_camera;
            uint32_t good = 0;
            
            xy_screenspace[0] =
                active_ui_elements[i].user_set.screenspace_x;
            xy_screenspace[1] =
                active_ui_elements[i].user_set.screenspace_y;
            z = active_ui_elements[i].user_set.z;
            
            for (
                int32_t zs_i = 0;
                zs_i < (int32_t)T1_zsprites_to_render->size;
                zs_i++)
            {
                if (
                    T1_zsprites_to_render->cpu_data[zs_i].zsprite_id ==
                        active_ui_elements[i].label_zsprite_id)
                {
                    xy_screenspace[0] =
                        T1_engineglobals_x_to_screenspace_x(
                            T1_zsprites_to_render->cpu_data[zs_i].
                                simd_stats.xyz[0],
                            T1_zsprites_to_render->cpu_data[zs_i].
                                simd_stats.xyz[2]);
                    xy_screenspace[1] =
                        T1_engineglobals_y_to_screenspace_y(
                            T1_zsprites_to_render->cpu_data[zs_i].
                                simd_stats.xyz[1],
                            T1_zsprites_to_render->cpu_data[zs_i].
                                simd_stats.xyz[2]);
                    z = T1_zsprites_to_render->cpu_data[zs_i].simd_stats.xyz[2];
                    ignore_camera = (uint8_t)
                        T1_zsprites_to_render->gpu_data[zs_i].
                            ignore_camera;
                    T1_zsprites_to_render->cpu_data[zs_i].deleted = true;
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
            
            if (active_ui_elements[i].slider_linked_value != NULL) {
                if (active_ui_elements[i].user_set.is_meta_enum) {
                    char * enum_as_str = NULL;
                    
                    switch (active_ui_elements[i].user_set.linked_type) {
                    case T1_TYPE_U8:
                        enum_as_str = T1_meta_enum_uint_to_string(
                            active_ui_elements[i].user_set.meta_struct_name,
                            *(uint8_t *)active_ui_elements[i].
                                slider_linked_value,
                                &good);
                        assert(good);
                    break;
                    default:
                    assert(0);
                    }
                    
                    T1_std_strcat_cap(
                        full_label,
                        FULL_LABEL_CAP,
                        enum_as_str == NULL ? "N/A" : enum_as_str);
                } else {
                    T1_std_strcat_cap(
                        full_label,
                        FULL_LABEL_CAP,
                        ": ");
                    
                    switch (active_ui_elements[i].user_set.linked_type)
                    {
                    case T1_TYPE_F32:
                        T1_std_strcat_float_cap(
                            full_label,
                            FULL_LABEL_CAP,
                            *(float *)active_ui_elements[i].
                                slider_linked_value);
                    break;
                    case T1_TYPE_U64:
                        T1_std_strcat_uint_cap(
                            full_label,
                            FULL_LABEL_CAP,
                            (uint32_t)*(uint64_t *)active_ui_elements[i].
                                slider_linked_value);
                    break;
                    case T1_TYPE_U32:
                        T1_std_strcat_uint_cap(
                            full_label,
                            FULL_LABEL_CAP,
                            *(uint32_t *)active_ui_elements[i].
                                slider_linked_value);
                    break;
                    case T1_TYPE_U16:
                        T1_std_strcat_uint_cap(
                            full_label,
                            FULL_LABEL_CAP,
                            *(uint16_t *)active_ui_elements[i].
                                slider_linked_value);
                    break;
                    case T1_TYPE_U8:
                        T1_std_strcat_uint_cap(
                            full_label,
                            FULL_LABEL_CAP,
                            *(uint8_t *)active_ui_elements[i].
                                slider_linked_value);
                    break;
                    default:
                        T1_std_strcat_cap(
                            full_label,
                            FULL_LABEL_CAP,
                            "?DATA");
                    break;
                    }
                }
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
    
    #if T1_PROFILER_ACTIVE == T1_ACTIVE
    T1_profiler_end("ui_elements_handle_touches()");
    #elif T1_PROFILER_ACTIVE == T1_INACTIVE
    // Pass
    #else
    #error "T1_PROFILER_ACTIVE undefined"
    #endif
}

static void set_slider_pos_from_current_val(
    ActiveUIElement * ae,
    T1GPUzSprite * pin_gpu_zsprite,
    T1CPUzSpriteSimdStats * pin_cpu_zsprite)
{
    log_assert(ae != NULL);
    log_assert(pin_gpu_zsprite != NULL);
    log_assert(pin_gpu_zsprite->touchable_id == ae->touchable_id);
    
    // deduce the offset for the pin, based on the current value
    // let's say the slider ranges from 1.0f to 10.0f
    // and our value is at 4.0f
    // this is the same as being at 3.0f on a 0.0f to 9.0f scale
    // this iteh same as 3.0f / 9.0f = 0.3f
    double initial_pct = 0.0;
    double curval_f64;
    switch (ae->user_set.linked_type) {
        case T1_TYPE_F32:
            curval_f64 = *(float *)ae->slider_linked_value;
            initial_pct = ((curval_f64 -
                ae->user_set.custom_float_min) /
                    (ae->user_set.custom_float_max
                        - ae->user_set.custom_float_min));
        break;
        case T1_TYPE_I64:
            curval_f64 = (double)*(int64_t *)ae->slider_linked_value;
            initial_pct = ((curval_f64 -
                (double)ae->user_set.custom_int_min) /
                    ((double)ae->user_set.custom_int_max
                        - (double)ae->user_set.custom_int_min));
        break;
        case T1_TYPE_I32:
            curval_f64 = (double)*(int32_t *)ae->slider_linked_value;
            initial_pct = ((curval_f64 -
                (double)ae->user_set.custom_int_min) /
                    ((double)ae->user_set.custom_int_max
                        - (double)ae->user_set.custom_int_min));
        break;
        case T1_TYPE_I16:
            curval_f64 = (double)*(int16_t *)ae->slider_linked_value;
            initial_pct = ((curval_f64 -
                (double)ae->user_set.custom_int_min) /
                    ((double)ae->user_set.custom_int_max
                        - (double)ae->user_set.custom_int_min));
        break;
        case T1_TYPE_I8:
            curval_f64 = (double)*(int8_t *)ae->slider_linked_value;
            initial_pct = ((curval_f64 -
                (double)ae->user_set.custom_int_min) /
                    ((double)ae->user_set.custom_int_max
                        - (double)ae->user_set.custom_int_min));
        break;
        case T1_TYPE_U64:
            curval_f64 = (double)*(uint64_t *)ae->slider_linked_value;
            initial_pct = ((curval_f64 -
                (double)ae->user_set.custom_uint_min) /
                    ((double)ae->user_set.custom_uint_max
                        - (double)ae->user_set.custom_uint_min));
        break;
        case T1_TYPE_U32:
            curval_f64 = (double)*(uint32_t *)ae->slider_linked_value;
            initial_pct = ((curval_f64 -
                (double)ae->user_set.custom_uint_min) /
                    ((double)ae->user_set.custom_uint_max
                        - (double)ae->user_set.custom_uint_min));
        break;
        case T1_TYPE_U16:
            curval_f64 = (double)*(uint16_t *)ae->slider_linked_value;
            initial_pct = ((curval_f64 -
                (double)ae->user_set.custom_uint_min) /
                    ((double)ae->user_set.custom_uint_max
                        - (double)ae->user_set.custom_uint_min));
        break;
        case T1_TYPE_U8:
            curval_f64 = (double)*(uint8_t *)ae->slider_linked_value;
            initial_pct = ((curval_f64 -
                (double)ae->user_set.custom_uint_min) /
                    ((double)ae->user_set.custom_uint_max
                        - (double)ae->user_set.custom_uint_min));
        break;
        default:
            log_append("Missing type, can't deducd initial slider val");
            initial_pct = 0.5;
    }
    
    if (initial_pct < 0.0) { initial_pct = 0.0; }
    if (initial_pct > 1.0) { initial_pct = 1.0; }
    
    float new_x_offset = (float)(ae->slider_width * initial_pct) - (ae->slider_width / 2);
    
    log_assert(new_x_offset >= -ae->slider_width / 2);
    log_assert(new_x_offset <= ae->slider_width);
    
    pin_cpu_zsprite->offset_xyz[0] = new_x_offset;
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
    log_assert(next_ui_element_settings->perm.linked_type != T1_TYPE_NOTSET);
    
    ActiveUIElement * next_ae = next_active_ui_element();
    next_ae->user_set = next_ui_element_settings->perm;
    next_ae->has_label = next_ui_element_settings->slider_label != NULL;
    if (next_ae->has_label) {
        T1_std_strcpy_cap(
            next_ae->user_set.label_prefix,
            128,
            next_ui_element_settings->slider_label);
    }
    next_ae->label_dirty = true;
    
    next_ae->background_zsprite_id = background_zsprite_id;
    next_ae->pin_zsprite_id        = pin_zsprite_id;
    next_ae->label_zsprite_id      = label_zsprite_id;
    
    if (next_ui_element_settings->perm.custom_min_max_vals) {
        next_ae->user_set.custom_int_min = next_ui_element_settings->perm.custom_int_min;
        next_ae->user_set.custom_int_max =
            next_ui_element_settings->perm.custom_int_max;
    } else {
        switch (next_ui_element_settings->perm.linked_type) {
            case T1_TYPE_I32:
                next_ae->user_set.custom_int_min = INT32_MIN;
                next_ae->user_set.custom_int_max = INT32_MAX;
            break;
            case T1_TYPE_U32:
                next_ae->
                    user_set.custom_uint_min = 0;
                next_ae->
                    user_set.custom_uint_max = UINT32_MAX;
            break;
            case T1_TYPE_I64:
                next_ae->user_set.custom_int_min = INT64_MIN;
                next_ae->user_set.custom_int_max = INT64_MAX;
            break;
            case T1_TYPE_U64:
                next_ae->user_set.custom_float_min = 0;
                next_ae->user_set.custom_float_max = INT64_MAX;
            break;
            case T1_TYPE_F32:
                next_ae->user_set.custom_float_min = T1_F32_MIN;
                next_ae->user_set.custom_float_max = T1_F32_MAX;
            break;
            default:
                log_assert(0);
        }
    }
    next_ae->slider_width =
        T1_engineglobals_screenspace_width_to_width(
            next_ui_element_settings->perm.slider_width_screenspace,
            next_ui_element_settings->perm.z);
    next_ae->slideable = true;
    next_ae->deleted = false;
    next_ae->slider_linked_value = linked_value_ptr;
    next_ae->touchable_id = T1_zspriteid_next_ui_element_touchable_id();
    
    T1zSpriteRequest slider_back;
    T1_zsprite_request_next(&slider_back);
    T1_zsprite_construct_quad_around(
        /* const float mid_x: */
            T1_engineglobals_screenspace_x_to_x(
                next_ae->user_set.screenspace_x,
                next_ae->user_set.z),
        /* const float mid_y: */
            T1_engineglobals_screenspace_y_to_y(
                next_ae->user_set.screenspace_y,
                next_ae->user_set.z),
        /* const float z: */
            next_ae->user_set.z,
        /* const float width: */
            T1_engineglobals_screenspace_width_to_width(
                next_ae->user_set.
                    slider_width_screenspace,
                next_ae->user_set.z),
        /* const float height: */
            T1_engineglobals_screenspace_height_to_height(
                next_ae->user_set.slider_height_screenspace,
                next_ae->user_set.z),
        /* zPolygon * recipient: */
            &slider_back);
    
    slider_back.cpu_data->zsprite_id = background_zsprite_id;
    
    slider_back.gpu_data->base_mat = next_ae->user_set.back_mat;
    
    slider_back.gpu_data->ignore_lighting =
        next_ae->user_set.ignore_lighting;
    slider_back.gpu_data->ignore_camera =
        next_ae->user_set.ignore_camera;
    log_assert(slider_back.cpu_data->visible);
    log_assert(!slider_back.cpu_data->committed);
    log_assert(!slider_back.cpu_data->deleted);
    T1_zsprite_commit(&slider_back);
    
    T1zSpriteRequest slider_pin;
    T1_zsprite_request_next(&slider_pin);
    float pin_z = next_ae->user_set.z - 0.00001f;
    T1_zsprite_construct_quad_around(
        /* const float mid_x: */
            T1_engineglobals_screenspace_x_to_x(
                next_ae->user_set.screenspace_x,
                next_ae->user_set.z),
        /* const float mid_y: */
            T1_engineglobals_screenspace_y_to_y(
                next_ae->user_set.screenspace_y,
                next_ae->user_set.z),
        /* const float z: */
            pin_z,
        /* const float width: */
            T1_engineglobals_screenspace_width_to_width(
                next_ae->user_set.pin_width_screenspace,
                next_ae->user_set.z),
        /* const float height: */
            T1_engineglobals_screenspace_height_to_height(
                next_ae->user_set.pin_height_screenspace,
                next_ae->user_set.z),
        /* zPolygon * recipient: */
            &slider_pin);
    
    slider_pin.cpu_data->zsprite_id = pin_zsprite_id;
    slider_pin.cpu_data->simd_stats.offset_xyz[0] = 0.0f;
    slider_pin.cpu_data->simd_stats.offset_xyz[1] = 0.0f;
    
    slider_pin.gpu_data->base_mat.texturearray_i =
        next_ae->user_set.slider_pin_tex.array_i;
    slider_pin.gpu_data->base_mat.texture_i =
        next_ae->user_set.slider_pin_tex.slice_i;
    
    slider_pin.gpu_data->base_mat.diffuse_rgb[0] =
        next_ui_element_settings->slider_pin_rgba[0];
    slider_pin.gpu_data->base_mat.diffuse_rgb[1] =
        next_ui_element_settings->slider_pin_rgba[1];
    slider_pin.gpu_data->base_mat.diffuse_rgb[2] =
        next_ui_element_settings->slider_pin_rgba[2];
    slider_pin.gpu_data->base_mat.alpha =
        next_ui_element_settings->slider_pin_rgba[3];
    
    slider_pin.gpu_data->ignore_lighting =
        next_ae->user_set.ignore_lighting;
    slider_pin.gpu_data->ignore_camera =
        next_ae->user_set.ignore_camera;
    slider_pin.gpu_data->touchable_id =
        next_ae->touchable_id;
    
    set_slider_pos_from_current_val(
        next_ae,
        slider_pin.gpu_data,
        &slider_pin.cpu_data->simd_stats);
    
    T1_zsprite_commit(&slider_pin);
}

void T1_uielement_request_button(
    const int32_t button_object_id,
    const int32_t button_label_id,
    void (* onclick_funcptr)(int64_t),
    const int64_t clicked_arg)
{
    log_assert(next_ui_element_settings->perm.button_width_screenspace  > 5.0f);
    log_assert(next_ui_element_settings->perm.button_height_screenspace > 5.0f);
    
    ActiveUIElement * next_ae = next_active_ui_element();
    next_ae->clicked_arg = clicked_arg;
    next_ae->user_set = next_ui_element_settings->perm;
    next_ae->has_label = next_ui_element_settings->slider_label != NULL;
    if (next_ae->has_label) {
        T1_std_strcpy_cap(
            next_ae->user_set.label_prefix,
            128,
            next_ui_element_settings->slider_label);
    }
    next_ae->label_dirty = true;
    next_ae->slideable = false;
    next_ae->clicked_funcptr = onclick_funcptr;
    
    next_ae->background_zsprite_id = button_object_id;
    next_ae->pin_zsprite_id        = -1;
    next_ae->label_zsprite_id      = button_label_id;
    
    next_ae->slider_width =
        T1_engineglobals_screenspace_width_to_width(
            next_ui_element_settings->perm.slider_width_screenspace,
            next_ui_element_settings->perm.z);
    next_ae->deleted = false;
    next_ae->slider_linked_value = NULL;
    next_ae->touchable_id = T1_zspriteid_next_ui_element_touchable_id();
    
    T1zSpriteRequest button_request;
    T1_zsprite_request_next(&button_request);
    T1_zsprite_construct_quad_around(
        /* const float mid_x: */
            T1_engineglobals_screenspace_x_to_x(
                next_ae->user_set.screenspace_x,
                next_ae->user_set.z),
        /* const float mid_y: */
            T1_engineglobals_screenspace_y_to_y(
                next_ae->user_set.screenspace_y,
                next_ae->user_set.z),
        /* const float z: */
            next_ae->user_set.z,
        /* const float width: */
            T1_engineglobals_screenspace_width_to_width(
                next_ae->user_set.button_width_screenspace,
                next_ae->user_set.z),
        /* const float height: */
            T1_engineglobals_screenspace_height_to_height(
                next_ae->user_set.button_height_screenspace,
                next_ae->user_set.z),
        /* PolygonRequest * stack_recipient: */
            &button_request);
    
    button_request.cpu_data->zsprite_id = button_object_id;
    button_request.gpu_data->touchable_id = next_ae->touchable_id;
    button_request.gpu_data->ignore_camera =
        next_ae->user_set.ignore_camera;
    button_request.gpu_data->ignore_lighting =
        next_ae->user_set.ignore_camera;
    button_request.gpu_data->base_mat = next_ae->user_set.back_mat;
    button_request.gpu_data->alpha = 1.0f;
    T1_zsprite_commit(&button_request);
}

void T1_uielement_delete(const int32_t with_zsprite_id)
{
    for (uint32_t i = 0; i < active_ui_elements_size; i++) {
        if (active_ui_elements[i].background_zsprite_id == with_zsprite_id) {
            active_ui_elements[i].slider_linked_value = NULL;
            active_ui_elements[i].touchable_id = -1;
            active_ui_elements[i].deleted = true;
        }
    }
}

void T1_uielement_delete_all(void) {
    for (uint32_t i = 0; i < active_ui_elements_size; i++) {
        active_ui_elements[i].deleted = false;
    }
    T1_zspriteid_clear_ui_element_touchable_ids();
    active_ui_elements_size = 0;
}
