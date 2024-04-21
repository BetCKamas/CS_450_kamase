#version 410 core
// Change to 410 for macOS

layout(location=0) out vec4 out_color;

in vec4 vertexColor; // Now interpolated across face
in vec4 interPos; 
in vec3 interNormal;

struct PointLight{
	vec4 pos;
	vec4 color;
};

uniform PointLight light;
uniform float metallic;
uniform float roughness;

const float PI = 3.14159265359;

vec3 getFresnelAtAngleZero(vec3 albedo, float metallic)
{


	vec3 F0 = vec3(0.04, 0.04, 0.04);
	F0 = mix(F0, albedo, metallic);

	return F0;
}

vec3 getFresnel(vec3 F0, vec3 L, vec3 H)
{
	float cosAngle = max(0, dot(L,H));
	
	vec3 FTheta = F0 + ((vec3(1.0) - F0)*(pow((1.0 - max(0.0, cos(cosAngle))), 5)));

	return FTheta;
}

float getNDF(vec3 H, vec3 N, float roughness)
{
	float alpha = pow(roughness, 2);
	float alphaSquared = pow(alpha, 2);
	float dotNHSquared = pow(dot(N,H), 2);

	float NDF = alphaSquared / (PI*pow((dotNHSquared*((alphaSquared - 1.0)+1.0)), 2));

	return NDF;
}

float getSchlickGeo(vec3 B, vec3 N, float roughness)
{
	float k = pow((roughness+1.0), 2) / 8.0;
	float sGeo = dot(N,B) / (dot(N,B)*(1.0-k)+k);
	return sGeo;
}

float getGF(vec3 L, vec3 V, vec3 N, float roughness)
{
	float GL = getSchlickGeo(L, N, roughness);
	float GV = getSchlickGeo(V, N, roughness);

	return GL*GV;

}

void main()
{	
	vec3 N = normalize(interNormal);

	vec3 lightPos = vec3(light.pos);

	vec3 L = lightPos - vec3(interPos);
	L = normalize(L);

	vec3 V = vec3(-interPos);
	V = normalize(V);

	vec3 H = normalize(V + L);

	vec3 F0 = getFresnelAtAngleZero(vec3(vertexColor), metallic);

	vec3 f = getFresnel(F0, L, H);

	vec3 kS = f;

	vec3 kD = 1.0 - kS;
	kD *= vec3(vertexColor);
	kD /= PI;

	float NDF = getNDF(H, N, roughness);
	float G = getGF(L, V, N, roughness);

	kS = kS*NDF*G;
	float denom = (4.0 * max(0, dot(N,L)) * max(0, dot(N,V))) + 0.0001;
	kS /= denom;

	vec3 finalColor = (kD+kS) * vec3(light.color) * max(0, dot(N,L));
	
	//out_color = vec4(0.04, 0.04, 0.04, 1.0);
	out_color = vec4(finalColor, 1.0);
	//out_color = vertexColor;
}
