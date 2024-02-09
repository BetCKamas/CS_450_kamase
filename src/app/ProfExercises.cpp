#include <iostream>
#include <cstring>
#include <thread>
#include <vector>
#include <GL/glew.h>
#include <GLFW/glfw3.h>
#include "Shader.hpp"
#include "glm/glm.hpp"
#include "glm/gtc/matrix_transform.hpp"
#define GLM_ENABLE_EXPERIMENTAL
#include "glm/gtx/string_cast.hpp"
#include "glm/gtx/transform.hpp"
#include "glm/gtc/type_ptr.hpp"
using namespace std;

glm::mat4 modelMat(1.0);
string transformString = "v";

void printRM(string name, glm::mat3 &m) {
    cout << name << ": " << endl;
    for(int i = 0; i < 3; i++) {
        for(int j = 0; j < 3; j++) {
            cout << m[j][i] << ",";
        }
        cout << endl;
    }
}

void printRM(string name, glm::mat4 &m) {
    cout << name << ": " << endl;
    for(int i = 0; i < 4; i++) {
        for(int j = 0; j < 4; j++) {
            cout << m[j][i] << ",";
        }
        cout << endl;
    }
}

static void key_callback(GLFWwindow *window,
                        int key, int scancode,
                        int action, int mods) {
    if(action == GLFW_PRESS || action == GLFW_REPEAT) {
        if(key == GLFW_KEY_ESCAPE) {
            glfwSetWindowShouldClose(window, true);
        }
        else if(key == GLFW_KEY_Q) {
            modelMat = glm::rotate(glm::radians(5.0f), glm::vec3(0,0,1))*modelMat;
            transformString = "R(+5)*" + transformString;
        }
        else if(key == GLFW_KEY_E) {
            modelMat = glm::rotate(glm::radians(-5.0f), glm::vec3(0,0,1))*modelMat;
            transformString = "R(-5)*" + transformString;
        }
        else if(key == GLFW_KEY_SPACE) {
            modelMat = glm::mat4(1.0);
            transformString = "v";
        }
        else if(key == GLFW_KEY_F) {
            modelMat = glm::scale(glm::vec3(0.8,1,1))*modelMat;
            transformString = "Sx(0.8)*" + transformString;
        }
        else if(key == GLFW_KEY_G) {
            modelMat = glm::scale(glm::vec3(1.25,1,1))*modelMat;
            transformString = "Sx(1.25)*" + transformString;
        }
        else if(key == GLFW_KEY_R) {
            modelMat = glm::scale(glm::vec3(1,0.8,1))*modelMat;
            transformString = "Sy(0.8)*" + transformString;
        }
        else if(key == GLFW_KEY_T) {
            modelMat = glm::scale(glm::vec3(1,1.25,1))*modelMat;
            transformString = "Sy(1.25)*" + transformString;
        }
        else if(key == GLFW_KEY_W) {
            modelMat = glm::translate(glm::vec3(0,0.1,0))*modelMat;
            transformString = "Ty(+0.1)*" + transformString;
        }
        else if(key == GLFW_KEY_S) {
            modelMat = glm::translate(glm::vec3(0,-0.1,0))*modelMat;
            transformString = "Ty(-0.1)*" + transformString;
        }
        else if(key == GLFW_KEY_A) {
            modelMat = glm::translate(glm::vec3(-0.1,0,0))*modelMat;
            transformString = "Tx(-0.1)*" + transformString;
        }
        else if(key == GLFW_KEY_D) {
            modelMat = glm::translate(glm::vec3(0.1,0,0))*modelMat;
            transformString = "Tx(+0.1)*" + transformString;
        }

        printRM("Model", modelMat);
        cout << transformString << endl;
    }
}

static void error_callback(int error, const char* desc) {
    cerr << "ERROR " << error << ": " << desc << endl;
}

