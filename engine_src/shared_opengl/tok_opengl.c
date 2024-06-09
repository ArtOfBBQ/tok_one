#include "tok_opengl.h"

static unsigned int VAO;
static unsigned int diamond_program_id;
static unsigned int alphablending_program_id;

static char opengl_info_log[512];

static unsigned int vertex_VBO; // vertex array buffer
static unsigned int camera_VBO; // binding 2
static unsigned int polygons_VBO; // binding 3
static unsigned int locked_vertices_VBO; // binding 4
static unsigned int projection_constants_VBO; // binding 5
static unsigned int light_collection_VBO; // binding 6
static unsigned int materials_VBO; // binding 7

static GLuint texture_array_ids[TEXTUREARRAYS_SIZE];

static void shadersource_apply_macro_inplace(
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

static void opengl_compile_shader(
    char * shader_source,
    GLenum SHADER_ENUM_TYPE,
    GLuint * shader_id)
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
    
    log_assert(!glGetError());
    *shader_id = extptr_glCreateShader(SHADER_ENUM_TYPE);
    log_assert(!glGetError());
    char * shader_source_as_ptr = shader_source;
    extptr_glShaderSource(
        *shader_id,
        1,
        &shader_source_as_ptr,
        NULL);
    
    log_assert(!glGetError());
    extptr_glCompileShader(*shader_id);
    GLint is_compiled = INT8_MAX;
    opengl_info_log[0] = '\0';
    extptr_glGetShaderiv(
        /* GLuint id: */
            *shader_id,
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
                *shader_id,
            /* GLsizei max length: */
                512,
            /* GLsizei * length: */
                NULL,
            /* GLchar * infolog: */
                opengl_info_log);
        
        printf("opengl_info_log: %s\n", opengl_info_log);
    }
    
    log_assert(!glGetError());    
}

void opengl_copy_projection_constants(
    GPUProjectionConstants * projection_constants)
{
    log_assert(projection_constants->zfar > 5.0f);
    log_assert(projection_constants->znear < 0.1f);
    
    log_assert(!glGetError());
    extptr_glGenBuffers(1, &projection_constants_VBO);
    log_assert(!glGetError());
    extptr_glBindBuffer(
        GL_SHADER_STORAGE_BUFFER,
        projection_constants_VBO);
    log_assert(!glGetError());
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
    // Vertex Array Buffer
    extptr_glBindBuffer(GL_ARRAY_BUFFER, vertex_VBO);
    extptr_glBufferData(
        GL_ARRAY_BUFFER,
        sizeof(GPUVertex) * frame_data->vertices_size,
        frame_data->vertices,
        GL_STATIC_DRAW);
    extptr_glBindBuffer(GL_ARRAY_BUFFER, 0);
    
    // binding 2
    extptr_glBindBuffer(GL_SHADER_STORAGE_BUFFER, camera_VBO);
    extptr_glBufferData(
        GL_SHADER_STORAGE_BUFFER,
        sizeof(GPUCamera),
        frame_data->camera,
        GL_STATIC_DRAW);
    log_assert(!glGetError());
    extptr_glBindBuffer(GL_SHADER_STORAGE_BUFFER, 0);
    
    // binding 3
    extptr_glBindBuffer(GL_SHADER_STORAGE_BUFFER, polygons_VBO);
    uint32_t polygons_size = frame_data->polygon_collection->size;
    if (polygons_size < 5) { polygons_size = 5; }
    extptr_glBufferData(
        GL_SHADER_STORAGE_BUFFER,
        sizeof(GPUPolygon) * polygons_size,
        frame_data->polygon_collection->polygons,
        GL_STATIC_DRAW);
    extptr_glBindBuffer(GL_SHADER_STORAGE_BUFFER, 0);
    
    // binding 6
    extptr_glBindBuffer(GL_SHADER_STORAGE_BUFFER, light_collection_VBO);
    log_assert(!glGetError());
    extptr_glBufferData(
        GL_SHADER_STORAGE_BUFFER,
        sizeof(GPULightCollection),
        frame_data->light_collection,
        GL_STATIC_DRAW);
    log_assert(!glGetError());
    extptr_glBindBuffer(GL_SHADER_STORAGE_BUFFER, 0);

    // binding 7
    extptr_glBindBuffer(GL_SHADER_STORAGE_BUFFER, materials_VBO);
    log_assert(!glGetError());
    extptr_glBufferData(
        GL_SHADER_STORAGE_BUFFER,
        sizeof(GPUPolygonMaterial) *
            MAX_MATERIALS_PER_POLYGON *
            MAX_POLYGONS_PER_BUFFER,
        frame_data->polygon_materials,
        GL_STATIC_DRAW);
    log_assert(!glGetError());
    extptr_glBindBuffer(GL_SHADER_STORAGE_BUFFER, 0);
}

