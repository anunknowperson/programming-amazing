#include "core/ControllerImpl.h"

#include <cassert>
#include <chrono>
#include <core/View.h>
#include <graphics/vulkan/vk_engine.h>
#include <iostream>
#include <utility>

ControllerImpl::ControllerImpl(IModel::Ptr model, IView::Ptr view)
    : _model(std::move(model)), _view(std::move(view)) {}

void ControllerImpl::update() {
    _model->get_engine().update(_model);
    _model->getCamera()->update();
}

void ControllerImpl::run() {
    _view->run();
}

std::weak_ptr<const MeshController> ControllerImpl::getMeshController() {
    if (!_mesh_controller) {
        _mesh_controller = std::make_shared<MeshController>(_model);
    }
    return _mesh_controller;
}

void ControllerImpl::process_event(const SDL_Event& e) {
    _model->getCamera()->processSDLEvent(e);
    _view->process_event(e);
}
