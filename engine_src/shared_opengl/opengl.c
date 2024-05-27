#include "opengl.h"

GPUSharedDataCollection gpu_shared_data_collection;

// We'll need these 2 identifiers while drawing
GLuint program_id;
GLuint fragment_shader_id;
unsigned int VAO                       = UINT32_MAX;
unsigned int GPUPolygons_VBO_id        = UINT32_MAX;
unsigned int GPUVertex_VBO_id          = UINT32_MAX;
unsigned int GPULockedVertex_VBO_id    = UINT32_MAX;
unsigned int GPULightCollection_VBO_id = UINT32_MAX;
static GLuint err_value                = UINT32_MAX;

static GLuint texture_array_ids[TEXTUREARRAYS_SIZE];

static void opengl_copy_polygons(
    GPUPolygonCollection * polygon_collection)
{
    printf("copy polygons...\n");
    extptr_glBindBuffer(
        GL_SHADER_STORAGE_BUFFER,
        GPUPolygons_VBO_id);
    
    extptr_glBufferData(
        /* GLenum target: */
            GL_SHADER_STORAGE_BUFFER,
        /* GLsizeiptr size: */
            sizeof(GPUPolygon) * polygon_collection->size,
        /* const void * data: */
            polygon_collection->polygons,
        /* GLenum usage: */
            GL_STREAM_DRAW);
    // TODO: STREAM_DRAW is just my guess after reading docs
    // need to revisit after being more familiar with OpenGL
    
    // binding = 2 in the shader code
    extptr_glBindBufferBase(
        GL_SHADER_STORAGE_BUFFER,
        2,
        GPUPolygons_VBO_id);
    gpu_assert(glGetError() == 0);
}

static void opengl_copy_lights(
    GPULightCollection * light_collection)
{
    printf("copy lights...\n");
    extptr_glBindBuffer(
        GL_SHADER_STORAGE_BUFFER,
        GPULightCollection_VBO_id);
    
    // binding = 3 in the shader code
    extptr_glBindBufferBase(
        GL_SHADER_STORAGE_BUFFER,
        3,
        GPULightCollection_VBO_id);
    
    extptr_glBufferData(
        /* GLenum target: */
            GL_SHADER_STORAGE_BUFFER,
        /* GLsizeiptr size: */
            sizeof(GPULightCollection),
        /* const void * data: */
            light_collection,
        /* GLenum usage: */
            GL_STATIC_DRAW);
    // TODO: STATIC_DRAW is just my guess after reading docs
    // need to revisit after being more familiar with OpenGL
    
    #ifndef GPU_IGNORE_ASSERTS
    float doublechecks[10];
    doublechecks[0] = 12345;
    extptr_glGetBufferSubData(
        /* GLenum     target: */
            GL_SHADER_STORAGE_BUFFER,
        /* GLintptr  offset: */
            0,
        /* size_t     size: */
            sizeof(float) * 10,
        /* void *     data: */
            doublechecks);
    
    for (uint32_t light_i = 0; light_i < 10; light_i++)
    {
        gpu_assert(
            doublechecks[light_i] ==
                light_collection->light_x[light_i]);
    }
    gpu_assert(glGetError() == 0);
    #endif
    
    gpu_assert(glGetError() == 0);
}

