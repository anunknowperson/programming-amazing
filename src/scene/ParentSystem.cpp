#include "scene/ParentSystem.h"

namespace {
void updateParent(flecs::entity e, const Parent &p) {
    if (!p.parent.is_alive() || !p.parent.has<Child>()) {
        e.destruct();
    } else {
        if (p.parent.has<Child>()) {
            auto *child = p.parent.get_mut<Child>();
            child->children.push_back(e);
        }
    }
}

void updateChild(Child &c) {
    auto &children = c.children;
    const auto newEnd =
            std::ranges::remove_if(children, [](const flecs::entity &child) {
                return (!child.is_alive() || !child.has<Parent>());
            }).begin();
    children.erase(newEnd, children.end());
}

void removeChild(flecs::entity e, const Child &c) {
    if (c.children.empty()) {
        e.remove<Child>();
    }
}

void changeParent(flecs::entity e, Parent &p, const PreviousParent &pp) {
    if (pp.parent.is_alive() and pp.parent.has<Child>()) {
        auto *child = pp.parent.get_mut<Child>();
        const auto newEnd =
                std::ranges::remove_if(child->children,
                                       [e](const flecs::entity &e_child) {
                                           return e_child == e;
                                       })
                        .begin();
        child->children.erase(newEnd, child->children.end());
    }
    if (p.parent.is_alive()) {
        auto *child = p.parent.get_mut<Child>();
        child->children.push_back(e);
    }
    p.parent = pp.parent;
    e.remove<PreviousParent>();
}
}  // namespace

void setRelation(flecs::entity child, flecs::entity parent) {
#ifndef NDEBUG
    if (!parent.is_alive()) {
        LOGW("Trying to set entity %s as parent, but it is not alive",
             parent.name().c_str());

        return;
    }
    if (!child.is_alive()) {
        LOGWF("Trying to set entity {} as child, but it is not alive",
              child.name().c_str());

        return;
    }
    if (child == parent) {
        LOGWF("Trying to set entity {} as its own parent",
              child.name().c_str());

        return;
    }
#endif
    if (child.has<Parent>()) {
        if (child.get<Parent>()->parent == parent) {
            return;
        } else {
            child.set<PreviousParent>({child.get<Parent>()->parent});
        }
    }
    if (parent.has<Child>()) {
        auto &children = parent.get_mut<Child>()->children;
        if (std::ranges::find(children, child) != children.end()) {
            return;
        }
        children.push_back(child);
    }
    child.set<Parent>({parent});
}

void removeRelation(flecs::entity child) {
#ifndef NDEBUG
    if (!child.is_alive() || !child.has<Parent>()) {
        LOGWF("Trying to remove parent from entity {}, but it is not alive or "
              "has no parent",
              child.name().c_str());
        return;
    }
#endif
    child.remove<Parent>();
}

void removeRelation(flecs::entity removing_child, flecs::entity parent) {
#ifndef NDEBUG
    if (!parent.is_alive() || !parent.has<Child>() ||
        !parent.get<Child>()->children.empty()) {
        LOGWF("Trying to remove entity {} from parent {}, but parent is not "
              "alive or has no children",
              removing_child.name().c_str(), parent.name().c_str());
        return;
    }
    if (!removing_child.is_alive() || !removing_child.has<Parent>()) {
        LOGWF("Trying to remove entity {} from parent {}, but child is not "
              "alive or has no parent",
              removing_child.name().c_str(), parent.name().c_str());
        return;
    }
#endif
    auto &children = parent.get_mut<Child>()->children;
    const auto newEnd = std::ranges::remove_if(
                                children,
                                [removing_child](const flecs::entity &child) {
                                    return child == removing_child;
                                })
                                .begin();
    children.erase(newEnd, children.end());
}

void ParentSystem(const flecs::world &world) {
    world.system<Parent>("UpdateParent").kind(flecs::OnAdd).each(updateParent);

    world.system<Child>("UpdateChild").kind(flecs::OnSet).each(updateChild);

    world.system<Child>("RemoveChild").kind(flecs::OnRemove).each(removeChild);

    world.system<Parent, PreviousParent>("ChangeParent")
            .kind(flecs::OnAdd)
            .each(changeParent);
}