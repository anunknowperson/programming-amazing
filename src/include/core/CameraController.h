#pragma once

#include <SDL2/SDL.h>
#include <glm/glm.hpp>
#include <glm/gtc/quaternion.hpp>

#include "scene/Camera.h"

class CameraController {
public:
    explicit CameraController(Camera& camera);

    // Управление позицией и вращением
    void setPosition(const glm::vec3& position);
    glm::vec3 getPosition() const;

    void setRotation(const glm::quat& rotation);
    glm::quat getRotation() const;

    void lookAt(const glm::vec3& target);

    // Обработка событий ввода
    void processSDLEvent(const SDL_Event& event);

private:
    Camera& _camera;  // Ссылка на управляемую камеру
    float _pitch;     // Угол наклона (по вертикали)
    float _yaw;       // Угол поворота (по горизонтали)
};
