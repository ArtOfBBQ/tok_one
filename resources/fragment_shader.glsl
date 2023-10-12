#version 330 core

in vec4 vertex_color;
in vec4 vertex_lighting;

out vec4 FragColor;

void main()
{
    // FragColor = vertex_color;
    FragColor = vec4(0.5f, vertex_color[1], vertex_color[2], 1.0f);
}

