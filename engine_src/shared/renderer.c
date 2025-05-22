#include "renderer.h"

static uint32_t renderer_initialized = false;

void renderer_init(void) {
    renderer_initialized = true;
    
    common_memset_char(&camera, 0, sizeof(GPUCamera));
}

static bool32_t is_last_clicked = false;

#if RAW_SHADER_ACTIVE
static void add_line_vertex(
    GPUDataForSingleFrame * frame_data,
    const float xyz[3])
{
    log_assert(frame_data->line_vertices != NULL);
    
    if (frame_data->line_vertices_size >= MAX_LINE_VERTICES) {
        return;
    }
    
    common_memcpy(
        &frame_data->line_vertices[frame_data->line_vertices_size].xyz,
        xyz,
        sizeof(float) * 3);
    
    frame_data->line_vertices[frame_data->line_vertices_size].color =
        0.0f;
    
    frame_data->line_vertices_size += 1;
}

static void add_point_vertex(
    GPUDataForSingleFrame * frame_data,
    const float xyz[3],
    float color)
{
    log_assert(frame_data->point_vertices != NULL);
    
    if (frame_data->point_vertices_size >= MAX_POINT_VERTICES) {
        return;
    }
    
    common_memcpy(
        &frame_data->point_vertices[frame_data->point_vertices_size].xyz,
        xyz,
        sizeof(float) * 3);
    
    frame_data->point_vertices[frame_data->point_vertices_size].color = color;
    
    frame_data->point_vertices_size += 1;
}

inline static void draw_bounding_sphere(
    GPUDataForSingleFrame * frame_data,
    const float center_xyz[3],
    const float sphere_radius)
{
    float cur_point[3];
    for (float x_angle = 0.2f; x_angle < 6.28f; x_angle += 0.75f)
    {
        for (float y_angle = 0.0f; y_angle < 6.28f; y_angle += 0.75f)
        {
            cur_point[0] = 0.0f;
            cur_point[1] = 0.0f;
            cur_point[2] = sphere_radius;
            
            x_rotate_f3(cur_point, x_angle);
            y_rotate_f3(cur_point, y_angle);
            
            cur_point[0] += center_xyz[0];
            cur_point[1] += center_xyz[1];
            cur_point[2] += center_xyz[2];
            
            add_point_vertex(frame_data, cur_point, /* color: */ 0.0f);
        }
    }
}
#endif

inline static void add_alphablending_zpolygons_to_workload(
    GPUDataForSingleFrame * frame_data)
{
    frame_data->first_alphablend_i = frame_data->vertices_size;
    
    // Copy all vertices that do use alpha blending
    for (
        int32_t cpu_zp_i = 0;
        cpu_zp_i < (int32_t)zsprites_to_render->size;
        cpu_zp_i++)
    {
        if (
            zsprites_to_render->cpu_data[cpu_zp_i].deleted ||
            !zsprites_to_render->cpu_data[cpu_zp_i].visible ||
            !zsprites_to_render->cpu_data[cpu_zp_i].committed ||
            !zsprites_to_render->cpu_data[cpu_zp_i].alpha_blending_enabled)
        {
            continue;
        }
        
        int32_t mesh_id = zsprites_to_render->cpu_data[cpu_zp_i].mesh_id;
        log_assert(mesh_id >= 0);
        log_assert(mesh_id < (int32_t)all_mesh_summaries_size);
        
        int32_t vert_tail_i =
            all_mesh_summaries[mesh_id].vertices_head_i +
                all_mesh_summaries[mesh_id].vertices_size;
        assert(vert_tail_i < MAX_VERTICES_PER_BUFFER);
        
        for (
            int32_t vert_i = all_mesh_summaries[mesh_id].vertices_head_i;
            vert_i < vert_tail_i;
            vert_i += 1)
        {
            frame_data->vertices[frame_data->vertices_size].locked_vertex_i =
                vert_i;
            frame_data->vertices[frame_data->vertices_size].polygon_i =
                cpu_zp_i;
            frame_data->vertices_size += 1;
            log_assert(frame_data->vertices_size < MAX_VERTICES_PER_BUFFER);
        }
    }
}

