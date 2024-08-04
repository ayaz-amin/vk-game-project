#version 450

layout(location=0) in vec3 position;
layout(location=1) in vec3 color;
layout(location=2) in vec2 tex_coords;

layout(push_constant) uniform constants
{
    mat4 model;
    mat4 view_proj;
} pc;

layout(location=0) out vec2 out_coords;

void main()
{
    gl_Position = pc.view_proj * pc.model * vec4(position, 1.0);
    out_coords = tex_coords;
}
