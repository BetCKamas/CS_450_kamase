#include <iostream>
#include <fstream>
#include <sstream>
#include <thread>
#include <vector>
#include <algorithm>
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
#include "Shader.hpp"
#include "glm/gtc/matrix_transform.hpp"
#define GLM_ENABLE_EXPERIMENTAL
#include "glm/gtx/transform.hpp"
#include "glm/gtx/string_cast.hpp"
#include "glm/gtc/type_ptr.hpp"
#include "Utility.hpp"

using namespace std;

struct PointLight{
    glm::vec4 pos;
    glm::vec4 color;
};

float rotAngle = 0.0f;
glm::vec3 eye = glm::vec3(0,0,1);
glm::vec3 lookAtP = glm::vec3(0,0,0);
glm::vec2 mousePos;
PointLight light;
float metallic = 0.0f;
float roughness = 0.1f;
const float changeValue = 0.1f;


static void key_callback(GLFWwindow *window, int key, int scancode, int action, int mods){
    if(action == GLFW_PRESS || action == GLFW_REPEAT){
        if(key == GLFW_KEY_ESCAPE){
            glfwSetWindowShouldClose(window, true);
        } else if (key == GLFW_KEY_J){
            //cout << "ADDING" << endl;
            rotAngle += 1.0f;
        } else if (key == GLFW_KEY_K){
            //cout << "subtracting" << endl;
            rotAngle -= 1.0f;
        } else if (key == GLFW_KEY_W){
			glm::vec3 moveF = (lookAtP - eye);
			moveF = glm::normalize(moveF);
			glm::vec3 speed = moveF*0.1f;
			lookAtP += speed;
			eye += speed;
		} else if (key == GLFW_KEY_S){
			glm::vec3 moveB = (lookAtP - eye);
			moveB = glm::normalize(moveB);
			glm::vec3 speed = moveB*0.1f;
			lookAtP -= speed;
			eye -= speed;
		} else if (key == GLFW_KEY_D){
			glm::vec3 moveR = glm::cross((lookAtP - eye), glm::vec3(0,1,0));
			moveR = glm::normalize(moveR);
			glm::vec3 speed = moveR*0.1f;
			lookAtP += speed;
			eye += speed;
			
		} else if (key == GLFW_KEY_A){
			glm::vec3 moveL = glm::cross((lookAtP - eye), glm::vec3(0,1,0));
			moveL = glm::normalize(moveL);
			glm::vec3 speed = moveL*0.1f;
			lookAtP -= speed;
			eye -= speed;
		} else if (key == GLFW_KEY_1){
            light.color = glm::vec4(1,1,1,1); // white
		} else if (key == GLFW_KEY_2){
            light.color = glm::vec4(1,0,0,1); // red
		} else if (key == GLFW_KEY_3){
            light.color = glm::vec4(0,1,0,1); // green
		} else if (key == GLFW_KEY_4){
            light.color = glm::vec4(0,0,1,1); // blue
		} else if (key == GLFW_KEY_V){
            metallic = max(0.0f, (metallic-changeValue));
            //cout << metallic << endl;
        } else if (key == GLFW_KEY_B){
            metallic = min(1.0f, (metallic+changeValue));
            //cout << metallic << endl;
        } else if (key == GLFW_KEY_N){
            roughness = max(0.1f, (roughness-changeValue));
            //cout << roughness << endl;
        } else if (key == GLFW_KEY_M){
            roughness = min(0.7f, (roughness+changeValue));
            //cout << roughness << endl;
        }
        
    }
}

glm::mat4 makeLocalRotate(glm::vec3 offset, glm::vec3 axis, float angle){
    glm::mat4 t1 = glm::translate(glm::mat4(1.0), -offset);
    glm::mat4 r = glm::rotate(glm::mat4(1.0), glm::radians(angle), axis);
    glm::mat4 t2 = glm::translate(glm::mat4(1.0), offset);
    return t2*r*t1;
}

static void mouse_position_callback(GLFWwindow* window, double xpos, double ypos){
    int bW, bH;

    glm::vec2 relMousePos = mousePos - glm::vec2(xpos, ypos);
    glfwGetFramebufferSize(window, &bW, &bH);

    if(bW > 0 && bH > 0){
        float relScaledX = relMousePos.x / float(bW);
        float relScaledY = relMousePos.y / float(bH);

        glm::vec4 lookAtD = glm::vec4(lookAtP, 1.0);

        // for x motion, rotate around y (0,1,0)
        glm::mat4 xCamMove = makeLocalRotate(eye, glm::vec3(0,1,0), (30.0f*relScaledX));
        lookAtD = xCamMove*lookAtD;
		lookAtP = glm::vec3(lookAtD);

        // for y motion, rotate around local x cross of lookAt-eye & (0,1,0)
        glm::mat4 yCamMove = makeLocalRotate(eye, glm::cross((lookAtP-eye), glm::vec3(0,1,0)), 30.0f*relScaledY);
        lookAtD = yCamMove*lookAtD;
        lookAtP = glm::vec3(lookAtD);

    }

    mousePos = glm::vec2(xpos, ypos);
}

