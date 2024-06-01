#version 460 core

in  vec4 vertex_to_frag_color;
in  vec4 vertex_to_frag_lighting;
out vec4 out_color;

void main() {
    out_color = (vertex_to_frag_color * vertex_to_frag_lighting);
}

