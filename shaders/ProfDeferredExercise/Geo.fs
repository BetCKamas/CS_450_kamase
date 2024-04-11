#version 430 core
// 410 mac

layout(location=0) out vec3 gPosition;
layout(location=1) out vec3 gNormal;
layout(location=2) out vec4 gAlbedoSpec;

in vec4 interColor;
in vec3 interPos;
in vec3 interNormal;
in vec2 interUV;
in vec3 interTangent;

uniform sampler2D diffuseTexture;
uniform sampler2D normalTexture;

void main() {
    vec3 N = normalize(interNormal);
    vec3 T = normalize(interTangent);
    T = normalize(T - dot(T,N)*N);
    vec3 B = normalize(cross(N,T));

    vec3 texN = vec3(texture(normalTexture, interUV));
    texN.x = (texN.x - 0.5)*2.0;
    texN.y = (texN.y - 0.5)*2.0;
    texN = normalize(texN);

    mat3 toView = mat3(T,B,N);

    N = normalize(toView*texN);

    vec3 texColor = vec3(texture(diffuseTexture, interUV));
    vec3 vertColor = vec3(interColor);

    vec3 albedo = vertColor; //texColor; // texColor*vertColor;

    gPosition = interPos;
    gNormal = N;
    gAlbedoSpec.rgb = albedo;
    gAlbedoSpec.a = 1;
}
