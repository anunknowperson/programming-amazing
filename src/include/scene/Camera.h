#pragma once

#include <SDL2/SDL_events.h>
#include <glm/detail/qualifier.hpp>
#include <glm/ext/matrix_float4x4.hpp>
#include <glm/ext/vector_float3.hpp>

class Camera {
public:
    glm::vec3 velocity;
    glm::vec3 position;
    // vertical rotation
    float pitch{0.f};
    // horizontal rotation
    float yaw{0.f};

    [[nodiscard]] glm::mat4 getViewMatrix() const;
    [[nodiscard]] glm::mat4 getRotationMatrix() const;

    void processSDLEvent(const SDL_Event& e);

    void update();
};