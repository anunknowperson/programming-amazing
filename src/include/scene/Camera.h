#pragma once

#include <glm/glm.hpp>
#include <glm/gtc/quaternion.hpp>
#include <glm/mat4x4.hpp>

class Camera {
public:
    Camera();
    Camera(const glm::vec3& position, float fov, float screenWidth,
           float screenHeight);

    // Методы доступа
    glm::vec3 getPosition() const;
    void setPosition(const glm::vec3& position);

    glm::quat getRotation() const;
    void setRotation(const glm::quat& rotation);

    float getFOV() const;
    void setFOV(float fov);

    float getScreenWidth() const;
    void setScreenWidth(float screenWidth);

    float getScreenHeight() const;
    void setScreenHeight(float screenHeight);

    glm::mat4 getViewMatrix() const;

private:
    void updateViewMatrix();  // Обновление матрицы вида

    glm::vec3 _position;
    glm::quat _rotation;
    float _fov;
    float _screenWidth;
    float _screenHeight;
    glm::mat4 _viewMatrix;
};
