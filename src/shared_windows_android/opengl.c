#include "opengl.h"

float example_vertices[] = {
    -0.5f, -0.5f, 0.0f,
    0.5f, -0.5f, 0.0f,
    0.0f, 0.5, 0.0f
};

// We'll need these 2 identifiers while drawing
GLuint program_id;
unsigned int VAO;


ptr_gl_compile_shader * glCompileShader;
ptr_gl_get_shader_iv * glGetShaderiv;
ptr_gl_get_shader_info_log * glGetShaderInfoLog;
ptr_gl_create_shader * glCreateShader;
ptr_gl_create_program * glCreateProgram;
ptr_gl_link_program * glLinkProgram;
ptr_gl_shader_source * glShaderSource;
ptr_gl_attach_shader * glAttachShader;
ptr_gl_use_program * glUseProgram;
ptr_gl_gen_buffers * glGenBuffers;
ptr_gl_bind_buffer * glBindBuffer;
ptr_gl_buffer_data * glBufferData;
ptr_gl_gen_vertex_arrays * glGenVertexArrays;
ptr_gl_bind_vertex_array * glBindVertexArray;
ptr_gl_vertex_attrib_pointer * glVertexAttribPointer;
ptr_gl_enable_vertex_attrib_array * glEnableVertexAttribArray;

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
    printf("vertex shader source was loaded, compiling\n");
    glCompileShader(vertex_shader_id);
    unsigned int success;
    char info_log[512];
    glGetShaderiv(vertex_shader_id, GL_COMPILE_STATUS, &success);
    if (success) {
        printf("vertex shader source was compiled\n");
    } else {
        printf("failed to compile vertex shader\n");
        glGetShaderInfoLog(vertex_shader_id, 512, NULL, info_log);
        printf("%s\n", info_log);
        assert(0);
    }
    
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
    printf("fragment shader src was loaded, compiling\n");
    glCompileShader(fragment_shader_id);
    glGetShaderiv(
        fragment_shader_id,
        GL_COMPILE_STATUS,
        &success);
    if (success) {
        printf("fragment shader source was compiled\n");
    } else {
        printf("failed to compile fragment shader\n");
        glGetShaderInfoLog(fragment_shader_id, 512, NULL, info_log);
        printf("%s\n", info_log);
        assert(0);
    }
    
    program_id = glCreateProgram();
    printf("created GL program with id: %u\n", program_id);
    glAttachShader(program_id, vertex_shader_id);
    printf("attached vertex shader to program\n");
    glAttachShader(program_id, fragment_shader_id);
    printf("attached fragment shader to program\n");
    
    glLinkProgram(program_id);
    printf(
        "linked program with program_id: %u\n",
        program_id);
    
    // TODO: this should be stored in bound vertex array
    // and happen after glBindBuffer & glBufferData
    // so maybe we have to do it every frame
    // we also MUST use a VAO (vertex array obj) or
    // openGL will most likely draw nothing
    // glBindBuffer(stuff);
    // glBufferData(stuff);
    glGenVertexArrays(1, &VAO);
    printf("created vertex array with id: %u\n", VAO);
    glBindVertexArray(VAO);
    printf("vertex array bound (active)\n");
    unsigned int VBO;
    glGenBuffers(1, &VBO);
    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    glBufferData(
        GL_ARRAY_BUFFER,
        sizeof(example_vertices),
        example_vertices,
        GL_STATIC_DRAW);
    glVertexAttribPointer(
        /* location (in shader source code): */ 0,
        /* number of vals in Vertex: */ 3,
        /* type of data: */ GL_FLOAT,
        /* normalize data: */ GL_FALSE,
        /* stride (size in bytes): */ 3 * sizeof(float),
        /* offset : */ (void*)0);
    glEnableVertexAttribArray(0);
    printf("finished glVertexAttribPointer()\n", VAO);
}

