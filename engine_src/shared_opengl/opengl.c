#include "opengl.h"

// We'll need these 2 identifiers while drawing
GLuint program_id;
GLuint fragment_shader_id;
unsigned int VAO = UINT32_MAX;
unsigned int VBO = UINT32_MAX;
static GLuint err_value = UINT32_MAX;

static GLuint texture_array_ids[TEXTUREARRAYS_SIZE];

#ifndef LOGGER_IGNORE_ASSERTS
static void spotcheck_uniform(
    char * uniform_name,
    float expected_value)
{
    GLint loc = glGetUniformLocation(
        program_id,
        uniform_name);
    assert(glGetError() == 0);
    
    if (loc < 0) {
        printf(
            "error on spot check, variable %s returns location %i\n",
            uniform_name,
            loc);
        assert(loc >= 0);
    }
    
    GLfloat actual_gpu_value;
    actual_gpu_value = 23.555f;
    glGetUniformfv(
        program_id,
        loc,
        &actual_gpu_value);
    err_value = glGetError();
    if (err_value != 0) {
        printf("error trying to fetch uniform for spot-checking\n");
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
        printf(
            "uniform name: %s, at location: %i\n",
            uniform_name,
            loc);
        assert(0);
    }
    if (actual_gpu_value != expected_value) {
        printf(
            "uniform %s's actual gpu value was %f, expected: %f\n",
            uniform_name,
            actual_gpu_value,
            expected_value);
        assert(0);
    }
}
#endif

static void opengl_set_polygons(
    GPUPolygonCollection * polygon_collection)
{
    /*
    Reminder: If MAX_POLYGONS_PER_BUFFER gets updated,
    you need to update the glsl vertex shader
    */
    assert(MAX_POLYGONS_PER_BUFFER == 1000);
    
    GLint loc = glGetUniformLocation(
        program_id,
        "zpolygons_x");
    assert(glGetError() == 0);
    glUniform1fv(loc, MAX_POLYGONS_PER_BUFFER, polygon_collection->x);
    assert(glGetError() == 0);
    #ifndef LOGGER_IGNORE_ASSERTS
    spotcheck_uniform(
        "zpolygons_x[0]",
        polygon_collection->x[0]);
    spotcheck_uniform(
        "zpolygons_x[8]",
        polygon_collection->x[8]);
    #endif
    
    loc = glGetUniformLocation(
        program_id,
        "zpolygons_y");
    assert(glGetError() == 0);
    glUniform1fv(loc, MAX_POLYGONS_PER_BUFFER, polygon_collection->y);
    assert(glGetError() == 0);
    #ifndef LOGGER_IGNORE_ASSERTS
    spotcheck_uniform(
        "zpolygons_y[2]",
        polygon_collection->y[2]);
    spotcheck_uniform(
        "zpolygons_y[5]",
        polygon_collection->y[5]);
    #endif
    
    loc = glGetUniformLocation(
        program_id,
        "zpolygons_z");
    assert(glGetError() == 0);
    glUniform1fv(loc, MAX_POLYGONS_PER_BUFFER, polygon_collection->z);
    assert(glGetError() == 0);
    
    loc = glGetUniformLocation(
        program_id,
        "zpolygons_x_angle");
    assert(glGetError() == 0);
    glUniform1fv(loc, MAX_POLYGONS_PER_BUFFER, polygon_collection->x_angle);
    assert(glGetError() == 0);
    
    loc = glGetUniformLocation(
        program_id,
        "zpolygons_y_angle");
    assert(glGetError() == 0);
    glUniform1fv(loc, MAX_POLYGONS_PER_BUFFER, polygon_collection->y_angle);
    assert(glGetError() == 0);
    #ifndef LOGGER_IGNORE_ASSERTS
    spotcheck_uniform(
        "zpolygons_y_angle[2]",
        polygon_collection->y_angle[2]);
    spotcheck_uniform(
        "zpolygons_y_angle[5]",
        polygon_collection->y_angle[5]);
    #endif
    
    loc = glGetUniformLocation(
        program_id,
        "zpolygons_z_angle");
    assert(glGetError() == 0);
    glUniform1fv(loc, MAX_POLYGONS_PER_BUFFER, polygon_collection->z_angle);
    assert(glGetError() == 0);
    
    loc = glGetUniformLocation(
        program_id,
        "zpolygons_scale_factor");
    assert(glGetError() == 0);
    glUniform1fv(
        loc,
        MAX_POLYGONS_PER_BUFFER,
        polygon_collection->scale_factor);
    assert(glGetError() == 0);
    
    loc = glGetUniformLocation(
        program_id,
        "zpolygons_ignore_lighting");
    assert(glGetError() == 0);
    glUniform1fv(
        loc,
        MAX_POLYGONS_PER_BUFFER,
        polygon_collection->ignore_lighting);
    assert(glGetError() == 0);
    
    loc = glGetUniformLocation(
        program_id,
        "zpolygons_ignore_camera");
    assert(glGetError() == 0);
    glUniform1fv(
        loc,
        MAX_POLYGONS_PER_BUFFER,
        polygon_collection->ignore_camera);
    assert(glGetError() == 0);
}