inline static void add_opaque_zpolygons_to_workload(
    GPUDataForSingleFrame * frame_data)
{
    log_assert(frame_data->vertices_size == 0);
    
    int32_t cur_vals[4];
    int32_t incr_vals[4];
    incr_vals[0] = 2;
    incr_vals[1] = 0;
    incr_vals[2] = 2;
    incr_vals[3] = 0;
    SIMD_VEC4I incr = simd_load_vec4i(incr_vals);
    
    for (
        int32_t cpu_zp_i = 0;
        cpu_zp_i < (int32_t)zsprites_to_render->size;
        cpu_zp_i++)
    {
        if (
            zsprites_to_render->cpu_data[cpu_zp_i].deleted ||
            !zsprites_to_render->cpu_data[cpu_zp_i].visible ||
            !zsprites_to_render->cpu_data[cpu_zp_i].committed ||
            zsprites_to_render->cpu_data[cpu_zp_i].alpha_blending_enabled)
        {
            continue;
        }
        
        int32_t mesh_id = zsprites_to_render->cpu_data[cpu_zp_i].mesh_id;
        log_assert(mesh_id >= 0);
        log_assert(mesh_id < (int32_t)all_mesh_summaries_size);
        
        int32_t vert_tail_i =
            all_mesh_summaries[mesh_id].vertices_head_i +
                all_mesh_summaries[mesh_id].vertices_size;
        log_assert(vert_tail_i < MAX_VERTICES_PER_BUFFER);
        
        /*
        We are free to overflow the vertices buffer, since its end is not
        in use yet anyway.
        */
        int32_t vert_i = all_mesh_summaries[mesh_id].vertices_head_i;
        cur_vals[0] = vert_i-2;
        cur_vals[1] = cpu_zp_i;
        cur_vals[2] = vert_i-1;
        cur_vals[3] = cpu_zp_i;
        SIMD_VEC4I cur  = simd_load_vec4i(cur_vals);
        
        int32_t verts_to_copy = vert_tail_i - vert_i;
        #ifndef LOGGER_IGNORE_ASSERTS
        uint32_t previous_verts_size = frame_data->vertices_size;
        #endif
        
        for (int32_t i = 0; i < verts_to_copy; i += 2) {
            cur = simd_add_vec4i(cur, incr);
            simd_store_vec4i(
                (frame_data->vertices + frame_data->vertices_size),
                cur);
            frame_data->vertices_size += 2;
            
            #ifndef LOGGER_IGNORE_ASSERTS
            log_assert(frame_data->vertices_size < MAX_VERTICES_PER_BUFFER);
            log_assert(frame_data->vertices[frame_data->vertices_size-2].
                locked_vertex_i == (vert_i + i));
            log_assert(frame_data->vertices[frame_data->vertices_size-1].
                locked_vertex_i == (vert_i + i + 1));
            #endif
        }
        
        if (verts_to_copy % 2 == 1) {
            frame_data->vertices_size -= 1;
        }
        
        #ifndef LOGGER_IGNORE_ASSERTS
        log_assert(frame_data->vertices_size ==
            (previous_verts_size + (uint32_t)verts_to_copy));
        #endif
    }
}

