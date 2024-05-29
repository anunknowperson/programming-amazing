#pragma once

#include <memory>

#include "interfaces/IController.h"
#include "interfaces/IModel.h"
#include "interfaces/IView.h"

class ViewImpl : public IView
{
public:
	ViewImpl(IController::Ptr controller, IModel::Ptr model);
    ~ViewImpl();
    /*
	void showCurrentSettings() const override;
	void runMenu() const override;*/

    void run() const override;

private:
	IController::Ptr _controller;
	IModel::Ptr _model;

    struct SDL_Window *window{nullptr};
};