static void opengl_set_lights(
    GPULightCollection * light_collection)
{
    /*
    Reminder: If MAX_LIGHTS_PER_BUFFER gets updated,
    you need to update the glsl vertex shader
    */
    assert(MAX_LIGHTS_PER_BUFFER == 75);
    
    GLint loc = glGetUniformLocation(
        program_id,
        "lights_x");
    assert(glGetError() == 0);
    glUniform1fv(loc, MAX_LIGHTS_PER_BUFFER, light_collection->light_x);
    assert(glGetError() == 0);
    #ifndef LOGGER_IGNORE_ASSERTS
    spotcheck_uniform(
        "lights_x[0]",
        light_collection->light_x[0]);
    spotcheck_uniform(
        "lights_x[8]",
        light_collection->light_x[8]);
    spotcheck_uniform(
        "lights_x[12]",
        light_collection->light_x[12]);
    #endif
    
    loc = glGetUniformLocation(
        program_id,
        "lights_y");
    assert(glGetError() == 0);
    glUniform1fv(loc, MAX_LIGHTS_PER_BUFFER, light_collection->light_y);
    assert(glGetError() == 0);
    #ifndef LOGGER_IGNORE_ASSERTS
    spotcheck_uniform(
        "lights_y[0]",
        light_collection->light_y[0]);
    spotcheck_uniform(
        "lights_y[1]",
        light_collection->light_y[1]);
    #endif
    
    loc = glGetUniformLocation(
        program_id,
        "lights_z");
    assert(glGetError() == 0);
    glUniform1fv(loc, MAX_LIGHTS_PER_BUFFER, light_collection->light_z);
    assert(glGetError() == 0);
    #ifndef LOGGER_IGNORE_ASSERTS
    spotcheck_uniform(
        "lights_z[1]",
        light_collection->light_z[1]);
    #endif
    
    loc = glGetUniformLocation(
        program_id,
        "lights_ambient");
    assert(glGetError() == 0);
    glUniform1fv(loc, MAX_LIGHTS_PER_BUFFER, light_collection->ambient);
    assert(glGetError() == 0);
    #ifndef LOGGER_IGNORE_ASSERTS
    spotcheck_uniform(
        "lights_ambient[5]",
        light_collection->ambient[5]);
    #endif
    
    loc = glGetUniformLocation(
        program_id,
        "lights_diffuse");
    assert(glGetError() == 0);
    assert(loc >= 0);
    glUniform1fv(loc, MAX_LIGHTS_PER_BUFFER, light_collection->diffuse);
    assert(glGetError() == 0);
    #ifndef LOGGER_IGNORE_ASSERTS
    spotcheck_uniform(
        "lights_diffuse[0]",
        light_collection->diffuse[0]);
    spotcheck_uniform(
        "lights_diffuse[7]",
        light_collection->diffuse[7]);
    #endif
    
    loc = glGetUniformLocation(
        program_id,
        "lights_reach");
    assert(glGetError() == 0);
    glUniform1fv(loc, MAX_LIGHTS_PER_BUFFER, light_collection->reach);
    assert(glGetError() == 0);
    #ifndef LOGGER_IGNORE_ASSERTS
    spotcheck_uniform(
        "lights_reach[4]",
        light_collection->reach[4]);
    spotcheck_uniform(
        "lights_reach[14]",
        light_collection->reach[14]);
    #endif
    
    loc = glGetUniformLocation(
        program_id,
        "lights_red");
    assert(glGetError() == 0);
    assert(loc >= 0);
    glUniform1fv(
        loc,
        MAX_LIGHTS_PER_BUFFER,
        light_collection->red);
    assert(glGetError() == 0);
    #ifndef LOGGER_IGNORE_ASSERTS
    spotcheck_uniform(
        "lights_red[0]",
        light_collection->red[0]);
    spotcheck_uniform(
        "lights_red[4]",
        light_collection->red[4]);
    #endif
    
    loc = glGetUniformLocation(
        program_id,
        "lights_green");
    assert(glGetError() == 0);
    assert(loc >= 0);
    glUniform1fv(
        loc,
        MAX_LIGHTS_PER_BUFFER,
        light_collection->green);
    assert(glGetError() == 0);
    #ifndef LOGGER_IGNORE_ASSERTS
    spotcheck_uniform(
        "lights_green[0]",
        light_collection->green[0]);
    spotcheck_uniform(
        "lights_green[1]",
        light_collection->green[1]);
    spotcheck_uniform(
        "lights_green[4]",
        light_collection->green[4]);
    spotcheck_uniform(
        "lights_green[74]",
        light_collection->green[74]);
    #endif
    
    loc = glGetUniformLocation(
        program_id,
        "lights_blue");
    assert(glGetError() == 0);
    assert(loc >= 0);
    glUniform1fv(
        loc,
        MAX_LIGHTS_PER_BUFFER,
        light_collection->blue);
    assert(glGetError() == 0);
    #ifndef LOGGER_IGNORE_ASSERTS
    spotcheck_uniform(
        "lights_blue[0]",
        light_collection->blue[0]);
    spotcheck_uniform(
        "lights_blue[1]",
        light_collection->blue[1]);
    spotcheck_uniform(
        "lights_blue[4]",
        light_collection->blue[4]);
    spotcheck_uniform(
        "lights_blue[74]",
        light_collection->blue[74]);
    #endif
    
    loc = glGetUniformLocation(
        program_id,
        "lights_size");
    assert(glGetError() == 0);
    int size[1];
    size[0] = (int32_t)light_collection->lights_size;
    glUniform1iv(
        loc,
        1,
        &size);
    assert(glGetError() == 0);
    printf("lights size: %i\n", (int32_t)light_collection->lights_size);
    
    #ifndef LOGGER_IGNORE_ASSERTS
    int doublecheck_lights_size;
    doublecheck_lights_size = 234;
    glGetUniformiv(program_id, loc, &doublecheck_lights_size);
    assert(glGetError() == 0);
    assert(doublecheck_lights_size == light_collection->lights_size);
    #endif
}

