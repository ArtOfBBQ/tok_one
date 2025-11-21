#include "T1_clientlogic.h"

static int32_t base_mesh_id = 1;
static int32_t example_particles_id = -1;

#if 0
static void load_obj_basemodel(
    char error_message[128],
    uint32_t * success)
{
    *success = 0;
    
    char writables_path[256];
    writables_path[0] = '\0';
    
    T1_platform_get_writables_path(
        /* char * recipient: */
            writables_path,
        /* const uint32_t recipient_size: */
            256);
    
    T1_platform_open_folder_in_window_if_possible(writables_path);
    
    char dir_sep[4];
    T1_platform_get_directory_separator(dir_sep);
    
    char writables_filepath[256];
    T1_std_strcpy_cap(writables_filepath, 256, writables_path);
    T1_std_strcat_cap(writables_filepath, 256, dir_sep);
    T1_std_strcat_cap(writables_filepath, 256, "basemodel.obj");
    
    if (!T1_platform_file_exists(writables_filepath)) {
        T1_std_strcpy_cap(
            error_message,
            128,
            "Couldn't load basemodel.obj from "
            "writables folder. Type WRITABLES in "
            "terminal to open the folder.");
        return;
    }
    
    T1FileBuffer buffer;
    buffer.good = 0;
    buffer.size_without_terminator = T1_platform_get_filesize(writables_filepath);
    buffer.contents = T1_mem_malloc_from_managed(
        buffer.size_without_terminator+1);
    
    T1_platform_read_file(
        /* const char * filepath: */
            writables_filepath,
        /* FileBuffer *out_preallocatedbuffer: */
            &buffer);
    
    if (buffer.good) {
        T1_objmodel_new_mesh_id_from_obj_mtl_text(
            /* const char * original_obj_filename: */
                "basemodel.obj",
            /* const char * obj_text: */
                buffer.contents,
            /* const char * mtl_text: */
                NULL);
        
        T1_platform_gpu_copy_locked_vertices();
        T1_particle_effects[0].zpolygon_cpu.mesh_id = base_mesh_id;
    } else {
        T1_std_strcpy_cap(
            error_message,
            128,
            "Couldn't read basemodel.obj from "
            "writables folder. Type WRITABLES in "
            "terminal to open the folder.");
        return;
    }
    
    T1_mem_free_from_managed(buffer.contents);
    
    *success = 1;
    return;
}
#endif

typedef struct {
    char property_name[128];
    char property_type_name[128];
    size_t property_offset;
    int32_t slider_zsprite_id;
    int32_t label_zsprite_id;
    int32_t pin_zsprite_id;
} SliderForParticleProperty;

#define MAX_SLIDER_PARTICLE_PROPS 100


typedef struct ParticleDesignerState {
   SliderForParticleProperty regs[MAX_SLIDER_PARTICLE_PROPS];
   size_t inspecting_field_extra_offset;
   uint32_t regs_head_i;
   uint32_t regs_size;
   int32_t title_zsprite_id;
   int32_t title_label_zsprite_id;
   float slider_width;
   float slider_height;
   float menu_element_height;
   float whitespace_height;
   T1ParticleEffect * editing;
   T1ParticleEffect previous_values;
   char inspecting_field[128];
} ParticleDesignerState;

static ParticleDesignerState * pds = NULL;

void T1_clientlogic_init(void) {
    pds = T1_mem_malloc_from_unmanaged(sizeof(ParticleDesignerState));
    T1_std_memset(pds, 0, sizeof(ParticleDesignerState));
    
    pds->title_zsprite_id = T1_zspriteid_next_ui_element_id();
    pds->title_label_zsprite_id = T1_zspriteid_next_ui_element_id();
}

static float get_whitespace_height(void) {
    return T1_engine_globals->window_height / 200.0f;
}

static float get_menu_element_height(void) {
    return T1_engine_globals->window_height / 20.0f;
}

static float get_slider_height_screenspace(void) {
    return ((get_menu_element_height() - get_whitespace_height()) * 5) / 7;
}

static float get_slider_width_screenspace(void) {
    return get_slider_height_screenspace() * 20.0f;
}

static float get_slider_y_screenspace(int32_t i) {
    return T1_engine_globals->window_height - 130 -
            (pds->menu_element_height * i);
}

