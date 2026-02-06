#include "T1_ui_widget.h"

static int32_t T1_ui_widget_sliding_touch_id = -1;
static int32_t T1_ui_widget_clicking_zsprite_id = -1;

typedef struct ActiveUIElement {
    T1UIWidgetProps props;
    
    int64_t clicked_arg;
    
    int32_t back_zsprite_id;
    int32_t pin_zsprite_id;
    int32_t label_zsprite_id;
    int32_t touch_id;
    
    bool32_t slideable;
    bool32_t deleted;
    
    void * slider_linked_value;
    void (* clicked_funcptr)(int64_t);
    
    uint8_t pos_updated_this_frame;
    uint8_t has_label;
    uint8_t label_dirty;
} ActiveUIElement;

T1UIWidgetProps * T1_ui_widget_next_props = NULL;

#define ACTIVE_UI_ELEMENTS_SIZE 5000
uint32_t T1_ui_widget_list_size = 0;
ActiveUIElement * T1_ui_widget_list = NULL;

static void T1_ui_widget_update_pos_if_needed(
    ActiveUIElement * ae)
{
    if (!ae->pos_updated_this_frame) {
        ae->pos_updated_this_frame = true;
        
        float avg_xyz[3];
        bool8_t found;
        T1_texquad_get_avg_xyz(
            avg_xyz,
            ae->back_zsprite_id,
            &found);
        
        if (found) {
            ae->props.screen_x =
                T1_render_view_x_to_screen_x_noz(
                    avg_xyz[0]);
            ae->props.screen_y =
                T1_render_view_y_to_screen_y_noz(
                    avg_xyz[1]);
            ae->props.z =
                avg_xyz[2];
        }
    }
}

static void T1_ui_widget_get_pos(
    ActiveUIElement * ae,
    float * xyz)
{
    T1_ui_widget_update_pos_if_needed(ae);
    
    xyz[0] = T1_render_view_screen_x_to_x_noz(
        ae->props.screen_x);
    xyz[1] = T1_render_view_screen_y_to_y_noz(
        ae->props.screen_y);
    xyz[2] = ae->props.z;
}

static float T1_ui_widget_get_pin_pos_given_mouse(
    ActiveUIElement * ae,
    const float mouse_x,
    double * recip_pin_pct)
{
    T1_ui_widget_update_pos_if_needed(ae);
    
    float min_slider_x =
        ae->props.screen_x -
        (ae->props.width_screen / 2);
    float max_slider_x =
        ae->props.screen_x +
        (ae->props.width_screen / 2);
    
    float new_slider_x = mouse_x;
    new_slider_x = T1_std_maxf(
        new_slider_x, min_slider_x);
    new_slider_x = T1_std_minf(
        new_slider_x, max_slider_x);
    
    *recip_pin_pct = (new_slider_x - min_slider_x) / (ae->props.width_screen);
    T1_log_assert(*recip_pin_pct >= 0.0f);
    T1_log_assert(*recip_pin_pct <= 1.0f);
    
    return T1_render_view_screen_width_to_width_noz(
            ((float)*recip_pin_pct) * ae->props.width_screen)
        - T1_render_view_screen_width_to_width_noz(
            ae->props.width_screen / 2.0f);
}

