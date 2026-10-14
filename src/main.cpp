#include <glad/glad.h>
#include <GLFW/glfw3.h>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#include <algorithm>
#include <cmath>
#include <cstdlib>
#include <filesystem>
#include <iostream>
#include <string>
#include <vector>

#ifdef _WIN32
#include <windows.h>
#include <mmsystem.h>
#endif

#include "Camera.h"

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

Camera camera(glm::vec3(0.0f, 5.7f, 11.8f), glm::vec3(0.0f, 1.0f, 0.0f), -90.0f, -25.0f);
glm::vec3 cameraTarget(0.0f, 1.2f, 0.0f);
float cameraDistance = 11.8f;
float lastMouseX = 0.0f;
float lastMouseY = 0.0f;
bool isDraggingCamera = false;

void UpdateCameraOrbit() {
    camera.UpdateCameraVectors();
    camera.Position = cameraTarget - camera.Front * cameraDistance;
}

void ScrollCallback(GLFWwindow* window, double xOffset, double yOffset) {
    cameraDistance = std::clamp(cameraDistance - static_cast<float>(yOffset) * 0.6f, 6.0f, 24.0f);
    UpdateCameraOrbit();
}

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
    float z = 0.65f;
    float y = 0.0f;
    float vy = 0.0f;
    float direction = 1.0f;
    float rotation = 0.0f;
    bool onGround = true;
    float walkTime = 0.0f;
} player;
bool playerWalking = false;

bool doorOpen = false;
float doorAngle = 0.0f;
bool fWasDown = false;
bool gWasDown = false;
bool upWasDown = false;
bool armRaised = false;
bool rWasDown = false;
float armLift = 0.0f;

enum class ToiletSequencePhase {
    Idle,
    WalkingToDoor,
    FacingCameraBeforeEntry,
    MassagingBelly,
    TurningToDoor,
    OpeningIn,
    WalkingIn,
    ClosingIn,
    WaitingInside,
    OpeningOut,
    TurningOut,
    WalkingOut,
    FinalTurn,
    ClosingOut,
    FinalPose
};

struct ToiletSequence {
    bool active = false;
    bool exitMusicStarted = false;
    ToiletSequencePhase phase = ToiletSequencePhase::Idle;
    float timer = 0.0f;
    glm::vec3 from{0.0f, 0.0f, 0.0f};
    glm::vec3 to{0.0f, 0.0f, 0.0f};
};

ToiletSequence toiletSequence;
bool shrekHappyAfterToilet = false;

float cameraFacingRotation() {
    // Character forward is local +Z. Face toward the camera's horizontal position.
    const float yaw = glm::radians(camera.Yaw);
    return std::atan2(-std::cos(yaw), -std::sin(yaw));
}

struct StoneState {
    bool inHand = false;
    bool flying = false;
    glm::vec3 position{0.0f, 0.0f, 0.0f};
    glm::vec3 velocity{0.0f, 0.0f, 0.0f};
};

StoneState stone;
bool stoneBounceSoundPlayed = false;

bool exitSongDeviceOpen = false;
bool bellyGrowlDeviceOpen = false;
bool shrekSighDeviceOpen = false;
bool stoneBounceDeviceOpen = false;
bool shrekVictoryDeviceOpen = false;

void PlayAudioAsset(const char* filename, const char* alias, bool& deviceOpen, int maxDurationMs = 0) {
#ifdef _WIN32
    if (deviceOpen) {
        const std::string stopCommand = std::string("stop ") + alias;
        const std::string closeCommand = std::string("close ") + alias;
        mciSendStringA(stopCommand.c_str(), nullptr, 0, nullptr);
        mciSendStringA(closeCommand.c_str(), nullptr, 0, nullptr);
        deviceOpen = false;
    }

    std::filesystem::path track = std::filesystem::current_path() / "assets" / filename;
    if (!std::filesystem::exists(track)) {
        track = std::filesystem::current_path().parent_path() / "assets" / filename;
    }
    if (!std::filesystem::exists(track)) {
        std::cerr << "Audio file not found: " << track.string() << std::endl;
        return;
    }

    const std::string openCommand = std::string("open \"") + track.string() + "\" type MPEGVideo alias " + alias;
    MCIERROR error = mciSendStringA(openCommand.c_str(), nullptr, 0, nullptr);
    if (error != 0) {
        char message[MAXERRORLENGTH]{};
        mciGetErrorStringA(error, message, MAXERRORLENGTH);
        std::cerr << "Could not open " << filename << ": " << message << std::endl;
        return;
    }
    deviceOpen = true;

    std::string playCommand = std::string("play ") + alias + " from 0";
    if (maxDurationMs > 0) {
        playCommand += " to " + std::to_string(maxDurationMs);
    }
    error = mciSendStringA(playCommand.c_str(), nullptr, 0, nullptr);
    if (error != 0) {
        char message[MAXERRORLENGTH]{};
        mciGetErrorStringA(error, message, MAXERRORLENGTH);
        std::cerr << "Could not play " << filename << ": " << message << std::endl;
        const std::string closeCommand = std::string("close ") + alias;
        mciSendStringA(closeCommand.c_str(), nullptr, 0, nullptr);
        deviceOpen = false;
    }
#else
    (void)filename;
    (void)alias;
    (void)deviceOpen;
    (void)maxDurationMs;
#endif
}

