#include <glad/glad.h>
#include <GLFW/glfw3.h>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#include <filesystem>
#include <iostream>
#include <string>
#include <vector>

#include "Camera.h"
#include "Shader.h"
#include "Texture.h"

namespace {
std::filesystem::path ResolveProjectRoot(const std::filesystem::path& executablePath) {
    std::vector<std::filesystem::path> candidates;

    const std::filesystem::path current = std::filesystem::absolute(std::filesystem::current_path());
    candidates.push_back(current);
    if (!current.empty()) {
        candidates.push_back(current / "..");
    }

    std::filesystem::path executable = std::filesystem::absolute(executablePath);
    if (!executable.empty()) {
        candidates.push_back(executable.parent_path());
        if (executable.parent_path() != executable.root_path()) {
            candidates.push_back(executable.parent_path() / "..");
        }
    }

    for (const auto& candidate : candidates) {
        const auto shaderPath = candidate / "shaders" / "vertex.glsl";
        const auto texturesFolder = candidate / "textures";
        if (std::filesystem::exists(shaderPath) && std::filesystem::exists(texturesFolder)) {
            return candidate;
        }
    }

    return current;
}

std::filesystem::path ResolveAssetPath(const std::filesystem::path& relativePath, const std::filesystem::path& projectRoot) {
    std::vector<std::filesystem::path> candidates = {
        projectRoot / relativePath,
        std::filesystem::absolute(std::filesystem::current_path()) / relativePath,
        std::filesystem::absolute(std::filesystem::current_path()).parent_path() / relativePath
    };

    for (const auto& candidate : candidates) {
        if (std::filesystem::exists(candidate)) {
            return candidate;
        }
    }

    return projectRoot / relativePath;
}
}

static Camera* gCamera = nullptr;
static float gLastX = 0.0f;
static float gLastY = 0.0f;
static bool gFirstMouse = true;

static void FramebufferSizeCallback(GLFWwindow* window, int width, int height) {
    glViewport(0, 0, width, height);
}

static void MouseCallback(GLFWwindow* window, double xpos, double ypos) {
    if (gCamera == nullptr) {
        return;
    }

    if (glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_LEFT) != GLFW_PRESS) {
        gFirstMouse = true;
        return;
    }

    if (gFirstMouse) {
        gLastX = static_cast<float>(xpos);
        gLastY = static_cast<float>(ypos);
        gFirstMouse = false;
        return;
    }

    float xOffset = static_cast<float>(xpos - gLastX);
    float yOffset = static_cast<float>(gLastY - ypos);

    gLastX = static_cast<float>(xpos);
    gLastY = static_cast<float>(ypos);

    gCamera->ProcessMouseMovement(xOffset, yOffset);
}

static void ProcessInput(GLFWwindow* window, Camera& camera, float deltaTime) {
    if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS) {
        glfwSetWindowShouldClose(window, true);
    }

    if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS) {
        camera.ProcessKeyboard(CameraDirection::FORWARD, deltaTime);
    }
    if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS) {
        camera.ProcessKeyboard(CameraDirection::BACKWARD, deltaTime);
    }
    if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS) {
        camera.ProcessKeyboard(CameraDirection::LEFT, deltaTime);
    }
    if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS) {
        camera.ProcessKeyboard(CameraDirection::RIGHT, deltaTime);
    }
    if (glfwGetKey(window, GLFW_KEY_Q) == GLFW_PRESS) {
        camera.ProcessKeyboard(CameraDirection::UP, deltaTime);
    }
    if (glfwGetKey(window, GLFW_KEY_E) == GLFW_PRESS) {
        camera.ProcessKeyboard(CameraDirection::DOWN, deltaTime);
    }

    if (glfwGetKey(window, GLFW_KEY_UP) == GLFW_PRESS) {
        camera.Pitch += 60.0f * deltaTime;
        camera.Pitch = glm::clamp(camera.Pitch, -89.0f, 89.0f);
        camera.UpdateCameraVectors();
    }
    if (glfwGetKey(window, GLFW_KEY_DOWN) == GLFW_PRESS) {
        camera.Pitch -= 60.0f * deltaTime;
        camera.Pitch = glm::clamp(camera.Pitch, -89.0f, 89.0f);
        camera.UpdateCameraVectors();
    }
    if (glfwGetKey(window, GLFW_KEY_LEFT) == GLFW_PRESS) {
        camera.Yaw -= 60.0f * deltaTime;
        camera.UpdateCameraVectors();
    }
    if (glfwGetKey(window, GLFW_KEY_RIGHT) == GLFW_PRESS) {
        camera.Yaw += 60.0f * deltaTime;
        camera.UpdateCameraVectors();
    }
}

