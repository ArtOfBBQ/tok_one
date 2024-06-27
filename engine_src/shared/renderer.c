#include "renderer.h"

static uint32_t renderer_initialized = false;

void init_renderer(void) {
    renderer_initialized = true;
    
    camera.xyz[0]       = 0.0f;
    camera.xyz[1]       = 0.0f;
    camera.xyz[2]       = 0.0f;
    camera.xyz_angle[0] = 0.0f;
    camera.xyz_angle[1] = 0.0f;
    camera.xyz_angle[2] = 0.0f;
}

static bool32_t is_last_clicked = false;

inline static void add_point_vertex(
    GPUDataForSingleFrame * frame_data,
    float x,
    float y,
    float z,
    float ignore_camera)
{
    if (frame_data->polygon_collection->size >= MAX_POLYGONS_PER_BUFFER) {
        return;
    }
    
    frame_data->polygon_collection->polygons[
        frame_data->polygon_collection->size].xyz[0] = x;
    frame_data->polygon_collection->polygons[
        frame_data->polygon_collection->size].xyz[1] = y;
    frame_data->polygon_collection->polygons[
        frame_data->polygon_collection->size].xyz[2] = z;
    frame_data->polygon_collection->polygons[
        frame_data->polygon_collection->size].ignore_lighting = true;
    frame_data->polygon_collection->polygons[
        frame_data->polygon_collection->size].ignore_camera =
            ignore_camera;
    frame_data->polygon_collection->polygons[
        frame_data->polygon_collection->size].bonus_rgb[0] = 0.0f;
    frame_data->polygon_collection->polygons[
        frame_data->polygon_collection->size].bonus_rgb[1] = 0.0f;
    frame_data->polygon_collection->polygons[
        frame_data->polygon_collection->size].bonus_rgb[2] = 0.0f;
    
    frame_data->polygon_collection->polygons[
        frame_data->polygon_collection->size].scale_factor = 1.0f;
    
    frame_data->polygon_collection->polygons[
        frame_data->polygon_collection->size].xyz_offset[0] = 0.0f;
    frame_data->polygon_collection->polygons[
        frame_data->polygon_collection->size].xyz_offset[1] = 0.0f;
    frame_data->polygon_collection->polygons[
        frame_data->polygon_collection->size].xyz_offset[2] = 0.0f;
    
    frame_data->polygon_collection->polygons[
        frame_data->polygon_collection->size].xyz_angle[0] = 0.0f;
    frame_data->polygon_collection->polygons[
        frame_data->polygon_collection->size].xyz_angle[1] = 0.0f;
    frame_data->polygon_collection->polygons[
        frame_data->polygon_collection->size].xyz_angle[2] = 0.0f;
    
    frame_data->polygon_collection->polygons[
        frame_data->polygon_collection->size].xyz_multiplier[0] = 1.0f;
    frame_data->polygon_collection->polygons[
        frame_data->polygon_collection->size].xyz_multiplier[1] = 1.0f;
    frame_data->polygon_collection->polygons[
        frame_data->polygon_collection->size].xyz_multiplier[2] = 1.0f;
    
    frame_data->vertices[frame_data->vertices_size].polygon_i =
        (int)frame_data->polygon_collection->size;
    
    frame_data->polygon_materials[MAX_MATERIALS_PER_POLYGON *
        frame_data->polygon_collection->size].rgba[0] = 0.0f;
    frame_data->polygon_materials[MAX_MATERIALS_PER_POLYGON *
        frame_data->polygon_collection->size].rgba[1] = is_last_clicked ?
            ((platform_get_current_time_microsecs() / 25000) % 80) * 0.01f :
            1.0f;
    frame_data->polygon_materials[MAX_MATERIALS_PER_POLYGON *
        frame_data->polygon_collection->size].rgba[2] = 1.0f;
    frame_data->polygon_materials[MAX_MATERIALS_PER_POLYGON *
        frame_data->polygon_collection->size].rgba[3] = 1.0f;
    frame_data->polygon_materials[MAX_MATERIALS_PER_POLYGON *
        frame_data->polygon_collection->size].texturearray_i = -1;
    frame_data->polygon_materials[MAX_MATERIALS_PER_POLYGON *
        frame_data->polygon_collection->size].texture_i = -1;
    
    frame_data->vertices[frame_data->vertices_size].locked_vertex_i =
        all_mesh_summaries[2].vertices_head_i;
    
    frame_data->polygon_collection->size += 1;
    frame_data->vertices_size += 1;
}

