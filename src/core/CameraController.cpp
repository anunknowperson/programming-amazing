#include "core/CameraController.h"

#include "iostream"

#define GLM_ENABLE_EXPERIMENTAL
#include <glm/gtx/component_wise.hpp>
#include <glm/gtx/quaternion.hpp>

CameraController::CameraController(Camera& camera)
    : _camera(camera), _pitch(0.0f), _yaw(0.0f) {}

void CameraController::setPosition(const glm::vec3& position) {
    _camera.setPosition(position);
}

glm::vec3 CameraController::getPosition() const {
    return _camera.getPosition();
}

void CameraController::setRotation(const glm::quat& rotation) {
    _camera.setRotation(rotation);
}

glm::quat CameraController::getRotation() const {
    return _camera.getRotation();
}

void CameraController::lookAt(const glm::vec3& target) {
    glm::vec3 direction = glm::normalize(target - _camera.getPosition());
    glm::quat rotation =
            glm::quatLookAt(direction, glm::vec3(0.0f, 1.0f, 0.0f));
    setRotation(rotation);
}

void CameraController::processSDLEvent(const SDL_Event& event) {
    if (event.type == SDL_MOUSEMOTION) {
        float sensitivity = 0.0001f;  // Adjusted sensitivity
        _yaw -= event.motion.xrel * sensitivity;
        _pitch -= event.motion.yrel * sensitivity;

        // Clamp pitch to avoid gimbal lock
        if (_pitch > glm::radians(89.0f)) _pitch = glm::radians(89.0f);
        if (_pitch < glm::radians(-89.0f)) _pitch = glm::radians(-89.0f);

        // Convert yaw and pitch to quaternion
        glm::quat yawQuat = glm::angleAxis(_yaw, glm::vec3(0.0f, 1.0f, 0.0f));
        glm::quat pitchQuat =
                glm::angleAxis(_pitch, glm::vec3(1.0f, 0.0f, 0.0f));
        glm::quat rotation = yawQuat * pitchQuat;
        setRotation(rotation);
    }

    if (event.type == SDL_KEYDOWN) {
        float moveSpeed = 0.1f;
        glm::vec3 position = _camera.getPosition();

        switch (event.key.keysym.sym) {
            case SDLK_w:
                position.z -= moveSpeed;
                break;
            case SDLK_s:
                position.z += moveSpeed;
                break;
            case SDLK_a:
                position.x -= moveSpeed;
                break;
            case SDLK_d:
                position.x += moveSpeed;
                break;
        }

        _camera.setPosition(position);
    }
}