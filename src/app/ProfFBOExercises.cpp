#include <iostream>
#include <cstring>
#include <thread>
#include <vector>
#include <GL/glew.h>
#include <GLFW/glfw3.h>
#include "Shader.hpp"
#include "MeshData.hpp"
#include "glm/glm.hpp"
#include "glm/gtc/matrix_transform.hpp"
#define GLM_ENABLE_EXPERIMENTAL
#include "glm/gtx/string_cast.hpp"
#include "glm/gtx/transform.hpp"
#include "glm/gtc/type_ptr.hpp"
#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"
using namespace std;

glm::mat4 modelMat(1.0);
string transformString = "v";
glm::mat4 viewMat(1.0);
glm::mat4 projMat(1.0);
glm::vec2 lastMousePos(0,0);
bool leftMouseDown = false;

struct PointLight {
    glm::vec4 pos = glm::vec4(0,0,0,1);
    glm::vec4 color = glm::vec4(1,1,1,1);
};

PointLight light;

struct FBO {
    unsigned int ID;
    int width;
    int height;
    vector<unsigned int> colorIDs;
    unsigned int depthRBO;

    void clear() {
        ID = 0;
        width = 0;
        height = 0;
        colorIDs.clear();
        depthRBO = 0;
    };
};

unsigned int createColorAttachment(
    int width, int height, 
    int internal,
    int format, int type, 
    int texFilter, int colorAttach) {

    unsigned int texID = 0;
    glGenTextures(1, &texID);
    glBindTexture(GL_TEXTURE_2D, texID);
    glTexImage2D(GL_TEXTURE_2D, 0, internal, width, height, 0,
                        format, type, 0);
        
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, texFilter);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, texFilter);

    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0 + colorAttach,
                            GL_TEXTURE_2D, texID, 0);

    glBindTexture(GL_TEXTURE_2D, 0);
    return texID;
}

unsigned int createDepthRBO(int width, int height) {
    unsigned int rbo = 0;
    glGenRenderbuffers(1, &rbo);
    glBindRenderbuffer(GL_RENDERBUFFER, rbo);
    glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH24_STENCIL8, width, height);
    glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_STENCIL_ATTACHMENT,
                                    GL_RENDERBUFFER, rbo);
    glBindRenderbuffer(GL_RENDERBUFFER, 0);
    return rbo;
}

void createFBO(FBO &fboObj, int width, int height) {
    fboObj.clear();
    glGenFramebuffers(1, &(fboObj.ID));
    fboObj.width = width;
    fboObj.height = height;
    glBindFramebuffer(GL_FRAMEBUFFER, fboObj.ID);
    fboObj.colorIDs.push_back(createColorAttachment(width, height,
                                            GL_RGB, GL_RGB, GL_UNSIGNED_BYTE,
                                            GL_LINEAR, 0));
    fboObj.depthRBO = createDepthRBO(width, height);
    if(glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE) {
        cerr << "ERROR: Incomplete FBO!" << endl;
        fboObj.clear();
    }

    glBindFramebuffer(GL_FRAMEBUFFER, 0);                                            
}

static void mouse_button_callback(GLFWwindow *window, int button,
                                    int action, int mods) {
    if(action == GLFW_PRESS) {
        if(button == GLFW_MOUSE_BUTTON_LEFT) {
            leftMouseDown = true;
            cout << "LEFT MOUSE DOWN" << endl;
        }
    }
    else if(action == GLFW_RELEASE) {
        if(button == GLFW_MOUSE_BUTTON_LEFT) {
            leftMouseDown = false;
            cout << "LEFT MOUSE UP" << endl;
        }        
    }
}

static void mouse_motion_callback(GLFWwindow *window, double xpos, double ypos) {
    glm::vec2 mousePos = glm::vec2(xpos, ypos);
    glm::vec2 mouseDiff = mousePos - lastMousePos;

    int fw, fh;
    glfwGetFramebufferSize(window, &fw, &fh);

    if(fw > 0 && fh > 0) {
        mouseDiff.x /= fw;
        mouseDiff.y /= fh;

        mouseDiff.y = -mouseDiff.y;
    }       
    cout << "MOUSE DIFF: " << glm::to_string(mouseDiff) << endl;
    
    float angle = 2.0f*mouseDiff.x;
    glm::mat4 R;
    if(!leftMouseDown) {
        R = glm::rotate(angle, glm::vec3(0,1,0));
    }
    else {
        R = glm::rotate(angle, glm::vec3(1,0,0));
    }
    modelMat = R*modelMat;   
    
    lastMousePos = glm::vec2(xpos, ypos);
}

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