static void opengl_set_camera(
    GPUCamera * arg_camera)
{
    GLint loc = -1;
    
    assert(program_id == 1);
    glUseProgram(program_id);
    assert(glGetError() == 0);
    
    loc = glGetUniformLocation(
        program_id,
        "camera_position");
    assert(glGetError() == 0);
    // there is also glProgramUniform3f, but that's not in OpenGL 3.0
    // We are using 'glUseProgram' explicitly here and before this so
    // I don't think that can be the problem
    float cam_pos[3];
    cam_pos[0] = arg_camera->x;
    cam_pos[1] = arg_camera->y;
    cam_pos[2] = arg_camera->z;
    glUniform3fv(
        loc,
        1,
        cam_pos);
    
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
        assert(0);
    }
    
    loc = glGetUniformLocation(
        program_id,
        "camera_angle");
    assert(glGetError() == 0);
    float cam_angle[3];
    cam_angle[0] = arg_camera->x_angle;
    cam_angle[1] = arg_camera->y_angle;
    cam_angle[2] = arg_camera->z_angle;
    glUniform3fv(
        loc,
        1,
        cam_angle);
    assert(glGetError() == 0);
    
    float doublecheck_cam_angle[3];
    doublecheck_cam_angle[2] = 234.12f;
    glGetUniformfv(program_id, loc, doublecheck_cam_angle);
    assert(glGetError() == 0);
    assert(doublecheck_cam_angle[0] == arg_camera->x_angle);
    // assert(doublecheck_cam_angle[1] == arg_camera->y_angle);
    // assert(doublecheck_cam_angle[2] == arg_camera->z_angle);
}