void StopAudioAsset(const char* alias, bool& deviceOpen) {
#ifdef _WIN32
    if (deviceOpen) {
        const std::string stopCommand = std::string("stop ") + alias;
        const std::string closeCommand = std::string("close ") + alias;
        mciSendStringA(stopCommand.c_str(), nullptr, 0, nullptr);
        mciSendStringA(closeCommand.c_str(), nullptr, 0, nullptr);
        deviceOpen = false;
    }
#else
    (void)alias;
    (void)deviceOpen;
#endif
}

void StartExitSong() {
#ifdef _WIN32
    if (exitSongDeviceOpen) {
        mciSendStringA("stop shrek_exit_song", nullptr, 0, nullptr);
        mciSendStringA("close shrek_exit_song", nullptr, 0, nullptr);
        exitSongDeviceOpen = false;
    }

    std::filesystem::path track = std::filesystem::current_path() / "assets" / "All Star.mp3";
    if (!std::filesystem::exists(track)) {
        track = std::filesystem::current_path().parent_path() / "assets" / "All Star.mp3";
    }
    if (!std::filesystem::exists(track)) {
        std::cerr << "To play the exit music, place All Star.mp3 at: " << track.string() << std::endl;
        return;
    }

    const std::string openCommand = "open \"" + track.string() + "\" type MPEGVideo alias shrek_exit_song";
    MCIERROR error = mciSendStringA(openCommand.c_str(), nullptr, 0, nullptr);
    if (error != 0) {
        char message[MAXERRORLENGTH]{};
        mciGetErrorStringA(error, message, MAXERRORLENGTH);
        std::cerr << "Could not open exit music: " << message << std::endl;
        return;
    }
    exitSongDeviceOpen = true;

    // Play from the beginning; the indoor wait lets the lead-in run before the door opens.
    error = mciSendStringA("play shrek_exit_song from 0", nullptr, 0, nullptr);
    if (error != 0) {
        char message[MAXERRORLENGTH]{};
        mciGetErrorStringA(error, message, MAXERRORLENGTH);
        std::cerr << "Could not play exit music: " << message << std::endl;
        mciSendStringA("close shrek_exit_song", nullptr, 0, nullptr);
        exitSongDeviceOpen = false;
    }
#else
    std::cerr << "Exit music playback is currently implemented for Windows." << std::endl;
#endif
}

void StopExitSong() {
#ifdef _WIN32
    if (exitSongDeviceOpen) {
        mciSendStringA("stop shrek_exit_song", nullptr, 0, nullptr);
        mciSendStringA("close shrek_exit_song", nullptr, 0, nullptr);
        exitSongDeviceOpen = false;
    }
#endif
}

