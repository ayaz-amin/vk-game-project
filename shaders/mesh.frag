#version 450

layout(location=0) in vec2 tex_coords;

layout(binding=0) uniform sampler2D tex_sampler;

layout(location=0) out vec4 fragColor;

void main()
{
    fragColor = texture(tex_sampler, tex_coords);
}
