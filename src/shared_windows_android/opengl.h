#include "../shared/static_redefinitions.h"
#include "../shared/platform_layer.h"

#include <gl/gl.h>
#include <assert.h>

#ifndef OPENGL_H
#define OPENGL_H

#define GL_VERTEX_SHADER 0x8B31
#define GL_FRAGMENT_SHADER 0x8B30
typedef char GLchar;

// These typedefs are function pointers
// they weren't in OpenGL v1.0, but were added as
// extensions
// you need query for their availability and set the
// pointers during runtime with wglGetProcAddress()
// to make it work without a library
typedef BOOL WINAPI manual_wgl_swap_interval_ext(int interval);

typedef void WINAPI ptr_gl_attach_shader(GLuint program, GLuint shader);
typedef void ptr_gl_compile_shader(GLuint shader_id);
typedef GLuint ptr_gl_create_shader(GLenum type);
typedef GLuint ptr_gl_create_program(void);
typedef void ptr_gl_link_program(GLuint program_id);
typedef void ptr_gl_shader_source(GLuint shader_id, GLsizei count, GLchar **string, GLint * length);
typedef void ptr_gl_use_program(GLuint program_id);

extern ptr_gl_compile_shader * glCompileShader;
extern ptr_gl_create_shader * glCreateShader;
extern ptr_gl_create_program * glCreateProgram;
extern ptr_gl_link_program * glLinkProgram;
extern ptr_gl_shader_source * glShaderSource;
extern ptr_gl_attach_shader * glAttachShader;
extern ptr_gl_use_program * glUseProgram;

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