glm::mat4 makeRotateZ(glm::vec3 offset){
    glm::mat4 t1 = glm::translate(glm::mat4(1.0), -offset);
    glm::mat4 r = glm::rotate(glm::mat4(1.0), glm::radians(rotAngle), glm::vec3(0,0,1));
    glm::mat4 t2 =glm::translate(glm::mat4(1.0),offset);
    return t2*r*t1;
}

void renderScene(vector<MeshGL> &allMeshes, aiNode *node, glm::mat4 parentMat, GLint modelMatLoc, int level, GLint normMatLoc, glm::mat4 viewMat){
    glm::mat4 nodeT(1.0);
    aiMatToGLM4(node->mTransformation, nodeT);
    glm::mat4 modelMat = parentMat*nodeT;
    glm::mat4 tmpModel = modelMat;
    glm::vec3 pos = glm::vec3(modelMat[3]);
    glm::mat4 R(1.0);
    R = makeRotateZ(pos);
    tmpModel = R*modelMat;

    glm::mat3 normMat = glm::transpose(glm::inverse(glm::mat3(viewMat*tmpModel)));

    glUniformMatrix4fv(modelMatLoc, 1, GL_FALSE, glm::value_ptr(tmpModel));
    glUniformMatrix3fv(normMatLoc, 1, GL_FALSE, glm::value_ptr(normMat));

    for(int i = 0; i < node->mNumMeshes; i++){
        int index = node->mMeshes[i];
        drawMesh(allMeshes.at(index));
    }
    for(int i = 0; i < node->mNumChildren; i++){
        renderScene(allMeshes, node->mChildren[i], modelMat, modelMatLoc, level+1, normMatLoc, viewMat);
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
    m.vertices.push_back(upperLeft);    // 0
    m.vertices.push_back(topMiddle);    // 1
    m.vertices.push_back(upperRight);   // 2
    m.vertices.push_back(lowerLeft);    // 3
    m.vertices.push_back(lowerRight);   // 4

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
        current.color = glm::vec4(1.0, 1.0, 0.0, 1.0); // yellow
        current.normal = glm::vec3(mesh->mNormals[i].x, mesh->mNormals[i].y, mesh->mNormals[i].z);
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
    GLFWwindow* window = setupGLFW("Assign07: kamase", 4, 1, 800, 800, DEBUG_MODE);

    // GLEW setup
    setupGLEW(window);


    double mx, my;
    glfwGetCursorPos(window, &mx, &my);
    mousePos = glm::vec2(mx, my);


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

    glfwSetCursorPosCallback(window, mouse_position_callback);
    glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);


    // Set the background color to a shade of purple
    glClearColor(0.5f, 0.0f, 0.7f, 1.0f);

    // Create and load shaders
    GLuint programID = 0;
    try {
        // Load vertex shader code and fragment shader code
        string vertexCode = readFileToString("./shaders/Assign07/Basic.vs");
        string fragCode = readFileToString("./shaders/Assign07/Basic.fs");

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

    glfwSetKeyCallback(window, key_callback);

    light.pos = glm::vec4(0.5, 0.5, 0.5, 1);
    light.color = glm::vec4(1,1,1,1);

    GLint lightPosLoc = glGetUniformLocation(programID, "light.pos");
    GLint lightColorLoc = glGetUniformLocation(programID, "light.color");

    GLint roughnessLoc = glGetUniformLocation(programID, "roughness");
    GLint metallicLoc = glGetUniformLocation(programID, "metallic");

    GLint modelMatLoc = glGetUniformLocation(programID, "modelMat");
    GLint viewMatLoc = glGetUniformLocation(programID, "viewMat");
    GLint projMatLoc = glGetUniformLocation(programID, "projMat");
    GLint normMatLoc = glGetUniformLocation(programID, "normMat");
    
    vector<MeshGL> meshes;

    // Create OpenGL mesh (VAO) from data
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
        float aspectRatio;
        glfwGetFramebufferSize(window, &fwidth, &fheight);
        glViewport(0, 0, fwidth, fheight);

        // Clear the framebuffer
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        // Use shader program
        glUseProgram(programID);

        glUniform1f(metallicLoc, metallic);
        glUniform1f(roughnessLoc, roughness);


        glm::mat4 viewMat = glm::lookAt(eye, lookAtP, glm::vec3(0,1,0));
        glUniformMatrix4fv(viewMatLoc, 1, false, glm::value_ptr(viewMat));

        glm::vec4 lightPos = viewMat*light.pos;
        glUniform4fv(lightPosLoc, 1, glm::value_ptr(lightPos));
        glUniform4fv(lightColorLoc, 1, glm::value_ptr(light.color));


        if(fwidth > 0 && fheight > 0){
            aspectRatio = float(fwidth) / float(fheight);
        } else {
            aspectRatio = 1.0;
        }

        glm::mat4 projMat = glm::perspective(glm::radians(90.0f), aspectRatio, 0.01f, 50.0f);
        glUniformMatrix4fv(projMatLoc, 1, false, glm::value_ptr(projMat));

        // Draw object
        renderScene(meshes, object->mRootNode, glm::mat4(1.0), modelMatLoc, 0, normMatLoc, viewMat);

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

