#pragma once

#include <glm/ext/matrix_float4x4.hpp>
#include <memory>
#include <string>
#include <unordered_map>

#include "graphics/vulkan/vk_engine.h"
#include "interfaces/IModel.h"
#include "scene/Camera.h"

class Mesh;

class ModelImpl : public IModel {
public:
    /*! \brief
     * throws std::runtime_error()
     */
    ModelImpl();
    ~ModelImpl() override;

    ModelImpl(const ModelImpl &) = delete;
    ModelImpl &operator=(const ModelImpl &) = delete;

    void registerWindow(struct SDL_Window *window) override;
    void updateVulkan() override;

    void createMesh(std::string name) override;
    void setMeshTransform(std::string name, glm::mat4x4 transform) override;

    Camera *getCamera() override;

private:
    std::unordered_map<std::string, std::shared_ptr<Mesh>> _meshes;

    VulkanEngine _engine;

    Camera _camera;
};