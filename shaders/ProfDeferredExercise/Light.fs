#version 430 core
// 410 for mac

layout(location=0) out vec4 out_color;

in vec2 interUV;

uniform sampler2D gPosition;
uniform sampler2D gNormal;
uniform sampler2D gAlbedoSpec;

struct PointLight {
    vec4 pos;
    vec4 color;
};

const int LIGHT_CNT = 10;

uniform PointLight lights[LIGHT_CNT];

void main() {
    vec3 interPos = vec3(texture(gPosition, interUV));
    vec3 N = vec3(texture(gNormal, interUV));
    vec4 albedoSpec = texture(gAlbedoSpec, interUV);
    vec3 albedo = albedoSpec.rgb;
    float shininess = albedoSpec.a;

    vec3 finalColor = vec3(0,0,0);

    for(int i = 0; i < LIGHT_CNT; i++) {
        vec3 lightPos = vec3(lights[i].pos);
        vec3 lightColor = vec3(lights[i].color);

        vec3 L = lightPos - interPos;
        L = normalize(L);

        float diff = max(0, dot(L,N));
        vec3 diffColor = diff*albedo;
        finalColor += diffColor;
    }

    finalColor = finalColor / (finalColor + vec3(1.0));
    out_color = vec4(finalColor, 1.0);
    //vec3 color = albedo*vec3(lights[0].color);
    //out_color = vec4(color, 1);
}
