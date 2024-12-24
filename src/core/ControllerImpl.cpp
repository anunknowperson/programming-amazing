#include "core/ControllerImpl.h"

#include <chrono>
#include <cmath>
#include <string>
#include <utility>

#define GLM_ENABLE_EXPERIMENTAL
#include <glm/gtx/transform.hpp>

#include "core/View.h"
#include "scene/Camera.h"

ControllerImpl::ControllerImpl(IModel::Ptr model) : _model(std::move(model)) {}

double getCurrentGlobalTime() {
    // Get the current time point
    const auto now = std::chrono::system_clock::now();

    // Cast to a time duration since the epoch
    const auto durationSinceEpoch = now.time_since_epoch();

    // Convert to seconds in double precision
    const std::chrono::duration<double> seconds = durationSinceEpoch;

    // Return the double value
    return seconds.count();
}

void updateCube(const std::shared_ptr<IModel> &_model, int name) {
    const double sinValue = std::sin(getCurrentGlobalTime() + name) * 5.0;

    const glm::mat4 scale = glm::scale(glm::vec3{0.2f});
    const glm::mat4 translation = glm::translate(
            glm::vec3{static_cast<float>(name) - 2.5f, sinValue, 0});

    _model->setMeshTransform("cube" + std::to_string(name),
                             scale * translation);
}

void updateCubes(const std::shared_ptr<IModel> &_model) {
    for (int i = 0; i < 5; i++) {
        updateCube(_model, i);
    }
}

void ControllerImpl::update() const {
    _model->updateVulkan();

    _model->getCamera()->update();

    updateCubes(_model);
}

void ControllerImpl::processEvent(SDL_Event &e) const {
    _model->getCamera()->processSDLEvent(e);
}

void ControllerImpl::init() const {
    const auto controller = shared_from_this();
    const auto view = createView(controller, _model);
    view->run();
}
