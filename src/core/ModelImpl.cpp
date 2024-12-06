#include "core/ModelImpl.h"

#include <cassert>
#include <cstring>
#include <fastgltf/core.hpp>
#include <fastgltf/glm_element_traits.hpp>
#include <fastgltf/tools.hpp>
#include <iostream>

#include "core/config.h"
#include "graphics/vulkan/MeshNode.h"
#define GLM_ENABLE_EXPERIMENTAL
#include <glm/gtx/quaternion.hpp>

namespace {

VkFilter extract_filter(fastgltf::Filter filter) {
    switch (filter) {
        // nearest samplers
        case fastgltf::Filter::Nearest:
        case fastgltf::Filter::NearestMipMapNearest:
        case fastgltf::Filter::NearestMipMapLinear:
            return VK_FILTER_NEAREST;

        // linear samplers
        case fastgltf::Filter::Linear:
        case fastgltf::Filter::LinearMipMapNearest:
        case fastgltf::Filter::LinearMipMapLinear:
        default:
            return VK_FILTER_LINEAR;
    }
}

VkSamplerMipmapMode extract_mipmap_mode(fastgltf::Filter filter) {
    switch (filter) {
        case fastgltf::Filter::NearestMipMapNearest:
        case fastgltf::Filter::LinearMipMapNearest:
            return VK_SAMPLER_MIPMAP_MODE_NEAREST;

        case fastgltf::Filter::NearestMipMapLinear:
        case fastgltf::Filter::LinearMipMapLinear:
        default:
            return VK_SAMPLER_MIPMAP_MODE_LINEAR;
    }
}

constexpr auto kOptionsGLTF = fastgltf::Options::DontRequireValidAssetMember |
                              fastgltf::Options::AllowDouble |
                              fastgltf::Options::LoadGLBBuffers |
                              fastgltf::Options::LoadExternalBuffers;

bool check_parser_result(fastgltf::Asset& gltf,
                         fastgltf::Expected<fastgltf::Asset>& load) {
    if (!load) {
        std::cerr << "Failed to load glTF: "
                  << fastgltf::to_underlying(load.error()) << std::endl;
        return false;
    }
    gltf = std::move(load.get());
    return true;
}

bool load_file(fastgltf::Asset& gltf, std::string_view filePath) {
    fastgltf::Parser parser{};

    fastgltf::GltfDataBuffer data;
    data.loadFromFile(filePath);

    std::filesystem::path path = filePath;

    const auto type = fastgltf::determineGltfFileType(&data);
    if (type == fastgltf::GltfType::glTF) {
        auto load = parser.loadGltf(&data, path.parent_path(), kOptionsGLTF);
        return check_parser_result(gltf, load);
    }
    if (type == fastgltf::GltfType::GLB) {
        auto load =
                parser.loadGltfBinary(&data, path.parent_path(), kOptionsGLTF);
        return check_parser_result(gltf, load);
    }
    std::cerr << "Failed to determine glTF container" << std::endl;
    return false;
}

void init_descriptor_pool(const VulkanEngine& engine,
                          Mesh::GLTF::LoadedGLTF& file,
                          const fastgltf::Asset& gltf) {
    // we can stimate the descriptors we will need accurately
    std::vector<DescriptorAllocatorGrowable::PoolSizeRatio> sizes = {
            {VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 3},
            {VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 3},
            {VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 1}};

    file.descriptorPool.init(engine._device,
                             static_cast<uint32_t>(gltf.materials.size()),
                             sizes);
}

void load_samplers(const VulkanEngine& engine, Mesh::GLTF::LoadedGLTF& file,
                   fastgltf::Asset& gltf) {
    for (auto& [magFilter, minFilter, wrapS, wrapT, name] : gltf.samplers) {
        VkSamplerCreateInfo sample = {
                .sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO,
                .pNext = nullptr,
                .magFilter = extract_filter(
                        magFilter.value_or(fastgltf::Filter::Nearest)),
                .minFilter = extract_filter(
                        minFilter.value_or(fastgltf::Filter::Nearest)),
                .mipmapMode = extract_mipmap_mode(
                        minFilter.value_or(fastgltf::Filter::Nearest)),
                .minLod = 0,
                .maxLod = VK_LOD_CLAMP_NONE};

        VkSampler newSampler;
        vkCreateSampler(engine._device, &sample, nullptr, &newSampler);

        file.samplers.push_back(newSampler);
    }
}

void load_all_textures(const VulkanEngine& engine, fastgltf::Asset& gltf,
                       std::vector<AllocatedImage>& images) {
    for ([[maybe_unused]] fastgltf::Image& image : gltf.images) {
        images.push_back(engine._errorCheckerboardImage);
    }
}

void create_material_data_buffer(const VulkanEngine& engine,
                                 Mesh::GLTF::LoadedGLTF& file,
                                 const fastgltf::Asset& gltf) {
    file.materialDataBuffer = engine.create_buffer(
            sizeof(GLTFMetallic_Roughness::MaterialConstants) *
                    gltf.materials.size(),
            VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, VMA_MEMORY_USAGE_CPU_TO_GPU);
}

void grab_textures_from_GLTF(
        const Mesh::GLTF::LoadedGLTF& file, fastgltf::Asset& gltf,
        const fastgltf::Material& mat,
        GLTFMetallic_Roughness::MaterialResources& materialResources,
        const std::vector<AllocatedImage>& images) {
    if (mat.pbrData.baseColorTexture.has_value()) {
        const size_t img =
                gltf.textures[mat.pbrData.baseColorTexture.value().textureIndex]
                        .imageIndex.value();
        const size_t sampler =
                gltf.textures[mat.pbrData.baseColorTexture.value().textureIndex]
                        .samplerIndex.value();

        materialResources.colorImage = images[img];
        materialResources.colorSampler = file.samplers[sampler];
    }
}

void load_material_data(
        VulkanEngine& engine, Mesh::GLTF::LoadedGLTF& file,
        fastgltf::Asset& gltf,
        std::vector<std::shared_ptr<Mesh::GLTF::GLTFMaterial>>& materials,
        const std::vector<AllocatedImage>& images) {
    auto* sceneMaterialConstants =
            static_cast<GLTFMetallic_Roughness::MaterialConstants*>(
                    file.materialDataBuffer.info.pMappedData);

    for (size_t data_index{}; fastgltf::Material & mat : gltf.materials) {
        auto newMat = std::make_shared<Mesh::GLTF::GLTFMaterial>();
        materials.push_back(newMat);
        file.materials[mat.name.c_str()] = newMat;

        // write material parameters to buffer
        sceneMaterialConstants[data_index] = {
                .colorFactors{
                        mat.pbrData.baseColorFactor[0],
                        mat.pbrData.baseColorFactor[1],
                        mat.pbrData.baseColorFactor[2],
                        mat.pbrData.baseColorFactor[3],
                },
                .metal_rough_factors{
                        mat.pbrData.metallicFactor,
                        mat.pbrData.roughnessFactor,
                        {},
                        {},
                },
        };

        auto passType = MaterialPass::MainColor;
        if (mat.alphaMode == fastgltf::AlphaMode::Blend) {
            passType = MaterialPass::Transparent;
        }

        GLTFMetallic_Roughness::MaterialResources materialResources{
                // default the material textures
                engine._whiteImage, engine._defaultSamplerLinear,
                engine._whiteImage, engine._defaultSamplerLinear,
                // set the uniform buffer for the material data
                file.materialDataBuffer.buffer,
                static_cast<uint32_t>(
                        data_index *
                        sizeof(GLTFMetallic_Roughness::MaterialConstants))};

        grab_textures_from_GLTF(file, gltf, mat, materialResources, images);
        // build material
        newMat->data = engine.metalRoughMaterial.write_material(
                engine._device, passType, materialResources,
                file.descriptorPool);

        ++data_index;
    }
}

void load_indexes(const fastgltf::Asset& gltf, std::vector<uint32_t>& indices,
                  fastgltf::Primitive& p, size_t initial_vtx) {
    {
        const fastgltf::Accessor& indexaccessor =
                gltf.accessors[p.indicesAccessor.value()];
        indices.reserve(indices.size() + indexaccessor.count);

        fastgltf::iterateAccessor<std::uint32_t>(
                gltf, indexaccessor, [&](std::uint32_t idx) {
                    indices.push_back(idx + static_cast<uint32_t>(initial_vtx));
                });
    }
}

void load_vertex_positions(const fastgltf::Asset& gltf,
                           std::vector<Vertex>& vertices,
                           fastgltf::Primitive& p, size_t initial_vtx) {
    const fastgltf::Accessor& posAccessor =
            gltf.accessors[p.findAttribute("POSITION")->second];
    vertices.resize(vertices.size() + posAccessor.count);

    fastgltf::iterateAccessorWithIndex<glm::vec3>(
            gltf, posAccessor, [&](const glm::vec3 v, size_t index) {
                vertices[initial_vtx + index] = {
                        .position = v,
                        .normal = {1, 0, 0},
                        .color = glm::vec4{1.},
                        .uv_x = 0,
                        .uv_y = 0,
                };
            });
}

void load_vertex_normals(const fastgltf::Asset& gltf,
                         std::vector<Vertex>& vertices, fastgltf::Primitive& p,
                         size_t initial_vtx) {
    const auto normals = p.findAttribute("NORMAL");
    if (normals != p.attributes.end()) {
        fastgltf::iterateAccessorWithIndex<glm::vec3>(
                gltf, gltf.accessors[normals->second],
                [&](const glm::vec3 v, size_t index) {
                    vertices[initial_vtx + index].normal = v;
                });
    }
}

void load_UVs(const fastgltf::Asset& gltf, std::vector<Vertex>& vertices,
              fastgltf::Primitive& p, size_t initial_vtx) {
    const auto uv = p.findAttribute("TEXCOORD_0");
    if (uv != p.attributes.end()) {
        fastgltf::iterateAccessorWithIndex<glm::vec2>(
                gltf, gltf.accessors[uv->second],
                [&](glm::vec2 v, size_t index) {
                    vertices[initial_vtx + index].uv_x = v.x;
                    vertices[initial_vtx + index].uv_y = v.y;
                });
    }
}

void load_vertex_colors(const fastgltf::Asset& gltf,
                        std::vector<Vertex>& vertices, fastgltf::Primitive& p,
                        size_t initial_vtx) {
    const auto colors = p.findAttribute("COLOR_0");
    if (colors != p.attributes.end()) {
        fastgltf::iterateAccessorWithIndex<glm::vec4>(
                gltf, gltf.accessors[colors->second],
                [&](const glm::vec4 v, size_t index) {
                    vertices[initial_vtx + index].color = v;
                });
    }
}

void define_new_surface_material(
        Mesh::GLTF::GeoSurface& newSurface, fastgltf::Primitive& p,
        const std::vector<std::shared_ptr<Mesh::GLTF::GLTFMaterial>>&
                materials) {
    if (p.materialIndex.has_value()) {
        newSurface.material = materials[p.materialIndex.value()];
    } else {
        newSurface.material = materials[0];
    }
}

void upload_mesh_to_engine(
        VulkanEngine& engine, Mesh::GLTF::LoadedGLTF& file,
        fastgltf::Asset& gltf,
        std::vector<std::shared_ptr<Mesh::GLTF::MeshAsset>>& meshes,
        const std::vector<std::shared_ptr<Mesh::GLTF::GLTFMaterial>>&
                materials) {
    // use the same vectors for all meshes so that the memory doesn't reallocate
    // as often
    std::vector<uint32_t> indices;
    std::vector<Vertex> vertices;

    for (auto& [primitives, weights, name] : gltf.meshes) {
        auto newmesh = std::make_shared<Mesh::GLTF::MeshAsset>();
        meshes.push_back(newmesh);
        file.meshes[name.c_str()] = newmesh;
        newmesh->name = name;

        // clear the mesh arrays each mesh, we don't want to merge them by error
        indices.clear();
        vertices.clear();

        for (auto&& p : primitives) {
            Mesh::GLTF::GeoSurface newSurface;
            newSurface.startIndex = static_cast<uint32_t>(indices.size());
            newSurface.count = static_cast<uint32_t>(
                    gltf.accessors[p.indicesAccessor.value()].count);

            const size_t initial_vtx = vertices.size();

            load_indexes(gltf, indices, p, initial_vtx);
            load_vertex_positions(gltf, vertices, p, initial_vtx);
            load_vertex_normals(gltf, vertices, p, initial_vtx);

            load_UVs(gltf, vertices, p, initial_vtx);
            load_vertex_colors(gltf, vertices, p, initial_vtx);
            define_new_surface_material(newSurface, p, materials);
            newmesh->surfaces.push_back(newSurface);
        }

        newmesh->meshBuffers = engine.uploadMesh(indices, vertices);
    }
}

void load_nodes(Mesh::GLTF::LoadedGLTF& file, fastgltf::Asset& gltf,
                std::vector<std::shared_ptr<Mesh::GLTF::MeshAsset>>& meshes,
                std::vector<std::shared_ptr<ENode>>& nodes) {
    for (fastgltf::Node& node : gltf.nodes) {
        std::shared_ptr<ENode> newNode;

        // find if the node has a mesh, and if it does hook it to the mesh
        // pointer and allocate it with the meshnode class
        if (node.meshIndex.has_value()) {
            newNode = std::make_shared<MeshNode>();
            dynamic_cast<MeshNode*>(newNode.get())->mesh =
                    meshes[*node.meshIndex];
        } else {
            newNode = std::make_shared<ENode>();
        }

        nodes.push_back(newNode);
        file.nodes[node.name.c_str()];

        std::visit(fastgltf::visitor{
                           [&](fastgltf::Node::TransformMatrix matrix) {
                               memcpy(&newNode->localTransform, matrix.data(),
                                      sizeof(matrix));
                           },
                           [&](const fastgltf::TRS& transform) {
                               const glm::vec3 tl(transform.translation[0],
                                                  transform.translation[1],
                                                  transform.translation[2]);
                               const glm::quat rot(transform.rotation[3],
                                                   transform.rotation[0],
                                                   transform.rotation[1],
                                                   transform.rotation[2]);
                               const glm::vec3 sc(transform.scale[0],
                                                  transform.scale[1],
                                                  transform.scale[2]);

                               const glm::mat4 tm =
                                       glm::translate(glm::mat4(1.f), tl);
                               const glm::mat4 rm = glm::toMat4(rot);
                               const glm::mat4 sm =
                                       glm::scale(glm::mat4(1.f), sc);

                               newNode->localTransform = tm * rm * sm;
                           }},
                   node.transform);
    }
}

void setup_nodes_relationships(Mesh::GLTF::LoadedGLTF& file,
                               fastgltf::Asset& gltf,
                               std::vector<std::shared_ptr<ENode>>& nodes) {
    // run loop again to set up transform hierarchy
    for (size_t i = 0; i < gltf.nodes.size(); i++) {
        fastgltf::Node& node = gltf.nodes[i];
        const std::shared_ptr<ENode>& sceneNode = nodes[i];

        for (const auto& c : node.children) {
            sceneNode->children.push_back(nodes[c]);
            nodes[c]->parent = sceneNode;
        }
    }

    // find the top nodes, with no parents
    for (auto& node : nodes) {
        if (node->parent.lock() == nullptr) {
            file.topNodes.push_back(node);
            node->refreshTransform(glm::mat4{1.f});
        }
    }
}

std::optional<const std::shared_ptr<const Mesh::GLTF::LoadedGLTF>> loadGLTF(
        VulkanEngine& engine, std::string_view filePath) {
    fmt::print("Loading GLTF: {}", filePath);

    auto scene = std::make_shared<Mesh::GLTF::LoadedGLTF>();
    scene->creator = &engine;
    Mesh::GLTF::LoadedGLTF& file = *scene;

    fastgltf::Asset gltf;

    if (!load_file(gltf, filePath)) {
        return {};
    }

    init_descriptor_pool(engine, file, gltf);

    load_samplers(engine, file, gltf);

    std::vector<AllocatedImage> images;
    load_all_textures(engine, gltf, images);

    create_material_data_buffer(engine, file, gltf);

    std::vector<std::shared_ptr<Mesh::GLTF::GLTFMaterial>> materials;
    load_material_data(engine, file, gltf, materials, images);

    std::vector<std::shared_ptr<Mesh::GLTF::MeshAsset>> meshes;
    upload_mesh_to_engine(engine, file, gltf, meshes, materials);

    std::vector<std::shared_ptr<ENode>> nodes;
    load_nodes(file, gltf, meshes, nodes);
    setup_nodes_relationships(file, gltf, nodes);
    return scene;
}
}  // unnamed namespace