static bool32_t opengl_copy_camera(
    GPUCamera * arg_camera)
{
    GLint loc = -1;
    
    gpu_assert(program_id == 1);
    extptr_glUseProgram(program_id);
    gpu_assert(glGetError() == 0);
    
    loc = extptr_glGetUniformLocation(
        program_id,
        "camera_xyz");
    if (loc != 30) {
        printf("camera_xyz uniform is not at location 30!\n");
        return false;
    }
    gpu_assert(glGetError() == 0);
    
    // there is also glProgramUniform3f, but that's not in OpenGL 3.0
    extptr_glUniform3fv(
        loc,
        1,
        arg_camera->xyz);
    
    #ifndef LOGGER_IGNORE_ASSERTS
    err_value = glGetError();
    if (err_value != 0) {
        printf("error trying to set arg_camera pos uniform\n");
        switch (err_value) {
            case GL_INVALID_VALUE:
                printf("%s\n", "GL_INVALID_VALUE");
                break;
            case GL_INVALID_ENUM:
                printf("%s\n", "GL_INVALID_ENUM");
                break;
            case GL_INVALID_OPERATION:
                printf("%s\n", "GL_INVALID_OPERATION");
                break;
            default:
                printf("%s\n", "unhandled!");
        }
        gpu_assert(0);
    }
    
    loc = extptr_glGetUniformLocation(
        program_id,
        "camera_xyz_angle");
    if (loc != 31) {
        printf("camera_xyz_angle uniform is not at location 31!\n");
        return false;
    }
    gpu_assert(glGetError() == 0);
    extptr_glUniform3fv(
        loc,
        1,
        camera.xyz_angle);
    gpu_assert(glGetError() == 0);
    
    float doublecheck_cam_angle[3];
    doublecheck_cam_angle[2] = 234.12f;
    extptr_glGetUniformfv(program_id, loc, doublecheck_cam_angle);
    gpu_assert(glGetError() == 0);
    gpu_assert(doublecheck_cam_angle[0] == arg_camera->xyz_angle[0]);
    gpu_assert(doublecheck_cam_angle[1] == arg_camera->xyz_angle[1]);
    gpu_assert(doublecheck_cam_angle[2] == arg_camera->xyz_angle[2]);
    #endif
    
    return true;
}

void platform_gpu_copy_locked_vertices(void)
{
    printf("%s\n", "Copy locked vertices...\n");
    
    extptr_glBindBuffer(
        GL_SHADER_STORAGE_BUFFER,
        GPULockedVertex_VBO_id);
    
    // binding = 4 in the shader code
    extptr_glBindBufferBase(
        GL_SHADER_STORAGE_BUFFER,
        4,
        GPULockedVertex_VBO_id);
    
    extptr_glBufferData(
        /* GLenum target: */
            GL_SHADER_STORAGE_BUFFER,
        /* GLsizeiptr size: */
            sizeof(GPULockedVertex) * ALL_LOCKED_VERTICES_SIZE,
        /* const void * data: */
            gpu_shared_data_collection.locked_vertices,
        /* GLenum usage: */
            GL_STATIC_DRAW);
    // TODO: STATIC_DRAW is just my guess after reading docs
    // need to revisit after being more familiar with OpenGL
    
    gpu_assert(glGetError() == 0);    
}

static void opengl_copy_vertices(
    GPUVertex * vertices,
    uint32_t vertices_size)
{
    printf("%s\n", "Copy vertices...\n");
    
    extptr_glBindBuffer(
        GL_ARRAY_BUFFER,
        GPUVertex_VBO_id);
    
    extptr_glBufferData(
        /* GLenum target: */
            GL_ARRAY_BUFFER,
        /* GLsizeiptr size: */
            sizeof(GPUVertex) * vertices_size,
        /* const void * data: */
            vertices,
        /* GLenum usage: */
            GL_STATIC_DRAW);
    
    #ifndef GPU_IGNORE_ASSERTS
    unsigned int doublechecks[10];
    doublechecks[0] = 12345;
    extptr_glGetBufferSubData(
        /* GLenum     target: */
            GL_ARRAY_BUFFER,
        /* GLintptr  offset: */
            0,
        /* size_t     size: */
            sizeof(float) * 6,
        /* void *     data: */
            doublechecks);
    
    for (uint32_t vert_i = 0; vert_i < 3; vert_i++)
    {
        gpu_assert(
            doublechecks[(vert_i*2) + 0] ==
                vertices[vert_i].locked_vertex_i);
        gpu_assert(
            doublechecks[(vert_i*2) + 1] ==
                vertices[vert_i].polygon_i);
    }
    
    gpu_assert(glGetError() == 0);
    #endif
}

