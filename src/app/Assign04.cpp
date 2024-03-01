#include <iostream>
#include <fstream>
#include <sstream>
#include <thread>
#include <vector>
#include <GL/glew.h>
#include <GLFW/glfw3.h>
#include <assimp/Importer.hpp>
#include <assimp/scene.h>
#include <assimp/postprocess.h>
#include "glm/glm.hpp"
#include "MeshData.hpp"
#include "MeshGLData.hpp"
#include "GLSetup.hpp"
#include "Shader.hpp"
#include "glm/gtc/matrix_transform.hpp"
#define GLM_ENABLE_EXPERIMENTAL
#include "glm/gtx/transform.hpp"
#include "glm/gtx/string_cast.hpp"
#include "glm/gtc/type_ptr.hpp"
#include "Utility.hpp"

using namespace std;

float rotAngle = 0.0f;

glm::mat4 makeRotateZ(glm::vec3 offset){

}

void renderScene(vector<MeshGL> &allMeshes, aiNode *node, glm::mat4 parentMat, GLint modelMatLoc, int level){

}
static void key_callback(GLFWwindow *window, int key, int scancode, int action, int mods){
    if(action == GLFW_PRESS || action == GLFW_REPEAT){
        if(key == GLFW_KEY_ESCAPE){
            glfwSetWindowShouldClose(window, true);
        } else if (key == GLFW_KEY_J){
            rotAngle += 1.0f;
        } else if (key == GLFW_KEY_K){
			rotAngle -= 1.0f;
		}

    }
}
// Create very simple mesh: a quad (4 vertices, 6 indices, 2 triangles)
void createSimpleQuad(Mesh &m) {
	// Clear out vertices and elements
	m.vertices.clear();
	m.indices.clear();

	// Create four corners
	Vertex upperLeft, upperRight;
	Vertex lowerLeft, lowerRight;

	// Set positions of vertices
	// Note: glm::vec3(x, y, z)
	upperLeft.position = glm::vec3(-0.5, 0.5, 0.0);
	upperRight.position = glm::vec3(0.5, 0.5, 0.0);
	lowerLeft.position = glm::vec3(-0.5, -0.5, 0.0);
	lowerRight.position = glm::vec3(0.5, -0.5, 0.0);

	// Set vertex colors (red, green, blue, white)
	// Note: glm::vec4(red, green, blue, alpha)
	upperLeft.color = glm::vec4(1.0, 0.0, 0.0, 1.0);
	upperRight.color = glm::vec4(0.0, 1.0, 0.0, 1.0);
	lowerLeft.color = glm::vec4(0.0, 0.0, 1.0, 1.0);
	lowerRight.color = glm::vec4(1.0, 1.0, 1.0, 1.0);

	// Add to mesh's list of vertices
	m.vertices.push_back(upperLeft);
	m.vertices.push_back(upperRight);
	m.vertices.push_back(lowerLeft);
	m.vertices.push_back(lowerRight);

	// Add indices for two triangles
	m.indices.push_back(0);
	m.indices.push_back(3);
	m.indices.push_back(1);

	m.indices.push_back(0);
	m.indices.push_back(2);
	m.indices.push_back(3);
}


// Create very simple mesh: a quad (4 vertices, 6 indices, 2 triangles)
void createSimplePentagon(Mesh &m) {
	// Clear out vertices and elements
	m.vertices.clear();
	m.indices.clear();

	// Create four corners
	Vertex upperLeft, upperRight;
	Vertex lowerLeft, lowerRight;

	// Create topMiddle point
	Vertex topMiddle;

	// Set positions of vertices
	// Note: glm::vec3(x, y, z)
	upperLeft.position = glm::vec3(-0.5, 0.5, 0.0);
	topMiddle.position = glm::vec3(0.0, 0.75, 0.0);
	upperRight.position = glm::vec3(0.5, 0.5, 0.0);
	lowerLeft.position = glm::vec3(-0.5, -0.5, 0.0);
	lowerRight.position = glm::vec3(0.5, -0.5, 0.0);

	// Set vertex colors (red, yellow, green, blue, white)
	// Note: glm::vec4(red, green, blue, alpha)
	upperLeft.color = glm::vec4(1.0, 0.0, 0.0, 1.0);
	topMiddle.color = glm::vec4(1.0, 1.0, 0.0 , 1.0);
	upperRight.color = glm::vec4(0.0, 1.0, 0.0, 1.0);
	lowerLeft.color = glm::vec4(0.0, 0.0, 1.0, 1.0);
	lowerRight.color = glm::vec4(1.0, 1.0, 1.0, 1.0);

	// Add to mesh's list of vertices
	m.vertices.push_back(upperLeft); 	// 0
	m.vertices.push_back(topMiddle);	// 1
	m.vertices.push_back(upperRight); 	// 2
	m.vertices.push_back(lowerLeft); 	// 3
	m.vertices.push_back(lowerRight);	// 4

	// Add indices for three triangles
	m.indices.push_back(0);
	m.indices.push_back(2);
	m.indices.push_back(1);
	
	m.indices.push_back(0);
	m.indices.push_back(4);
	m.indices.push_back(2);

	m.indices.push_back(0);
	m.indices.push_back(3);
	m.indices.push_back(4);

}

