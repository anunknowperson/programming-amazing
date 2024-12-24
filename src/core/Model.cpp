#include <memory>

#include "core/ModelImpl.h"

IModel::Ptr createModel() {
    return std::make_shared<ModelImpl>();
}
