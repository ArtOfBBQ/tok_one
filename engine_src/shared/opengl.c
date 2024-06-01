#include "opengl.h"

static unsigned int VAO;
static GLuint program_id;

static unsigned int vertex_VBO; // vertex array buffer
static unsigned int camera_VBO; // binding 2
static unsigned int polygons_VBO; // binding 3
static unsigned int lights_VBO;
static unsigned int locked_vertices_VBO; // binding 4
static unsigned int projection_constants_VBO; // binding 5

void shadersource_apply_macro_inplace(
    char * shader_source,
    char * to_replace,
    char * replacement)
{
    uint32_t i = 0;
    
    uint32_t to_replace_len  = get_string_length(to_replace);
    uint32_t replacement_len = get_string_length(replacement);
    
    tok_assert(replacement_len <= to_replace_len);
    
    uint32_t padding_spaces = to_replace_len - replacement_len;
    
    while (shader_source[i] != '\0') {
        bool32_t match = true;
        for (uint32_t j = 0; j < to_replace_len; j++) {
            if (
                shader_source[i + j] != to_replace[j])
            {
                match = false;
            }
        }
    
        if (match) {
            for (uint32_t j = 0; j < padding_spaces; j++) {
                shader_source[i+j] = ' ';
            }
            i += padding_spaces;
            
            for (uint32_t j = 0; j < replacement_len; j++) {
                shader_source[i+j] = replacement[j];
            }
            i += replacement_len;
        }
        
        i++;
    }
}

static void opengl_compile_shader(
    char * shader_source,
    GLenum SHADER_ENUM_TYPE)
{
    // First, manually apply macros
    char replacement[128];
    replacement[0] = '\0';
    strcat_uint_capped(replacement, 128, MAX_POLYGONS_PER_BUFFER);
    shadersource_apply_macro_inplace(
        /* char * shader_source: */
            shader_source,
        /* char * to_replace: */
            "MAX_POLYGONS_PER_BUFFER",
        /* char * replacement: */
            replacement);
    replacement[0] = '\0';
    strcat_uint_capped(replacement, 128, MAX_LIGHTS_PER_BUFFER);
    shadersource_apply_macro_inplace(
        /* char * shader_source: */
            shader_source,
        /* char * to_replace: */
            "MAX_LIGHTS_PER_BUFFER",
        /* char * replacement: */
            replacement);
    replacement[0] = '\0';
    strcat_uint_capped(replacement, 128, MAX_VERTICES_PER_BUFFER);
    shadersource_apply_macro_inplace(
        /* char * shader_source: */
            shader_source,
        /* char * to_replace: */
            "MAX_VERTICES_PER_BUFFER",
        /* char * replacement: */
            replacement);
    replacement[0] = '\0';
    strcat_uint_capped(replacement, 128, ALL_LOCKED_VERTICES_SIZE);
    shadersource_apply_macro_inplace(
        /* char * shader_source: */
            shader_source,
        /* char * to_replace: */
            "ALL_LOCKED_VERTICES_SIZE",
        /* char * replacement: */
            replacement);
    replacement[0] = '\0';
    strcat_uint_capped(replacement, 128, MAX_MATERIALS_PER_POLYGON);
    shadersource_apply_macro_inplace(
        /* char * shader_source: */
            shader_source,
        /* char * to_replace: */
            "MAX_MATERIALS_PER_POLYGON",
        /* char * replacement: */
            replacement);
   
    
    tok_assert(!glGetError());
    GLuint shader_id = extptr_glCreateShader(SHADER_ENUM_TYPE);
    tok_assert(!glGetError());
    char * shader_source_as_ptr = shader_source;
    extptr_glShaderSource(
        shader_id,
        1,
        &shader_source_as_ptr,
        NULL);
    
    tok_assert(!glGetError());
    extptr_glCompileShader(shader_id);
    GLint is_compiled = INT8_MAX;
    info_log[0] = '\0';
    extptr_glGetShaderiv(
        /* GLuint id: */
            shader_id,
        /* GLenum pname: */
            GL_COMPILE_STATUS,
        /* GLint * params: */
            &is_compiled);
    
    if (is_compiled == GL_FALSE) {
        printf(
            "Failed to compile shader:\n%s\n****\n",
            shader_source);
        GLenum err_value = glGetError();
        
        extptr_glGetShaderInfoLog(
            /* GLuint shader id: */
                shader_id,
            /* GLsizei max length: */
                512,
            /* GLsizei * length: */
                NULL,
            /* GLchar * infolog: */
                info_log);
        
        printf("info_log: %s\n", info_log);
    }
    
    tok_assert(!glGetError());
    
    extptr_glAttachShader(program_id, shader_id);
    tok_assert(!glGetError());
}

