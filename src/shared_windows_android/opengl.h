#include "../shared/static_redefinitions.h"
#include "../shared/vertex_types.h"
#include "../shared/platform_layer.h"
#include "../shared/software_renderer.h"

#include <gl/gl.h>
#include <assert.h>

#ifndef OPENGL_H
#define OPENGL_H

#define VERTEX_BUFFER_SIZE 500000

#define GL_VERTEX_SHADER 0x8B31
#define GL_FRAGMENT_SHADER 0x8B30
#define GL_ARRAY_BUFFER 0x8892
#define GL_COMPILE_STATUS 0x8B81
#define GL_STATIC_DRAW 0x88E4
#define GL_DYNAMIC_DRAW 0x88E8
#define GL_UNIFORM_BUFFER 0x8A11
#define GL_TEXTURE0 0x84C0

typedef char GLchar;
typedef int GLsizeiptr;

// These typedefs are function pointers
// they weren't in OpenGL v1.0, but were added as
// extensions
// you need query for their availability and set the
// pointers during runtime with wglGetProcAddress()
// to make it work without a library
// this is done in windows.c
typedef BOOL WINAPI manual_wgl_swap_interval_ext(int interval);

typedef void WINAPI ptr_gl_attach_shader(GLuint program, GLuint shader);
typedef void ptr_gl_compile_shader(GLuint shader_id);
typedef ptr_gl_get_shader_iv(GLuint shader, GLenum pname, GLint * params);
typedef ptr_gl_get_shader_info_log(GLuint shader_id, GLsizei maxLength, GLsizei *length, GLchar *infolog);
typedef GLuint ptr_gl_create_shader(GLenum type);
typedef GLuint ptr_gl_create_program(void);
typedef void ptr_gl_link_program(GLuint program_id);
typedef void ptr_gl_shader_source(GLuint shader_id, GLsizei count, GLchar **string, GLint * length);
typedef void ptr_gl_use_program(GLuint program_id);
typedef void ptr_gl_gen_buffers(GLsizei size, GLuint* buffers);
typedef void ptr_gl_bind_buffer(GLenum target, GLuint buffer_id);
typedef void ptr_gl_buffer_data(GLenum mode, GLsizeiptr size, const GLvoid* data, GLenum usage);
typedef void ptr_gl_gen_vertex_arrays(GLsizei n, GLuint * arrays);
typedef void ptr_gl_bind_vertex_array(GLuint array_id);
typedef void ptr_gl_vertex_attrib_pointer(GLuint index, GLint size, GLenum type, GLboolean normalized, GLsizei stride, const GLvoid * pointer);
typedef void ptr_gl_enable_vertex_attrib_array(GLuint id);
typedef void ptr_gl_generate_mipmap(GLenum target);
typedef void ptr_gl_active_texture(GLenum unit);
typedef void ptr_gl_uniform_1i(GLint location, GLint x);
typedef GLint ptr_gl_get_uniform_location(
    GLuint program,
    const GLchar * name);

extern Vertex gpu_workload_buffer[VERTEX_BUFFER_SIZE];

// We'll need these 2 identifiers while drawing
extern GLuint program_id;
extern unsigned int VAO;
extern unsigned int texture_ids[TEXTURES_SIZE];

extern ptr_gl_compile_shader * glCompileShader;
extern ptr_gl_get_shader_iv * glGetShaderiv;
extern ptr_gl_get_shader_info_log * glGetShaderInfoLog;
extern ptr_gl_create_shader * glCreateShader;
extern ptr_gl_create_program * glCreateProgram;
extern ptr_gl_link_program * glLinkProgram;
extern ptr_gl_shader_source * glShaderSource;
extern ptr_gl_attach_shader * glAttachShader;
extern ptr_gl_use_program * glUseProgram;
extern ptr_gl_gen_buffers * glGenBuffers;
extern ptr_gl_bind_buffer * glBindBuffer;
extern ptr_gl_buffer_data * glBufferData;
extern ptr_gl_gen_vertex_arrays * glGenVertexArrays;
extern ptr_gl_bind_vertex_array * glBindVertexArray;
extern ptr_gl_vertex_attrib_pointer * glVertexAttribPointer;
extern ptr_gl_enable_vertex_attrib_array * glEnableVertexAttribArray;
extern ptr_gl_generate_mipmap * glGenerateMipmap;
extern ptr_gl_active_texture * glActiveTexture;
extern ptr_gl_uniform_1i * glUniform1i;
extern ptr_gl_get_uniform_location * glGetUniformLocation;

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
    
    bool32_t EXT_texture_sRGB_decode;
    bool32_t GL_ARB_framebuffer_sRGB;
} OpenGLInfo;

// TODO: this is out of place here
// should be moved to a string manipulation header
// or something?
static bool32_t are_equal_strings(
    char * str1,
    char * str2,
    size_t len);

OpenGLInfo get_opengl_info();

void opengl_compile_shaders();

#endif