static void T1_ui_widget_get_pin_pos(
    ActiveUIElement * ae,
    float * xyz)
{
    T1_log_assert(ae != NULL);
    T1_log_assert(xyz != NULL);
    
    double initial_pct = 0.0;
    double curval_f64;
    switch (ae->props.linked_type) {
        case T1_TYPE_F32:
            curval_f64 = *(float *)ae->slider_linked_value;
            initial_pct = ((curval_f64 -
                ae->props.custom_float_min) /
                    (ae->props.custom_float_max
                        - ae->props.
                            custom_float_min));
        break;
        case T1_TYPE_I64:
            curval_f64 = (double)*(int64_t *)ae->slider_linked_value;
            initial_pct = ((curval_f64 -
                (double)ae->props.custom_int_min) /
                    ((double)ae->props.
                        custom_int_max -
                            (double)ae->props.
                                custom_int_min));
        break;
        case T1_TYPE_I32:
            curval_f64 = (double)*(int32_t *)ae->slider_linked_value;
            initial_pct = ((curval_f64 -
                (double)ae->props.custom_int_min) /
                    ((double)ae->props.custom_int_max
                        - (double)ae->props.custom_int_min));
        break;
        case T1_TYPE_I16:
            curval_f64 = (double)*(int16_t *)ae->slider_linked_value;
            initial_pct = ((curval_f64 -
                (double)ae->props.custom_int_min) /
                    ((double)ae->props.custom_int_max
                        - (double)ae->props.custom_int_min));
        break;
        case T1_TYPE_I8:
            curval_f64 = (double)*(int8_t *)ae->slider_linked_value;
            initial_pct = ((curval_f64 -
                (double)ae->props.custom_int_min) /
                    ((double)ae->props.custom_int_max
                        - (double)ae->props.custom_int_min));
        break;
        case T1_TYPE_U64:
            curval_f64 = (double)*(uint64_t *)ae->slider_linked_value;
            initial_pct = ((curval_f64 -
                (double)ae->props.custom_uint_min) /
                    ((double)ae->props.custom_uint_max
                        - (double)ae->props.custom_uint_min));
        break;
        case T1_TYPE_U32:
            curval_f64 = (double)*(uint32_t *)ae->slider_linked_value;
            initial_pct = ((curval_f64 -
                (double)ae->props.custom_uint_min) /
                    ((double)ae->props.custom_uint_max
                        - (double)ae->props.custom_uint_min));
        break;
        case T1_TYPE_U16:
            curval_f64 = (double)*(uint16_t *)ae->slider_linked_value;
            initial_pct = ((curval_f64 -
                (double)ae->props.custom_uint_min) /
                    ((double)ae->props.custom_uint_max
                        - (double)ae->props.custom_uint_min));
        break;
        case T1_TYPE_U8:
            curval_f64 = (double)*(uint8_t *)ae->slider_linked_value;
            initial_pct = ((curval_f64 -
                (double)ae->props.
                    custom_uint_min) /
                    ((double)ae->props.
                        custom_uint_max
                        - (double)ae->props.
                            custom_uint_min));
        break;
        default:
            T1_log_append("Missing type, can't deducd initial slider val");
            initial_pct = 0.5;
    }
    
    if (initial_pct < 0.0) { initial_pct = 0.0; }
    if (initial_pct > 1.0) { initial_pct = 1.0; }
    
    float slider_width =
    T1_render_view_screen_width_to_width_noz(
        /* const float screen_width: */
            ae->props.width_screen);
    
    float new_x_offset =
        (float)(slider_width *
            initial_pct) -
        (slider_width / 2);
    
    T1_log_assert(new_x_offset >= -slider_width / 2);
    T1_log_assert(new_x_offset <= slider_width);
    
    xyz[0] = new_x_offset;
    xyz[1] =
        T1_render_view_screen_y_to_y(
            ae->props.screen_y,
            1.0f);
    xyz[2] =
        ae->props.z;
}

static void T1_ui_widget_set_label(
    char * recip,
    const uint32_t recip_cap,
    ActiveUIElement * widget)
{
    if (
        widget->props.label_prefix[0] != '\0' &&
        !widget->props.is_meta_enum)
    {
        T1_std_strcpy_cap(
            recip,
            recip_cap,
            widget->props.label_prefix);
    }
    
    uint32_t good = 0;
    
    if (widget->slider_linked_value != NULL)
    {
        if (widget->props.is_meta_enum)
        {
            char * enum_as_str = NULL;
            
            switch (widget->props.linked_type)
            {
            case T1_TYPE_U8:
                enum_as_str = T1_meta_enum_uint_to_string(
                    widget->props.meta_struct_name,
                    *(uint8_t *)widget->slider_linked_value,
                    &good);
                assert(good);
            break;
            default:
            assert(0);
            }
            
            T1_std_strcat_cap(
                recip,
                recip_cap,
                enum_as_str == NULL ? "N/A" : enum_as_str);
        } else {
            T1_std_strcat_cap(
                recip,
                recip_cap,
                ": ");
            
            switch (widget->props.linked_type)
            {
            case T1_TYPE_F32:
                T1_std_strcat_float_cap(
                    recip,
                    recip_cap,
                    *(float *)widget->
                        slider_linked_value);
            break;
            case T1_TYPE_U64:
                T1_std_strcat_uint_cap(
                    recip,
                    recip_cap,
                    (uint32_t)*(uint64_t *)widget->
                        slider_linked_value);
            break;
            case T1_TYPE_U32:
                T1_std_strcat_uint_cap(
                    recip,
                    recip_cap,
                    *(uint32_t *)widget->
                        slider_linked_value);
            break;
            case T1_TYPE_U16:
                T1_std_strcat_uint_cap(
                    recip,
                    recip_cap,
                    *(uint16_t *)widget->
                        slider_linked_value);
            break;
            case T1_TYPE_U8:
                T1_std_strcat_uint_cap(
                    recip,
                    recip_cap,
                    *(uint8_t *)widget->
                        slider_linked_value);
            break;
            default:
                T1_std_strcat_cap(
                    recip,
                    recip_cap,
                    "?DATA");
            break;
            }
        }
    }
}

