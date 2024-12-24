#pragma once

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <map>

namespace engine::graphics {

/** @brief User-friendly graphics API interface.
 *
 * @details This is a transparent class that allows you to perform various
 * drawings on the screen, as well as register meshes, materials, lights,
 * viewports, and so on.
 * */

class Graphics {
public:
    /** @brief Obtain class instance  **/
    static Graphics *getInstance();

    /** @brief Creates a mesh instance and returns its ID.
     * @details All mesh instance parameters are set to default.
     * */

    std::uint64_t create_mesh_instance();

    /** @brief CSets the transformation matrix to the mesh instance by ID.
     * @param rid Mesh Instance Rendering ID.
     * @param matrix New matrix.
     * */

    void set_mesh_instance_transform(std::uint64_t rid, glm::mat4 matrix);

    /** @brief Destroys a mesh instance by its ID. **/

    void free_mesh_instance(std::uint64_t rid);

    ~Graphics() = default;

private:
    /*! \brief ctor */
    Graphics() = default;

    std::map<std::uint64_t, std::uint64_t> meshes;
};

}  // namespace engine::graphics