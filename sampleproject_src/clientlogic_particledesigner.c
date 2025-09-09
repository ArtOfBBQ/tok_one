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
    
    platform_get_writables_path(
        /* char * recipient: */
            writables_path,
        /* const uint32_t recipient_size: */
            256);
    
    platform_open_folder_in_window_if_possible(writables_path);
    
    char dir_sep[4];
    platform_get_directory_separator(dir_sep);
    
    char writables_filepath[256];
    T1_std_strcpy_cap(writables_filepath, 256, writables_path);
    T1_std_strcat_cap(writables_filepath, 256, dir_sep);
    T1_std_strcat_cap(writables_filepath, 256, "basemodel.obj");
    
    if (!platform_file_exists(writables_filepath)) {
        T1_std_strcpy_cap(
            error_message,
            128,
            "Couldn't load basemodel.obj from "
            "writables folder. Type WRITABLES in "
            "terminal to open the folder.");
        return;
    }
    
    FileBuffer buffer;
    buffer.good = 0;
    buffer.size_without_terminator = platform_get_filesize(writables_filepath);
    buffer.contents = malloc_from_managed(
        buffer.size_without_terminator+1);
    
    platform_read_file(
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
        
        platform_gpu_copy_locked_vertices();
        particle_effects[0].zpolygon_cpu.mesh_id = base_mesh_id;
    } else {
        T1_std_strcpy_cap(
            error_message,
            128,
            "Couldn't read basemodel.obj from "
            "writables folder. Type WRITABLES in "
            "terminal to open the folder.");
        return;
    }
    
    free_from_managed(buffer.contents);
    
    *success = 1;
    return;
}

typedef struct {
    char property_name[128];
    size_t property_offset;
    int32_t slider_zsprite_id;
    int32_t label_zsprite_id;
    int32_t pin_zsprite_id;
} SliderForParticleProperty;

#define MAX_SLIDER_PARTICLE_PROPS 100


typedef struct ParticleDesignerState {
   SliderForParticleProperty regs[MAX_SLIDER_PARTICLE_PROPS];
   uint32_t regs_size;
   ParticleEffect * editing;
   ParticleEffect previous_values;
} ParticleDesignerState;

static ParticleDesignerState * pds = NULL;

void client_logic_init(void) {
    pds = malloc_from_unmanaged(sizeof(ParticleDesignerState));
    T1_std_memset(pds, 0, sizeof(ParticleDesignerState));
}

