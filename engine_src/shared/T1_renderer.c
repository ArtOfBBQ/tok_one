#include "T1_renderer.h"

static uint32_t renderer_initialized = false;

void renderer_init(void) {
    renderer_initialized = true;
    
    T1_std_memset(&camera, 0, sizeof(T1GPUCamera));
}

#if T1_RAW_SHADER_ACTIVE == T1_ACTIVE
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

#if 0
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

#elif T1_RAW_SHADER_ACTIVE == T1_INACTIVE
// Pass
#else
#error "T1_RAW_SHADER_ACTIVE undefined"
#endif


// TODO: transform triangles (rotate etc.), then compare
// This might not be nescessary at all for our purposes
#if 0
static int compare_triangles_furthest_camera_dist(
    const void * a,
    const void * b)
{
    GPUVertexIndices * tris[2];
    tris[0] = (GPUVertexIndices *)a;
    tris[1] = (GPUVertexIndices *)b;
    
    float dists[2];
    float avg_xyz[3];
    float cur_cam[3];
    
    dists[0] = 0.0f;
    dists[1] = 0.0f;
    
    for (uint32_t i = 0; i < 2; i++) {
        avg_xyz[0] = 0.0f;
        avg_xyz[1] = 0.0f;
        avg_xyz[2] = 0.0f;
        
        for (uint32_t vert_i = 0; vert_i < 3; vert_i++) {
            log_assert(tris[i][vert_i].locked_vertex_i < (int32_t)all_mesh_vertices->size);
            avg_xyz[0] += all_mesh_vertices->gpu_data[
                tris[i][vert_i].locked_vertex_i].xyz[0];
            avg_xyz[1] += all_mesh_vertices->gpu_data[
                tris[i][vert_i].locked_vertex_i].xyz[1];
            avg_xyz[2] += all_mesh_vertices->gpu_data[
                tris[i][vert_i].locked_vertex_i].xyz[2];
        }
        
        #if T1_LOGGER_ASSERTS_ACTIVE == T1_ACTIVE
        log_assert(tris[i][0].polygon_i < (int32_t)all_mesh_vertices->size);
        log_assert(tris[i][0].polygon_i == tris[i][1].polygon_i);
        log_assert(tris[i][0].polygon_i == tris[i][2].polygon_i);
        #elif T1_LOGGER_ASSERTS_ACTIVE == T1_INACTIVE
        #else
        #error "T1_LOGGER_ASSERTS_ACTIVE undefined"
        #endif
        
        avg_xyz[0] *=
            zsprites_to_render->gpu_data[tris[i][0].polygon_i].xyz_multiplier[0];
        avg_xyz[1] *=
            zsprites_to_render->gpu_data[tris[i][0].polygon_i].xyz_multiplier[1];
        avg_xyz[2] *=
            zsprites_to_render->gpu_data[tris[i][0].polygon_i].xyz_multiplier[2];
        avg_xyz[0] *= zsprites_to_render->gpu_data[tris[i][0].polygon_i].
            scale_factor;
        avg_xyz[1] *= zsprites_to_render->gpu_data[tris[i][0].polygon_i].
            scale_factor;
        avg_xyz[2] *= zsprites_to_render->gpu_data[tris[i][0].polygon_i].
            scale_factor;
        avg_xyz[0] /= 3.0f;
        avg_xyz[1] /= 3.0f;
        avg_xyz[2] /= 3.0f;
        
        avg_xyz[0] += zsprites_to_render->
            gpu_data[tris[i][0].polygon_i].xyz[0];
        avg_xyz[1] += zsprites_to_render->
            gpu_data[tris[i][0].polygon_i].xyz[1];
        avg_xyz[2] += zsprites_to_render->
            gpu_data[tris[i][0].polygon_i].xyz[2];
        
        avg_xyz[0] += zsprites_to_render->gpu_data[tris[i][0].polygon_i].xyz_offset[0];
        avg_xyz[1] += zsprites_to_render->gpu_data[tris[i][0].polygon_i].xyz_offset[1];
        avg_xyz[2] += zsprites_to_render->gpu_data[tris[i][0].polygon_i].xyz_offset[2];
        
        float ign_cam = zsprites_to_render->gpu_data[tris[i][0].polygon_i].
            ignore_camera;
        
        cur_cam[0] = camera.xyz[0] * (1.0f - ign_cam);
        cur_cam[1] = camera.xyz[1] * (1.0f - ign_cam);
        cur_cam[2] = camera.xyz[2] * (1.0f - ign_cam);
        
        dists[i] =
            ((cur_cam[0] - avg_xyz[0]) * (cur_cam[0] - avg_xyz[0])) +
            ((cur_cam[1] - avg_xyz[1]) * (cur_cam[1] - avg_xyz[1])) +
            ((cur_cam[2] - avg_xyz[2]) * (cur_cam[2] - avg_xyz[2]));
    }
    
    if (dists[0] < dists[1]) {
        return 1;
    } else if (dists[0] > dists[1]) {
        return -1;
    } else {
        return
            (int)(tris[0][0].polygon_i + tris[0][0].locked_vertex_i) -
            (int)(tris[1][0].polygon_i + tris[1][0].locked_vertex_i);
    }
}
#endif

