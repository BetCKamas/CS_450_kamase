#version 430 core
// 410 for mac

layout(location=0) in vec3 position;

void main() {
    gl_Position = vec4(position, 1.0);
}