void opengl_render_triangles(
    GPUDataForSingleFrame * frame_data)
{
    printf(
        "render %u triangles...\n",
        frame_data->vertices_size);
    
    gpu_assert(program_id < INT32_MAX);
    gpu_assert(VAO < UINT32_MAX);
    
    extptr_glUseProgram(program_id);
    gpu_assert(glGetError() == GL_NO_ERROR);
    
    extptr_glBindVertexArray(VAO);
    gpu_assert(glGetError() == GL_NO_ERROR);
    
    if (frame_data->vertices_size < 1) { return; }
    
    opengl_copy_vertices(
        frame_data->vertices,
        frame_data->vertices_size); 
    
    opengl_copy_polygons(frame_data->polygon_collection);
    
    opengl_copy_lights(frame_data->light_collection);
    
    bool32_t camera_success = opengl_copy_camera(frame_data->camera);
    if (!camera_success) {
        printf("ERROR - camera copy failed!\n");
        return;
    }
    
    #ifndef LOGGER_IGNORE_ASSERTS
    err_value = glGetError();
    if (err_value != GL_NO_ERROR) {
        switch (err_value) {
            case GL_INVALID_VALUE:
                printf(
                    "%s\n",
                    "GL_INVALID_VALUE");
                break;
            case GL_INVALID_ENUM:
                printf(
                    "%s\n",
                    "GL_INVALID_ENUM");
                break;
            case GL_INVALID_OPERATION:
                printf(
                    "%s\n",
                    "GL_INVALID_OPERATION");
                break;
            default:
                printf(
                    "%s\n",
                    "unhandled error when sending buffer data!");
                break;
        }
        gpu_assert(0);
    }
    #endif
    
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    
    // glPointSize(10); // for GL_POINTS
    glDrawArrays(
        /* GLenum mode: */
            GL_TRIANGLES,
        /* GLint first: */
            0,
        /* GLint count (# of vertices to render): */
            frame_data->vertices_size);
    
    #ifndef LOGGER_IGNORE_ASSERTS
    err_value = glGetError();
    if (err_value != GL_NO_ERROR) {
        switch (err_value) {
            case GL_INVALID_VALUE:
                printf(
                    "%s\n",
                    "GL_INVALID_VALUE");
                break;
            case GL_INVALID_ENUM:
                printf(
                    "%s\n",
                    "GL_INVALID_ENUM");
                break;
            case GL_INVALID_OPERATION:
                printf(
                    "%s\n",
                    "GL_INVALID_OPERATION");
                break;
            default:
                printf(
                    "%s\n",
                    "unhandled error when sending buffer data!");
                break;
        }
        gpu_assert(0);
    }
    #endif
}
 
static bool32_t opengl_compile_given_shader(
    GLuint shader_id,
    char * shader_source,
    GLint source_length)
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
    
    // Next, actually compile the shader
    printf("compile shader with id: %u\n", shader_id);
    extptr_glShaderSource(
        /* GLuint handle: */
            shader_id,
        /* shader count : */
            1,
        /* const GLchar ** string: */
            &shader_source,
        /* const GLint * length: */
            &source_length);
    gpu_assert(glGetError() == 0);
    
    extptr_glCompileShader(shader_id);
    gpu_assert(glGetError() == 0);
    
    GLint is_compiled = INT8_MAX;
    char info_log[512];
    info_log[0] = '\0';
    extptr_glGetShaderiv(
        /* GLuint id: */
            shader_id,
        /* GLenum pname: */
            GL_COMPILE_STATUS,
        /* GLint * params: */
            &is_compiled);
    
    if (is_compiled == GL_FALSE) {
        printf("Failed to compile shader:\n%s\n****\n", shader_source);
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
        return false;
    } else if (is_compiled == GL_TRUE) {
        
    } else {
        printf(
            "glGetShaderiv() returned an impossible value. "
            "Expected GL_TRUE (%i) or GL_FALSE (%i), "
            "got: %i\n",
            GL_TRUE,
            GL_FALSE,
            is_compiled);
        gpu_assert(0);
        return false;
    }

    return true;
}