void extractMeshData(aiMesh *mesh, Mesh &m){
	m.vertices.clear();
	m.indices.clear();

	for(int i = 0; i < mesh->mNumVertices; i++){
		Vertex current;
		current.position = glm::vec3(mesh->mVertices[i].x, mesh->mVertices[i].y, mesh->mVertices[i].z);
		current.color = glm::vec4(0.0, 0.5, 0.5, 1.0);
		m.vertices.push_back(current);
	}

	for(int i = 0; i < mesh->mNumFaces; i++){
		aiFace face = mesh->mFaces[i];
		for(int j = 0; j < face.mNumIndices; j++){
			m.indices.push_back(face.mIndices[j]);
		}
	}
}

// Main
int main(int argc, char **argv) {
	string filename;

	if(argc >= 2){
		filename = argv[1];
	} else {
		filename = "sampleModels/sphere.obj";
	}
	
	// Are we in debugging mode?
	bool DEBUG_MODE = true;

	// GLFW setup
	// Switch to 4.1 if necessary for macOS
	GLFWwindow* window = setupGLFW("Assign04: kamase", 4, 1, 800, 800, DEBUG_MODE);

	// GLEW setup
	setupGLEW(window);

	// Check OpenGL version
	checkOpenGLVersion();

	// Assimp initialize 
	Assimp::Importer importer;
	const aiScene *object = importer.ReadFile(filename, aiProcess_Triangulate | aiProcess_FlipUVs |
								aiProcess_GenNormals | aiProcess_JoinIdenticalVertices);

	if(!object | object->mFlags & AI_SCENE_FLAGS_INCOMPLETE || !object->mRootNode) {
		cerr << "Error: " << importer.GetErrorString() << endl;
		exit(1);
	}


	// Set up debugging (if requested)
	if(DEBUG_MODE) checkAndSetupOpenGLDebugging();

	// Set the background color to a shade of purple
	glClearColor(0.5f, 0.0f, 0.7f, 1.0f);

	// Create and load shaders
	GLuint programID = 0;
	try {
		// Load vertex shader code and fragment shader code
		string vertexCode = readFileToString("./shaders/Assign04/Basic.vs");
		string fragCode = readFileToString("./shaders/Assign04/Basic.fs");

		// Print out shader code, just to check
		if(DEBUG_MODE) printShaderCode(vertexCode, fragCode);

		// Create shader program from code
		programID = initShaderProgramFromSource(vertexCode, fragCode);
	}
	catch (exception e) {
		// Close program
		cleanupGLFW(window);
		exit(EXIT_FAILURE);
	}

	// Create simple quad
	// Mesh m;
	// createSimpleQuad(m);
	// createSimplePentagon(m);

	// Create OpenGL mesh (VAO) from data
	// MeshGL mgl;
	// createMeshGL(m, mgl);

	vector<MeshGL> meshes;

	for(int i = 0; i < object->mNumMeshes; i++){
		Mesh m;
		MeshGL mgl;
		extractMeshData(object->mMeshes[i], m);
		createMeshGL(m, mgl);
		meshes.push_back(mgl);
	}

	// Enable depth testing
	glEnable(GL_DEPTH_TEST);

	while (!glfwWindowShouldClose(window)) {
		// Set viewport size
		int fwidth, fheight;
		glfwGetFramebufferSize(window, &fwidth, &fheight);
		glViewport(0, 0, fwidth, fheight);

		// Clear the framebuffer
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

		// Use shader program
		glUseProgram(programID);

		// Draw object
		// drawMesh(mgl);
		for(int i = 0; i < meshes.size(); i++){
			drawMesh(meshes[i]);
		}

		// Swap buffers and poll for window events
		glfwSwapBuffers(window);
		glfwPollEvents();

		// Sleep for 15 ms
		this_thread::sleep_for(chrono::milliseconds(15));
	}

	// Clean up mesh
	// cleanupMesh(mgl);

	for(int i = 0; i < meshes.size(); i++){
		cleanupMesh(meshes[i]);
	}

	meshes.clear();

	// Clean up shader programs
	glUseProgram(0);
	glDeleteProgram(programID);

	// Destroy window and stop GLFW
	cleanupGLFW(window);


	return 0;
}
