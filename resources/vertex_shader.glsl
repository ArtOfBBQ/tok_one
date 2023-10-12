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
out vec4 vertex_lighting;
// out float fragment_texturearray_i;
// out float fragment_texture_i;
// out vec2 fragment_uv;

struct GPUProjectionConstants {
    float near;
    float far;
    float q;
    float field_of_view_rad;
    float field_of_view_modifier;
    float x_multiplier;
    float y_multiplier;
};

uniform GPUProjectionConstants projection_constants;

vec4 x_rotate(vec4 vertices, float x_angle) {
    vec4 rotated_vertices = vertices;
    float cos_angle = cos(x_angle);
    float sin_angle = sin(x_angle);
    
    rotated_vertices[1] =
        vertices[1] * cos_angle -
        vertices[2] * sin_angle;
    rotated_vertices[2] =
        vertices[1] * sin_angle +
        vertices[2] * cos_angle;
    
    return rotated_vertices;
}

vec4 y_rotate(vec4 vertices, float y_angle) {
    vec4 rotated_vertices = vertices;
    float cos_angle = cos(y_angle);
    float sin_angle = sin(y_angle);
    
    rotated_vertices[0] =
        vertices[0] * cos_angle +
        vertices[2] * sin_angle;
    rotated_vertices[2] =
        vertices[2] * cos_angle -
        vertices[0] * sin_angle;
    
    return rotated_vertices;
}

vec4 z_rotate(vec4 vertices, float z_angle) {
    vec4 rotated_vertices = vertices;
    float cos_angle = cos(z_angle);
    float sin_angle = sin(z_angle);
    
    rotated_vertices[0] =
        vertices[0] * cos_angle -
        vertices[1] * sin_angle;
    rotated_vertices[1] =
        vertices[1] * cos_angle +
        vertices[0] * sin_angle;
    
    return rotated_vertices;
}

void main()
{
    vec4 parent_mesh_pos = vec4(parent_xyz, 0.0f);
    
    vec4 mesh_vertices = vec4(xyz, 1.0f);
    
    mesh_vertices *= scale_factor;
    mesh_vertices[3] = 1.0f;
    
    vec4 mesh_normals = vec4(normal, 1.0f);
    
    vec4 camera_position = vec4(0.0f, 0.0f, 0.0f, 0.0f);
    
    // rotate vertices
    vec4 x_rotated_vertices = x_rotate(
        mesh_vertices,
        parent_angle[0]);
    vec4 x_rotated_normals  = x_rotate(
        mesh_normals,
        parent_angle[0]);
    vec4 y_rotated_vertices = y_rotate(
        x_rotated_vertices,
        parent_angle[1]);
    vec4 y_rotated_normals  = y_rotate(
        x_rotated_normals,
        parent_angle[1]);
    vec4 z_rotated_vertices = z_rotate(
        y_rotated_vertices,
        parent_angle[2]);
    vec4 z_rotated_normals  = z_rotate(
        y_rotated_normals,
        parent_angle[2]);
    
    vec4 translated_pos = z_rotated_vertices + parent_mesh_pos;
    
    // projection
    gl_Position             = translated_pos;
    //gl_Position[0]       *= projection_constants.x_multiplier;
    //gl_Position[1]       *= projection_constants.field_of_view_modifier;
    //gl_Position[3]        = gl_Position[2];
    //gl_Position[2]        =     
    //    (gl_Position[2] * projection_constants.q) -
    //    (projection_constants.near * projection_constants.q);
    
    vertex_color = rgba;
    clamp(vertex_color, 0.10f, 1.0f);
    // out.texturearray_i = input_array[vertex_i].texturearray_i;
    // out.texture_i = input_array[vertex_i].texture_i;
    // out.texture_coordinate = vector_float2(
    //     input_array[vertex_i].uv[0],
    //     input_array[vertex_i].uv[1]);
    
    if (ignore_lighting > 0.0f) {
        vertex_lighting = vec4(1.0f, 1.0f, 1.0f, 1.0f);
        return;
    }
    
    clamp(vertex_lighting, 0.15f, 1.0f);
    vertex_lighting[3] = 1.0f;    
}

