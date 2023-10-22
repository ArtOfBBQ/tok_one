#include "opengl.h"

// We'll need these 2 identifiers while drawing
GLuint program_id;
GLuint fragment_shader_id;
unsigned int VAO = UINT32_MAX;
unsigned int VBO = UINT32_MAX;
static GLuint err_value = UINT32_MAX;

void opengl_render_triangles(GPUDataForSingleFrame * frame_data) {
    
    assert(VAO < UINT32_MAX);
    assert(VBO < UINT32_MAX);
    
    glUseProgram(program_id);
    
    if (frame_data->vertices_size < 1) { return; }
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
    
    for (uint32_t i = 0; i < frame_data->vertices_size; i++) {
        assert(frame_data->vertices->texture_i < 500);
        assert(frame_data->vertices->texturearray_i < TEXTUREARRAYS_SIZE);
    }
    
    glBufferData(
        /* target: */
            GL_ARRAY_BUFFER,
        /* size_in_bytes: */
            sizeof(GPUVertex) * frame_data->vertices_size,
        /* const GLvoid * data: (to init with, or NULL to copy no data) */
            (const GLvoid *)frame_data->vertices,
        /* usage: */
            GL_DYNAMIC_DRAW);
    
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

    opengl_set_camera(frame_data->camera);
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
    
    opengl_set_projection_constants(frame_data->projection_constants);
    
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
    
    // glPointSize(10); // for GL_POINTS
    glDrawArrays(
        /* GLenum mode: */
            GL_TRIANGLES, 
        /* GLint first: */
            0,
        /* GLint count (# of vertices to render): */
            frame_data->vertices_size);
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
}

