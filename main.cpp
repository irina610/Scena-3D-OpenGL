#if defined (__APPLE__)
#define GLFW_INCLUDE_GLCOREARB
#define GL_SILENCE_DEPRECATION
#else
#define GLEW_STATIC
#include <GL/glew.h>
#endif

#include <GLFW/glfw3.h>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/matrix_inverse.hpp>
#include <glm/gtc/type_ptr.hpp>

#include "Window.h"
#include "Shader.hpp"
#include "Camera.hpp"
#include "Model3D.hpp"

#include <iostream>
#include <vector>

#include <windows.h>
#include <mmsystem.h>
#pragma comment(lib, "winmm.lib")

#define glCheckError() glCheckError_(__FILE__, __LINE__)

// window
gps::Window myWindow;

int glWindowWidth = 1920;
int glWindowHeight = 1080;

bool presentationMode = false;
float presentationAngle = 0.0f;

// matrices
glm::mat4 model;
glm::mat4 view;
glm::mat4 projection;
glm::mat3 normalMatrix;

// light parameters
glm::vec3 dirLightDirection;
glm::vec3 dirLightColor;
bool sunVisible = true;

glm::vec3 pointLightPosition;
glm::vec3 pointLightColor;
bool pointLightOn = true;

// attenuation
float constant = 1.0f;
float linear = 0.22f;
float quadratic = 0.20f;

// fog
bool fogEnabled = false;
float fogDensity = 0.05f;
glm::vec3 fogColor = glm::vec3(0.5f, 0.6f, 0.7f);

// sun movement
float sunAngleX = 45.0f;
float sunAngleY = 45.0f;

// Locations
GLint modelLoc;
GLint viewLoc;
GLint projectionLoc;
GLint normalMatrixLoc;
GLint dirLightDirectionLoc;
GLint dirLightColorLoc;
GLint pointLightPositionLoc;
GLint pointLightColorLoc;
GLint pointLightOnLoc;
GLint constantLoc;
GLint linearLoc;
GLint quadraticLoc;
GLint fogEnabledLoc;
GLint fogDensityLoc;
GLint fogColorLoc;

// Camera
gps::Camera myCamera(
    glm::vec3(0.0f, 10.0f, 5.5f),
    glm::vec3(0.0f, 0.0f, -10.0f),
    glm::vec3(0.0f, 1.0f, 0.0f));

GLfloat cameraSpeed = 0.7f;
GLboolean pressedKeys[1024];

// Models
gps::Model3D teapot;
GLfloat angle = 0.0f;

// Eagle
gps::Model3D eagle;
float eagleAngle = 0.0f;

// Dragon
gps::Model3D dragonBody;
gps::Model3D dragonHead;
gps::Model3D dragonWingL_Part1;
gps::Model3D dragonWingL_Part2;
gps::Model3D dragonWingR_Part1;
gps::Model3D dragonWingR_Part2;
float wingAngle = 0.0f;

// Shaders
gps::Shader myBasicShader;
gps::Shader rainShader;
gps::Shader depthMapShader; 

// Rain
bool rainEnabled = false;
GLuint rainVAO, rainVBO;
const int nrRaindrops = 2000;

// variabile umbre
GLuint shadowMapFBO;
GLuint depthMapTexture;
const unsigned int SHADOW_WIDTH = 4096; 
const unsigned int SHADOW_HEIGHT = 4096;

// Functie pentru initializarea ploii
void initRain() {
    std::vector<glm::vec3> rainVertices;
    for (int i = 0; i < nrRaindrops; i++) {
        glm::vec3 pos = glm::vec3(
            (rand() % 100) - 50.0f,
            (rand() % 50),
            (rand() % 100) - 50.0f
        );
        rainVertices.push_back(pos);
        rainVertices.push_back(pos + glm::vec3(0.0f, 0.5f, 0.0f));
    }
    glGenVertexArrays(1, &rainVAO);
    glBindVertexArray(rainVAO);
    glGenBuffers(1, &rainVBO);
    glBindBuffer(GL_ARRAY_BUFFER, rainVBO);
    glBufferData(GL_ARRAY_BUFFER, rainVertices.size() * sizeof(glm::vec3), rainVertices.data(), GL_STATIC_DRAW);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(glm::vec3), (void*)0);
    glEnableVertexAttribArray(0);
}

