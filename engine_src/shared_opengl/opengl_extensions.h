#ifndef TOKONE_OPENGL_EXTENSIONS_H
#define TOKONE_OPENGL_EXTENSIONS_H

#include <GL/gl.h>

// /*
// This header file contains a bunch of definitions that don't come with
// OpenGL 'out of the box', they are extensions that need to be loaded at
// run-time.
// 
// There is a bunch of straightforward #define constants, but after that
// are a bunch of function pointers. We will set these to NULL and it will
// be the job of the OS-specific code to query the address of each of
// those functions and set them.
// */

// /* Start of constants section */
#define WGL_CONTEXT_MAJOR_VERSION_ARB     0x2091
#define WGL_CONTEXT_MINOR_VERSION_ARB     0x2092
// #define WGL_CONTEXT_FLAGS_ARB             0x2094
#define WGL_CONTEXT_CORE_PROFILE_BIT_ARB  0x00000001
#define WGL_CONTEXT_PROFILE_MASK_ARB      0x9126

#define GL_COMPILE_STATUS                 0x8B81
#define GL_LINK_STATUS                    0x8B82
#define GL_VALIDATE_STATUS                0x8B83

#define GL_VERTEX_SHADER                  0x8B31
#define GL_FRAGMENT_SHADER                0x8B30
#define GL_ARRAY_BUFFER                   0x8892


extern HGLRC (* extptr_wglCreateContextAttribsARB)(
    HDC device_context,
    HGLRC gl_render_context,
    const int * attribute_list);

extern int (* extptr_glGetUniformLocation)(
    GLint program,
    const char * name);

extern void (* extptr_glShaderSource)(
    GLuint shader,
    GLsizei count,
    const char **string,
    const GLint *length);

extern void (* extptr_glCompileShader)(GLuint shader);

extern void (* extptr_glGetShaderiv)(
    GLuint shader,
    GLenum pname,
    GLint *params);

extern GLuint (* extptr_glCreateProgram)(void);

extern GLuint (* extptr_glCreateShader)(GLenum shaderType);

extern void (* extptr_glGetShaderInfoLog)(
    GLuint shader,
    GLsizei maxLength,
    GLsizei * length,
    char * infoLog);

extern void (* extptr_glAttachShader)(
    GLuint program,
    GLuint shader);

extern void (* extptr_glLinkProgram)(GLuint program);

extern void (* extptr_glGetProgramiv)(
    GLuint program,
    GLenum pname,
    GLint *params);

extern void (* extptr_glUseProgram)(GLuint program);

extern void (* extptr_glGenVertexArrays)(
    GLsizei n,
    GLuint * arrays);

extern void (* extptr_glGenBuffers)(
    GLsizei n,
    GLuint * buffers);

extern void (* extptr_glBindVertexArray)(GLuint array);

extern void (* extptr_glBindBuffer)(
    GLenum target,
    GLuint buffer);

extern void (* extptr_glVertexAttribIPointer)(
    GLuint index,
    GLint size,
    GLenum type,
    GLsizei stride,
    const void * pointer);

void (* extptr_glVertexAttribPointer)(
    GLuint index,
    GLint size,
    GLenum type,
    GLboolean normalized,
    GLsizei stride,
    const void * pointer);

void (* extptr_glEnableVertexAttribArray)(GLuint index);

void (* extptr_glValidateProgram)(GLuint program);

void (* extptr_glGetProgramInfoLog)(
    GLuint program,
    GLsizei maxLength,
    GLsizei *length,
    char *infoLog);

#endif // TOKONE_OPENGL_EXTENSIONS_H

