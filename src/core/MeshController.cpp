#include "core/MeshController.h"

#include <graphics/vulkan/vk_engine.h>
#include <ranges>

#include "scene/Mesh.h"

MeshController::MeshController(IModel::Ptr model) : _model(std::move(model)) {}

Mesh::rid_t MeshController::create_mesh(
        const std::filesystem::path& file_path) const {
    return _model->createMesh(file_path);
}

void MeshController::delete_mesh(Mesh::rid_t id) const {
    _model->delete_mesh(id);
}

void MeshController::set_transform(Mesh::rid_t id, glm::mat4 t) const {
    _model->setMeshTransform(id, t);
}

glm::mat4 MeshController::get_transform(Mesh::rid_t id) const {
    const auto transform = _model->get_mesh_transform(id);
    return transform;
}

std::vector<Mesh::rid_t> MeshController::get_meshes() const {
    const auto map = _model->get_meshes();
    std::vector<Mesh::rid_t> v;
    v.reserve(map.size());
    for (const auto rid : std::views::keys(map)) {
        v.push_back(rid);
    }
    return v;
}
