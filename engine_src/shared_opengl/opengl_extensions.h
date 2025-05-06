#ifndef TOKONE_OPENGL_EXTENSIONS_H
#define TOKONE_OPENGL_EXTENSIONS_H

#include <windows.h>
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

#define GLchar     char
#define GLsizeiptr ptrdiff_t
#define GLintptr   ptrdiff_t

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
#define GL_SHADER_STORAGE_BUFFER          0x90D2

#define GL_TEXTURE0                       0x84C0
#define GL_TEXTURE_2D_ARRAY               0x8C1A
#define GL_CLAMP_TO_EDGE                  0x812F

#define GL_STATIC_DRAW                    0x88E4
#define GL_STREAM_DRAW                    0x88E0

#define GL_READ_ONLY                      0x88B8
#define GL_WRITE_ONLY                     0x88B9
#define GL_READ_WRITE                     0x88BA

#define GL_MAX_VERTEX_ATTRIBS             0x8869


void init_opengl_extensions(
    void (* fetch_extension_func_address_func)(
        void ** extptr,
        char * func_name));

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
extern void (* extptr_glBindVertexArray)(
    GLuint array);
extern void (* extptr_glBindBuffer)(
    GLenum target,
    GLuint buffer);
extern void (* extptr_glVertexAttribIPointer)(
    GLuint index,
    GLint size,
    GLenum type,
    GLsizei stride,
    const void * pointer);
/*
index
Specifies the index of the generic vertex attribute to be modified.

size
Specifies the number of components per generic vertex attribute.
Must be 1, 2, 3, or 4. The initial value is 4.

type
Specifies the data type of each component in the array.
Symbolic constants GL_BYTE, GL_UNSIGNED_BYTE, GL_SHORT,
GL_UNSIGNED_SHORT, GL_FIXED, or GL_FLOAT are accepted. The initial
value is GL_FLOAT.

normalized
Specifies whether fixed-point data values should be normalized
(GL_TRUE) or converted directly as fixed-point values (GL_FALSE)
when they are accessed.

stride
Specifies the byte offset between consecutive generic vertex attributes.
If stride is 0, the generic vertex attributes are understood to be
tightly packed in the array.
The initial value is 0.

pointer
Specifies a pointer to the first component of the first generic vertex
attribute in the array. The initial value is 0.
*/
void (* extptr_glVertexAttribPointer)(
    GLuint index,
    GLint size,
    GLenum type,
    GLboolean normalized,
    GLsizei stride,
    const void * pointer);
void (* extptr_glEnableVertexAttribArray)(
    GLuint index);
void (* extptr_glValidateProgram)(
    GLuint program);
void (* extptr_glGetProgramInfoLog)(
    GLuint program,
    GLsizei maxLength,
    GLsizei *length,
    char *infoLog);
void (* extptr_glBufferData)(
    GLenum target,
    GLsizeiptr size,
    const void * data,
    GLenum usage);
void (* extptr_glBindBufferBase)(
    GLenum target,
    GLuint index,
    GLuint buffer);
void (* extptr_glUniform3fv)(
    GLint location,
    GLsizei count,
    const GLfloat * value);
void (* extptr_glGetUniformfv)(
    GLuint program,
    GLint location,
    GLfloat * params);
void (* extptr_glGetIntegerv)(
    GLenum pname,
    GLint * params);
void * (* extptr_glMapBuffer)(
    GLenum target,
    GLenum access);
GLboolean (* extptr_glUnmapBuffer)(
    GLenum target);

/*
glGetBufferSubData

offset
Specifies the offset into the buffer object's data store from
which data will be returned, measured in bytes.

size
Specifies the size in bytes of the data store region being
returned.

data
Specifies a pointer to the location where buffer object data is
returned.
*/ 
void (* extptr_glGetBufferSubData)(
    GLenum         target,
    GLintptr       offset,
    GLsizeiptr     size,
    void *         data);

void (* extptr_glActiveTexture)(GLenum texture);

void (* extptr_glUniform1iv)(
    GLint location,
    GLsizei count,
    const GLint * value);

void (* extptr_glTexImage3D)(
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

void (* extptr_glTexSubImage3D)(
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
    const GLvoid * data);

#endif // TOKONE_OPENGL_EXTENSIONS_H