static unsigned int CreateCubeVAO() {
    const float vertices[] = {
        // positions            normals              texture coords
        -0.5f, -0.5f, -0.5f,  0.0f,  0.0f, -1.0f,  0.0f, 0.0f,
         0.5f, -0.5f, -0.5f,  0.0f,  0.0f, -1.0f,  1.0f, 0.0f,
         0.5f,  0.5f, -0.5f,  0.0f,  0.0f, -1.0f,  1.0f, 1.0f,
         0.5f,  0.5f, -0.5f,  0.0f,  0.0f, -1.0f,  1.0f, 1.0f,
        -0.5f,  0.5f, -0.5f,  0.0f,  0.0f, -1.0f,  0.0f, 1.0f,
        -0.5f, -0.5f, -0.5f,  0.0f,  0.0f, -1.0f,  0.0f, 0.0f,

        -0.5f, -0.5f,  0.5f,  0.0f,  0.0f,  1.0f,  0.0f, 0.0f,
         0.5f, -0.5f,  0.5f,  0.0f,  0.0f,  1.0f,  1.0f, 0.0f,
         0.5f,  0.5f,  0.5f,  0.0f,  0.0f,  1.0f,  1.0f, 1.0f,
         0.5f,  0.5f,  0.5f,  0.0f,  0.0f,  1.0f,  1.0f, 1.0f,
        -0.5f,  0.5f,  0.5f,  0.0f,  0.0f,  1.0f,  0.0f, 1.0f,
        -0.5f, -0.5f,  0.5f,  0.0f,  0.0f,  1.0f,  0.0f, 0.0f,

        -0.5f,  0.5f,  0.5f, -1.0f,  0.0f,  0.0f,  1.0f, 0.0f,
        -0.5f,  0.5f, -0.5f, -1.0f,  0.0f,  0.0f,  1.0f, 1.0f,
        -0.5f, -0.5f, -0.5f, -1.0f,  0.0f,  0.0f,  0.0f, 1.0f,
        -0.5f, -0.5f, -0.5f, -1.0f,  0.0f,  0.0f,  0.0f, 1.0f,
        -0.5f, -0.5f,  0.5f, -1.0f,  0.0f,  0.0f,  0.0f, 0.0f,
        -0.5f,  0.5f,  0.5f, -1.0f,  0.0f,  0.0f,  1.0f, 0.0f,

         0.5f,  0.5f,  0.5f,  1.0f,  0.0f,  0.0f,  1.0f, 0.0f,
         0.5f,  0.5f, -0.5f,  1.0f,  0.0f,  0.0f,  1.0f, 1.0f,
         0.5f, -0.5f, -0.5f,  1.0f,  0.0f,  0.0f,  0.0f, 1.0f,
         0.5f, -0.5f, -0.5f,  1.0f,  0.0f,  0.0f,  0.0f, 1.0f,
         0.5f, -0.5f,  0.5f,  1.0f,  0.0f,  0.0f,  0.0f, 0.0f,
         0.5f,  0.5f,  0.5f,  1.0f,  0.0f,  0.0f,  1.0f, 0.0f,

        -0.5f, -0.5f, -0.5f,  0.0f, -1.0f,  0.0f,  0.0f, 1.0f,
         0.5f, -0.5f, -0.5f,  0.0f, -1.0f,  0.0f,  1.0f, 1.0f,
         0.5f, -0.5f,  0.5f,  0.0f, -1.0f,  0.0f,  1.0f, 0.0f,
         0.5f, -0.5f,  0.5f,  0.0f, -1.0f,  0.0f,  1.0f, 0.0f,
        -0.5f, -0.5f,  0.5f,  0.0f, -1.0f,  0.0f,  0.0f, 0.0f,
        -0.5f, -0.5f, -0.5f,  0.0f, -1.0f,  0.0f,  0.0f, 1.0f,

        -0.5f,  0.5f, -0.5f,  0.0f,  1.0f,  0.0f,  0.0f, 1.0f,
         0.5f,  0.5f, -0.5f,  0.0f,  1.0f,  0.0f,  1.0f, 1.0f,
         0.5f,  0.5f,  0.5f,  0.0f,  1.0f,  0.0f,  1.0f, 0.0f,
         0.5f,  0.5f,  0.5f,  0.0f,  1.0f,  0.0f,  1.0f, 0.0f,
        -0.5f,  0.5f,  0.5f,  0.0f,  1.0f,  0.0f,  0.0f, 0.0f,
        -0.5f,  0.5f, -0.5f,  0.0f,  1.0f,  0.0f,  0.0f, 1.0f
    };

    unsigned int vao = 0;
    unsigned int vbo = 0;

    glGenVertexArrays(1, &vao);
    glGenBuffers(1, &vbo);

    glBindVertexArray(vao);
    glBindBuffer(GL_ARRAY_BUFFER, vbo);
    glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);

    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);

    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)(3 * sizeof(float)));
    glEnableVertexAttribArray(1);

    glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)(6 * sizeof(float)));
    glEnableVertexAttribArray(2);

    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindVertexArray(0);

    return vao;
}

