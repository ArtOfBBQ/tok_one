#version 460 core

in vec2 vert_to_frag_uv;
in vec4 vert_to_frag_color;
flat in int vert_to_frag_texturearray_i;
flat in int vert_to_frag_texture_i;
in vec4 vert_to_frag_lighting;

out vec4 FragColor;

// Takes an extra 3rd texture coordinate (layer),
// which is just an index in the array
uniform sampler2DArray texture_array_1;

void main()
{
    FragColor = vert_to_frag_color; 
    
    if (
        vert_to_frag_texturearray_i < 0 ||
        vert_to_frag_texture_i < 0)
    {
        FragColor *= vert_to_frag_lighting;
    } else {
        
        vec4 texture_sample = texture(
            texture_array_1,
            vec3(vert_to_frag_uv, float(vert_to_frag_texture_i)));
        
        FragColor *= texture_sample;
        // FragColor *= vert_to_frag_lighting;
    }
    
    if (FragColor[3] < 0.01f) {
        discard;
        return;
    }
    
    // FragColor[3] = 1.0f;
}

