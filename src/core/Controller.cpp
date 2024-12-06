#include "core/Controller.h"

#include "core/ControllerImpl.h"

IController::Ptr createController(IModel::Ptr model, IView::Ptr view) {
    return std::make_shared<ControllerImpl>(std::move(model), std::move(view));
}