void opengl_copy_projection_constants(
    GPUProjectionConstants * projection_constants)
{
    tok_assert(!glGetError());
    extptr_glGenBuffers(1, &projection_constants_VBO);
    
    tok_assert(!glGetError());
    extptr_glBindBuffer(
        GL_SHADER_STORAGE_BUFFER,
        projection_constants_VBO);
    
    tok_assert(!glGetError());
    extptr_glBindBufferBase(
        GL_SHADER_STORAGE_BUFFER,
        5,
        projection_constants_VBO);
    
    extptr_glBufferData(
        GL_SHADER_STORAGE_BUFFER,
        sizeof(GPUProjectionConstants),
        projection_constants,
        GL_STATIC_DRAW);
    extptr_glBindBuffer(GL_SHADER_STORAGE_BUFFER, 0);
}

void opengl_copy_locked_vertices(
    GPULockedVertex * locked_vertices)
{
    extptr_glBindBuffer(GL_SHADER_STORAGE_BUFFER, locked_vertices_VBO);
    extptr_glBindBufferBase(
        GL_SHADER_STORAGE_BUFFER,
        4,
        locked_vertices_VBO);
    extptr_glBufferData(
        GL_SHADER_STORAGE_BUFFER,
        sizeof(GPULockedVertex) * ALL_LOCKED_VERTICES_SIZE,
        locked_vertices,
        GL_STATIC_DRAW);
    
    extptr_glBindBuffer(GL_SHADER_STORAGE_BUFFER, 0);
}

static void copy_single_frame_data(
    GPUDataForSingleFrame * frame_data)
{
    extptr_glBindBuffer(GL_ARRAY_BUFFER, vertex_VBO);
    extptr_glBufferData(
        GL_ARRAY_BUFFER,
        sizeof(GPUVertex) * frame_data->vertices_size,
        frame_data->vertices,
        GL_STATIC_DRAW);
    extptr_glBindBuffer(GL_ARRAY_BUFFER, 0);
    
    extptr_glBindBuffer(GL_SHADER_STORAGE_BUFFER, camera_VBO);
    extptr_glBufferData(
        GL_SHADER_STORAGE_BUFFER,
        sizeof(GPUCamera),
        frame_data->camera,
        GL_STATIC_DRAW);
    tok_assert(!glGetError());
    extptr_glBindBuffer(GL_SHADER_STORAGE_BUFFER, 0);
    
    extptr_glBindBuffer(GL_SHADER_STORAGE_BUFFER, polygons_VBO);
    uint32_t polygons_size = frame_data->polygon_collection->size;
    if (polygons_size < 5) { polygons_size = 5; }
    extptr_glBufferData(
        GL_SHADER_STORAGE_BUFFER,
        sizeof(GPUPolygon) * frame_data->polygon_collection->size,
        frame_data->polygon_collection->polygons,
        GL_STATIC_DRAW);
    extptr_glBindBuffer(GL_SHADER_STORAGE_BUFFER, 0);    

    extptr_glBindBuffer(GL_SHADER_STORAGE_BUFFER, lights_VBO);
    extptr_glBufferData(
        GL_SHADER_STORAGE_BUFFER,
        sizeof(GPULightCollection),
        frame_data->light_collection,
        GL_STATIC_DRAW);
    tok_assert(!glGetError());
    extptr_glBindBuffer(GL_SHADER_STORAGE_BUFFER, 0);
}