void client_logic_early_startup(
    bool32_t * success,
    char * error_message)
{
    *success = 0;
    
    mouse_scroll_pos = -20.0f;
    
    uint32_t ok = 0;
    T1_meta_struct(GPUConstMat, &ok);
    assert(ok);
    T1_meta_array(GPUConstMat, T1_TYPE_F32, ambient_rgb, 3, &ok);
    T1_meta_array(GPUConstMat, T1_TYPE_F32, diffuse_rgb, 3, &ok);
    T1_meta_array(GPUConstMat, T1_TYPE_F32, specular_rgb, 3, &ok);
    T1_meta_array(GPUConstMat, T1_TYPE_F32, rgb_cap, 3, &ok);
    T1_meta_field(GPUConstMat, T1_TYPE_I32, texturearray_i, &ok);
    T1_meta_field(GPUConstMat, T1_TYPE_I32, texture_i, &ok);
    #if NORMAL_MAPPING_ACTIVE
    T1_meta_field(GPUConstMat, T1_TYPE_I32, normalmap_texturearray_i, &ok);
    T1_meta_field(GPUConstMat, T1_TYPE_I32, normalmap_texture_i, &ok);
    #endif
    T1_meta_field(GPUConstMat, T1_TYPE_F32, specular_exponent, &ok);
    T1_meta_field(GPUConstMat, T1_TYPE_F32, refraction, &ok);
    T1_meta_field(GPUConstMat, T1_TYPE_F32, alpha, &ok);
    T1_meta_field(GPUConstMat, T1_TYPE_F32, illum, &ok);
    assert(ok);
    
    T1_meta_struct(GPUzSprite, &ok);
    assert(ok);
    T1_meta_array(GPUzSprite, T1_TYPE_F32, xyz, 3, &ok);
    T1_meta_array(GPUzSprite, T1_TYPE_F32, xyz_angle, 3, &ok);
    T1_meta_array(GPUzSprite, T1_TYPE_F32, bonus_rgb, 3, &ok);
    T1_meta_array(GPUzSprite, T1_TYPE_F32, xyz_mult, 3, &ok);
    T1_meta_array(GPUzSprite, T1_TYPE_F32, xyz_offset, 3, &ok);
    T1_meta_array(GPUzSprite, T1_TYPE_F32, base_mat_uv_offsets, 2, &ok);
    T1_meta_field(GPUzSprite, T1_TYPE_F32, scale_factor, &ok);
    T1_meta_field(GPUzSprite, T1_TYPE_F32, alpha, &ok);
    T1_meta_field(GPUzSprite, T1_TYPE_F32, ignore_lighting, &ok);
    T1_meta_field(GPUzSprite, T1_TYPE_F32, ignore_camera, &ok);
    T1_meta_field(GPUzSprite, T1_TYPE_U32, remove_shadow, &ok);
    T1_meta_field(GPUzSprite, T1_TYPE_I32, touchable_id, &ok);
    T1_meta_struct_field(GPUzSprite, GPULockedMat, base_mat, &ok);
    assert(ok);
    
    T1_meta_struct(ParticleEffect, &ok);
    assert(ok);
    T1_meta_struct_array(ParticleEffect, GPUzSprite, init_rand_add, 2, &ok);
    T1_meta_struct_array(ParticleEffect, GPUzSprite, pertime_rand_add, 2, &ok);
    T1_meta_struct_field(ParticleEffect, GPUzSprite, pertime_add, &ok);
    T1_meta_struct_field(ParticleEffect, GPUzSprite, perexptime_add, &ok);
    T1_meta_field(ParticleEffect, T1_TYPE_U64, lifespan, &ok);
    T1_meta_reg_custom_uint_limits_for_last_field(0, 50000000, &ok);
    T1_meta_field(ParticleEffect, T1_TYPE_U64, pause_per_set, &ok);
    T1_meta_reg_custom_uint_limits_for_last_field(0, 10000000, &ok);
    T1_meta_field(ParticleEffect,
        T1_TYPE_U32, spawns_per_sec, &ok);
    T1_meta_reg_custom_uint_limits_for_last_field(1, 10000, &ok);
    T1_meta_field(ParticleEffect, T1_TYPE_U32, loops, &ok);
    T1_meta_reg_custom_uint_limits_for_last_field(0, 20, &ok);
    T1_meta_field(ParticleEffect, T1_TYPE_F32, light_reach, &ok);
    T1_meta_reg_custom_float_limits_for_last_field(0.1f, 10.0f, &ok);
    T1_meta_field(ParticleEffect, T1_TYPE_F32, light_strength, &ok);
    T1_meta_reg_custom_float_limits_for_last_field(0.1f, 4.0f, &ok);
    T1_meta_array(ParticleEffect, T1_TYPE_F32, light_rgb, 3, &ok);
    T1_meta_reg_custom_float_limits_for_last_field(0.0f, 1.0f, &ok);
    T1_meta_field(ParticleEffect, T1_TYPE_U8, shattered, &ok);
    T1_meta_reg_custom_uint_limits_for_last_field(0, 1, &ok);
    T1_meta_field(ParticleEffect, T1_TYPE_U8, cast_light, &ok);
    T1_meta_reg_custom_uint_limits_for_last_field(0, 1, &ok);
    assert(ok);
    
    example_particles_id = next_nonui_object_id();
    
    load_obj_basemodel(error_message, success);
    if (!*success) {
        return;
    }
    *success = 0;
    
    engine_globals->draw_axes = true;
    
    *success = 1;
}

#define HEIGHT_PER_SLIDER 60.0f
static float get_slider_y_screenspace(uint32_t i) {
    return engine_globals->window_height + 100 -
            (HEIGHT_PER_SLIDER * i);
}

