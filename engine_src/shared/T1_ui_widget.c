#include "T1_ui_widget.h"

#include "T1_std.h"
#include "T1_log.h"
#include "T1_mem.h"
#include "T1_global.h"
#include "T1_id.h"
#include "T1_meta.h"
#include "T1_io.h"
#include "T1_zsprite.h"
#include "T1_tex_array.h"
#include "T1_zsprite_anim.h"
#include "T1_texquad_anim.h"
#include "T1_text.h"
#include "T1_profiler.h"
#include "T1_render_view.h"


static s32 T1_ui_widget_sliding_touch_id = -1;
static s32 T1_ui_widget_clicking_T1_id = -1;

typedef struct ActiveUIElement {
    T1UIWidgetProps props;
    
    s64 clicked_arg;
    
    s32 back_T1_id;
    s32 pin_T1_id;
    s32 label_T1_id;
    s32 touch_id;
    
    void * slider_linked_value;
    void (* clicked_funcptr)(s64);
    
    u8 slideable;
    u8 deleted;
    u8 pos_updated_this_frame;
    u8 has_label;
    u8 label_dirty;
} ActiveUIElement;

T1UIWidgetProps * T1_ui_widget_next_props = NULL;

#define ACTIVE_UI_ELEMENTS_SIZE 5000
u32 T1_ui_widget_list_size = 0;
ActiveUIElement * T1_ui_widget_list = NULL;

