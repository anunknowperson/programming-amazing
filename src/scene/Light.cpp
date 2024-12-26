#include "scene/Light.h"

#include <glm/gtc/matrix_transform.hpp>

Light::Light()
    : _position(glm::vec3(0.0f, 0.0f, 0.0f)),
      _rotation(glm::quat(1.0f, 0.0f, 0.0f, 0.0f)),
      _radius(5.0f),
      _strength(1.0f),
      _color(glm::vec3(1.0f, 1.0f, 1.0f)) {}

glm::vec3 Light::getPosition() const {
    return _position;
}

void Light::setPosition(const glm::vec3& position) {
    _position = position;
}

glm::quat Light::getRotation() const {
    return _rotation;
}

void Light::setRotation(const glm::quat& rotation) {
    _rotation = rotation;
}

float Light::getRadius() const {
    return _radius;
}

void Light::setRadius(float radius) {
    _radius = radius;
}

float Light::getStrength() const {
    return _strength;
}

void Light::setStrength(float strength) {
    _strength = strength;
}

glm::vec3 Light::getColor() const {
    return _color;
}

void Light::setColor(glm::vec3 color) {
    _color = color;
}