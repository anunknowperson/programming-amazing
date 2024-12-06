#pragma once

/*! \brief
 *
 * Interface to show user their interaction abilities
 * to see and change backlight settings
 */
class IView {
public:
    virtual ~IView() = default;

    virtual void run() const = 0;
    virtual void process_event(const SDL_Event& e) = 0;

    using Ptr = std::shared_ptr<IView>;
};
