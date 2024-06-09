#version 460 core

in vec2 vert_to_frag_uv;
in vec4 vert_to_frag_color;
in vec4 vert_to_frag_lighting;
flat in int vert_to_frag_texturearray_i;
flat in int vert_to_frag_texture_i;
out vec4 out_color;

// Takes an extra 3rd texture coordinate (layer),
// which is just an index in the array
uniform sampler2DArray texture_arrays[32];

void main() {
    out_color = vert_to_frag_color; 
    
    if (
        vert_to_frag_texturearray_i >= 0 &&
        vert_to_frag_texture_i >= 0)
    {
        vec4 texture_sample = texture(
            texture_arrays[vert_to_frag_texturearray_i],
            vec3(vert_to_frag_uv, float(vert_to_frag_texture_i)));
        
        out_color *= texture_sample;
    }
}

