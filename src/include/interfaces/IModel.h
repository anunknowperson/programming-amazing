#pragma once

#include <array>
#include <cstdint>
#include <memory>
#include <string_view>

#include "SDL2/SDL.h"
#include "SDL2/SDL_vulkan.h"
#include "scene/Camera.h"
#include "scene/Mesh.h"

/*!
 * \brief Interface for managing models and their integration with Vulkan.
 *
 * The IModel class provides an interface for handling models, registering a
 window for Vulkan rendering,
 * updating Vulkan-related states, creating meshes, setting mesh
 transformations, and accessing the camera.

 */
class IModel {
public:
    /*!
     * \brief Virtual destructor for the interface.
     *
     * Ensures proper cleanup of derived classes.
     */
    virtual ~IModel() = default;

    /*!
     * \brief Registers a window for Vulkan rendering.
     *
     * \param window Pointer to an SDL_Window.
     *
     * This method registers the given window for rendering using Vulkan.
     * It is essential to call this method before performing any Vulkan
     * rendering operations.
     */
    virtual void registerWindow(struct SDL_Window* window) = 0;

    /*!
     * \brief Loads (parses) a new mesh to the model
     * \param file_path The path to the mesh
     * \return Returns render-id of the mesh that helps identify the provided
     * mesh.
     */
    virtual Mesh::rid_t createMesh(const std::filesystem::path& file_path) = 0;
    /*!
     * \brief Deletes the provided mesh from a storage
     * @param rid Render-id of the mesh to be deleted
     */
    virtual void delete_mesh(Mesh::rid_t rid) = 0;

    /*!
     * \brief Sets the transformation matrix for a mesh identified by render-id
     * \param rid Render-id of the mesh to be transformed
     * \param transform Transformation matrix to be applied to the mesh.
     */
    virtual void setMeshTransform(Mesh::rid_t rid, glm::mat4x4 transform) = 0;
    /*!
     * \brief Gets the transformation matrix of a mesh.
     * \param rid Render-id of the mesh
     * \return Returns a transform matrix of the mesh
     */
    virtual glm::mat4 get_mesh_transform(Mesh::rid_t rid) = 0;

    /*!
     * \brief Stores mesh data
     */
    struct MeshPair {
        /*!
         * \brief Pointer to the loaded mesh (GLTF)
         */
        std::shared_ptr<const Mesh::GLTF::LoadedGLTF> ptr;
        /*!
         * \brief Transform matrix of the mesh
         */
        glm::mat4 transform;
    };

    using MeshMap = std::unordered_map<Mesh::rid_t, MeshPair>;
    /*!
     * \brief Gets all the stored meshes
     * \return Returns a MeshMap that stores pairs of the render-id of the mesh
     * and the mesh data
     */
    virtual const MeshMap& get_meshes() = 0;

    /*!
     * \brief Retrieves the camera instance.
     *
     * \return Pointer to the Camera instance.
     *
     * This method returns a pointer to the camera instance managed by the
     * model. Can be used to access and modify camera properties.
     */
    [[nodiscard]] virtual Camera* getCamera() = 0;

    /*!
     * \brief Gets the stored engine
     * \return Returns a ref to the engine
     */
    virtual VulkanEngine& get_engine() = 0;

    using Ptr = std::shared_ptr<IModel>;
};
