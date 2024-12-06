#pragma once

#include <array>
#include <deque>
#include <functional>
#include <memory>
#include <optional>
#include <span>
#include <string>
#include <vector>

#include "core/Logging.h"
#include "glm/mat4x4.hpp"
#include "glm/vec4.hpp"
#include "vk_mem_alloc.h"
#include "vulkan/vulkan.h"

#define VK_CHECK(x)                                      \
    do {                                                 \
        VkResult err = x;                                \
        if (err) {                                       \
            LOGE("Detected Vulkan error: {}", (int)err); \
        }                                                \
    } while (0)

struct AllocatedImage {
    VkImage image;
    VkImageView imageView;
    VmaAllocation allocation;
    VkExtent3D imageExtent;
    VkFormat imageFormat;
};

struct AllocatedBuffer {
    VkBuffer buffer;
    VmaAllocation allocation;
    VmaAllocationInfo info;
};

struct Vertex {
    glm::vec3 position;
    glm::vec3 normal;
    glm::vec4 color;
    float uv_x;
    float uv_y;
};

// holds the resources needed for a mesh
struct GPUMeshBuffers {
    AllocatedBuffer indexBuffer;
    AllocatedBuffer vertexBuffer;
    VkDeviceAddress vertexBufferAddress;
};

// push constants for our mesh object draws
struct GPUDrawPushConstants {
    glm::mat4 worldMatrix;
    VkDeviceAddress vertexBuffer;
};

enum class MaterialPass : uint8_t { MainColor, Transparent, Other };

struct MaterialPipeline {
    VkPipeline pipeline;
    VkPipelineLayout layout;
};

struct MaterialInstance {
    MaterialPipeline* pipeline;
    VkDescriptorSet materialSet;
    MaterialPass passType;
};

struct DrawContext;

// base class for a renderable dynamic object
struct IRenderable {
    virtual ~IRenderable() = default;
    virtual void Draw(const glm::mat4& topMatrix, DrawContext& ctx) = 0;

    using Ptr = std::shared_ptr<IRenderable>;
};

// implementation of a drawable scene node.
// the scene node can hold children and will also keep a transform to propagate
// to them
struct ENode : public IRenderable {
    // parent pointer must be a weak pointer to avoid circular dependencies
    std::weak_ptr<ENode> parent;
    std::vector<std::shared_ptr<ENode>> children;

    glm::mat4 localTransform;
    glm::mat4 worldTransform;

    void refreshTransform(const glm::mat4& parentMatrix) {
        worldTransform = parentMatrix * localTransform;
        for (auto c : children) {
            c->refreshTransform(worldTransform);
        }
    }

    void Draw(const glm::mat4& topMatrix, DrawContext& ctx) override {
        // draw children
        for (auto& c : children) {
            c->Draw(topMatrix, ctx);
        }
    }
};
