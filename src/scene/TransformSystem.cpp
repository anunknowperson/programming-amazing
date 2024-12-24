#include "scene/TransformSystem.h"

#include "glm/gtx/matrix_decompose.hpp"
#include "scene/ParentSystem.h"

namespace {
void UpdateChildrenGlobal(flecs::entity e, const GlobalTransform &t) {
    if (e.has<Child>()) {
        auto c = e.get<Child>();
        for (auto &child : c->children) {
            auto *child_transform = child.get<LocalTransform>();
            auto *child_global_transform = child.get_mut<GlobalTransform>();
            child_global_transform->TransformMatrix =
                    t.TransformMatrix *
                    glm::translate(glm::f64mat4(1.0),
                                   child_transform->position) *
                    glm::mat4_cast(child_transform->rotation) *
                    glm::scale(glm::f64mat4(1.0),
                               glm::f64vec3(child_transform->scale));
        }
    }
}

void CreateChildLocalIfParentSet(flecs::entity e, const Parent &p) {
    if (e.has<GlobalTransform>()) {
        const auto t = e.get<GlobalTransform>();
        const auto local =
                t->TransformMatrix *
                glm::inverse(p.parent.get<GlobalTransform>()->TransformMatrix);
        glm::f64vec3 position;
        glm::f64quat rotation;
        glm::f64vec3 scale;
        glm::f64vec3 skew;
        glm::f64vec4 perspective;
        glm::decompose(local, scale, rotation, position, skew, perspective);
        e.set(LocalTransform{position, rotation, scale.x});
    }
}

void UpdateChildLocalIfParentChanged(flecs::entity e, const Parent &p) {
    const auto t = e.get<GlobalTransform>();
    const auto local =
            t->TransformMatrix *
            glm::inverse(p.parent.get<GlobalTransform>()->TransformMatrix);
    glm::f64vec3 position;
    glm::f64quat rotation;
    glm::f64vec3 scale;
    glm::f64vec3 skew;
    glm::f64vec4 perspective;
    glm::decompose(local, scale, rotation, position, skew, perspective);
    auto *transform = e.get_mut<LocalTransform>();
    transform->position = position;
    transform->rotation = rotation;
    transform->scale = scale.x;
}

void UpdateChildLocalIfGlobalChanged(flecs::entity e,
                                     const GlobalTransform &t) {
    if (e.has<Parent>()) {
        const auto p = e.get<Parent>();
        const auto local =
                t.TransformMatrix *
                glm::inverse(p->parent.get<GlobalTransform>()->TransformMatrix);
        glm::f64vec3 position;
        glm::f64quat rotation;
        glm::f64vec3 scale;
        glm::f64vec3 skew;
        glm::f64vec4 perspective;
        glm::decompose(local, scale, rotation, position, skew, perspective);
        auto *transform = e.get_mut<LocalTransform>();
        transform->position = position;
        transform->rotation = rotation;
        transform->scale = scale.x;
    }
}

void UpdateChildGlobalIfLocalChanged(flecs::entity e, const LocalTransform &t) {
    [[maybe_unused]] auto global = e.get_mut<GlobalTransform>()
                                           ->TransformMatrix =
            getMatrixFromLocal(t) *
            e.get<Parent>()->parent.get<GlobalTransform>()->TransformMatrix;
}
}  // namespace

void setLocalFromMatrix(flecs::entity e, const glm::mat4 &mat) {
    const glm::vec3 position = mat[3];
    const auto scale = sqrt(static_cast<double>(glm::length(mat[0])));
#ifndef NDEBUG
    glm::float64 epsilon = 0.0001;
    const auto scale_y = static_cast<double>(glm::length(mat[1]));
    const auto scale_z = static_cast<double>(glm::length(mat[2]));
    if (abs(scale - scale_y) > epsilon || abs(scale - scale_z) > epsilon ||
        abs(scale_y - scale_z) > epsilon) {
        LOGW("Trying to convert a float4x4 to a LocalTransform, but the scale "
             "is not uniform")
    }
#endif

    auto pos_matrix = glm::mat3(mat);

#ifndef NDEBUG
    const auto dot_product1 =
            static_cast<double>(glm::dot(pos_matrix[0], pos_matrix[1]));
    const auto dot_product2 =
            static_cast<double>(glm::dot(pos_matrix[0], pos_matrix[2]));
    const auto dot_product3 =
            static_cast<double>(glm::dot(pos_matrix[1], pos_matrix[2]));
    if (abs(dot_product1) > epsilon || abs(dot_product2) > epsilon ||
        abs(dot_product3) > epsilon) {
        LOGW("Trying to convert a float4x4 to a LocalTransform, but the "
             "rotation is not orthogonal");
    }
#endif
    pos_matrix = glm::orthonormalize(pos_matrix);
    const glm::quat rotation = glm::quat_cast(pos_matrix);
    e.set<LocalTransform>({position, rotation, scale});
}