void opengl_init(
    char * vertex_shader_source,
    char * fragment_shader_source)
{
    tok_assert(!glGetError());
    program_id = extptr_glCreateProgram();
    printf("Created program_id: %u\n", program_id);
    tok_assert(program_id > 0);
    tok_assert(!glGetError());
    
    tok_assert(!glGetError());
    extptr_glGenVertexArrays(1, &VAO);
    extptr_glBindVertexArray(VAO);
    tok_assert(!glGetError());
    extptr_glGenBuffers(1, &vertex_VBO);
    extptr_glBindBuffer(GL_ARRAY_BUFFER, vertex_VBO);
    extptr_glVertexAttribIPointer(
        /* GLuint index: (of the vertex input) */
            0,
        /* GLint size: (3 for vec3, 4 for vec4 etc.) */
            2,
        /* GLenum type: */
            GL_UNSIGNED_INT,
        /* GLsizei stride: (0 means tightly packed) */
            2 * sizeof(unsigned int),
        /* const void * pointer (offset to 1st element): */
            0);
    tok_assert(!glGetError());
    extptr_glEnableVertexAttribArray(0);
    extptr_glBindBuffer(GL_ARRAY_BUFFER, 0);
    
    tok_assert(!glGetError());
    extptr_glGenBuffers(1, &camera_VBO);
    extptr_glBindBuffer(GL_SHADER_STORAGE_BUFFER, camera_VBO);
    extptr_glBindBufferBase(
        GL_SHADER_STORAGE_BUFFER,
        2,
        camera_VBO);
    extptr_glBufferData(
        GL_SHADER_STORAGE_BUFFER,
        sizeof(GPUCamera),
        0,
        GL_STATIC_DRAW);
    extptr_glBindBuffer(GL_SHADER_STORAGE_BUFFER, 0);
    
    tok_assert(!glGetError());
    extptr_glGenBuffers(1, &polygons_VBO);
    extptr_glBindBuffer(GL_SHADER_STORAGE_BUFFER, polygons_VBO);
    extptr_glBindBufferBase(
        GL_SHADER_STORAGE_BUFFER,
        3,
        polygons_VBO);
    extptr_glBufferData(
        GL_SHADER_STORAGE_BUFFER,
        MAX_POLYGONS_PER_BUFFER * sizeof(GPUPolygon),
        0,
        GL_STATIC_DRAW);
    extptr_glBindBuffer(GL_SHADER_STORAGE_BUFFER, 0);
    
    tok_assert(!glGetError());
    extptr_glGenBuffers(1, &locked_vertices_VBO);
    tok_assert(!glGetError());
    extptr_glBindBuffer(GL_SHADER_STORAGE_BUFFER, locked_vertices_VBO);
    tok_assert(!glGetError());
    extptr_glBindBufferBase(
        GL_SHADER_STORAGE_BUFFER,
        4,
        locked_vertices_VBO);
    extptr_glBufferData(
        GL_SHADER_STORAGE_BUFFER,
        ALL_LOCKED_VERTICES_SIZE * sizeof(GPULockedVertex),
        0,
        GL_STATIC_DRAW);
    extptr_glBindBuffer(GL_SHADER_STORAGE_BUFFER, 0);
    
    tok_assert(!glGetError());
    extptr_glGenBuffers(1, &projection_constants_VBO);
    tok_assert(!glGetError());
    extptr_glBindBuffer(
        GL_SHADER_STORAGE_BUFFER,
        projection_constants_VBO);
    tok_assert(!glGetError());
    extptr_glBindBufferBase(
        GL_SHADER_STORAGE_BUFFER,
        5,
        projection_constants_VBO);
    extptr_glBufferData(
        GL_SHADER_STORAGE_BUFFER,
        sizeof(GPUProjectionConstants),
        0,
        GL_STATIC_DRAW);
    extptr_glBindBuffer(GL_SHADER_STORAGE_BUFFER, 0);

    tok_assert(!glGetError());
    extptr_glGenBuffers(1, &lights_VBO);
    tok_assert(!glGetError());
    extptr_glBindBuffer(
        GL_SHADER_STORAGE_BUFFER,
        lights_VBO);
    tok_assert(!glGetError());
    extptr_glBindBufferBase(
        GL_SHADER_STORAGE_BUFFER,
        6,
        lights_VBO);
    extptr_glBufferData(
        GL_SHADER_STORAGE_BUFFER,
        sizeof(GPULightCollection),
        0,
        GL_STATIC_DRAW);
    extptr_glBindBuffer(GL_SHADER_STORAGE_BUFFER, 0);
    
    opengl_compile_shader(
        vertex_shader_source,
        GL_VERTEX_SHADER);
    opengl_compile_shader(
        fragment_shader_source,
        GL_FRAGMENT_SHADER);
    
    extptr_glLinkProgram(program_id);
    tok_assert(!glGetError());
    
    unsigned int success = 0;
    extptr_glGetProgramiv(program_id, GL_LINK_STATUS, &success);
    if (!success) {
        extptr_glGetProgramInfoLog(program_id, 512, NULL, info_log);
        printf("ERROR - GL_LINK_STATUS: %s\n", info_log);
        getchar();
        return;
    }    
}

void opengl_render_frame(GPUDataForSingleFrame * frame)
{ 
    extptr_glUseProgram(program_id);
    // extptr_glBindVertexArray(VAO);
    GLint error = glGetError();
    if (error != 0) {
        printf("glUseProgram threw error: %i\n", error);
        getchar();
        return;
    }
    
    glClearColor(0.0f, 0.2f, 0.3f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT);
    
    glDepthRange(0.01, 100.0);
    
    // glUseProgram(program_id); // TODO: reactivate if needed?
    copy_single_frame_data(frame);
    
    glDrawArrays(
        /* GLenum	mode : */
            GL_TRIANGLES,
        /* GLint	first: */
            0,
        /* GLsizei	count: */
            3);
    
    assert(!glGetError());
}