void T1_clientlogic_early_startup(
    bool32_t * success,
    char * error_message)
{
    *success = 0;
    
    mouse_scroll_pos = 0.0f;
    
    uint32_t ok = 0;
    T1_meta_struct(T1GPUConstMat, &ok);
    assert(ok);
    T1_meta_array(T1GPUConstMat, T1_TYPE_F32, ambient_rgb, 3, &ok);
    T1_meta_reg_float_limits_for_last_field(-1.0f, 1.0f, &ok);
    T1_meta_array(T1GPUConstMat, T1_TYPE_F32, diffuse_rgb, 3, &ok);
    T1_meta_reg_float_limits_for_last_field(-1.5f, 1.5f, &ok);
    T1_meta_array(T1GPUConstMat, T1_TYPE_F32, specular_rgb, 3, &ok);
    T1_meta_reg_float_limits_for_last_field(-1.0f, 1.0f, &ok);
    T1_meta_array(T1GPUConstMat, T1_TYPE_F32, rgb_cap, 3, &ok);
    T1_meta_reg_float_limits_for_last_field(-1.0f, 2.0f, &ok);
    T1_meta_field(T1GPUConstMat, T1_TYPE_F32, specular_exponent, &ok);
    T1_meta_reg_float_limits_for_last_field(-1.0f, 1.0f, &ok);
    T1_meta_field(T1GPUConstMat, T1_TYPE_F32, refraction, &ok);
    T1_meta_reg_float_limits_for_last_field(-1.0f, 1.0f, &ok);
    T1_meta_field(T1GPUConstMat, T1_TYPE_F32, alpha, &ok);
    T1_meta_reg_float_limits_for_last_field(-1.0f, 1.0f, &ok);
    T1_meta_field(T1GPUConstMat, T1_TYPE_F32, illum, &ok);
    T1_meta_reg_float_limits_for_last_field(-1.0f, 1.0f, &ok);
    assert(ok);
    
    T1_meta_struct(T1CPUzSpriteSimdStats, &ok);
    assert(ok);
    T1_meta_array(T1CPUzSpriteSimdStats, T1_TYPE_F32, xyz, 3, &ok);
    T1_meta_reg_float_limits_for_last_field(-2.0f, 2.0f, &ok);
    T1_meta_array(T1CPUzSpriteSimdStats, T1_TYPE_F32, offset_xyz, 3, &ok);
    T1_meta_array(T1CPUzSpriteSimdStats, T1_TYPE_F32, mul_xyz, 3, &ok);
    T1_meta_reg_float_limits_for_last_field(-0.5f, 0.5f, &ok);
    T1_meta_array(T1CPUzSpriteSimdStats, T1_TYPE_F32, angle_xyz, 3, &ok);
    T1_meta_reg_float_limits_for_last_field(-3.6f, 3.6f, &ok);
    T1_meta_field(T1CPUzSpriteSimdStats, T1_TYPE_F32, scale_factor, &ok);
    T1_meta_field(T1CPUzSpriteSimdStats, T1_TYPE_F32, ignore_camera, &ok);
    T1_meta_reg_float_limits_for_last_field(-1.0f, 1.0f, &ok);
    
    T1_meta_struct(T1CPUzSprite, &ok);
    assert(ok);
    T1_meta_struct_field(T1CPUzSprite, T1CPUzSpriteSimdStats, simd_stats, &ok);
    T1_meta_field(T1CPUzSprite, T1_TYPE_U8, alpha_blending_enabled, &ok);
    T1_meta_reg_uint_limits_for_last_field(0, 1, &ok);
    T1_meta_field(T1CPUzSprite, T1_TYPE_U8, visible, &ok);
    T1_meta_reg_uint_limits_for_last_field(0, 1, &ok);
    assert(ok);
    
    T1_meta_struct(T1GPUzSprite, &ok);
    assert(ok);
    T1_meta_struct_field(T1GPUzSprite, T1GPUConstMat, base_mat, &ok);
    T1_meta_array(T1GPUzSprite, T1_TYPE_F32, bonus_rgb, 3, &ok);
    T1_meta_reg_float_limits_for_last_field(0.0f, 2.0f, &ok);
    T1_meta_array(T1GPUzSprite, T1_TYPE_F32, base_mat_uv_offsets, 2, &ok);
    T1_meta_reg_float_limits_for_last_field(-1.0f, 1.0f, &ok);
    T1_meta_field(T1GPUzSprite, T1_TYPE_F32, alpha, &ok);
    T1_meta_reg_float_limits_for_last_field(-2.0f, 2.0f, &ok);
    T1_meta_field(T1GPUzSprite, T1_TYPE_F32, ignore_lighting, &ok);
    T1_meta_reg_float_limits_for_last_field(-1.0f, 1.0f, &ok);
    T1_meta_reg_float_limits_for_last_field(-1.0f, 1.0f, &ok);
    T1_meta_field(T1GPUzSprite, T1_TYPE_U32, remove_shadow, &ok);
    T1_meta_reg_uint_limits_for_last_field(0, 1, &ok);
    assert(ok);
    
    T1_meta_enum(T1EasingType, T1_TYPE_U8, &ok);
    assert(ok);
    T1_meta_enum_value(T1EasingType,
        EASINGTYPE_NONE, &ok);
    T1_meta_enum_value(T1EasingType, EASINGTYPE_ALWAYS_1, &ok);
    T1_meta_enum_value(T1EasingType,
        EASINGTYPE_EASEOUT_ELASTIC_ZERO_TO_ONE, &ok);
    T1_meta_enum_value(T1EasingType, EASINGTYPE_INOUT_SINE, &ok);
    T1_meta_enum_value(T1EasingType, EASINGTYPE_OUT_QUADRATIC, &ok);
    T1_meta_enum_value(T1EasingType,
        EASINGTYPE_SINGLE_BOUNCE_ZERO_TO_ZERO, &ok);
    T1_meta_enum_value(T1EasingType,
        EASINGTYPE_DOUBLE_BOUNCE_ZERO_TO_ZERO, &ok);
    T1_meta_enum_value(T1EasingType,
        EASINGTYPE_QUADRUPLE_BOUNCE_ZERO_TO_ZERO, &ok);
    T1_meta_enum_value(T1EasingType,
        EASINGTYPE_OCTUPLE_BOUNCE_ZERO_TO_ZERO, &ok);
    T1_meta_enum_value(T1EasingType,
        EASINGTYPE_SINGLE_PULSE_ZERO_TO_ZERO, &ok);
    T1_meta_enum_value(T1EasingType,
        EASINGTYPE_OCTUPLE_PULSE_ZERO_TO_ZERO, &ok);
    assert(ok);
    
    T1_meta_struct(T1GPUCircle, &ok);
    assert(ok);
    T1_meta_array(T1GPUCircle, T1_TYPE_F32, xyz, 3, &ok);
    T1_meta_reg_float_limits_for_last_field(-10.0f, 10.0f, &ok);
    T1_meta_field(T1GPUCircle, T1_TYPE_F32, size, &ok);
    T1_meta_reg_float_limits_for_last_field(-8.0f, 32.0f, &ok);
    T1_meta_array(T1GPUCircle, T1_TYPE_F32, rgba, 4, &ok);
    T1_meta_reg_float_limits_for_last_field(-1.0f, 2.5f, &ok);
    assert(ok);
    
    T1_meta_struct(T1ParticleMod, &ok);
    assert(ok);
    T1_meta_struct_field(T1ParticleMod, T1GPUCircle, gpu_stats, &ok);
    T1_meta_field(T1ParticleMod, T1_TYPE_U64, start_delay, &ok);
    T1_meta_reg_uint_limits_for_last_field(0, 30000000, &ok);
    T1_meta_field(T1ParticleMod, T1_TYPE_U64, duration, &ok);
    T1_meta_reg_uint_limits_for_last_field(0, 50000000, &ok);
    T1_meta_enum_field(T1ParticleMod, T1EasingType, T1_TYPE_U8, easing_type, &ok);
    T1_meta_reg_uint_limits_for_last_field(0, EASINGTYPE_OUTOFBOUNDS-1, &ok);
    T1_meta_field(T1ParticleMod, T1_TYPE_U8, rand_pct_add, &ok);
    T1_meta_reg_uint_limits_for_last_field(0, 100, &ok);
    T1_meta_field(T1ParticleMod, T1_TYPE_U8, rand_pct_sub, &ok);
    T1_meta_reg_uint_limits_for_last_field(0, 100, &ok);
    
    T1_meta_struct(T1ParticleEffect, &ok);
    assert(ok);
    T1_meta_struct_array(T1ParticleEffect, T1ParticleMod, mods, T1_PARTICLE_MODS_CAP, &ok);
    T1_meta_struct_field(T1ParticleEffect, T1GPUCircle, base, &ok);
    T1_meta_field(T1ParticleEffect, T1_TYPE_U64, spawn_lifespan, &ok);
    T1_meta_reg_uint_limits_for_last_field(0, 50000000, &ok);
    T1_meta_field(T1ParticleEffect, T1_TYPE_U64, loop_duration, &ok);
    T1_meta_reg_uint_limits_for_last_field(1000000, 50000000, &ok);
    T1_meta_field(T1ParticleEffect, T1_TYPE_U64, pause_per_spawn, &ok);
    T1_meta_reg_uint_limits_for_last_field(0, 500000, &ok);
    T1_meta_field(T1ParticleEffect,
        T1_TYPE_U32, spawns_per_loop, &ok);
    T1_meta_reg_uint_limits_for_last_field(1, 400000, &ok);
    T1_meta_field(T1ParticleEffect, T1_TYPE_U32, loops, &ok);
    T1_meta_reg_uint_limits_for_last_field(0, 20, &ok);
    T1_meta_field(T1ParticleEffect, T1_TYPE_F32, light_reach, &ok);
    T1_meta_reg_float_limits_for_last_field(0.1f, 10.0f, &ok);
    T1_meta_field(T1ParticleEffect, T1_TYPE_F32, light_strength, &ok);
    T1_meta_reg_float_limits_for_last_field(0.1f, 4.0f, &ok);
    T1_meta_array(T1ParticleEffect, T1_TYPE_F32, light_rgb, 3, &ok);
    T1_meta_reg_float_limits_for_last_field(0.0f, 1.0f, &ok);
    T1_meta_field(T1ParticleEffect, T1_TYPE_U8, modifiers_size, &ok);
    T1_meta_reg_uint_limits_for_last_field(
        1, T1_PARTICLE_MODS_CAP, &ok);
    T1_meta_field(T1ParticleEffect, T1_TYPE_U8, cast_light, &ok);
    T1_meta_reg_uint_limits_for_last_field(0, 1, &ok);
    assert(ok);
    
    example_particles_id = T1_zspriteid_next_nonui_id();
    
    //    load_obj_basemodel(error_message, success);
    //    if (!*success) {
    //        return;
    //    }
    //    *success = 0;
    
    T1_engine_globals->draw_axes = true;
    
    pds->whitespace_height   = get_whitespace_height();
    pds->menu_element_height = get_menu_element_height();
    pds->slider_height       = get_slider_height_screenspace();
    pds->slider_width        = get_slider_width_screenspace();
    
    *success = 1;
}