inline static void draw_hitbox(
    GPUDataForSingleFrame * frame_data,
    const float x,
    const float y,
    const float z,
    const float width,
    const float height,
    const float depth,
    const float x_angle,
    const float y_angle,
    const float z_angle,
    const float ignore_camera)
{
    float left_x   = x - (width / 2);
    float right_x  = x + (width / 2);
    float top_y    = y + (height / 2);
    float bottom_y = y - (height / 2);
    float front_z  = z - (depth / 2);
    float back_z   = z + (depth / 2);
    
    zVertex topleftfront;
    topleftfront.x = left_x;
    topleftfront.y = top_y;
    topleftfront.z = front_z;
    x_rotate_zvertex(
        &topleftfront,
        x_angle);
    y_rotate_zvertex(
        &topleftfront,
        y_angle);
    z_rotate_zvertex(
        &topleftfront,
        z_angle);
    
    zVertex rightbottomback;
    rightbottomback.x = right_x;
    rightbottomback.y = bottom_y;
    rightbottomback.z = back_z;
    x_rotate_zvertex(
        &rightbottomback,
        x_angle);
    y_rotate_zvertex(
        &rightbottomback,
        y_angle);
    z_rotate_zvertex(
        &rightbottomback,
        z_angle);
    
    // left top front -> right top front
    add_point_vertex(
        frame_data,
        topleftfront.x, topleftfront.y, topleftfront.z,
        ignore_camera);
    add_point_vertex(
        frame_data,
        rightbottomback.x, topleftfront.y, topleftfront.z,
        ignore_camera);
    
    // left bottom front -> right bottom front
    add_point_vertex(
        frame_data,
        topleftfront.x, rightbottomback.y, topleftfront.z,
        ignore_camera);
    add_point_vertex(
        frame_data,
        rightbottomback.x, rightbottomback.y, topleftfront.z,
        ignore_camera);
    
    // left top back -> right top back
    add_point_vertex(
        frame_data,
        topleftfront.x, topleftfront.y, rightbottomback.z,
        ignore_camera);
    add_point_vertex(
        frame_data,
        rightbottomback.x, topleftfront.y, rightbottomback.z,
        ignore_camera);
    
    // left bottom back -> right bottom back
    add_point_vertex(
        frame_data,
        topleftfront.x, rightbottomback.y, rightbottomback.z,
        ignore_camera);
    add_point_vertex(
        frame_data,
        rightbottomback.x, rightbottomback.y, rightbottomback.z,
        ignore_camera);
    
    // left top front -> left top back
    add_point_vertex(
        frame_data,
        topleftfront.x, topleftfront.y, topleftfront.z,
        ignore_camera);
    add_point_vertex(
        frame_data,
        topleftfront.x, topleftfront.y, rightbottomback.z,
        ignore_camera);
    
    // right top front -> right top back
    add_point_vertex(
        frame_data,
        rightbottomback.x, topleftfront.y, topleftfront.z,
        ignore_camera);
    add_point_vertex(
        frame_data,
        rightbottomback.x, topleftfront.y, rightbottomback.z,
        ignore_camera);
    
    // left bottom front -> left bottom back
    add_point_vertex(
        frame_data,
        topleftfront.x, rightbottomback.y, topleftfront.z,
        ignore_camera);
    add_point_vertex(
        frame_data,
        topleftfront.x, rightbottomback.y, rightbottomback.z,
        ignore_camera);
    
    // right bottom front -> right bottom back
    add_point_vertex(
        frame_data,
        rightbottomback.x, rightbottomback.y, topleftfront.z,
        ignore_camera);
    add_point_vertex(
        frame_data,
        rightbottomback.x, rightbottomback.y, rightbottomback.z,
        ignore_camera);
    
    // left top front -> left bottom front
    add_point_vertex(
        frame_data,
        topleftfront.x, topleftfront.y, topleftfront.z,
        ignore_camera);
    add_point_vertex(
        frame_data,
        topleftfront.x, rightbottomback.y, topleftfront.z,
        ignore_camera);
    
    // right top front -> right bottom front
    add_point_vertex(
        frame_data,
        rightbottomback.x, topleftfront.y, topleftfront.z,
        ignore_camera);
    add_point_vertex(
        frame_data,
        rightbottomback.x, rightbottomback.y, topleftfront.z,
        ignore_camera);
    
    // left top back -> left bottom back
    add_point_vertex(
        frame_data,
        topleftfront.x, topleftfront.y, rightbottomback.z,
        ignore_camera);
    add_point_vertex(
        frame_data,
        topleftfront.x, rightbottomback.y, rightbottomback.z,
        ignore_camera);
    
    // right top back -> right bottom back
    add_point_vertex(
        frame_data,
        rightbottomback.x, topleftfront.y, rightbottomback.z,
        ignore_camera);
    add_point_vertex(
        frame_data,
        rightbottomback.x, rightbottomback.y, rightbottomback.z,
        ignore_camera);
}

