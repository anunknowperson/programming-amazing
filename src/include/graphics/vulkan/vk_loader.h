#pragma once

#include <filesystem>
#include <unordered_map>

#include "vk_descriptors.h"
#include "vk_types.h"

struct GLTFMaterial {
    MaterialInstance data;
};

struct GeoSurface {
    uint32_t startIndex;
    uint32_t count;
    std::shared_ptr<GLTFMaterial> material;
};

struct MeshAsset {
    std::string name;

    std::vector<GeoSurface> surfaces;
    GPUMeshBuffers meshBuffers;
};

// forward declaration
class VulkanEngine;

std::optional<std::vector<std::shared_ptr<MeshAsset>>> loadGltfMeshes(
        VulkanEngine* engine, std::filesystem::path filePath);

struct LoadedGLTF final : public IRenderable {
    LoadedGLTF() = default;

    // storage for all the data on a given glTF file
    std::unordered_map<std::string, std::shared_ptr<MeshAsset>> meshes;
    std::unordered_map<std::string, std::shared_ptr<ENode>> nodes;
    std::unordered_map<std::string, AllocatedImage> images;
    std::unordered_map<std::string, std::shared_ptr<GLTFMaterial>> materials;

    // nodes that dont have a parent, for iterating through the file in tree
    // order
    std::vector<std::shared_ptr<ENode>> topNodes;

    std::vector<VkSampler> samplers;

    DescriptorAllocatorGrowable descriptorPool;

    AllocatedBuffer materialDataBuffer;

    VulkanEngine* creator;

    ~LoadedGLTF() {
        clearAll();
    };

    void Draw(const glm::mat4& topMatrix, DrawContext& ctx);

private:
    void clearAll();
};

std::optional<std::shared_ptr<LoadedGLTF>> loadGltf(
        VulkanEngine* engine, const std::filesystem::path& filePath);