static void destroy_all_sliders(void) {
    T1_zsprite_delete(pds->title_zsprite_id);
    T1_zsprite_delete(pds->title_label_zsprite_id);
    
    log_assert(pds->regs_size < MAX_SLIDER_PARTICLE_PROPS);
    for (uint32_t i = pds->regs_head_i; i < pds->regs_size; i++) {
        T1_zsprite_delete(pds->regs[i].label_zsprite_id);
        T1_zsprite_delete(pds->regs[i].pin_zsprite_id);
        T1_zsprite_delete(pds->regs[i].slider_zsprite_id);
    }
}

static void redraw_all_sliders(void);

static void clicked_btn(int64_t arg) {
    if (arg == -1) {
        T1_zspriteid_clear_ui_element_touchable_ids();
        
        for (uint32_t i = 0; i < pds->regs_size; i++) {
            T1_uielement_delete(pds->regs[i].slider_zsprite_id);
            T1_uielement_delete(pds->regs[i].label_zsprite_id);
        }
        
        pds->inspecting_field_extra_offset = 0;
        T1_std_strcpy_cap(
            pds->inspecting_field,
            128,
            "T1ParticleEffect");
    } else {
        T1_std_strcpy_cap(
            pds->inspecting_field,
            128,
            pds->regs[pds->regs_head_i + arg].property_type_name);
        pds->inspecting_field_extra_offset +=
            pds->regs[pds->regs_head_i + arg].property_offset;
    }
    
    destroy_all_sliders();
    redraw_all_sliders();
}

