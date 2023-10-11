#version 330 core

layout (location = 0) in float x;
layout (location = 1) in float y;

// out vec4 vertex_color;
// out float fragment_lighting;
// out float fragment_texturearray_i;
// out float fragment_texture_i;
// out vec2 fragment_uv;

void main()
{
    // vertex_color = rgba;
    // fragment_lighting = lighting;
    // fragment_texturearray_i = texturearray_i;
    // fragment_texture_i = texture_i;
    // fragment_uv = vec2(uv[0], uv[1]);
    
    gl_Position = vec4(x, y, 0.8, 1.0);
}

