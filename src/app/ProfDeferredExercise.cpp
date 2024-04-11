#include <iostream>
#include <cstring>
#include <thread>
#include <vector>
#include <GL/glew.h>
#include <GLFW/glfw3.h>
#include "Shader.hpp"
#include "MeshData.hpp"
#include "MeshGLData.hpp"
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
    GLint posLoc = -1;
    GLint colorLoc = -1;
};

const int LIGHT_CNT = 10;
PointLight lights[LIGHT_CNT];

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

struct GBuffer {
    FBO fbo;
    vector<int> locs;

    void startGeometry() {
        glBindFramebuffer(GL_FRAMEBUFFER, fbo.ID);
        //cout << "fbo.ID: " << fbo.ID << endl;
    };

    void endGeometry() {
        glBindFramebuffer(GL_FRAMEBUFFER, 0);
    };

    void startLighting() {
        for(int i = 0; i < locs.size(); i++) {
            glActiveTexture(GL_TEXTURE0 + i);
            glBindTexture(GL_TEXTURE_2D, fbo.colorIDs.at(i));
            glUniform1i(locs.at(i), i);
            //cout << "colorIDs: " << fbo.colorIDs.at(i) << endl;
            //cout << "locs: " << locs.at(i) << endl;
        }
    };

    void endLighting() {
        for(int i = 0; i < locs.size(); i++) {
            glActiveTexture(GL_TEXTURE0 + i);
            glBindTexture(GL_TEXTURE_2D, 0);
        }
    };

