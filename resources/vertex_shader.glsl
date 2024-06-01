#version 460 core
layout (location=0) in uvec2 invertexids;

out vec4 vertex_to_frag_color;
out vec4 vertex_to_frag_lighting;

struct GPUCamera {
    float xyz[3];           // 12 bytes
    float xyz_angle[3];     // 12 bytes
    float padding[2];       //  8 bytes
};
layout (std430, binding=2) buffer camera_buffer
{
    GPUCamera camera;
};

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
};
layout (std430, binding=3) buffer polygons_buffer
{
    GPUPolygon polygons[MAX_POLYGONS_PER_BUFFER];
};

struct GPULockedVertex {
    float        xyz       [3];     // 12 bytes
    float        normal_xyz[3];     // 12 bytes
    float        uv        [2];     // 8 bytes
    unsigned int parent_material_i; // 4 bytes
    float padding[3];               // 12 bytes of padding
};
layout (std430, binding=4) buffer locked_vertices_buffer
{
    GPULockedVertex locked_vertices[ALL_LOCKED_VERTICES_SIZE];
};

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
layout (std430, binding=5) buffer projection_constants_buffer
{
    GPUProjectionConstants projection_constants;
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
layout (std430, binding=6) buffer lights_buffer
{
    GPULightCollection light_collection;
};

vec4 x_rotate(vec4 vertices, float x_angle) {
    vec4 rotated_vertices = vertices;
    float cos_angle = cos(x_angle);
    float sin_angle = sin(x_angle);
    
    rotated_vertices[1] =
        (vertices[1] * cos_angle) -
        (vertices[2] * sin_angle);
    rotated_vertices[2] =
        (vertices[1] * sin_angle) +
        (vertices[2] * cos_angle);
    
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
    
    float sum_squares = dot(squared_diffs, vec4(1.0f,1.0f,1.0f,1.0f));
    
    return sqrt(sum_squares);
}

void main()
{
    unsigned int locked_vertex_i = invertexids[0];
    unsigned int polygon_i = invertexids[1];
    
    vec4 pos = vec4(
        locked_vertices[locked_vertex_i].xyz[0],
        locked_vertices[locked_vertex_i].xyz[1],
        locked_vertices[locked_vertex_i].xyz[2],
        1.0);

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
    
    pos *= vertex_multipliers;
    pos += vertex_offsets;
    
    pos *= polygons[polygon_i].scale_factor;
    pos[3] = 1.0f;
    
    vec4 normals = vec4(
        locked_vertices[locked_vertex_i].normal_xyz[0],
        locked_vertices[locked_vertex_i].normal_xyz[1],
        locked_vertices[locked_vertex_i].normal_xyz[2],
        1.0f);
    
    pos = x_rotate(
        pos,
        polygons[polygon_i].xyz_angle[0]);
    normals = x_rotate(
        normals,
        polygons[polygon_i].xyz_angle[0]);
    pos = y_rotate(
        pos,
        polygons[polygon_i].xyz_angle[1]);
    normals = y_rotate(
        normals,
        polygons[polygon_i].xyz_angle[1]);
    pos = z_rotate(
        pos,
        polygons[polygon_i].xyz_angle[2]);
    normals = z_rotate(
        normals,
        polygons[polygon_i].xyz_angle[2]);
    
    vec4 parent_pos = vec4(
        polygons[polygon_i].xyz[0],
        polygons[polygon_i].xyz[1],
        polygons[polygon_i].xyz[2],
        0.0);
    
    pos += parent_pos;
    vec4 translated_pos = pos;
    
    vec4 camera_position = vec4(
        camera.xyz[0],
        camera.xyz[1],
        camera.xyz[2],
        0.0f);
    vec4 camera_translated_pos = pos - camera_position;
    
    // rotate around camera
    camera_translated_pos = x_rotate(
        camera_translated_pos,
        -camera.xyz_angle[0]);
    camera_translated_pos = y_rotate(
        camera_translated_pos,
        -camera.xyz_angle[1]);
    camera_translated_pos = z_rotate(
        camera_translated_pos,
        -camera.xyz_angle[2]);
    
    float ignore_cam = polygons[polygon_i].ignore_camera;
    pos =
        (camera_translated_pos * (1.0f - ignore_cam)) +
        (pos * ignore_cam);
    
    // projection
    pos[0] *= projection_constants.x_multiplier;
    pos[1] *= projection_constants.field_of_view_modifier;
    pos[3]  = pos[2];
    pos[2]  =     
        (pos[2] * projection_constants.q) -
        (projection_constants.znear * projection_constants.q);
    
    gl_Position = pos;

    // Use materials here instead
    vertex_to_frag_color = vec4(
        0.4f,
        0.3f,
        0.4f,
        1.0f);
    
    vec4 bonus_rgb = vec4(
        polygons[polygon_i].bonus_rgb[0],
        polygons[polygon_i].bonus_rgb[1],
        polygons[polygon_i].bonus_rgb[2],
        0.0f);
    
    vertex_to_frag_color += bonus_rgb;
    
    vertex_to_frag_lighting = vec4(0.0f, 0.0f, 0.0f, 0.0f);
    
    for (
        unsigned int i = 0;
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
        
        vertex_to_frag_lighting += (
            distance_mod *
            light_color *
            light_collection.ambient[i]);

        // diffuse lighting
        normals = normalize(normals);
        
        vec4 vec_from_light_to_vertex = normalize(
            translated_pos - light_pos);
        
        float visibility_rating = max(
            0.0f,
            -1.0f * dot(
                normals,
                vec_from_light_to_vertex));
        
        vertex_to_frag_lighting += (
            light_color *
            distance_mod *
            (light_collection.diffuse[i] * 3.0f) *
            visibility_rating);
    }
    
    vertex_to_frag_lighting = clamp(
        vertex_to_frag_lighting, 0.05f, 7.5f);
    
    float ignore_light =
        polygons[polygon_i].ignore_lighting;
    
    vec4 all_ones = vec4(1.0f, 1.0f, 1.0f, 1.0f);
    vertex_to_frag_lighting =
        ((1.0f - ignore_light) * vertex_to_frag_lighting) +
        (ignore_light * all_ones);
};