void setLocalFromPosition(flecs::entity e, const glm::vec3 &pos) {
    e.set<LocalTransform>({pos, glm::quat(1, 0, 0, 0), 1});
}

void setLocalFromRotation(flecs::entity e, const glm::quat &rot) {
    e.set<LocalTransform>({glm::vec3(0, 0, 0), rot, 1});
}

void setLocalFromScale(flecs::entity e, const glm::float64 &scale) {
    e.set<LocalTransform>({glm::vec3(0, 0, 0), glm::quat(1, 0, 0, 0), scale});
}

glm::f64mat4 getMatrixFromLocal(flecs::entity e) {
#ifndef NDEBUG
    if (!e.has<LocalTransform>()) {
        LOGW("Trying to convert a LocalTransform to a float4x4, but the entity "
             "does not have a LocalTransform");
        return glm::f64mat4(1.0);
    }
#endif
    const auto *transform = e.get_mut<LocalTransform>();
    return glm::translate(glm::f64mat4(1.0), transform->position) *
           glm::mat4_cast(transform->rotation) *
           glm::scale(glm::f64mat4(1.0), glm::f64vec3(transform->scale));
}

glm::f64mat4 getMatrixFromLocal(const LocalTransform &t) {
    return glm::translate(glm::f64mat4(1.0), t.position) *
           glm::mat4_cast(t.rotation) *
           glm::scale(glm::f64mat4(1.0), glm::f64vec3(t.scale));
}

void localRotate(flecs::entity e, const glm::f64quat &rot) {
#ifndef NDEBUG
    if (!e.has<LocalTransform>()) {
        LOGW("Trying to rotate an entity, but it does not have a "
             "LocalTransform");
        return;
    }
#endif
    auto *transform = e.get_mut<LocalTransform>();
    transform->rotation = rot * transform->rotation;
}

void localRotateX(flecs::entity e, const glm::float64 &angle) {
#ifndef NDEBUG
    if (!e.has<LocalTransform>()) {
        LOGW("Trying to rotate an entity, but it does not have a "
             "LocalTransform");
        return;
    }
#endif
    auto *transform = e.get_mut<LocalTransform>();
    transform->rotation =
            transform->rotation * glm::f64quat(angle, glm::f64vec3(1, 0, 0));
}

void localRotateY(flecs::entity e, const glm::float64 &angle) {
#ifndef NDEBUG
    if (!e.has<LocalTransform>()) {
        LOGW("Trying to rotate an entity, but it does not have a "
             "LocalTransform");
        return;
    }
#endif
    auto *transform = e.get_mut<LocalTransform>();
    transform->rotation =
            transform->rotation * glm::f64quat(angle, glm::f64vec3(0, 1, 0));
}

void localRotateZ(flecs::entity e, const glm::float64 &angle) {
#ifndef NDEBUG
    if (!e.has<LocalTransform>()) {
        LOGW("Trying to rotate an entity, but it does not have a "
             "LocalTransform");
        return;
    }
#endif
    auto *transform = e.get_mut<LocalTransform>();
    transform->rotation =
            transform->rotation * glm::f64quat(angle, glm::f64vec3(0, 0, 1));
}

void localTranslate(flecs::entity e, const glm::vec3 &pos) {
#ifndef NDEBUG
    if (!e.has<LocalTransform>()) {
        LOGW("Trying to translate an entity, but it does not have a "
             "LocalTransform");
        return;
    }
#endif
    auto *transform = e.get_mut<LocalTransform>();
    transform->position += pos;
}

void localTranslateX(flecs::entity e, const glm::float64 &distance) {
#ifndef NDEBUG
    if (!e.has<LocalTransform>()) {
        LOGW("Trying to translate an entity, but it does not have a "
             "LocalTransform");
        return;
    }
#endif
    auto *transform = e.get_mut<LocalTransform>();
    transform->position.x += distance;
}

void localTranslateY(flecs::entity e, const glm::float64 &distance) {
#ifndef NDEBUG
    if (!e.has<LocalTransform>()) {
        LOGW("Trying to translate an entity, but it does not have a "
             "LocalTransform");
        return;
    }
#endif
    auto *transform = e.get_mut<LocalTransform>();
    transform->position.y += distance;
}

void localTranslateZ(flecs::entity e, const glm::float64 &distance) {
#ifndef NDEBUG
    if (!e.has<LocalTransform>()) {
        LOGW("Trying to translate an entity, but it does not have a "
             "LocalTransform");
        return;
    }
#endif
    auto *transform = e.get_mut<LocalTransform>();
    transform->position.z += distance;
}

void localSetScale(flecs::entity e, const glm::float64 &scale) {
#ifndef NDEBUG
    if (!e.has<LocalTransform>()) {
        LOGW("Trying to set the scale of an entity, but it does not have a "
             "LocalTransform");
        return;
    }
#endif
    auto *transform = e.get_mut<LocalTransform>();
    transform->scale = scale;
}

