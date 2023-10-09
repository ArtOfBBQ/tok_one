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

#define GL_VERTEX_SHADER 0x8B31
#define GL_FRAGMENT_SHADER 0x8B30
#define GL_ARRAY_BUFFER 0x8892
#define GL_COMPILE_STATUS 0x8B81
#define GL_STATIC_DRAW 0x88E4
#define GL_DYNAMIC_DRAW 0x88E8
#define GL_UNIFORM_BUFFER 0x8A11
#define GL_TEXTURE0 0x84C0
#define GL_CLAMP_TO_EDGE 0x812F
#define GL_TEXTURE_2D_ARRAY 0x8C1A
// #define GL_TEXTURE_MAG_FILTER 0x2800
#define GL_TEXTURE_MAX_LEVEL 0x813D
#define GL_TEXTURE_BASE_LEVEL 0x813C
#define GL_UNPACK_IMAGE_HEIGHT 0x806E


typedef char GLchar;
// typedef int GLsizeiptr;

// These typedefs are function pointers
// they weren't in OpenGL v1.0, but were added as
// extensions
// you need query for their availability and set the
// pointers during runtime with wglGetProcAddress()
// to make it work without a library
// this is done in windows.c
typedef void ptr_gl_compile_shader(GLuint shader_id);
typedef ptr_gl_get_shader_iv(
    GLuint shader,
    GLenum pname,
    GLint * params);
typedef ptr_gl_get_shader_info_log(
    GLuint shader_id,
    GLsizei maxLength,
    GLsizei *length,
    GLchar *infolog);
typedef GLuint ptr_gl_create_shader(GLenum type);
typedef GLuint ptr_gl_create_program(void);
typedef void ptr_gl_link_program(GLuint program_id);
typedef void ptr_gl_shader_source(
    GLuint shader_id,
    GLsizei count,
    GLchar **string,
    GLint * length);
typedef void ptr_gl_use_program(GLuint program_id);
typedef void ptr_gl_gen_buffers(GLsizei size, GLuint* buffers);
typedef void ptr_gl_bind_buffer(GLenum target, GLuint buffer_id);
typedef void ptr_gl_buffer_data(
    GLenum mode,
    GLsizeiptr size,
    const GLvoid* data,
    GLenum usage);
typedef void ptr_gl_gen_vertex_arrays(GLsizei n, GLuint * arrays);
typedef void ptr_gl_bind_vertex_array(GLuint array_id);
typedef void ptr_gl_vertex_attrib_pointer(
    GLuint index,
    GLint size,
    GLenum type,
    GLboolean normalized,
    GLsizei stride,
    const GLvoid * pointer);
typedef void ptr_gl_enable_vertex_attrib_array(GLuint id);
typedef void ptr_gl_generate_mipmap(GLenum target);
typedef void ptr_gl_active_texture(GLenum unit);
typedef void ptr_gl_uniform_1i(GLint location, GLint x);
typedef GLint ptr_gl_get_uniform_location(
    GLuint program,
    const GLchar * name);
typedef void ptr_gl_tex_image_3d(
    GLenum target,
    GLint level,
    GLint internalformat,
    GLsizei width,
    GLsizei height,
    GLsizei depth,
    GLint border,
    GLenum format,
    GLenum type,
    const void * data);
typedef void ptr_gl_tex_sub_image_3d(
    GLenum target,
    GLint level,
    GLint xoffset,
    GLint yoffset,
    GLint zoffset,
    GLsizei width,
    GLsizei height,
    GLsizei depth,
    GLenum format,
    GLenum type,
    const void * pixels);
typedef void ptr_gl_tex_storage_3d(
    GLenum target,
    GLsizei levels,
    GLenum internalformat,
    GLsizei width,
    GLsizei height,
    GLsizei depth);


// extern Vertex gpu_workload_buffer[VERTEX_BUFFER_SIZE];
// extern unsigned int texture_array_ids[TEXTUREARRAYS_SIZE];

// We'll need these 2 identifiers while drawing
extern GLuint program_id;
extern unsigned int VAO;

extern ptr_gl_compile_shader * glCompileShader;
extern ptr_gl_get_shader_iv * glGetShaderiv;
extern ptr_gl_get_shader_info_log * glGetShaderInfoLog;
extern ptr_gl_create_shader * glCreateShader;
extern ptr_gl_create_program * glCreateProgram;
extern ptr_gl_link_program * glLinkProgram;
extern ptr_gl_shader_source * glShaderSource;
// extern ptr_gl_attach_shader * glAttachShader;
extern ptr_gl_use_program * glUseProgram;
extern ptr_gl_gen_buffers * glGenBuffers;
extern ptr_gl_bind_buffer * glBindBuffer;
extern ptr_gl_buffer_data * glBufferData;
extern ptr_gl_gen_vertex_arrays * glGenVertexArrays;
extern ptr_gl_bind_vertex_array * glBindVertexArray;
extern ptr_gl_vertex_attrib_pointer * glVertexAttribPointer;
extern ptr_gl_enable_vertex_attrib_array * glEnableVertexAttribArray;
extern ptr_gl_generate_mipmap * glGenerateMipmap;
extern ptr_gl_uniform_1i * glUniform1i;
extern ptr_gl_get_uniform_location * glGetUniformLocation;
extern ptr_gl_tex_storage_3d * glTexStorage3D;


// info about what OpenGL functionality
// is/isnt available on platform
typedef struct OpenGLInfo
{
    // TODO: some of these string fields (like extensions)
    // are parsed into bool fields and then not used
    // anymore. They might be useful
    // for debugging right now but should be deleted
    char * vendor;
    char * renderer;
    char * version;
    char * shading_language_version;
    char * extensions; 
} OpenGLInfo;

OpenGLInfo get_opengl_info();

void opengl_compile_shaders();

#endif