inline static void add_alphablending_zpolygons_to_workload(
    T1GPUFrame * frame_data)
{
    frame_data->first_alphablend_i = frame_data->verts_size;
    
    // Copy all vertices that do use alpha blending
    for (
        int32_t cpu_zp_i = 0;
        cpu_zp_i < (int32_t)T1_zsprites_to_render->size;
        cpu_zp_i++)
    {
        if (
            T1_zsprites_to_render->cpu_data[cpu_zp_i].deleted ||
            !T1_zsprites_to_render->cpu_data[cpu_zp_i].visible ||
            !T1_zsprites_to_render->cpu_data[cpu_zp_i].committed ||
            !T1_zsprites_to_render->cpu_data[cpu_zp_i].alpha_blending_enabled)
        {
            continue;
        }
        
        int32_t mesh_id = T1_zsprites_to_render->cpu_data[cpu_zp_i].mesh_id;
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
            frame_data->verts[frame_data->verts_size].locked_vertex_i =
                vert_i;
            frame_data->verts[frame_data->verts_size].polygon_i =
                cpu_zp_i;
            frame_data->verts_size += 1;
            log_assert(frame_data->verts_size < MAX_VERTICES_PER_BUFFER);
        }
    }
    
    #if 0
    if (
        frame_data->vertices_size > frame_data->first_alphablend_i)
    {
        log_assert(
            (frame_data->vertices_size - frame_data->first_alphablend_i) % 3 == 0);
        log_assert(frame_data->first_alphablend_i % 3 == 0);
        
        #if T1_LOGGER_ASSERTS_ACTIVE == T1_ACTIVE
        uint32_t initial_first_alphablend_i = frame_data->first_alphablend_i;
        uint32_t initial_vertices_size = frame_data->vertices_size;
        
        for (
            int32_t i = (int)frame_data->first_alphablend_i;
            i < (int)frame_data->vertices_size;
            i += 3)
        {
            log_assert(
                frame_data->vertices[i+0].polygon_i ==
                frame_data->vertices[i+1].polygon_i);
            log_assert(
                frame_data->vertices[i+0].polygon_i ==
                frame_data->vertices[i+2].polygon_i);
            log_assert(
                frame_data->vertices[i+0].locked_vertex_i !=
                frame_data->vertices[i+1].locked_vertex_i);
            log_assert(
                frame_data->vertices[i+0].locked_vertex_i !=
                frame_data->vertices[i+2].locked_vertex_i);
            log_assert(
                frame_data->vertices[i+1].locked_vertex_i !=
                frame_data->vertices[i+2].locked_vertex_i);
        }
        #elif T1_LOGGER_ASSERTS_ACTIVE == T1_INACTIVE
        // Pass
        #else
        #error "T1_LOGGER_ASSERTS_ACTIVE undefined"
        #endif
        
        qsort(
            /* base: */
                frame_data->vertices + frame_data->first_alphablend_i,
            /* size_t nel: */
                (frame_data->vertices_size -
                    frame_data->first_alphablend_i) / 3,
            /* size_t width: */
                sizeof(GPUVertexIndices) * 3,
            /* int (* _Nonnull compar)(const void *, const void *): */
                compare_triangles_furthest_camera_dist);
        
        #if T1_LOGGER_ASSERTS_ACTIVE
        log_assert(
            frame_data->first_alphablend_i == initial_first_alphablend_i);
        log_assert(frame_data->vertices_size == initial_vertices_size);
        for (
            int32_t i = (int)frame_data->first_alphablend_i;
            i < (int)frame_data->vertices_size;
            i += 3)
        {
            log_assert(
                frame_data->vertices[i+0].polygon_i ==
                frame_data->vertices[i+1].polygon_i);
            log_assert(
                frame_data->vertices[i+0].polygon_i ==
                frame_data->vertices[i+2].polygon_i);
            log_assert(
                frame_data->vertices[i+0].locked_vertex_i !=
                frame_data->vertices[i+1].locked_vertex_i);
            log_assert(
                frame_data->vertices[i+0].locked_vertex_i !=
                frame_data->vertices[i+2].locked_vertex_i);
            log_assert(
                frame_data->vertices[i+1].locked_vertex_i !=
                frame_data->vertices[i+2].locked_vertex_i);
        }
        #endif
    }
    #endif
}