static void DrawObject(
    const Shader& shader,
    const Texture& texture,
    unsigned int vao,
    const glm::mat4& model,
    const glm::vec3& color,
    bool useTexture = true)
{
    shader.Use();
    texture.Bind(0);
    shader.SetInt("texture1", 0);
    shader.SetBool("useTexture", useTexture);
    shader.SetVec3("objectColor", color);
    shader.SetMat4("model", model);

    glBindVertexArray(vao);
    glDrawArrays(GL_TRIANGLES, 0, 36);
    glBindVertexArray(0);
}

int main(int argc, char* argv[]) {
    const std::filesystem::path projectRoot = ResolveProjectRoot(argc > 0 ? argv[0] : "");
    const std::filesystem::path vertexShaderPath = ResolveAssetPath("shaders/vertex.glsl", projectRoot);
    const std::filesystem::path fragmentShaderPath = ResolveAssetPath("shaders/fragment.glsl", projectRoot);
    const std::filesystem::path wallTexturePath = ResolveAssetPath("textures/shrek_toilet.ppm", projectRoot);

    if (!glfwInit()) {
        std::cerr << "Failed to initialize GLFW" << std::endl;
        return -1;
    }

    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

    GLFWmonitor* primaryMonitor = glfwGetPrimaryMonitor();
    const GLFWvidmode* videoMode = glfwGetVideoMode(primaryMonitor);
    const int windowWidth = static_cast<int>(videoMode->width * 0.8f);
    const int windowHeight = static_cast<int>(videoMode->height * 0.8f);

    GLFWwindow* window = glfwCreateWindow(windowWidth, windowHeight, "OpenGL Lab 4", nullptr, nullptr);
    if (!window) {
        std::cerr << "Failed to create GLFW window" << std::endl;
        glfwTerminate();
        return -1;
    }

    glfwSetWindowPos(window,
        (videoMode->width - windowWidth) / 2,
        (videoMode->height - windowHeight) / 2);

    glfwMakeContextCurrent(window);
    glfwSetFramebufferSizeCallback(window, FramebufferSizeCallback);
    glfwSetCursorPosCallback(window, MouseCallback);
    glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_NORMAL);

    if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress)) {
        std::cerr << "Failed to initialize GLAD" << std::endl;
        return -1;
    }

    glEnable(GL_DEPTH_TEST);
    glClearColor(0.1f, 0.15f, 0.2f, 1.0f);

    Shader shader(vertexShaderPath.string().c_str(), fragmentShaderPath.string().c_str());
    Camera camera(glm::vec3(0.0f, 2.0f, 6.0f));
    gCamera = &camera;

    Texture wallTexture(wallTexturePath.string());
    unsigned int cubeVAO = CreateCubeVAO();

    float lastFrame = static_cast<float>(glfwGetTime());

    while (!glfwWindowShouldClose(window)) {
        float currentFrame = static_cast<float>(glfwGetTime());
        float deltaTime = currentFrame - lastFrame;
        lastFrame = currentFrame;

        ProcessInput(window, camera, deltaTime);

        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        glm::mat4 projection = glm::perspective(glm::radians(45.0f), 1280.0f / 720.0f, 0.1f, 100.0f);
        glm::mat4 view = camera.GetViewMatrix();

        shader.Use();
        shader.SetMat4("projection", projection);
        shader.SetMat4("view", view);
        shader.SetVec3("lightPos", glm::vec3(2.5f, 5.0f, 3.0f));
        shader.SetVec3("viewPos", camera.Position);

        float rotationY = static_cast<float>(glfwGetTime()) * 25.0f;

        glm::mat4 groundModel = glm::mat4(1.0f);
        groundModel = glm::translate(groundModel, glm::vec3(0.0f, -1.9f, 0.0f));
        groundModel = glm::rotate(groundModel, glm::radians(rotationY), glm::vec3(0.0f, 1.0f, 0.0f));
        groundModel = glm::scale(groundModel, glm::vec3(12.0f, 0.4f, 12.0f));
        DrawObject(shader, wallTexture, cubeVAO, groundModel, glm::vec3(0.12f, 0.48f, 0.18f), false);

        glm::mat4 outhouseModel = glm::mat4(1.0f);
        outhouseModel = glm::translate(outhouseModel, glm::vec3(0.0f, -0.2f, 0.0f));
        outhouseModel = glm::rotate(outhouseModel, glm::radians(rotationY), glm::vec3(0.0f, 1.0f, 0.0f));
        outhouseModel = glm::scale(outhouseModel, glm::vec3(1.8f, 2.7f, 1.4f));
        DrawObject(shader, wallTexture, cubeVAO, outhouseModel, glm::vec3(0.91f, 0.83f, 0.73f));

        glm::mat4 roofModel = glm::mat4(1.0f);
        roofModel = glm::translate(roofModel, glm::vec3(0.0f, 1.55f, 0.0f));
        roofModel = glm::rotate(roofModel, glm::radians(rotationY), glm::vec3(0.0f, 1.0f, 0.0f));
        roofModel = glm::rotate(roofModel, glm::radians(-40.0f), glm::vec3(1.0f, 0.0f, 0.0f));
        roofModel = glm::scale(roofModel, glm::vec3(2.4f, 0.4f, 2.2f));
        DrawObject(shader, wallTexture, cubeVAO, roofModel, glm::vec3(0.53f, 0.35f, 0.22f));

        glm::mat4 roofFrontModel = glm::mat4(1.0f);
        roofFrontModel = glm::translate(roofFrontModel, glm::vec3(0.0f, 1.7f, 0.0f));
        roofFrontModel = glm::rotate(roofFrontModel, glm::radians(rotationY), glm::vec3(0.0f, 1.0f, 0.0f));
        roofFrontModel = glm::rotate(roofFrontModel, glm::radians(40.0f), glm::vec3(1.0f, 0.0f, 0.0f));
        roofFrontModel = glm::scale(roofFrontModel, glm::vec3(2.35f, 0.35f, 2.15f));
        DrawObject(shader, wallTexture, cubeVAO, roofFrontModel, glm::vec3(0.37f, 0.26f, 0.18f));

        glm::mat4 baseModel = glm::mat4(1.0f);
        baseModel = glm::translate(baseModel, glm::vec3(0.0f, -1.5f, 0.0f));
        baseModel = glm::rotate(baseModel, glm::radians(rotationY), glm::vec3(0.0f, 1.0f, 0.0f));
        baseModel = glm::scale(baseModel, glm::vec3(2.3f, 0.45f, 1.8f));
        DrawObject(shader, wallTexture, cubeVAO, baseModel, glm::vec3(0.44f, 0.29f, 0.15f));

        glm::mat4 doorModel = glm::mat4(1.0f);
        doorModel = glm::translate(doorModel, glm::vec3(0.0f, -0.5f, 0.72f));
        doorModel = glm::rotate(doorModel, glm::radians(rotationY), glm::vec3(0.0f, 1.0f, 0.0f));
        doorModel = glm::scale(doorModel, glm::vec3(0.7f, 1.4f, 0.12f));
        DrawObject(shader, wallTexture, cubeVAO, doorModel, glm::vec3(0.42f, 0.27f, 0.17f));

        glm::mat4 seatModel = glm::mat4(1.0f);
        seatModel = glm::translate(seatModel, glm::vec3(0.0f, -0.6f, 0.0f));
        seatModel = glm::rotate(seatModel, glm::radians(rotationY), glm::vec3(0.0f, 1.0f, 0.0f));
        seatModel = glm::scale(seatModel, glm::vec3(0.25f, 0.25f, 0.25f));
        DrawObject(shader, wallTexture, cubeVAO, seatModel, glm::vec3(0.49f, 0.31f, 0.18f));

        glfwSwapBuffers(window);
        glfwPollEvents();
    }

    glfwTerminate();
    return 0;
}