static ActiveUIElement *
next_active_ui_element(void)
{
    for (
        uint32_t i = 0;
        i < T1_ui_widget_list_size;
        i++)
    {
        if (T1_ui_widget_list[i].deleted)
        {
            T1_std_memset(
                &T1_ui_widget_list[i],
                0,
                sizeof(ActiveUIElement));
            
            return &T1_ui_widget_list[i];
        }
    }
    
    T1_log_assert(T1_ui_widget_list_size <
        ACTIVE_UI_ELEMENTS_SIZE);
    T1_ui_widget_list[T1_ui_widget_list_size].
        deleted = true;
    T1_ui_widget_list_size += 1;
    T1_std_memset(
        &T1_ui_widget_list
            [T1_ui_widget_list_size - 1],
        0,
        sizeof(ActiveUIElement));
    
    return &T1_ui_widget_list[
        T1_ui_widget_list_size - 1];
}

void T1_ui_widget_init(void) {
    T1_ui_widget_next_props =
        T1_mem_malloc_unmanaged(
            sizeof(T1UIWidgetProps));
    T1_std_memset(T1_ui_widget_next_props, 0, sizeof(T1UIWidgetProps));
    
    T1_ui_widget_next_props->width_screen = 100;
    T1_ui_widget_next_props->height_screen =  40;
    T1_ui_widget_next_props->tex_array_i = -1;
    T1_ui_widget_next_props->tex_slice_i = -1;
    T1_ui_widget_next_props->slider_pin_tex.array_i = -1;
    T1_ui_widget_next_props->slider_pin_tex.slice_i = -1;
    
    T1_ui_widget_list = (ActiveUIElement *)T1_mem_malloc_unmanaged(
        sizeof(ActiveUIElement) *
            ACTIVE_UI_ELEMENTS_SIZE);
    T1_std_memset(
        T1_ui_widget_list,
        0,
        sizeof(ActiveUIElement) *
            ACTIVE_UI_ELEMENTS_SIZE);
}

static void redraw_dirty_labels(void) {
    // redraw dirty labels
    for (
        uint32_t i = 0;
        i < T1_ui_widget_list_size;
        i++)
    {
        if (
            T1_ui_widget_list[i].has_label &&
            T1_ui_widget_list[i].label_dirty)
        {
            T1_ui_widget_list[i].label_dirty =
                false;
            
            T1_texquad_delete(
                T1_ui_widget_list[i].label_zsprite_id);
            
            char full_label[T1_UI_WIDGET_STR_CAP];
            T1_std_memset(
                full_label,
                0,
                T1_UI_WIDGET_STR_CAP);
            
            T1_ui_widget_set_label(
                full_label,
                T1_UI_WIDGET_STR_CAP,
                T1_ui_widget_list + i);
            
            T1_ui_widget_update_pos_if_needed(
                T1_ui_widget_list + i);
            
            float xy_screen[3];
            float z;
            
            xy_screen[0] = T1_ui_widget_list[i].props.screen_x;
            xy_screen[1] = T1_ui_widget_list[i].props.screen_y;
            z = T1_ui_widget_list[i].props.z - 0.01f;
            
            T1_text_props->font_height = (T1_ui_widget_list[i].
                props.height_screen * 2) / 3;
            T1_text_props->f32.rgba[0] = 0.5f;
            T1_text_props->f32.rgba[1] = 1.0f;
            T1_text_props->f32.rgba[2] = 0.5f;
            T1_text_props->f32.rgba[3] = 1.0f;
            
            if (full_label[0] != '\0') {
                T1_text_request_label_around(
                    /* const int32_t with_object_id: */
                        T1_ui_widget_list[i].
                            label_zsprite_id,
                    /* const char * text_to_draw: */
                        full_label,
                    /* const float mid_x_pixelspace: */
                        xy_screen[0],
                    /* const float mid_y_pixelspace: */
                        xy_screen[1],
                    /* const float z: */
                        z,
                    /* const float max_width: */
                        T1_ui_widget_list[i].props.
                            width_screen);
            }
        }
    }
}

