#pragma once

#include <glm/glm.hpp>
#include <glm/gtc/quaternion.hpp>

class Light {
public:
    Light();

    glm::vec3 getPosition() const;
    void setPosition(const glm::vec3& position);

    glm::quat getRotation() const;
    void setRotation(const glm::quat& rotation);

    float getRadius() const;
    void setRadius(float radius);

    float getStrength() const;
    void setStrength(float strength);

    glm::vec3 getColor() const;
    void setColor(glm::vec3 color);

private:
    glm::vec3 _position;
    glm::quat _rotation;
    float _radius;
    float _strength;
    glm::vec3 _color;
};