#pragma once

#include <SDL_events.h>
#include <memory>

#include "IController.h"
#include "interfaces/IModel.h"

class ControllerImpl : public IController,
                       public std::enable_shared_from_this<ControllerImpl> {
public:
    explicit ControllerImpl(IModel::Ptr model);

    void init() const override;
    void update() const override;
    void processEvent(SDL_Event& e) const override;

private:
    std::shared_ptr<IModel> _model;
};
