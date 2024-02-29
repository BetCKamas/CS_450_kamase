#version 430 core
// 410 for mac

layout(location=0) in vec3 position;
layout(location=1) in vec4 color;
layout(location=2) in vec3 normal;
layout(location=3) in vec2 texcoord;

uniform mat4 modelMat;
uniform mat4 viewMat;
uniform mat4 projMat;
uniform mat3 normalMat;

out vec4 interColor;
out vec3 interPos;
out vec3 interNormal;
out vec2 interUV;

void main() {
    vec4 pos = vec4(position, 1.0);
    vec4 viewpos = viewMat*modelMat*pos;
    vec4 projpos = projMat*viewpos;

    interColor = color;
    interPos = vec3(viewpos);
    interNormal = normalMat*normal;

    interUV = texcoord;

    gl_Position = projpos;
}

