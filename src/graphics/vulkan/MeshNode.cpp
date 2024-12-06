#include "graphics/vulkan/MeshNode.h"

#include <glm/glm.hpp>

#include "graphics/vulkan/vk_engine.h"

void MeshNode::Draw(const glm::mat4& topMatrix, DrawContext& ctx) {
    const glm::mat4 nodeMatrix = topMatrix * worldTransform;

    for (const auto& [startIndex, count, material] : mesh->surfaces) {
        RenderObject def{};
        def.indexCount = count;
        def.firstIndex = startIndex;
        def.indexBuffer = mesh->meshBuffers.indexBuffer.buffer;
        def.material = &material->data;

        def.transform = nodeMatrix;
        def.vertexBufferAddress = mesh->meshBuffers.vertexBufferAddress;

        ctx.OpaqueSurfaces.push_back(def);
    }

    ENode::Draw(topMatrix, ctx);
}
