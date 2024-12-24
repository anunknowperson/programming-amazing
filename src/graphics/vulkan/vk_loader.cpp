#include "graphics/vulkan/vk_loader.h"

#include <array>
#include <cstdint>
#include <cstring>
#include <fastgltf/core.hpp>
#include <fastgltf/glm_element_traits.hpp>
#include <fastgltf/tools.hpp>
#include <fastgltf/types.hpp>
#include <fastgltf/util.hpp>
#include <fmt/base.h>
#include <vk_mem_alloc.h>
#define GLM_ENABLE_EXPERIMENTAL
#include <glm/detail/qualifier.hpp>
#include <glm/detail/type_mat4x4.hpp>
#include <glm/detail/type_vec2.hpp>
#include <glm/detail/type_vec3.hpp>
#include <glm/detail/type_vec4.hpp>
#include <glm/ext/quaternion_float.hpp>
#include <glm/gtx/quaternion.hpp>
#include <iostream>
#include <span>
#include <utility>
#include <variant>

#include "graphics/vulkan/vk_descriptors.h"
#include "graphics/vulkan/vk_engine.h"
#include "graphics/vulkan/vk_types.h"

std::optional<std::vector<std::shared_ptr<MeshAsset>>> loadGltfMeshes(
        VulkanEngine* engine, const std::filesystem::path& filePath) {
    if (!std::filesystem::exists(filePath)) {
        std::cout << "Failed to find " << filePath << '\n';
        return {};
    }

    std::cout << "Loading " << filePath << '\n';

    fastgltf::Asset gltf;

    // Parse the glTF file and get the constructed asset

    static constexpr auto supportedExtensions =
            fastgltf::Extensions::KHR_mesh_quantization |
            fastgltf::Extensions::KHR_texture_transform |
            fastgltf::Extensions::KHR_materials_variants;

    fastgltf::Parser parser(supportedExtensions);

    auto path = std::filesystem::path{filePath};

    constexpr auto gltfOptions =
            fastgltf::Options::DontRequireValidAssetMember |
            fastgltf::Options::AllowDouble | fastgltf::Options::LoadGLBBuffers |
            fastgltf::Options::LoadExternalBuffers |
            fastgltf::Options::LoadExternalImages |
            fastgltf::Options::GenerateMeshIndices;

    fastgltf::GltfDataBuffer data;
    data.loadFromFile(path);

    auto asset = parser.loadGltf(&data, path.parent_path(), gltfOptions);

    if (asset) {
        gltf = std::move(asset.get());
    } else {
        fmt::print("Failed to load glTF: {} \n",
                   fastgltf::to_underlying(asset.error()));
        return {};
    }

    std::vector<std::shared_ptr<MeshAsset>> meshes;

    // use the same vectors for all meshes so that the memory doesnt reallocate
    // as often
    std::vector<uint32_t> indices;
    std::vector<Vertex> vertices;
    for (auto& [primitives, _, name] : gltf.meshes) {
        MeshAsset newmesh;

        newmesh.name = name;

        // clear the mesh arrays each mesh, we dont want to merge them by error
        indices.clear();
        vertices.clear();

        for (auto&& p : primitives) {
            GeoSurface newSurface;
            newSurface.startIndex = (uint32_t)indices.size();
            newSurface.count =
                    (uint32_t)gltf.accessors[p.indicesAccessor.value()].count;

            auto initial_vtx = static_cast<uint32_t>(vertices.size());

            // load indexes
            {
                fastgltf::Accessor& indexaccessor =
                        gltf.accessors[p.indicesAccessor.value()];
                indices.reserve(indices.size() + indexaccessor.count);

                fastgltf::iterateAccessor<std::uint32_t>(
                        gltf, indexaccessor, [&](std::uint32_t idx) {
                            indices.push_back(idx + initial_vtx);
                        });
            }

            // load vertex positions
            {
                fastgltf::Accessor& posAccessor =
                        gltf.accessors[p.findAttribute("POSITION")->second];
                vertices.resize(vertices.size() + posAccessor.count);

                fastgltf::iterateAccessorWithIndex<glm::vec3>(
                        gltf, posAccessor, [&](glm::vec3 v, size_t index) {
                            Vertex newvtx;
                            newvtx.position = v;
                            newvtx.normal = {1, 0, 0};
                            newvtx.color = glm::vec4{1.f};
                            newvtx.uv_x = 0;
                            newvtx.uv_y = 0;
                            vertices[initial_vtx + index] = newvtx;
                        });
            }

            // load vertex normals
            auto normals = p.findAttribute("NORMAL");
            if (normals != p.attributes.end()) {
                fastgltf::iterateAccessorWithIndex<glm::vec3>(
                        gltf, gltf.accessors[normals->second],
                        [&](glm::vec3 v, size_t index) {
                            vertices[initial_vtx + index].normal = v;
                        });
            }

            // load UVs
            auto uv = p.findAttribute("TEXCOORD_0");
            if (uv != p.attributes.end()) {
                fastgltf::iterateAccessorWithIndex<glm::vec2>(
                        gltf, gltf.accessors[uv->second],
                        [&](glm::vec2 v, size_t index) {
                            vertices[initial_vtx + index].uv_x = v.x;
                            vertices[initial_vtx + index].uv_y = v.y;
                        });
            }

            // load vertex colors
            auto colors = p.findAttribute("COLOR_0");
            if (colors != p.attributes.end()) {
                fastgltf::iterateAccessorWithIndex<glm::vec4>(
                        gltf, gltf.accessors[colors->second],
                        [&](glm::vec4 v, size_t index) {
                            vertices[initial_vtx + index].color = v;
                        });
            }
            newmesh.surfaces.push_back(newSurface);
        }

        // display the vertex normals
        constexpr bool OverrideColors = true;
        if (OverrideColors) {
            for (Vertex& vtx : vertices) {
                vtx.color = glm::vec4(vtx.normal, 1.f);
            }
        }
        newmesh.meshBuffers = engine->uploadMesh(indices, vertices);

        meshes.emplace_back(std::make_shared<MeshAsset>(std::move(newmesh)));
    }

    return meshes;
}

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