void opengl_render_triangles(GPUDataForSingleFrame * frame_data) {
    
    assert(VAO < UINT32_MAX);
    assert(VBO < UINT32_MAX);
    
    glUseProgram(program_id);
    
    if (frame_data->vertices_size < 1) { return; }

    #ifndef LOGGER_IGNORE_ASSERTS
    assert(frame_data->vertices != NULL); 
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
                printf("%s\n", "unhandled error at start of frame!");
                break;
        }
        assert(0);
    }
    #endif

    #ifndef LOGGER_IGNORE_ASSERTS
    for (uint32_t i = 0; i < frame_data->vertices_size; i++) {
        if (
            frame_data->vertices[i].polygon_i < 0 ||
            frame_data->vertices[i].polygon_i >=
                frame_data->polygon_collection->size)
        {
            printf(
                "invalid polygon_i: %u\n",
                frame_data->vertices[i].polygon_i);
            assert(frame_data->vertices[i].polygon_i >= 0);
            assert(
                frame_data->vertices[i].polygon_i <
                    frame_data->polygon_collection->size);
        }
    }
    #endif
    
    glBufferData(
        /* target: */
            GL_ARRAY_BUFFER,
        /* size_in_bytes: */
            sizeof(GPUVertex) * frame_data->vertices_size,
        /* const GLvoid * data: (to init with, or NULL to copy no data) */
            (const GLvoid *)frame_data->vertices,
        /* usage: */
            GL_DYNAMIC_DRAW);
    
    #ifndef LOGGER_IGNORE_ASSERTS
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
                printf("%s\n", "unhandled error when sending buffer data!");
                break;
        }
        assert(0);
    }
    #endif
    
    opengl_set_polygons(frame_data->polygon_collection);
    #ifndef LOGGER_IGNORE_ASSERTS
    err_value = glGetError();
    #endif
    
    opengl_set_lights(frame_data->light_collection);
    #ifndef LOGGER_IGNORE_ASSERTS
    err_value = glGetError();
    #endif
    
    opengl_set_camera(frame_data->camera);
    #ifndef LOGGER_IGNORE_ASSERTS
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
                printf("%s\n", "unhandled error when sending buffer data!");
                break;
        }
        assert(0);
    }
    #endif
    
    // opengl_set_projection_constants(frame_data->projection_constants); 
    // err_value = glGetError();
    // if (err_value != GL_NO_ERROR) {
    //     switch (err_value) {
    //         case GL_INVALID_VALUE:
    //             printf("%s\n", "GL_INVALID_VALUE");
    //             break;
    //         case GL_INVALID_ENUM:
    //             printf("%s\n", "GL_INVALID_ENUM");
    //             break;
    //         case GL_INVALID_OPERATION:
    //             printf("%s\n", "GL_INVALID_OPERATION");
    //             break;
    //         default:
    //             printf("%s\n", "unhandled error when sending buffer data!");
    //             break;
    //     }
    //     assert(0);
    // }
    
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
                printf("%s\n", "GL_INVALID_VALUE");
                break;
            case GL_INVALID_ENUM:
                printf("%s\n", "GL_INVALID_ENUM");
                break;
            case GL_INVALID_OPERATION:
                printf("%s\n", "GL_INVALID_OPERATION");
                break;
            default:
                printf("%s\n", "unhandled error when sending buffer data!");
                break;
        }
        assert(0);
    }
    #endif
}