void initMusic() {
    
    mciSendString(TEXT("open \"muzica/intro.mp3\" type mpegvideo alias headSound"), NULL, 0, NULL);
}

// initializare umbre
void initShadowMap() {
    glGenFramebuffers(1, &shadowMapFBO);

    // Generam textura de adancime
    glGenTextures(1, &depthMapTexture);
    glBindTexture(GL_TEXTURE_2D, depthMapTexture);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT, SHADOW_WIDTH, SHADOW_HEIGHT, 0, GL_DEPTH_COMPONENT, GL_FLOAT, NULL);

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

    float borderColor[] = { 1.0f, 1.0f, 1.0f, 1.0f };
    glTexParameterfv(GL_TEXTURE_2D, GL_TEXTURE_BORDER_COLOR, borderColor);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);

    // Atasam textura la Framebuffer
    glBindFramebuffer(GL_FRAMEBUFFER, shadowMapFBO);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, depthMapTexture, 0);

    glDrawBuffer(GL_NONE);
    glReadBuffer(GL_NONE);

    glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

// calcul matrice lumina
glm::mat4 computeLightSpaceTrMatrix() {

    float size = 30.0f; 
    glm::mat4 lightProjection = glm::ortho(-size, size, -size, size, 1.0f, 200.0f);
    glm::vec3 lightDir = glm::normalize(dirLightDirection);
    glm::vec3 lightPos = lightDir * 50.0f;
    glm::mat4 lightView = glm::lookAt(lightPos, glm::vec3(0.0f), glm::vec3(0.0f, 1.0f, 0.0f));

    return lightProjection * lightView;
}

// Check Error standard
GLenum glCheckError_(const char* file, int line) {
    GLenum errorCode;
    while ((errorCode = glGetError()) != GL_NO_ERROR) {
        std::string error;
        switch (errorCode) {
        case GL_INVALID_ENUM: error = "INVALID_ENUM"; break;
        case GL_INVALID_VALUE: error = "INVALID_VALUE"; break;
        case GL_INVALID_OPERATION: error = "INVALID_OPERATION"; break;
        case GL_OUT_OF_MEMORY: error = "OUT_OF_MEMORY"; break;
        case GL_INVALID_FRAMEBUFFER_OPERATION: error = "INVALID_FRAMEBUFFER_OPERATION"; break;
        }
        std::cout << error << " | " << file << " (" << line << ")" << std::endl;
    }
    return errorCode;
}

void windowResizeCallback(GLFWwindow* window, int width, int height) {
    glWindowWidth = width;
    glWindowHeight = height;
    glViewport(0, 0, width, height);
    projection = glm::perspective(glm::radians(45.0f), (float)width / (float)height, 0.1f, 200.0f);
    myBasicShader.useShaderProgram();
    glUniformMatrix4fv(projectionLoc, 1, GL_FALSE, glm::value_ptr(projection));
}

void keyboardCallback(GLFWwindow* window, int key, int scancode, int action, int mode) {
    if (key == GLFW_KEY_ESCAPE && action == GLFW_PRESS)
        glfwSetWindowShouldClose(window, GL_TRUE);
    if (key >= 0 && key < 1024) {
        if (action == GLFW_PRESS) pressedKeys[key] = true;
        else if (action == GLFW_RELEASE) pressedKeys[key] = false;
    }
}

float lastX = glWindowWidth / 2.0f;
float lastY = glWindowHeight / 2.0f;
bool firstMouseMovement = true;

void mouseCallback(GLFWwindow* window, double xpos, double ypos) {
    if (firstMouseMovement) {
        lastX = xpos;
        lastY = ypos;
        firstMouseMovement = false;
        return;
    }
    float xoffset = xpos - lastX;
    float yoffset = lastY - ypos;
    lastX = xpos;
    lastY = ypos;
    myCamera.rotate(yoffset * 0.1f, xoffset * 0.1f);
}

