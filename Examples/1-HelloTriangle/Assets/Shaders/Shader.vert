#version 450

vec2 VertexPos[3] = {
    vec2( 0.0, -0.5),
    vec2(-0.5,  0.5),
    vec2( 0.5,  0.5)
};

vec3 VertexColor[3] = {
    vec3(1.0, 0.0, 0.0),
    vec3(0.0, 1.0, 0.0),
    vec3(0.0, 0.0, 1.0)
};

layout(location = 0) out vec3 FragColor;

void main()
{
    gl_Position = vec4(VertexPos[gl_VertexIndex], 0.0, 1.0);
    FragColor = VertexColor[gl_VertexIndex];
}
