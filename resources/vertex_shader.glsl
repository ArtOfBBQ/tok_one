#version 460 core

layout (location = 0) in vec3 xyz;
layout (location = 1) in vec3 normal;
layout (location = 2) in vec2 uv;
layout (location = 3) in vec4 rgba;
layout (location = 4) in int texturearray_i;
layout (location = 5) in int texture_i;
layout (location = 6) in int polygon_i;

struct GPUVertex {
    int locked_vertex_i; // index into GPULockedVertex buffer
    int polygon_i;       // index into GPUPolygonCollection buffer
};

uniform GPUVertex vertices[MAX_VERTICES_PER_BUFFER];

struct GPULockedVertex {
    float        xyz       [3];
    float        normal_xyz[3];
    float        uv        [2];
    unsigned int parent_material_i;
};

uniform GPULockedVertex locked_vertices[ALL_LOCKED_VERTICES_SIZE];

struct GPUCamera {
    float x;
    float y;
    float z;
    float x_angle;
    float y_angle;
    float z_angle;
};

uniform GPUCamera camera;

struct GPUPolygon {
    float        xyz[3];
    float        xyz_angle[3];
    float        bonus_rgb[3];
    float        xyz_multiplier[3]; // determines width/height/depth
    float        xyz_offset[3];
    float        scale_factor;
    float        ignore_lighting;
    float        ignore_camera;
    float        simd_padding[6];
}; // 24 floats (3 SIMD runs)

// This wrapper struct doesn't help in OpenGL, just pass the member
//struct GPUPolygonCollection {
//    GPUPolygon   polygons[MAX_POLYGONS_PER_BUFFER];
//    unsigned int size;
//};

uniform GPUPolygon polygons[MAX_POLYGONS_PER_BUFFER];

struct GPUPolygonMaterial {
    float rgba[4];
    int   texturearray_i;
    int   texture_i;
    float simd_padding[2];
};

struct GPULightCollection {
    float        light_x[MAX_LIGHTS_PER_BUFFER];
    float        light_y[MAX_LIGHTS_PER_BUFFER];
    float        light_z[MAX_LIGHTS_PER_BUFFER];
    float        ambient[MAX_LIGHTS_PER_BUFFER];
    float        diffuse[MAX_LIGHTS_PER_BUFFER];
    float        reach  [MAX_LIGHTS_PER_BUFFER];
    float        red    [MAX_LIGHTS_PER_BUFFER];
    float        green  [MAX_LIGHTS_PER_BUFFER];
    float        blue   [MAX_LIGHTS_PER_BUFFER];
    unsigned int lights_size;
};

struct GPUProjectionConstants {
    float znear;
    float zfar;
    float q;
    float field_of_view_rad;
    float field_of_view_modifier;
    float x_multiplier;
    float y_multiplier;
};



//uniform zPolygon zpolygons[800];

//uniform float lights_x      [75];
//uniform float lights_y      [75];
//uniform float lights_z      [75];
//uniform float lights_ambient[75];
//uniform float lights_diffuse[75];
//uniform float lights_reach  [75];
//uniform float lights_red    [75];
//uniform float lights_green  [75];
//uniform float lights_blue   [75];
//uniform int lights_size;
//
//uniform vec3  camera_position;
//uniform vec3  camera_angle;
//uniform float projection_constants_near;
//uniform float projection_constants_q;
//uniform float projection_constants_fov_modifier;
//uniform float projection_constants_x_multiplier;

out vec2 vert_to_frag_uv;
out vec4 vert_to_frag_color;
flat out int vert_to_frag_texturearray_i;
flat out int vert_to_frag_texture_i;
out vec4 vert_to_frag_lighting;

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
        (vertices[0] * cos_angle) +
        (vertices[2] * sin_angle);
    rotated_vertices[2] =
        (vertices[2] * cos_angle) -
        (vertices[0] * sin_angle);
    
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
    uint polygon_i = vertices[gl_VertexID].polygon_i;
    uint locked_vertex_i = vertices[gl_VertexID].locked_vertex_i;
    uint locked_material_i = (polygon_i * MAX_MATERIALS_PER_POLYGON) +
        locked_vertices[locked_vertex_i].parent_material_i;
    
    vec4 parent_mesh_position = vec4(
        polygons[polygon_i].xyz[0],
        polygons[polygon_i].xyz[1],
        polygons[polygon_i].xyz[2],
        0.0f);
    
    vec4 mesh_vertices = vec4(
        locked_vertices[locked_vertex_i].xyz[0],
        locked_vertices[locked_vertex_i].xyz[1],
        locked_vertices[locked_vertex_i].xyz[2],
        0.0f);
    
    vec4 vertex_multipliers = vec4(
        polygons[polygon_i].xyz_multiplier[0],
        polygons[polygon_i].xyz_multiplier[1],
        polygons[polygon_i].xyz_multiplier[2],
        1.0f);
    
    vec4 vertex_offsets = vec4(
        polygons[polygon_i].xyz_offset[0],
        polygons[polygon_i].xyz_offset[1],
        polygons[polygon_i].xyz_offset[2],
        0.0f);
    
    mesh_vertices *= vertex_multipliers;
    mesh_vertices += vertex_offsets;
    
    mesh_vertices *= polygons[polygon_i].scale_factor;
    mesh_vertices[3] = 1.0f;
    
    vec4 mesh_normals = vec4(
        locked_vertices[locked_vertex_i].normal_xyz[0],
        locked_vertices[locked_vertex_i].normal_xyz[1],
        locked_vertices[locked_vertex_i].normal_xyz[2],
        1.0f);
    
    vec4 x_rotated_vertices = x_rotate(
        mesh_vertices,
        polygons[polygon_i].xyz_angle[0]);
    vec4 x_rotated_normals  = x_rotate(
        mesh_normals,
        polygons[polygon_i].xyz_angle[0]);
    
    vec4 y_rotated_vertices = y_rotate(
        x_rotated_vertices,
        polygons[polygon_i].xyz_angle[1]);
    vec4 y_rotated_normals  = y_rotate(
        x_rotated_normals,
        polygons[polygon_i].xyz_angle[1]);
    
    vec4 z_rotated_vertices = z_rotate(
        y_rotated_vertices,
        polygons[polygon_i].xyz_angle[2]);
    vec4 z_rotated_normals  = z_rotate(
        y_rotated_normals,
        polygons[polygon_i].xyz_angle[2]);
    
    vec4 translated_pos = z_rotated_vertices + parent_mesh_position;
    
    // translate to world position
    vec4 camera_translated_pos =
        translated_pos - vec4(camera.x, camera.y, camera.z, 0.0f);
    
    vec4 camera_x_rotated = x_rotate(
        camera_translated_pos,
        -camera.x_angle);
    vec4 camera_y_rotated = y_rotate(
        camera_x_rotated,
        -camera.y_angle);
    vec4 camera_z_rotated = z_rotate(
        camera_y_rotated,
        -camera.z_angle);
    
    float ignore_cam = polygons[polygon_i].ignore_camera;
    gl_Position =
        (camera_z_rotated * (1.0f - ignore_cam)) +
        (translated_pos * ignore_cam);
}