static void redraw_all_sliders(void) {
    float whitespace_height = 6.0f;
    
    next_ui_element_settings->perm.ignore_camera         = true;
    next_ui_element_settings->perm.ignore_lighting       = true;
    next_ui_element_settings->perm.pin_width_screenspace = 20;
    next_ui_element_settings->perm.pin_height_screenspace =
        ((HEIGHT_PER_SLIDER - whitespace_height) * 5) / 6;
    next_ui_element_settings->perm.slider_width_screenspace =
        next_ui_element_settings->perm.pin_height_screenspace * 10;
    next_ui_element_settings->perm.slider_height_screenspace = ((HEIGHT_PER_SLIDER - whitespace_height) * 5) / 6;
    next_ui_element_settings->slider_pin_rgba[0]        = 0.7f;
    next_ui_element_settings->slider_pin_rgba[1]        = 0.5f;
    next_ui_element_settings->slider_pin_rgba[2]        = 0.5f;
    next_ui_element_settings->slider_pin_rgba[3]        = 1.0f;
    next_ui_element_settings->slider_background_rgba[0] = 0.2f;
    next_ui_element_settings->slider_background_rgba[1] = 0.2f;
    next_ui_element_settings->slider_background_rgba[2] = 0.3f;
    next_ui_element_settings->slider_background_rgba[3] = 1.0f;
    
    uint32_t num_properties = T1_meta_get_num_of_fields_in_struct(
        ParticleEffect);
    
    for (uint32_t i = 0; i < num_properties; i++) {
        
        T1MetaField field = T1_meta_get_field_at_index(
            "ParticleEffect",
            i);
        
        float cur_x = engine_globals->window_width -
            (next_ui_element_settings->perm.slider_width_screenspace / 2) - 15.0f;
        
        float cur_y = get_slider_y_screenspace(i);
        float cur_z = 0.75f;
        
        next_ui_element_settings->perm.screenspace_x = cur_x;
        next_ui_element_settings->perm.screenspace_y = cur_y;
        next_ui_element_settings->perm.z = cur_z;
        
        pds->regs[i].slider_zsprite_id = next_ui_element_object_id();
        pds->regs[i].label_zsprite_id = next_ui_element_object_id();
        pds->regs[i].pin_zsprite_id = next_ui_element_object_id();
        
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
                text_request_label_around_x_at_top_y(
                    /* const int32_t with_object_id: */
                        pds->regs[i].slider_zsprite_id,
                    /* const char * text_to_draw: */
                        field.name,
                    /* const float mid_x_pixelspace: */
                        cur_x,
                    /* const float top_y_pixelspace: */
                        cur_y,
                    /* const float z: */
                        cur_z,
                    /* const float max_width: */
                        engine_globals->window_width * 2);
            break;
            case T1_TYPE_U64:
            case T1_TYPE_I64:
            case T1_TYPE_I32:
            case T1_TYPE_U32:
            case T1_TYPE_F32:
                next_ui_element_settings->perm.linked_type =
                    field.data_type;
                next_ui_element_settings->slider_label = field.name;
                next_ui_element_settings->slider_label_shows_value = true;
                
                T1_uielement_request_slider(
                    pds->regs[i].slider_zsprite_id,
                    pds->regs[i].pin_zsprite_id,
                    pds->regs[i].label_zsprite_id,
                    ((char *)pds->editing + field.offset));
            break;
            default:
                font_settings->font_height = 20;
                font_settings->ignore_camera = true;
                font_settings->ignore_lighting = true;
                text_request_label_around(
                    /* const int32_t with_object_id: */
                        pds->regs[i].label_zsprite_id,
                    /* const char * text_to_draw: */
                        "implement me!",
                    /* const float mid_x_pixelspace: */
                        cur_x,
                    /* const float mid_y_pixelspace: */
                        cur_y,
                    /* const float z: */
                        cur_z,
                    /* const float max_width: */
                        engine_globals->window_width * 2);
        }
        
        pds->regs_size += 1;
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
    
    pds->editing = &particle_effects[0];
    construct_particle_effect(pds->editing);
    particle_effects_size = 1;
    
    pds->editing->zpolygon_cpu.mesh_id = BASIC_CUBE_MESH_ID;
    pds->editing->zpolygon_gpu.xyz[0] = 0.0f;
    pds->editing->zpolygon_gpu.xyz[1] = 0.0f;
    pds->editing->zpolygon_gpu.xyz[2] = 1.0f;
    
    redraw_all_sliders();
}

static float scroll_y_offset = 0;
static int32_t slider_labels_object_id = -1;
void client_logic_late_startup(void) {
    
    request_gfx_from_empty_scene();
}

