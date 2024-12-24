#pragma once

#include <filesystem>

#include "glm/detail/type_mat4x4.hpp"
#include "graphics/vulkan/vk_engine.h"

class Mesh {
public:
    Mesh(const std::string& filePath);
    ~Mesh();

    void set_transform(glm::mat4 t);
    glm::mat4 get_transform();

private:
    glm::mat4 _transform;

    int64_t _rid;
};