void opengl_compile_shaders(
    char * vertex_shader_source,
    uint32_t vertex_shader_source_size,
    char * fragment_shader_source,
    uint32_t fragment_shader_source_size)
{
    printf("Compile shaders...\n");
    // allocate buffer memory...
    GLenum err_value;
    
    program_id = extptr_glCreateProgram();
    printf("Created program with program_id: %u\n", program_id);
    gpu_assert(program_id == 1);
    GLuint vertex_shader_id = extptr_glCreateShader(GL_VERTEX_SHADER);
    err_value = glGetError();
    gpu_assert(err_value == 0);
    
    gpu_assert(vertex_shader_source_size > 0);
    gpu_assert(vertex_shader_source != NULL);
    
    bool32_t shader_success = opengl_compile_given_shader(
        /* GLuint shader_id: */
            vertex_shader_id,
        /* char * shader_source: */
            vertex_shader_source,
        /* GLint source_length: */
            vertex_shader_source_size);
    err_value = glGetError();
    gpu_assert(err_value == 0);
    if (!shader_success) { return; }
    
    fragment_shader_id = extptr_glCreateShader(GL_FRAGMENT_SHADER);
    err_value = glGetError();
    gpu_assert(err_value == 0);
    gpu_assert(fragment_shader_source_size > 0);
    gpu_assert(fragment_shader_source != NULL);
    shader_success = opengl_compile_given_shader(
        /* GLuint shader_id: */
            fragment_shader_id,
        /* char * shader_source: */
            fragment_shader_source,
        /* GLint source_length: */
            fragment_shader_source_size);
    err_value = glGetError();
    gpu_assert(err_value == 0);
    if (!shader_success) { return; }
    
    // attach compiled shaders to program
    extptr_glAttachShader(program_id, vertex_shader_id);
    err_value = glGetError();
    gpu_assert(err_value == 0);
    extptr_glAttachShader(program_id, fragment_shader_id);
    err_value = glGetError();
    gpu_assert(err_value == 0);
    extptr_glLinkProgram(program_id);
    err_value = glGetError();
    gpu_assert(err_value == 0);
    
    /* 
    GL_LINK_STATUS
    params returns GL_TRUE if the last link operation on program was
    successful, and GL_FALSE otherwise.
    */
    GLint success = -1;
    extptr_glGetProgramiv(
        /* GLuint program id: */
            program_id,
        /* GLenum pname: */
            GL_LINK_STATUS,
        /* GLint * params: */
            &success);
    
    if (!success) {
        printf("glLinkProgram() failed.\n");
        err_value = glGetError();
        if (err_value != 0) {
            printf("%s\n", "opengl: bad link status");
            switch (err_value) {
                case GL_INVALID_VALUE:
                    printf("%s\n", "GL_INVALID_VALUE");
                    break;
                case GL_INVALID_ENUM:
                    printf("%s\n", "GL_INVALID_ENUM");
                    break;
                case GL_INVALID_OPERATION:
                    printf("%s\n", "GL_INVALID_OPERATION");
                    break;
                default:
                    printf("%s\n", "unhandled!");
            }
        }

        char errorlog[512];
        uint32_t errorlog_size = 0;
        memset(errorlog, 0, 512);
        extptr_glGetProgramInfoLog(
            /* GLuint program: */
                program_id,
            /* GLsizei maxLength: */
                512,
            /* GLsizei *length: */
                &errorlog_size,
            /* GLchar *infoLog: */
                errorlog);
        printf("errorlog: %s\n", errorlog);
        application_running = false;
        return;
    } else {
        printf("glLinkProgram() succeeded.\n");
    }
    
    extptr_glUseProgram(program_id);
    err_value = glGetError();
    if (err_value != 0) {
        printf("glUseProgram() failed.\n");
        application_running = false;
        return;
    }
    
    extptr_glGenVertexArrays(1, &VAO);
    err_value = glGetError();
    if (VAO >= INT16_MAX) {
        printf("VAO was not set by glGenVertexArrays\n");
        application_running = false;
        return;
    }
    gpu_assert(err_value == GL_NO_ERROR);
    
    extptr_glBindVertexArray(VAO);
    err_value = glGetError();
    gpu_assert(err_value == GL_NO_ERROR);
    
    // We didn't use the standard alpha channel in Metal, so it shouldn't
    // be enabled here
    // glEnable(GL_BLEND);
    // glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    
    // The depth buffer is so much easier here than with Metal.
    // On metal, we chose 32-bit floats
    // for depth, the test is LESS OR EQUAL,
    glEnable(GL_DEPTH_TEST);
    glClearDepth(50.0f);
    glDepthFunc(GL_LEQUAL); // on metal: LEQUAL
    
    glEnable(GL_CULL_FACE);
    glCullFace(GL_FRONT);
    
    extptr_glGenBuffers(1, &GPULightCollection_VBO_id);
    extptr_glGenBuffers(1, &GPUPolygons_VBO_id);
    extptr_glGenBuffers(1, &GPULockedVertex_VBO_id);
    
    extptr_glGenBuffers(1, &GPUVertex_VBO_id);
    extptr_glBindBuffer(
        GL_ARRAY_BUFFER,
        GPUVertex_VBO_id);
    
    /*
    Attribute pointers describe the fields of our 'in' data fields
    (we use them for the Vertex type in shared/cpu_gpu_shared_types.h)
    
    These attributes will be associated to GPUVertex_VBO_id and
    GL_ARRAY_BUFFER because those are currently bound by glBindBuffer
    */
    GLenum current_type = GL_INT;
    
    gpu_assert(current_type == GL_INT);
    /*
    This is another massive trap in OpenGL:
    You have to use glVertexAttribIPointer, not
    glVertexAttribPointer, when and only when setting up a
    vertex input that's an int. The other version also accepts
    GL_INT and the documentation claims that it can be used for
    ints, but it silently fails and garbles your int values to
    some huge value even if you set normalize data to GL_FALSE.
    */
    extptr_glVertexAttribIPointer(
        /* GLuint index (location in shader source): */
            0,
        /* GLint size (number of components per vertex, must be 1-4): */
            1,
        /* GLenum type (of data): */
            GL_INT,
        /* GLsizei stride; */
            sizeof(GPUVertex),
        /* const GLvoid * pointer (offset) : */
            (void *)(0 * sizeof(float)));
    err_value = glGetError();
    if (err_value != GL_NO_ERROR) {
        printf("Failed to set VertexAttribIPointer for location 0\n");
        switch (err_value) {
            case GL_INVALID_VALUE:
                printf("%s\n", "GL_INVALID_VALUE");
                break;
            case GL_INVALID_ENUM:
                printf("%s\n", "GL_INVALID_ENUM");
                break;
            case GL_INVALID_OPERATION:
                printf("%s\n", "GL_INVALID_OPERATION");
                break;
            default:
                printf("%s\n", "unhandled!");
        }
        gpu_assert(0);
    }
    
    extptr_glVertexAttribIPointer(
        /* GLuint index (location in shader source): */
            1,
        /* GLint size (number of components per vertex, must be 1-4): */
            1,
        /* GLenum type (of data): */
            GL_INT,
        /* GLsizei stride; */
            sizeof(GPUVertex),
        /* const GLvoid * pointer (offset) : */
            (void *)(1 * sizeof(float)));
    
    err_value = glGetError();
    if (err_value != GL_NO_ERROR) {
        printf("Failed to set VertexAttribIPointer for location 1\n");
        switch (err_value) {
            case GL_INVALID_VALUE:
                printf("%s\n", "GL_INVALID_VALUE");
                break;
            case GL_INVALID_ENUM:
                printf("%s\n", "GL_INVALID_ENUM");
                break;
            case GL_INVALID_OPERATION:
                printf(
                    "%s\n",
                    "GL_INVALID_OPERATION");
                GLint max_attribs = -1;
                extptr_glGetIntegerv(
                    GL_MAX_VERTEX_ATTRIBS,
                    &max_attribs);
                printf(
                    "GL_MAX_ATTRIBS was: %i\n",
                    max_attribs);
                break;
            default:
                printf("%s\n", "unhandled!");
        }
        gpu_assert(0);
    }
    
    
    extptr_glEnableVertexAttribArray(0);
    err_value = glGetError();
    if (err_value != GL_NO_ERROR) {
        switch (err_value) {
            case GL_INVALID_VALUE:
                printf("%s\n", "GL_INVALID_VALUE");
                break;
            case GL_INVALID_ENUM:
                printf("%s\n", "GL_INVALID_ENUM");
                break;
            case GL_INVALID_OPERATION:
                printf("%s\n", "GL_INVALID_OPERATION");
                break;
            default:
                printf("%s\n", "unhandled!");
        }
        gpu_assert(0);
    }
    
    extptr_glEnableVertexAttribArray(1);
    err_value = glGetError();
    if (err_value != GL_NO_ERROR) {
        switch (err_value) {
            case GL_INVALID_VALUE:
                printf("%s\n", "GL_INVALID_VALUE");
                break;
            case GL_INVALID_ENUM:
                printf("%s\n", "GL_INVALID_ENUM");
                break;
            case GL_INVALID_OPERATION:
                printf("%s\n", "GL_INVALID_OPERATION");
                break;
            default:
                printf("%s\n", "unhandled!");
        }
        gpu_assert(0);
    }
    
    // validate program
    success = 0;
    extptr_glValidateProgram(program_id);
    extptr_glGetProgramiv(program_id, GL_VALIDATE_STATUS, &success);
    gpu_assert(success);
    gpu_assert(glGetError() == 0);

    printf("Compiled shaders...\n");
}

