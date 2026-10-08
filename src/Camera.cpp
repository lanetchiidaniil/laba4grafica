#include "Camera.h"

Camera::Camera(glm::vec3 position, glm::vec3 up, float yaw, float pitch)
    : Position(position), WorldUp(up), Yaw(yaw), Pitch(pitch), MovementSpeed(3.5f), MouseSensitivity(0.15f) {
    UpdateCameraVectors();
}

glm::mat4 Camera::GetViewMatrix() const {
    return glm::lookAt(Position, Position + Front, Up);
}

void Camera::ProcessKeyboard(CameraDirection direction, float deltaTime) {
    float velocity = MovementSpeed * deltaTime;

    switch (direction) {
    case CameraDirection::FORWARD:
        Position += Front * velocity;
        break;
    case CameraDirection::BACKWARD:
        Position -= Front * velocity;
        break;
    case CameraDirection::LEFT:
        Position -= Right * velocity;
        break;
    case CameraDirection::RIGHT:
        Position += Right * velocity;
        break;
    case CameraDirection::UP:
        Position += WorldUp * velocity;
        break;
    case CameraDirection::DOWN:
        Position -= WorldUp * velocity;
        break;
    }
}

void Camera::ProcessMouseMovement(float xOffset, float yOffset, bool constrainPitch) {
    xOffset *= MouseSensitivity;
    yOffset *= MouseSensitivity;

    Yaw += xOffset;
    Pitch += yOffset;

    if (constrainPitch) {
        Pitch = glm::clamp(Pitch, -89.0f, 89.0f);
    }

    UpdateCameraVectors();
}

void Camera::UpdateCameraVectors() {
    glm::vec3 front;
    front.x = cos(glm::radians(Yaw)) * cos(glm::radians(Pitch));
    front.y = sin(glm::radians(Pitch));
    front.z = sin(glm::radians(Yaw)) * cos(glm::radians(Pitch));
    Front = glm::normalize(front);
    Right = glm::normalize(glm::cross(Front, WorldUp));
    Up = glm::normalize(glm::cross(Right, Front));
}
