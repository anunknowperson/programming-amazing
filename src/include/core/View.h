#pragma once

#include "IController.h"
#include "interfaces/IModel.h"
#include "interfaces/IView.h"

IView::Ptr createView(IModel::Ptr model);