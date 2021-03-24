#version 450

vec2 Positions[3] = vec2[](
    vec2(0.f, -0.5f),
    vec2(0.5, 0.5),
    vec2(-0.5, 0.5)
);

vec3 Colors[3] = vec3[](
    vec3(1.0f, 0.0f, 0.0f),
    vec3(0.0f, 1.0f, 0.0f),
    vec3(0.0f, 0.0f, 1.0f)
);

layout(location = 0) out vec3 FragColor;

void main()
{
    gl_Position = vec4(Positions[gl_VertexIndex], 0.0, 1.0);
    FragColor = Colors[gl_VertexIndex];
}