static void opengl_compile_given_shader(
    GLuint shader_id,
    char * shader_source,
    GLint source_length)
{
    printf("compile shader with id: %u\n", shader_id);
    glShaderSource(
        /* GLuint handle: */
            shader_id,
        /* shader count : */
            1,
        /* const GLchar ** string: */
            &shader_source,
        /* const GLint * length: */
            &source_length);
    
    glCompileShader(shader_id);
    
    GLint is_compiled = INT8_MAX;
    char info_log[512];
    info_log[0] = '\0';
    glGetShaderiv(
        /* GLuint id: */
            shader_id,
        /* GLenum pname: */
            GL_COMPILE_STATUS,
        /* GLint * params: */
            &is_compiled);
    
    if (is_compiled == GL_FALSE) {
        GLenum err_value = glGetError();
        
        glGetShaderInfoLog(
            /* GLuint shader id: */
                shader_id,
            /* GLsizei max length: */
                512,
            /* GLsizei * length: */
                NULL,
            /* GLchar * infolog: */
                info_log);
        printf("info_log: %s\n", info_log);
        assert(0);
    } else if (is_compiled == GL_TRUE) {
        
    } else {
        printf(
            "glGetShaderiv() returned an impossible value. Expected GL_TRUE "
            "(%i) or GL_FALSE (%i), got: %i\n",
            GL_TRUE,
            GL_FALSE,
            is_compiled);
        assert(0);
    }
}

