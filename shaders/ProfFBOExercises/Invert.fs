#version 430 core
// 410 for mac

layout(location=0) out vec4 out_color;

in vec2 interUV;

uniform sampler2D screenTexture;

void main() {
    vec4 color = texture(screenTexture, interUV);
    color.rgb = 1.0f - color.rgb;
    out_color = color;
}
