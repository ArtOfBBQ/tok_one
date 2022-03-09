#version 330 core

typedef struct Vertex {
    float x;
    float y;
    float uv[2];
    float RGBA[4];
    float lighting;    // multiply by this lighting after
                       // color/texture
    int32_t texture_i; // -1 for no texture
}  Vertex;

typedef struct RasterizerPixel {
    vec4 position;
    vec4 color;
    vec2 texture_coordinate;
    float lighting;
    uint32_t texture_i;
} RasterizerPixel;

layout (location = 0) in Vertex vertex;
out RasterizerPixel;

void main()
{
    gl_Position = vec4(vertex.x, vertex.y, vertex.z, 1.0);
}

