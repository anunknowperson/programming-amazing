#include "IController.h"
#include "core/Controller.h"
#include "core/Model.h"
#include "core/View.h"
#include "interfaces/IModel.h"

IController::Ptr createInstance() {
    const auto model = createModel();
    const auto view = createView(model);
    const auto controller = createController(model, view);
    return controller;
}
