#include "opengl_extensions.h"

HGLRC(* extptr_wglCreateContextAttribsARB)(HDC, HGLRC, const int *) = NULL;
int (* extptr_glGetUniformLocation)(GLint, const char *) = NULL;

void (* extptr_glShaderSource)(
    GLuint,
    GLsizei,
    const char **,
    const GLint *) = NULL;

void (* extptr_glCompileShader)(GLuint shader) = NULL;

void (* extptr_glGetShaderiv)(
    GLuint shader,
    GLenum pname,
    GLint *params) = NULL;

GLuint (* extptr_glCreateProgram)(void) = NULL;

GLuint (* extptr_glCreateShader)(GLenum shaderType) = NULL;

void (* extptr_glGetShaderInfoLog)(
    GLuint,
    GLsizei,
    GLsizei *,
    char *) = NULL;
void (* extptr_glAttachShader)(
    GLuint program,
    GLuint shader) = NULL;
void (* extptr_glLinkProgram)(GLuint program) = NULL;
void (* extptr_glGetProgramiv)(
    GLuint program,
    GLenum pname,
    GLint *params) = NULL;
void (* extptr_glUseProgram)(GLuint program) = NULL;
void (* extptr_glGenVertexArrays)(
    GLsizei n,
    GLuint * arrays) = NULL;
void (* extptr_glGenBuffers)(
    GLsizei n,
    GLuint * buffers) = NULL;
void (* extptr_glBindVertexArray)(GLuint array) = NULL;
void (* extptr_glBindBuffer)(
    GLenum target,
    GLuint buffer) = NULL;
void (* extptr_glVertexAttribIPointer)(
    GLuint index,
    GLint size,
    GLenum type,
    GLsizei stride,
    const void * pointer) = NULL;
void (* extptr_glVertexAttribPointer)(
    GLuint index,
    GLint size,
    GLenum type,
    GLboolean normalized,
    GLsizei stride,
    const void * pointer) = NULL;
void (* extptr_glEnableVertexAttribArray)(GLuint index) = NULL;
void (* extptr_glValidateProgram)(GLuint program) = NULL;
void (* extptr_glGetProgramInfoLog)(
    GLuint program,
    GLsizei maxLength,
    GLsizei *length,
    char *infoLog) = NULL;
void (* extptr_glBufferData)(
    GLenum target,
    GLsizeiptr size,
    const void * data,
    GLenum usage) = NULL;
void (* extptr_glBindBufferBase)(
    GLenum target,
    GLuint index,
    GLuint buffer) = NULL;
void (* extptr_glUniform3fv)(
    GLint location,
    GLsizei count,
    const GLfloat * value) = NULL;
void (* extptr_glGetUniformfv)(
    GLuint program,
    GLint location,
    GLfloat * params) = NULL;
void (* extptr_glGetIntegerv)(
    GLenum pname,
    GLint * params) = NULL;
void (* extptr_glGetBufferSubData)(
    GLenum     target,
    GLintptr   offset,
    GLsizeiptr size,
    void *     data) = NULL;
void * (* extptr_glMapBuffer)(
    GLenum target,
    GLenum access) = NULL;
GLboolean (* extptr_glUnmapBuffer)(
    GLenum target) = NULL;

void init_opengl_extensions(
    void (* fetch_extension_func_address)(
        void ** extptr,
        char * func_name))
{
    fetch_extension_func_address(
        (void **)&extptr_wglCreateContextAttribsARB,
        "wglCreateContextAttribsARB");
    fetch_extension_func_address(
        (void **)&extptr_glGetUniformLocation,
        "glGetUniformLocation");
    fetch_extension_func_address(
        (void **)&extptr_glCreateProgram,
        "glCreateProgram");
    fetch_extension_func_address(
        (void **)&extptr_glCreateShader,
        "glCreateShader");
    fetch_extension_func_address(
        (void **)&extptr_glShaderSource,
        "glShaderSource");
    fetch_extension_func_address(
        (void **)&extptr_glCompileShader,
        "glCompileShader");
    fetch_extension_func_address(
        (void **)&extptr_glGetShaderiv,
        "glGetShaderiv");
    fetch_extension_func_address(
        (void **)&extptr_glGetShaderInfoLog,
        "glGetShaderInfoLog");
    fetch_extension_func_address(
        (void **)&extptr_glAttachShader,
        "glAttachShader");
    fetch_extension_func_address(
        (void **)&extptr_glLinkProgram,
        "glLinkProgram");
    fetch_extension_func_address(
        (void **)&extptr_glGetProgramiv,
        "glGetProgramiv");
    fetch_extension_func_address(
        (void **)&extptr_glUseProgram,
        "glUseProgram");
    fetch_extension_func_address(
        (void **)&extptr_glGenVertexArrays,
        "glGenVertexArrays");
    fetch_extension_func_address(
        (void **)&extptr_glGenBuffers,
        "glGenBuffers");
    fetch_extension_func_address(
        (void **)&extptr_glBindVertexArray,
        "glBindVertexArray");
    fetch_extension_func_address(
        (void **)&extptr_glBindBuffer,
        "glBindBuffer");
    fetch_extension_func_address(
        (void **)&extptr_glVertexAttribIPointer,
        "glVertexAttribIPointer");
    fetch_extension_func_address(
        (void **)&extptr_glVertexAttribPointer,
        "glVertexAttribPointer");
    fetch_extension_func_address(
        (void **)&extptr_glEnableVertexAttribArray,
        "glEnableVertexAttribArray");
    fetch_extension_func_address(
        (void **)&extptr_glValidateProgram,
        "glValidateProgram");
    fetch_extension_func_address(
        (void **)&extptr_glGetProgramInfoLog,
        "glGetProgramInfoLog");
    fetch_extension_func_address(
        (void **)&extptr_glBufferData,
        "glBufferData");
    fetch_extension_func_address(
        (void **)&extptr_glBindBufferBase,
        "glBindBufferBase");
    fetch_extension_func_address(
        (void **)&extptr_glUniform3fv,
        "glUniform3fv");
    fetch_extension_func_address(
        (void **)&extptr_glGetUniformfv,
        "glGetUniformfv");
    fetch_extension_func_address(
        (void **)&extptr_glGetIntegerv,
        "glGetIntegerv");
    fetch_extension_func_address(
        (void **)&extptr_glGetBufferSubData,
        "glGetBufferSubData");
    fetch_extension_func_address(
        (void **)&extptr_glMapBuffer,
        "glMapBuffer");
    fetch_extension_func_address(
        (void **)&extptr_glUnmapBuffer,
        "glUnmapBuffer");
}