glm::vec3 computeNormal(glm::vec3 A, glm::vec3 B, glm::vec3 C) {
    glm::vec3 AB = B - A;
    glm::vec3 AC = C - A;
    glm::vec3 N = glm::cross(AB, AC);
    N = glm::normalize(N);
    return N;
}

void computeAllNormals(Mesh &m) {
    for(int i = 0; i < m.vertices.size(); i++) {
        m.vertices[i].normal = glm::vec3(0,0,0);
    }

    for(int i = 0; i < m.indices.size(); i += 3) {
        int ind0 = m.indices[i];
        int ind1 = m.indices[i+1];
        int ind2 = m.indices[i+2];

        glm::vec3 A = m.vertices[ind0].position;
        glm::vec3 B = m.vertices[ind1].position;
        glm::vec3 C = m.vertices[ind2].position;

        glm::vec3 N = computeNormal(A,B,C);

        m.vertices[ind0].normal += N;
        m.vertices[ind1].normal += N;
        m.vertices[ind2].normal += N;
    }

    for(int i = 0; i < m.vertices.size(); i++) {
        m.vertices[i].normal = glm::normalize(m.vertices[i].normal);
    }
}

void makeCylinder(Mesh &m, float length, float radius, int faceCnt) {
    m.vertices.clear();
    m.indices.clear();

    double angleInc = glm::radians(360.0/faceCnt);
    double halfLen = length/2.0;

    for(int i = 0; i < faceCnt; i++) {
        double angle = angleInc*i;
        double z = radius*cos(angle);
        double y = radius*sin(angle);
        glm::vec3 left = glm::vec3(-halfLen, y, z);
        glm::vec3 right = glm::vec3(+halfLen, y, z);
        Vertex vleft, vright;
        vleft.position = left;
        vright.position = right;
        vleft.color = glm::vec4(1,0,0,1);
        vright.color = glm::vec4(0,1,0,1);

        vleft.texcoord = glm::vec2(0.0f, 2.0f*((float)i)/faceCnt);
        vright.texcoord = glm::vec2(2.0f, 2.0f*((float)i)/faceCnt);

        m.vertices.push_back(vleft);
        m.vertices.push_back(vright);
    }

    int vcnt = m.vertices.size();

    for(int i = 0; i < faceCnt; i++) {
        int k = i*2;
        // 0,1,2     1,3,2
        m.indices.push_back(k);
        m.indices.push_back(k+1);
        m.indices.push_back((k+2)%vcnt);

        m.indices.push_back(k+1);
        m.indices.push_back((k+3)%vcnt);
        m.indices.push_back((k+2)%vcnt);
    }

    computeAllNormals(m);
}

unsigned int loadAndCreateTexture(string filename) {
    
    int texWidth, texHeight, texComponents;
    stbi_set_flip_vertically_on_load(true);
    unsigned char *imageData = stbi_load(filename.c_str(), 
                                            &texWidth, 
                                            &texHeight, 
                                            &texComponents, 0);
    unsigned int texID = 0;
    if(imageData) {
        glGenTextures(1, &texID);
        GLenum format;
        if(texComponents == 3) {
            format = GL_RGB;
            glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
        }
        else {
            format = GL_RGBA;
        }
        glBindTexture(GL_TEXTURE_2D, texID);
        glTexImage2D(GL_TEXTURE_2D, 0, format, texWidth, texHeight, 0,
                            format, GL_UNSIGNED_BYTE, imageData);
        
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

        glTexParameteri(GL_TEXTURE_2D, 
                        GL_TEXTURE_WRAP_S,
                        GL_REPEAT); 
                        //GL_MIRRORED_REPEAT);
        glTexParameteri(GL_TEXTURE_2D, 
                        GL_TEXTURE_WRAP_T, 
                        GL_REPEAT);
                        //GL_MIRRORED_REPEAT);
        glBindTexture(GL_TEXTURE_2D, 0);

        stbi_image_free(imageData);
    }
    else {
        cerr << "ERROR: Texture could not load!" << endl;
        glfwTerminate();
        exit(1);
    }

    return texID;
}