inline static void zpolygon_hitboxes_to_lines(
    GPUDataForSingleFrame * frame_data)
{
    #ifndef LOGGER_IGNORE_ASSERTS
    if (
        window_globals->visual_debug_last_clicked_touchable_id < 0 &&
        !window_globals->visual_debug_mode)
    {
        return;
    }
    
    for (uint32_t zp_i = 0; zp_i < zpolygons_to_render->size; zp_i++) {
        if (zpolygons_to_render->cpu_data[zp_i].touchable_id >= 0) {
            is_last_clicked =
                window_globals->visual_debug_last_clicked_touchable_id ==
                    zpolygons_to_render->cpu_data[zp_i].touchable_id;
            if (window_globals->visual_debug_mode ||
                is_last_clicked)
            {
                draw_hitbox(
                    /* frame data: */
                        frame_data,
                    /* const float x: */
                        zpolygons_to_render->gpu_data[zp_i].xyz[0] +
                            zpolygons_to_render->gpu_data[zp_i].xyz_offset[0],
                    /* const float y: */
                        zpolygons_to_render->gpu_data[zp_i].xyz[1] +
                            zpolygons_to_render->gpu_data[zp_i].xyz_offset[1],
                    /* const float z: */
                        zpolygons_to_render->gpu_data[zp_i].xyz[2] +
                            zpolygons_to_render->gpu_data[zp_i].xyz_offset[2],
                    /* const float width: */
                        zpolygons_to_render->cpu_data[zp_i].hitbox_width,
                    /* const float height: */
                        zpolygons_to_render->cpu_data[zp_i].hitbox_height,
                    /* const float depth: */
                        zpolygons_to_render->cpu_data[zp_i].hitbox_depth,
                    /* const float x_angle: */
                        -zpolygons_to_render->gpu_data[zp_i].xyz_angle[0],
                    /* const float y_angle: */
                        -zpolygons_to_render->gpu_data[zp_i].xyz_angle[1],
                    /* const float z_angle: */
                        -zpolygons_to_render->gpu_data[zp_i].xyz_angle[2],
                    /* const float ignore_camera: */
                        zpolygons_to_render->gpu_data[zp_i].ignore_camera);
            }
        }
    }
    #else
    (void)frame_data;
    #endif
}

