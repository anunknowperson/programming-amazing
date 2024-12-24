#include "scene/Camera.h"
#define GLM_ENABLE_EXPERIMENTAL
#include "glm/gtx/quaternion.hpp"
#include "glm/gtx/transform.hpp"

void Camera::update() {
    const glm::mat4 cameraRotation = getRotationMatrix();
    position += glm::vec3(cameraRotation * glm::vec4(velocity * 0.05f, 0.f));
}

void Camera::processSDLEvent(const SDL_Event& e) {
    if (e.type == SDL_KEYDOWN) {
        if (e.key.keysym.sym == SDLK_w) {
            velocity.z = -1;
        }
        if (e.key.keysym.sym == SDLK_s) {
            velocity.z = 1;
        }
        if (e.key.keysym.sym == SDLK_a) {
            velocity.x = -1;
        }
        if (e.key.keysym.sym == SDLK_d) {
            velocity.x = 1;
        }
    }

    if (e.type == SDL_KEYUP) {
        if (e.key.keysym.sym == SDLK_w) {
            velocity.z = 0;
        }
        if (e.key.keysym.sym == SDLK_s) {
            velocity.z = 0;
        }
        if (e.key.keysym.sym == SDLK_a) {
            velocity.x = 0;
        }
        if (e.key.keysym.sym == SDLK_d) {
            velocity.x = 0;
        }
    }

    if (e.type == SDL_MOUSEMOTION) {
        yaw += (float)e.motion.xrel / 200.f;
        pitch -= (float)e.motion.yrel / 200.f;
    }
}

glm::mat4 Camera::getViewMatrix() const {
    // to create a correct model view, we need to move the world in opposite
    // direction to the camera
    //  so we will create the camera model matrix and invert
    const glm::mat4 cameraTranslation =
            glm::translate(glm::mat4(1.f), position);
    const glm::mat4 cameraRotation = getRotationMatrix();
    return glm::inverse(cameraTranslation * cameraRotation);
}

glm::mat4 Camera::getRotationMatrix() const {
    // fairly typical FPS style camera. we join the pitch and yaw rotations into
    // the final rotation matrix

    const glm::quat pitchRotation =
            glm::angleAxis(pitch, glm::vec3{1.f, 0.f, 0.f});
    const glm::quat yawRotation =
            glm::angleAxis(yaw, glm::vec3{0.f, -1.f, 0.f});

    return glm::toMat4(yawRotation) * glm::toMat4(pitchRotation);
}