#pragma once

#include "IController.h"
#include "interfaces/IModel.h"
#include "interfaces/IView.h"

IController::Ptr createController(IModel::Ptr model, IView::Ptr view);
