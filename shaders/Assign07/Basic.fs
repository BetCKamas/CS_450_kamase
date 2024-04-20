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

void main()
{	
	vec3 N = normalize(interNormal);

	vec3 lightPos = vec3(light.pos);

	vec3 L = lightPos - vec3(interPos);
	L = normalize(L);

	vec3 V = vec3(-interPos);
	V = normalize(V);

	vec3 H = normalize(V + L);
	float hvr = max(0, dot(N,H));
	float shininess = 10.0;
	float shinyPower = pow(hvr, shininess);

	float diffuseCoef = max(0, dot(N,L));
	vec3  diffColor = vec3(diffuseCoef * vertexColor * light.color);

	vec3 kd = vec3(vertexColor);
	vec3 I = vec3(light.color);
	vec3 ks = vec3(1.0,1.0,1.0);

	float specularCoef = pow(max(0,dot(N,H)), shininess); 
	vec3 specColor = kd*I*(max(0, dot(N,L))) + ks*I*(max(0, dot(N,L))*shinyPower);
	

	out_color = vec4(diffColor + specColor, 1.0);
}