inline static void add_alphablending_zpolygons_to_workload(
    GPUDataForSingleFrame * frame_data)
{
    frame_data->first_alphablend_i = frame_data->vertices_size;
    
    // Copy all vertices that do use alpha blending
    for (
        int32_t cpu_zp_i = 0;
        cpu_zp_i < (int32_t)zpolygons_to_render->size;
        cpu_zp_i++)
    {
        if (
            zpolygons_to_render->cpu_data[cpu_zp_i].deleted ||
            !zpolygons_to_render->cpu_data[cpu_zp_i].visible ||
            !zpolygons_to_render->cpu_data[cpu_zp_i].committed ||
            !zpolygons_to_render->cpu_data[cpu_zp_i].alpha_blending_enabled)
        {
            continue;
        }
        
        int32_t mesh_id = zpolygons_to_render->cpu_data[cpu_zp_i].mesh_id;
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
            log_assert(frame_data->vertices_size < ALL_LOCKED_VERTICES_SIZE);
        }
    }
}

inline static void add_opaque_zpolygons_to_workload(
    GPUDataForSingleFrame * frame_data)
{
    assert(frame_data->polygon_collection->size == 0);
    assert(frame_data->vertices_size == 0);
    
    memcpy(
        /* void * dest: */
            frame_data->polygon_collection,
        /* const void * src: */
            zpolygons_to_render->gpu_data,
        /* size_t n: */
            sizeof(GPUPolygon) * zpolygons_to_render->size);
    frame_data->polygon_collection->size = zpolygons_to_render->size;
    
    log_assert(
        frame_data->polygon_collection->size <= zpolygons_to_render->size);
    log_assert(
        zpolygons_to_render->size < MAX_POLYGONS_PER_BUFFER);
    log_assert(
        frame_data->polygon_collection->size < MAX_POLYGONS_PER_BUFFER);
    
    memcpy(
        /* void *__dst: */
            frame_data->polygon_materials,
        /* const void *__src: */
            zpolygons_to_render->gpu_materials,
        /* size_t __n: */
            sizeof(GPUPolygonMaterial) *
                MAX_MATERIALS_PER_POLYGON *
                zpolygons_to_render->size);
    
    for (
        int32_t cpu_zp_i = 0;
        cpu_zp_i < (int32_t)zpolygons_to_render->size;
        cpu_zp_i++)
    {
        if (
            zpolygons_to_render->cpu_data[cpu_zp_i].deleted ||
            !zpolygons_to_render->cpu_data[cpu_zp_i].visible ||
            !zpolygons_to_render->cpu_data[cpu_zp_i].committed ||
            zpolygons_to_render->cpu_data[cpu_zp_i].alpha_blending_enabled)
        {
            continue;
        }
        
        int32_t mesh_id = zpolygons_to_render->cpu_data[cpu_zp_i].mesh_id;
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
            log_assert(frame_data->vertices_size < ALL_LOCKED_VERTICES_SIZE);
        }
    }
}

void hardware_render(
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
    
    log_assert(zpolygons_to_render->size < MAX_POLYGONS_PER_BUFFER);
    
    add_opaque_zpolygons_to_workload(frame_data);
    
    if (application_running) {
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
    }
    
    frame_data->first_alphablend_i = frame_data->vertices_size;
    
    add_alphablending_zpolygons_to_workload(frame_data);
    
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
    
    frame_data->first_line_i = frame_data->vertices_size;
    
    if (application_running) {
        zpolygon_hitboxes_to_lines(
            frame_data);
    }
    
    if (window_globals->wireframe_mode) {
        frame_data->first_alphablend_i = 0;
        frame_data->first_line_i = 0;
    }
    
    if (window_globals->debug_lights_mode) {
        for (
            uint32_t i = 0;
            i < frame_data->light_collection->lights_size;
            i++)
        {
            draw_hitbox(
                frame_data,
                frame_data->light_collection->light_x[i],
                frame_data->light_collection->light_y[i],
                frame_data->light_collection->light_z[i],
                0.1f,
                0.1f,
                0.1f,
                /* x_angle: */ 0.0f,
                /* y_angle: */ 0.0f,
                /* z_angle: */ 0.0f,
                /* ignore camera: */ 0.0f);
        }
    }
}
