#pragma once

#include <array>
#include <cstdint>
#include <memory>
#include <string>
#include <string_view>

#include "SDL2/SDL.h"
#include "SDL2/SDL_vulkan.h"
#include "core/CameraController.h"
#include "scene/Camera.h"

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
     * \brief Updates Vulkan-related states.
     *
     * This method updates internal Vulkan-related states or data structures.
     * Should be called regularly to keep Vulkan rendering in sync with the
     * application state.
     */
    virtual void updateVulkan() = 0;

    /*!
     * \brief Creates a new mesh with the given name.
     *
     * \param name Name of the mesh to be created.
     *
     * This method creates a new mesh identified by the provided name.
     */
    virtual void createMesh(std::string name) = 0;

    /*!
     * \brief Sets the transformation matrix for a mesh.
     *
     * \param name Name of the mesh.
     * \param transform Transformation matrix to be applied to the mesh.
     *
     * This method sets the transformation matrix for the mesh identified by the
     * provided name.
     */
    virtual void setMeshTransform(std::string name, glm::mat4x4 transform) = 0;

    /*!
     * \brief Retrieves the camera instance.
     *
     * \return Pointer to the Camera instance.
     *
     * This method returns a pointer to the camera instance managed by the
     * model. Can be used to access and modify camera properties.
     */
    virtual Camera* getCamera() = 0;
    virtual CameraController* getCameraController() = 0;

    /*! \brief
     * Gets the chip handler from HIDAPI required to change the settings by the
     * Controller
     * @return
     * Returns the device handler in case, can't be nullptr
     */
    //[[nodiscard]] virtual hid_device* getChipHandler() const = 0;

    /*! \brief
     * Requests from chip the level of brightness
     * @return
     * Returns the level of brightness [0; 100]
     */
    // virtual uint8_t getBrightness() = 0;

    /*! \brief
     * Requests RGB values from the chip
     * @return
     * Returns the struct with .R, .G, .B fields
     */
    // virtual Color getRGB() = 0;

    using Ptr = std::shared_ptr<IModel>;
};
