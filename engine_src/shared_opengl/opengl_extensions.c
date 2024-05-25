#include "opengl_extensions.h"

HGLRC(* extptr_wglCreateContextAttribsARB)(HDC, HGLRC, const int *) = NULL;
int (* extptr_glGetUniformLocation)(GLint, const GLchar *) = NULL;

void (* extptr_glShaderSource)(
    GLuint,
    GLsizei,
    const GLchar **,
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
    GLchar *infoLog) = NULL;

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

