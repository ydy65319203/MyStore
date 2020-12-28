#version 330
layout(location = 0) in vec3 vec3VertexPos;
layout(location = 1) in vec2 vec2TexurePos;

out vec2 vec2TexCoord;

uniform mat4 mat4MVP;

void main()
{
    gl_Position = mat4MVP * vec4(vec3VertexPos, 1.0f);
    vec2TexCoord = vec2TexurePos;
}