void opengl_init(
    char * vertex_shader_source,
    char * fragment_shader_source,
    char * alphablending_fragment_shader_source)
{
    glDepthRange(0.0f, 1.0f);
    
    glEnable(GL_DEPTH_TEST);
    glClearDepth(10.0f);
    
    log_assert(!glGetError());
    diamond_program_id = extptr_glCreateProgram();
    printf("Created diamond_program_id: %u\n", diamond_program_id);
    log_assert(diamond_program_id > 0);
    log_assert(!glGetError());
    
    log_assert(!glGetError());
    extptr_glGenVertexArrays(1, &VAO);
    extptr_glBindVertexArray(VAO);
    
    log_assert(!glGetError());
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
    log_assert(!glGetError());
    extptr_glEnableVertexAttribArray(0);
    extptr_glBindBuffer(GL_ARRAY_BUFFER, 0);
    
    log_assert(!glGetError());
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
    
    log_assert(!glGetError());
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
    
    log_assert(!glGetError());
    extptr_glGenBuffers(1, &locked_vertices_VBO);
    log_assert(!glGetError());
    extptr_glBindBuffer(GL_SHADER_STORAGE_BUFFER, locked_vertices_VBO);
    log_assert(!glGetError());
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
    
    log_assert(!glGetError());
    extptr_glGenBuffers(1, &projection_constants_VBO);
    log_assert(!glGetError());
    extptr_glBindBuffer(
        GL_SHADER_STORAGE_BUFFER,
        projection_constants_VBO);
    log_assert(!glGetError());
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
    
    log_assert(!glGetError());
    extptr_glGenBuffers(1, &light_collection_VBO);
    log_assert(!glGetError());
    extptr_glBindBuffer(
        GL_SHADER_STORAGE_BUFFER,
        light_collection_VBO);
    log_assert(!glGetError());
    extptr_glBindBufferBase(
        GL_SHADER_STORAGE_BUFFER,
        6,
        light_collection_VBO);
    extptr_glBufferData(
        GL_SHADER_STORAGE_BUFFER,
        sizeof(GPULightCollection),
        0,
        GL_STATIC_DRAW);
    extptr_glBindBuffer(GL_SHADER_STORAGE_BUFFER, 0);

    log_assert(!glGetError());
    extptr_glGenBuffers(1, &materials_VBO);
    log_assert(!glGetError());
    extptr_glBindBuffer(
        GL_SHADER_STORAGE_BUFFER,
        materials_VBO);
    log_assert(!glGetError());
    extptr_glBindBufferBase(
        GL_SHADER_STORAGE_BUFFER,
        7,
        materials_VBO);
    extptr_glBufferData(
        GL_SHADER_STORAGE_BUFFER,
        sizeof(GPUPolygonMaterial) *
            MAX_MATERIALS_PER_POLYGON *
            MAX_POLYGONS_PER_BUFFER,
        0,
        GL_STATIC_DRAW);
    extptr_glBindBuffer(GL_SHADER_STORAGE_BUFFER, 0);
    
    GLuint vertex_shader_id;
    GLuint fragment_shader_id;
    
    opengl_compile_shader(
        vertex_shader_source,
        GL_VERTEX_SHADER,
        &vertex_shader_id);
    opengl_compile_shader(
        fragment_shader_source,
        GL_FRAGMENT_SHADER,
        &fragment_shader_id);
    
    extptr_glAttachShader(diamond_program_id, vertex_shader_id);
    log_assert(!glGetError());
    extptr_glAttachShader(diamond_program_id, fragment_shader_id);
    log_assert(!glGetError());
    
    extptr_glLinkProgram(diamond_program_id);
    log_assert(!glGetError());

    unsigned int success = 0;
    extptr_glGetProgramiv(diamond_program_id, GL_LINK_STATUS, &success);
    if (!success) {
        extptr_glGetProgramInfoLog(
            diamond_program_id,
            512,
            NULL,
            opengl_info_log);
        printf("ERROR - GL_LINK_STATUS: %s\n", opengl_info_log);
        getchar();
        return;
    }
    
    GLuint alphablending_fragment_shader_id;
    opengl_compile_shader(
        alphablending_fragment_shader_source,
        GL_FRAGMENT_SHADER,
        &alphablending_fragment_shader_id);

    log_assert(!glGetError());
    alphablending_program_id = extptr_glCreateProgram();
    log_assert(alphablending_program_id > 0);
    log_assert(!glGetError());
    
    extptr_glAttachShader(alphablending_program_id, vertex_shader_id);
    log_assert(!glGetError());
    extptr_glAttachShader(
        alphablending_program_id,
        alphablending_fragment_shader_id);
    log_assert(!glGetError());
    
    extptr_glLinkProgram(alphablending_program_id);
    log_assert(!glGetError());
    
    success = 0;
    extptr_glGetProgramiv(
        alphablending_program_id,
        GL_LINK_STATUS,
        &success);
    if (!success) {
        extptr_glGetProgramInfoLog(
            alphablending_program_id,
            512,
            NULL,
            opengl_info_log);
        printf("ERROR - GL_LINK_STATUS: %s\n", opengl_info_log);
        getchar();
        return;
    }
}

