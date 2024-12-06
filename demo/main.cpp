#define GLM_ENABLE_EXPERIMENTAL
#include <glm/gtx/transform.hpp>
#include <iostream>
#include <ranges>

#include "IController.h"

void createCubes(const std::weak_ptr<const MeshController>& mesh_controller) {
    for (int i = 0; i < 5; i++) {
        if (const auto sp = mesh_controller.lock()) {
            sp->create_mesh("/basicmesh.glb");
        } else {
            throw std::runtime_error("Cubes could not be created");
        }
    }
}

double getCurrentGlobalTime() {
    // Get the current time point
    const auto now = std::chrono::system_clock::now();

    // Cast to a time duration since the epoch
    const auto durationSinceEpoch = now.time_since_epoch();

    // Convert to seconds in double precision
    const std::chrono::duration<double> seconds = durationSinceEpoch;

    // Return the double value
    return seconds.count();
}

void updateCube(const std::shared_ptr<const MeshController>& mesh_controller, Mesh::rid_t rid, int8_t i) {
    const double sinValue = std::sin(getCurrentGlobalTime() + static_cast<double>(i)) * 5.;

    const glm::mat4 scale = glm::scale(glm::vec3{0.2f});
    const glm::mat4 translation = glm::translate(glm::vec3{static_cast<float>(i) - 2.5f, sinValue, 0});
    mesh_controller->set_transform(rid, scale * translation);
}

void updateCubes(const std::weak_ptr<const MeshController>& mesh_controller) {
    if (const auto sp = mesh_controller.lock()) {
        auto meshes = sp->get_meshes();
        for (int8_t i {}; const auto key : meshes) {
            updateCube(sp, key, i);
            ++i;
        }
    } else {
        throw std::runtime_error("Cubes could not be updated");
    }
}

int main([[maybe_unused]] int argc, [[maybe_unused]] char* args[])
{
	try
	{
        const auto controller = createInstance();
	    const auto mesh_controller = controller->getMeshController();
	    createCubes(mesh_controller);

	    SDL_Event e;
	    bool bQuit = false;
	    bool stop_rendering = false;

	    // main loop
	    while (!bQuit) {
	        // Handle events on queue
	        while (SDL_PollEvent(&e) != 0) {
	            // close the window when user alt-f4s or clicks the X button
	            if (e.type == SDL_QUIT) bQuit = true;

	            if (e.type == SDL_WINDOWEVENT) {
	                if (e.window.event == SDL_WINDOWEVENT_MINIMIZED) {
	                    stop_rendering = true;
	                }
	                if (e.window.event == SDL_WINDOWEVENT_RESTORED) {
	                    stop_rendering = false;
	                }
	            }

	            controller->process_event(e);
	        }

	        // do not draw if we are minimized
	        if (stop_rendering) {
	            // throttle the speed to avoid the endless spinning
	            std::this_thread::sleep_for(std::chrono::milliseconds(100));
	            continue;
	        }

	        controller->run();
	        controller->update();
	        updateCubes(mesh_controller);
	    }
	} catch (std::runtime_error const& e)
	{
		std::cerr << "Unhandled exception: " << e.what() << '\n';
	}

	return 0;
}