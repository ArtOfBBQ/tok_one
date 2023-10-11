#version 330 core

layout (location = 0) in vec3 xyz;
layout (location = 1) in vec3 normal;
layout (location = 2) in vec2 uv;
layout (location = 3) in vec4 rgba;
layout (location = 4) in float texturearray_i;
layout (location = 5) in float texture_i;
layout (location = 6) in vec3 parent_xyz;
layout (location = 7) in vec3 parent_angle;
layout (location = 8) in float scale_factor;
layout (location = 9) in float ignore_lighting;
layout (location = 10) in float ignore_camera;
layout (location = 11) in float touchable_id;

out vec4 vertex_color;
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
    
    gl_Position = vec4(xyz, 1.0);
    vertex_color = rgba;
}

