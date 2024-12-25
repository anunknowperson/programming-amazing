#include "graphics/vulkan/pipelines.h"

#include "graphics/vulkan/vk_engine.h"

void GLTFMetallic_Roughness::build_pipelines(VulkanEngine* engine) {
    VkShaderModule meshFragShader =
            load_shader(engine, "./shaders/mesh.frag.spv", "fragment");
    VkShaderModule meshVertexShader =
            load_shader(engine, "./shaders/mesh.vert.spv", "vertex");

    create_material_layout(engine);
    VkPipelineLayout newLayout = create_pipeline_layout(engine);

    opaquePipeline.layout = newLayout;
    transparentPipeline.layout = newLayout;

    build_opaque_pipeline(engine, meshVertexShader, meshFragShader, newLayout);
    build_transparent_pipeline(engine, meshVertexShader, meshFragShader,
                               newLayout);

    vkDestroyShaderModule(engine->_device, meshFragShader, nullptr);
    vkDestroyShaderModule(engine->_device, meshVertexShader, nullptr);
}

VkShaderModule GLTFMetallic_Roughness::load_shader(VulkanEngine* engine,
                                                   const char* path,
                                                   const char* type) {
    VkShaderModule shaderModule;
    if (!vkutil::load_shader_module(path, engine->_device, &shaderModule)) {
        fmt::println("Error when building the {} shader module", type);
    }
    return shaderModule;
}

void GLTFMetallic_Roughness::create_material_layout(VulkanEngine* engine) {
    DescriptorLayoutBuilder layoutBuilder;
    layoutBuilder.add_binding(0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER);
    layoutBuilder.add_binding(1, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER);
    layoutBuilder.add_binding(2, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER);

    materialLayout = layoutBuilder.build(
            engine->_device,
            VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT);
}

VkPipelineLayout GLTFMetallic_Roughness::create_pipeline_layout(
        VulkanEngine* engine) {
    VkPushConstantRange matrixRange{};
    matrixRange.offset = 0;
    matrixRange.size = sizeof(GPUDrawPushConstants);
    matrixRange.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;

    VkDescriptorSetLayout layouts[] = {engine->_gpuSceneDataDescriptorLayout,
                                       materialLayout};
    VkPipelineLayoutCreateInfo mesh_layout_info =
            vkinit::pipeline_layout_create_info();
    mesh_layout_info.setLayoutCount = 2;
    mesh_layout_info.pSetLayouts = layouts;
    mesh_layout_info.pPushConstantRanges = &matrixRange;
    mesh_layout_info.pushConstantRangeCount = 1;

    VkPipelineLayout newLayout;
    VK_CHECK(vkCreatePipelineLayout(engine->_device, &mesh_layout_info, nullptr,
                                    &newLayout));

    return newLayout;
}

void GLTFMetallic_Roughness::build_opaque_pipeline(VulkanEngine* engine,
                                                   VkShaderModule vertexShader,
                                                   VkShaderModule fragShader,
                                                   VkPipelineLayout layout) {
    PipelineBuilder pipelineBuilder;
    pipelineBuilder.set_shaders(vertexShader, fragShader);
    pipelineBuilder.set_input_topology(VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST);
    pipelineBuilder.set_polygon_mode(VK_POLYGON_MODE_FILL);
    pipelineBuilder.set_cull_mode(VK_CULL_MODE_NONE, VK_FRONT_FACE_CLOCKWISE);
    pipelineBuilder.set_multisampling_none();
    pipelineBuilder.disable_blending();
    pipelineBuilder.enable_depthtest(true, VK_COMPARE_OP_GREATER_OR_EQUAL);
    pipelineBuilder.set_color_attachment_format(engine->_drawImage.imageFormat);
    pipelineBuilder.set_depth_format(engine->_depthImage.imageFormat);
    pipelineBuilder._pipelineLayout = layout;

    opaquePipeline.pipeline = pipelineBuilder.build_pipeline(engine->_device);
}

void GLTFMetallic_Roughness::build_transparent_pipeline(
        VulkanEngine* engine, VkShaderModule vertexShader,
        VkShaderModule fragShader, VkPipelineLayout layout) {
    PipelineBuilder pipelineBuilder;
    pipelineBuilder.set_shaders(vertexShader, fragShader);
    pipelineBuilder.enable_blending_additive();
    pipelineBuilder.enable_depthtest(false, VK_COMPARE_OP_GREATER_OR_EQUAL);
    pipelineBuilder._pipelineLayout = layout;

    transparentPipeline.pipeline =
            pipelineBuilder.build_pipeline(engine->_device);
}

MaterialInstance GLTFMetallic_Roughness::write_material(
        VkDevice device, MaterialPass pass, const MaterialResources& resources,
        DescriptorAllocatorGrowable& descriptorAllocator) {
    MaterialInstance matData{};
    matData.passType = pass;
    matData.pipeline = (pass == MaterialPass::Transparent)
                               ? &transparentPipeline
                               : &opaquePipeline;
    matData.materialSet = descriptorAllocator.allocate(device, materialLayout);

    writer.clear();
    writer.write_buffer(0, resources.dataBuffer, sizeof(MaterialConstants),
                        resources.dataBufferOffset,
                        VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER);
    writer.write_image(1, resources.colorImage.imageView,
                       resources.colorSampler,
                       VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
                       VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER);
    writer.write_image(2, resources.metalRoughImage.imageView,
                       resources.metalRoughSampler,
                       VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
                       VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER);

    writer.update_set(device, matData.materialSet);
    writer.update_set(device, matData.materialSet);

    return matData;
}