void updateSunDirection() {
    
    if (!sunVisible) {
        dirLightColor = glm::vec3(0.05f, 0.05f, 0.15f);
        fogColor = glm::vec3(0.05f, 0.05f, 0.1f);
        dirLightDirection = glm::normalize(glm::vec3(0.0f, 1.0f, -0.5f));
    }
    else {
        dirLightDirection = glm::vec3(
            cos(glm::radians(sunAngleY)) * cos(glm::radians(sunAngleX)),
            sin(glm::radians(sunAngleX)),
            sin(glm::radians(sunAngleY)) * cos(glm::radians(sunAngleX))
        );
        dirLightDirection = glm::normalize(dirLightDirection);

        if (dirLightDirection.y > 0.2f) {
            dirLightColor = glm::vec3(1.0f, 0.95f, 0.9f);
            fogColor = glm::vec3(0.5f, 0.6f, 0.7f);
        }
        else if (dirLightDirection.y > 0.0f) {
            dirLightColor = glm::vec3(1.0f, 0.6f, 0.3f);
            fogColor = glm::vec3(0.8f, 0.5f, 0.4f);
        }
        else {
            dirLightColor = glm::vec3(0.05f, 0.05f, 0.1f);
            fogColor = glm::vec3(0.05f, 0.05f, 0.1f);
        }
    }

    myBasicShader.useShaderProgram();
    glUniform3fv(dirLightDirectionLoc, 1, glm::value_ptr(dirLightDirection));
    glUniform3fv(dirLightColorLoc, 1, glm::value_ptr(dirLightColor));
    glUniform3fv(fogColorLoc, 1, glm::value_ptr(fogColor));
}

int viewMode = 0;

void processMovement() {
    if (pressedKeys[GLFW_KEY_W]) myCamera.move(gps::MOVE_FORWARD, cameraSpeed);
    if (pressedKeys[GLFW_KEY_S]) myCamera.move(gps::MOVE_BACKWARD, cameraSpeed);
    if (pressedKeys[GLFW_KEY_A]) myCamera.move(gps::MOVE_LEFT, cameraSpeed);
    if (pressedKeys[GLFW_KEY_D]) myCamera.move(gps::MOVE_RIGHT, cameraSpeed);

    if (pressedKeys[GLFW_KEY_Q]) {
        angle += 1.0f;
        model = glm::rotate(glm::mat4(1.0f), glm::radians(angle), glm::vec3(0, 1, 0));
    }
    if (pressedKeys[GLFW_KEY_E]) {
        angle -= 1.0f;
        model = glm::rotate(glm::mat4(1.0f), glm::radians(angle), glm::vec3(0, 1, 0));
    }

    // LOGICA MUZICA (Tasta X) 
    static bool xKeyPressed = false;
    static bool isMusicOn = false; 

    if (pressedKeys[GLFW_KEY_X] && !xKeyPressed) {
        xKeyPressed = true; 

        if (isMusicOn) {
            // Comanda pentru PAUZA
            mciSendString(TEXT("pause headSound"), NULL, 0, NULL);
            isMusicOn = false;
        }
        else {
            // Comanda pentru PLAY
            mciSendString(TEXT("play headSound repeat"), NULL, 0, NULL);
            isMusicOn = true;
        }
    }
    if (!pressedKeys[GLFW_KEY_X]) {
        xKeyPressed = false; 
    }

    // View Modes
    static bool mKeyPressed = false;
    if (pressedKeys[GLFW_KEY_M] && !mKeyPressed) {
        mKeyPressed = true;
        viewMode = (viewMode + 1) % 3;
        switch (viewMode) {
        case 0: glPolygonMode(GL_FRONT_AND_BACK, GL_FILL); break;
        case 1: glPolygonMode(GL_FRONT_AND_BACK, GL_LINE); break;
        case 2: glPolygonMode(GL_FRONT_AND_BACK, GL_POINT); break;
        }
    }
    if (!pressedKeys[GLFW_KEY_M]) mKeyPressed = false;

    // Toggle point light
    static bool fKeyPressed = false;
    if (pressedKeys[GLFW_KEY_F] && !fKeyPressed) {
        pointLightOn = !pointLightOn;
        fKeyPressed = true;
        myBasicShader.useShaderProgram();
        glUniform1i(pointLightOnLoc, pointLightOn);
    }
    if (!pressedKeys[GLFW_KEY_F]) fKeyPressed = false;

    // Toggle Night Mode
    static bool nKeyPressed = false;
    if (pressedKeys[GLFW_KEY_N] && !nKeyPressed) {
        sunVisible = !sunVisible;
        nKeyPressed = true;
        updateSunDirection();
    }
    if (!pressedKeys[GLFW_KEY_N]) nKeyPressed = false;

    // Toggle Fog
    static bool cKeyPressed = false;
    if (pressedKeys[GLFW_KEY_C] && !cKeyPressed) {
        fogEnabled = !fogEnabled;
        cKeyPressed = true;
        myBasicShader.useShaderProgram();
        glUniform1i(fogEnabledLoc, fogEnabled);
    }
    if (!pressedKeys[GLFW_KEY_C]) cKeyPressed = false;

    // Move Sun
    bool sunMoved = false;
    if (pressedKeys[GLFW_KEY_UP]) { sunAngleX += 1.0f; sunMoved = true; }
    if (pressedKeys[GLFW_KEY_DOWN]) { sunAngleX -= 1.0f; sunMoved = true; }
    if (pressedKeys[GLFW_KEY_LEFT]) { sunAngleY -= 1.0f; sunMoved = true; }
    if (pressedKeys[GLFW_KEY_RIGHT]) { sunAngleY += 1.0f; sunMoved = true; }
    if (sunMoved && sunVisible) updateSunDirection();

    // Rain
    static bool pKeyPressed = false;
    if (pressedKeys[GLFW_KEY_P] && !pKeyPressed) {
        rainEnabled = !rainEnabled;
        pKeyPressed = true;
    }
    if (!pressedKeys[GLFW_KEY_P]) pKeyPressed = false;

    // Presentation
    static bool zKeyPressed = false;
    if (pressedKeys[GLFW_KEY_Z] && !zKeyPressed) {
        presentationMode = !presentationMode;
        zKeyPressed = true;
    }
    if (!pressedKeys[GLFW_KEY_Z]) zKeyPressed = false;
}