int main(int argc, char **argv) {
    cout << "BEGIN OPENGL ADVENTURE!" << endl;

    glm::vec3 A(1,4,0);
    glm::vec3 B = glm::vec3(2,3,2);
    cout << "A: " << glm::to_string(A) << endl;
    cout << "B: " << glm::to_string(B) << endl;

    cout << "A.x: " << A.x << endl;

    glm::vec3 C = B - A;
    cout << "C: " << glm::to_string(C) << endl;

    float lenA = glm::length(A);
    cout << "Len(A): " << lenA << endl;
    glm::vec3 nA = glm::normalize(A);
    cout << "Normalized A: " << glm::to_string(nA) << endl;

    glm::vec3 E(7,8,0);
    glm::vec3 AcE = glm::cross(A, E);
    cout << "(A x E): " << glm::to_string(AcE) << endl;
    






    glfwSetErrorCallback(error_callback);
    if(!glfwInit()) {
        cerr << "ERROR: GLFW FAILED!" << endl;
        exit(1);
    }

    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3); // Mac: 1
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, true);

    glfwWindowHint(GLFW_OPENGL_DEBUG_CONTEXT, true);
	
    int frameWidth = 800;
    int frameHeight = 600;
    GLFWwindow *window = glfwCreateWindow(frameWidth, frameHeight, 
                                            "ProfExercises03", 
                                            NULL, NULL);

    if(!window) {
        cerr << "ERROR: WINDOW FAILED!" << endl;
        glfwTerminate();
        exit(1);
    }

    glfwMakeContextCurrent(window);
    glfwSwapInterval(1);

    glfwSetKeyCallback(window, key_callback);

    glewExperimental = true;
    GLenum err = glewInit();
    if(err != GLEW_OK) {
        cerr << "GLEW ERROR: ";
        cerr << glewGetErrorString(err) << endl;
        glfwTerminate();
        exit(1);
    }

    string vertCode = readFileToString("./shaders/ProfExercises/Simple.vs");
	string fragCode = readFileToString("./shaders/ProfExercises/Simple.fs");
    cout << vertCode << endl;
    cout << fragCode << endl;

    GLuint vertID = glCreateShader(GL_VERTEX_SHADER);
    GLuint fragID = glCreateShader(GL_FRAGMENT_SHADER);

    const char *vertPtr = vertCode.c_str();
    const char *fragPtr = fragCode.c_str();
    glShaderSource(vertID, 1, &vertPtr, NULL);
    glShaderSource(fragID, 1, &fragPtr, NULL);

    glCompileShader(vertID);
    glCompileShader(fragID);

    GLuint progID = glCreateProgram();
    glAttachShader(progID, vertID);
    glAttachShader(progID, fragID);
    glLinkProgram(progID);

    glDeleteShader(vertID);
    glDeleteShader(fragID);

    GLint modelMatLoc = glGetUniformLocation(progID, "modelMat");
    cout << "modelMatLoc: " << modelMatLoc << endl;

    vector<GLfloat> vertOnly = {
        -0.3f, -0.3f, 0.0f,
        0.3f, -0.3f, 0.0f,
        -0.3f, 0.3f, 0.0f,
        0.3f, 0.3f, 0.0f,
    };

    vector<GLuint> indices = { 0, 1, 2, 1, 3, 2 };
    int indexCnt = (int)indices.size();

    GLuint VBO = 0;
    GLuint EBO = 0;
    GLuint VAO = 0;

    glGenBuffers(1, &VBO);
    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    glBufferData(GL_ARRAY_BUFFER, vertOnly.size()*sizeof(float), 
                        vertOnly.data(), GL_STATIC_DRAW);
    glBindBuffer(GL_ARRAY_BUFFER, 0);

    
    glGenVertexArrays(1, &VAO);
    glBindVertexArray(VAO);
    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, false, 3*sizeof(float), 0);

    glGenBuffers(1, &EBO);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, indices.size()*sizeof(GLuint),
                    indices.data(), GL_STATIC_DRAW);
    

    glBindVertexArray(0);
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);

    cout << "VAO: " << VAO << endl;
    cout << "VBO: " << VBO << endl;
    cout << "EBO: " << EBO << endl;
    cout << "progID: " << progID << endl;




    glClearColor(1.0, 1.0, 0.0, 1.0);
    glEnable(GL_DEPTH_TEST);

    while(!glfwWindowShouldClose(window)) {
        glfwGetFramebufferSize(window, &frameWidth, &frameHeight);
        glViewport(0,0,frameWidth,frameHeight);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        glUseProgram(progID);

        glUniformMatrix4fv(modelMatLoc, 1, false, glm::value_ptr(modelMat));

        glBindVertexArray(VAO);
        glDrawElements(GL_TRIANGLES, indexCnt, 
                        GL_UNSIGNED_INT, (void*)0);
        glBindVertexArray(0);
        glUseProgram(0);

        glfwSwapBuffers(window);
        glfwPollEvents();
        this_thread::sleep_for(chrono::milliseconds(15));
    }

    glBindVertexArray(0);
    glDeleteVertexArrays(1, &VAO);
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
    glDeleteBuffers(1, &EBO);
    glDeleteBuffers(1, &VBO);

    glUseProgram(0);
    glDeleteProgram(progID);

    glfwDestroyWindow(window);
    glfwTerminate();

    return 0;
}