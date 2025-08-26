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
    common_strcpy_capped(writables_filepath, 256, writables_path);
    common_strcat_capped(writables_filepath, 256, dir_sep);
    common_strcat_capped(writables_filepath, 256, "basemodel.obj");
    
    if (!platform_file_exists(writables_filepath)) {
        common_strcpy_capped(
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
        common_strcpy_capped(
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

typedef struct SliderParticleProperty {
    char property_name[128];
    size_t property_offset;
} SliderParticleProperty;

#define MAX_SLIDER_PARTICLE_PROPS 100


typedef struct ParticleDesignerState {
   SliderParticleProperty regs[MAX_SLIDER_PARTICLE_PROPS];
   uint32_t regs_size;
} ParticleDesignerState;

static ParticleDesignerState * pds = NULL;

void client_logic_init(void) {
    pds = malloc_from_unmanaged(sizeof(ParticleDesignerState));
    common_memset_char(pds, 0, sizeof(ParticleDesignerState));
}

void client_logic_early_startup(
    bool32_t * success,
    char * error_message)
{
    *success = 0;
    
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
    T1_meta_struct_field(GPUzSprite,
        T1_TYPE_STRUCT, GPULockedMat, base_mat, &ok);
    assert(ok);
    
    T1_meta_struct(ParticleEffect, &ok);
    assert(ok);
    T1_meta_struct_array(ParticleEffect,
        T1_TYPE_STRUCT, GPUzSprite, init_rand_add, 2, &ok);
    T1_meta_struct_array(ParticleEffect,
        T1_TYPE_STRUCT, GPUzSprite, pertime_rand_add, 2, &ok);
    T1_meta_struct_field(ParticleEffect,
        T1_TYPE_STRUCT, GPUzSprite, pertime_add, &ok);
    T1_meta_struct_field(ParticleEffect,
        T1_TYPE_STRUCT, GPUzSprite, perexptime_add, &ok);
    T1_meta_struct_field(ParticleEffect,
        T1_TYPE_STRUCT, GPUzSprite, zpolygon_cpu, &ok);
    T1_meta_struct_field(ParticleEffect,
        T1_TYPE_STRUCT, GPUzSprite, zpolygon_gpu, &ok);
    T1_meta_field(ParticleEffect, T1_TYPE_U64, random_seed, &ok);
    T1_meta_field(ParticleEffect, T1_TYPE_U64, elapsed, &ok);
    T1_meta_field(ParticleEffect, T1_TYPE_U64, lifespan, &ok);
    T1_meta_field(ParticleEffect, T1_TYPE_U64, pause_per_set, &ok);
    T1_meta_field(ParticleEffect, T1_TYPE_I32, zsprite_id, &ok);
    T1_meta_field(ParticleEffect, T1_TYPE_U32, deleted, &ok);
    T1_meta_field(ParticleEffect, T1_TYPE_U32, committed, &ok);
    T1_meta_field(ParticleEffect,
        T1_TYPE_U32, spawns_per_sec, &ok);
    T1_meta_field(ParticleEffect,
        T1_TYPE_U32, verts_per_particle, &ok);
    T1_meta_field(ParticleEffect, T1_TYPE_U32, loops, &ok);
    T1_meta_field(ParticleEffect, T1_TYPE_U32, shattered, &ok);
    T1_meta_field(ParticleEffect, T1_TYPE_U32, cast_light, &ok);
    T1_meta_field(ParticleEffect, T1_TYPE_F32, light_reach, &ok);
    T1_meta_field(ParticleEffect, T1_TYPE_F32, light_strength, &ok);
    T1_meta_array(ParticleEffect, T1_TYPE_F32, light_rgb, 3, &ok);
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
    
    next_ui_element_settings->ignore_camera = true;
    next_ui_element_settings->ignore_lighting = true;
    next_ui_element_settings->pin_width_screenspace = 20;
    next_ui_element_settings->pin_height_screenspace = 60;
    next_ui_element_settings->slider_width_screenspace = 250;
    next_ui_element_settings->slider_height_screenspace = 50;
    next_ui_element_settings->slider_pin_rgba[0] = 0.7f;
    next_ui_element_settings->slider_pin_rgba[1] = 0.5f;
    next_ui_element_settings->slider_pin_rgba[2] = 0.5f;
    next_ui_element_settings->slider_pin_rgba[3] = 1.0f;
    next_ui_element_settings->slider_background_rgba[0] = 0.2f;
    next_ui_element_settings->slider_background_rgba[1] = 0.2f;
    next_ui_element_settings->slider_background_rgba[2] = 0.3f;
    next_ui_element_settings->slider_background_rgba[3] = 1.0f;
    font_settings->font_height = 14;
    font_settings->mat.ambient_rgb[0] = 0.5f;
    font_settings->mat.ambient_rgb[1] = 1.0f;
    font_settings->mat.ambient_rgb[2] = 0.5f;
    font_settings->mat.alpha = 1.0f;
    
    request_float_slider(
        /* const int32_t background_object_id: */
            50,
        /* const int32_t pin_object_id: */
            51,
        /* const float x_screenspace: */
            engine_globals->window_width - next_ui_element_settings->slider_width_screenspace - 10,
        /* const float y_screenspace: */
            engine_globals->window_height -
                next_ui_element_settings->
                    slider_height_screenspace -
                10,
        /* const float z: */
            1.0f,
        /* const float min_value: */
            0.0f,
        /* const float max_value: */
            1.0f,
        /* float * linked_value: */
            &light->reach);
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

#if 0
static float get_slider_y_screenspace(uint32_t i) {
    return (engine_globals->window_height -
        ((float)i * 30.0f)) -
        ((float)(i / 13) * 30.0f) +
        scroll_y_offset;
}

static float get_title_y_screenspace(uint32_t i) {
    return get_slider_y_screenspace(i) + 30.0f;
}
#endif

void client_logic_update(uint64_t microseconds_elapsed)
{
    client_handle_keypresses(microseconds_elapsed);
}

void client_logic_update_after_render_pass(void) {
    
}

void client_logic_evaluate_terminal_command(
    char * command,
    char * response,
    const uint32_t response_cap)
{
    if (
        common_are_equal_strings(
            command,
            "EXAMPLE COMMAND"))
    {
        common_strcpy_capped(
            response,
            response_cap,
            "Hello from clientlogic!");
        return;
    }
    
    common_strcpy_capped(
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
    
    request_gfx_from_empty_scene();
}

void client_logic_shutdown(void) {
    // Your application shutdown code goes here!
}