void drawShrek(float time) {
    glm::mat4 root(1.0f);
    root = glm::translate(root, glm::vec3(player.x, player.y, player.z));
    root = glm::rotate(root, player.rotation, glm::vec3(0, 1, 0));
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

    float swing = (playerWalking && player.onGround) ? std::sin(player.walkTime * 10.0f) * 0.16f : 0.0f;

    // Two straight arm segments meet at the elbow, halfway from shoulder to hand.
    auto drawSegment = [&](glm::vec3 a, glm::vec3 b, glm::vec3 color) {
        glm::vec3 dir = b - a;
        float length = glm::length(dir);
        if (length < 0.001f) return;

        glm::vec3 dirN = glm::normalize(dir);
        glm::vec3 up(0.0f, 1.0f, 0.0f);
        glm::vec3 axis = glm::cross(up, dirN);
        float angle = std::acos(glm::clamp(glm::dot(up, dirN), -1.0f, 1.0f));

        glm::mat4 m = glm::translate(root, (a + b) * 0.5f);
        if (glm::length(axis) > 0.0001f) {
            m = glm::rotate(m, angle, glm::normalize(axis));
        }
        else if (dirN.y < 0.0f) {
            m = glm::rotate(m, glm::pi<float>(), glm::vec3(1.0f, 0.0f, 0.0f));
        }
        m = glm::scale(m, glm::vec3(0.12f, length, 0.12f));
        drawMesh(cube, m, color);
    };

    const float leftLift = glm::clamp(armLift, 0.0f, 1.0f);
    const bool finalPose = toiletSequence.active && toiletSequence.phase == ToiletSequencePhase::FinalPose;
    const bool massaging = toiletSequence.active && toiletSequence.phase == ToiletSequencePhase::MassagingBelly;
    const glm::vec3 leftShoulder(-0.38f, 1.26f, 0.0f);
    const glm::vec3 upperArmRest(0.0f, -0.34f, 0.06f);
    const glm::vec3 lowerArmRest(0.0f, -0.34f, 0.06f);
    const glm::vec3 upperArmRaised(0.0f, 0.08f, 0.29f);
    const glm::vec3 lowerArmRaised(0.0f, 0.088f, 0.29f);
    glm::vec3 leftElbow;
    glm::vec3 leftHandTarget;
    if (massaging) {
        const float rubAngle = time * 8.0f;
        leftElbow = glm::vec3(-0.14f + 0.035f * std::cos(rubAngle), 1.03f + 0.025f * std::sin(rubAngle), 0.27f);
        leftHandTarget = glm::vec3(0.02f + 0.12f * std::cos(rubAngle), 0.91f + 0.06f * std::sin(rubAngle), 0.48f);
    } else if (finalPose) {
        const float wave = std::sin(time * 12.0f) * 0.055f;
        leftElbow = glm::vec3(-0.31f, 1.40f, 0.16f);
        leftHandTarget = glm::vec3(-0.24f + wave, 1.68f, 0.29f);
    } else {
        const glm::vec3 upperArm = glm::mix(upperArmRest, upperArmRaised, leftLift);
        const glm::vec3 lowerArm = glm::mix(lowerArmRest, lowerArmRaised, leftLift);
        leftElbow = leftShoulder + upperArm;
        leftHandTarget = leftElbow + lowerArm;
    }

    if (stone.flying) {
        glm::mat4 thrownStone = glm::translate(glm::mat4(1.0f), stone.position);
        thrownStone = glm::scale(thrownStone, glm::vec3(0.18f, 0.18f, 0.18f));
        drawMesh(sphere, thrownStone, glm::vec3(0.55f, 0.55f, 0.55f));
    }

    // Mirror the resting left-arm proportions; only the left arm animates.
    const glm::vec3 rightShoulder(0.38f, 1.26f, 0.0f);
    const glm::vec3 rightElbow = rightShoulder + upperArmRest;
    const glm::vec3 rightHand = rightElbow + lowerArmRest;

    // Legs and boots
    part({-0.16f,0.30f,0},{0.18f,0.48f,0.22f},pants,true);
    part({ 0.16f,0.30f,0},{0.18f,0.48f,0.22f},pants,true);
    part({-0.16f+swing,0.08f,0.07f},{0.28f,0.13f,0.34f},boots,true);
    part({ 0.16f-swing,0.08f,0.07f},{0.28f,0.13f,0.34f},boots,true);
    // Body, tunic and vest
    part({0.0f,0.94f,0},{0.58f,0.70f,0.38f},shirt,true);
    part({0.0f,1.00f,0.205f},{0.39f,0.55f,0.08f},vest,true);
    // Neck and head
    part({0.0f,1.36f,0},{0.25f,0.22f,0.24f},green,true);
    part({0.0f,1.66f,0.02f},{0.53f,0.53f,0.46f},green,true);
    // Ears: green horizontal ogre-like ears
    part({-0.34f,1.72f,0},{0.36f,0.13f,0.14f},darkGreen,true);
    part({ 0.34f,1.72f,0},{0.36f,0.13f,0.14f},darkGreen,true);

    // Large, visible face block
    part({-0.20f,1.78f,0.23f},{0.13f,0.12f,0.07f},{0.96f,0.94f,0.77f},true);
    part({ 0.20f,1.78f,0.23f},{0.13f,0.12f,0.07f},{0.96f,0.94f,0.77f},true);
    part({-0.20f,1.78f,0.29f},{0.055f,0.07f,0.035f},{0.08f,0.08f,0.04f},true);
    part({ 0.20f,1.78f,0.29f},{0.055f,0.07f,0.035f},{0.08f,0.08f,0.04f},true);

    // His brows tighten while his stomach hurts and relax into a happy look afterward.
    const float browY = massaging ? 1.85f : (shrekHappyAfterToilet || finalPose ? 1.93f : 1.89f);
    part({-0.19f,browY,0.33f},{0.23f,0.06f,0.06f},darkGreen,true);
    part({ 0.19f,browY,0.33f},{0.23f,0.06f,0.06f},darkGreen,true);
    part({-0.19f,1.80f,0.40f},{0.14f,0.10f,0.08f},{0.96f,0.94f,0.77f},true);
    part({ 0.19f,1.80f,0.40f},{0.14f,0.10f,0.08f},{0.96f,0.94f,0.77f},true);
    part({-0.19f,1.80f,0.49f},{0.07f,0.07f,0.05f},{0.05f,0.05f,0.05f},true);
    part({ 0.19f,1.80f,0.49f},{0.07f,0.07f,0.05f},{0.05f,0.05f,0.05f},true);
    part({0.0f,1.58f,0.43f},{0.19f,0.12f,0.12f},{0.22f,0.12f,0.06f},true);
    part({0.0f,1.42f,0.42f},{0.26f,0.09f,0.07f},darkGreen,true);
    part({-0.09f,1.45f,0.48f},{0.06f,0.04f,0.03f},{0.95f,0.95f,0.95f},true);
    part({ 0.09f,1.45f,0.48f},{0.06f,0.04f,0.03f},{0.95f,0.95f,0.95f},true);
    part({0.0f,1.33f,0.47f},{0.19f,0.05f,0.04f},{0.08f,0.08f,0.08f},true);

    // Open, uneasy mouth during the stomach rub; otherwise keep his friendly smile.
    if (massaging) {
        part({0.0f,1.47f,0.49f},{0.17f,0.14f,0.05f},{0.08f,0.035f,0.03f},true);
        part({0.0f,1.39f,0.49f},{0.10f,0.025f,0.025f},{0.78f,0.65f,0.43f},true);
    } else {
        part({0.0f,1.48f,0.49f},{0.30f,0.10f,0.04f},{0.16f,0.08f,0.08f},false);
        part({-0.18f,1.53f,0.49f},{0.08f,0.06f,0.03f},{0.22f,0.10f,0.08f},false);
        part({ 0.18f,1.53f,0.49f},{0.08f,0.06f,0.03f},{0.22f,0.10f,0.08f},false);
    }
    part({0.0f,1.68f,0.23f},{0.15f,0.09f,0.10f},green,true);
    part({0.0f,1.58f,0.27f},{0.25f,0.18f,0.18f},green,true);

    // Render arms last so the body cannot hide their shoulders and upper arms.
    drawSegment(leftShoulder, leftElbow, shirt);
    drawSegment(leftElbow, leftHandTarget, shirt);
    glm::mat4 leftHand = glm::translate(root, leftHandTarget);
    leftHand = glm::scale(leftHand, glm::vec3(0.18f, 0.18f, 0.18f));
    drawMesh(sphere, leftHand, green);
    if (stone.inHand) {
        glm::mat4 heldStone = glm::translate(root, leftHandTarget + glm::vec3(0.0f, -0.10f, 0.20f));
        heldStone = glm::scale(heldStone, glm::vec3(0.16f, 0.16f, 0.16f));
        drawMesh(sphere, heldStone, glm::vec3(0.55f, 0.55f, 0.55f));
    }
    drawSegment(rightShoulder, rightElbow, shirt);
    drawSegment(rightElbow, rightHand, shirt);
    glm::mat4 rightHandMesh = glm::translate(root, rightHand);
    rightHandMesh = glm::scale(rightHandMesh, glm::vec3(0.18f, 0.18f, 0.18f));
    drawMesh(sphere, rightHandMesh, green);
}