static void set_next_ui_element_state_for_sliders(void) {
    next_ui_element_settings->perm.ignore_camera = true;
    next_ui_element_settings->perm.ignore_lighting = true;
    next_ui_element_settings->perm.pin_width_screenspace = 20;
    next_ui_element_settings->perm.pin_height_screenspace =
        pds->slider_height;
    next_ui_element_settings->perm.slider_width_screenspace =
        pds->slider_width;
    next_ui_element_settings->perm.slider_height_screenspace =
        pds->slider_height;
    next_ui_element_settings->perm.button_width_screenspace =
        pds->slider_width;
    next_ui_element_settings->perm.button_height_screenspace =
        pds->slider_height;
    next_ui_element_settings->slider_pin_rgba[0] = 0.7f;
    next_ui_element_settings->slider_pin_rgba[1] = 0.5f;
    next_ui_element_settings->slider_pin_rgba[2] = 0.5f;
    next_ui_element_settings->slider_pin_rgba[3] = 1.0f;
    T1_material_construct(&next_ui_element_settings->perm.back_mat);
    next_ui_element_settings->perm.back_mat.diffuse_rgb[0] = 0.2f;
    next_ui_element_settings->perm.back_mat.diffuse_rgb[1] = 0.2f;
    next_ui_element_settings->perm.back_mat.diffuse_rgb[2] = 0.3f;
    next_ui_element_settings->perm.back_mat.alpha          = 1.0f;
    next_ui_element_settings->perm.z = 0.75f;
    next_ui_element_settings->slider_label_shows_value = true;
    
}

