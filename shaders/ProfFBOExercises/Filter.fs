#version 430 core
// 410 for mac

layout(location=0) out vec4 out_color;

in vec2 interUV;

uniform sampler2D screenTexture;

const float off = 1.0/300.0;
vec2 offsets[9] = vec2[](
    vec2(-off, -off),
    vec2(-off, 0),
    vec2(-off, off),

    vec2(0, -off),
    vec2(0, 0),
    vec2(0, off),

    vec2(off, -off),
    vec2(off, 0),
    vec2(off, off)
);

float sharp_kernel[9] = float[] (
    -1, -1, -1,
    -1, 9, -1,
    -1, -1, -1
);

float edge_kernel[9] = float[] (
    -1, 0, +1,
    -2, 0, +2,
    -1, 0, +1
);

void main() {
    float kernel[9] = sharp_kernel;
    //float kernel[9] = edge_kernel;

    vec4 samples[9] = vec4[] (
        kernel[0]*texture(screenTexture, interUV + offsets[0]),
        kernel[1]*texture(screenTexture, interUV + offsets[1]),
        kernel[2]*texture(screenTexture, interUV + offsets[2]),

        kernel[3]*texture(screenTexture, interUV + offsets[3]),
        kernel[4]*texture(screenTexture, interUV + offsets[4]),
        kernel[5]*texture(screenTexture, interUV + offsets[5]),

        kernel[6]*texture(screenTexture, interUV + offsets[6]),
        kernel[7]*texture(screenTexture, interUV + offsets[7]),
        kernel[8]*texture(screenTexture, interUV + offsets[8])
    );

    vec4 final_color = samples[0] + samples[1] + samples[2]
                        + samples[3] + samples[4] + samples[5]
                        + samples[6] + samples[7] + samples[8];
    
    out_color = vec4(final_color.rgb, 1.0);
}
