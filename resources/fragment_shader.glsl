#version 330 core

in vec4 vertex_color;
in float fragment_lighting;
in float fragment_texture_i;
in vec2 fragment_uv;

out vec4 FragColor;

uniform sampler2D texture0;
uniform sampler2D texture1;

void main()
{
    if (fragment_texture_i < 0) {
        FragColor = vertex_color;
    } else {
        
        FragColor =
            mix(
                texture(texture0, fragment_uv),
                texture(texture1, fragment_uv),
            0.5);
    }
    
    FragColor = FragColor * fragment_lighting;
}