void T1_ui_widget_handle_touches(uint64_t ms_elapsed)
{
    (void)ms_elapsed;
    
    for (
        uint32_t w_i = 0;
        w_i < T1_ui_widget_list_size;
        w_i++)
    {
        T1_ui_widget_list[w_i].
            pos_updated_this_frame = false;
    }
    
    if (T1_ui_widget_sliding_touch_id >= 0)
    {
        int32_t ui_elem_i = -1;
        for (
            int32_t elem_i = 0;
            elem_i < (int32_t)
                T1_ui_widget_list_size;
            elem_i++)
        {
            if (
                !T1_ui_widget_list[elem_i].
                    deleted &&
                T1_ui_widget_list[elem_i].
                    slideable &&
                T1_io_events
                    [T1_IO_LAST_TOUCH_OR_LCLICK_START].touch_id_top ==
                T1_ui_widget_list[elem_i].touch_id)
            {
                T1_io_events
                    [T1_IO_LAST_TOUCH_OR_LCLICK_START].
                        handled = true;
                ui_elem_i = elem_i;
                break;
            }
        }
        
        if (ui_elem_i >= 0)
        {
            T1_log_assert(
                T1_ui_widget_list[ui_elem_i].
                    slideable);
            
            T1_ui_widget_list[ui_elem_i].
                label_dirty = true;
            
            double pin_pct = 0.0f;
            float pin_offset_x =
                T1_ui_widget_get_pin_pos_given_mouse(
                T1_ui_widget_list + ui_elem_i,
                T1_io_events
                    [T1_IO_LAST_MOUSE_OR_TOUCH_MOVE].screen_x,
                &pin_pct);
            
            T1TexQuadAnim * slide = T1_texquad_anim_request_next(true);
            slide->affect_touch_id =
                T1_ui_widget_sliding_touch_id;
            slide->gpu_vals.f32.offset_xy[0] =
                pin_offset_x;
            slide->duration_us = 1;
            slide->gpu_f32_active = true;
            T1_texquad_anim_commit_and_instarun(slide);
            
            T1_log_assert(
                T1_ui_widget_list[ui_elem_i].
                    slider_linked_value != NULL);
            
            double new_val_f64 =
                T1_ui_widget_list[ui_elem_i].
                    props.custom_float_min +
                ((T1_ui_widget_list[ui_elem_i].
                    props.custom_float_max -
                T1_ui_widget_list[ui_elem_i].props.
                    custom_float_min) *
                        pin_pct);
            
            uint64_t new_val_u64 =
                T1_ui_widget_list[ui_elem_i].
                    props.custom_uint_min +
                (uint64_t)((double)(T1_ui_widget_list[ui_elem_i].
                    props.custom_uint_max -
                    T1_ui_widget_list[ui_elem_i].
                        props.custom_uint_min) *
                            pin_pct);
            
            int64_t new_val_i64 =
                T1_ui_widget_list[ui_elem_i].
                    props.custom_int_min +
                (int64_t)((double)(
                    T1_ui_widget_list[ui_elem_i].
                        props.custom_int_max -
                    T1_ui_widget_list[ui_elem_i].
                        props.custom_int_min) *
                            pin_pct);
            
            switch (T1_ui_widget_list[ui_elem_i].
                props.linked_type)
            {
                case T1_TYPE_I64:
                    *((int64_t *)T1_ui_widget_list[ui_elem_i].
                        slider_linked_value) = new_val_i64;
                break;
                case T1_TYPE_I32:
                    *((int32_t *)T1_ui_widget_list[ui_elem_i].
                        slider_linked_value) = (int32_t)new_val_i64;
                break;
                case T1_TYPE_I16:
                    *((int16_t *)T1_ui_widget_list[ui_elem_i].
                        slider_linked_value) = (int16_t)new_val_i64;
                break;
                case T1_TYPE_I8:
                    *((int8_t *)T1_ui_widget_list[ui_elem_i].
                        slider_linked_value) = (int8_t)new_val_i64;
                break;
                case T1_TYPE_U64:
                    *((uint64_t *)T1_ui_widget_list[ui_elem_i].
                        slider_linked_value) = new_val_u64;
                break;
                case T1_TYPE_U32:
                    *((uint32_t *)T1_ui_widget_list[ui_elem_i].
                        slider_linked_value) =
                            (uint32_t)new_val_u64;
                break;
                case T1_TYPE_U16:
                    *((uint16_t *)T1_ui_widget_list[ui_elem_i].
                        slider_linked_value) =
                            (uint16_t)new_val_u64;
                break;
                case T1_TYPE_U8:
                    *((uint8_t *)T1_ui_widget_list[ui_elem_i].
                        slider_linked_value) =
                            (uint8_t)new_val_u64;
                break;
                case T1_TYPE_F32:
                    *((float *)T1_ui_widget_list[ui_elem_i].
                        slider_linked_value) = (float)new_val_f64;
                break;
                default:
                    log_dump_and_crash(
                        "Unhandled slider data type");
            }
        }
    }
    
    if (
        !T1_io_events[T1_IO_LAST_TOUCH_OR_LCLICK_END].
            handled)
    {
        if (T1_ui_widget_sliding_touch_id >= 0)
        {
            T1_ui_widget_sliding_touch_id = -1;
        }
        
        if (
            T1_ui_widget_clicking_zsprite_id >= 0)
        {
            int32_t ui_elem_i = -1;
            for (
                ui_elem_i = 0;
                ui_elem_i < (int32_t)T1_ui_widget_list_size;
                ui_elem_i++)
            {
                if (
                    !T1_ui_widget_list[ui_elem_i].deleted &&
                    T1_ui_widget_list[ui_elem_i].
                        clicked_funcptr != NULL &&
                    T1_io_events[T1_IO_LAST_TOUCH_OR_LCLICK_END].
                        touch_id_top ==
                    T1_ui_widget_list[ui_elem_i].
                        touch_id)
                {
                    T1_io_events[T1_IO_LAST_TOUCH_OR_LCLICK_END].
                        handled = true;
                    T1_ui_widget_list[ui_elem_i].
                        clicked_funcptr(
                            T1_ui_widget_list[ui_elem_i].clicked_arg);
                    break;
                }
            }
            
            T1_ui_widget_sliding_touch_id = -1;
        }
    }
    
    if (
        !T1_io_events[T1_IO_LAST_TOUCH_OR_LCLICK_START].
            handled)
    {
        if (
            T1_io_events[T1_IO_LAST_TOUCH_OR_LCLICK_START].
                touch_id_top >= 0 &&
            T1_io_events[T1_IO_LAST_TOUCH_OR_LCLICK_START].
                touch_id_top < T1_ZSPRITEID_LAST_UI_TOUCH)
        {
            for (
                uint32_t i = 0;
                i < T1_ui_widget_list_size;
                i++)
            {
                if (
                    T1_ui_widget_list[i].deleted ||
                    T1_io_events
                        [T1_IO_LAST_TOUCH_OR_LCLICK_START].
                            touch_id_top !=
                    T1_ui_widget_list[i].touch_id)
                {
                    continue;
                }
                
                if (T1_ui_widget_list[i].slideable)
                {
                    T1_ui_widget_sliding_touch_id =
                        T1_ui_widget_list[i].
                            touch_id;
                    T1_io_events
                        [T1_IO_LAST_TOUCH_OR_LCLICK_END].
                            handled = true;
                }
                
                if (T1_ui_widget_list[i].clicked_funcptr != NULL) {
                    T1_ui_widget_clicking_zsprite_id =
                        T1_ui_widget_list[i].back_zsprite_id;
                }
                
                if (
                    T1_ui_widget_list[i].props.
                        sfx_filename[0] != '\0')
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
                
                T1_io_events[T1_IO_LAST_TOUCH_OR_LCLICK_START].
                    handled = true;
            }
        }
    }
    
    redraw_dirty_labels();
}

