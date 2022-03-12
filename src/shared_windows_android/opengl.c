#include "opengl.h"

// We'll need these 2 identifiers while drawing
GLuint program_id;
unsigned int VAO;

Vertex gpu_workload_buffer[VERTEX_BUFFER_SIZE];


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
    printf("opengl_compile_shaders()...\n");
    
    printf("allocate buffer memory...\n");
    
    /* blue triangle */
    gpu_workload_buffer[0].x = -2.0f;
    gpu_workload_buffer[0].y = 0.5f;
    gpu_workload_buffer[0].uv[0] = 0.5f;
    gpu_workload_buffer[0].uv[1] = 1.0f;
    gpu_workload_buffer[0].RGBA[0] = 0.2f;
    gpu_workload_buffer[0].RGBA[1] = 0.3f;
    gpu_workload_buffer[0].RGBA[2] = 1.0f;
    gpu_workload_buffer[0].RGBA[3] = 1.0f;
    gpu_workload_buffer[0].lighting = 0.5f;
    gpu_workload_buffer[0].texture_i = 1;
    
    gpu_workload_buffer[1].x = -1.0f;
    gpu_workload_buffer[1].y = 0.5f;
    gpu_workload_buffer[1].uv[0] = 0.5f;
    gpu_workload_buffer[1].uv[1] = 1.0f;
    gpu_workload_buffer[1].RGBA[0] = 0.2f;
    gpu_workload_buffer[1].RGBA[1] = 0.2f;
    gpu_workload_buffer[1].RGBA[2] = 1.0f;
    gpu_workload_buffer[1].RGBA[3] = 1.0f;
    gpu_workload_buffer[1].lighting = 0.5f;
    gpu_workload_buffer[1].texture_i = 1;
    
    gpu_workload_buffer[2].x = -0.5;
    gpu_workload_buffer[2].y = 2.0f;
    gpu_workload_buffer[2].uv[0] = 0.5f;
    gpu_workload_buffer[2].uv[1] = 1.0f;
    gpu_workload_buffer[2].RGBA[0] = 0.3f;
    gpu_workload_buffer[2].RGBA[1] = 0.3f;
    gpu_workload_buffer[2].RGBA[2] = 0.7f;
    gpu_workload_buffer[2].RGBA[3] = 1.0f;
    gpu_workload_buffer[2].lighting = 0.5f;
    gpu_workload_buffer[2].texture_i = 1;
    
    /* red triangle */
    gpu_workload_buffer[3].x = -0.5f;
    gpu_workload_buffer[3].y = -5.5f;
    gpu_workload_buffer[3].uv[0] = 0.5f;
    gpu_workload_buffer[3].uv[1] = 1.0f;
    gpu_workload_buffer[3].RGBA[0] = 1.0f;
    gpu_workload_buffer[3].RGBA[1] = 0.0f;
    gpu_workload_buffer[3].RGBA[2] = 0.0f;
    gpu_workload_buffer[3].RGBA[3] = 1.0f;
    gpu_workload_buffer[3].lighting = 0.5f;
    gpu_workload_buffer[3].texture_i = 1;
    
    gpu_workload_buffer[4].x = 0.5f;
    gpu_workload_buffer[4].y = -2.5f;
    gpu_workload_buffer[4].uv[0] = 0.5f;
    gpu_workload_buffer[4].uv[1] = 1.0f;
    gpu_workload_buffer[4].RGBA[0] = 1.0f;
    gpu_workload_buffer[4].RGBA[1] = 0.2f;
    gpu_workload_buffer[4].RGBA[2] = 0.2f;
    gpu_workload_buffer[4].RGBA[3] = 1.0f;
    gpu_workload_buffer[4].lighting = 1.5f;
    gpu_workload_buffer[4].texture_i = 1;
    
    gpu_workload_buffer[5].x = 2.0;
    gpu_workload_buffer[5].y = 0.5f;
    gpu_workload_buffer[5].uv[0] = 0.5f;
    gpu_workload_buffer[5].uv[1] = 1.0f;
    gpu_workload_buffer[5].RGBA[0] = 1.0f;
    gpu_workload_buffer[5].RGBA[1] = 0.0f;
    gpu_workload_buffer[5].RGBA[2] = 0.3f;
    gpu_workload_buffer[5].RGBA[3] = 1.0f;
    gpu_workload_buffer[5].lighting = 0.5f;
    gpu_workload_buffer[5].texture_i = 1;

    /* green triangle: */
    gpu_workload_buffer[6].x = 0.2;
    gpu_workload_buffer[6].y = -0.2f;
    gpu_workload_buffer[6].uv[0] = 0.5f;
    gpu_workload_buffer[6].uv[1] = 1.0f;
    gpu_workload_buffer[6].RGBA[0] = 0.0f;
    gpu_workload_buffer[6].RGBA[1] = 1.0f;
    gpu_workload_buffer[6].RGBA[2] = 0.0f;
    gpu_workload_buffer[6].RGBA[3] = 1.0f;
    gpu_workload_buffer[6].lighting = 0.5f;
    gpu_workload_buffer[6].texture_i = -1;
    
    gpu_workload_buffer[7].x = 0.6f;
    gpu_workload_buffer[7].y = -0.2f;
    gpu_workload_buffer[7].uv[0] = 0.5f;
    gpu_workload_buffer[7].uv[1] = 1.0f;
    gpu_workload_buffer[7].RGBA[0] = 0.0f;
    gpu_workload_buffer[7].RGBA[1] = 0.8f;
    gpu_workload_buffer[7].RGBA[2] = 0.2f;
    gpu_workload_buffer[7].RGBA[3] = 1.0f;
    gpu_workload_buffer[7].lighting = 0.5f;
    gpu_workload_buffer[7].texture_i = -1;
    
    gpu_workload_buffer[8].x = 0.4;
    gpu_workload_buffer[8].y = 0.2f;
    gpu_workload_buffer[8].uv[0] = 0.5f;
    gpu_workload_buffer[8].uv[1] = 1.0f;
    gpu_workload_buffer[8].RGBA[0] = 0.1f;
    gpu_workload_buffer[8].RGBA[1] = 0.9f;
    gpu_workload_buffer[8].RGBA[2] = 0.1f;
    gpu_workload_buffer[8].RGBA[3] = 1.0f;
    gpu_workload_buffer[8].lighting = 0.5f;
    gpu_workload_buffer[8].texture_i = -1;
    
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
    glGetShaderiv(
        vertex_shader_id,
        GL_COMPILE_STATUS,
        &success);
    if (success) {
        printf("vertex shader source was compiled\n");
    } else {
        printf("failed to compile vertex shader\n");
        printf("source was: \n%s\n", vertex_source_file->contents);
        glGetShaderInfoLog(vertex_shader_id, 512, NULL, info_log);
        printf("%s\n", info_log);
        assert(0);
    }
    
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
    glCompileShader(
        fragment_shader_id);
    glGetShaderiv(
        fragment_shader_id,
        GL_COMPILE_STATUS,
        &success);
    if (success) {
        printf("fragment shader source was compiled\n");
    } else {
        printf("failed to compile fragment shader\n");
        printf("source was: \n%s\n", fragment_source_file->contents);
        glGetShaderInfoLog(
            fragment_shader_id,
            512,
            NULL,
            info_log);
        printf("%s\n", info_log);
        assert(0);
    }
    
    program_id = glCreateProgram();
    printf(
        "created GL program with id: %u\n",
        program_id);
    glAttachShader(program_id, vertex_shader_id);
    printf("attached vertex shader to program\n");
    glAttachShader(
        program_id,
        fragment_shader_id);
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
    assert(sizeof(float) == 4); // x,y,uv,rgba,lighting
    assert(sizeof(uint32_t) == 4); // texture_i
    assert(sizeof(Vertex) == 40);
    assert(sizeof(gpu_workload_buffer) == 40 * VERTEX_BUFFER_SIZE);
    assert(offsetof(Vertex, x) == 0);
    assert(offsetof(Vertex, y) == 4);
    assert(offsetof(Vertex, uv) == 8);
    assert(offsetof(Vertex, RGBA) == 16);
    assert(offsetof(Vertex, lighting) == 32);
    assert(offsetof(Vertex, texture_i) == 36);
    glBufferData(
        GL_ARRAY_BUFFER,
        VERTEX_BUFFER_SIZE * sizeof(Vertex),
        gpu_workload_buffer,
        GL_DYNAMIC_DRAW);
    /*
    Attribute pointers describe the fields of our data
    sructure (the Vertex struct in shared/vertex_types.h)
    */
    
    // struct field: float x;
    glVertexAttribPointer(
        /* location (in shader source): */ 0,
        /* array/vector element count: */ 1,
        /* type of data: */ GL_FLOAT,
        /* normalize data: */ GL_FALSE,
        /* stride to next 'x': */ sizeof(Vertex),
        /* offset : */ (void*)(offsetof(Vertex, x)));
    // struct field; float y;
    glVertexAttribPointer(
        /* location (in shader source): */ 1,
        /* array/vector size: */ 1,
        /* type of data: */ GL_FLOAT,
        /* normalize data: */ GL_FALSE,
        /* sizeof parent struct: */ sizeof(Vertex),
        /* offset : */ (void*)(offsetof(Vertex, y)));
    // struct field; float uv[2];
    glVertexAttribPointer(
        /* location (in shader source): */ 2,
        /* array/vector size: */ 2,
        /* type of data: */ GL_FLOAT,
        /* normalize data: */ GL_FALSE,
        /* sizeof parent struct: */ sizeof(Vertex),
        /* offset : */ (void*)(offsetof(Vertex, uv)));
    // struct field: float RGBA[4];
    glVertexAttribPointer(
        /* location (in shader source): */ 3,
        /* array/vector size: */ 4,
        /* type of data: */ GL_FLOAT,
        /* normalize data: */ GL_FALSE,
        /* sizeof parent struct: */ sizeof(Vertex),
        /* offset : */ (void*)(offsetof(Vertex, RGBA)));
    // struct field: float lightning:
    glVertexAttribPointer(
        /* location (in shader source): */ 4,
        /* array/vector size: */ 1,
        /* type of data: */ GL_FLOAT,
        /* normalize data: */ GL_FALSE,
        /* sizeof parent struct: */ sizeof(Vertex),
        /* offset : */ (void*)(offsetof(Vertex, lighting)));
    // struct field: int32_t texture_i;
    glVertexAttribPointer(
        /* location (in shader source): */ 5,
        /* array/vector size: */ 1,
        /* type of data: */ GL_INT,
        /* normalize data: */ GL_FALSE,
        /* sizeof parent struct: */ sizeof(Vertex),
        /* offset : */ (void*)(offsetof(Vertex, texture_i)));
    
    glEnableVertexAttribArray(0);
    glEnableVertexAttribArray(1);
    glEnableVertexAttribArray(2);
    glEnableVertexAttribArray(3);
    glEnableVertexAttribArray(4);
    glEnableVertexAttribArray(5);
    
    printf("finished opengl_compile_shaders()...\n");
}

