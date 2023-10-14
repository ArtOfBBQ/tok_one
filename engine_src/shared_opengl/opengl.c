#include "opengl.h"

// We'll need these 2 identifiers while drawing
GLuint program_id;
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
    
    frame_data->vertices[0].ignore_camera = 1.0f;
    frame_data->vertices[0].RGBA[1] = 1.0f;
    frame_data->vertices[1].ignore_camera = 1.0f;
    frame_data->vertices[2].ignore_camera = 1.0f;
    
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
     
    glPointSize(10); // for GL_POINTS
    glDrawArrays(
        /* GLenum mode: */
            GL_POINTS, 
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
        printf("%s\n", "failed to compile shader with source: ");
        printf("%s\n", "*******");
        printf("%s\n", shader_source);
        printf("%s\n", "*******");
        GLenum err_value = glGetError();
        printf("glGetError returned: %u\n", err_value);
        printf("%s\n", "*******");
        
        glGetShaderInfoLog(
            /* GLuint shader id: */
                shader_id,
            /* GLsizei max length: */
                512,
            /* GLsizei * length: */
                NULL,
            /* GLchar * infolog: */
                info_log);
        printf("shader info log: %s\n", info_log);
        printf("%s\n", "*******");
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
    printf("program_id: %u\n", program_id);
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
    
    GLuint fragment_shader_id = glCreateShader(GL_FRAGMENT_SHADER);
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
    uint32_t field_sizes[12] = { 3, 3, 2, 4, 1, 1, 3, 3, 1, 1, 1, 1 };
    uint32_t cur_offset = 0;
    for (uint32_t _ = 0; _ < 12; _++) {
        
        glVertexAttribPointer(
            /* GLuint index (location in shader source): */
                _,
            /* GLint size (number of components per vertex, must be 1-4): */
                field_sizes[_],
            /* GLenum type (of data): */
                GL_FLOAT,
            /* GLboolean normalize data: */
                GL_FALSE,
            /* GLsizei stride; */
                sizeof(GPUVertex),
            /* const GLvoid * pointer (offset) : */
                (void *)(cur_offset * sizeof(float)));
        
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
    printf("set camera: {%f,%f,%f}", camera->x, camera->y, camera->z);
    printf(
        " - angle: {%f,%f,%f}\n",
        camera->x_angle, camera->y_angle, camera->z_angle);
    
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
    
    float doublecheck_cam_pos[3];
    doublecheck_cam_pos[0] = 3234.5f;
    doublecheck_cam_pos[1] = 3214.5f;
    doublecheck_cam_pos[2] = 1.5f;
    assert(loc == 0);
    glGetUniformfv(program_id, loc, doublecheck_cam_pos);
    printf(
        "(position) doublecheck values: {%f,%f,%f}\n",
        doublecheck_cam_pos[0],
        doublecheck_cam_pos[1],
        doublecheck_cam_pos[2]);
    assert(glGetError() == 0);
    assert(doublecheck_cam_pos[0] == camera->x);
    assert(doublecheck_cam_pos[1] == camera->y);
    assert(doublecheck_cam_pos[2] == camera->z);
    
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

void opengl_set_projection_constants(
    GPUProjectionConstants * pjc)
{
    assert(pjc != NULL);
    glUseProgram(program_id);
    
    printf(
        "setting pjc: near %f, far %f, q: %f, fov_rad: %f, fov_mod: %f, "
        "x_multiplier: %f, y_multiplier: %f\n",
        pjc->near,
        pjc->far,
        pjc->q,
        pjc->field_of_view_rad,
        pjc->field_of_view_modifier,
        pjc->x_multiplier,
        pjc->y_multiplier);
    
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
    printf("projection_constants_near at loc: %i\n", loc);
    assert(glGetError() == 0);
    assert(loc == 2);
    assert(pjc->near > 0.099f);
    assert(pjc->near < 0.110f);
    printf("copying pjc->near to uniform: %f\n", pjc->near);
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
    printf("projection_constants_q at loc: %i\n", loc);
    // reminder: don't use glUniform1f, it's buggy on ubuntu with some
    // drivers, use glUniform1fv instead
    glUniform1fv(loc, 1, &pjc->q);
    assert(glGetError() == 0);
    
    loc = -1;
    loc = glGetUniformLocation(
        program_id,
        "projection_constants_fov_modifier");
    printf("projection_constants_fov_modifier at loc: %i\n", loc);
    assert(loc == 4);
    // reminder: don't use glUniform1f, it's buggy on ubuntu with some
    // drivers, use glUniform1fv instead
    glUniform1fv(loc, 1, &pjc->field_of_view_modifier);
    assert(glGetError() == 0);
    
    loc = -1;
    loc = glGetUniformLocation(
        program_id,
        "projection_constants_x_multiplier");
    printf("projection_constants_x_multiplier at loc: %i\n", loc);
    assert(loc == 5);
    // reminder: don't use glUniform1f, it's buggy on ubuntu with some
    // drivers, use glUniform1fv instead
    glUniform1fv(loc, 1, &pjc->x_multiplier);
    assert(glGetError() == 0);
    
    // TODO: find out why the uniform
    // TODO: 'projection_constants_near' isn't getting set
    loc = glGetUniformLocation(
        program_id,
        "projection_constants_near");
    assert(loc == 2);
    printf("loc for check_near: %i\n", loc);
    GLfloat check_near = -123.0f;
    glGetUniformfv(program_id, loc, &check_near);
    
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
    printf("check_near: %f, pjc->near: %f\n", check_near, pjc->near);
    assert(check_near == pjc->near);
}