void T1_ui_widget_request_slider(
    const int32_t back_zsprite_id,
    const int32_t label_zsprite_id,
    const int32_t pin_zsprite_id,
    void * linked_value_ptr)
{
    T1_log_assert(T1_ui_widget_next_props != NULL);
    T1_log_assert(back_zsprite_id != pin_zsprite_id);
    T1_log_assert(back_zsprite_id != label_zsprite_id);
    T1_log_assert(T1_ui_widget_next_props->
        width_screen > 0);
    T1_log_assert(T1_ui_widget_next_props->
        height_screen > 0);
    T1_log_assert(T1_ui_widget_next_props->
        pin_width_screen > 0);
    T1_log_assert(T1_ui_widget_next_props->
        pin_height_screen > 0);
    T1_log_assert(T1_ui_widget_next_props->
        linked_type != T1_TYPE_NOTSET);
    
    ActiveUIElement * next_ae =
        next_active_ui_element();
    next_ae->props = *T1_ui_widget_next_props;
    next_ae->has_label = next_ae->props.label_prefix[0] != '\0';
    next_ae->label_dirty = true;
    next_ae->back_zsprite_id = back_zsprite_id;
    next_ae->pin_zsprite_id = pin_zsprite_id;
    next_ae->label_zsprite_id = label_zsprite_id;
    
    if (!T1_ui_widget_next_props->
        custom_min_max_vals)
    {
        switch (
            T1_ui_widget_next_props->linked_type)
        {
            case T1_TYPE_I32:
                next_ae->props.custom_int_min = INT32_MIN;
                next_ae->props.custom_int_max = INT32_MAX;
            break;
            case T1_TYPE_U32:
                next_ae->props.
                    custom_uint_min = 0;
                next_ae->props.
                    custom_uint_max = UINT32_MAX;
            break;
            case T1_TYPE_I64:
                next_ae->props.custom_int_min = INT64_MIN;
                next_ae->props.custom_int_max = INT64_MAX;
            break;
            case T1_TYPE_U64:
                next_ae->props.custom_float_min = 0;
                next_ae->props.custom_float_max = INT64_MAX;
            break;
            case T1_TYPE_F32:
                next_ae->props.custom_float_min = T1_F32_MIN;
                next_ae->props.custom_float_max = T1_F32_MAX;
            break;
            default:
                T1_log_assert(0);
        }
    }
    
    next_ae->slideable = true;
    next_ae->deleted = false;
    next_ae->slider_linked_value = linked_value_ptr;
    next_ae->touch_id = T1_zspriteid_next_ui_element_touch_id();
    
    float xyz_pos[3];
    T1_ui_widget_get_pos(
        next_ae,
        xyz_pos);
    
    T1FlatTexQuadRequest slider_back;
    T1_texquad_fetch_next(&slider_back);
    
    slider_back.gpu->f32.xyz[0] = xyz_pos[0];
    slider_back.gpu->f32.xyz[1] = xyz_pos[1];
    slider_back.gpu->f32.xyz[2] = xyz_pos[2];
    
    slider_back.gpu->f32.size_xy[0] =
        T1_render_view_screen_width_to_width_noz(
            next_ae->props.
                width_screen);
    slider_back.gpu->f32.size_xy[1] =
        T1_render_view_screen_height_to_height_noz(
            next_ae->props.
                height_screen);
    
    slider_back.gpu->f32.rgba[0] = 0.0f;
    slider_back.gpu->f32.rgba[1] = 0.0f;
    slider_back.gpu->f32.rgba[2] = 0.5f;
    slider_back.gpu->f32.rgba[3] = 1.0f;
    
    slider_back.cpu->zsprite_id =
        next_ae->back_zsprite_id;
    T1_texquad_commit(&slider_back);
    
    T1FlatTexQuadRequest slider_pin;
    T1_texquad_fetch_next(&slider_pin);
    float pin_z = next_ae->props.z - 0.00001f;
    slider_pin.gpu->f32.xyz[0] = xyz_pos[0];
    slider_pin.gpu->f32.xyz[1] = xyz_pos[1];
    slider_pin.gpu->f32.xyz[2] = pin_z;
    slider_pin.gpu->f32.size_xy[0] =
        T1_render_view_screen_width_to_width_noz(
            next_ae->props.
                pin_width_screen);
    slider_pin.gpu->f32.size_xy[1] =
        T1_render_view_screen_height_to_height_noz(
            next_ae->props.
                pin_height_screen);
    slider_pin.cpu->zsprite_id = pin_zsprite_id;
    
    slider_pin.gpu->i32.tex_array_i =
        next_ae->props.slider_pin_tex.array_i;
    slider_pin.gpu->i32.tex_slice_i =
        next_ae->props.slider_pin_tex.slice_i;
    
    slider_pin.gpu->f32.rgba[0] =
        T1_ui_widget_next_props->pin_rgba[0];
    slider_pin.gpu->f32.rgba[1] =
        T1_ui_widget_next_props->pin_rgba[1];
    slider_pin.gpu->f32.rgba[2] =
        T1_ui_widget_next_props->pin_rgba[2];
    slider_pin.gpu->f32.rgba[3] =
        T1_ui_widget_next_props->pin_rgba[3];
    slider_pin.gpu->i32.touch_id =
        next_ae->touch_id;
    
    slider_pin.gpu->f32.xyz[0] = xyz_pos[0];
    slider_pin.gpu->f32.xyz[1] = xyz_pos[1];
    slider_pin.gpu->f32.xyz[2] = xyz_pos[2];
    
    T1_texquad_commit(&slider_pin);
}

