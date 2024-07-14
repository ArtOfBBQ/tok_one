#ifndef TOK_LINES
#define TOK_LINES

#include <string.h> // memset

#include "common.h"
#include "logger.h"
#include "clientlogic_macro_settings.h"
#include "cpu_to_gpu_types.h"

/*
****************************************************************************
Below here are things intended to be used by the client program
****************************************************************************
*/


typedef struct CPUPointData {
    int32_t object_id;
    bool32_t deleted;
    bool32_t committed;
} CPUPointData;

typedef struct PointRequest {
    GPURawVertex * gpu_vertex;
    CPUPointData * cpu_data;
} PointRequest;

typedef struct CPULineData {
    int32_t object_id;
    bool32_t deleted;
    bool32_t committed;
} CPULineData;

typedef struct LineRequest {
    GPURawVertex * gpu_vertices;
    CPULineData * cpu_data;
} LineRequest;

/*
If you want to add a new line, make a LineRequest (on the stack or whatever)
and call this function. It will set the pointers of your LineRequest so you
can fill in the object_id (if you want to reference it later), and the
gpu_vertices.

gpu_vertices[0] represents the start of the line and gpu_vertices[1] is the end.

You don't need to set 'committed' or 'deleted' by yourself, call commit_line()
instead.
*/
void fetch_next_line(LineRequest * stack_recipient);

/*
Make a LineRequest (on the stack or whatever) and call this with an object_id.

If a line exists with that object_id, the pointers in your LineRequest will be
set so you can edit its properties. (So you can move your lines etc.)

returns false if no such object_id, else true
*/
bool32_t fetch_line_by_object_id(
    LineRequest * stack_recipient,
    const int32_t object_id);

/*
Delete all lines with a given object_id
*/
void delete_line_object(const int32_t object_id);

/*
Call this when you want to start rendering the line.
*/
void commit_line(LineRequest * to_commit);


void fetch_next_point(PointRequest * stack_recipient);
bool32_t fetch_point_by_object_id(
    PointRequest * stack_recipient,
    const int32_t object_id);
void delete_point_object(const int32_t object_id);
void commit_point(PointRequest * to_commit);

/*
****************************************************************************
Below here are functions intended to be used by the engine, not by the client
program.
****************************************************************************
*/

void add_points_and_lines_to_workload(GPUDataForSingleFrame * frame_data);

#endif // TOK_LINES
