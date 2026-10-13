import os
import zipfile

root = r"C:\Users\User\Desktop\shrek_outhouse_opengl"
os.makedirs(root, exist_ok=True)

files = {}

files["main.cpp"] = r'''#include <glad/glad.h>
#include <GLFW/glfw3.h>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#include <algorithm>
#include <cmath>
#include <cstdlib>
#include <iostream>
#include <string>
#include <vector>

static const int WINDOW_W = 1280;
static const int WINDOW_H = 720;

const char* VERTEX_SHADER = R"GLSL(
#version 330 core
layout (location = 0) in vec3 aPos;
uniform mat4 uModel;
uniform mat4 uView;
uniform mat4 uProjection;
out vec3 vWorldPos;
void main() {
    vec4 world = uModel * vec4(aPos, 1.0);
    vWorldPos = world.xyz;
    gl_Position = uProjection * uView * world;
}
)GLSL";

const char* FRAGMENT_SHADER = R"GLSL(
#version 330 core
in vec3 vWorldPos;
uniform vec3 uColor;
uniform vec3 uLightDir;
out vec4 FragColor;
void main() {
    vec3 normal = normalize(cross(dFdx(vWorldPos), dFdy(vWorldPos)));
    float diffuse = max(dot(normal, normalize(-uLightDir)), 0.0);
    float light = 0.38 + diffuse * 0.72;
    FragColor = vec4(uColor * light, 1.0);
}
)GLSL";

GLuint makeShader(GLenum type, const char* src) {
    GLuint shader = glCreateShader(type);
    glShaderSource(shader, 1, &src, nullptr);
    glCompileShader(shader);
    GLint ok = GL_FALSE;
    glGetShaderiv(shader, GL_COMPILE_STATUS, &ok);
    if (!ok) {
        char log[2048]{};
        glGetShaderInfoLog(shader, sizeof(log), nullptr, log);
        std::cerr << "Shader compilation error:\n" << log << '\n';
    }
    return shader;
}

GLuint makeProgram() {
    GLuint vs = makeShader(GL_VERTEX_SHADER, VERTEX_SHADER);
    GLuint fs = makeShader(GL_FRAGMENT_SHADER, FRAGMENT_SHADER);
    GLuint program = glCreateProgram();
    glAttachShader(program, vs);
    glAttachShader(program, fs);
    glLinkProgram(program);
    GLint ok = GL_FALSE;
    glGetProgramiv(program, GL_LINK_STATUS, &ok);
    if (!ok) {
        char log[2048]{};
        glGetProgramInfoLog(program, sizeof(log), nullptr, log);
        std::cerr << "Shader link error:\n" << log << '\n';
    }
    glDeleteShader(vs);
    glDeleteShader(fs);
    return program;
}

struct Mesh {
    GLuint vao = 0, vbo = 0;
    GLsizei count = 0;
};

Mesh makeMesh(const std::vector<float>& vertices) {
    Mesh mesh;
    mesh.count = static_cast<GLsizei>(vertices.size() / 3);
    glGenVertexArrays(1, &mesh.vao);
    glGenBuffers(1, &mesh.vbo);
    glBindVertexArray(mesh.vao);
    glBindBuffer(GL_ARRAY_BUFFER, mesh.vbo);
    glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(float), vertices.data(), GL_STATIC_DRAW);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);
    glBindVertexArray(0);
    return mesh;
}

Mesh makeCube() {
    const float v[] = {
        // back
        -0.5f,-0.5f,-0.5f,  0.5f,0.5f,-0.5f,  0.5f,-0.5f,-0.5f,
         0.5f,0.5f,-0.5f, -0.5f,-0.5f,-0.5f, -0.5f,0.5f,-0.5f,
        // front
        -0.5f,-0.5f, 0.5f,  0.5f,-0.5f,0.5f,  0.5f,0.5f,0.5f,
         0.5f,0.5f,0.5f, -0.5f,0.5f,0.5f, -0.5f,-0.5f,0.5f,
        // left
        -0.5f,-0.5f,-0.5f, -0.5f,-0.5f,0.5f, -0.5f,0.5f,0.5f,
        -0.5f,0.5f,0.5f, -0.5f,0.5f,-0.5f, -0.5f,-0.5f,-0.5f,
        // right
         0.5f,-0.5f,-0.5f,  0.5f,0.5f,0.5f,  0.5f,-0.5f,0.5f,
         0.5f,0.5f,0.5f,  0.5f,-0.5f,-0.5f,  0.5f,0.5f,-0.5f,
        // bottom
        -0.5f,-0.5f,-0.5f,  0.5f,-0.5f,0.5f,  0.5f,-0.5f,-0.5f,
         0.5f,-0.5f,0.5f, -0.5f,-0.5f,-0.5f, -0.5f,-0.5f,0.5f,
        // top
        -0.5f,0.5f,-0.5f,  0.5f,0.5f,-0.5f,  0.5f,0.5f,0.5f,
         0.5f,0.5f,0.5f, -0.5f,0.5f,0.5f, -0.5f,0.5f,-0.5f
    };
    return makeMesh(std::vector<float>(std::begin(v), std::end(v)));
}

Mesh makeSphere(int slices = 18, int stacks = 12) {
    std::vector<float> v;
    for (int y = 0; y < stacks; ++y) {
        float lat0 = -glm::half_pi<float>() + glm::pi<float>() * y / stacks;
        float lat1 = -glm::half_pi<float>() + glm::pi<float>() * (y + 1) / stacks;
        for (int x = 0; x < slices; ++x) {
            float lon0 = glm::two_pi<float>() * x / slices;
            float lon1 = glm::two_pi<float>() * (x + 1) / slices;
            auto p = [](float lat, float lon) {
                return glm::vec3(std::cos(lat) * std::cos(lon),
                                 std::sin(lat),
                                 std::cos(lat) * std::sin(lon));
            };
            glm::vec3 a=p(lat0,lon0), b=p(lat0,lon1), c=p(lat1,lon1), d=p(lat1,lon0);
            for (glm::vec3 q : {a,b,c, a,c,d}) {
                v.push_back(q.x); v.push_back(q.y); v.push_back(q.z);
            }
        }
    }
    return makeMesh(v);
}

GLuint program;
Mesh cube, sphere;
GLint uModel, uView, uProjection, uColor, uLightDir;
glm::mat4 view, projection;

void drawMesh(const Mesh& mesh, const glm::mat4& model, const glm::vec3& color) {
    glUniformMatrix4fv(uModel, 1, GL_FALSE, glm::value_ptr(model));
    glUniform3fv(uColor, 1, glm::value_ptr(color));
    glBindVertexArray(mesh.vao);
    glDrawArrays(GL_TRIANGLES, 0, mesh.count);
}

glm::mat4 transform(glm::vec3 position, glm::vec3 scale, float rotationY = 0.0f) {
    glm::mat4 m(1.0f);
    m = glm::translate(m, position);
    m = glm::rotate(m, rotationY, glm::vec3(0,1,0));
    return glm::scale(m, scale);
}

void box(glm::vec3 p, glm::vec3 s, glm::vec3 c, float rotY = 0.0f) {
    drawMesh(cube, transform(p, s, rotY), c);
}
void ball(glm::vec3 p, glm::vec3 s, glm::vec3 c) {
    drawMesh(sphere, transform(p, s), c);
}

struct Player {
    float x = -2.5f;
    float y = 0.0f;
    float vy = 0.0f;
    float direction = 1.0f;
    bool onGround = true;
    float walkTime = 0.0f;
} player;

bool doorOpen = false;
float doorAngle = 0.0f;
bool fWasDown = false;
bool upWasDown = false;

void drawShrek(float time) {
    glm::mat4 root(1.0f);
    root = glm::translate(root, glm::vec3(player.x, player.y, 0.65f));
    root = glm::rotate(root, player.direction < 0 ? glm::pi<float>() : 0.0f, glm::vec3(0,1,0));
    auto part = [&](glm::vec3 p, glm::vec3 s, glm::vec3 color, bool isSphere = false) {
        glm::mat4 m = glm::translate(root, p);
        m = glm::scale(m, s);
        drawMesh(isSphere ? sphere : cube, m, color);
    };
    const glm::vec3 green(0.48f, 0.72f, 0.12f);
    const glm::vec3 darkGreen(0.32f, 0.52f, 0.07f);
    const glm::vec3 shirt(0.88f, 0.82f, 0.65f);
    const glm::vec3 vest(0.30f, 0.20f, 0.10f);
    const glm::vec3 pants(0.28f, 0.18f, 0.09f);
    const glm::vec3 boots(0.12f, 0.08f, 0.045f);

    float swing = (!player.onGround) ? 0.0f : std::sin(time * 10.0f) * 0.16f;
    // Legs and boots
    part({-0.16f,0.30f,0},{0.18f,0.48f,0.22f},pants,true);
    part({ 0.16f,0.30f,0},{0.18f,0.48f,0.22f},pants,true);
    part({-0.16f+swing,0.08f,0.07f},{0.28f,0.13f,0.34f},boots,true);
    part({ 0.16f-swing,0.08f,0.07f},{0.28f,0.13f,0.34f},boots,true);
    // Body, tunic and vest
    part({0.0f,0.94f,0},{0.66f,0.70f,0.38f},shirt,true);
    part({0.0f,1.00f,0.205f},{0.43f,0.55f,0.08f},vest,true);
    // Arms and hands
    part({-0.40f,0.95f,0},{0.17f,0.50f,0.19f},shirt,true);
    part({ 0.40f,0.95f,0},{0.17f,0.50f,0.19f},shirt,true);
    part({-0.43f,0.67f,0.02f},{0.19f,0.19f,0.20f},green,true);
    part({ 0.43f,0.67f,0.02f},{0.19f,0.19f,0.20f},green,true);
    // Neck and head
    part({0.0f,1.36f,0},{0.25f,0.22f,0.24f},green,true);
    part({0.0f,1.66f,0.02f},{0.53f,0.53f,0.46f},green,true);
    // Ears: green horizontal ogre-like ears
    part({-0.34f,1.72f,0},{0.36f,0.13f,0.14f},darkGreen,true);
    part({ 0.34f,1.72f,0},{0.36f,0.13f,0.14f},darkGreen,true);
    part({-0.20f,1.78f,0.23f},{0.13f,0.12f,0.07f},{0.96f,0.94f,0.77f},true);
    part({ 0.20f,1.78f,0.23f},{0.13f,0.12f,0.07f},{0.96f,0.94f,0.77f},true);
    part({-0.20f,1.78f,0.29f},{0.055f,0.07f,0.035f},{0.08f,0.08f,0.04f},true);
    part({ 0.20f,1.78f,0.29f},{0.055f,0.07f,0.035f},{0.08f,0.08f,0.04f},true);
    part({0.0f,1.59f,0.27f},{0.23f,0.14f,0.16f},green,true);
    part({0.0f,1.53f,0.34f},{0.19f,0.055f,0.035f},{0.20f,0.08f,0.05f},true);
}

void drawOuthouse() {
    const glm::vec3 wood(0.32f,0.19f,0.09f);
    const glm::vec3 woodLight(0.43f,0.27f,0.13f);
    const glm::vec3 dark(0.12f,0.075f,0.04f);
    const float x = 3.4f, z = -0.25f;
    // Body and roof
    box({x,1.15f,z},{1.75f,2.3f,1.55f},wood);
    // Front planks create visible wood texture
    for (int i=0;i<7;i++) {
        float px = x - 0.70f + i*0.23f;
        box({px,1.15f,z+0.79f},{0.035f,2.12f,0.035f}, i%2 ? woodLight : dark);
    }
    box({x,2.34f,z},{2.0f,0.20f,1.82f},{0.20f,0.13f,0.07f});
    // Simple pitched roof
    box({x-0.47f,2.52f,z},{1.10f,0.20f,1.95f},{0.24f,0.16f,0.08f},-0.20f);
    box({x+0.47f,2.52f,z},{1.10f,0.20f,1.95f},{0.24f,0.16f,0.08f},0.20f);
    // Roof grass tufts
    for (int i=0;i<35;i++) {
        float gx = x - 0.88f + (i%12)*0.16f;
        float gz = z - 0.78f + (i/12)*0.55f;
        box({gx,2.68f + 0.025f*(i%3),gz},{0.10f,0.12f,0.10f},
            (i%2)?glm::vec3(0.20f,0.43f,0.08f):glm::vec3(0.30f,0.55f,0.12f));
    }
    // Dark doorway behind hinged door
    box({x-0.02f,1.08f,z+0.805f},{0.90f,1.85f,0.04f},{0.055f,0.035f,0.02f});
    // Door rotates around its left edge
    glm::mat4 door(1.0f);
    door = glm::translate(door, glm::vec3(x-0.47f,1.08f,z+0.86f));
    door = glm::rotate(door, doorAngle, glm::vec3(0,1,0));
    door = glm::translate(door, glm::vec3(0.45f,0,0));
    door = glm::scale(door, glm::vec3(0.90f,1.82f,0.10f));
    drawMesh(cube, door, woodLight);
    // Plank lines on the door, transformed with the door
    for (int i=0;i<4;i++) {
        glm::mat4 line(1.0f);
        line = glm::translate(line, glm::vec3(x-0.47f,1.08f,z+0.925f));
        line = glm::rotate(line, doorAngle, glm::vec3(0,1,0));
        line = glm::translate(line, glm::vec3(0.13f + i*0.21f,0,0));
        line = glm::scale(line, glm::vec3(0.025f,1.78f,0.018f));
        drawMesh(cube,line,dark);
    }
    // Handle
    glm::mat4 handle(1.0f);
    handle = glm::translate(handle, glm::vec3(x-0.47f,1.08f,z+0.99f));
    handle = glm::rotate(handle, doorAngle, glm::vec3(0,1,0));
    handle = glm::translate(handle, glm::vec3(0.76f,0.05f,0));
    handle = glm::scale(handle, glm::vec3(0.09f,0.09f,0.05f));
    drawMesh(sphere,handle,{0.82f,0.63f,0.22f});
    // Step
    box({x,0.12f,z+1.05f},{1.15f,0.24f,0.48f},{0.38f,0.24f,0.12f});
}

void drawWorld(float time) {
    // Grass-covered terrain slab
    box({0.0f,-0.32f,0.0f},{12.0f,0.60f,8.0f},{0.22f,0.35f,0.10f});
    box({0.0f,-0.015f,0.0f},{11.95f,0.08f,7.95f},{0.20f,0.46f,0.12f});
    // Procedural grass blades on the surface (fixed pattern, no random flicker)
    for (int i=0;i<520;i++) {
        float gx = -5.7f + std::fmod(i*1.731f,11.4f);
        float gz = -3.7f + std::fmod(i*2.317f,7.4f);
        if (gx > 2.3f && gx < 4.5f && gz > -1.7f && gz < 1.0f) continue;
        float h = 0.08f + 0.10f * float((i*7)%11)/10.0f;
        float lean = float((i%5)-2)*0.035f;
        box({gx+lean,-0.005f+h*0.5f,gz},{0.035f,h,0.035f},
            (i%3==0)?glm::vec3(0.32f,0.58f,0.13f):glm::vec3(0.13f,0.37f,0.07f),
            float(i%7)*0.25f);
    }
    // Fence at the back and sides
    for (int i=0;i<17;i++) {
        float x=-5.6f+i*0.70f;
        box({x,0.48f,-3.45f},{0.12f,1.0f,0.12f},{0.28f,0.17f,0.08f});
        box({x,0.78f,-3.45f},{0.15f,0.12f,0.16f},{0.38f,0.23f,0.10f});
    }
    box({0,0.48f,-3.45f},{11.6f,0.13f,0.12f},{0.38f,0.23f,0.10f});
    box({0,0.20f,-3.45f},{11.6f,0.13f,0.12f},{0.38f,0.23f,0.10f});
    for (int i=0;i<11;i++) {
        float z=-3.1f+i*0.62f;
        box({-5.7f,0.45f,z},{0.12f,0.90f,0.12f},{0.28f,0.17f,0.08f});
        box({ 5.7f,0.45f,z},{0.12f,0.90f,0.12f},{0.28f,0.17f,0.08f});
    }
    // Decorative rocks
    ball({-4.2f,0.22f,0.6f},{1.0f,0.62f,0.78f},{0.31f,0.32f,0.29f});
    ball({-3.8f,0.16f,0.85f},{0.65f,0.42f,0.58f},{0.38f,0.37f,0.32f});
    ball({5.0f,0.20f,-0.8f},{0.85f,0.55f,0.72f},{0.34f,0.34f,0.31f});
    ball({1.4f,0.12f,-2.0f},{0.48f,0.30f,0.42f},{0.39f,0.38f,0.33f});
    drawOuthouse();
    drawShrek(time);
}

void keyCallback(GLFWwindow* window, int key, int, int action, int) {
    if (key == GLFW_KEY_ESCAPE && action == GLFW_PRESS)
        glfwSetWindowShouldClose(window, GLFW_TRUE);
}

int main() {
    if (!glfwInit()) {
        std::cerr << "Failed to initialize GLFW\n";
        return 1;
    }
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
#ifdef __APPLE__
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
#endif

    GLFWwindow* window = glfwCreateWindow(WINDOW_W, WINDOW_H, "Shrek's Outhouse - OpenGL", nullptr, nullptr);
    if (!window) {
        std::cerr << "Failed to create GLFW window\n";
        glfwTerminate();
        return 1;
    }
    glfwMakeContextCurrent(window);
    glfwSetKeyCallback(window, keyCallback);
    glfwSwapInterval(1);

    if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress)) {
        std::cerr << "Failed to initialize GLAD\n";
        glfwDestroyWindow(window);
        glfwTerminate();
        return 1;
    }

    glEnable(GL_DEPTH_TEST);
    glDisable(GL_CULL_FACE);
    program = makeProgram();
    cube = makeCube();
    sphere = makeSphere();
    uModel = glGetUniformLocation(program, "uModel");
    uView = glGetUniformLocation(program, "uView");
    uProjection = glGetUniformLocation(program, "uProjection");
    uColor = glGetUniformLocation(program, "uColor");
    uLightDir = glGetUniformLocation(program, "uLightDir");

    projection = glm::perspective(glm::radians(48.0f),
        float(WINDOW_W) / float(WINDOW_H), 0.1f, 100.0f);

    float previous = static_cast<float>(glfwGetTime());
    while (!glfwWindowShouldClose(window)) {
        float now = static_cast<float>(glfwGetTime());
        float dt = std::min(now - previous, 0.04f);
        previous = now;

        // Left/right movement, clamped to the grassy play area
        if (glfwGetKey(window, GLFW_KEY_LEFT) == GLFW_PRESS) {
            player.x -= 3.0f * dt;
            player.direction = -1.0f;
            player.walkTime += dt;
        }
        if (glfwGetKey(window, GLFW_KEY_RIGHT) == GLFW_PRESS) {
            player.x += 3.0f * dt;
            player.direction = 1.0f;
            player.walkTime += dt;
        }
        player.x = std::clamp(player.x, -5.15f, 5.15f);

        bool upDown = glfwGetKey(window, GLFW_KEY_UP) == GLFW_PRESS;
        if (upDown && !upWasDown && player.onGround) {
            player.vy = 5.4f;
            player.onGround = false;
        }
        upWasDown = upDown;
        player.y += player.vy * dt;
        if (!player.onGround) {
            player.vy -= 13.5f * dt;
            if (player.y <= 0.0f) {
                player.y = 0.0f;
                player.vy = 0.0f;
                player.onGround = true;
            }
        }

        bool fDown = glfwGetKey(window, GLFW_KEY_F) == GLFW_PRESS;
        if (fDown && !fWasDown && std::abs(player.x - 3.4f) < 2.0f && player.y < 0.15f)
            doorOpen = !doorOpen;
        fWasDown = fDown;
        float targetAngle = doorOpen ? glm::radians(-105.0f) : 0.0f;
        doorAngle += (targetAngle - doorAngle) * std::min(1.0f, dt * 5.0f);

        int w, h;
        glfwGetFramebufferSize(window, &w, &h);
        glViewport(0, 0, w, h);
        glClearColor(0.20f, 0.31f, 0.38f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        view = glm::lookAt(glm::vec3(0.0f, 5.7f, 11.8f),
                           glm::vec3(0.0f, 0.9f, 0.0f),
                           glm::vec3(0.0f, 1.0f, 0.0f));
        glUseProgram(program);
        glUniformMatrix4fv(uView, 1, GL_FALSE, glm::value_ptr(view));
        glm::mat4 proj = glm::perspective(glm::radians(48.0f),
            float(std::max(w,1)) / float(std::max(h,1)), 0.1f, 100.0f);
        glUniformMatrix4fv(uProjection, 1, GL_FALSE, glm::value_ptr(proj));
        glUniform3f(uLightDir, -0.4f, -1.0f, -0.3f);

        drawWorld(now);
        glfwSwapBuffers(window);
        glfwPollEvents();
    }

    glDeleteVertexArrays(1, &cube.vao);
    glDeleteBuffers(1, &cube.vbo);
    glDeleteVertexArrays(1, &sphere.vao);
    glDeleteBuffers(1, &sphere.vbo);
    glDeleteProgram(program);
    glfwDestroyWindow(window);
    glfwTerminate();
    return 0;
}
'''

