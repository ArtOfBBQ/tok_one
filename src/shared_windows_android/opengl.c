#include "opengl.h"

ptr_gl_compile_shader * glCompileShader;
ptr_gl_create_shader * glCreateShader;
ptr_gl_create_program * glCreateProgram;
ptr_gl_link_program * glLinkProgram;
ptr_gl_shader_source * glShaderSource;
ptr_gl_attach_shader * glAttachShader;
ptr_gl_use_program * glUseProgram;

static bool32_t are_equal_strings(
    char * str1,
    char * str2,
    size_t len)
{
    for (size_t i = 0; i < len; i++) {
        if (str1[i] != str2[i]) {
            return false;
        }
    }
    
    return true;
}

OpenGLInfo get_opengl_info() {
    OpenGLInfo return_value = {};
    
    return_value.vendor = (char *)glGetString(GL_VENDOR);
    return_value.renderer =
        (char *)glGetString(GL_RENDERER);
    return_value.version = (char *)glGetString(GL_VERSION);
    return_value.shading_language_version =
        (char *)glGetString(GL_VENDOR);
    return_value.vendor = (char *)glGetString(GL_VENDOR);
    return_value.extensions =
        (char *)glGetString(GL_EXTENSIONS);

    printf("opengl version: %s\n", return_value.version);
    
    char * at = return_value.extensions; 
    char * end = at;
    while (*end) {
        while (*at == ' ') {
            at++;
        }
        end = at;
        while (*end && *end != ' ') { end++; }
        
        if (
            are_equal_strings(
                at,
                "EXT_texture_sRGB_decode",
                end - at))
        {
            return_value.EXT_texture_sRGB_decode = true;
        } else if (
            are_equal_strings(
                at,
                "GL_ARB_framebuffer_sRGB;",
                end - at))
        {
            return_value.GL_ARB_framebuffer_sRGB = true;
        } else if (
            are_equal_strings(
                at,
                "put your expected extension string here",
                end - at))
        {
            // reserved
        }
        
        at = end;
    }
    
    return return_value;
}

void opengl_compile_shaders() {
    GLuint vertex_shader_id = glCreateShader(
        GL_VERTEX_SHADER);
    printf(
        "received vertex_shader_id: %u\n",
        vertex_shader_id);
    FileBuffer * vertex_source_file =
        platform_read_file("vertex_shader.glsl");
    assert(vertex_source_file->size > 0);
    assert(vertex_source_file->contents != NULL);
    glShaderSource(
        /* shader handle: */ vertex_shader_id,
        /* shader count : */ 1,
        /* shader source: */
            &vertex_source_file->contents,
        /* source length: */
            NULL);
    printf("shader source was loaded ,compiling\n");
    glCompileShader(vertex_shader_id);

    // TODO: consider adding this extension so we can check
    // for success
    // int success = 0;
    // glGetShaderiv(vertex_shader_id, GL_COMPILE_STATUS, &success);
    // assert(success);
    
    GLuint fragment_shader_id =
        glCreateShader(GL_FRAGMENT_SHADER);
    FileBuffer * fragment_source_file =
        platform_read_file("fragment_shader.glsl");
    assert(fragment_source_file->size > 0);
    assert(fragment_source_file->contents != NULL);
    glShaderSource(
        /* shader handle: */ fragment_shader_id,
        /* shader count : */ 1,
        /* shader source: */
            &fragment_source_file->contents,
        /* source length: */
            NULL);
    glCompileShader(fragment_shader_id);
    
    
    GLuint program_id = glCreateProgram();
    glAttachShader(program_id, vertex_shader_id);
    glAttachShader(program_id, fragment_shader_id);
    
    glLinkProgram(program_id);
    glUseProgram(program_id);
}

