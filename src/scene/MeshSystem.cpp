#include "scene/MeshSystem.h"

namespace {

void UpdateMesh(flecs::entity e, const GlobalTransform &gt) {
    engine::graphics::Graphics::getInstance()->set_mesh_instance_transform(
            e.get_mut<MeshComponent>()->MeshID, gt.TransformMatrix);
}

void DestroyMesh(const MeshComponent &mc) {
    engine::graphics::Graphics::getInstance()->free_mesh_instance(mc.MeshID);
}
}  // namespace

void MeshSystem(const flecs::world &world) {
    world.system<GlobalTransform>("UpdateMesh")
            .kind(flecs::OnUpdate)
            .each(UpdateMesh);
    world.system<MeshComponent>("DestroyMesh")
            .kind(flecs::OnRemove)
            .each(DestroyMesh);
}