void T1_ui_widget_request_button(
    const int32_t button_object_id,
    const int32_t button_label_id,
    void (* onclick_funcptr)(int64_t),
    const int64_t clicked_arg)
{
    T1_log_assert(T1_ui_widget_next_props->
        width_screen > 5.0f);
    T1_log_assert(T1_ui_widget_next_props->
        height_screen > 5.0f);
    
    ActiveUIElement * next_ae =
        next_active_ui_element();
    next_ae->clicked_arg = clicked_arg;
    next_ae->props = *T1_ui_widget_next_props;
    next_ae->has_label = T1_ui_widget_next_props->
        label_prefix[0] != '\0';
    
    next_ae->label_dirty = true;
    next_ae->slideable = false;
    next_ae->clicked_funcptr = onclick_funcptr;
    
    next_ae->back_zsprite_id = button_object_id;
    next_ae->pin_zsprite_id = -1;
    next_ae->label_zsprite_id = button_label_id;
    
    next_ae->deleted = false;
    next_ae->slider_linked_value = NULL;
    next_ae->touch_id = T1_zspriteid_next_ui_element_touch_id();
    
    float btn_pos[3];
    T1_ui_widget_get_pos(
        next_ae,
        btn_pos);
    
    T1FlatTexQuadRequest button_request;
    T1_texquad_fetch_next(&button_request);
    
    button_request.gpu->f32.xyz[0] = btn_pos[0];
    button_request.gpu->f32.xyz[1] = btn_pos[1];
    button_request.gpu->f32.xyz[2] = btn_pos[2];
    button_request.gpu->f32.size_xy[0] =
        T1_render_view_screen_width_to_width_noz(
            next_ae->props.
                width_screen);
    button_request.gpu->f32.size_xy[1] =
        T1_render_view_screen_height_to_height_noz(
            next_ae->props.
                height_screen);
    
    button_request.cpu->zsprite_id = button_object_id;
    button_request.gpu->i32.touch_id = next_ae->touch_id;
    button_request.gpu->f32.rgba[0] = 0.0f;
    button_request.gpu->f32.rgba[1] = 0.0f;
    button_request.gpu->f32.rgba[2] = 0.65f;
    button_request.gpu->f32.rgba[3] = 1.0f;
    T1_texquad_commit(&button_request);
}

void T1_ui_widget_delete(const int32_t with_zsprite_id)
{
    for (
        uint32_t i = 0;
        i < T1_ui_widget_list_size;
        i++)
    {
        if (
            T1_ui_widget_list[i].back_zsprite_id ==
                with_zsprite_id)
        {
            T1_ui_widget_list[i].
                slider_linked_value = NULL;
            T1_ui_widget_list[i].touch_id = -1;
            T1_ui_widget_list[i].deleted = true;
        }
    }
}

void T1_ui_widget_delete_all(void) {
    for (
        uint32_t i = 0;
        i < T1_ui_widget_list_size;
        i++)
    {
        T1_ui_widget_list[i].deleted = false;
    }
    T1_zspriteid_clear_ui_element_touch_ids();
    T1_ui_widget_list_size = 0;
}
