#pragma once

#include "SDL2/SDL_events.h"
#include "graphics/vulkan/vk_types.h"

class Camera {
public:
    glm::vec3 velocity;
    glm::vec3 position;
    // vertical rotation
    float pitch{0.f};
    // horizontal rotation
    float yaw{0.f};

    glm::mat4 getViewMatrix();
    glm::mat4 getRotationMatrix();

    void processSDLEvent(const SDL_Event& e);

    void update();
};