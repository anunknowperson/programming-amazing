#pragma once

#include <glm/detail/type_mat4x4.hpp>
#include <string_view>

#include "interfaces/IModel.h"
#include "scene/Mesh.h"

/*! \brief
 * A class that allows you to add, delete, modify meshes
 */
class MeshController {
public:
    MeshController() = delete;
    explicit MeshController(IModel::Ptr model);
    ~MeshController() = default;
    /*! \brief Adds a mesh to the ones to be rendered
     * \param file_path The path to the mesh
     * \return Returns render-id of the added mesh
     */
    Mesh::rid_t create_mesh(const std::filesystem::path& file_path) const;
    /*! \brief Removes meshes from those to be rendered
     * \param id Render-id of the mesh
     */
    void delete_mesh(Mesh::rid_t id) const;

    /*! \brief Sets a transform matrix of a mesh
     * \param id Render-id of the mesh
     * \param t Transform matrix
     */
    void set_transform(Mesh::rid_t id, glm::mat4 t) const;
    /*! \brief Gets a transform matrix of a mesh
     * \param id Render-id of the mesh
     * \return Returns a transform matrix of the mesh
     */
    [[nodiscard]] glm::mat4 get_transform(Mesh::rid_t id) const;
    /*! \brief Gets all meshes render-ids
     * \return Returns a vector of all render-ids
     */
    [[nodiscard]] std::vector<Mesh::rid_t> get_meshes() const;

private:
    IModel::Ptr _model;
};