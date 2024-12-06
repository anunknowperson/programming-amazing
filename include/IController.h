#pragma once

#include <cstdint>
#include <expected>
#include <memory>
#include <string_view>

#include "SDL2/SDL.h"
#include "core/MeshController.h"

/*! \brief
 * Interface for interacting with the chip that allows changing keyboard
 * backlight settings.
 *
 * The IController class provides an interface for controlling the keyboard
 * backlight settings. It includes methods for initialization, updating the
 * controller status, processing events, changing the brightness level, and
 * changing the color of the backlight.
 */

class IController {
public:
    /*!
     * \brief Virtual destructor for the interface.
     */
    virtual ~IController() = default;
    /*!
     * \brief Renders a new frame
     */
    virtual void run() = 0;
    /*!
     * \brief Applies updates
     */
    virtual void update() = 0;
    /*!
     * \brief Gets a controller to interact with meshes
     * \return Pointer to the MeshController
     */
    [[nodiscard]] virtual std::weak_ptr<const MeshController>
    getMeshController() = 0;
    /*!
     * \brief Handles user actions (SDL events)
     * @param e SDL event
     */
    virtual void process_event(const SDL_Event& e) = 0;
    /*!
     * \brief Shared pointer type for IController.
     */
    using Ptr = std::shared_ptr<IController>;
};

IController::Ptr createInstance();