/* reminder: this is mutex protected */
void platform_gpu_init_texture_array(
    const int32_t texture_array_i,
    const uint32_t num_images,
    const uint32_t single_image_width,
    const uint32_t single_image_height)
{
    // TODO: implement me!
    #if 0
    printf(
        "opengl must init texture array: %i with %u images (%ux%u)\n",
        texture_array_i,
        num_images,
        single_image_width,
        single_image_height);
    
    glGenTextures(1, &texture_array_ids[texture_array_i]);
    gpu_assert(glGetError() == 0);
    gpu_assert(texture_array_i < 31);
    
    glActiveTexture(
        GL_TEXTURE0 + texture_array_i);
    gpu_assert(glGetError() == 0);
    
    glBindTexture(
        GL_TEXTURE_2D_ARRAY,
        texture_array_ids[texture_array_i]);
    gpu_assert(glGetError() == 0);
    
    char name_in_shader[64];
    strcpy_capped(name_in_shader, 64, "texture_arrays[");
    strcat_uint_capped(name_in_shader, 64, texture_array_i);
    strcat_capped(name_in_shader, 64, "]");
    GLuint loc = extptr_glGetUniformLocation(
        program_id,
        name_in_shader);
    gpu_assert(glGetError() == 0);
    
    glUniform1iv(loc, 1, &texture_array_i);
    gpu_assert(glGetError() == 0);
    
    //There is also glTexStorage3D in openGL 4
    glTexImage3D(
        /* GLenum target: */
            GL_TEXTURE_2D_ARRAY,
        /* GLint level (mipmap, 0 is base): */
            0,
        /* GLint internalFormat: */
            GL_RGBA8,
        /* GLsizei width: */
            single_image_width,
        /* GLsizei height: */
            single_image_height,
        /* GLsizei depth: */
            num_images,
        /* GLint border (must be 0): */
            0,
        /* GLenum format: */
            GL_RGBA,
        /* GLenum type (of input data): */
            GL_UNSIGNED_BYTE,
        /* const GLvoid * data: */
            NULL);
    
    err_value = glGetError();
    if (err_value != GL_NO_ERROR) {
        printf("%s\n", "glTexImage3D failed!");
        switch (err_value) {
            case GL_INVALID_ENUM:
                printf("%s\n", "GL_INVALID_ENUM");
                break;
            default:
                printf("%s\n", "Unhandled error");
        }
        gpu_assert(err_value == GL_NO_ERROR);
    }
    
    glTexParameteri(
        /* GLenum target: */
            GL_TEXTURE_2D_ARRAY,
        /* GLenum pname: */
            GL_TEXTURE_WRAP_S,
        /* GLint param: */
            GL_CLAMP_TO_EDGE);
    gpu_assert(glGetError() == 0);
    
    glTexParameteri(
        /* GLenum target: */
            GL_TEXTURE_2D_ARRAY,
        /* GLenum pname: */
            GL_TEXTURE_WRAP_T,
        /* GLint param: */
            GL_CLAMP_TO_EDGE);
    gpu_assert(glGetError() == 0);
    
    glTexParameteri(
        /* GLenum target: */
            GL_TEXTURE_2D_ARRAY,
        /* GLenum pname: */
            GL_TEXTURE_MIN_FILTER,
        /* GLint param: */
            GL_NEAREST);
    gpu_assert(glGetError() == 0);
    
    glTexParameteri(
        /* GLenum target: */
            GL_TEXTURE_2D_ARRAY,
        /* GLenum pname: */
            GL_TEXTURE_MAG_FILTER,
        /* GLint param: */
            GL_NEAREST);
    gpu_assert(glGetError() == 0);
    #endif
}

