#include <GL/gl.h>
#include <assert.h>
#include <stddef.h>

#include "cpu_gpu_shared_types.h"
#include "cpu_to_gpu_types.h"
#include "common.h"

#ifndef OPENGL_H
#define OPENGL_H

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

void opengl_render_triangles(void);

void opengl_compile_shaders(
    char * vertex_shader_source,
    uint32_t vertex_shader_source_size,
    char * fragment_shader_source,
    uint32_t fragment_shader_source_size);

#endif