void Pipelines::init(VkDevice device,
                     VkDescriptorSetLayout singleImageDescriptorLayout,
                     AllocatedImage drawImage) {
    _device = device;
    _singleImageDescriptorLayout = singleImageDescriptorLayout;
    _drawImage = drawImage;

    init_mesh_pipeline();
    init_triangle_pipeline();
}

void Pipelines::init_mesh_pipeline() {
    VkShaderModule fragShader =
            load_shader("./shaders/tex_image.frag.spv", "fragment");
    VkShaderModule vertShader =
            load_shader("./shaders/colored_triangle_mesh.vert.spv", "vertex");

    create_mesh_pipeline_layout();
    build_mesh_pipeline(fragShader, vertShader);

    vkDestroyShaderModule(_device, fragShader, nullptr);
    vkDestroyShaderModule(_device, vertShader, nullptr);
}

void Pipelines::init_triangle_pipeline() {
    VkShaderModule fragShader =
            load_shader("./shaders/colored_triangle.frag.spv", "fragment");
    VkShaderModule vertShader =
            load_shader("./shaders/colored_triangle.vert.spv", "vertex");

    create_triangle_pipeline_layout();
    build_triangle_pipeline(fragShader, vertShader);

    vkDestroyShaderModule(_device, fragShader, nullptr);
    vkDestroyShaderModule(_device, vertShader, nullptr);
}

VkShaderModule Pipelines::load_shader(const std::filesystem::path path, const char* type) {
    std::string str = path.string();
    const char* cPath = str.c_str();  // warning: potential dangling pointer in c string if str leaves scope. Ñ~ÑÄ ÑrÑÇÑÄÑtÑu ÑÑÑÖÑÑ Ñ~ÑÄÑÇÑ}, ÑÑÑpÑ{ Ñ{ÑpÑ{ ÑÅÑÄÑÉÑ|Ñu ÑxÑpÑsÑÇÑÖÑxÑ{Ñy ÑÅÑÖÑÑÑé ÑqÑÄÑ|ÑéÑäÑu Ñ~Ñu ÑyÑÉÑÅÑÄÑ|ÑéÑxÑÖÑuÑÑÑÉÑë.
    VkShaderModule shaderModule;
    if (!vkutil::load_shader_module(cPath, _device, &shaderModule)) {
        fmt::println("Error when building the {} shader module", type);
    } else {
        fmt::println("{} shader successfully loaded", type);
    }
    return shaderModule;
}

void Pipelines::create_mesh_pipeline_layout() {
    VkPushConstantRange bufferRange{};
    bufferRange.offset = 0;
    bufferRange.size = sizeof(GPUDrawPushConstants);
    bufferRange.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;

    VkPipelineLayoutCreateInfo pipeline_layout_info =
            vkinit::pipeline_layout_create_info();
    pipeline_layout_info.pPushConstantRanges = &bufferRange;
    pipeline_layout_info.pushConstantRangeCount = 1;
    pipeline_layout_info.pSetLayouts = &_singleImageDescriptorLayout;
    pipeline_layout_info.setLayoutCount = 1;

    VK_CHECK(vkCreatePipelineLayout(_device, &pipeline_layout_info, nullptr,
                                    &meshPipelineLayout));
}

void Pipelines::build_mesh_pipeline(VkShaderModule fragShader,
                                    VkShaderModule vertShader) {
    PipelineBuilder pipelineBuilder;
    pipelineBuilder._pipelineLayout = meshPipelineLayout;
    pipelineBuilder.set_shaders(vertShader, fragShader);
    pipelineBuilder.set_input_topology(VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST);
    pipelineBuilder.set_polygon_mode(VK_POLYGON_MODE_FILL);
    pipelineBuilder.set_cull_mode(VK_CULL_MODE_NONE, VK_FRONT_FACE_CLOCKWISE);
    pipelineBuilder.set_multisampling_none();
    pipelineBuilder.disable_blending();
    pipelineBuilder.enable_depthtest(true, VK_COMPARE_OP_GREATER);
    pipelineBuilder.set_color_attachment_format(_drawImage.imageFormat);
    pipelineBuilder.set_depth_format(VK_FORMAT_UNDEFINED);

    meshPipeline = pipelineBuilder.build_pipeline(_device);
}

void Pipelines::create_triangle_pipeline_layout() {
    VkPipelineLayoutCreateInfo pipeline_layout_info =
            vkinit::pipeline_layout_create_info();
    VK_CHECK(vkCreatePipelineLayout(_device, &pipeline_layout_info, nullptr,
                                    &trianglePipelineLayout));
}

void Pipelines::build_triangle_pipeline(VkShaderModule fragShader,
                                        VkShaderModule vertShader) {
    PipelineBuilder pipelineBuilder;
    pipelineBuilder._pipelineLayout = trianglePipelineLayout;
    pipelineBuilder.set_shaders(vertShader, fragShader);
    pipelineBuilder.set_input_topology(VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST);
    pipelineBuilder.set_polygon_mode(VK_POLYGON_MODE_FILL);
    pipelineBuilder.set_cull_mode(VK_CULL_MODE_NONE, VK_FRONT_FACE_CLOCKWISE);
    pipelineBuilder.set_multisampling_none();
    pipelineBuilder.disable_blending();
    pipelineBuilder.disable_depthtest();
    pipelineBuilder.set_color_attachment_format(_drawImage.imageFormat);
    pipelineBuilder.set_depth_format(VK_FORMAT_UNDEFINED);

    trianglePipeline = pipelineBuilder.build_pipeline(_device);
}

void Pipelines::destroy() {
    vkDestroyPipelineLayout(_device, trianglePipelineLayout, nullptr);
    vkDestroyPipeline(_device, trianglePipeline, nullptr);

    vkDestroyPipelineLayout(_device, meshPipelineLayout, nullptr);
    vkDestroyPipeline(_device, meshPipeline, nullptr);
}