static void opengl_compile_given_shader(
    GLuint shader_id,
    char * shader_source,
    GLint source_length)
{
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
    
    /*
    Attribute pointers describe the fields of our data
    sructure (the Vertex struct in shared/vertex_types.h)
    */
    assert(sizeof(int) == sizeof(float));
    assert(sizeof(GLint) == sizeof(GLfloat));
    assert(sizeof(GLint) == sizeof(int));
    uint32_t field_sizes[12] = { 3, 3, 2, 4, 1, 1, 3, 3, 1, 1, 1, 1 };
    uint32_t cur_offset = 0;
    for (uint32_t _ = 0; _ < 12; _++) {
        
        GLenum current_type = GL_FLOAT;
        if (_ >= 4 && _ <= 5) {
            current_type = GL_INT;
        }
        
        if (current_type == GL_INT) {
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
                    current_type,
                /* GLsizei stride; */
                    sizeof(GPUVertex),
                /* const GLvoid * pointer (offset) : */
                    (void *)(cur_offset * sizeof(float)));
        } else {
            glVertexAttribPointer(
                /* GLuint index (location in shader source): */
                    _,
                /* GLint size (number of components per vertex, must be 1-4): */
                    field_sizes[_],
                /* GLenum type (of data): */
                    current_type,
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
    
    // validate program
    success = 0;
    glValidateProgram(program_id);
    glGetProgramiv(program_id, GL_VALIDATE_STATUS, &success);
    assert(success);
    assert(glGetError() == 0);
}

void opengl_set_camera(
    GPUCamera * camera)
{
    GLint loc = -1;
    
    assert(program_id == 1);
    glUseProgram(program_id);
    assert(glGetError() == 0);
    
    loc = glGetUniformLocation(
        program_id,
        "camera_position");
    assert(loc == 0);
    assert(glGetError() == 0);
    // there is also glProgramUniform3f, but that's not in OpenGL 3.0
    // We are using 'glUseProgram' explicitly here and before this so
    // I don't think that can be the problem
    float cam_pos[3];
    cam_pos[0] = camera->x;
    cam_pos[1] = camera->y;
    cam_pos[2] = camera->z;
    glUniform3fv(
        loc,
        1,
        cam_pos);
    err_value = glGetError();
    if (err_value != 0) {
        printf("error trying to set camera pos uniform\n");
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
    assert(loc == 1);
    assert(glGetError() == 0);
    float cam_angle[3];
    cam_angle[0] = camera->x_angle;
    cam_angle[1] = camera->y_angle;
    cam_angle[2] = camera->z_angle;
    glUniform3fv(
        loc,
        1,
        cam_angle);
    assert(glGetError() == 0);
    
    float doublecheck_cam_angle[3];
    doublecheck_cam_angle[2] = 234.12f;
    glGetUniformfv(program_id, loc, doublecheck_cam_angle);
    assert(glGetError() == 0);
    assert(doublecheck_cam_angle[0] == camera->x_angle);
    assert(doublecheck_cam_angle[1] == camera->y_angle);
    assert(doublecheck_cam_angle[2] == camera->z_angle);
}

static GLuint texture_array_ids[TEXTUREARRAYS_SIZE];

/* reminder: this is mutex protected */
void platform_gpu_init_texture_array(
    const int32_t texture_array_i,
    const uint32_t num_images,
    const uint32_t single_image_width,
    const uint32_t single_image_height)
{
    printf(
        "opengl must init texture array: %i\n",
        texture_array_i);
    
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
    
    GLuint loc = glGetUniformLocation(
        program_id,
        "texture_arrays[0]");
    assert(glGetError() == 0);
    
    uint32_t value = 0;
    glUniform1iv(loc, 1, &value);
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
            GL_MIRRORED_REPEAT);
    assert(glGetError() == 0);
    glTexParameteri(
        /* GLenum target: */
            GL_TEXTURE_2D_ARRAY,
        /* GLenum pname: */
            GL_TEXTURE_WRAP_T,
        /* GLint param: */
            GL_MIRRORED_REPEAT);
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
            GL_LINEAR);
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
    printf("opengl_push_texture()\n");
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
    
    glTexSubImage3D(
        /* GLenum target: */
            GL_TEXTURE_2D_ARRAY,
        /* GLint level: */
            0,
        /* GLint xoffset: */
            0,
        /* GLint yoffset: */
            0,
        /* GLint zoffset: */
            0,
        /* GLsizei width: */
            image_width,
        /* GLsizei height: */
            image_height,
        /* GLsizei depth: */
            texture_i,
        /* GLenum format: */
            GL_RGBA,
        /* GLenum type: */
            GL_UNSIGNED_BYTE,
        /* const GLvoid * data: */
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
    glDepthRangef(
        /* GLfloat nearVal: */ 
            pjc->near,
        /* GLfloat farVal: */
            pjc->far);
    
    GLint loc = -1;
    
    loc = glGetUniformLocation(
        program_id,
        "projection_constants_near");
    assert(glGetError() == 0);
    assert(loc == 2);
    assert(pjc->near > 0.099f);
    assert(pjc->near < 0.110f);
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
    assert(loc == 3);
    // reminder: don't use glUniform1f, it's buggy on ubuntu with some
    // drivers, use glUniform1fv instead
    glUniform1fv(loc, 1, &pjc->q);
    assert(glGetError() == 0);
    
    loc = -1;
    loc = glGetUniformLocation(
        program_id,
        "projection_constants_fov_modifier");
    assert(loc == 4);
    // reminder: don't use glUniform1f, it's buggy on ubuntu with some
    // drivers, use glUniform1fv instead
    glUniform1fv(loc, 1, &pjc->field_of_view_modifier);
    assert(glGetError() == 0);
    
    loc = -1;
    loc = glGetUniformLocation(
        program_id,
        "projection_constants_x_multiplier");
    assert(loc == 5);
    // reminder: don't use glUniform1f, it's buggy on ubuntu with some
    // drivers, use glUniform1fv instead
    glUniform1fv(loc, 1, &pjc->x_multiplier);
    assert(glGetError() == 0);
}