void setLocalFromEntity(flecs::entity e, const flecs::entity &parent) {
#ifndef NDEBUG
    if (!e.has<LocalTransform>()) {
        LOGW("Trying to set the LocalTransform of an entity from another "
             "entity, but one of them does not have a LocalTransform");
        return;
    }
#endif
    auto *transform = e.get_mut<LocalTransform>();
    const auto *parent_transform = parent.get_mut<LocalTransform>();
    transform->position = parent_transform->position;
    transform->rotation = parent_transform->rotation;
    transform->scale = parent_transform->scale;
}

void inverseLocal(flecs::entity e) {
#ifndef NDEBUG
    if (!e.has<LocalTransform>()) {
        LOGW("Trying to inverse transform an entity, but it does not have a "
             "LocalTransform");
        return;
    }
#endif
    auto *transform = e.get_mut<LocalTransform>();
    transform->rotation = glm::inverse(transform->rotation);
    transform->position = -transform->position;
}

void setGlobalFromPosition(flecs::entity e, const glm::f64vec3 &pos) {
    e.set(GlobalTransform{glm::translate(glm::f64mat4(1.0), pos)});
}

void setGlobalFromRotation(flecs::entity e, const glm::f64quat &rot) {
    e.set(GlobalTransform{glm::mat4_cast(rot)});
}

void setGlobalFromScale(flecs::entity e, const glm::float64 &scale) {
    e.set(GlobalTransform{glm::scale(glm::f64mat4(1.0), glm::f64vec3(scale))});
}

void globalRotate(flecs::entity e, const glm::f64quat &rot) {
    e.set(GlobalTransform{glm::f64mat4(1.0) * glm::mat4_cast(rot)});
}

void globalRotateX(flecs::entity e, const glm::float64 &angle) {
    e.set(GlobalTransform{
            glm::f64mat4(1.0) *
            glm::mat4_cast(glm::f64quat(angle, glm::f64vec3(1, 0, 0)))});
}

void globalRotateY(flecs::entity e, const glm::float64 &angle) {
    e.set(GlobalTransform{
            glm::f64mat4(1.0) *
            glm::mat4_cast(glm::f64quat(angle, glm::f64vec3(0, 1, 0)))});
}

void globalRotateZ(flecs::entity e, const glm::float64 &angle) {
    e.set(GlobalTransform{
            glm::f64mat4(1.0) *
            glm::mat4_cast(glm::f64quat(angle, glm::f64vec3(0, 0, 1)))});
}

void globalTranslate(flecs::entity e, const glm::f64vec3 &pos) {
    e.set(GlobalTransform{glm::translate(glm::f64mat4(1.0), pos)});
}

void globalTranslateX(flecs::entity e, const glm::float64 &distance) {
    e.set(GlobalTransform{
            glm::translate(glm::f64mat4(1.0), glm::f64vec3(distance, 0, 0))});
}

void globalTranslateY(flecs::entity e, const glm::float64 &distance) {
    e.set(GlobalTransform{
            glm::translate(glm::f64mat4(1.0), glm::f64vec3(0, distance, 0))});
}

void globalTranslateZ(flecs::entity e, const glm::float64 &distance) {
    e.set(GlobalTransform{
            glm::translate(glm::f64mat4(1.0), glm::f64vec3(0, 0, distance))});
}

void globalSetScale(flecs::entity e, const glm::float64 &scale) {
    e.set(GlobalTransform{glm::scale(glm::f64mat4(1.0), glm::f64vec3(scale))});
}

void setGlobalFromEntity(flecs::entity e, const flecs::entity &parent) {
    if (parent.has<GlobalTransform>()) {
        e.set(GlobalTransform{parent.get<GlobalTransform>()->TransformMatrix});
    }
}

void inverseGlobal(flecs::entity e) {
    auto *transform = e.get_mut<GlobalTransform>();
    transform->TransformMatrix = glm::inverse(transform->TransformMatrix);
}

void TransformSystem(const flecs::world &world) {
    world.system<GlobalTransform>("UpdateChildrenGlobal")
            .kind(flecs::OnSet)
            .each(UpdateChildrenGlobal);
    world.system<Parent>("CreateChildLocalIfParentSet")
            .kind(flecs::OnAdd)
            .each(CreateChildLocalIfParentSet);
    world.system<Parent>("UpdateChildLocalIfParentChanged")
            .kind(flecs::OnSet)
            .each(UpdateChildLocalIfParentChanged);
    world.system<GlobalTransform>("UpdateChildLocalIfGlobalChanged")
            .kind(flecs::OnSet)
            .each(UpdateChildLocalIfGlobalChanged);
    world.system<LocalTransform>("UpdateChildGlobalIfLocalChanged")
            .kind(flecs::OnSet)
            .each(UpdateChildGlobalIfLocalChanged);
}