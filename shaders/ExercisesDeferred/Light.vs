#version 430 core
// 410 for mac

layout(location=0) in vec3 position;
layout(location=3) in vec2 texcoord;

out vec2 interUV;

void main() {
    gl_Position = vec4(position, 1.0);    
    interUV = texcoord;
}