static float clickray_elapsed = 0.0f;
void renderer_hardware_render(
    GPUDataForSingleFrame * frame_data,
    uint64_t elapsed_nanoseconds)
{
    (void)elapsed_nanoseconds;
    
    if (renderer_initialized != true) {
        log_append("renderer not initialized, aborting...\n");
        return;
    }
    
    if (
        frame_data == NULL ||
        frame_data->vertices == NULL)
    {
        log_append("ERROR: platform layer didnt pass recipients\n");
        return;
    }
    
    log_assert(zsprites_to_render->size < MAX_POLYGONS_PER_BUFFER);
    
    common_memcpy(
        /* void * dest: */
            frame_data->polygon_collection->polygons,
        /* const void * src: */
            zsprites_to_render->gpu_data,
        /* size_t n: */
            sizeof(GPUzSprite) * zsprites_to_render->size);
    frame_data->polygon_collection->size = zsprites_to_render->size;
    
    log_assert(
        frame_data->polygon_collection->size <= zsprites_to_render->size);
    log_assert(
        zsprites_to_render->size < MAX_POLYGONS_PER_BUFFER);
    log_assert(
        frame_data->polygon_collection->size < MAX_POLYGONS_PER_BUFFER);
    
    common_memcpy(
        /* void *__dst: */
            frame_data->polygon_materials,
        /* const void *__src: */
            zsprites_to_render->gpu_materials,
        /* size_t __n: */
            sizeof(GPUzSpriteMaterial) *
                MAX_MATERIALS_PER_POLYGON *
                zsprites_to_render->size);
    frame_data->polygon_collection->size = zsprites_to_render->size;
    
    *frame_data->postprocessing_constants =
        window_globals->postprocessing_constants;
    
    add_opaque_zpolygons_to_workload(frame_data);
    
    if (application_running) {
        #if PARTICLES_ACTIVE
        add_particle_effects_to_workload(
            /* GPUDataForSingleFrame * frame_data: */
                frame_data,
            /* uint64_t elapsed_nanoseconds: */
                elapsed_nanoseconds,
            /* const uint32_t alpha_blending: */
                false);
        
        add_lineparticle_effects_to_workload(
            frame_data,
            elapsed_nanoseconds,
            false);
        #endif
    }
    
    add_alphablending_zpolygons_to_workload(frame_data);
    
    #if PARTICLES_ACTIVE
    add_particle_effects_to_workload(
        /* GPUDataForSingleFrame * frame_data: */
            frame_data,
        /* uint64_t elapsed_nanoseconds: */
            elapsed_nanoseconds,
        /* const uint32_t alpha_blending: */
            true);
    
    add_lineparticle_effects_to_workload(
            frame_data,
            elapsed_nanoseconds,
            true);
    #endif
    
    #if RAW_SHADER_ACTIVE
    add_points_and_lines_to_workload(frame_data);
    
    if (application_running && window_globals->draw_axes) {
        // TODO: draw axes
        float axis_vertices[6];
        common_memset_float(axis_vertices, 0.0f, sizeof(float) * 6);
        
        #define DISTANT_FLOAT 3.5f
        
        add_point_vertex(
            /* GPUDataForSingleFrame * frame_data: */
                frame_data,
            /* const float xyz[3]: */
                axis_vertices,
            /* const float color: */
                3.0f);
        add_line_vertex(
            /* GPUDataForSingleFrame * frame_data: */
                frame_data,
            /* const float xyz[3]: */
                axis_vertices);
        axis_vertices[3] = DISTANT_FLOAT;
        add_line_vertex(
            /* GPUDataForSingleFrame * frame_data: */
                frame_data,
            /* const float xyz[3]: */
                axis_vertices + 3);
        
        add_line_vertex(
            /* GPUDataForSingleFrame * frame_data: */
                frame_data,
            /* const float xyz[3]: */
                axis_vertices);
        axis_vertices[3] =  0.0f;
        axis_vertices[4] =  DISTANT_FLOAT;
        add_line_vertex(
            /* GPUDataForSingleFrame * frame_data: */
                frame_data,
            /* const float xyz[3]: */
                axis_vertices + 3);
        
        add_line_vertex(
            /* GPUDataForSingleFrame * frame_data: */
                frame_data,
            /* const float xyz[3]: */
                axis_vertices);
        axis_vertices[4] =  0.0f;
        axis_vertices[5] =  DISTANT_FLOAT;
        add_line_vertex(
            /* GPUDataForSingleFrame * frame_data: */
                frame_data,
            /* const float xyz[3]: */
                axis_vertices + 3);
    }
    
    if (application_running && window_globals->draw_imputed_normals) {
        assert(0);
    }
    
    if (application_running && window_globals->draw_clickray) {
        add_line_vertex(
            /* GPUDataForSingleFrame * frame_data: */
                frame_data,
            /* const float xyz[3]: */
                window_globals->last_clickray_origin);
        
        float clickray_end[3];
        common_memcpy(
            clickray_end,
            window_globals->last_clickray_origin,
            sizeof(float) * 3);
        clickray_end[0] += window_globals->last_clickray_direction[0];
        clickray_end[1] += window_globals->last_clickray_direction[1];
        clickray_end[2] += window_globals->last_clickray_direction[2];
        add_line_vertex(
            /* GPUDataForSingleFrame * frame_data: */
                frame_data,
            /* const float xyz[3]: */
                clickray_end);
        
        if (window_globals->draw_clickray)
        {
            add_point_vertex(
                /* GPUDataForSingleFrame * frame_data: */
                    frame_data,
                /* const float * xyz: */
                    window_globals->last_clickray_collision, 0.33f);
        }
    }
    
    if (application_running && window_globals->draw_mouseptr) {
        float xyz[3];
        float z = 0.05f;
        xyz[0] = windowsize_screenspace_x_to_x(
            user_interactions[INTR_PREVIOUS_MOUSE_OR_TOUCH_MOVE].screen_x,
            z) + camera.xyz[0];
        xyz[1] = windowsize_screenspace_y_to_y(
            user_interactions[INTR_PREVIOUS_MOUSE_OR_TOUCH_MOVE].screen_y,
            z) + camera.xyz[1];
        xyz[2] = z + camera.xyz[2];
        add_point_vertex(
            /* GPUDataForSingleFrame * frame_data: */
                frame_data,
            /* const float * xyz: */
                xyz,
            /* const float color: */
                0.33f);
    }
    
    #endif
}
