#version 410 core
// Change to 410 for macOS

layout(location=0) in vec3 position;
layout(location=1) in vec4 color;

uniform mat4 modelMat;
uniform mat4 viewMat;
uniform mat4 projMat;

out vec4 vertexColor;

void main()
{		
	// Get position of vertex (object space)
	vec4 objPos = vec4(position, 1.0);

	// transformations
	gl_Position = projMat*viewMat*modelMat*objPos;

	// Output per-vertex color
	vertexColor = color;
}
