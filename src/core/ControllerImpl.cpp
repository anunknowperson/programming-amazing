#include "core/ControllerImpl.h"

#include <cassert>
#include <chrono>
#include <iostream>
#include <utility>

#include "core/CameraController.h"
#include "scene/Camera.h"

#define GLM_ENABLE_EXPERIMENTAL
#include <core/View.h>
#include <glm/gtx/transform.hpp>

ControllerImpl::ControllerImpl(IModel::Ptr model) : _model(std::move(model)) {}

double getCurrentGlobalTime() {
    // Get the current time point
    auto now = std::chrono::system_clock::now();

    // Cast to a time duration since the epoch
    auto durationSinceEpoch = now.time_since_epoch();

    // Convert to seconds in double precision
    std::chrono::duration<double> seconds = durationSinceEpoch;

    // Return the double value
    return seconds.count();
}

void updateCube(const std::shared_ptr<IModel> &_model, int name) {
    double sinValue = std::sin(getCurrentGlobalTime() + name) * 5.0;

    glm::mat4 scale = glm::scale(glm::vec3{0.2f});
    glm::mat4 translation = glm::translate(
            glm::vec3{static_cast<float>(name) - 2.5f, sinValue, 0});

    _model->setMeshTransform("cube" + std::to_string(name), scale * translation);
}

void updateCubes(const std::shared_ptr<IModel> &_model) {
    for (int i = 0; i < 5; i++) {
        updateCube(_model, i);
    }
}

void ControllerImpl::update() const {
    _model->updateVulkan();

    updateCubes(_model);
}

void ControllerImpl::processEvent(SDL_Event &e) const {
    _model->getCameraController()->processSDLEvent(e);
}

void ControllerImpl::init() const {
    const auto controller = shared_from_this();
    const auto view = createView(controller, _model);
    view->run();
}
