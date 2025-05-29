#include "T1_lines.h"

static GPURawVertex line_vertices[MAX_LINE_VERTICES];
static CPULineData lines_cpu_data[MAX_LINE_VERTICES / 2];
static int32_t lines_to_render_size = 0;

static GPURawVertex point_vertices[MAX_POINT_VERTICES];
static CPUPointData points_cpu_data[MAX_POINT_VERTICES];
static int32_t points_to_render_size = 0;

void fetch_next_line(LineRequest * stack_recipient)
{
    common_memset_char(stack_recipient, 0, sizeof(LineRequest));
    
    int32_t use_i = -1;
    for (
        int32_t l_i = 0;
        l_i < lines_to_render_size;
        l_i++)
    {
        if (lines_cpu_data[l_i].deleted)
        {
            use_i = l_i;
            break;
        }
    }
    
    if (use_i < 0) {
        if ((use_i + 1) >= MAX_LINE_VERTICES) {
            return;
        }
        use_i = lines_to_render_size;
        lines_to_render_size += 1;
    }
    
    log_assert(use_i * 2 < MAX_LINE_VERTICES);
    
    stack_recipient->cpu_data = &lines_cpu_data[use_i];
    stack_recipient->gpu_vertices = &line_vertices[use_i * 2];
    stack_recipient->cpu_data->committed = false;
    stack_recipient->cpu_data->deleted = false;
    return;
}

bool32_t fetch_line_by_object_id(
    LineRequest * recipient,
    const int32_t object_id)
{
    common_memset_char(recipient, 0, sizeof(LineRequest));
    
    for (
        int32_t l_i = 0;
        l_i < lines_to_render_size;
        l_i++)
    {
        if (!lines_cpu_data[l_i].deleted &&
            lines_cpu_data[l_i].object_id == object_id)
        {
            recipient->cpu_data = &lines_cpu_data[l_i];
            recipient->gpu_vertices = &line_vertices[l_i * 2];
            return true;
        }
    }
    
    log_assert(recipient->cpu_data == NULL);
    log_assert(recipient->gpu_vertices == NULL);
    return false;
}

void delete_line_object(const int32_t object_id)
{
    for (int32_t line_i = 0; line_i < lines_to_render_size; line_i++) {
        if (lines_cpu_data[line_i].object_id == object_id) {
            lines_cpu_data[line_i].deleted = true;
        }
    }
}

void commit_line(LineRequest * to_commit)
{
    to_commit->cpu_data->committed = true;
}

void fetch_next_point(PointRequest * stack_recipient)
{
    common_memset_char(stack_recipient, 0, sizeof(PointRequest));
    
    int32_t use_i = -1;
    for (
        int32_t p_i = 0;
        p_i < points_to_render_size;
        p_i++)
    {
        if (points_cpu_data[p_i].deleted)
        {
            use_i = p_i;
            break;
        }
    }
    
    assert(points_to_render_size <= MAX_POINT_VERTICES);
    
    if (use_i < 0) {
        use_i = points_to_render_size;
        points_to_render_size += 1;
    }
    
    assert(use_i < MAX_POINT_VERTICES);
    
    stack_recipient->cpu_data = &points_cpu_data[use_i];
    stack_recipient->gpu_vertex = &point_vertices[use_i];
    stack_recipient->cpu_data->committed = false;
    stack_recipient->cpu_data->deleted = false;
    return;
}

bool32_t fetch_point_by_object_id(
    PointRequest * recipient,
    const int32_t object_id)
{
    common_memset_char(recipient, 0, sizeof(PointRequest));
    
    for (
        int32_t l_i = 0;
        l_i < points_to_render_size;
        l_i++)
    {
        if (!points_cpu_data[l_i].deleted &&
            points_cpu_data[l_i].object_id == object_id)
        {
            recipient->cpu_data     = &points_cpu_data[l_i];
            recipient->gpu_vertex   = &point_vertices[l_i];
            return true;
        }
    }
    
    log_assert(recipient->cpu_data == NULL);
    log_assert(recipient->gpu_vertex == NULL);
    return false;
}

void delete_point_object(const int32_t object_id)
{
    for (int32_t point_i = 0; point_i < points_to_render_size; point_i++) {
        if (points_cpu_data[point_i].object_id == object_id) {
            points_cpu_data[point_i].deleted = true;
        }
    }
}

void commit_point(PointRequest * to_commit)
{
    to_commit->cpu_data->committed = true;
}

#if RAW_SHADER_ACTIVE
void add_points_and_lines_to_workload(
    GPUDataForSingleFrame * frame_data)
{
    frame_data->line_vertices_size = 0;
    
    int32_t next_start = 0;
    int32_t next_skip_or_end = 0;
    
    while (next_skip_or_end < lines_to_render_size) {
        while (
            !lines_cpu_data[next_skip_or_end].deleted &&
            lines_cpu_data[next_skip_or_end].committed &&
            next_skip_or_end < lines_to_render_size)
        {
            next_skip_or_end += 1;
        }
        
        int32_t vertices_to_copy = (next_skip_or_end - next_start) * 2;
        if (vertices_to_copy > 0) {
            log_assert(vertices_to_copy % 2 == 0);
            common_memcpy(
                frame_data->line_vertices,
                line_vertices + (next_start * 2),
                sizeof(GPURawVertex) * (uint32_t)vertices_to_copy);
            next_start += vertices_to_copy / 2;
            frame_data->line_vertices_size += (uint32_t)vertices_to_copy;
        } else {
            break;
        }
    }
    
    frame_data->point_vertices_size = 0;
    
    next_start = 0;
    next_skip_or_end = 0;
    
    while (next_skip_or_end < lines_to_render_size) {
        while (
            !points_cpu_data[next_skip_or_end].deleted &&
            points_cpu_data[next_skip_or_end].committed &&
            next_skip_or_end < points_to_render_size)
        {
            next_skip_or_end += 1;
        }
        
        int32_t vertices_to_copy = (next_skip_or_end - next_start);
        if (vertices_to_copy > 0) {
            common_memcpy(
                frame_data->point_vertices,
                point_vertices + next_start,
                sizeof(GPURawVertex) * (uint32_t)vertices_to_copy);
            next_start += vertices_to_copy;
            frame_data->point_vertices_size += (uint32_t)vertices_to_copy;
        } else {
            break;
        }
    }
}
#endif