static void redraw_all_sliders(void) {
    set_next_ui_element_state_for_sliders();
    
    pds->regs_size = 0;
    
    uint32_t num_properties = internal_T1_meta_get_num_of_fields_in_struct(
        pds->inspecting_field);
    
    float cur_x = T1_engine_globals->window_width -
        (pds->slider_width / 2) - 15.0f;
    float cur_y = get_slider_y_screenspace(-1);
    next_ui_element_settings->perm.screenspace_x = cur_x;
    next_ui_element_settings->perm.screenspace_y = cur_y;
    
    // draw the title of the object we're inspecting which also serves as
    // an "up" button
    next_ui_element_settings->slider_label = pds->inspecting_field;
    next_ui_element_settings->perm.back_mat.diffuse_rgb[2] = 0.6f;
    next_ui_element_settings->perm.ignore_camera = true;
    T1_uielement_request_button(
        pds->title_zsprite_id,
        pds->title_label_zsprite_id,
        clicked_btn,
        -1);
    next_ui_element_settings->perm.back_mat.diffuse_rgb[2] = 0.3f;
    
    for (int32_t i = 0; i < (int32_t)num_properties; i++) {
        
        T1MetaField field = T1_meta_get_field_at_index(
            pds->inspecting_field,
            (uint32_t)i);
        
        next_ui_element_settings->perm.is_meta_enum = field.is_enum;
        if (next_ui_element_settings->perm.is_meta_enum) {
            next_ui_element_settings->perm.meta_struct_name =
                field.enum_type_name;
            log_assert(
                next_ui_element_settings->perm.
                    meta_struct_name != NULL);
            log_assert(
                next_ui_element_settings->perm.
                    meta_struct_name[0] != '\0');
        }
        
        for (
            uint32_t array_i = 0;
            array_i < field.array_sizes[0];
            array_i++)
        {
            if (field.data_type == T1_TYPE_STRUCT) {
                log_assert(field.struct_type_name != NULL);
            }
            
            cur_y = get_slider_y_screenspace(i);
            
            next_ui_element_settings->perm.screenspace_y = cur_y;
            char expanded_field_name[256];
            T1_std_strcpy_cap(expanded_field_name, 256, field.name);
            if (field.array_sizes[0] > 1) {
                T1_std_strcat_cap(expanded_field_name, 256, "[");
                T1_std_strcat_uint_cap(expanded_field_name, 256, array_i);
                T1_std_strcat_cap(expanded_field_name, 256, "]");
            }
            next_ui_element_settings->slider_label = expanded_field_name;
            
            uint32_t good = 0;
            T1MetaField indexed_field = T1_meta_get_field_from_strings(
                pds->inspecting_field,
                expanded_field_name,
                &good);
            log_assert(good);
            log_assert(indexed_field.data_type != T1_TYPE_NOTSET);
            
            if (array_i > 0) {
                log_assert(indexed_field.offset != field.offset);
            }
            
            T1_std_strcpy_cap(
                pds->regs[pds->regs_size].property_name,
                128,
                field.name);
            if (field.data_type == T1_TYPE_STRUCT) {
                T1_std_strcpy_cap(
                    pds->regs[pds->regs_size].property_type_name,
                    128,
                    field.struct_type_name);
            }
            
            pds->regs[pds->regs_size].property_offset =
                (size_t)indexed_field.offset;
            if (pds->regs[pds->regs_size].slider_zsprite_id == 0) {
                pds->regs[pds->regs_size].slider_zsprite_id =
                    T1_zspriteid_next_ui_element_id();
                pds->regs[pds->regs_size].label_zsprite_id =
                    T1_zspriteid_next_ui_element_id();
                pds->regs[pds->regs_size].pin_zsprite_id =
                    T1_zspriteid_next_ui_element_id();
            }
            
            switch (field.data_type) {
                case T1_TYPE_U64:
                case T1_TYPE_U32:
                case T1_TYPE_U16:
                case T1_TYPE_U8:
                    next_ui_element_settings->perm.custom_min_max_vals = true;
                    next_ui_element_settings->perm.custom_uint_max =
                        field.custom_uint_max;
                    next_ui_element_settings->perm.custom_uint_min =
                        field.custom_uint_min;
                break;
                case T1_TYPE_I64:
                case T1_TYPE_I32:
                case T1_TYPE_I16:
                case T1_TYPE_I8:
                    next_ui_element_settings->perm.custom_min_max_vals = true;
                    next_ui_element_settings->perm.custom_int_max =
                        field.custom_int_max;
                    next_ui_element_settings->perm.custom_int_min =
                        field.custom_int_min;
                break;
                case T1_TYPE_F32:
                    next_ui_element_settings->perm.custom_min_max_vals = true;
                    next_ui_element_settings->perm.custom_float_max =
                        field.custom_float_max;
                    next_ui_element_settings->perm.custom_float_min =
                        field.custom_float_min;
                break;
                default:
                    next_ui_element_settings->perm.custom_min_max_vals = false;
            }
            
            switch (field.data_type) {
                case T1_TYPE_STRUCT:
                    T1_uielement_request_button(
                        pds->regs[pds->regs_size].slider_zsprite_id,
                        pds->regs[pds->regs_size].label_zsprite_id,
                        clicked_btn,
                        (int64_t)pds->regs_size);
                break;
                case T1_TYPE_I64:
                case T1_TYPE_I32:
                case T1_TYPE_I16:
                case T1_TYPE_I8:
                case T1_TYPE_U64:
                case T1_TYPE_U32:
                case T1_TYPE_U16:
                case T1_TYPE_U8:
                case T1_TYPE_F32:
                    next_ui_element_settings->perm.linked_type =
                        field.data_type;
                    
                    T1_uielement_request_slider(
                        pds->regs[pds->regs_size].slider_zsprite_id,
                        pds->regs[pds->regs_size].pin_zsprite_id,
                        pds->regs[pds->regs_size].label_zsprite_id,
                        ((char *)pds->editing +
                            pds->inspecting_field_extra_offset +
                            indexed_field.offset));
                break;
                default:
                    font_settings->font_height = 20;
                    font_settings->ignore_camera = true;
                    font_settings->ignore_lighting = true;
                    text_request_label_around(
                        /* const int32_t with_object_id: */
                            pds->regs[pds->regs_size].label_zsprite_id,
                        /* const char * text_to_draw: */
                            "implement me!",
                        /* const float mid_x_pixelspace: */
                            cur_x,
                        /* const float mid_y_pixelspace: */
                            cur_y,
                        /* const float z: */
                            0.75f,
                        /* const float max_width: */
                            T1_engine_globals->window_width * 2);
            }
            
            pds->regs_size += 1;
        }
    }
}

