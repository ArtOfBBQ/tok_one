#version 330 core

layout (location = 0) in float x;
layout (location = 1) in float y;
layout (location = 2) in vec2 uv;
layout (location = 3) in vec4 rgba;
layout (location = 4) in float lighting;
layout (location = 5) in int texture_i;

out vec4 vertex_color;
out float fragment_lighting;
out int fragment_texture_i;
out vec2 fragment_uv;

void main()
{
    vertex_color = rgba;
    fragment_lighting = lighting;
    fragment_texture_i = texture_i;
    fragment_uv = uv;
    
    if (texture_i < 0) {
        gl_Position = vec4(x, y, 1.0, 1.0);
    } else {
        gl_Position = vec4(x / 5.0, y / 5, 1.0, 1.0);
    }
}

