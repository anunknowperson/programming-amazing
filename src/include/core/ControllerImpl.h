#pragma once

#include "IController.h"
#include "interfaces/IModel.h"
#include "interfaces/IView.h"

class ControllerImpl : public IController {
public:
    explicit ControllerImpl(IModel::Ptr model, IView::Ptr view);

    void run() final;
    void update() final;
    std::weak_ptr<const MeshController> getMeshController() final;
    void process_event(const SDL_Event& e) final;

private:
    IModel::Ptr _model;
    IView::Ptr _view;
    std::shared_ptr<MeshController> _mesh_controller;
};
