#include <iostream>
#include <cstring>
#include <thread>

#include <GL/glew.h>
#include <GLFW/glfw3.h>

#include "Shader.hpp"

using namespace std;

static void error_callback(int error, const char* desc){
    cerr << "ERROR " << error << ": " << desc << endl;
}

int main(int argc, char **argv){
    cout << "BEGIN OPENGL ADVENTURE" << endl;
    glfwSetErrorCallback(error_callback);

    if(!glfwInit()){
        cerr << "ERROR: GLFW FAILED!" << endl;
        exit(1);
    }

    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 1);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, true);

    int frameWidth = 800;
    int frameHeight = 600;

    GLFWwindow *window = glfwCreateWindow(frameWidth, frameHeight, "Exercises03", NULL, NULL);

    if(!window){
        cerr << "ERROR WINDOW FAILED!" << endl;
        glfwTerminate();
        exit(1);
    }

    glfwMakeContextCurrent(window);
    glfwSwapInterval(1);

    glewExperimental = true;
    GLenum err = glewInit();
    if(err != GLEW_OK){
        cerr << "GLEW ERROR!" << endl;
        cerr << glewGetErrorString(err) << endl;
        glfwTerminate();
        exit(1);
    }

    string vertCode = readFileToString("./shaders/Exercises03/Simple.vs");
	string fragCode = readFileToString("./shaders/Exercises03/Simple.fs");
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
    
    glClearColor(0.3, 0.3, 0.0, 1.0);
    glEnable(GL_DEPTH_TEST);

    while(!glfwWindowShouldClose(window)){
        glfwGetFramebufferSize(window, &frameWidth, &frameHeight);
        glViewport(0, 0, frameWidth, frameHeight);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        glUseProgram(progID);
        glfwSwapBuffers(window);
        glfwPollEvents();
        this_thread::sleep_for(chrono::milliseconds(15));
    }

    glUseProgram(0);
    glDeleteProgram(progID);

    glfwDestroyWindow(window);
    glfwTerminate();

    return 0;
}

