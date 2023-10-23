#version 460 core

// typedef struct GPUVertex {
//     float x;
//     float y;
//     float z;
//     /* Currently same for "entire" triangle, but who cares about 3 vertices */
//     float normal_x;
//     float normal_y;
//     float normal_z;
//     float uv  [2];
//     float RGBA[4];
//     int   texturearray_i; // -1 for no texture
//     int   texture_i;      // -1 for no texture
//     /* Same for parent */
//     float parent_x; // same for entire parent
//     float parent_y; // same for entire parent
//     float parent_z; // same for entire parent
//     float x_angle; // same for entire parent
//     float y_angle; // same for entire parent
//     float z_angle; // same for entire parent
//     float scale_factor; // same for entire parent
//     float ignore_lighting; // same for entire parent
//     float ignore_camera; // same for entire parent
//     int   touchable_id; // same for entire parent
// } GPUVertex;
layout (location =  0) in vec3 xyz;
layout (location =  1) in vec3 normal;
layout (location =  2) in vec2 uv;
layout (location =  3) in vec4 rgba;
layout (location =  4) in int texturearray_i;
layout (location =  5) in int texture_i;
layout (location =  6) in vec3 parent_xyz;
layout (location =  7) in vec3 parent_angle;
layout (location =  8) in float scale_factor;
layout (location =  9) in float ignore_lighting;
layout (location = 10) in float ignore_camera;
layout (location = 11) in int touchable_id;

out vec2 vert_to_frag_uv;
out vec4 vert_to_frag_color;
flat out int vert_to_frag_texturearray_i;
flat out int vert_to_frag_texture_i;
out vec4 vert_to_frag_lighting;

uniform vec3 camera_position;
uniform vec3 camera_angle;
uniform float projection_constants_near;
uniform float projection_constants_q;
uniform float projection_constants_fov_modifier;
uniform float projection_constants_x_multiplier;

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

float get_distance(
    vec4 a,
    vec4 b)
{
    vec4 squared_diffs = (a-b)*(a-b);
    
    float sum_squares = dot(
        squared_diffs,
        vec4(1.0f,1.0f,1.0f,1.0f));
    
    return sqrt(sum_squares);
}

void main()
{
    vec4 parent_mesh_pos = vec4(parent_xyz, 0.0f);
    
    vec4 mesh_vertices = vec4(xyz, 1.0f);
    
    mesh_vertices *= scale_factor;
    mesh_vertices[3] = 1.0f;
    
    vec4 mesh_normals = vec4(normal, 1.0f);
    
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
    
    // translate to world position
    if (ignore_camera < 0.9f) {
        vec4 camera_translated_pos =
            translated_pos - vec4(camera_position, 0.0f);
        
        vec4 camera_x_rotated = x_rotate(
            camera_translated_pos,
            -camera_angle[0]);
        vec4 camera_y_rotated = y_rotate(
            camera_x_rotated,
            -camera_angle[1]);
        vec4 camera_z_rotated = z_rotate(
            camera_y_rotated,
            -camera_angle[2]);
        
        gl_Position = camera_z_rotated;
        
    } else {
        gl_Position = translated_pos;
    }
    
    // projection
    gl_Position[0] = gl_Position[0] * projection_constants_x_multiplier;
    gl_Position[1] = gl_Position[1] * projection_constants_fov_modifier;
    gl_Position[3] = gl_Position[2];
    gl_Position[2] =
        (gl_Position[2] * projection_constants_q) -
        (projection_constants_near * projection_constants_q);
    
    vert_to_frag_color = rgba;
    
    vert_to_frag_uv = uv;
    
    vert_to_frag_texturearray_i = texturearray_i;
    vert_to_frag_texture_i = texture_i;
    
    if (ignore_lighting > 0.0f) {
        vert_to_frag_lighting = vec4(1.0f, 1.0f, 1.0f, 1.0f);
        return;
    }
    
    // init lighting
    vert_to_frag_lighting = vec4(0.0f, 0.0f, 0.0f, 1.0f);
    
    // add bonus for each light
    vert_to_frag_lighting[0] = 0.20f;
    vert_to_frag_lighting[1] = 0.20f;
    vert_to_frag_lighting[2] = 0.20f;
    
    // at the end
    clamp(vert_to_frag_lighting, 0.05f, 1.0f);
    vert_to_frag_lighting[3] = 1.0f;
}

