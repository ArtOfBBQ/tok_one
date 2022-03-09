#version 330 core

in RasterizerPixel rasterizer_pixel;
out vec4 frag_color;

void main()
{
    frag_color = vec4(
        rasterizer_pixel.color[0]
            * rasterizer_pixel.lighting,
        rasterizer_pixel.color[1]
            * rasterizer_pixel.lighting,
        rasterizer_pixel.color[2]
            * rasterizer_pixel.lighting,
        rasterizer_pixel.color[3]);
}

