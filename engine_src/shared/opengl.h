#ifndef TOK_OPENGL_H
#define TOK_OPENGL_H

#include "cpu_gpu_shared_types.h"
#include "cpu_to_gpu_types.h"

void opengl_init(
    char * vertex_shader_source,
    char * fragment_shader_source);

void opengl_copy_locked_vertices(
    GPULockedVertex * locked_vertices);

void opengl_copy_projection_constants(
    GPUProjectionConstants * projection_constants);

void opengl_render_frame(GPUDataForSingleFrame * frame);

#endif

