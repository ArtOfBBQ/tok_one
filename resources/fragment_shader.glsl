#version 330 core

in vec4 vertex_color;
in float fragment_lighting;
// in int fragment_texture_i;
// in vec2 fragment_uv;

out vec4 FragColor;

void main()
{
    FragColor = vertex_color * fragment_lighting;
}