std::optional<std::shared_ptr<LoadedGLTF>> loadGltf(VulkanEngine* engine,
                                                    std::string_view filePath) {
    fmt::print("Loading GLTF: {}", filePath);

    auto scene = std::make_shared<LoadedGLTF>();
    scene->creator = engine;
    LoadedGLTF& file = *scene;

    fastgltf::Parser parser{};

    constexpr auto gltfOptions =
            fastgltf::Options::DontRequireValidAssetMember |
            fastgltf::Options::AllowDouble | fastgltf::Options::LoadGLBBuffers |
            fastgltf::Options::LoadExternalBuffers;
    // fastgltf::Options::LoadExternalImages;

    fastgltf::GltfDataBuffer data;
    data.loadFromFile(filePath);

    fastgltf::Asset gltf;

    std::filesystem::path path = filePath;

    auto type = fastgltf::determineGltfFileType(&data);
    if (type == fastgltf::GltfType::glTF) {
        auto load = parser.loadGltf(&data, path.parent_path(), gltfOptions);
        if (load) {
            gltf = std::move(load.get());
        } else {
            std::cerr << "Failed to load glTF: "
                      << fastgltf::to_underlying(load.error()) << std::endl;
            return {};
        }
    } else if (type == fastgltf::GltfType::GLB) {
        auto load =
                parser.loadGltfBinary(&data, path.parent_path(), gltfOptions);
        if (load) {
            gltf = std::move(load.get());
        } else {
            std::cerr << "Failed to load glTF: "
                      << fastgltf::to_underlying(load.error()) << std::endl;
            return {};
        }
    } else {
        std::cerr << "Failed to determine glTF container" << std::endl;
        return {};
    }

    // we can stimate the descriptors we will need accurately
    std::vector<DescriptorAllocatorGrowable::PoolSizeRatio> sizes = {
            {VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 3},
            {VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 3},
            {VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 1}};

    file.descriptorPool.init(engine->_device,
                             static_cast<uint32_t>(gltf.materials.size()),
                             sizes);

    // load samplers
    for (fastgltf::Sampler& sampler : gltf.samplers) {
        VkSamplerCreateInfo sampl = {
                .sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO,
                .pNext = nullptr};
        sampl.maxLod = VK_LOD_CLAMP_NONE;
        sampl.minLod = 0;

        sampl.magFilter = extract_filter(
                sampler.magFilter.value_or(fastgltf::Filter::Nearest));
        sampl.minFilter = extract_filter(
                sampler.minFilter.value_or(fastgltf::Filter::Nearest));

        sampl.mipmapMode = extract_mipmap_mode(
                sampler.minFilter.value_or(fastgltf::Filter::Nearest));

        VkSampler newSampler;
        vkCreateSampler(engine->_device, &sampl, nullptr, &newSampler);

        file.samplers.push_back(newSampler);
    }

    std::vector<std::shared_ptr<MeshAsset>> meshes;
    std::vector<std::shared_ptr<ENode>> nodes;
    std::vector<AllocatedImage> images;
    std::vector<std::shared_ptr<GLTFMaterial>> materials;

    // load all textures
    images.reserve(gltf.images.size());
    for (size_t i = 0; i < gltf.images.size(); i++) {
        images.push_back(engine->_errorCheckerboardImage);
    }

    // create buffer to hold the material data
    file.materialDataBuffer = engine->create_buffer(
            sizeof(GLTFMetallic_Roughness::MaterialConstants) *
                    gltf.materials.size(),
            VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, VMA_MEMORY_USAGE_CPU_TO_GPU);
    uint32_t data_index = 0;
    auto* sceneMaterialConstants =
            (GLTFMetallic_Roughness::MaterialConstants*)
                    file.materialDataBuffer.info.pMappedData;

    for (fastgltf::Material& mat : gltf.materials) {
        auto newMat = std::make_shared<GLTFMaterial>();
        materials.push_back(newMat);
        file.materials[mat.name.c_str()] = newMat;

        GLTFMetallic_Roughness::MaterialConstants constants;
        constants.colorFactors.x = mat.pbrData.baseColorFactor[0];
        constants.colorFactors.y = mat.pbrData.baseColorFactor[1];
        constants.colorFactors.z = mat.pbrData.baseColorFactor[2];
        constants.colorFactors.w = mat.pbrData.baseColorFactor[3];

        constants.metal_rough_factors.x = mat.pbrData.metallicFactor;
        constants.metal_rough_factors.y = mat.pbrData.roughnessFactor;
        // write material parameters to buffer
        sceneMaterialConstants[data_index] = constants;

        auto passType = MaterialPass::MainColor;
        if (mat.alphaMode == fastgltf::AlphaMode::Blend) {
            passType = MaterialPass::Transparent;
        }

        GLTFMetallic_Roughness::MaterialResources materialResources;
        // default the material textures
        materialResources.colorImage = engine->_whiteImage;
        materialResources.colorSampler = engine->_defaultSamplerLinear;
        materialResources.metalRoughImage = engine->_whiteImage;
        materialResources.metalRoughSampler = engine->_defaultSamplerLinear;

        // set the uniform buffer for the material data
        materialResources.dataBuffer = file.materialDataBuffer.buffer;
        materialResources.dataBufferOffset =
                data_index * sizeof(GLTFMetallic_Roughness::MaterialConstants);
        // grab textures from gltf file
        if (mat.pbrData.baseColorTexture.has_value()) {
            size_t img = gltf.textures[mat.pbrData.baseColorTexture.value()
                                               .textureIndex]
                                 .imageIndex.value();
            size_t sampler = gltf.textures[mat.pbrData.baseColorTexture.value()
                                                   .textureIndex]
                                     .samplerIndex.value();

            materialResources.colorImage = images[img];
            materialResources.colorSampler = file.samplers[sampler];
        }
        // build material
        newMat->data = engine->metalRoughMaterial.write_material(
                engine->_device, passType, materialResources,
                file.descriptorPool);

        data_index++;
    }

    // use the same vectors for all meshes so that the memory doesnt reallocate
    // as
    // often
    std::vector<uint32_t> indices;
    std::vector<Vertex> vertices;

    for (auto& [primitives, _, name] : gltf.meshes) {
        auto newmesh = std::make_shared<MeshAsset>();
        meshes.push_back(newmesh);
        file.meshes[name.c_str()] = newmesh;
        newmesh->name = name;

        // clear the mesh arrays each mesh, we dont want to merge them by error
        indices.clear();
        vertices.clear();

        for (auto&& p : primitives) {
            GeoSurface newSurface;
            newSurface.startIndex = (uint32_t)indices.size();
            newSurface.count =
                    (uint32_t)gltf.accessors[p.indicesAccessor.value()].count;

            auto initial_vtx = static_cast<uint32_t>(vertices.size());

            // load indexes
            {
                fastgltf::Accessor& indexaccessor =
                        gltf.accessors[p.indicesAccessor.value()];
                indices.reserve(indices.size() + indexaccessor.count);

                fastgltf::iterateAccessor<std::uint32_t>(
                        gltf, indexaccessor, [&](std::uint32_t idx) {
                            indices.push_back(idx + initial_vtx);
                        });
            }

            // load vertex positions
            {
                fastgltf::Accessor& posAccessor =
                        gltf.accessors[p.findAttribute("POSITION")->second];
                vertices.resize(vertices.size() + posAccessor.count);

                fastgltf::iterateAccessorWithIndex<glm::vec3>(
                        gltf, posAccessor, [&](glm::vec3 v, size_t index) {
                            Vertex newvtx;
                            newvtx.position = v;
                            newvtx.normal = {1, 0, 0};
                            newvtx.color = glm::vec4{1.f};
                            newvtx.uv_x = 0;
                            newvtx.uv_y = 0;
                            vertices[initial_vtx + index] = newvtx;
                        });
            }

            // load vertex normals
            auto normals = p.findAttribute("NORMAL");
            if (normals != p.attributes.end()) {
                fastgltf::iterateAccessorWithIndex<glm::vec3>(
                        gltf, gltf.accessors[normals->second],
                        [&](glm::vec3 v, size_t index) {
                            vertices[initial_vtx + index].normal = v;
                        });
            }

            // load UVs
            auto uv = p.findAttribute("TEXCOORD_0");
            if (uv != p.attributes.end()) {
                fastgltf::iterateAccessorWithIndex<glm::vec2>(
                        gltf, gltf.accessors[uv->second],
                        [&](glm::vec2 v, size_t index) {
                            vertices[initial_vtx + index].uv_x = v.x;
                            vertices[initial_vtx + index].uv_y = v.y;
                        });
            }

            // load vertex colors
            auto colors = p.findAttribute("COLOR_0");
            if (colors != p.attributes.end()) {
                fastgltf::iterateAccessorWithIndex<glm::vec4>(
                        gltf, gltf.accessors[colors->second],
                        [&](glm::vec4 v, size_t index) {
                            vertices[initial_vtx + index].color = v;
                        });
            }

            if (p.materialIndex.has_value()) {
                newSurface.material = materials[p.materialIndex.value()];
            } else {
                newSurface.material = materials[0];
            }

            newmesh->surfaces.push_back(newSurface);
        }

        newmesh->meshBuffers = engine->uploadMesh(indices, vertices);
    }

    // load all nodes and their meshes
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
                           [&](const fastgltf::Node::TransformMatrix& matrix) {
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

    // run loop again to setup transform hierarchy
    for (size_t i = 0; i < gltf.nodes.size(); i++) {
        fastgltf::Node& node = gltf.nodes[i];
        std::shared_ptr<ENode>& sceneNode = nodes[i];

        for (auto& c : node.children) {
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
    return scene;
}

void LoadedGLTF::Draw(const glm::mat4& topMatrix, DrawContext& ctx) {
    // create renderables from the scenenodes
    for (const auto& n : topNodes) {
        n->Draw(topMatrix, ctx);
    }
}

void LoadedGLTF::clearAll() {}