inline static void add_opaque_zpolygons_to_workload(
    T1GPUFrame * frame_data)
{
    log_assert(frame_data->verts_size == 0);
    
    int32_t cur_vals[4];
    int32_t incr_vals[4];
    incr_vals[0] = 2;
    incr_vals[1] = 0;
    incr_vals[2] = 2;
    incr_vals[3] = 0;
    SIMD_VEC4I incr = simd_load_vec4i(incr_vals);
    
    for (
        int32_t cpu_zp_i = 0;
        cpu_zp_i < (int32_t)T1_zsprites_to_render->size;
        cpu_zp_i++)
    {
        if (
            T1_zsprites_to_render->cpu_data[cpu_zp_i].deleted ||
            !T1_zsprites_to_render->cpu_data[cpu_zp_i].visible ||
            !T1_zsprites_to_render->cpu_data[cpu_zp_i].committed ||
            T1_zsprites_to_render->cpu_data[cpu_zp_i].alpha_blending_enabled)
        {
            continue;
        }
        
        int32_t mesh_id = T1_zsprites_to_render->cpu_data[cpu_zp_i].mesh_id;
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
        
        #if T1_LOGGER_ASSERTS_ACTIVE == T1_ACTIVE
        uint32_t previous_verts_size = frame_data->verts_size;
        #elif T1_LOGGER_ASSERTS_ACTIVE == T1_INACTIVE
        // Pass
        #else
        #error "T1_LOGGER_ASSERTS_ACTIVE undefined"
        #endif
        
        for (int32_t i = 0; i < verts_to_copy; i += 2) {
            cur = simd_add_vec4i(cur, incr);
            simd_store_vec4i(
                (frame_data->verts + frame_data->verts_size),
                cur);
            frame_data->verts_size += 2;
            
            #if T1_LOGGER_ASSERTS_ACTIVE == T1_ACTIVE
            log_assert(frame_data->verts_size < MAX_VERTICES_PER_BUFFER);
            log_assert(frame_data->verts[frame_data->verts_size-2].
                locked_vertex_i == (vert_i + i));
            log_assert(frame_data->verts[frame_data->verts_size-1].
                locked_vertex_i == (vert_i + i + 1));
            #elif T1_LOGGER_ASSERTS_ACTIVE == T1_INACTIVE
            // Pass
            #else
            #error "T1_LOGGER_ASSERTS_ACTIVE undefined"
            #endif
        }
        
        if (verts_to_copy % 2 == 1) {
            frame_data->verts_size -= 1;
        }
        
        #if T1_LOGGER_ASSERTS_ACTIVE == T1_ACTIVE
        log_assert(frame_data->verts_size ==
            (previous_verts_size + (uint32_t)verts_to_copy));
        #elif T1_LOGGER_ASSERTS_ACTIVE == T1_INACTIVE
        // Pass
        #else
        #error "T1_LOGGER_ASSERTS_ACTIVE undefined"
        #endif
    }
}

static void construct_transformation_matrices(void) {
    
    for (uint32_t i = 0; i < T1_zsprites_to_render->size; i++) {
        float cx = cosf(T1_zsprites_to_render->cpu_data[i].angle_xyz[0]);
        float sx = sinf(T1_zsprites_to_render->cpu_data[i].angle_xyz[0]);
        
        float cy = cosf(T1_zsprites_to_render->cpu_data[i].angle_xyz[1]);
        float sy = sinf(T1_zsprites_to_render->cpu_data[i].angle_xyz[1]);
        
        float cz = cosf(T1_zsprites_to_render->cpu_data[i].angle_xyz[2]);
        float sz = sinf(T1_zsprites_to_render->cpu_data[i].angle_xyz[2]);
        
        // Rx * Ry * Rz  (order: X → Y → Z)
        float r00 =  cy * cz;
        float r01 =  cy * sz;
        float r02 = -sy;
        float r10 =  sx * sy * cz - cx * sz;
        float r11 =  sx * sy * sz + cx * cz;
        float r12 =  sx * cy;
        float r20 =  cx * sy * cz + sx * sz;
        float r21 =  cx * sy * sz - sx * cz;
        float r22 =  cx * cy;
        
        // Fill 4x4 in **column-major** order
        float * m = T1_zsprites_to_render->gpu_data[i].transform_mat_4x4;
        m[0]  = r00;  m[1]  = r10;  m[2]  = r20;  m[3]  = 0.0f;
        m[4]  = r01;  m[5]  = r11;  m[6]  = r21;  m[7]  = 0.0f;
        m[8]  = r02;  m[9]  = r12;  m[10] = r22;  m[11] = 0.0f;
        m[12] = 0.0f; m[13] = 0.0f; m[14] = 0.0f; m[15] = 1.0f;
    }
}

