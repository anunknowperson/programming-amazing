#pragma once

#include <filesystem>

#include "scene/Mesh.h"
#include "vk_types.h"

class RenderableGLTF : public IRenderable {
public:
    using LoadedGltfPtr = std::shared_ptr<const Mesh::GLTF::LoadedGLTF>;
    explicit RenderableGLTF(LoadedGltfPtr gltf);

    void Draw(const glm::mat4& topMatrix, DrawContext& ctx) final;

private:
    std::shared_ptr<const Mesh::GLTF::LoadedGLTF> _gltf;
};