void client_logic_threadmain(int32_t threadmain_id) {
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
    
    if (keypress_map[TOK_KEY_OPENSQUAREBRACKET] == true)
    {
        scroll_y_offset -= 15.0f;
    }
    
    if (keypress_map[TOK_KEY_CLOSESQUAREBRACKET] == true)
    {
        scroll_y_offset += 15.0f;
    }
    
    if (keypress_map[TOK_KEY_LEFTARROW] == true)
    {
        camera.xyz[0] -= cam_speed;
    }
    
    if (keypress_map[TOK_KEY_RIGHTARROW] == true)
    {
        camera.xyz[0] += cam_speed;
    }
    
    if (keypress_map[TOK_KEY_DOWNARROW] == true)
    {
        camera.xyz[1] -= cam_speed;
    }
    
    if (keypress_map[TOK_KEY_UPARROW] == true)
    {
        camera.xyz[1] += cam_speed;
    }
    
    if (keypress_map[TOK_KEY_A] == true) {
        camera.xyz_angle[0] += cam_rotation_speed;
    }
    
    if (keypress_map[TOK_KEY_Z] == true) {
        camera.xyz_angle[2] -= cam_rotation_speed;
    }
    
    if (keypress_map[TOK_KEY_X] == true) {
        camera.xyz_angle[2] += cam_rotation_speed;
    }
    
    if (keypress_map[TOK_KEY_Q] == true) {
        camera.xyz_angle[0] -= cam_rotation_speed;
    }
    
    if (keypress_map[TOK_KEY_W] == true) {
        camera.xyz_angle[1] -= cam_rotation_speed;
    }
    
    if (keypress_map[TOK_KEY_S] == true) {
        camera.xyz_angle[1] += cam_rotation_speed;
    }
    
    if (keypress_map[TOK_KEY_L] == true) {
        keypress_map[TOK_KEY_L] = false;
        LineParticle * lines = next_lineparticle_effect();
        zSpriteRequest lines_polygon;
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
                engineglobals_screenspace_width_to_width(75.0f, 0.5f),
            /* const float height: */
                engineglobals_screenspace_height_to_height(75.0f, 0.5f),
            /* PolygonRequest * stack_recipient: */
                &lines_polygon);
        lines_polygon.gpu_data->ignore_camera = false;
        lines_polygon.gpu_data->ignore_lighting = true;
        
        lines_polygon.cpu_data->committed = true;
        lines->waypoint_duration[0] = 1250000;
        lines->waypoint_x[0] = engineglobals_screenspace_x_to_x(
            /* const float screenspace_x: */
                0,
            /* const float given_z: */
                0.5f);
        lines->waypoint_y[0] = engineglobals_screenspace_y_to_y(
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
        
        lines->waypoint_x[1] = engineglobals_screenspace_x_to_x(
            /* const float screenspace_x: */
                engine_globals->window_width,
            /* const float given_z: */
                0.5f);
        lines->waypoint_y[1] = engineglobals_screenspace_y_to_y(
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
        commit_lineparticle_effect(lines);
    }
    
    if (keypress_map[TOK_KEY_BACKSLASH] == true) {
        // / key
        camera.xyz[2] -= 0.01f;
    }
    
    if (keypress_map[TOK_KEY_UNDERSCORE] == true) {
        camera.xyz[2] += 0.01f;
    }
}

void client_logic_update(uint64_t microseconds_elapsed)
{
    client_handle_keypresses(microseconds_elapsed);
    
    #if 1
    for (uint32_t i = 0; i < pds->regs_size; i++) {
        int32_t target_zsprite_ids[3];
        target_zsprite_ids[0] = pds->regs[i].slider_zsprite_id;
        target_zsprite_ids[1] = pds->regs[i].pin_zsprite_id;
        target_zsprite_ids[2] = pds->regs[i].label_zsprite_id;
        if (target_zsprite_ids[0] == target_zsprite_ids[1]) {
            return;
        }
        if (target_zsprite_ids[0] == target_zsprite_ids[2]) {
            return;
        }
        if (target_zsprite_ids[1] == target_zsprite_ids[2]) {
            return;
        }
        
        float new_y = get_slider_y_screenspace(i) -
            (mouse_scroll_pos * 30.0f);
        
        for (uint32_t j = 0; j < 3; j++) {
            T1ScheduledAnimation * anim =
                T1_scheduled_animations_request_next(true);
            anim->affected_zsprite_id = target_zsprite_ids[j];
            anim->delete_other_anims_targeting_same_object_id_on_commit = true;
            anim->gpu_polygon_vals.xyz[1] =
                engineglobals_screenspace_y_to_y(new_y, 1.0f);
            anim->duration_us = 60000;
            T1_scheduled_animations_commit(anim);
        }
    }
    #endif
}

void client_logic_update_after_render_pass(void) {
    
}

void client_logic_evaluate_terminal_command(
    char * command,
    char * response,
    const uint32_t response_cap)
{
    if (
        T1_std_are_equal_strings(
            command,
            "EXAMPLE COMMAND"))
    {
        T1_std_strcpy_cap(
            response,
            response_cap,
            "Hello from clientlogic!");
        return;
    }
    
    T1_std_strcpy_cap(
        response,
        response_cap,
        "Unrecognized command - see client_logic_evaluate_terminal_command() "
        "in clientlogic.c");
}

void client_logic_window_resize(
    const uint32_t new_height,
    const uint32_t new_width)
{
    zlights_to_apply_size = 0;
    delete_all_ui_elements();
    T1_scheduled_animations_delete_all();
    zsprites_to_render->size = 0;
    particle_effects_size = 0;
    clear_ui_element_touchable_ids();
    
    request_gfx_from_empty_scene();
}

void client_logic_shutdown(void) {
    // Your application shutdown code goes here!
}
