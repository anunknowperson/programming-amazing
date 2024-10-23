#pragma once

#include <glm/glm.hpp>
#include <glm/gtc/quaternion.hpp>
#include <glm/mat4x4.hpp>

// Класс Camera отвечает только за хранение данных о камере
// Имплементация содержится в CameraController
class Camera {
public:
    // Конструктор по умолчанию.
    // Устанавливает позицию камеры в начало координат, угол обзора 45 градусов,
    // а также разрешение экрана 800x600

    Camera();
    // Конструктор с параметрами.
    // Позволяет задать положение камеры, угол обзора (в градусах) и размеры
    // экрана.
    Camera(const glm::vec3& position, float fov, float screenWidth,
           float screenHeight);

    glm::mat4 getViewMatrix() const;  // Возвращает матрицу вида

private:
    // Поля, которые контроллер будет использовать для управления камерой
    glm::vec3 _position;
    glm::quat _rotation;
    float _fov;            // Угол обзора в градусах
    float _screenWidth;    // Ширина экрана
    float _screenHeight;   // Высота экрана
    glm::vec3 _direction;  // Направление взгляда камеры
    glm::mat4 _viewMatrix;  // Матрица вида, используемая для преобразований

    void updateViewMatrix();  // Обновляет матрицу вида

    friend class CameraController;  // Чтобы контроллер имел доступ к приватным
                                    // полям
};