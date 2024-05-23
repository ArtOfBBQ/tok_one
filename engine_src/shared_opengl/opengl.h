#ifndef OPENGL_H
#define OPENGL_H

#include <GL/gl.h>
#include <assert.h>
#include <stddef.h>
#include <stdint.h>

#include "opengl_extensions.h"
#include "cpu_gpu_shared_types.h"
#include "cpu_to_gpu_types.h"

#ifndef NULL
#define NULL 0
#endif

typedef char GLchar;
// typedef int GLsizeiptr;

// extern Vertex gpu_workload_buffer[VERTEX_BUFFER_SIZE];
// extern unsigned int texture_array_ids[TEXTUREARRAYS_SIZE];

// We'll need these 2 identifiers while drawing
extern GLuint program_id;
extern unsigned int VAO;

void platform_gpu_copy_locked_vertices(void);

// void opengl_render_triangles(GPUDataForSingleFrame * frame_data);

void opengl_compile_shaders(
    char * vertex_shader_source,
    uint32_t vertex_shader_source_size,
    char * fragment_shader_source,
    uint32_t fragment_shader_source_size);

void opengl_set_projection_constants(GPUProjectionConstants * pjc);

void shadersource_apply_macro_inplace(
    char * shader_source,
    char * to_replace,
    char * replacement);

#endif