files["CMakeLists.txt"] = r'''cmake_minimum_required(VERSION 3.16)
project(ShrekOuthouse LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(OpenGL REQUIRED)
find_package(glfw3 CONFIG REQUIRED)
find_package(glm CONFIG REQUIRED)
find_package(glad CONFIG REQUIRED)

add_executable(ShrekOuthouse main.cpp)
target_link_libraries(ShrekOuthouse PRIVATE OpenGL::GL glfw glm::glm glad::glad)
'''

files["rebuild_and_run.bat"] = r'''@echo off
setlocal
if not exist build mkdir build
cmake -S . -B build -G "MinGW Makefiles"
if errorlevel 1 goto error
cmake --build build
if errorlevel 1 goto error
if exist build\ShrekOuthouse.exe (
  build\ShrekOuthouse.exe
) else (
  echo Executable not found. Check CMake output.
  pause
)
goto end
:error
echo Build failed. Check that CMake, MinGW and the GLFW, GLM, GLAD packages are installed.
pause
:end
endlocal
'''

files["README_RU.txt"] = r'''Shrek's Outhouse — OpenGL starter project

Что уже реализовано:
- 3D-сцена на GLFW + GLAD + GLM и GLSL;
- ограниченный участок травы, забор, камни и деревянный туалет;
- стилизованный персонаж-огр, собранный из геометрических примитивов;
- стрелки влево/вправо для движения;
- стрелка вверх для прыжка;
- F рядом с туалетом открывает/закрывает дверь;
- Esc закрывает приложение.

ВАЖНО:
Персонаж пока сделан из примитивов, это не детализированная импортированная модель.
Изображение-концепт не является скриншотом запущенного приложения.

Зависимости:
Нужны CMake, MinGW-w64, OpenGL, GLFW, GLM и GLAD, установленные так,
чтобы CMake find_package мог найти glfw3, glm и glad.
Пример: vcpkg с интеграцией в CMake либо собственные CMake package config файлы.

Сборка:
1. Открой PowerShell в папке проекта.
2. Если зависимости установлены через vcpkg, укажи toolchain-файл при конфигурации:
   cmake -S . -B build -G "MinGW Makefiles" -DCMAKE_TOOLCHAIN_FILE="C:/vcpkg/scripts/buildsystems/vcpkg.cmake"
3. cmake --build build
4. Запусти build/ShrekOuthouse.exe

Либо настрой путь к vcpkg/toolchain в CMake GUI и запусти rebuild_and_run.bat.
Если у тебя уже есть собственные CMake-цели для GLFW/GLAD/GLM, адаптируй
find_package и target_link_libraries под названия целей в твоём проекте.
'''

for name, content in files.items():
    with open(os.path.join(root, name), "w", encoding="utf-8", newline="\n") as f:
        f.write(content)

zip_path = os.path.join(r"C:\Users\User\Desktop", "ShrekOuthouse_OpenGL_starter.zip")
with zipfile.ZipFile(zip_path, "w", zipfile.ZIP_DEFLATED) as z:
    for name in files:
        z.write(os.path.join(root, name), arcname=f"ShrekOuthouse/{name}")

print(f"Создан архив: {zip_path}")
print("Файлы:", ", ".join(files.keys()))
