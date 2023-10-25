#version 460 core

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

uniform float lights_x[50];
uniform float lights_y[50];
uniform float lights_z[50];
uniform float lights_ambient[50];
uniform float lights_diffuse[50];
uniform float lights_reach[50];
uniform float lights_red[50];
uniform float lights_green[50];
uniform float lights_blue[50];
uniform int lights_size;

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
    for (
        int i = 0;
        i < lights_size;
        i++)
    {
        // ambient lighting
        vec4 light_pos = vec4(lights_x[i], lights_y[i], lights_z[i], 1.0f);
        vec4 light_rgba = vec4(
            lights_red[i],
            lights_green[i],
            lights_red[i],
            1.0f);
        float distance = get_distance(
            light_pos,
            translated_pos);
        float distance_mod = (lights_reach[i] + 0.5f)
            - (distance * distance);
        distance_mod = clamp(distance_mod, 0.0f, 5.0f);
        
        vert_to_frag_lighting +=
            (light_rgba * distance_mod * lights_ambient[i]);
        
        // diffuse lighting
        vec4 normalized_normals = normalize(z_rotated_normals);
        
        vec4 vec_from_light_to_vertex = normalize(
            translated_pos - light_pos);
        
        float dot = dot(
            normalized_normals,
            vec_from_light_to_vertex);
        float visibility_rating = max(
            0.0f,
            -1.0f * dot);
        
         vec4 lighting_to_add = (
            light_rgba *
            distance_mod *
            (lights_diffuse[i] * 3.0f) *
            visibility_rating);
        
        vert_to_frag_lighting += lighting_to_add;
    }
    
    // at the end
    clamp(vert_to_frag_lighting, 0.05f, 1.0f);
    vert_to_frag_lighting[3] = 1.0f;
}