void initOpenGLWindow() {
    myWindow.Create(1920, 1080, "OpenGL Project Core");
}

void setWindowCallbacks() {
    glfwSetWindowSizeCallback(myWindow.getWindow(), windowResizeCallback);
    glfwSetKeyCallback(myWindow.getWindow(), keyboardCallback);
    glfwSetCursorPosCallback(myWindow.getWindow(), mouseCallback);
    glfwSetInputMode(myWindow.getWindow(), GLFW_CURSOR, GLFW_CURSOR_DISABLED);
    glfwSetCursorPos(myWindow.getWindow(), glWindowWidth / 2.0, glWindowHeight / 2.0);
}

void initOpenGLState() {
    glClearColor(fogColor.r, fogColor.g, fogColor.b, 1.0f);
    glViewport(0, 0, myWindow.getWindowDimensions().width, myWindow.getWindowDimensions().height);
    glEnable(GL_FRAMEBUFFER_SRGB);
    glEnable(GL_DEPTH_TEST);
    glDepthFunc(GL_LESS);
    glEnable(GL_CULL_FACE);
    glCullFace(GL_BACK);
    glFrontFace(GL_CCW);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
}

void initModels() {
    teapot.LoadModel("models/Principal.obj");
    eagle.LoadModel("models/Vultur.obj");
    dragonBody.LoadModel("models/componenteDRAGON/dragon_body.obj");
    dragonHead.LoadModel("models/componenteDRAGON/dragon_head.obj");
    dragonWingL_Part1.LoadModel("models/componenteDRAGON/dragon_wing_left1.obj");
    dragonWingL_Part2.LoadModel("models/componenteDRAGON/dragon_wing_left2.obj");
    dragonWingR_Part1.LoadModel("models/componenteDRAGON/dragon_wing_right1.obj");
    dragonWingR_Part2.LoadModel("models/componenteDRAGON/dragon_wing_right2.obj");
}

void initShaders() {
    myBasicShader.loadShader("shaders/basic.vert", "shaders/basic.frag");
    rainShader.loadShader("shaders/rain.vert", "shaders/rain.frag");
    depthMapShader.loadShader("shaders/depth.vert", "shaders/depth.frag");
}

