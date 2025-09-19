#include "T1_clientlogic.h"

static int32_t base_mesh_id = 1;
static int32_t example_particles_id = -1;

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
    return ((get_menu_element_height() - get_whitespace_height()) * 5) / 6;
}

static float get_slider_width_screenspace(void) {
    return get_slider_height_screenspace() * 10.0f;
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
    T1_meta_reg_custom_float_limits_for_last_field(-1.0f, 1.0f, &ok);
    T1_meta_array(T1GPUConstMat, T1_TYPE_F32, diffuse_rgb, 3, &ok);
    T1_meta_reg_custom_float_limits_for_last_field(-1.0f, 1.0f, &ok);
    T1_meta_array(T1GPUConstMat, T1_TYPE_F32, specular_rgb, 3, &ok);
    T1_meta_reg_custom_float_limits_for_last_field(-1.0f, 1.0f, &ok);
    T1_meta_array(T1GPUConstMat, T1_TYPE_F32, rgb_cap, 3, &ok);
    T1_meta_reg_custom_float_limits_for_last_field(-1.0f, 2.0f, &ok);
    // T1_meta_field(T1GPUConstMat, T1_TYPE_I32, texturearray_i, &ok);
    // T1_meta_field(T1GPUConstMat, T1_TYPE_I32, texture_i, &ok);
    #if T1_NORMAL_MAPPING_ACTIVE == T1_ACTIVE
    T1_meta_field(T1GPUConstMat, T1_TYPE_I32, normalmap_texturearray_i, &ok);
    T1_meta_field(T1GPUConstMat, T1_TYPE_I32, normalmap_texture_i, &ok);
    #elif T1_NORMAL_MAPPING_ACTIVE == T1_INACTIVE
    #else
    #error
    #endif
    T1_meta_field(T1GPUConstMat, T1_TYPE_F32, specular_exponent, &ok);
    T1_meta_reg_custom_float_limits_for_last_field(-1.0f, 1.0f, &ok);
    T1_meta_field(T1GPUConstMat, T1_TYPE_F32, refraction, &ok);
    T1_meta_reg_custom_float_limits_for_last_field(-1.0f, 1.0f, &ok);
    T1_meta_field(T1GPUConstMat, T1_TYPE_F32, alpha, &ok);
    T1_meta_reg_custom_float_limits_for_last_field(-1.0f, 1.0f, &ok);
    T1_meta_field(T1GPUConstMat, T1_TYPE_F32, illum, &ok);
    T1_meta_reg_custom_float_limits_for_last_field(-1.0f, 1.0f, &ok);
    assert(ok);
    
    T1_meta_struct(T1CPUzSprite, &ok);
    assert(ok);
    T1_meta_field(T1CPUzSprite, T1_TYPE_U8, alpha_blending_enabled, &ok);
    T1_meta_reg_custom_uint_limits_for_last_field(0, 1, &ok);
    T1_meta_field(T1CPUzSprite, T1_TYPE_U8, visible, &ok);
    T1_meta_reg_custom_uint_limits_for_last_field(0, 1, &ok);
    assert(ok);
    
    T1_meta_struct(T1GPUzSprite, &ok);
    assert(ok);
    T1_meta_array(T1GPUzSprite, T1_TYPE_F32, xyz, 3, &ok);
    T1_meta_reg_custom_float_limits_for_last_field(-2.0f, 2.0f, &ok);
    T1_meta_array(T1GPUzSprite, T1_TYPE_F32, xyz_angle, 3, &ok);
    T1_meta_reg_custom_float_limits_for_last_field(-3.6f, 3.6f, &ok);
    T1_meta_array(T1GPUzSprite, T1_TYPE_F32, bonus_rgb, 3, &ok);
    T1_meta_reg_custom_float_limits_for_last_field(0.0f, 2.0f, &ok);
    T1_meta_array(T1GPUzSprite, T1_TYPE_F32, xyz_mult, 3, &ok);
    T1_meta_reg_custom_float_limits_for_last_field(0.25f, 20.0f, &ok);
    T1_meta_array(T1GPUzSprite, T1_TYPE_F32, xyz_offset, 3, &ok);
    T1_meta_reg_custom_float_limits_for_last_field(-2.0f, 2.0f, &ok);
    T1_meta_array(T1GPUzSprite, T1_TYPE_F32, base_mat_uv_offsets, 2, &ok);
    T1_meta_reg_custom_float_limits_for_last_field(-1.0f, 1.0f, &ok);
    T1_meta_field(T1GPUzSprite, T1_TYPE_F32, scale_factor, &ok);
    T1_meta_reg_custom_float_limits_for_last_field(-1.0f, 1.0f, &ok);
    T1_meta_field(T1GPUzSprite, T1_TYPE_F32, alpha, &ok);
    T1_meta_reg_custom_float_limits_for_last_field(-2.0f, 2.0f, &ok);
    T1_meta_field(T1GPUzSprite, T1_TYPE_F32, ignore_lighting, &ok);
    T1_meta_reg_custom_float_limits_for_last_field(-1.0f, 1.0f, &ok);
    T1_meta_field(T1GPUzSprite, T1_TYPE_F32, ignore_camera, &ok);
    T1_meta_reg_custom_float_limits_for_last_field(-1.0f, 1.0f, &ok);
    T1_meta_field(T1GPUzSprite, T1_TYPE_U32, remove_shadow, &ok);
    T1_meta_reg_custom_uint_limits_for_last_field(0, 1, &ok);
    T1_meta_struct_field(T1GPUzSprite, T1GPUConstMat, base_mat, &ok);
    assert(ok);
    
    T1_meta_struct(T1ParticleEffect, &ok);
    assert(ok);
    T1_meta_struct_array(T1ParticleEffect, T1GPUzSprite, init_rand_add, 2, &ok);
    T1_meta_struct_array(T1ParticleEffect, T1GPUzSprite, pertime_rand_add, 2, &ok);
    T1_meta_struct_field(T1ParticleEffect, T1GPUzSprite, pertime_add, &ok);
    T1_meta_struct_field(T1ParticleEffect, T1GPUzSprite, perexptime_add, &ok);
    T1_meta_struct_field(T1ParticleEffect, T1GPUzSprite, zpolygon_gpu, &ok);
    
    T1_meta_struct_field(T1ParticleEffect, CPUzSprite, zpolygon_cpu, &ok);
    T1_meta_field(T1ParticleEffect, T1_TYPE_U64, lifespan, &ok);
    T1_meta_reg_custom_uint_limits_for_last_field(0, 50000000, &ok);
    T1_meta_field(T1ParticleEffect, T1_TYPE_U64, pause_per_spawn, &ok);
    T1_meta_reg_custom_uint_limits_for_last_field(0, 100000, &ok);
    T1_meta_field(T1ParticleEffect,
        T1_TYPE_U32, spawns_per_sec, &ok);
    T1_meta_reg_custom_uint_limits_for_last_field(1, 10000, &ok);
    T1_meta_field(T1ParticleEffect, T1_TYPE_U32, loops, &ok);
    T1_meta_reg_custom_uint_limits_for_last_field(0, 20, &ok);
    T1_meta_field(T1ParticleEffect, T1_TYPE_F32, light_reach, &ok);
    T1_meta_reg_custom_float_limits_for_last_field(0.1f, 10.0f, &ok);
    T1_meta_field(T1ParticleEffect, T1_TYPE_F32, light_strength, &ok);
    T1_meta_reg_custom_float_limits_for_last_field(0.1f, 4.0f, &ok);
    T1_meta_array(T1ParticleEffect, T1_TYPE_F32, light_rgb, 3, &ok);
    T1_meta_reg_custom_float_limits_for_last_field(0.0f, 1.0f, &ok);
    T1_meta_field(T1ParticleEffect, T1_TYPE_U8, cast_light, &ok);
    T1_meta_reg_custom_uint_limits_for_last_field(0, 1, &ok);
    assert(ok);
    
    example_particles_id = T1_zspriteid_next_nonui_id();
    
    load_obj_basemodel(error_message, success);
    if (!*success) {
        return;
    }
    *success = 0;
    
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
    camera.xyz[1] =  0.0f;
    camera.xyz[2] = -0.5f;
    
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
    pds->editing->zpolygon_gpu.xyz_mult[0] = T1_zsprite_get_x_multiplier_for_width(&pds->editing->zpolygon_cpu, 0.05f);
    pds->editing->zpolygon_gpu.xyz_mult[1] = T1_zsprite_get_y_multiplier_for_height(&pds->editing->zpolygon_cpu, 0.05f);
    pds->editing->zpolygon_gpu.xyz_mult[2] = T1_zsprite_get_z_multiplier_for_depth(&pds->editing->zpolygon_cpu, 0.05f);
    pds->editing->zpolygon_cpu.mesh_id = BASIC_CUBE_MESH_ID;
    pds->editing->zpolygon_gpu.xyz[0] = 0.0f;
    pds->editing->zpolygon_gpu.xyz[1] = 0.0f;
    pds->editing->zpolygon_gpu.xyz[2] = 0.5f;
    pds->editing->zpolygon_gpu.alpha = 1.0f;
    pds->editing->zpolygon_gpu.base_mat.alpha = 1.0f;
    pds->editing->pertime_rand_add[0].xyz[0] = -0.25f;
    pds->editing->pertime_rand_add[1].xyz[0] =  0.25f;
    pds->editing->pertime_rand_add[0].xyz[2] = -0.25f;
    pds->editing->pertime_rand_add[1].xyz[2] =  0.25f;
    pds->editing->pertime_add.xyz[1] = 0.5f;
    pds->editing->lifespan = 1000000;
    pds->editing->spawns_per_sec = 500;
    
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
    
    if (T1_keypress_map[TOK_KEY_L] == true) {
        T1_keypress_map[TOK_KEY_L] = false;
        T1LineParticle * lines = T1_particle_lineparticle_get_next();
        T1zSpriteRequest lines_polygon;
        lines_polygon.cpu_data = &lines->zpolygon_cpu;
        lines_polygon.gpu_data = &lines->zpolygon_gpu;
        zsprite_construct_quad(
            /* const float left_x: */
                0.0f,
            /* const float bottom_y: */
                0.0f,
            /* const float z: */
                0.5f,
            /* const float width: */
                T1_engineglobals_screenspace_width_to_width(75.0f, 0.5f),
            /* const float height: */
                T1_engineglobals_screenspace_height_to_height(75.0f, 0.5f),
            /* PolygonRequest * stack_recipient: */
                &lines_polygon);
        lines_polygon.gpu_data->ignore_camera = false;
        lines_polygon.gpu_data->ignore_lighting = true;
        
        lines_polygon.cpu_data->committed = true;
        lines->waypoint_duration[0] = 1250000;
        lines->waypoint_x[0] = T1_engineglobals_screenspace_x_to_x(
            /* const float screenspace_x: */
                0,
            /* const float given_z: */
                0.5f);
        lines->waypoint_y[0] = T1_engineglobals_screenspace_y_to_y(
            /* const float screenspace_y: */
                0,
            /* const float given_z: */
                0.5f);
        lines->waypoint_z[0] = 0.5f;
        lines->waypoint_r[0] = 0.8f;
        lines->waypoint_g[0] = 0.1f;
        lines->waypoint_b[0] = 0.1f;
        lines->waypoint_a[0] = 1.0f;
        lines->waypoint_scalefactor[0] = 1.0f;
        lines->waypoint_duration[0] = 350000;
        
        lines->waypoint_x[1] = T1_engineglobals_screenspace_x_to_x(
            /* const float screenspace_x: */
                T1_engine_globals->window_width,
            /* const float given_z: */
                0.5f);
        lines->waypoint_y[1] = T1_engineglobals_screenspace_y_to_y(
            /* const float screenspace_y: */
                0,
            /* const float given_z: */
                0.5f);
        lines->waypoint_z[1] = 0.5f;
        
        lines->waypoint_r[1] = 0.4f;
        lines->waypoint_g[1] = 0.8f;
        lines->waypoint_b[1] = 0.2f;
        lines->waypoint_a[1] = 1.0f;
        lines->waypoint_scalefactor[1] = 0.85f;
        lines->waypoint_duration[1] = 350000;
                
        lines->trail_delay = 500000;
        lines->waypoints_size = 2;
        lines->particle_count = 50;
        lines->particle_zangle_variance_pct = 15;
        lines->particle_rgb_variance_pct = 15;
        lines->particle_scalefactor_variance_pct = 35;
        T1_particle_lineparticle_commit(lines);
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
            anim->gpu_polygon_vals.xyz[0] =
                T1_engineglobals_screenspace_x_to_x(new_x, new_z);
            anim->gpu_polygon_vals.xyz[1] =
                T1_engineglobals_screenspace_y_to_y(new_y, new_z);
            anim->gpu_polygon_vals.xyz[2] = new_z;
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
        anim->gpu_polygon_vals.xyz[0] =
            T1_engineglobals_screenspace_x_to_x(new_x, new_z);
        anim->gpu_polygon_vals.xyz[1] =
            T1_engineglobals_screenspace_y_to_y(new_title_y, new_z);
        anim->gpu_polygon_vals.xyz[2] = new_z;
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

#if T1_TEXTURES_ACTIVE == T1_ACTIVE
static void load_texture(const char * writables_filename) {
    uint32_t ok = 0;
    T1Tex tex = T1_texture_array_get_filename_location(writables_filename);
    
    if (tex.array_i >= 0 || tex.slice_i >= 0) {
        log_dump_and_crash("That texture was already registered!");
        return;
    }
    
    size_t len = T1_std_strlen(writables_filename);
    if (
        writables_filename[len-3] == 'p' &&
        writables_filename[len-2] == 'n' &&
        writables_filename[len-1] == 'g')
    {
        T1_texture_files_runtime_register_png_from_writables(
            /* const char * filepath: */
                writables_filename,
            /* uint32_t * good: */
                &ok);
        log_assert(ok);
        
        tex = T1_texture_array_get_filename_location(writables_filename);
        
        if (tex.array_i < 0 || tex.slice_i < 0) {
            log_assert(0);
            return;
        }
        
        log_assert(
            T1_texture_arrays[tex.array_i].images[tex.slice_i].
                image.rgba_values_freeable != NULL);
        log_assert(
            T1_texture_arrays[tex.array_i].images[tex.slice_i].
                image.rgba_values_page_aligned != NULL);
        
        pds->editing->zpolygon_gpu.base_mat.texturearray_i = tex.array_i;
        pds->editing->zpolygon_gpu.base_mat.texture_i = tex.slice_i;
        
        #if T1_SCHEDULED_ANIMS_ACTIVE == T1_ACTIVE
        float tempquad_z = 0.9f;
        T1zSpriteRequest temp_quad;
        T1_zsprite_request_next(&temp_quad);
        zsprite_construct_quad(
            T1_engineglobals_screenspace_x_to_x(25.0f, tempquad_z),
            T1_engineglobals_screenspace_y_to_y(25.0f, tempquad_z),
            tempquad_z,
            T1_engineglobals_screenspace_width_to_width(100.0f, tempquad_z),
            T1_engineglobals_screenspace_height_to_height(100.0f, tempquad_z),
            &temp_quad);
        temp_quad.gpu_data->base_mat.diffuse_rgb[0] = 1.0f;
        temp_quad.gpu_data->base_mat.diffuse_rgb[1] = 1.0f;
        temp_quad.gpu_data->base_mat.diffuse_rgb[2] = 1.0f;
        temp_quad.gpu_data->ignore_camera = true;
        temp_quad.gpu_data->ignore_lighting = true;
        temp_quad.cpu_data->alpha_blending_enabled = true;
        temp_quad.cpu_data->zsprite_id = T1_zspriteid_next_nonui_id();
        temp_quad.gpu_data->base_mat.texturearray_i = tex.array_i;
        temp_quad.gpu_data->base_mat.texture_i = tex.slice_i;
        T1_zsprite_commit(&temp_quad);
        
        T1ScheduledAnimation * fade = T1_scheduled_animations_request_next(true);
        fade->pause_us = 3000000;
        fade->duration_us = 2000000;
        fade->gpu_polygon_vals.alpha = 0.0f;
        fade->affected_zsprite_id = temp_quad.cpu_data->zsprite_id;
        T1_scheduled_animations_commit(fade);
        #elif T1_SCHEDULED_ANIMS_ACTIVE == T1_INACTIVE
        #else
        #error
        #endif // T1_SCHEDULED_ANIMS_ACTIVE
        
    } else {
        return;
    }
}
#elif T1_TEXTURES_ACTIVE == T1_INACTIVE
#else
#error
#endif

void T1_clientlogic_evaluate_terminal_command(
    char * command,
    char * response,
    const uint32_t response_cap)
{
    #if T1_TEXTURES_ACTIVE == T1_ACTIVE
    if (
        T1_std_string_starts_with(
            command,
            "LOAD TEXTURE ") &&
        command[13] != ' ' &&
        command[13] != '\0')
    {
        char res_name[128];
        T1_std_strcpy_cap(res_name, 128, command + 13);
        
        T1_std_strtolower(res_name);
        
        char writables_path[256];
        writables_path[0] = '\0';
        
        T1_platform_get_writables_path(
            /* char * recipient: */
                writables_path,
            /* const uint32_t recipient_size: */
                256);
        
        char dir_sep[4];
        T1_platform_get_directory_separator(dir_sep);
        
        char writables_filepath[256];
        
        T1_std_strcpy_cap(writables_filepath, 256, writables_path);
        T1_std_strcat_cap(writables_filepath, 256, dir_sep);
        T1_std_strcat_cap(writables_filepath, 256, res_name);
        
        if (T1_platform_file_exists(writables_filepath)) {
            T1_std_strcat_cap(response, response_cap, "Found, loading...");
            
            T1Tex tex = T1_texture_array_get_filename_location(res_name);
            
            if (tex.array_i >= 0 || tex.slice_i >= 0) {
                T1_std_strcat_cap(
                    response,
                    response_cap,
                    "\nERROR: That texture already exists!");
                return;
            }
            
            load_texture(res_name);
            return;
        }
        
        T1_std_strcat_cap(
            response,
            response_cap,
            "No such texture in writables!");
        
        return;
    }
    #elif T1_TEXTURES_ACTIVE == T1_INACTIVE
    if (
        T1_std_string_starts_with(
            command,
            "LOAD TEXTURE ") &&
        command[13] != ' ' &&
        command[13] != '\0')
    {
        T1_std_strcat_cap(
            response,
            response_cap,
            "T1_TEXTURES_ACTIVE is set to 0, no support for textures!");
    }
    #else
    #error
    #endif
    
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
