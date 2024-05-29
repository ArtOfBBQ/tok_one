#version 460 core

layout (location=0) in unsigned int locked_vertex_i;
layout (location=1) in unsigned int polygon_i;

layout(location = 30) uniform vec3 camera_xyz;
layout(location = 31) uniform vec3 camera_xyz_angle;

struct GPUProjectionConstants {
    float znear;
    float zfar;
    float q;
    float field_of_view_rad;
    float field_of_view_modifier;
    float x_multiplier;
    float y_multiplier;
    float padding;
};
uniform GPUProjectionConstants projection_constants;

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
layout (std430, binding=2) buffer polygons_buffer
{
    GPUPolygon polygons[ALL_LOCKED_VERTICES_SIZE];
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
layout (std430, binding=3) buffer light_collection_buffer
{
    GPULightCollection light_collection;
};

struct GPULockedVertex {
    float        xyz       [3];     // 12 bytes
    float        normal_xyz[3];     // 12 bytes
    float        uv        [2];     //  8 bytes
    unsigned int parent_material_i; // 4 bytes
    float        padding[3];        // 12 bytes
};
layout (std430, binding=4) buffer locked_vertices_buffer
{
    GPULockedVertex locked_vertices[ALL_LOCKED_VERTICES_SIZE];
};

struct GPUPolygonMaterial {
    float rgba[4];
    int   texturearray_i;
    int   texture_i;
    float simd_padding[2];
};
layout (std430, binding=5) buffer polygon_materials_buffer
{
    GPUPolygonMaterial polygon_materials[MAX_POLYGONS_PER_BUFFER * MAX_MATERIALS_PER_POLYGON];
};

out  vec2 vert_to_frag_uv;
out  vec4 vert_to_frag_color;
flat out  int vert_to_frag_texturearray_i;
flat out  int vert_to_frag_texture_i;
out  vec4 vert_to_frag_lighting;

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
        translated_pos - vec4(
            camera_xyz[0],
            camera_xyz[1],
            camera_xyz[2],
            0.0f);
    
    vec4 camera_x_rotated = x_rotate(
        camera_translated_pos,
        -camera_xyz_angle[0]);
    vec4 camera_y_rotated = y_rotate(
        camera_x_rotated,
        -camera_xyz_angle[1]);
    vec4 camera_z_rotated = z_rotate(
        camera_y_rotated,
        -camera_xyz_angle[2]);
    
    float ignore_cam = polygons[polygon_i].ignore_camera;
    gl_Position =
        (camera_z_rotated * (1.0f - ignore_cam)) +
        (translated_pos * ignore_cam);
    
    // projection
    gl_Position[0] *= projection_constants.x_multiplier;
    gl_Position[1] *= projection_constants.field_of_view_modifier;
    gl_Position[3]  = gl_Position[2];
    gl_Position[2]  =     
        (gl_Position[2] * projection_constants.q) -
        (projection_constants.znear * projection_constants.q);
    
    vert_to_frag_color = vec4(
        polygon_materials[locked_material_i].rgba[0],
        polygon_materials[locked_material_i].rgba[1],
        polygon_materials[locked_material_i].rgba[2],
        polygon_materials[locked_material_i].rgba[3]);
    
    vec4 bonus_rgb = vec4(
        polygons[polygon_i].bonus_rgb[0],
        polygons[polygon_i].bonus_rgb[1],
        polygons[polygon_i].bonus_rgb[2],
        0.0f);
    
    vert_to_frag_color += bonus_rgb;
    
    vert_to_frag_texturearray_i =
        polygon_materials[locked_material_i].texturearray_i;
    
    vert_to_frag_texture_i =
        polygon_materials[locked_material_i].texture_i;
    
    vert_to_frag_uv = vec2(
        locked_vertices[locked_vertex_i].uv[0],
        locked_vertices[locked_vertex_i].uv[1]);
    
    vert_to_frag_lighting = vec4(
        0.0f, 0.0f, 0.0f, 0.0f);
    for (
        uint i = 0;
        i < light_collection.lights_size;
        i++)
    {
        // ambient lighting
        vec4 light_pos = vec4(
            light_collection.light_x[i],
            light_collection.light_y[i],
            light_collection.light_z[i],
            1.0f);
        vec4 light_color = vec4(
            light_collection.red[i],
            light_collection.green[i],
            light_collection.blue[i],
            1.0f);
        float distance = get_distance(
            light_pos,
            translated_pos);
        
        float distance_mod = (light_collection.reach[i] + 0.05f)
            - (distance * distance);
        distance_mod = clamp(distance_mod, 0.0f, 5.0f);
        
        vert_to_frag_lighting += (
            distance_mod *
            light_color *
            light_collection.ambient[i]);
        
        // diffuse lighting
        normalize(z_rotated_normals);
        
        vec4 vec_from_light_to_vertex = normalize(
            translated_pos - light_pos);
        
        float visibility_rating = max(
            0.0f,
            -1.0f * dot(
                z_rotated_normals,
                vec_from_light_to_vertex));
        
        vert_to_frag_lighting += (
           light_color *
           distance_mod *
           (light_collection.diffuse[i] * 3.0f) *
           visibility_rating);
    }
    
    vert_to_frag_lighting = clamp(vert_to_frag_lighting, 0.05f, 7.5f);

    float ignore_light = polygons[polygon_i].ignore_lighting;
    
    vec4 all_ones = vec4(1.0f, 1.0f, 1.0f, 1.0f);
    
    vert_to_frag_lighting =
        ((1.0f - ignore_light) * vert_to_frag_lighting) +
        (ignore_light * all_ones);
}