GLuint loadAndCreateShaderProgram(string vertFile, string fragFile) {
    
    string vertCode = readFileToString(vertFile);
	string fragCode = readFileToString(fragFile);
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

    return progID;
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

    double mx, my;
    glfwGetCursorPos(window, &mx, &my);
    lastMousePos = glm::vec2(mx, my);

    glfwSetInputMode(window, GLFW_CURSOR, 
                        GLFW_CURSOR_DISABLED);

    glfwSetKeyCallback(window, key_callback);
    glfwSetMouseButtonCallback(window, mouse_button_callback);
    glfwSetCursorPosCallback(window, mouse_motion_callback);

    glewExperimental = true;
    GLenum err = glewInit();
    if(err != GLEW_OK) {
        cerr << "GLEW ERROR: ";
        cerr << glewGetErrorString(err) << endl;
        glfwTerminate();
        exit(1);
    }

    GLuint progID = loadAndCreateShaderProgram("./shaders/ProfFBOExercises/Simple.vs",
                                                "./shaders/ProfFBOExercises/Simple.fs");

    GLuint quadProgID = loadAndCreateShaderProgram("./shaders/ProfFBOExercises/Quad.vs",
                                                "./shaders/ProfFBOExercises/Quad.fs");

    GLint modelMatLoc = glGetUniformLocation(progID, "modelMat");
    GLint viewMatLoc = glGetUniformLocation(progID, "viewMat");
    GLint projMatLoc = glGetUniformLocation(progID, "projMat");
    GLint normalMatLoc = glGetUniformLocation(progID, "normalMat");
    cout << "modelMatLoc: " << modelMatLoc << endl;
    cout << "viewMatLoc: " << viewMatLoc << endl;
    cout << "projMatLoc: " << projMatLoc << endl;
    cout << "normalMatLoc: " << normalMatLoc << endl;
    GLint lightPosLoc = glGetUniformLocation(progID, "light.pos");
    GLint lightColorLoc = glGetUniformLocation(progID, "light.color");

    glfwGetFramebufferSize(window, &frameWidth, &frameHeight);
    FBO fbo;
    createFBO(fbo, frameWidth, frameHeight);

    unsigned int diffTexID = loadAndCreateTexture("test.png");
    unsigned int normTexID = loadAndCreateTexture("normal.png");

    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, diffTexID);
    glActiveTexture(GL_TEXTURE1);
    glBindTexture(GL_TEXTURE_2D, normTexID);

    GLint diffuseTexLoc = glGetUniformLocation(progID, "diffuseTexture");
    GLint normalTexLoc = glGetUniformLocation(progID, "normalTexture");

    /*
    vector<GLfloat> vertOnly = {
        -0.3f, -0.3f, 0.0f,
        0.3f, -0.3f, 0.0f,
        -0.3f, 0.3f, 0.0f,
        0.3f, 0.3f, 0.0f,
    };
    */

   float quadScale = 1.0f; //0.3f;

    //vector<Vertex> vertOnly;
    Mesh quad;

    Vertex v0;
    v0.position = glm::vec3(-quadScale, -quadScale, 0.0f);
    v0.color = glm::vec4(0,1,0,1);
    v0.normal = glm::normalize(glm::vec3(-1,-1,1));
    quad.vertices.push_back(v0);

    Vertex v1;
    v1.position = glm::vec3(quadScale, -quadScale, 0.0f);
    v1.color = glm::vec4(0.5,0.5,0,1);
    v1.normal = glm::normalize(glm::vec3(1,-1,1));
    quad.vertices.push_back(v1);

    Vertex v2;
    v2.position = glm::vec3(-quadScale, quadScale, 0.0f);
    v2.color = glm::vec4(0,1,1,1);
    v2.normal = glm::normalize(glm::vec3(-1,1,1));
    quad.vertices.push_back(v2);

    Vertex v3;
    v3.position = glm::vec3(quadScale, quadScale, 0.0f);
    v3.color = glm::vec4(0,0,1,1);
    v3.normal = glm::normalize(glm::vec3(1,1,1));
    quad.vertices.push_back(v3);

    quad.indices = { 0, 1, 2, 1, 3, 2 };
    
    Mesh cylinder;
    makeCylinder(cylinder, 7.0, 2.0, 36);

    Mesh m = cylinder; //quad;
    int indexCnt = (int)m.indices.size();

    GLuint VBO = 0;
    GLuint EBO = 0;
    GLuint VAO = 0;

    glGenBuffers(1, &VBO);
    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    glBufferData(GL_ARRAY_BUFFER, m.vertices.size()*sizeof(Vertex), 
                        m.vertices.data(), GL_STATIC_DRAW);
    glBindBuffer(GL_ARRAY_BUFFER, 0);

    
    glGenVertexArrays(1, &VAO);
    glBindVertexArray(VAO);
    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    glEnableVertexAttribArray(0);
    glEnableVertexAttribArray(1);
    glEnableVertexAttribArray(2);
    glEnableVertexAttribArray(3);
    glEnableVertexAttribArray(4);
    glVertexAttribPointer(0, 3, GL_FLOAT, false, sizeof(Vertex), 
                            (void*)offsetof(Vertex, position));
    glVertexAttribPointer(1, 4, GL_FLOAT, false, sizeof(Vertex), 
                            (void*)offsetof(Vertex, color));
    glVertexAttribPointer(2, 3, GL_FLOAT, false, sizeof(Vertex), 
                            (void*)offsetof(Vertex, normal));
    glVertexAttribPointer(3, 2, GL_FLOAT, false, sizeof(Vertex), 
                            (void*)offsetof(Vertex, texcoord));
    glVertexAttribPointer(4, 3, GL_FLOAT, false, sizeof(Vertex), 
                            (void*)offsetof(Vertex, tangent));

    glGenBuffers(1, &EBO);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, m.indices.size()*sizeof(GLuint),
                    m.indices.data(), GL_STATIC_DRAW);
    

    glBindVertexArray(0);
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);

    cout << "VAO: " << VAO << endl;
    cout << "VBO: " << VBO << endl;
    cout << "EBO: " << EBO << endl;
    cout << "progID: " << progID << endl;




    glClearColor(1.0, 1.0, 0.0, 1.0);
    glEnable(GL_DEPTH_TEST);

    light.pos = glm::vec4(0, 20, 0, 1.0);

    while(!glfwWindowShouldClose(window)) {

        glBindFramebuffer(GL_FRAMEBUFFER, fbo.ID);

        glfwGetFramebufferSize(window, &frameWidth, &frameHeight);
        float aspect = 1.0f;
        if(frameHeight > 0) {
            aspect = ((float)frameWidth) / ((float)frameHeight);
        }
        float fov = glm::radians(90.0f);

        glViewport(0,0,frameWidth,frameHeight);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        glUseProgram(progID);

        glUniformMatrix4fv(modelMatLoc, 1, false, glm::value_ptr(modelMat));

        viewMat = glm::lookAt(glm::vec3(4,4,4), glm::vec3(0,0,0), glm::vec3(0,1,0));
        glUniformMatrix4fv(viewMatLoc, 1, false, glm::value_ptr(viewMat));

        projMat = glm::perspective(fov, aspect, 0.1f, 1000.0f);
        glUniformMatrix4fv(projMatLoc, 1, false, glm::value_ptr(projMat));

        glm::mat3 normalMat = glm::transpose(glm::inverse(glm::mat3(viewMat*modelMat)));
        glUniformMatrix3fv(normalMatLoc, 1, false, glm::value_ptr(normalMat));

        glm::vec4 lightPos = viewMat*light.pos;
        glUniform4fv(lightPosLoc, 1, glm::value_ptr(lightPos));
        glUniform4fv(lightColorLoc, 1, glm::value_ptr(light.color));

        glUniform1i(diffuseTexLoc, 0);
        glUniform1i(normalTexLoc, 1);

        glBindVertexArray(VAO);
        glDrawElements(GL_TRIANGLES, indexCnt, 
                        GL_UNSIGNED_INT, (void*)0);
        glBindVertexArray(0);
        glUseProgram(0);

        glfwSwapBuffers(window);
        glfwPollEvents();
        this_thread::sleep_for(chrono::milliseconds(15));
    }

    glDeleteFramebuffers(1, &(fbo.ID));

    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, 0);
    glActiveTexture(GL_TEXTURE1);
    glBindTexture(GL_TEXTURE_2D, 0);
    glDeleteTextures(1, &diffTexID);
    glDeleteTextures(1, &normTexID);

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