void opengl_render_frame(GPUDataForSingleFrame * frame)
{
    log_assert(frame->vertices != NULL);
    log_assert(frame->light_collection != NULL);
    log_assert(frame->polygon_collection != NULL);
    log_assert(frame->camera != NULL);


    // extptr_glBindVertexArray(VAO);
    GLint error = glGetError();
    if (error != 0) {
        printf("glUseProgram threw error: %i\n", error);
        getchar();
        return;
    }
    
    glClearColor(0.0f, 0.05, 0.1f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    
    extptr_glUseProgram(diamond_program_id); // TODO: actually nescessary?
    extptr_glBindVertexArray(VAO);
    
    //extptr_glBindBuffer(vertex_VBO);
    //assert(!glGetError());
    //extptr_glBindBuffer(camera_VBO);
    //assert(!glGetError());
    //extptr_glBindBuffer(polygons_VBO);
    //assert(!glGetError());
    
    copy_single_frame_data(frame);
    
    if (frame->vertices_size > 0) {
        log_assert(frame->vertices_size % 3 == 0);
        //uint32_t triangles_size = frame->vertices_size / 3;
        //printf(
        //    "drawing %u triangles in %u meshes\n",
        //    triangles_size,
        //    frame->polygon_collection->size);
        glDisable(GL_BLEND);
        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA); 
        glDrawArrays(
            /* GLenum	mode : */
                GL_TRIANGLES,
            /* GLint	first: */
                0,
            /* GLsizei	count: */
                frame->first_alphablend_i);
        assert(!glGetError());
        
        extptr_glUseProgram(alphablending_program_id);
        glEnable(GL_BLEND);
        glDrawArrays(
            /* GLenum	mode : */
                GL_TRIANGLES,
            /* GLint	first: */
                frame->first_alphablend_i,
            /* GLsizei	count: */
                frame->first_line_i - frame->first_alphablend_i);
        assert(!glGetError());
        
        glDrawArrays(
            /* GLenum	mode : */
                GL_LINES,
            /* GLint	first: */
                frame->first_line_i,
            /* GLsizei	count: */
                frame->vertices_size - frame->first_line_i);
        assert(!glGetError());
    }
}

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
    
    extptr_glActiveTexture(GL_TEXTURE0 + texture_array_i);
    assert(glGetError() == 0);
    
    glBindTexture(
        GL_TEXTURE_2D_ARRAY,
        texture_array_ids[texture_array_i]);
    assert(glGetError() == 0);
    
    char name_in_shader[64];
    strcpy_capped(name_in_shader, 64, "texture_arrays[");
    strcat_uint_capped(name_in_shader, 64, texture_array_i);
    strcat_capped(name_in_shader, 64, "]");
    GLuint loc = extptr_glGetUniformLocation(
        diamond_program_id,
        name_in_shader);
    assert(glGetError() == 0);
    
    extptr_glUniform1iv(loc, 1, &texture_array_i);
    assert(glGetError() == 0);
    
    //There is also glTexStorage3D in openGL 4
    extptr_glTexImage3D(
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
    
    GLint err_value = glGetError();
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
    log_assert(image_width > 0);
    log_assert(image_height > 0);
    log_assert(parent_texture_array_images_size > texture_i);
    
    extptr_glActiveTexture(
        GL_TEXTURE0 + texture_array_i);
    assert(glGetError() == 0);
    
    glBindTexture(
        GL_TEXTURE_2D_ARRAY,
        texture_array_ids[texture_array_i]);
    assert(glGetError() == 0);
   
    extptr_glTexSubImage3D(
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
    
    GLint err_value = glGetError();
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

