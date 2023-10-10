#include "opengl.h"

// We'll need these 2 identifiers while drawing
GLuint program_id;
unsigned int VAO;

void opengl_render_triangles(void) {
    
    // TODO: Learn exactly when nescessary, I hope we can just set & forget
    // glUseProgram(program_id);
    
    unsigned int VBO;
    glGenBuffers(1, &VBO);
    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    assert(sizeof(float) == 4); // x,y,uv,rgba,lighting
    assert(sizeof(uint32_t) == 4); // texture_i
    
    GPUVertex data[3];
    data[0].x = 0.25f;
    data[0].y = 0.75;
    data[0].z = 1.00f;
    data[1].x = 0.75f;
    data[1].y = 0.75f;
    data[1].z = 1.00f;
    data[2].x = 0.25f;
    data[2].y = 0.25f;
    data[2].z = 1.00f;
    
    glBufferData(
        /* target: */
            GL_ARRAY_BUFFER,
        /* size: */
            3,
        /* data: (to init with, or NULL to copy no data) */
            data,
        /* usage: */
            GL_DYNAMIC_DRAW);
    
    glDrawArrays(
        /* GLenum mode: */
            GL_TRIANGLES,
        /* GLint first: */
            0,
        /* GLint count: */
            3);
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
    // opengl_compile_shaders()...
    
    // allocate buffer memory...
    GLuint vertex_shader_id = glCreateShader(GL_VERTEX_SHADER);
    
    assert(vertex_shader_source_size > 0);
    assert(vertex_shader_source != NULL);

    opengl_compile_given_shader(
        /* GLuint shader_id: */
            vertex_shader_id,
        /* char * shader_source: */
            vertex_shader_source,
        /* GLint source_length: */
            vertex_shader_source_size);
    
    GLuint fragment_shader_id =
        glCreateShader(GL_FRAGMENT_SHADER);
    assert(fragment_shader_source_size > 0);
    assert(fragment_shader_source != NULL);
    opengl_compile_given_shader(
        /* GLuint shader_id: */
            fragment_shader_id,
        /* char * shader_source: */
            fragment_shader_source,
        /* GLint source_length: */
            fragment_shader_source_size);
    
    // attach compiled shaders to program
    program_id = glCreateProgram();
    glAttachShader(program_id, vertex_shader_id);
    glAttachShader(program_id, fragment_shader_id);
    glLinkProgram(program_id);
    
    GLint success = -1;
    glGetProgramiv(
        /* GLuint program id: */
            program_id,
        /* GLenum pname: */
            GL_LINK_STATUS,
        /* GLint * params: */
            &success);
    assert(success);
    
    // glDeleteShader(vertex_shader_id); // TODO: activate after everything works
    // glDeleteShader(fragment_shader_id); // TODO: activate after everything works
    
    glGenVertexArrays(1, &VAO);
    glBindVertexArray(VAO);
    
    // TODO: texture arrays
    //for (uint32_t i = 0; i < TEXTUREARRAYS_SIZE; i++) {
    //    glActiveTexture(GL_TEXTURE0 + i);
    //    
    //    glGenTextures(
    //        1,
    //        &texture_array_ids[i]);
    //    glBindTexture(
    //        GL_TEXTURE_2D_ARRAY,
    //        texture_array_ids[i]);
    //    
    //    glTexParameteri(
    //        GL_TEXTURE_2D_ARRAY,
    //        GL_TEXTURE_BASE_LEVEL,
    //        0); // single image
    //    glTexParameteri(
    //        GL_TEXTURE_2D_ARRAY,
    //        GL_TEXTURE_MAX_LEVEL,
    //        1); // single image
    //    glTexParameteri(
    //        GL_TEXTURE_2D_ARRAY,
    //        GL_TEXTURE_MAG_FILTER,
    //        GL_LINEAR);
    //    glTexParameteri(
    //        GL_TEXTURE_2D_ARRAY,
    //        GL_TEXTURE_MIN_FILTER,
    //        GL_LINEAR);
    //    glTexParameteri(
    //        GL_TEXTURE_2D_ARRAY,
    //        GL_TEXTURE_WRAP_S,
    //        GL_CLAMP_TO_EDGE);
    //    glTexParameteri(
    //        GL_TEXTURE_2D_ARRAY,
    //        GL_TEXTURE_WRAP_T,
    //        GL_CLAMP_TO_EDGE);
    //    
    //    assert(texture_arrays[i].image->width > 0);
    //    assert(texture_arrays[i].image->height > 0);
    //    
    //    uint32_t tiles_x =
    //        texture_arrays[i].sprite_columns;
    //    uint32_t tiles_y = texture_arrays[i].sprite_rows;
    //    uint32_t pixels_x =
    //        texture_arrays[i].image->width / tiles_x;
    //    uint32_t pixels_y =
    //        texture_arrays[i].image->height / tiles_y;
    //    uint32_t tile_count = tiles_x * tiles_y;
    //    
    //    glTexStorage3D(
    //        GL_TEXTURE_2D_ARRAY,
    //        1,
    //        GL_RGB8,
    //        pixels_x,
    //        pixels_y,
    //        tile_count);
    //   
    //    glPixelStorei(
    //        GL_UNPACK_ROW_LENGTH,
    //        texture_arrays[i].image->width);
    //    glPixelStorei(
    //        GL_UNPACK_IMAGE_HEIGHT,
    //        texture_arrays[i].image->height);

    //    for (GLsizei x = 0; x < tiles_x; x++) {
    //        for (GLsizei y = 0; y < tiles_y; y++) {
    //            glTexSubImage3D(
    //                GL_TEXTURE_2D_ARRAY,
    //                0,
    //                0,
    //                0,
    //                x * tiles_x + y,
    //                pixels_x,
    //                pixels_y,
    //                1,
    //                GL_RGBA,
    //                GL_UNSIGNED_BYTE,
    //                texture_arrays[i].image->rgba_values + (
    //                    (
    //                        x *
    //                        pixels_y *
    //                        texture_arrays[i].image->width +
    //                        y *
    //                        pixels_x
    //                    ) * 4));
    //        }
    //    }
    //    
    //    glUseProgram(program_id);
    //    
    //    char texture_name[11] = "texturemapX";
    //    texture_name[10] = '0' + i;
    //    glUniform1i(
    //        glGetUniformLocation(
    //            program_id,
    //            texture_name),
    //        0);
    //}
     
    /*
    Attribute pointers describe the fields of our data
    sructure (the Vertex struct in shared/vertex_types.h)
    */
    
    // struct field: float x;
    glVertexAttribPointer(
        /* location (in shader source): */
            0,
        /* array/vector element count: */
            1,
        /* type of data: */
            GL_FLOAT,
        /* normalize data: */
            GL_FALSE,
        /* stride to next 'x': */
            sizeof(GPUVertex),
        /* offset : */
            (void*)(offsetof(GPUVertex, x)));
    
    // struct field; float y;
    glVertexAttribPointer(
        /* location (in shader source): */
            1,
        /* array/vector size: */
            1,
        /* type of data: */
            GL_FLOAT,
        /* normalize data: */
            GL_FALSE,
        /* sizeof parent struct: */
            sizeof(GPUVertex),
        /* offset : */
            (void*)(offsetof(GPUVertex, y)));
    
    // struct field; float uv[2];
    glVertexAttribPointer(
        /* location (in shader source): */
            2,
        /* array/vector size: */
            2,
        /* type of data: */
            GL_FLOAT,
        /* normalize data: */
            GL_FALSE,
        /* sizeof parent struct: */
            sizeof(GPUVertex),
        /* offset : */
            (void*)(offsetof(GPUVertex, uv)));
    
    // struct field: float RGBA[4];
    glVertexAttribPointer(
        /* location (in shader source): */
            3,
        /* array/vector size: */
            4,
        /* type of data: */
            GL_FLOAT,
        /* normalize data: */
            GL_FALSE,
        /* sizeof parent struct: */
            sizeof(GPUVertex),
        /* offset : */
            (void*)(offsetof(GPUVertex, RGBA)));
    
    // struct field: float lightning:
    glVertexAttribPointer(
        /* location (in shader source): */
            4,
        /* array/vector size: */
            1,
        /* type of data: */
            GL_FLOAT,
        /* normalize data: */
            GL_FALSE,
        /* sizeof parent struct: */
            sizeof(GPUVertex),
        /* offset : */
            (void*)(offsetof(GPUVertex, RGBA)));
    
    glVertexAttribPointer(
        /* location (in shader source): */
            5,
        /* array/vector size: */
            1,
        /* type of data: */
            GL_INT,
        /* normalize data: */
            GL_FALSE,
        /* sizeof parent struct: */
            sizeof(GPUVertex),
        /* offset : */
            (void*)(offsetof(GPUVertex, texturearray_i)));
    
    glVertexAttribPointer(
        /* location (in shader source): */
            6,
        /* array/vector size: */
            1,
        /* type of data: */
            GL_INT,
        /* normalize data: */
            GL_FALSE,
        /* sizeof parent struct: */
            sizeof(GPUVertex),
        /* offset : */
            (void*)(offsetof(GPUVertex, texture_i)));
    
    glEnableVertexAttribArray(0);
    glEnableVertexAttribArray(1);
    glEnableVertexAttribArray(2);
    glEnableVertexAttribArray(3);
    glEnableVertexAttribArray(4);
    glEnableVertexAttribArray(5);
    glEnableVertexAttribArray(6);
}