    void cleanup() {
        glDeleteFramebuffers(1, &(fbo.ID));
        fbo.clear();
        locs.clear();
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

void createGBuffer(GBuffer &gb, int width, int height, int lightProgID, 
                    string *uniformNames) {    
    glGenFramebuffers(1, &(gb.fbo.ID));
    gb.fbo.width = width;
    gb.fbo.height = height;
    glBindFramebuffer(GL_FRAMEBUFFER, gb.fbo.ID);
    for(int i = 0; i < 2; i++) {
        gb.fbo.colorIDs.push_back(createColorAttachment(width, height,
                                                GL_RGBA16F, GL_RGBA, 
                                                GL_FLOAT,
                                                GL_NEAREST, i));
    }

    gb.fbo.colorIDs.push_back(createColorAttachment(width, height,
                                                GL_RGBA, GL_RGBA, 
                                                GL_UNSIGNED_BYTE,
                                                GL_NEAREST, 2));

    // glDrawBuffers(3, gb.fbo.colorIDs.data());
    unsigned int attachments[3] = { GL_COLOR_ATTACHMENT0, 
                                    GL_COLOR_ATTACHMENT1, 
                                    GL_COLOR_ATTACHMENT2 };
    glDrawBuffers(3, attachments);

    for(int i = 0; i < gb.fbo.colorIDs.size(); i++) {
        gb.locs.push_back(
            glGetUniformLocation(lightProgID, uniformNames[i].c_str())
        );
    }
    
    gb.fbo.depthRBO = createDepthRBO(width, height);
    if(glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE) {
        cerr << "ERROR: Incomplete GBuffer::FBO!" << endl;        
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
        vright.color = glm::vec4(1,1,0,1);

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

    // Load vertex shader code and fragment shader code
    string vertexCode = readFileToString(vertFile);
    string fragCode = readFileToString(fragFile);

    // Print out shader code, just to check
    printShaderCode(vertexCode, fragCode);

	// Create shader program from code
	GLuint programID = initShaderProgramFromSource(vertexCode, fragCode);

    return programID;
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

    GLuint geoProgID = loadAndCreateShaderProgram(
        "./shaders/ProfDeferredExercise/Geo.vs",
        "./shaders/ProfDeferredExercise/Geo.fs");

    GLuint lightProgID = loadAndCreateShaderProgram(
        "./shaders/ProfDeferredExercise/Light.vs",                                                
        "./shaders/ProfDeferredExercise/Light.fs");

    GLint modelMatLoc = glGetUniformLocation(geoProgID, "modelMat");
    GLint viewMatLoc = glGetUniformLocation(geoProgID, "viewMat");
    GLint projMatLoc = glGetUniformLocation(geoProgID, "projMat");
    GLint normalMatLoc = glGetUniformLocation(geoProgID, "normalMat");
    cout << "modelMatLoc: " << modelMatLoc << endl;
    cout << "viewMatLoc: " << viewMatLoc << endl;
    cout << "projMatLoc: " << projMatLoc << endl;
    cout << "normalMatLoc: " << normalMatLoc << endl;

    float lightAngleInc = glm::radians(360.0f / LIGHT_CNT);
    float radius = 4.0f; //0.8f;

    for(int i = 0; i < LIGHT_CNT; i++) {
        lights[i].pos = glm::vec4(radius * sin(lightAngleInc*i),
                                    0.1f, //3.0f, //0.1f,
                                    radius * cos(lightAngleInc*i),
                                    1.0f);
        cout << "light " << std::to_string(i) 
                << ": " << glm::to_string(lights[i].pos) << endl;
        string pos_str = "lights[" + to_string(i) + "].pos";
        string color_str = "lights[" + to_string(i) + "].color";
        lights[i].posLoc = glGetUniformLocation(lightProgID, pos_str.c_str());
        lights[i].colorLoc = glGetUniformLocation(lightProgID, color_str.c_str());
    }

    //GLint lightPosLoc = glGetUniformLocation(geoProgID, "light.pos");
    //GLint lightColorLoc = glGetUniformLocation(geoProgID, "light.color");

    glfwGetFramebufferSize(window, &frameWidth, &frameHeight);
    //FBO fbo;
    //createFBO(fbo, frameWidth, frameHeight);

    GBuffer gb;
    createGBuffer(gb, frameWidth, frameHeight, lightProgID, 
                    new string[3] { "gPosition", "gNormal", "gAlbedoSpec"});
                    
    unsigned int diffTexID = loadAndCreateTexture("test.png");
    unsigned int normTexID = loadAndCreateTexture("normal.png");
   
    GLint diffuseTexLoc = glGetUniformLocation(geoProgID, "diffuseTexture");
    GLint normalTexLoc = glGetUniformLocation(geoProgID, "normalTexture");

    GLint screenTexLoc = glGetUniformLocation(lightProgID, "screenTexture");

    /*
    vector<GLfloat> vertOnly = {
        -0.3f, -0.3f, 0.0f,
        0.3f, -0.3f, 0.0f,
        -0.3f, 0.3f, 0.0f,
        0.3f, 0.3f, 0.0f,
    };
    */

   float quadScale = 1.0f; //0.75f; //1.0f; //0.3f;

    //vector<Vertex> vertOnly;
    Mesh quad;

    Vertex v0;
    v0.position = glm::vec3(-quadScale, -quadScale, 0.0f);
    v0.color = glm::vec4(0,1,0,1);
    v0.normal = glm::normalize(glm::vec3(-1,-1,1));
    v0.texcoord = glm::vec2(0,0);
    quad.vertices.push_back(v0);

    Vertex v1;
    v1.position = glm::vec3(quadScale, -quadScale, 0.0f);
    v1.color = glm::vec4(0.5,0.5,0,1);
    v1.normal = glm::normalize(glm::vec3(1,-1,1));
    v1.texcoord = glm::vec2(1,0);
    quad.vertices.push_back(v1);

    Vertex v2;
    v2.position = glm::vec3(-quadScale, quadScale, 0.0f);
    v2.color = glm::vec4(0,1,1,1);
    v2.normal = glm::normalize(glm::vec3(-1,1,1));
    v2.texcoord = glm::vec2(0,1);
    quad.vertices.push_back(v2);

    Vertex v3;
    v3.position = glm::vec3(quadScale, quadScale, 0.0f);
    v3.color = glm::vec4(0,0,1,1);
    v3.normal = glm::normalize(glm::vec3(1,1,1));
    v3.texcoord = glm::vec2(1,1);
    quad.vertices.push_back(v3);

    quad.indices = { 0, 1, 2, 1, 3, 2 };
    
    Mesh cylinder;
    makeCylinder(cylinder, 7.0, 2.0, 36);

    MeshGL mainGL;
    createMeshGL(cylinder, mainGL); 

    MeshGL quadGL;
    createMeshGL(quad, quadGL);



    cout << "geoProgID: " << geoProgID << endl;




    glClearColor(1.0, 1.0, 0.0, 1.0);
    glEnable(GL_DEPTH_TEST);

    //light.pos = glm::vec4(0, 20, 0, 1.0);

    while(!glfwWindowShouldClose(window)) {

        // GEOMETRY PASS /////////////////////////////////////////////////
        gb.startGeometry();

        glfwGetFramebufferSize(window, &frameWidth, &frameHeight);
        float aspect = 1.0f;
        if(frameHeight > 0) {
            aspect = ((float)frameWidth) / ((float)frameHeight);
        }
        float fov = glm::radians(90.0f);

        glViewport(0,0,frameWidth,frameHeight);
        glClearColor(0.0, 0.0, 0.0, 1.0);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        glUseProgram(geoProgID);

        glUniformMatrix4fv(modelMatLoc, 1, false, glm::value_ptr(modelMat));

        viewMat = glm::lookAt(glm::vec3(0,7,7), glm::vec3(0,0,0), glm::vec3(0,1,0));
        glUniformMatrix4fv(viewMatLoc, 1, false, glm::value_ptr(viewMat));

        projMat = glm::perspective(fov, aspect, 0.1f, 1000.0f);
        glUniformMatrix4fv(projMatLoc, 1, false, glm::value_ptr(projMat));

        glm::mat3 normalMat = glm::transpose(glm::inverse(glm::mat3(viewMat*modelMat)));
        glUniformMatrix3fv(normalMatLoc, 1, false, glm::value_ptr(normalMat));

        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, diffTexID);
        glActiveTexture(GL_TEXTURE1);
        glBindTexture(GL_TEXTURE_2D, normTexID);

        glUniform1i(diffuseTexLoc, 0);
        glUniform1i(normalTexLoc, 1);

        drawMesh(mainGL);

        gb.endGeometry();

        // LIGHTING PASS /////////////////////////////////////////////
        glUseProgram(lightProgID);
        gb.startLighting();       

        glViewport(0,0,frameWidth,frameHeight);
        glClearColor(0.0, 0.0, 1.0, 1.0);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        
        for(int i = 0; i < LIGHT_CNT; i++) {
            glm::vec4 lightPos = viewMat*lights[i].pos;
            glUniform4fv(lights[i].posLoc, 1, glm::value_ptr(lightPos));
            glUniform4fv(lights[i].colorLoc, 1, glm::value_ptr(lights[i].color));
        }

        drawMesh(quadGL);
        
        gb.endLighting();

        glUseProgram(0);

        glfwSwapBuffers(window);
        glfwPollEvents();
        this_thread::sleep_for(chrono::milliseconds(15));
    }

    //glDeleteFramebuffers(1, &(fbo.ID));
    gb.cleanup();

    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, 0);
    glActiveTexture(GL_TEXTURE1);
    glBindTexture(GL_TEXTURE_2D, 0);
    glDeleteTextures(1, &diffTexID);
    glDeleteTextures(1, &normTexID);

    cleanupMesh(quadGL);
    cleanupMesh(mainGL);

    glUseProgram(0);
    glDeleteProgram(geoProgID);

    glfwDestroyWindow(window);
    glfwTerminate();

    return 0;
}