#include "opengl.h"

// We'll need these 2 identifiers while drawing
GLuint program_id;
GLuint fragment_shader_id;
unsigned int VAO = UINT32_MAX;
unsigned int VBO = UINT32_MAX;
static GLuint err_value = UINT32_MAX;

static GLuint texture_array_ids[TEXTUREARRAYS_SIZE];

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
    
    for (
        uint32_t i = 0;
        i < frame_data->vertices_size;
        i++)
    {
        assert(frame_data->vertices[i].uv[0] > -0.1f);
        assert(frame_data->vertices[i].uv[1] > -0.1f);
        assert(frame_data->vertices[i].uv[0] <  1.05f);
        assert(frame_data->vertices[i].uv[1] <  1.05f);
        assert(frame_data->vertices[i].texture_i < 500);
        assert(frame_data->vertices[i].texturearray_i < TEXTUREARRAYS_SIZE);
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
    
    // TODO: query the data in GL_ARRAY_BUFFER to make sure it's correct
    for (uint32_t i = 0; i < 10; i++) {
        if (i >= frame_data->vertices_size) { break; }
        float actual = 1234345.0f;
        glGetBufferSubData(
            /* GLenum target: */
                GL_ARRAY_BUFFER,
            /* GLintptr offset: */
                (i * sizeof(GPUVertex)) + 0,
            /* GLsizeiptr size: */
                4,
            /* void * data): */
                &actual);
        assert(actual == frame_data->vertices[i].x);
        actual = 2934.2f;
        glGetBufferSubData(
            /* GLenum target: */
                GL_ARRAY_BUFFER,
            /* GLintptr offset: */
                (i * sizeof(GPUVertex)) + 4,
            /* GLsizeiptr size: */
                4,
            /* void * data): */
                &actual);
        assert(actual == frame_data->vertices[i].y);
        actual = 2934.2f;
        glGetBufferSubData(
            /* GLenum target: */
                GL_ARRAY_BUFFER,
            /* GLintptr offset: */
                (i * sizeof(GPUVertex)) + 8,
            /* GLsizeiptr size: */
                4,
            /* void * data): */
                &actual);
        assert(actual == frame_data->vertices[i].z);
        actual = 2934.2f;
        glGetBufferSubData(
            /* GLenum target: */
                GL_ARRAY_BUFFER,
            /* GLintptr offset: */
                (i * sizeof(GPUVertex)) + 12,
            /* GLsizeiptr size: */
                4,
            /* void * data): */
                &actual);
        assert(actual == frame_data->vertices[i].normal_x);
        actual = 2934.2f;
        glGetBufferSubData(
            /* GLenum target: */
                GL_ARRAY_BUFFER,
            /* GLintptr offset: */
                (i * sizeof(GPUVertex)) + 16,
            /* GLsizeiptr size: */
                4,
            /* void * data): */
                &actual);
        assert(actual == frame_data->vertices[i].normal_y);
        actual = 2934.2f;
        glGetBufferSubData(
            /* GLenum target: */
                GL_ARRAY_BUFFER,
            /* GLintptr offset: */
                (i * sizeof(GPUVertex)) + 20,
            /* GLsizeiptr size: */
                4,
            /* void * data): */
                &actual);
        assert(actual == frame_data->vertices[i].normal_z);
        actual = 234.0f;
        glGetBufferSubData(
            /* GLenum target: */
                GL_ARRAY_BUFFER,
            /* GLintptr offset: */
                (i * sizeof(GPUVertex)) + 24,
            /* GLsizeiptr size: */
                4,
            /* void * data): */
                &actual);
        assert(actual == frame_data->vertices[i].uv[0]);
        assert(actual < 1.05f && actual > -0.05f);
        actual = -125.0f;
        glGetBufferSubData(
            /* GLenum target: */
                GL_ARRAY_BUFFER,
            /* GLintptr offset: */
                (i * sizeof(GPUVertex)) + 28,
            /* GLsizeiptr size: */
                4,
            /* void * data: */
                &actual);
        assert(actual == frame_data->vertices[i].uv[1]);
        assert(actual < 1.05f && actual > -0.05f);
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

    glActiveTexture(GL_TEXTURE0);
    assert(glGetError() == 0);
    
    // glBindTexture(
    //     GL_TEXTURE_2D_ARRAY,
    //     texture_array_ids[texture_array_i]);
    // assert(glGetError() == 0);
    
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
    
    // We didn't use the standard alpha channel in Metal, so it shouldn't
    // be enabled here
    // glEnable(GL_BLEND);
    // glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    
    /*
    Attribute pointers describe the fields of our data
    sructure (the Vertex struct in shared/vertex_types.h)
    */
    assert(sizeof(int) == sizeof(float));
    assert(sizeof(GLint) == sizeof(GLfloat));
    assert(sizeof(GLint) == sizeof(int));
    uint32_t field_sizes[12]   = { 3, 3, 2, 4,  1,  1,  3,  3,  1,  1,  1,  1};
    uint32_t field_offsets[12] = { 0, 3, 6, 8, 12, 13, 14, 17, 20, 21, 22, 23};
    uint32_t cur_offset = 0;
    
    for (uint32_t _ = 0; _ < 12; _++) {
        
        printf("vertex attribute: %u (%u items)\n", _, field_sizes[_]);
        
        GLenum current_type = GL_FLOAT;
        if (_ >= 4 && _ <= 5 || _ == 11) {
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