void initUniforms() {
    myBasicShader.useShaderProgram();

    model = glm::rotate(glm::mat4(1.0f), glm::radians(angle), glm::vec3(0.0f, 1.0f, 0.0f));
    modelLoc = glGetUniformLocation(myBasicShader.shaderProgram, "model");

    view = myCamera.getViewMatrix();
    viewLoc = glGetUniformLocation(myBasicShader.shaderProgram, "view");
    glUniformMatrix4fv(viewLoc, 1, GL_FALSE, glm::value_ptr(view));

    normalMatrix = glm::mat3(glm::inverseTranspose(view * model));
    normalMatrixLoc = glGetUniformLocation(myBasicShader.shaderProgram, "normalMatrix");

    projection = glm::perspective(glm::radians(45.0f), (float)glWindowWidth / (float)glWindowHeight, 0.1f, 200.0f);
    projectionLoc = glGetUniformLocation(myBasicShader.shaderProgram, "projection");
    glUniformMatrix4fv(projectionLoc, 1, GL_FALSE, glm::value_ptr(projection));

    // Point Light
    pointLightPosition = glm::vec3(2.7299f, 5.8243f, 4.1866f);
    pointLightColor = glm::vec3(1.5f, 1.4f, 1.0f);
    pointLightPositionLoc = glGetUniformLocation(myBasicShader.shaderProgram, "pointLightPosition");
    pointLightColorLoc = glGetUniformLocation(myBasicShader.shaderProgram, "pointLightColor");
    pointLightOnLoc = glGetUniformLocation(myBasicShader.shaderProgram, "pointLightOn");
    glUniform3fv(pointLightPositionLoc, 1, glm::value_ptr(pointLightPosition));
    glUniform3fv(pointLightColorLoc, 1, glm::value_ptr(pointLightColor));
    glUniform1i(pointLightOnLoc, pointLightOn);

    constantLoc = glGetUniformLocation(myBasicShader.shaderProgram, "constant");
    linearLoc = glGetUniformLocation(myBasicShader.shaderProgram, "linear");
    quadraticLoc = glGetUniformLocation(myBasicShader.shaderProgram, "quadratic");
    glUniform1f(constantLoc, constant);
    glUniform1f(linearLoc, linear);
    glUniform1f(quadraticLoc, quadratic);

    fogEnabledLoc = glGetUniformLocation(myBasicShader.shaderProgram, "fogEnabled");
    fogDensityLoc = glGetUniformLocation(myBasicShader.shaderProgram, "fogDensity");
    fogColorLoc = glGetUniformLocation(myBasicShader.shaderProgram, "fogColor");
    glUniform1i(fogEnabledLoc, fogEnabled);
    glUniform1f(fogDensityLoc, fogDensity);

    dirLightDirectionLoc = glGetUniformLocation(myBasicShader.shaderProgram, "dirLightDirection");
    dirLightColorLoc = glGetUniformLocation(myBasicShader.shaderProgram, "dirLightColor");

    updateSunDirection();
}