// static float clickray_elapsed = 0.0f;
void renderer_hardware_render(
    T1GPUFrame * frame_data,
    uint64_t elapsed_us)
{
    (void)elapsed_us;
    
    if (renderer_initialized != true) {
        log_append("renderer not initialized, aborting...\n");
        return;
    }
    
    if (
        frame_data == NULL ||
        frame_data->verts == NULL)
    {
        log_append("ERROR: platform layer didnt pass recipients\n");
        return;
    }
    
    log_assert(T1_zsprites_to_render->size < MAX_ZSPRITES_PER_BUFFER);
    
    construct_transformation_matrices();
    
    T1_std_memcpy(
        /* void * dest: */
            frame_data->zsprite_list->polygons,
        /* const void * src: */
            T1_zsprites_to_render->gpu_data,
        /* size_t n: */
            sizeof(T1GPUzSprite) * T1_zsprites_to_render->size);
    frame_data->zsprite_list->size = T1_zsprites_to_render->size;
    
    log_assert(
        frame_data->zsprite_list->size <= T1_zsprites_to_render->size);
    log_assert(
        T1_zsprites_to_render->size < MAX_ZSPRITES_PER_BUFFER);
    log_assert(
        frame_data->zsprite_list->size < MAX_ZSPRITES_PER_BUFFER);
    
    frame_data->zsprite_list->size = T1_zsprites_to_render->size;
    
    *frame_data->postproc_consts =
        T1_engine_globals->postproc_consts;
    
    add_opaque_zpolygons_to_workload(frame_data);
    
    if (T1_app_running) {
        #if T1_PARTICLES_ACTIVE == T1_ACTIVE
        T1_particle_add_all_to_frame_data(
            /* GPUDataForSingleFrame * frame_data: */
                frame_data,
            /* uint64_t elapsed_us: */
                elapsed_us,
            /* const uint32_t alpha_blending: */
                false);
        
        #if 0
        T1_particle_lineparticle_add_all_to_frame_data(
            frame_data,
            elapsed_us,
            false);
        #endif
        #elif T1_PARTICLES_ACTIVE == T1_INACTIVE
        // Pass
        #else
        #error "T1_PARTICLES_ACTIVE undefined"
        #endif
    }
    
    add_alphablending_zpolygons_to_workload(frame_data);
    
    #if T1_PARTICLES_ACTIVE == T1_ACTIVE
    T1_particle_add_all_to_frame_data(
        /* GPUDataForSingleFrame * frame_data: */
            frame_data,
        /* uint64_t elapsed_us: */
            elapsed_us,
        /* const uint32_t alpha_blending: */
            true);
    
    #if 0
    T1_particle_lineparticle_add_all_to_frame_data(
            frame_data,
            elapsed_us,
            true);
    #endif
    
    #elif T1_PARTICLES_ACTIVE == T1_INACTIVE
    // Pass
    #else
    #error
    #endif
    
    #if T1_RAW_SHADER_ACTIVE == T1_ACTIVE
    add_points_and_lines_to_workload(frame_data);
    
    if (application_running && engine_globals->draw_axes) {
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
    
    if (
        application_running &&
        engine_globals->draw_imputed_normals)
    {
        assert(0);
    }
    
    if (application_running && engine_globals->draw_mouseptr) {
        float xyz[3];
        float z = 0.05f;
        xyz[0] = engineglobals_screenspace_x_to_x(
            user_interactions[INTR_PREVIOUS_MOUSE_OR_TOUCH_MOVE].screen_x,
            z) + camera.xyz[0];
        xyz[1] = engineglobals_screenspace_y_to_y(
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
    #elif T1_RAW_SHADER_ACTIVE == T1_INACTIVE
    // Pass
    #else
    #error
    #endif
}