static void request_gfx_from_empty_scene(void) {
    camera.xyz[0] =  0.0f;
    camera.xyz[1] = -0.5f;
    camera.xyz[2] =  0.0f;
    
    camera.xyz_angle[0] =  -0.35f;
    camera.xyz_angle[1] =   0.00f;
    camera.xyz_angle[2] =   0.00f;
    
    zLightSource * light = next_zlight();
    light->RGBA[0]       =  0.50f;
    light->RGBA[1]       =  0.15f;
    light->RGBA[2]       =  0.15f;
    light->RGBA[3]       =  1.00f;
    light->diffuse       =  1.00f;
    light->reach         =  5.00f;
    light->xyz[0]        = -2.00f;
    light->xyz[1]        =  0.50f;
    light->xyz[2]        =  0.75f;
    commit_zlight(light);
    
    pds->regs_head_i = 0;
    pds->regs_size = 0;
    T1_std_strcpy_cap(pds->inspecting_field, 128, "T1ParticleEffect");
    pds->inspecting_field_extra_offset = 0;
    
    redraw_all_sliders();
}

static float scroll_y_offset = 0;
static int32_t slider_labels_object_id = -1;
void T1_clientlogic_late_startup(void) {
    
    pds->editing = T1_particle_get_next();
    pds->editing->base.xyz[2] = 1.0f;
    pds->editing->base.size = 8.0f;
    pds->editing->base.rgba[2] = 1.0f;
    pds->editing->base.rgba[3] = 0.25f;
    pds->editing->spawn_lifespan = 1000000;
    pds->editing->loop_duration  = 1500000;
    pds->editing->spawns_per_loop = 3;
    pds->editing->pause_per_spawn = 100;
    pds->editing->spawns_per_loop = 1000;
    
    pds->editing->mods[0].gpu_stats.
        xyz[1] = 0.5f;
    pds->editing->mods[0].duration = 500000;
    
    pds->editing->mods[1].gpu_stats.
        xyz[0] = 0.5f;
    pds->editing->mods[1].duration = 500000;
    pds->editing->mods[1].rand_pct_add = 32;
    pds->editing->mods[1].rand_pct_sub = 32;
    
    pds->editing->modifiers_size = 2;
    
    T1_particle_commit(pds->editing);
    
    request_gfx_from_empty_scene();
}

void T1_clientlogic_threadmain(int32_t threadmain_id) {
    switch (threadmain_id) {
        default:
            log_append("unhandled threadmain_id: ");
            log_append_int(threadmain_id);
            log_append("\n");
    }
}

static void client_handle_keypresses(
    uint64_t microseconds_elapsed)
{
    float elapsed_mod = (float)((double)microseconds_elapsed / (double)16666);
    float cam_speed = 0.1f * elapsed_mod;
    float cam_rotation_speed = 0.05f * elapsed_mod;
    
    if (T1_keypress_map[TOK_KEY_OPENSQUAREBRACKET] == true)
    {
        scroll_y_offset -= 15.0f;
    }
    
    if (T1_keypress_map[TOK_KEY_CLOSESQUAREBRACKET] == true)
    {
        scroll_y_offset += 15.0f;
    }
    
    if (T1_keypress_map[TOK_KEY_LEFTARROW] == true)
    {
        camera.xyz[0] -= cam_speed;
    }
    
    if (T1_keypress_map[TOK_KEY_RIGHTARROW] == true)
    {
        camera.xyz[0] += cam_speed;
    }
    
    if (T1_keypress_map[TOK_KEY_DOWNARROW] == true)
    {
        camera.xyz[1] -= cam_speed;
    }
    
    if (T1_keypress_map[TOK_KEY_UPARROW] == true)
    {
        camera.xyz[1] += cam_speed;
    }
    
    if (T1_keypress_map[TOK_KEY_A] == true) {
        camera.xyz_angle[0] += cam_rotation_speed;
    }
    
    if (T1_keypress_map[TOK_KEY_Z] == true) {
        camera.xyz_angle[2] -= cam_rotation_speed;
    }
    
    if (T1_keypress_map[TOK_KEY_X] == true) {
        camera.xyz_angle[2] += cam_rotation_speed;
    }
    
    if (T1_keypress_map[TOK_KEY_Q] == true) {
        camera.xyz_angle[0] -= cam_rotation_speed;
    }
    
    if (T1_keypress_map[TOK_KEY_W] == true) {
        camera.xyz_angle[1] -= cam_rotation_speed;
    }
    
    if (T1_keypress_map[TOK_KEY_S] == true) {
        camera.xyz_angle[1] += cam_rotation_speed;
    }
    
    if (T1_keypress_map[TOK_KEY_BACKSLASH] == true) {
        // / key
        camera.xyz[2] -= 0.01f;
    }
    
    if (T1_keypress_map[TOK_KEY_UNDERSCORE] == true) {
        camera.xyz[2] += 0.01f;
    }
}