void opengl_compile_shaders(
    char * vertex_shader_source,
    uint32_t vertex_shader_source_size,
    char * fragment_shader_source,
    uint32_t fragment_shader_source_size)
{ 
    // allocate buffer memory...
    GLenum err_value;
    
    program_id = glCreateProgram();
    assert(program_id == 1);
    GLuint vertex_shader_id = glCreateShader(GL_VERTEX_SHADER);
    err_value = glGetError();
    assert(err_value == 0);
    
    assert(vertex_shader_source_size > 0);
    assert(vertex_shader_source != NULL);
    
    opengl_compile_given_shader(
        /* GLuint shader_id: */
            vertex_shader_id,
        /* char * shader_source: */
            vertex_shader_source,
        /* GLint source_length: */
            vertex_shader_source_size);
    err_value = glGetError();
    assert(err_value == 0);
    
    fragment_shader_id = glCreateShader(GL_FRAGMENT_SHADER);
    err_value = glGetError();
    assert(err_value == 0);
    assert(fragment_shader_source_size > 0);
    assert(fragment_shader_source != NULL);
    opengl_compile_given_shader(
        /* GLuint shader_id: */
            fragment_shader_id,
        /* char * shader_source: */
            fragment_shader_source,
        /* GLint source_length: */
            fragment_shader_source_size);
    err_value = glGetError();
    assert(err_value == 0);
    
    // attach compiled shaders to program
    glAttachShader(program_id, vertex_shader_id);
    err_value = glGetError();
    assert(err_value == 0);
    glAttachShader(program_id, fragment_shader_id);
    err_value = glGetError();
    assert(err_value == 0);
    glLinkProgram(program_id);
    err_value = glGetError();
    assert(err_value == 0);
    
    GLint success = -1;
    glGetProgramiv(
        /* GLuint program id: */
            program_id,
        /* GLenum pname: */
            GL_LINK_STATUS,
        /* GLint * params: */
            &success);
    assert(success);
    
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
        assert(0);
    }
    
    // TODO: Learn exactly when nescessary, I hope we can just set & forget
    glUseProgram(program_id); // TODO: can this have averse effects?
    err_value = glGetError();
    assert(err_value == 0);
    
    glGenVertexArrays(1, &VAO);
    err_value = glGetError();
    assert(err_value == GL_NO_ERROR);
    
    glGenBuffers(1, &VBO);
    err_value = glGetError();
    assert(err_value == GL_NO_ERROR);
    
    glBindVertexArray(VAO);
    err_value = glGetError();
    assert(err_value == GL_NO_ERROR);
    
    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    err_value = glGetError();
    assert(err_value == GL_NO_ERROR);
    
    // We didn't use the standard alpha channel in Metal, so it shouldn't
    // be enabled here
    // glEnable(GL_BLEND);
    // glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    
    // The depth buffer is so much easier here than with Metal.
    // On metal, we chose 32-bit floats
    // for depth, the test is LESS OR EQUAL,
    // 
    glEnable(GL_DEPTH_TEST);
    glClearDepth(50.0f);
    glDepthFunc(GL_LEQUAL); // on metal: LEQUAL

    glEnable(GL_CULL_FACE);
    glCullFace(GL_FRONT);
    
    /*
    Attribute pointers describe the fields of our data
    sructure (the Vertex struct in shared/cpu_gpu_shared_types.h)
    */
    assert(sizeof(int) == sizeof(float));
    assert(sizeof(GLint) == sizeof(GLfloat));
    assert(sizeof(GLint) == sizeof(int));
    uint32_t field_sizes[12]   = { 3, 3, 2, 4,  1,  1, 1  };
    uint32_t field_offsets[12] = { 0, 3, 6, 8, 12, 13, 14 };
    uint32_t cur_offset = 0;
    
    for (uint32_t _ = 0; _ < 7; _++) {
        
        printf("vertex attribute: %u (%u items)\n", _, field_sizes[_]);
        
        GLenum current_type = GL_FLOAT;
        if (_ >= 4 && _ <= 6) {
            current_type = GL_INT;
        }
        assert(cur_offset == field_offsets[_]);
        
        if (current_type == GL_INT) {
            printf("%s\n", "type GL_INT");
            /*
            This is another massive trap in OpenGL:
            You have to use glVertexAttribIPointer, not glVertexAttribPointer,
            when and only when setting up a vertex input that's an int. The
            other version also accepts GL_INT and the documentation claims that
            it can be used for ints, but it silently fails and garbles your
            int values to some huge value even if you set normalize data to
            GL_FALSE.
            */
            glVertexAttribIPointer(
                /* GLuint index (location in shader source): */
                    _,
                /* GLint size (number of components per vertex, must be 1-4): */
                    field_sizes[_],
                /* GLenum type (of data): */
                    GL_INT,
                /* GLsizei stride; */
                    sizeof(GPUVertex),
                /* const GLvoid * pointer (offset) : */
                    (void *)(cur_offset * sizeof(float)));
        } else {
            printf("%s\n", "type GL_FLOAT");
            /*
            DANGER: this function is part of the trap
            it bizarrely casts your input to floats, even if you explicitly
            state they're ints
            */
            glVertexAttribPointer(
                /* GLuint index (location in shader source): */
                    _,
                /* GLint size (no. of components per vertex, must be 1-4): */
                    field_sizes[_],
                /* GLenum type (of data): */
                    GL_FLOAT,
                /* GLboolean normalize data: */
                    GL_FALSE,
                /* GLsizei stride; */
                   sizeof(GPUVertex),
                /* const GLvoid * pointer (offset) : */
                    (void *)(cur_offset * sizeof(float)));
        }
        cur_offset += field_sizes[_];
        
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
            assert(0);
        }
        
        glEnableVertexAttribArray(_);
        assert(glGetError() == 0);
    }
    // now that we're done, the offset should be the entirety of a GPUVertex
    if (cur_offset != sizeof(GPUVertex) / 4) {
        printf("cur_offset: %i\n", cur_offset);
        printf(
            "sizeof(GPUVertex): %i, which div 4 is: %i\n",
            sizeof(GPUVertex),
            sizeof(GPUVertex) / 4);
        assert(0);
    }
    
    // validate program
    success = 0;
    glValidateProgram(program_id);
    glGetProgramiv(program_id, GL_VALIDATE_STATUS, &success);
    assert(success);
    assert(glGetError() == 0);
}