/* reminder: this is mutex protected  */
void platform_gpu_push_texture_slice(
    const int32_t texture_array_i,
    const int32_t texture_i,
    const uint32_t parent_texture_array_images_size,
    const uint32_t image_width,
    const uint32_t image_height,
    const uint8_t * rgba_values)
{
    // TODO: implement me!
    #if 0
    printf(
        "opengl_push_texture(): %u to array: %u\n",
        texture_i,
        texture_array_i);
    gpu_assert(image_width > 0);
    gpu_assert(image_height > 0);
    gpu_assert(parent_texture_array_images_size > texture_i);
    
    glActiveTexture(
        GL_TEXTURE0 + texture_array_i);
    gpu_assert(glGetError() == 0);
    
    glBindTexture(
        GL_TEXTURE_2D_ARRAY,
        texture_array_ids[texture_array_i]);
    gpu_assert(glGetError() == 0);
   
    // GL_UNSIGNED_BYTE is correct, opengl just shows very different results
    // from metal when alpha blending / testing is not set up yet and you're
    // not discarding any fragments.
    glTexSubImage3D(
        /*
        GLenum target:
        Specifies the target texture.
        Must be GL_TEXTURE_3D or GL_TEXTURE_2D_ARRAY
        */
            GL_TEXTURE_2D_ARRAY,
        /*
        GLint level:
        Specifies the level-of-detail number. Level 0 is the base image level.
        Level n is the nth mipmap reduction image 
        */
            0,
        /*
        GLint xoffset:
        Specifies a texel offset in the x direction within the texture array
        */
            0,
        /*
        GLint yoffset:
        Specifies a texel offset in the y direction within the texture array
        */
            0,
        /*
        GLint zoffset:
        Specifies a texel offset in the z direction within the texture array.
        */
            texture_i,
        /*
        GLsizei width:
        Specifies the width of the texture subimage 
        */
            image_width,
        /*
        GLsizei height:
        Specifies the height of the texture subimage 
        */
            image_height,
        /*
        GLsizei depth:
        Specifies the depth of the texture subimage 
        */
            1,
        /*
        GLenum format:
        Specifies the format of the pixel data.
        The following symbolic values are accepted:
        GL_RED, GL_RG, GL_RGB, GL_BGR, GL_RGBA, and GL_BGRA 
        */
            GL_RGBA,
        /*
        GLenum type:
        Specifies the data type of the pixel data.
        The following symbolic values are accepted:
        GL_UNSIGNED_BYTE, GL_BYTE, GL_UNSIGNED_SHORT, GL_SHORT,
        GL_UNSIGNED_INT, GL_INT, GL_FLOAT, GL_UNSIGNED_BYTE_3_3_2,
        GL_UNSIGNED_BYTE_2_3_3_REV, GL_UNSIGNED_SHORT_5_6_5,
        GL_UNSIGNED_SHORT_5_6_5_REV, GL_UNSIGNED_SHORT_4_4_4_4,
        GL_UNSIGNED_SHORT_4_4_4_4_REV, GL_UNSIGNED_SHORT_5_5_5_1,
        GL_UNSIGNED_SHORT_1_5_5_5_REV, GL_UNSIGNED_INT_8_8_8_8,
        GL_UNSIGNED_INT_8_8_8_8_REV, GL_UNSIGNED_INT_10_10_10_2,
        GL_UNSIGNED_INT_2_10_10_10_REV
        */
            GL_UNSIGNED_BYTE,
        /*
        const GLvoid * data:
        Specifies a pointer to the image data in memory.
        */
            rgba_values);
    
    err_value = glGetError();
    if (err_value != GL_NO_ERROR) {
        printf("%s\n", "Error during glTexSubImage3D!");
        switch (err_value) {
            case GL_INVALID_VALUE:
                printf("%s\n", "GL_INVALID_VALUE");
                break;
            case GL_INVALID_ENUM:
                printf("%s\n", "GL_INVALID_ENUM");
                break;
            case GL_INVALID_OPERATION:
                printf("%s\n", "GL_INVALID_OPERATION");
                break;
            default:
                printf("%s\n", "unhandled error at start of frame!");
                break;
        }
        gpu_assert(0);
    }
    #endif
}