ModelImpl::~ModelImpl() {
    _engine.cleanup();
}

ModelImpl::ModelImpl() = default;

void ModelImpl::registerWindow(struct SDL_Window* window) {
    _engine.mainCamera = &_camera;
    _engine.init(window);
}

VulkanEngine& ModelImpl::get_engine() {
    assert(_engine._isInitialized);
    return _engine;
}

Camera* ModelImpl::getCamera() {
    return &_camera;
}

Mesh::rid_t registerMesh(VulkanEngine& engine, ModelImpl::MeshMap& meshes,
                         const std::filesystem::path& filePath) {
    std::random_device rd;

    // Use the Mersenne Twister engine for high-quality random numbers
    std::mt19937_64 generator(rd());

    // Create a uniform distribution for int64_t
    std::uniform_int_distribution<Mesh::rid_t> distribution;

    // Generate and print a random int64_t value
    const Mesh::rid_t random_rid_t = distribution(generator);

    std::string structurePath = {std::string(ASSETS_DIR) + filePath.string()};
    auto structureFile = loadGLTF(engine, structurePath);

    assert(structureFile.has_value());

    engine.loadedScenes["structure"] = *structureFile;

    meshes[random_rid_t] = {structureFile.value(), glm::mat4(1.)};

    return random_rid_t;
}

Mesh::rid_t ModelImpl::createMesh(const std::filesystem::path& file_path) {
    assert(_engine._isInitialized);
    const Mesh::rid_t rid = registerMesh(_engine, _meshes, file_path);
    _meshes[rid].transform = 1.;
    return rid;
}

void ModelImpl::setMeshTransform(Mesh::rid_t rid, glm::mat4x4 transform) {
    _meshes.at(rid).transform = transform;
}

glm::mat4 ModelImpl::get_mesh_transform(Mesh::rid_t rid) {
    return _meshes.at(rid).transform;
}

void ModelImpl::delete_mesh(Mesh::rid_t rid) {
    if(!_meshes.erase(rid)) {
        throw std::invalid_argument("Invalid id of the mesh");
    }
}

const ModelImpl::MeshMap& ModelImpl::get_meshes() {
    return _meshes;
}
