#version 430 core
// 410 for mac

layout(location=0) out vec4 out_color;

in vec2 interUV;

uniform sampler2D screenTexture;

void main() {
    vec2 uv = vec2(interUV.x, 
                    interUV.y + 0.01*sin(interUV.x*100.0));
    
    out_color = texture(screenTexture, uv);
}