void T1_clientlogic_update(uint64_t microseconds_elapsed)
{
    client_handle_keypresses(microseconds_elapsed);
    
    #if T1_SCHEDULED_ANIMS_ACTIVE == T1_ACTIVE
    float new_x =
        T1_engine_globals->window_width - (pds->slider_width / 2) - 15.0f;
    float new_z = 0.75f;
    
    int32_t target_zsprite_ids[3];
    for (uint32_t i = pds->regs_head_i; i < pds->regs_size; i++) {
        target_zsprite_ids[0] = pds->regs[i].slider_zsprite_id;
        target_zsprite_ids[1] = pds->regs[i].pin_zsprite_id;
        target_zsprite_ids[2] = pds->regs[i].label_zsprite_id;
        if (target_zsprite_ids[0] == target_zsprite_ids[1]) {
            continue;
        }
        if (target_zsprite_ids[0] == target_zsprite_ids[2]) {
            continue;
        }
        if (target_zsprite_ids[1] == target_zsprite_ids[2]) {
            continue;
        }
                
        float new_y = get_slider_y_screenspace((int32_t)i) -
            (mouse_scroll_pos * 30.0f);
        for (uint32_t j = 0; j < 3; j++) {
            T1ScheduledAnimation * anim =
                T1_scheduled_animations_request_next(true);
            anim->affected_zsprite_id = target_zsprite_ids[j];
            anim->delete_other_anims_targeting_same_object_id_on_commit = true;
            anim->cpu_vals.xyz[0] =
                T1_engineglobals_screenspace_x_to_x(new_x, new_z);
            anim->cpu_vals.xyz[1] =
                T1_engineglobals_screenspace_y_to_y(new_y, new_z);
            anim->cpu_vals.xyz[2] = new_z;
            anim->duration_us = 60000;
            T1_scheduled_animations_commit(anim);
        }
    }
    
    target_zsprite_ids[0] = pds->title_zsprite_id;
    target_zsprite_ids[1] = pds->title_label_zsprite_id;
    float new_title_y = get_slider_y_screenspace(-1) -
        (mouse_scroll_pos * 30.0f);
    for (uint32_t j = 0; j < 2; j++) {
        T1ScheduledAnimation * anim =
            T1_scheduled_animations_request_next(true);
        anim->affected_zsprite_id = target_zsprite_ids[j];
        anim->delete_other_anims_targeting_same_object_id_on_commit = true;
        anim->cpu_vals.xyz[0] =
            T1_engineglobals_screenspace_x_to_x(new_x, new_z);
        anim->cpu_vals.xyz[1] =
            T1_engineglobals_screenspace_y_to_y(new_title_y, new_z);
        anim->cpu_vals.xyz[2] = new_z;
        anim->duration_us = 60000;
        T1_scheduled_animations_commit(anim);
    }
    #elif T1_SCHEDULED_ANIMS_ACTIVE == T1_INACTIVE
    #else
    #error
    #endif // T1_SCHEDULED_ANIMS_ACTIVE
}

void T1_clientlogic_update_after_render_pass(void) {
    
}

