﻿#pragma once

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

inline void vk_check(const VkResult err) {
    if (err != VK_SUCCESS) {
        LOGE("Detected Vulkan error: {}", static_cast<int>(err));
    }
}

struct AllocatedImage {
    VkImage image;
    VkImageView image_view;
    VmaAllocation allocation;
    VkExtent3D image_extent;
    VkFormat image_format;
};

struct AllocatedBuffer {
    VkBuffer buffer;
    VmaAllocation allocation;
    VmaAllocationInfo info;
};

struct Vertex {
    glm::vec3 position;
    float uv_x;
    glm::vec3 normal;
    float uv_y;
    glm::vec4 color;
};

// holds the resources needed for a mesh
struct GPUMeshBuffers {
    AllocatedBuffer index_buffer;
    AllocatedBuffer vertex_buffer;
    VkDeviceAddress vertex_buffer_address;
};

// push constants for our mesh object draws
struct GPUDrawPushConstants {
    glm::mat4 world_matrix;
    VkDeviceAddress vertex_buffer;
};

enum class MaterialPass : uint8_t { MainColor, Transparent, Other };

struct MaterialPipeline {
    VkPipeline pipeline;
    VkPipelineLayout layout;
};

struct MaterialInstance {
    MaterialPipeline* pipeline;
    VkDescriptorSet material_set;
    MaterialPass passType;
};

struct DrawContext;

// base class for a renderable dynamic object
class IRenderable {
    virtual void draw(const glm::mat4& topMatrix, DrawContext& ctx) = 0;
};

// implementation of a drawable scene node.
// the scene node can hold children and will also keep a transform to propagate
// to them
struct ENode : public IRenderable {
    ENode() = default;

    virtual ~ENode() = default;
    // parent pointer must be a weak pointer to avoid circular dependencies
    std::weak_ptr<ENode> parent;
    std::vector<std::shared_ptr<ENode>> children;

    glm::mat4 local_transform;
    glm::mat4 world_transform;

    void refreshTransform(const glm::mat4& parentMatrix) {
        world_transform = parentMatrix * local_transform;
        for (auto c : children) {
            c->refreshTransform(world_transform);
        }
    }

    virtual void draw(const glm::mat4& topMatrix, DrawContext& ctx) {
        // draw children
        for (auto& c : children) {
            c->draw(topMatrix, ctx);
        }
    }
};