// FUNCTIA CENTRALA DE DESENARE
void drawScene(gps::Shader shader, bool depthPass) {
    shader.useShaderProgram();

    glUniformMatrix4fv(glGetUniformLocation(shader.shaderProgram, "model"), 1, GL_FALSE, glm::value_ptr(model));
    
    if (!depthPass) {
        glUniformMatrix3fv(glGetUniformLocation(shader.shaderProgram, "normalMatrix"), 1, GL_FALSE, glm::value_ptr(normalMatrix));
    }
    teapot.Draw(shader);

    // VULTUR
    glm::mat4 modelEagle = glm::mat4(1.0f);
    glm::vec3 castlePivot = glm::vec3(-22.429f, 22.097f, 4.422f);
    modelEagle = glm::translate(modelEagle, castlePivot);
    modelEagle = glm::rotate(modelEagle, glm::radians(eagleAngle), glm::vec3(0.0f, 1.0f, 0.0f));
    modelEagle = glm::translate(modelEagle, glm::vec3(10.0f, -20.0f, 0.0f));
    modelEagle = glm::rotate(modelEagle, glm::radians(180.0f), glm::vec3(0.0f, 1.0f, 0.0f));

    glUniformMatrix4fv(glGetUniformLocation(shader.shaderProgram, "model"), 1, GL_FALSE, glm::value_ptr(modelEagle));
    if (!depthPass) {
        glm::mat3 normalMatrixEagle = glm::mat3(glm::inverseTranspose(view * modelEagle));
        glUniformMatrix3fv(glGetUniformLocation(shader.shaderProgram, "normalMatrix"), 1, GL_FALSE, glm::value_ptr(normalMatrixEagle));
    }
    eagle.Draw(shader);

    // DRAGON 
    float angleFlap = 20.0f * sin(glfwGetTime() * 5.0f);

    // Dragon Body
    glm::mat4 modelDragon = glm::mat4(1.0f);
    glUniformMatrix4fv(glGetUniformLocation(shader.shaderProgram, "model"), 1, GL_FALSE, glm::value_ptr(modelDragon));
    if (!depthPass) {
        glUniformMatrix3fv(glGetUniformLocation(shader.shaderProgram, "normalMatrix"), 1, GL_FALSE, glm::value_ptr(glm::mat3(glm::inverseTranspose(view * modelDragon))));
    }
    dragonBody.Draw(shader);
    dragonHead.Draw(shader);

    // Aripa Stanga
    glm::vec3 pivotStanga = glm::vec3(39.3f, 15.3f, -3.8f);
    glm::mat4 modelWingL = glm::mat4(1.0f);
    modelWingL = glm::translate(modelWingL, pivotStanga);
    modelWingL = glm::rotate(modelWingL, glm::radians(angleFlap), glm::vec3(5.96f, -3.391f, -2.7527f));
    modelWingL = glm::translate(modelWingL, -pivotStanga);

    glUniformMatrix4fv(glGetUniformLocation(shader.shaderProgram, "model"), 1, GL_FALSE, glm::value_ptr(modelWingL));
    if (!depthPass) {
        glUniformMatrix3fv(glGetUniformLocation(shader.shaderProgram, "normalMatrix"), 1, GL_FALSE, glm::value_ptr(glm::mat3(glm::inverseTranspose(view * modelWingL))));
    }
    dragonWingL_Part1.Draw(shader);
    dragonWingL_Part2.Draw(shader);

    // Aripa Dreapta
    glm::vec3 pivotDreapta = glm::vec3(39.375f, 15.334f, -5.08f);
    glm::mat4 modelWingR = glm::mat4(1.0f);
    modelWingR = glm::translate(modelWingR, pivotDreapta);
    modelWingR = glm::rotate(modelWingR, glm::radians(-angleFlap), glm::vec3(5.96f, -3.391f, -2.7527f));
    modelWingR = glm::translate(modelWingR, -pivotDreapta);

    glUniformMatrix4fv(glGetUniformLocation(shader.shaderProgram, "model"), 1, GL_FALSE, glm::value_ptr(modelWingR));
    if (!depthPass) {
        glUniformMatrix3fv(glGetUniformLocation(shader.shaderProgram, "normalMatrix"), 1, GL_FALSE, glm::value_ptr(glm::mat3(glm::inverseTranspose(view * modelWingR))));
    }
    dragonWingR_Part1.Draw(shader);
    dragonWingR_Part2.Draw(shader);
}