/* reminder: this is mutex protected */
void platform_gpu_init_texture_array(
    const int32_t texture_array_i,
    const uint32_t num_images,
    const uint32_t single_image_width,
    const uint32_t single_image_height)
{
    printf(
        "opengl must init texture array: %i with %u images (%ux%u)\n",
        texture_array_i,
        num_images,
        single_image_width,
        single_image_height);
    
    glGenTextures(1, &texture_array_ids[texture_array_i]);
    assert(glGetError() == 0);
    assert(texture_array_i < 31);
    
    glActiveTexture(
        GL_TEXTURE0 + texture_array_i);
    assert(glGetError() == 0);
    
    glBindTexture(
        GL_TEXTURE_2D_ARRAY,
        texture_array_ids[texture_array_i]);
    assert(glGetError() == 0);
    
    char name_in_shader[64];
    strcpy_capped(name_in_shader, 64, "texture_arrays[");
    strcat_uint_capped(name_in_shader, 64, texture_array_i);
    strcat_capped(name_in_shader, 64, "]");
    GLuint loc = glGetUniformLocation(
        program_id,
        name_in_shader);
    assert(glGetError() == 0);
    
    glUniform1iv(loc, 1, &texture_array_i);
    assert(glGetError() == 0);
    
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
        assert(err_value == GL_NO_ERROR);
    }
    
    glTexParameteri(
        /* GLenum target: */
            GL_TEXTURE_2D_ARRAY,
        /* GLenum pname: */
            GL_TEXTURE_WRAP_S,
        /* GLint param: */
            GL_CLAMP_TO_EDGE);
    assert(glGetError() == 0);
    
    glTexParameteri(
        /* GLenum target: */
            GL_TEXTURE_2D_ARRAY,
        /* GLenum pname: */
            GL_TEXTURE_WRAP_T,
        /* GLint param: */
            GL_CLAMP_TO_EDGE);
    assert(glGetError() == 0);
    
    glTexParameteri(
        /* GLenum target: */
            GL_TEXTURE_2D_ARRAY,
        /* GLenum pname: */
            GL_TEXTURE_MIN_FILTER,
        /* GLint param: */
            GL_NEAREST);
    assert(glGetError() == 0);
    
    glTexParameteri(
        /* GLenum target: */
            GL_TEXTURE_2D_ARRAY,
        /* GLenum pname: */
            GL_TEXTURE_MAG_FILTER,
        /* GLint param: */
            GL_NEAREST);
    assert(glGetError() == 0);
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
    printf(
        "opengl_push_texture(): %u to array: %u\n",
        texture_i,
        texture_array_i);
    assert(image_width > 0);
    assert(image_height > 0);
    assert(parent_texture_array_images_size > texture_i);
    
    glActiveTexture(
        GL_TEXTURE0 + texture_array_i);
    assert(glGetError() == 0);
    
    glBindTexture(
        GL_TEXTURE_2D_ARRAY,
        texture_array_ids[texture_array_i]);
    assert(glGetError() == 0);
   
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
        assert(0);
    }
}

void opengl_set_projection_constants(
    GPUProjectionConstants * pjc)
{
    assert(pjc != NULL);
    glUseProgram(program_id);
    
    // set viewport
    // these args might be getting clamped
    // to 0 - 1, I wish opengl would just
    // throw instead of silently clamping
    assert(pjc->near > 0.005f);
    assert(pjc->far < 11.0f);
    glDepthRangef(
        /* GLfloat nearVal: */ 
            pjc->near,
        /* GLfloat farVal: */
            pjc->far);
    
    glFrustum(
        0, 1,
        0, 1,
        pjc->near,
        pjc->far);
    
    GLint loc = -1;
    
    loc = glGetUniformLocation(
        program_id,
        "projection_constants_near");
    assert(glGetError() == 0);
    assert(glGetError() == 0);
    // reminder: don't use glUniform1f, it's buggy on ubuntu with some
    // drivers, use glUniform1fv instead
    glUniform1fv(loc, 1, &pjc->near);
    assert(glGetError() == 0);
    
    loc = -1;
    loc = glGetUniformLocation(
        program_id,
        "projection_constants_q");
    assert(glGetError() == 0);
    // reminder: don't use glUniform1f, it's buggy on ubuntu with some
    // drivers, use glUniform1fv instead
    glUniform1fv(loc, 1, &pjc->q);
    assert(glGetError() == 0);
    
    loc = -1;
    loc = glGetUniformLocation(
        program_id,
        "projection_constants_fov_modifier");
    // reminder: don't use glUniform1f, it's buggy on ubuntu with some
    // drivers, use glUniform1fv instead
    glUniform1fv(loc, 1, &pjc->field_of_view_modifier);
    assert(glGetError() == 0);
    
    loc = -1;
    loc = glGetUniformLocation(
        program_id,
        "projection_constants_x_multiplier");
    // reminder: don't use glUniform1f, it's buggy on ubuntu with some
    // drivers, use glUniform1fv instead
    glUniform1fv(loc, 1, &pjc->x_multiplier);
    assert(glGetError() == 0);
}

