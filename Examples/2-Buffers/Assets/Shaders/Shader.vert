#version 450

layout(location = 0) in vec2 VertexPos;
layout(location = 1) in vec3 VertexColor;

layout(location = 0) out vec3 FragColor;

void main()
{
    gl_Position = vec4(VertexPos, 0.0, 1.0);
    FragColor = VertexColor;
}