void T1_clientlogic_evaluate_terminal_command(
    char * command,
    char * response,
    const uint32_t response_cap)
{
    if (
        T1_std_are_equal_strings(command, "SAVE") ||
        T1_std_string_starts_with(
            command,
            "SAVE "))
    {
        char savefile_filename[128];
        T1_std_memset(savefile_filename, 0, 128);
        
        if (command[4] == '\0') {
            T1_std_memcpy(
                savefile_filename,
                "particles.t1p",
                T1_std_strlen("particles.t1p"));
        } else {
            size_t savefile_len =
                T1_std_strlen(command) - T1_std_strlen("SAVE ");
            T1_std_memcpy(
                savefile_filename,
                command + T1_std_strlen("SAVE "),
                savefile_len);
            T1_std_strtolower(savefile_filename);
        }
        
        T1_std_strcat_cap(
            response,
            response_cap,
            "Saving particle effect to file: ");
        T1_std_internal_strcat_cap(
            response,
            response_cap,
            savefile_filename);
        
        uint32_t savefile_cap = 1000000;
        char * savefile_bin = T1_mem_malloc_from_managed(savefile_cap);
        uint32_t savefile_size = 0;
        uint32_t savefile_good = 0;
        
        T1_meta_serialize_instance_to_buffer(
            /* const char * struct_name: */
                "T1ParticleEffect",
            /* void * to_serialize: */
                pds->editing,
            /* char * buffer: */
                savefile_bin,
            /* uint32_t * buffer_size: */
                &savefile_size,
            /* uint32_t buffer_cap: */
                savefile_cap,
            /* uint32_t * good: */
                &savefile_good);
        
        if (!savefile_good) {
            T1_std_strcat_cap(
                response,
                response_cap,
                "\nFailed to serialize...");
            return;
        }
        
        T1_platform_write_file_to_writables(
            savefile_filename,
            savefile_bin,
            savefile_size,
            &savefile_good);
        
        T1_mem_free_from_managed(savefile_bin);
        
        if (!savefile_good) {
            T1_std_strcat_cap(
                response,
                response_cap,
                "\nFailed to write to disk...");
            return;
        }
        
        return;
    }
    
    if (
        T1_std_are_equal_strings(command, "LOAD") ||
        T1_std_string_starts_with(
            command,
            "LOAD "))
    {
        char savefile_filename[128];
        T1_std_memset(savefile_filename, 0, 128);
        
        if (command[4] == '\0') {
            T1_std_memcpy(
                savefile_filename,
                "particles.t1p",
                T1_std_strlen("particles.t1p"));
        } else {
            size_t savefile_len =
                T1_std_strlen(command) - T1_std_strlen("LOAD ");
            T1_std_memcpy(
                savefile_filename,
                command + T1_std_strlen("LOAD "),
                savefile_len);
            T1_std_strtolower(savefile_filename);
        }
        
        uint32_t savefile_good = 0;
        size_t savefile_size = T1_platform_get_writable_size(savefile_filename);
        
        if (savefile_size < 1) {
            T1_std_strcat_cap(
                response,
                response_cap,
                "No such file: ");
            T1_std_strcat_cap(
                response,
                response_cap,
                savefile_filename);
            return;
        }
        
        char * savefile_bin = T1_mem_malloc_from_managed(savefile_size + 1);
        T1_std_memset(savefile_bin, 0, savefile_size + 1);
        
        T1_platform_read_file_from_writables(
                savefile_filename,
            /* char * recipient: */
                savefile_bin,
            /* const uint32_t recipient_size: */
                (uint32_t)savefile_size,
            /* uint32_t * good: */
                &savefile_good);
        
        if (savefile_size < 1) {
            T1_std_strcat_cap(
                response,
                response_cap,
                "File exists, but couldn't read it from writables...");
            T1_mem_free_from_managed(savefile_bin);
            return;
        }
        
        T1_meta_deserialize_instance_from_buffer(
            /* const char * struct_name: */
                "T1ParticleEffect",
            /* void * recipient: */
                pds->editing,
            /* char * buffer: */
                savefile_bin,
            /* const uint32_t buffer_size: */
                (uint32_t)savefile_size + 1,
            /* uint32_t * good: */
                &savefile_good);
        
        if (savefile_size < 1) {
            T1_std_strcat_cap(
                response,
                response_cap,
                "Read file, but couldn't deserialize it...");
            T1_mem_free_from_managed(savefile_bin);
            return;
        }
        
        T1_mem_free_from_managed(savefile_bin);
        
        T1_std_strcat_cap(
            response,
            response_cap,
            "Success...");
        return;
    }
    
    T1_std_strcpy_cap(
        response,
        response_cap,
        "Unrecognized command - see client_logic_evaluate_terminal_command() "
        "in clientlogic.c");
}

void T1_clientlogic_window_resize(
    const uint32_t new_height,
    const uint32_t new_width)
{
    mouse_scroll_pos = 0.0f;
    
    zlights_to_apply_size = 0;
    T1_uielement_delete_all();
    #if T1_SCHEDULED_ANIMS_ACTIVE == T1_ACTIVE
    T1_scheduled_animations_delete_all();
    #elif T1_SCHEDULED_ANIMS_ACTIVE == T1_INACTIVE
    #else
    #error
    #endif
    
    T1_zsprites_to_render->size = 0;
    T1_zspriteid_clear_ui_element_touchable_ids();
    
    pds->whitespace_height = get_whitespace_height();
    pds->menu_element_height = get_menu_element_height();
    pds->slider_height = get_slider_height_screenspace();
    pds->slider_width = get_slider_width_screenspace();
    
    request_gfx_from_empty_scene();
}

void T1_clientlogic_shutdown(void) {
    // Your application shutdown code goes here!
}