void renderScene() {
    // Actualizare animatii
    eagleAngle -= 3.0f;
    if (eagleAngle > 360.0f) eagleAngle -= 360.0f;

    // RANDARE IN TEXTURA DE ADANCIME (SHADOW MAP)
    depthMapShader.useShaderProgram();

    // Calcul matricea spatiului luminii
    glm::mat4 lightSpaceTrMatrix = computeLightSpaceTrMatrix();
    glUniformMatrix4fv(glGetUniformLocation(depthMapShader.shaderProgram, "lightSpaceTrMatrix"), 1, GL_FALSE, glm::value_ptr(lightSpaceTrMatrix));

    glViewport(0, 0, SHADOW_WIDTH, SHADOW_HEIGHT);
    glBindFramebuffer(GL_FRAMEBUFFER, shadowMapFBO);
    glClear(GL_DEPTH_BUFFER_BIT);

    drawScene(depthMapShader, true);

    glBindFramebuffer(GL_FRAMEBUFFER, 0);

   
    // RANDARE NORMALA A SCENEI
    glViewport(0, 0, glWindowWidth, glWindowHeight);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    glClearColor(fogColor.r, fogColor.g, fogColor.b, 1.0f);

    myBasicShader.useShaderProgram();

    // Calcul View Matrix (Camera)
    view = myCamera.getViewMatrix();
    glm::mat4 orbitRotation = glm::rotate(glm::mat4(1.0f), glm::radians(angle), glm::vec3(0.0f, 1.0f, 0.0f));
    view = view * orbitRotation;

    if (presentationMode) {
        presentationAngle += 2.0f;
        float radius = 50.0f; float camHeight = 15.0f;
        float camX = sin(glm::radians(presentationAngle)) * radius;
        float camZ = cos(glm::radians(presentationAngle)) * radius;
        view = glm::lookAt(glm::vec3(camX, camHeight, camZ), glm::vec3(0.0f, 2.0f, 0.0f), glm::vec3(0.0f, 1.0f, 0.0f));
    }
    glUniformMatrix4fv(viewLoc, 1, GL_FALSE, glm::value_ptr(view));

    glUniformMatrix4fv(glGetUniformLocation(myBasicShader.shaderProgram, "lightSpaceTrMatrix"), 1, GL_FALSE, glm::value_ptr(lightSpaceTrMatrix));

    glActiveTexture(GL_TEXTURE1);
    glBindTexture(GL_TEXTURE_2D, depthMapTexture);
    glUniform1i(glGetUniformLocation(myBasicShader.shaderProgram, "shadowMap"), 1);

    glActiveTexture(GL_TEXTURE0);

    glUniform3fv(dirLightDirectionLoc, 1, glm::value_ptr(dirLightDirection));

    drawScene(myBasicShader, false);

    // Ploaia  
    if (rainEnabled) {
        rainShader.useShaderProgram();
        glUniformMatrix4fv(glGetUniformLocation(rainShader.shaderProgram, "view"), 1, GL_FALSE, glm::value_ptr(view));
        glUniformMatrix4fv(glGetUniformLocation(rainShader.shaderProgram, "projection"), 1, GL_FALSE, glm::value_ptr(projection));
        glUniform1f(glGetUniformLocation(rainShader.shaderProgram, "time"), (float)glfwGetTime());
        glBindVertexArray(rainVAO);
        glLineWidth(1.0f);
        glDrawArrays(GL_LINES, 0, nrRaindrops * 2);
    }
}

void cleanup() {

    mciSendString(TEXT("close intro"), NULL, 0, NULL);
    myWindow.Delete();

}

int main(int argc, const char* argv[]) {
    try { initOpenGLWindow(); }
    catch (const std::exception& e) { std::cerr << e.what() << std::endl; return EXIT_FAILURE; }
    initOpenGLState();
    initModels();
    initShaders();
    initMusic();
    initShadowMap();
    initUniforms();
    setWindowCallbacks();
    initRain();

    std::cout << "=== CONTROLS ===" << std::endl;
    std::cout << "W / S        - Deplasare camera inainte / inapoi" << std::endl;
    std::cout << "A / D        - Deplasare camera stanga / dreapta" << std::endl;
    std::cout << "Mouse        - Rotire camera (yaw / pitch)" << std::endl;
    std::cout << "Q / E        - Rotire scena in jurul axei Y" << std::endl;
    std::cout << std::endl;

    std::cout << "M            - Schimbare mod vizualizare:" << std::endl;
    std::cout << "               * Solid (Fill)" << std::endl;
    std::cout << "               * Wireframe (Line)" << std::endl;
    std::cout << "               * Puncte (Point)" << std::endl;
    std::cout << "N            - Activare / Dezactivare mod noapte (lumina solara)" << std::endl;
    std::cout << "F            - Activare / Dezactivare lumina punctiforma (felinar)" << std::endl;
    std::cout << "C            - Activare / Dezactivare ceata" << std::endl;
    std::cout << "P            - Activare / Dezactivare ploaie" << std::endl;
    std::cout << std::endl;
    std::cout << "Sageti       - Miscare soare (schimbare directie lumina)" << std::endl;
    std::cout << "X            - Pornire / Oprire muzica de fundal" << std::endl;
    std::cout << "Z            - Activare / Dezactivare mod prezentare" << std::endl;
    std::cout << "ESC          - Iesire din aplicatie" << std::endl;
    std::cout << "==============================" << std::endl;


    while (!glfwWindowShouldClose(myWindow.getWindow())) {
        processMovement();
        renderScene();
        glfwPollEvents();
        glfwSwapBuffers(myWindow.getWindow());
        glCheckError();
    }
    cleanup();
    return EXIT_SUCCESS;
}