static void T1_ui_widget_update_pos_if_needed(
    ActiveUIElement * ae)
{
    if (!ae->pos_updated_this_frame) {
        ae->pos_updated_this_frame = true;
        
        f32 avg_xyz[3];
        b8 found;
        T1_texquad_get_avg_xyz(
            avg_xyz,
            ae->back_T1_id,
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
    f32 * xyz)
{
    T1_ui_widget_update_pos_if_needed(ae);
    
    xyz[0] = T1_render_view_screen_x_to_x_noz(
        ae->props.screen_x);
    xyz[1] = T1_render_view_screen_y_to_y_noz(
        ae->props.screen_y);
    xyz[2] = ae->props.z;
}

static f32 T1_ui_widget_get_pin_pos_given_mouse(
    ActiveUIElement * ae,
    const f32 mouse_x,
    f64 * recip_pin_pct)
{
    T1_ui_widget_update_pos_if_needed(ae);
    
    f32 min_slider_x =
        ae->props.screen_x -
        (ae->props.width_screen / 2);
    f32 max_slider_x =
        ae->props.screen_x +
        (ae->props.width_screen / 2);
    
    f32 new_slider_x = mouse_x;
    new_slider_x = T1_std_maxf(
        new_slider_x, min_slider_x);
    new_slider_x = T1_std_minf(
        new_slider_x, max_slider_x);
    
    *recip_pin_pct = (new_slider_x - min_slider_x) / (ae->props.width_screen);
    T1_log_assert(*recip_pin_pct >= 0.0f);
    T1_log_assert(*recip_pin_pct <= 1.0f);
    
    return T1_render_view_screen_width_to_width_noz(
            ((f32)*recip_pin_pct) * ae->props.width_screen)
        - T1_render_view_screen_width_to_width_noz(
            ae->props.width_screen / 2.0f);
}

#if 0
static void T1_ui_widget_get_pin_pos(
    ActiveUIElement * ae,
    f32 * xyz)
{
    T1_log_assert(ae != NULL);
    T1_log_assert(xyz != NULL);
    
    f64 initial_pct = 0.0;
    f64 curval_f64;
    switch (ae->props.linked_type) {
        case T1_TYPE_F32:
            curval_f64 = *(f32 *)ae->slider_linked_value;
            initial_pct = ((curval_f64 -
                ae->props.custom_f32_min) /
                    (ae->props.custom_f32_max
                        - ae->props.
                            custom_f32_min));
        break;
        case T1_TYPE_I64:
            curval_f64 = (f64)*(i64 *)ae->slider_linked_value;
            initial_pct = ((curval_f64 -
                (f64)ae->props.custom_int_min) /
                    ((f64)ae->props.
                        custom_int_max -
                            (f64)ae->props.
                                custom_int_min));
        break;
        case T1_TYPE_I32:
            curval_f64 = (f64)*(s32 *)ae->slider_linked_value;
            initial_pct = ((curval_f64 -
                (f64)ae->props.custom_int_min) /
                    ((f64)ae->props.custom_int_max
                        - (f64)ae->props.custom_int_min));
        break;
        case T1_TYPE_I16:
            curval_f64 = (f64)*(i16 *)ae->slider_linked_value;
            initial_pct = ((curval_f64 -
                (f64)ae->props.custom_int_min) /
                    ((f64)ae->props.custom_int_max
                        - (f64)ae->props.custom_int_min));
        break;
        case T1_TYPE_I8:
            curval_f64 = (f64)*(i8 *)ae->slider_linked_value;
            initial_pct = ((curval_f64 -
                (f64)ae->props.custom_int_min) /
                    ((f64)ae->props.custom_int_max
                        - (f64)ae->props.custom_int_min));
        break;
        case T1_TYPE_U64:
            curval_f64 = (f64)*(u64 *)ae->slider_linked_value;
            initial_pct = ((curval_f64 -
                (f64)ae->props.custom_uint_min) /
                    ((f64)ae->props.custom_uint_max
                        - (f64)ae->props.custom_uint_min));
        break;
        case T1_TYPE_U32:
            curval_f64 = (f64)*(u32 *)ae->slider_linked_value;
            initial_pct = ((curval_f64 -
                (f64)ae->props.custom_uint_min) /
                    ((f64)ae->props.custom_uint_max
                        - (f64)ae->props.custom_uint_min));
        break;
        case T1_TYPE_U16:
            curval_f64 = (f64)*(u16 *)ae->slider_linked_value;
            initial_pct = ((curval_f64 -
                (f64)ae->props.custom_uint_min) /
                    ((f64)ae->props.custom_uint_max
                        - (f64)ae->props.custom_uint_min));
        break;
        case T1_TYPE_U8:
            curval_f64 = (f64)*(u8 *)ae->slider_linked_value;
            initial_pct = ((curval_f64 -
                (f64)ae->props.
                    custom_uint_min) /
                    ((f64)ae->props.
                        custom_uint_max
                        - (f64)ae->props.
                            custom_uint_min));
        break;
        default:
            T1_log_append("Missing type, can't deducd initial slider val");
            initial_pct = 0.5;
    }
    
    if (initial_pct < 0.0) { initial_pct = 0.0; }
    if (initial_pct > 1.0) { initial_pct = 1.0; }
    
    f32 slider_width =
    T1_render_view_screen_width_to_width_noz(
        /* const f32 screen_width: */
            ae->props.width_screen);
    
    f32 new_x_offset =
        (f32)(slider_width *
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
#endif

static void T1_ui_widget_set_label(
    char * recip,
    const u32 recip_cap,
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
    
    u8 good = 0;
    
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
                    *(u8 *)widget->slider_linked_value,
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
                T1_std_strcat_f32_cap(
                    recip,
                    recip_cap,
                    *(f32 *)widget->
                        slider_linked_value);
            break;
            case T1_TYPE_U64:
                T1_std_strcat_u32_cap(
                    recip,
                    recip_cap,
                    (u32)*(u64 *)widget->
                        slider_linked_value);
            break;
            case T1_TYPE_U32:
                T1_std_strcat_u32_cap(
                    recip,
                    recip_cap,
                    *(u32 *)widget->
                        slider_linked_value);
            break;
            case T1_TYPE_U16:
                T1_std_strcat_u32_cap(
                    recip,
                    recip_cap,
                    *(u16 *)widget->
                        slider_linked_value);
            break;
            case T1_TYPE_U8:
                T1_std_strcat_u32_cap(
                    recip,
                    recip_cap,
                    *(u8 *)widget->
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
        u32 i = 0;
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
    T1_ui_widget_next_props->slider_pin_tex = T1_TEX_NONE;
    
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
        u32 i = 0;
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
                T1_ui_widget_list[i].label_T1_id);
            
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
            
            f32 xy_screen[3];
            f32 z;
            
            xy_screen[0] = T1_ui_widget_list[i].props.screen_x;
            xy_screen[1] = T1_ui_widget_list[i].props.screen_y;
            z = T1_ui_widget_list[i].props.z - 0.01f;
            
            T1_text_props->font_height = (T1_ui_widget_list[i].
                props.height_screen * 2) / 3;
            T1_text_props->f32s.rgba[0] = 0.5f;
            T1_text_props->f32s.rgba[1] = 1.0f;
            T1_text_props->f32s.rgba[2] = 0.5f;
            T1_text_props->f32s.rgba[3] = 1.0f;
            
            if (full_label[0] != '\0') {
                T1_text_request_label_around(
                    /* const s32 with_object_id: */
                        T1_ui_widget_list[i].
                            label_T1_id,
                    /* const char * text_to_draw: */
                        full_label,
                    /* const f32 mid_x_pixelspace: */
                        xy_screen[0],
                    /* const f32 mid_y_pixelspace: */
                        xy_screen[1],
                    /* const f32 z: */
                        z,
                    /* const f32 max_width: */
                        T1_ui_widget_list[i].props.
                            width_screen);
            }
        }
    }
}

b8 T1_ui_widget_handle_lclick(void) {
    T1_log_assert(0);
    
    return false;
}

#if 0
void T1_ui_widget_handle_touches(u64 ms_elapsed)
{
    (void)ms_elapsed;
    
    for (
        u32 w_i = 0;
        w_i < T1_ui_widget_list_size;
        w_i++)
    {
        T1_ui_widget_list[w_i].
            pos_updated_this_frame = false;
    }
    
    if (T1_ui_widget_sliding_touch_id >= 0)
    {
        s32 ui_elem_i = -1;
        for (
            s32 elem_i = 0;
            elem_i < (s32)
                T1_ui_widget_list_size;
            elem_i++)
        {
            if (
                !T1_ui_widget_list[elem_i].
                    deleted &&
                T1_ui_widget_list[elem_i].
                    slideable &&
                T1_io->events
                    [T1_IO_TOUCH_OR_LCLICK_START].touch_id_top ==
                T1_ui_widget_list[elem_i].touch_id)
            {
                T1_io->events
                    [T1_IO_TOUCH_OR_LCLICK_START].
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
            
            f64 pin_pct = 0.0f;
            f32 pin_offset_x =
                T1_ui_widget_get_pin_pos_given_mouse(
                T1_ui_widget_list + ui_elem_i,
                T1_io->events
                    [T1_IO_MOUSE_OR_TOUCH_MOVE].screen_x,
                &pin_pct);
            
            T1TexQuadAnim * slide = T1_texquad_anim_request_next(true);
            slide->affect_touch_id =
                T1_ui_widget_sliding_touch_id;
            slide->gpu_vals.f32s.offset_xy[0] = pin_offset_x;
            slide->duration_us = 1;
            slide->gpu_f32_active = true;
            T1_texquad_anim_commit_and_instarun(slide);
            
            T1_log_assert(
                T1_ui_widget_list[ui_elem_i].
                    slider_linked_value != NULL);
            
            f64 new_val_f64 =
                T1_ui_widget_list[ui_elem_i].
                    props.custom_fmin +
                ((T1_ui_widget_list[ui_elem_i].
                    props.custom_fmax -
                T1_ui_widget_list[ui_elem_i].props.
                    custom_fmin) *
                        pin_pct);
            
            u64 new_val_u64 =
                T1_ui_widget_list[ui_elem_i].
                    props.custom_umin +
                (u64)((f64)(T1_ui_widget_list[ui_elem_i].
                    props.custom_umax -
                    T1_ui_widget_list[ui_elem_i].
                        props.custom_umin) *
                            pin_pct);
            
            s64 new_val_i64 =
                T1_ui_widget_list[ui_elem_i].
                    props.custom_smin +
                (s64)((f64)(
                    T1_ui_widget_list[ui_elem_i].
                        props.custom_smax -
                    T1_ui_widget_list[ui_elem_i].
                        props.custom_smin) *
                            pin_pct);
            
            switch (T1_ui_widget_list[ui_elem_i].
                props.linked_type)
            {
                case T1_TYPE_I64:
                    *((s64 *)T1_ui_widget_list[ui_elem_i].
                        slider_linked_value) = new_val_i64;
                break;
                case T1_TYPE_I32:
                    *((s32 *)T1_ui_widget_list[ui_elem_i].
                        slider_linked_value) = (s32)new_val_i64;
                break;
                case T1_TYPE_I16:
                    *((s16 *)T1_ui_widget_list[ui_elem_i].
                        slider_linked_value) = (s16)new_val_i64;
                break;
                case T1_TYPE_I8:
                    *((s8 *)T1_ui_widget_list[ui_elem_i].
                        slider_linked_value) = (s8)new_val_i64;
                break;
                case T1_TYPE_U64:
                    *((u64 *)T1_ui_widget_list[ui_elem_i].
                        slider_linked_value) = new_val_u64;
                break;
                case T1_TYPE_U32:
                    *((u32 *)T1_ui_widget_list[ui_elem_i].
                        slider_linked_value) =
                            (u32)new_val_u64;
                break;
                case T1_TYPE_U16:
                    *((u16 *)T1_ui_widget_list[ui_elem_i].
                        slider_linked_value) =
                            (u16)new_val_u64;
                break;
                case T1_TYPE_U8:
                    *((u8 *)T1_ui_widget_list[ui_elem_i].
                        slider_linked_value) =
                            (u8)new_val_u64;
                break;
                case T1_TYPE_F32:
                    *((f32 *)T1_ui_widget_list[ui_elem_i].
                        slider_linked_value) = (f32)new_val_f64;
                break;
                default:
                    T1_log_dump_and_crash(
                        "Unhandled slider data type");
            }
        }
    }
    
    if (
        !T1_io->events[T1_IO_TOUCH_OR_LCLICK_END].
            handled)
    {
        if (T1_ui_widget_sliding_touch_id >= 0)
        {
            T1_ui_widget_sliding_touch_id = -1;
        }
        
        if (
            T1_ui_widget_clicking_T1_id >= 0)
        {
            s32 ui_elem_i = -1;
            for (
                ui_elem_i = 0;
                ui_elem_i < (s32)T1_ui_widget_list_size;
                ui_elem_i++)
            {
                if (
                    !T1_ui_widget_list[ui_elem_i].deleted &&
                    T1_ui_widget_list[ui_elem_i].
                        clicked_funcptr != NULL &&
                    T1_io->events[T1_IO_TOUCH_OR_LCLICK_END].
                        touch_id_top ==
                    T1_ui_widget_list[ui_elem_i].
                        touch_id)
                {
                    T1_io->events[T1_IO_TOUCH_OR_LCLICK_END].
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
        !T1_io->events[T1_IO_TOUCH_OR_LCLICK_START].
            handled)
    {
        if (
            T1_io->events[T1_IO_TOUCH_OR_LCLICK_START].
                touch_id_top >= 0 &&
            T1_io->events[T1_IO_TOUCH_OR_LCLICK_START].
                touch_id_top < T1_ID_LAST_UI_TOUCH)
        {
            for (
                u32 i = 0;
                i < T1_ui_widget_list_size;
                i++)
            {
                if (
                    T1_ui_widget_list[i].deleted ||
                    T1_io->events
                        [T1_IO_TOUCH_OR_LCLICK_START].
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
                    T1_io->events
                        [T1_IO_TOUCH_OR_LCLICK_END].
                            handled = true;
                }
                
                if (T1_ui_widget_list[i].clicked_funcptr != NULL) {
                    T1_ui_widget_clicking_T1_id =
                        T1_ui_widget_list[i].back_T1_id;
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
                
                T1_io->events[T1_IO_TOUCH_OR_LCLICK_START].
                    handled = true;
            }
        }
    }
    
    redraw_dirty_labels();
}
#endif

void T1_ui_widget_request_slider(
    const s32 back_T1_id,
    const s32 label_T1_id,
    const s32 pin_T1_id,
    void * linked_value_ptr)
{
    T1_log_assert(T1_ui_widget_next_props != NULL);
    T1_log_assert(back_T1_id != pin_T1_id);
    T1_log_assert(back_T1_id != label_T1_id);
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
    next_ae->back_T1_id = back_T1_id;
    next_ae->pin_T1_id = pin_T1_id;
    next_ae->label_T1_id = label_T1_id;
    
    if (!T1_ui_widget_next_props->
        custom_min_max_vals)
    {
        switch (
            T1_ui_widget_next_props->linked_type)
        {
            case T1_TYPE_I32:
                next_ae->props.custom_smin = INT32_MIN;
                next_ae->props.custom_smax = INT32_MAX;
            break;
            case T1_TYPE_U32:
                next_ae->props.
                    custom_umin = 0;
                next_ae->props.
                    custom_umax = UINT32_MAX;
            break;
            case T1_TYPE_I64:
                next_ae->props.custom_smin = INT64_MIN;
                next_ae->props.custom_smax = INT64_MAX;
            break;
            case T1_TYPE_U64:
                next_ae->props.custom_fmin = 0;
                next_ae->props.custom_fmax = INT64_MAX;
            break;
            case T1_TYPE_F32:
                next_ae->props.custom_fmin = T1_F32_MIN;
                next_ae->props.custom_fmax = T1_F32_MAX;
            break;
            default:
                T1_log_assert(0);
        }
    }
    
    next_ae->slideable = true;
    next_ae->deleted = false;
    next_ae->slider_linked_value = linked_value_ptr;
    next_ae->touch_id = T1_id_next_ui_element_touch_id();
    
    f32 xyz_pos[3];
    T1_ui_widget_get_pos(
        next_ae,
        xyz_pos);
    
    T1FlatTexQuadRequest slider_back;
    T1_texquad_fetch_next(&slider_back);
    
    slider_back.gpu->f32s.xyz[0] = xyz_pos[0];
    slider_back.gpu->f32s.xyz[1] = xyz_pos[1];
    slider_back.gpu->f32s.xyz[2] = xyz_pos[2];
    
    slider_back.gpu->f32s.wh[0] =
        T1_render_view_screen_width_to_width_noz(
            next_ae->props.
                width_screen);
    slider_back.gpu->f32s.wh[1] =
        T1_render_view_screen_height_to_height_noz(
            next_ae->props.
                height_screen);
    
    slider_back.gpu->f32s.rgba[0] = 0.0f;
    slider_back.gpu->f32s.rgba[1] = 0.0f;
    slider_back.gpu->f32s.rgba[2] = 0.5f;
    slider_back.gpu->f32s.rgba[3] = 1.0f;
    
    slider_back.cpu->T1_id =
        next_ae->back_T1_id;
    T1_texquad_commit(&slider_back);
    
    T1FlatTexQuadRequest slider_pin;
    T1_texquad_fetch_next(&slider_pin);
    f32 pin_z = next_ae->props.z - 0.00001f;
    slider_pin.gpu->f32s.xyz[0] = xyz_pos[0];
    slider_pin.gpu->f32s.xyz[1] = xyz_pos[1];
    slider_pin.gpu->f32s.xyz[2] = pin_z;
    slider_pin.gpu->f32s.wh[0] =
        T1_render_view_screen_width_to_width_noz(
            next_ae->props.
                pin_width_screen);
    slider_pin.gpu->f32s.wh[1] =
        T1_render_view_screen_height_to_height_noz(
            next_ae->props.
                pin_height_screen);
    slider_pin.cpu->T1_id = pin_T1_id;
    
    slider_pin.gpu->s32s.reserved_and_tex =
        0x00000000 | next_ae->props.slider_pin_tex;
    
    slider_pin.gpu->f32s.rgba[0] =
        T1_ui_widget_next_props->pin_rgba[0];
    slider_pin.gpu->f32s.rgba[1] =
        T1_ui_widget_next_props->pin_rgba[1];
    slider_pin.gpu->f32s.rgba[2] =
        T1_ui_widget_next_props->pin_rgba[2];
    slider_pin.gpu->f32s.rgba[3] =
        T1_ui_widget_next_props->pin_rgba[3];
    slider_pin.gpu->s32s.touch_id =
        next_ae->touch_id;
    
    slider_pin.gpu->f32s.xyz[0] = xyz_pos[0];
    slider_pin.gpu->f32s.xyz[1] = xyz_pos[1];
    slider_pin.gpu->f32s.xyz[2] = xyz_pos[2];
    
    T1_texquad_commit(&slider_pin);
}

void T1_ui_widget_request_button(
    const s32 button_object_id,
    const s32 button_label_id,
    void (* onclick_funcptr)(s64),
    const s64 clicked_arg)
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
    
    next_ae->back_T1_id = button_object_id;
    next_ae->pin_T1_id = -1;
    next_ae->label_T1_id = button_label_id;
    
    next_ae->deleted = false;
    next_ae->slider_linked_value = NULL;
    next_ae->touch_id = T1_id_next_ui_element_touch_id();
    
    f32 btn_pos[3];
    T1_ui_widget_get_pos(
        next_ae,
        btn_pos);
    
    T1FlatTexQuadRequest button_request;
    T1_texquad_fetch_next(&button_request);
    
    button_request.gpu->f32s.xyz[0] = btn_pos[0];
    button_request.gpu->f32s.xyz[1] = btn_pos[1];
    button_request.gpu->f32s.xyz[2] = btn_pos[2];
    button_request.gpu->f32s.wh[0] =
        T1_render_view_screen_width_to_width_noz(
            next_ae->props.
                width_screen);
    button_request.gpu->f32s.wh[1] =
        T1_render_view_screen_height_to_height_noz(
            next_ae->props.
                height_screen);
    
    button_request.cpu->T1_id = button_object_id;
    button_request.gpu->s32s.touch_id = next_ae->touch_id;
    button_request.gpu->f32s.rgba[0] = 0.0f;
    button_request.gpu->f32s.rgba[1] = 0.0f;
    button_request.gpu->f32s.rgba[2] = 0.65f;
    button_request.gpu->f32s.rgba[3] = 1.0f;
    T1_texquad_commit(&button_request);
}

void T1_ui_widget_delete(const s32 with_T1_id)
{
    for (
        u32 i = 0;
        i < T1_ui_widget_list_size;
        i++)
    {
        if (T1_ui_widget_list[i].back_T1_id == with_T1_id)
        {
            T1_ui_widget_list[i].slider_linked_value = NULL;
            T1_ui_widget_list[i].touch_id = -1;
            T1_ui_widget_list[i].deleted = true;
        }
    }
}

void T1_ui_widget_delete_all(void) {
    for (
        u32 i = 0;
        i < T1_ui_widget_list_size;
        i++)
    {
        T1_ui_widget_list[i].deleted = false;
    }
    T1_id_clear_ui_element_touch_ids();
    T1_ui_widget_list_size = 0;
}

/*
3D models
*/
f32 T1_get_x_mul_for_width(const s32 for_mesh_id, const f32 for_width) {
    return T1_global_get_x_mul_for_width(for_mesh_id, for_width); }
f32 T1_get_y_mul_for_height(const s32 for_mesh_id, const f32 for_height) {
    return T1_global_get_y_mul_for_height(for_mesh_id, for_height); }
f32 T1_get_z_mul_for_depth(const s32 for_mesh_id, const f32 for_depth) {
    return T1_global_get_z_mul_for_depth(for_mesh_id, for_depth); }
