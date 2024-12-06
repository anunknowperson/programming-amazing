#pragma once

#include "graphics/vulkan/vk_engine.h"
#include "interfaces/IModel.h"
#include "scene/Camera.h"
#include "scene/Mesh.h"

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

    Mesh::rid_t createMesh(const std::filesystem::path &file_path) final;
    void delete_mesh(Mesh::rid_t rid) final;

    void setMeshTransform(Mesh::rid_t rid, glm::mat4x4 transform) final;
    glm::mat4 get_mesh_transform(Mesh::rid_t) final;

    const MeshMap &get_meshes() final;

    Camera *getCamera() override;

    VulkanEngine &get_engine() final;

private:
    MeshMap _meshes;

    VulkanEngine _engine;

    Camera _camera;
};