#version 330 core

in vec4 vertex_color;
in float fragment_lighting;
in float fragment_texturearray_i;
in float fragment_texture_i;
in vec2 fragment_uv;

out vec4 FragColor;

uniform sampler2DArray texturemap0;
uniform sampler2DArray texturemap1;

void main()
{
    if (fragment_texture_i < 0) {
        FragColor = vertex_color;
    } else {
        FragColor =
            texture(
                texturemap1,
                vec3(fragment_uv, fragment_texture_i));
    }
    
    FragColor = FragColor * fragment_lighting;
}

