#include "graphics/vulkan/RenderableGLTF.h"

#include <iostream>

#include "graphics/vulkan/vk_engine.h"
#include "graphics/vulkan/vk_types.h"
#include "stb_image.h"

#define GLM_ENABLE_EXPERIMENTAL

#include <fastgltf/core.hpp>
#include <fastgltf/glm_element_traits.hpp>
#include <fastgltf/tools.hpp>
#include <fastgltf/types.hpp>
#include <glm/gtx/quaternion.hpp>

RenderableGLTF::RenderableGLTF(LoadedGltfPtr gltf) : _gltf(std::move(gltf)) {}

void RenderableGLTF::Draw(const glm::mat4& topMatrix, DrawContext& ctx) {
    // create renderables from the scenenodes
    for (auto& n : _gltf->topNodes) {
        n->Draw(topMatrix, ctx);
    }
}