void opengl_set_projection_constants(
    GPUProjectionConstants * pjc)
{
    #if 0
    gpu_assert(pjc != NULL);
    extptr_glUseProgram(program_id);
    
    // set viewport
    // these args might be getting clamped
    // to 0 - 1, I wish opengl would just
    // throw instead of silently clamping
    gpu_assert(pjc->znear > 0.005f);
    gpu_assert(pjc->zfar < 11.0f);
    glDepthRangef(
        /* GLfloat nearVal: */ 
            pjc->znear,
        /* GLfloat farVal: */
            pjc->zfar);
    
    glFrustum(
        0, 1,
        0, 1,
        pjc->znear,
        pjc->zfar);
    
    GLint loc = -1;
    
    loc = extptr_glGetUniformLocation(
        program_id,
        "projection_constants_near");
    gpu_assert(glGetError() == 0);
    gpu_assert(glGetError() == 0);
    // reminder: don't use glUniform1f, it's buggy on ubuntu with some
    // drivers, use glUniform1fv instead
    glUniform1fv(loc, 1, &pjc->znear);
    gpu_assert(glGetError() == 0);
    
    loc = -1;
    loc = extptr_glGetUniformLocation(
        program_id,
        "projection_constants_q");
    gpu_assert(glGetError() == 0);
    // reminder: don't use glUniform1f, it's buggy on ubuntu with some
    // drivers, use glUniform1fv instead
    glUniform1fv(loc, 1, &pjc->q);
    gpu_assert(glGetError() == 0);
    
    loc = -1;
    loc = extptr_glGetUniformLocation(
        program_id,
        "projection_constants_fov_modifier");
    // reminder: don't use glUniform1f, it's buggy on ubuntu with some
    // drivers, use glUniform1fv instead
    glUniform1fv(loc, 1, &pjc->field_of_view_modifier);
    gpu_assert(glGetError() == 0);
    
    loc = -1;
    loc = extptr_glGetUniformLocation(
        program_id,
        "projection_constants_x_multiplier");
    // reminder: don't use glUniform1f, it's buggy on ubuntu with some
    // drivers, use glUniform1fv instead
    glUniform1fv(loc, 1, &pjc->x_multiplier);
    gpu_assert(glGetError() == 0);
    #endif
}

void platform_gpu_update_viewport(void) {
    // TODO: implement me!
}

void shadersource_apply_macro_inplace(
    char * shader_source,
    char * to_replace,
    char * replacement)
{
    uint32_t i = 0;
    
    uint32_t to_replace_len  = get_string_length(to_replace);
    uint32_t replacement_len = get_string_length(replacement);
    
    log_assert(replacement_len <= to_replace_len);
    
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