void drawOuthouse() {
    const glm::vec3 wood(0.32f,0.19f,0.09f);
    const glm::vec3 woodLight(0.43f,0.27f,0.13f);
    const glm::vec3 dark(0.12f,0.075f,0.04f);
    const float x = 3.4f, z = -0.25f;
    // Back, side walls and front sections leave a real opening for the door.
    box({x,1.15f,z-0.72f},{1.75f,2.3f,0.11f},wood);
    box({x-0.82f,1.15f,z},{0.11f,2.3f,1.55f},wood);
    box({x+0.82f,1.15f,z},{0.11f,2.3f,1.55f},wood);
    box({x-0.67f,1.15f,z+0.72f},{0.405f,2.3f,0.11f},wood);
    box({x+0.6525f,1.15f,z+0.72f},{0.445f,2.3f,0.11f},wood);
    box({x-0.02f,2.12f,z+0.72f},{0.90f,0.36f,0.11f},wood);
    // Planks on the solid front sections, clear of the doorway.
    for (int i=0;i<3;i++) {
        box({x-0.80f+i*0.14f,1.15f,z+0.79f},{0.035f,2.12f,0.035f}, i%2 ? woodLight : dark);
        box({x+0.50f+i*0.14f,1.15f,z+0.79f},{0.035f,2.12f,0.035f}, i%2 ? dark : woodLight);
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

void StartToiletSequence() {
    const glm::vec3 beforeDoorPoint(3.4f, 0.0f, 1.55f);

    toiletSequence = {};
    toiletSequence.active = true;
    shrekHappyAfterToilet = false;
    toiletSequence.phase = ToiletSequencePhase::WalkingToDoor;
    toiletSequence.from = glm::vec3(player.x, player.y, player.z);
    toiletSequence.to = beforeDoorPoint;
    player.rotation = std::atan2(beforeDoorPoint.x - player.x, beforeDoorPoint.z - player.z);
    doorOpen = false;
}

void UpdateToiletSequence(float dt) {
    auto& seq = toiletSequence;
    if (!seq.active) {
        return;
    }

    seq.timer += dt;

    switch (seq.phase) {
    case ToiletSequencePhase::WalkingToDoor:
        {
            playerWalking = true;
            player.walkTime += dt;
            const glm::vec3 approach(3.4f, 0.0f, 1.55f);
            const float duration = std::max(0.5f, glm::distance(seq.from, approach) / 2.4f);
            const float t = std::clamp(seq.timer / duration, 0.0f, 1.0f);
            player.x = glm::mix(seq.from.x, approach.x, t);
            player.z = glm::mix(seq.from.z, approach.z, t);
            player.y = 0.0f;
            player.rotation = std::atan2(approach.x - seq.from.x, seq.from.z - approach.z);
            if (t >= 1.0f) {
                player.x = approach.x;
                player.z = approach.z;
                PlayAudioAsset("shrek_sigh.mp3", "shrek_sigh", shrekSighDeviceOpen);
                seq.phase = ToiletSequencePhase::FacingCameraBeforeEntry;
                seq.timer = 0.0f;
            }
        }
        break;

    case ToiletSequencePhase::FacingCameraBeforeEntry:
        {
            const float t = std::clamp(seq.timer / 0.45f, 0.0f, 1.0f);
            const float start = std::atan2(3.4f - seq.from.x, seq.from.z - 1.55f);
            const float target = cameraFacingRotation();
            const float delta = std::remainder(target - start, glm::two_pi<float>());
            const float smoothT = t * t * (3.0f - 2.0f * t);
            player.rotation = start + delta * smoothT;
            if (seq.timer >= 0.45f) {
                player.rotation = target;
                PlayAudioAsset("belly_growl.mp3", "belly_growl", bellyGrowlDeviceOpen);
                seq.phase = ToiletSequencePhase::MassagingBelly;
                seq.timer = 0.0f;
            }
        }
        break;

    case ToiletSequencePhase::MassagingBelly:
        player.rotation = cameraFacingRotation();
        if (seq.timer >= 1.8f) {
            seq.phase = ToiletSequencePhase::TurningToDoor;
            seq.timer = 0.0f;
        }
        break;

    case ToiletSequencePhase::TurningToDoor:
        {
            const float t = std::clamp(seq.timer / 0.45f, 0.0f, 1.0f);
            const float start = cameraFacingRotation();
            const float target = glm::pi<float>();
            const float delta = std::remainder(target - start, glm::two_pi<float>());
            const float smoothT = t * t * (3.0f - 2.0f * t);
            player.rotation = start + delta * smoothT;
            if (seq.timer >= 0.45f) {
                player.rotation = target;
                seq.phase = ToiletSequencePhase::OpeningIn;
                seq.timer = 0.0f;
            }
        }
        break;

    case ToiletSequencePhase::OpeningIn:
        doorOpen = true;
        if (seq.timer >= 0.75f) {
            seq.phase = ToiletSequencePhase::WalkingIn;
            seq.timer = 0.0f;
            seq.from = glm::vec3(player.x, player.y, player.z);
            seq.to = glm::vec3(3.4f, 0.0f, -0.05f);
        }
        break;

    case ToiletSequencePhase::WalkingIn:
        {
            playerWalking = true;
            player.walkTime += dt;
            const glm::vec3 insidePoint(3.4f, 0.0f, -0.05f);
            float t = std::clamp(seq.timer / 1.15f, 0.0f, 1.0f);
            player.x = glm::mix(seq.from.x, insidePoint.x, t);
            player.z = glm::mix(seq.from.z, insidePoint.z, t);
            player.y = 0.0f;
            player.rotation = glm::pi<float>();
            if (t >= 1.0f) {
                seq.phase = ToiletSequencePhase::ClosingIn;
                seq.timer = 0.0f;
                player.x = insidePoint.x;
                player.z = insidePoint.z;
            }
        }
        break;

    case ToiletSequencePhase::ClosingIn:
        doorOpen = false;
        player.rotation = glm::pi<float>();
        if (seq.timer >= 0.75f) {
            seq.phase = ToiletSequencePhase::WaitingInside;
            seq.timer = 0.0f;
        }
        break;

    case ToiletSequencePhase::WaitingInside:
        player.rotation = glm::pi<float>();
        if (!seq.exitMusicStarted && seq.timer >= 1.0f) {
            seq.exitMusicStarted = true;
            StartExitSong();
        }
        if (seq.timer >= 2.0f) {
            seq.phase = ToiletSequencePhase::OpeningOut;
            seq.timer = 0.0f;
        }
        break;

    case ToiletSequencePhase::OpeningOut:
        doorOpen = true;
        player.rotation = glm::pi<float>();
        if (seq.timer >= 0.75f) {
            seq.phase = ToiletSequencePhase::TurningOut;
            seq.timer = 0.0f;
        }
        break;

    case ToiletSequencePhase::TurningOut:
        {
            const float t = std::clamp(seq.timer / 0.4f, 0.0f, 1.0f);
            const float start = glm::pi<float>();
            const float target = cameraFacingRotation();
            const float delta = std::remainder(target - start, glm::two_pi<float>());
            const float smoothT = t * t * (3.0f - 2.0f * t);
            player.rotation = start + delta * smoothT;
        }
        if (seq.timer >= 0.4f) {
            seq.phase = ToiletSequencePhase::WalkingOut;
            seq.timer = 0.0f;
            player.rotation = cameraFacingRotation();
            seq.from = glm::vec3(player.x, player.y, player.z);
            seq.to = glm::vec3(3.4f, 0.0f, 1.55f);
        }
        break;

    case ToiletSequencePhase::WalkingOut:
        {
            playerWalking = true;
            player.walkTime += dt;
            const glm::vec3 outsideWaitPoint(3.4f, 0.0f, 1.55f);
            float t = std::clamp(seq.timer / 1.8f, 0.0f, 1.0f);
            player.x = glm::mix(seq.from.x, outsideWaitPoint.x, t);
            player.z = glm::mix(seq.from.z, outsideWaitPoint.z, t);
            player.y = 0.0f;
            player.rotation = cameraFacingRotation();
            if (t >= 1.0f) {
                seq.phase = ToiletSequencePhase::FinalTurn;
                seq.timer = 0.0f;
                player.x = outsideWaitPoint.x;
                player.z = outsideWaitPoint.z;
            }
        }
        break;

    case ToiletSequencePhase::FinalTurn:
        player.rotation = cameraFacingRotation();
        if (seq.timer >= 0.35f) {
            seq.phase = ToiletSequencePhase::ClosingOut;
            seq.timer = 0.0f;
        }
        break;

    case ToiletSequencePhase::ClosingOut:
        doorOpen = false;
        player.rotation = cameraFacingRotation();
        if (seq.timer >= 0.75f) {
            PlayAudioAsset("shrek_victory.mp3", "shrek_victory", shrekVictoryDeviceOpen);
            seq.phase = ToiletSequencePhase::FinalPose;
            seq.timer = 0.0f;
            player.x = 3.4f;
            player.z = 1.55f;
            player.rotation = cameraFacingRotation();
        }
        break;

    case ToiletSequencePhase::FinalPose:
        player.rotation = cameraFacingRotation();
        if (seq.timer >= 1.2f) {
            seq.active = false;
            shrekHappyAfterToilet = true;
            seq.phase = ToiletSequencePhase::Idle;
            seq.timer = 0.0f;
        }
        break;

    case ToiletSequencePhase::Idle:
    default:
        seq.active = false;
        break;
    }
}

void drawWorld(float time) {
    // Grass-covered terrain slab
    box({0.0f,-0.32f,0.0f},{12.0f,0.60f,8.0f},{0.22f,0.35f,0.10f});
    box({0.0f,-0.015f,0.0f},{11.95f,0.08f,7.95f},{0.20f,0.46f,0.12f});
    // Keep grass clear of the outhouse and all decorative rocks.
    auto grassBlocked = [](float gx, float gz) {
        if (std::abs(gx) > 5.48f || std::abs(gz) > 3.22f) return true;
        if (gx > 2.3f && gx < 4.5f && gz > -1.7f && gz < 1.0f) return true;
        if (std::abs(gx + 4.2f) < 0.65f && std::abs(gz - 0.6f) < 0.55f) return true;
        if (std::abs(gx - 4.65f) < 0.58f && std::abs(gz + 2.35f) < 0.52f) return true;
        if (std::abs(gx - 1.4f) < 0.38f && std::abs(gz + 2.0f) < 0.34f) return true;
        return false;
    };

    // Existing grass pattern (fixed, no random flicker).
    for (int i=0;i<520;i++) {
        float baseX = -5.7f + std::fmod(i*1.731f,11.4f);
        float gz = -3.7f + std::fmod(i*2.317f,7.4f);
        float gx = baseX + std::sin(time*1.7f + i*0.31f)*0.035f;
        if (grassBlocked(gx, gz)) continue;
        float h = 0.08f + 0.10f * float((i*7)%11)/10.0f;
        float lean = float((i%5)-2)*0.035f;
        box({gx+lean,-0.005f+h*0.5f,gz},{0.035f,h,0.035f},
            (i%3==0)?glm::vec3(0.32f,0.58f,0.13f):glm::vec3(0.13f,0.37f,0.07f),
            float(i%7)*0.25f);
    }

    // Add a second staggered set between the original tufts, at a smaller scale.
    for (int row=0; row<20; ++row) {
        for (int column=0; column<26; ++column) {
            const int i = row*26 + column;
            const float baseX = -5.48f + column*0.438f + (row%2 ? 0.219f : 0.0f);
            const float gz = -3.52f + row*0.37f;
            const float gx = baseX + std::sin(time*1.5f + i*0.27f)*0.022f;
            if (grassBlocked(gx, gz)) continue;
            const float h = (0.08f + 0.10f * float((i*7)%11)/10.0f) * 0.72f;
            const float lean = float((i%5)-2)*0.025f;
            box({gx+lean,-0.005f+h*0.5f,gz},{0.026f,h,0.026f},
                (i%3==0)?glm::vec3(0.32f,0.58f,0.13f):glm::vec3(0.13f,0.37f,0.07f),
                float(i%7)*0.25f);
        }
    }
    // Slow, soft-edged clouds drift across the far background.
    const float cloudX = -6.5f + std::fmod(time*0.22f, 13.0f);
    const glm::vec3 cloudColor(0.70f, 0.79f, 0.82f);
    ball({cloudX,4.5f,-5.3f},{1.25f,0.34f,0.42f},cloudColor);
    ball({cloudX-0.68f,4.42f,-5.3f},{0.75f,0.30f,0.42f},cloudColor);
    ball({cloudX+0.60f,4.45f,-5.3f},{0.85f,0.28f,0.42f},cloudColor);
    const float secondCloudX = 5.2f - std::fmod(time*0.14f, 12.0f);
    ball({secondCloudX,5.0f,-5.8f},{1.0f,0.28f,0.36f},cloudColor);
    ball({secondCloudX+0.58f,4.95f,-5.8f},{0.72f,0.24f,0.36f},cloudColor);
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
    // Match the two horizontal rails already used along the back fence.
    box({-5.7f,0.78f,0.0f},{0.13f,0.12f,6.20f},{0.38f,0.23f,0.10f});
    box({-5.7f,0.20f,0.0f},{0.13f,0.13f,6.20f},{0.38f,0.23f,0.10f});
    box({ 5.7f,0.78f,0.0f},{0.13f,0.12f,6.20f},{0.38f,0.23f,0.10f});
    box({ 5.7f,0.20f,0.0f},{0.13f,0.13f,6.20f},{0.38f,0.23f,0.10f});
    // Decorative rocks
    ball({-4.2f,0.22f,0.6f},{1.0f,0.62f,0.78f},{0.31f,0.32f,0.29f});
    ball({-3.8f,0.16f,0.85f},{0.65f,0.42f,0.58f},{0.38f,0.37f,0.32f});
    ball({4.65f,0.20f,-2.35f},{0.85f,0.55f,0.72f},{0.34f,0.34f,0.31f});
    ball({1.4f,0.12f,-2.0f},{0.48f,0.30f,0.42f},{0.39f,0.38f,0.33f});
    if (!stone.inHand && !stone.flying) {
        ball(stone.position, {0.26f, 0.26f, 0.26f}, {0.52f, 0.52f, 0.52f});
    }
    drawOuthouse();
    drawShrek(time);
}

void MouseCallback(GLFWwindow* window, double xpos, double ypos) {
    static bool firstMouse = true;
    static double lastX = 0.0;
    static double lastY = 0.0;

    if (glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_LEFT) != GLFW_PRESS) {
        firstMouse = true;
        return;
    }

    if (firstMouse) {
        lastX = xpos;
        lastY = ypos;
        firstMouse = false;
        return;
    }

    float xOffset = static_cast<float>(xpos - lastX);
    float yOffset = static_cast<float>(lastY - ypos);
    lastX = xpos;
    lastY = ypos;

    camera.Yaw -= xOffset * 0.2f;
    camera.Pitch -= yOffset * 0.2f;
    camera.Pitch = glm::clamp(camera.Pitch, -89.0f, 89.0f);
    UpdateCameraOrbit();
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
    glfwSetCursorPosCallback(window, MouseCallback);
    glfwSetScrollCallback(window, ScrollCallback);
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

        bool nearDoor = std::abs(player.x - 2.55f) < 1.1f && std::abs(player.z - 0.75f) < 0.95f && player.y < 0.15f;
        bool gDown = glfwGetKey(window, GLFW_KEY_G) == GLFW_PRESS;
        if (gDown && !gWasDown && !toiletSequence.active) {
            StartToiletSequence();
        }
        gWasDown = gDown;
        playerWalking = false;

        if (toiletSequence.active) {
            UpdateToiletSequence(dt);
        } else {
            float moveX = 0.0f;
            float moveZ = 0.0f;

            if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS) {
                moveX -= 1.0f;
            }
            if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS) {
                moveX += 1.0f;
            }
            if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS) {
                moveZ -= 1.0f;
            }
            if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS) {
                moveZ += 1.0f;
            }

            if (glfwGetKey(window, GLFW_KEY_Q) == GLFW_PRESS) {
                player.rotation += 1.8f * dt;
            }
            if (glfwGetKey(window, GLFW_KEY_E) == GLFW_PRESS) {
                player.rotation -= 1.8f * dt;
            }

            bool rDown = glfwGetKey(window, GLFW_KEY_R) == GLFW_PRESS;
            if (rDown && !rWasDown) {
                armRaised = !armRaised;
            }
            rWasDown = rDown;

            if (glfwGetKey(window, GLFW_KEY_T) == GLFW_PRESS && !stone.flying && stone.inHand) {
                stone.inHand = false;
                stone.flying = true;
                stoneBounceSoundPlayed = false;
                float angle = player.rotation;
                const float handY = 0.58f + 0.84f * armLift - 0.10f;
                const float handZ = 0.32f + 0.58f * armLift;
                stone.position = glm::vec3(
                    player.x + std::cos(angle) * -0.38f + std::sin(angle) * handZ,
                    player.y + handY,
                    player.z - std::sin(angle) * -0.38f + std::cos(angle) * handZ);
                // A medium toss: farther than the short version, but still lower and shorter than before.
                stone.velocity = glm::vec3(std::sin(angle) * 3.3f, 4.1f, std::cos(angle) * 3.3f);
            }

            float armTarget = armRaised ? 1.0f : 0.0f;
            armLift += (armTarget - armLift) * std::min(1.0f, dt * 8.0f);
            armLift = std::clamp(armLift, 0.0f, 1.0f);

            if (moveX != 0.0f || moveZ != 0.0f) {
                playerWalking = true;
                float length = std::sqrt(moveX * moveX + moveZ * moveZ);
                moveX /= length;
                moveZ /= length;
                player.x += moveX * 3.0f * dt;
                player.z += moveZ * 3.0f * dt;
                player.direction = (moveX >= 0.0f) ? 1.0f : -1.0f;
                player.walkTime += dt;
            }

            player.x = std::clamp(player.x, -5.15f, 5.15f);
            player.z = std::clamp(player.z, -3.10f, 3.10f);

            bool jumpPressed = glfwGetKey(window, GLFW_KEY_SPACE) == GLFW_PRESS;
            if (jumpPressed && !upWasDown && player.onGround) {
                player.vy = 5.4f;
                player.onGround = false;
            }
            upWasDown = jumpPressed;
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
            bool nearStone = std::sqrt((player.x + 4.2f) * (player.x + 4.2f) + (player.z - 0.6f) * (player.z - 0.6f)) < 1.5f;

            if (fDown && !fWasDown) {
                if (nearDoor) {
                    doorOpen = !doorOpen;
                } else if (nearStone && !stone.inHand && !stone.flying) {
                    stone.inHand = true;
                    stone.position = glm::vec3(-4.2f, 0.22f, 0.6f);
                }
            }
            fWasDown = fDown;
        }

        float targetAngle = doorOpen ? glm::radians(-105.0f) : 0.0f;
        doorAngle += (targetAngle - doorAngle) * std::min(1.0f, dt * 5.0f);

        if (stone.flying) {
            stone.position += stone.velocity * dt;
            stone.velocity.y -= 7.5f * dt;
            if (stone.position.y <= 0.18f) {
                stone.position.y = 0.18f;
                if (!stoneBounceSoundPlayed && stone.velocity.y < -0.8f) {
                    stoneBounceSoundPlayed = true;
                    PlayAudioAsset("stone_bounce.mp3", "stone_bounce", stoneBounceDeviceOpen, 2000);
                    stone.velocity.y = -stone.velocity.y * 0.32f;
                    stone.velocity.x *= 0.55f;
                    stone.velocity.z *= 0.55f;
                } else if (stone.velocity.y <= 0.0f) {
                    stone.velocity = glm::vec3(0.0f);
                    stone.flying = false;
                }
            }
        }

        if (stone.inHand) {
            stone.position = glm::vec3(player.x, player.y + 1.1f, player.z);
        }

        cameraTarget = glm::vec3(player.x, 1.0f + player.y * 0.5f, player.z);
        UpdateCameraOrbit();

        int w, h;
        glfwGetFramebufferSize(window, &w, &h);
        glViewport(0, 0, w, h);
        glClearColor(0.20f, 0.31f, 0.38f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        view = camera.GetViewMatrix();
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
    StopExitSong();
    StopAudioAsset("belly_growl", bellyGrowlDeviceOpen);
    StopAudioAsset("shrek_sigh", shrekSighDeviceOpen);
    StopAudioAsset("stone_bounce", stoneBounceDeviceOpen);
    StopAudioAsset("shrek_victory", shrekVictoryDeviceOpen);
    glfwDestroyWindow(window);
    glfwTerminate();
    return 0;
}
