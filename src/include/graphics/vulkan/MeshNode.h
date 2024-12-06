#pragma once

#include <glm/glm.hpp>

#include "graphics/vulkan/vk_types.h"
#include "scene/Mesh.h"

struct MeshNode final : ENode {
    std::shared_ptr<Mesh::GLTF::MeshAsset> mesh;

    void Draw(const glm::mat4& topMatrix, DrawContext& ctx) override;
};
