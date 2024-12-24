#include "graphics/vulkan/vk_engine.h"

#include "core/config.h"

#define VMA_IMPLEMENTATION
#include "SDL_vulkan.h"
#include "VkBootstrap.h"
#include "graphics/vulkan/vk_images.h"
#include "graphics/vulkan/vk_initializers.h"
#include "graphics/vulkan/vk_loader.h"
#include "graphics/vulkan/vk_pipelines.h"
#include "graphics/vulkan/vk_types.h"
#include "imgui.h"
#include "imgui_impl_sdl2.h"
#include "imgui_impl_vulkan.h"
#include "vk_mem_alloc.h"

#define GLM_ENABLE_EXPERIMENTAL
#include "glm/gtx/transform.hpp"

VulkanEngine* loadedEngine = nullptr;

VulkanEngine& VulkanEngine::Get() {
    return *loadedEngine;
}

#ifdef NDEBUG
constexpr bool bUseValidationLayers = false;
#else
constexpr bool bUseValidationLayers = true;
#endif

VKAPI_ATTR VkBool32 VKAPI_CALL VulkanEngine::debugCallback(
        VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
        VkDebugUtilsMessageTypeFlagsEXT messageType,
        const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData,
        [[maybe_unused]] void* pUserData) {
    std::string type;

    switch (messageType) {
        case VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT:
            type = "General";

            break;

        case VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT:
            type = "Validation";

            break;

        case VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT:
            type = "Performance";

            break;
    }

    std::string message = "(" + type + ")" + pCallbackData->pMessage;

    if (messageSeverity == VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT) {
        LOGD(message)
    } else if (messageSeverity ==
               VK_DEBUG_UTILS_MESSAGE_SEVERITY_INFO_BIT_EXT) {
        LOGI(message)
    } else if (messageSeverity ==
               VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT) {
        LOGW(message)
    } else if (messageSeverity >=
               VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT) {
        LOGE("{}", message)
    }

    return VK_FALSE;
}

void VulkanEngine::init_default_data() {
    std::array<Vertex, 4> rect_vertices{};

    rect_vertices[0].position = {0.5, -0.5, 0};
    rect_vertices[1].position = {0.5, 0.5, 0};
    rect_vertices[2].position = {-0.5, -0.5, 0};
    rect_vertices[3].position = {-0.5, 0.5, 0};

    rect_vertices[0].color = {0, 0, 0, 1};
    rect_vertices[1].color = {0.5, 0.5, 0.5, 1};
    rect_vertices[2].color = {1, 0, 0, 1};
    rect_vertices[3].color = {0, 1, 0, 1};

    std::array<uint32_t, 6> rect_indices{};

    rect_indices[0] = 0;
    rect_indices[1] = 1;
    rect_indices[2] = 2;

    auto path_to_assets = std::string(ASSETS_DIR) + "/basicmesh.glb";
    testMeshes = loadGltfMeshes(this, path_to_assets).value();

    // 3 default textures, white, grey, black. 1 pixel each
    uint32_t white = glm::packUnorm4x8(glm::vec4(1, 1, 1, 1));
    _whiteImage =
            create_image((void*)&white, VkExtent3D{1, 1, 1},
                         VK_FORMAT_R8G8B8A8_UNORM, VK_IMAGE_USAGE_SAMPLED_BIT);

    uint32_t grey = glm::packUnorm4x8(glm::vec4(0.66f, 0.66f, 0.66f, 1));
    _greyImage =
            create_image((void*)&grey, VkExtent3D{1, 1, 1},
                         VK_FORMAT_R8G8B8A8_UNORM, VK_IMAGE_USAGE_SAMPLED_BIT);

    uint32_t black = glm::packUnorm4x8(glm::vec4(0, 0, 0, 0));
    _blackImage =
            create_image((void*)&black, VkExtent3D{1, 1, 1},
                         VK_FORMAT_R8G8B8A8_UNORM, VK_IMAGE_USAGE_SAMPLED_BIT);

    // checkerboard image
    uint32_t magenta = glm::packUnorm4x8(glm::vec4(1, 0, 1, 1));
    std::array<uint32_t, 16 * 16> pixels{};  // for 16x16 checkerboard texture
    for (size_t x = 0; x < 16; x++) {
        for (size_t y = 0; y < 16; y++) {
            pixels[y * 16 + x] = ((x % 2) ^ (y % 2)) ? magenta : black;
        }
    }
    _errorCheckerboardImage =
            create_image(pixels.data(), VkExtent3D{16, 16, 1},
                         VK_FORMAT_R8G8B8A8_UNORM, VK_IMAGE_USAGE_SAMPLED_BIT);

    VkSamplerCreateInfo sampl = {.sType =
                                         VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO};

    sampl.magFilter = VK_FILTER_NEAREST;
    sampl.minFilter = VK_FILTER_NEAREST;

    vkCreateSampler(_device, &sampl, nullptr, &_defaultSamplerNearest);

    sampl.magFilter = VK_FILTER_LINEAR;
    sampl.minFilter = VK_FILTER_LINEAR;
    vkCreateSampler(_device, &sampl, nullptr, &_defaultSamplerLinear);

    GLTFMetallic_Roughness::MaterialResources materialResources{};
    // default the material textures
    materialResources.colorImage = _whiteImage;
    materialResources.colorSampler = _defaultSamplerLinear;
    materialResources.metalRoughImage = _whiteImage;
    materialResources.metalRoughSampler = _defaultSamplerLinear;

    // set the uniform buffer for the material data
    AllocatedBuffer materialConstants = create_buffer(
            sizeof(GLTFMetallic_Roughness::MaterialConstants),
            VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, VMA_MEMORY_USAGE_CPU_TO_GPU);

    // write the buffer
    auto* sceneUniformData =
            (GLTFMetallic_Roughness::MaterialConstants*)
                    materialConstants.allocation->GetMappedData();
    sceneUniformData->colorFactors = glm::vec4{1, 1, 1, 1};
    sceneUniformData->metal_rough_factors = glm::vec4{1, 0.5, 0, 0};

    _mainDeletionQueue.push_function(
            [=, this]() { destroy_buffer(materialConstants); });

    materialResources.dataBuffer = materialConstants.buffer;
    materialResources.dataBufferOffset = 0;

    defaultData = metalRoughMaterial.write_material(
            _device, MaterialPass::MainColor, materialResources,
            globalDescriptorAllocator);
}

void VulkanEngine::init_mesh_pipeline() {
    VkShaderModule triangleFragShader;
    if (!vkutil::load_shader_module("./shaders/tex_image.frag.spv", _device,
                                    &triangleFragShader)) {
        fmt::println("Error when building the triangle fragment shader module");
    } else {
        fmt::println("Triangle fragment shader successfully loaded");
    }

    VkShaderModule triangleVertexShader;
    if (!vkutil::load_shader_module("./shaders/colored_triangle_mesh.vert.spv",
                                    _device, &triangleVertexShader)) {
        fmt::println("Error when building the triangle vertex shader module");
    } else {
        fmt::println("Triangle vertex shader successfully loaded");
    }

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
                                    &_meshPipelineLayout));

    PipelineBuilder pipelineBuilder;

    // use the triangle layout we created
    pipelineBuilder._pipelineLayout = _meshPipelineLayout;
    // connecting the vertex and pixel shaders to the pipeline
    pipelineBuilder.set_shaders(triangleVertexShader, triangleFragShader);
    // it will draw triangles
    pipelineBuilder.set_input_topology(VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST);
    // filled triangles
    pipelineBuilder.set_polygon_mode(VK_POLYGON_MODE_FILL);
    // no backface culling
    pipelineBuilder.set_cull_mode(VK_CULL_MODE_NONE, VK_FRONT_FACE_CLOCKWISE);
    // no multisampling
    pipelineBuilder.set_multisampling_none();
    // no blending
    pipelineBuilder.disable_blending();

    // pipelineBuilder.disable_depthtest();
    pipelineBuilder.enable_depthtest(true, VK_COMPARE_OP_GREATER);

    // connect the image format we will draw into, from draw image
    pipelineBuilder.set_color_attachment_format(_drawImage.imageFormat);
    pipelineBuilder.set_depth_format(VK_FORMAT_UNDEFINED);

    // finally build the pipeline
    _meshPipeline = pipelineBuilder.build_pipeline(_device);

    // clean structures
    vkDestroyShaderModule(_device, triangleFragShader, nullptr);
    vkDestroyShaderModule(_device, triangleVertexShader, nullptr);

    _mainDeletionQueue.push_function([&]() {
        vkDestroyPipelineLayout(_device, _meshPipelineLayout, nullptr);
        vkDestroyPipeline(_device, _meshPipeline, nullptr);
    });
}

void VulkanEngine::init_triangle_pipeline() {
    VkShaderModule triangleFragShader;
    if (!vkutil::load_shader_module("./shaders/colored_triangle.frag.spv",
                                    _device, &triangleFragShader)) {
        fmt::println("Error when building the triangle fragment shader module");
    } else {
        fmt::println("Triangle fragment shader successfully loaded");
    }

    VkShaderModule triangleVertexShader;
    if (!vkutil::load_shader_module("./shaders/colored_triangle.vert.spv",
                                    _device, &triangleVertexShader)) {
        fmt::println("Error when building the triangle vertex shader module");
    } else {
        fmt::println("Triangle vertex shader successfully loaded");
    }

    // build the pipeline layout that controls the inputs/outputs of the shader
    // we are not using descriptor sets or other systems yet, so no need to use
    // anything other than empty default
    VkPipelineLayoutCreateInfo pipeline_layout_info =
            vkinit::pipeline_layout_create_info();
    VK_CHECK(vkCreatePipelineLayout(_device, &pipeline_layout_info, nullptr,
                                    &_trianglePipelineLayout));

    PipelineBuilder pipelineBuilder;

    // use the triangle layout we created
    pipelineBuilder._pipelineLayout = _trianglePipelineLayout;
    // connecting the vertex and pixel shaders to the pipeline
    pipelineBuilder.set_shaders(triangleVertexShader, triangleFragShader);
    // it will draw triangles
    pipelineBuilder.set_input_topology(VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST);
    // filled triangles
    pipelineBuilder.set_polygon_mode(VK_POLYGON_MODE_FILL);
    // no backface culling
    pipelineBuilder.set_cull_mode(VK_CULL_MODE_NONE, VK_FRONT_FACE_CLOCKWISE);
    // no multisampling
    pipelineBuilder.set_multisampling_none();
    // no blending
    pipelineBuilder.disable_blending();
    // no depth testing
    pipelineBuilder.disable_depthtest();

    // connect the image format we will draw into, from draw image
    pipelineBuilder.set_color_attachment_format(_drawImage.imageFormat);
    pipelineBuilder.set_depth_format(VK_FORMAT_UNDEFINED);

    // finally build the pipeline
    _trianglePipeline = pipelineBuilder.build_pipeline(_device);

    // clean structures
    vkDestroyShaderModule(_device, triangleFragShader, nullptr);
    vkDestroyShaderModule(_device, triangleVertexShader, nullptr);

    _mainDeletionQueue.push_function([&]() {
        vkDestroyPipelineLayout(_device, _trianglePipelineLayout, nullptr);
        vkDestroyPipeline(_device, _trianglePipeline, nullptr);
    });
}

void VulkanEngine::init_imgui() {
    // 1: create descriptor pool for IMGUI
    //  the size of the pool is very oversize, but it's copied from imgui demo
    //  itself.
    VkDescriptorPoolSize pool_sizes[] = {
            {VK_DESCRIPTOR_TYPE_SAMPLER, 1000},
            {VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1000},
            {VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE, 1000},
            {VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, 1000},
            {VK_DESCRIPTOR_TYPE_UNIFORM_TEXEL_BUFFER, 1000},
            {VK_DESCRIPTOR_TYPE_STORAGE_TEXEL_BUFFER, 1000},
            {VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 1000},
            {VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 1000},
            {VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC, 1000},
            {VK_DESCRIPTOR_TYPE_STORAGE_BUFFER_DYNAMIC, 1000},
            {VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT, 1000}};

    VkDescriptorPoolCreateInfo pool_info = {};
    pool_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
    pool_info.flags = VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT;
    pool_info.maxSets = 1000;
    pool_info.poolSizeCount = (uint32_t)std::size(pool_sizes);
    pool_info.pPoolSizes = pool_sizes;

    VkDescriptorPool imguiPool;
    VK_CHECK(vkCreateDescriptorPool(_device, &pool_info, nullptr, &imguiPool));

    // 2: initialize imgui library

    // this initializes the core structures of imgui
    ImGui::CreateContext();

    // this initializes imgui for SDL
    ImGui_ImplSDL2_InitForVulkan(_window);

    // this initializes imgui for Vulkan
    ImGui_ImplVulkan_InitInfo init_info = {};
    init_info.Instance = _instance;
    init_info.PhysicalDevice = _chosenGPU;
    init_info.Device = _device;
    init_info.Queue = _graphicsQueue;
    init_info.DescriptorPool = imguiPool;
    init_info.MinImageCount = 3;
    init_info.ImageCount = 3;
    init_info.UseDynamicRendering = true;

    // dynamic rendering parameters for imgui to use
    init_info.PipelineRenderingCreateInfo = {
            .sType = VK_STRUCTURE_TYPE_PIPELINE_RENDERING_CREATE_INFO};
    init_info.PipelineRenderingCreateInfo.colorAttachmentCount = 1;
    init_info.PipelineRenderingCreateInfo.pColorAttachmentFormats =
            &_swapchainImageFormat;

    init_info.MSAASamples = VK_SAMPLE_COUNT_1_BIT;

    ImGui_ImplVulkan_Init(&init_info);

    ImGui_ImplVulkan_CreateFontsTexture();

    // add destroy the imgui created structures
    _mainDeletionQueue.push_function([=, this]() {
        ImGui_ImplVulkan_Shutdown();
        vkDestroyDescriptorPool(_device, imguiPool, nullptr);
    });
}

void VulkanEngine::init_descriptors() {
    // create a descriptor pool that will hold 10 sets with 1 image each
    std::vector<DescriptorAllocatorGrowable::PoolSizeRatio> sizes = {
            {VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, 1},
            {VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 1}};

    globalDescriptorAllocator.init(_device, 10, sizes);

    // make the descriptor set layout for our compute draw
    {
        DescriptorLayoutBuilder builder;
        builder.add_binding(0, VK_DESCRIPTOR_TYPE_STORAGE_IMAGE);
        _drawImageDescriptorLayout =
                builder.build(_device, VK_SHADER_STAGE_COMPUTE_BIT);
    }

    {
        DescriptorLayoutBuilder builder;
        builder.add_binding(0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER);
        _gpuSceneDataDescriptorLayout =
                builder.build(_device, VK_SHADER_STAGE_VERTEX_BIT |
                                               VK_SHADER_STAGE_FRAGMENT_BIT);
    }

    {
        DescriptorLayoutBuilder builder;
        builder.add_binding(0, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER);
        _singleImageDescriptorLayout =
                builder.build(_device, VK_SHADER_STAGE_FRAGMENT_BIT);
    }

    // allocate a descriptor set for our draw image
    _drawImageDescriptors = globalDescriptorAllocator.allocate(
            _device, _drawImageDescriptorLayout);

    DescriptorWriter writer;
    writer.write_image(0, _drawImage.imageView, VK_NULL_HANDLE,
                       VK_IMAGE_LAYOUT_GENERAL,
                       VK_DESCRIPTOR_TYPE_STORAGE_IMAGE);

    writer.update_set(_device, _drawImageDescriptors);

    for (auto & _frame : _frames) {
        // create a descriptor pool
        std::vector<DescriptorAllocatorGrowable::PoolSizeRatio> frame_sizes = {
                {VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, 3},
                {VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 3},
                {VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 3},
                {VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 4},
        };

        _frame._frameDescriptors = DescriptorAllocatorGrowable{};
        _frame._frameDescriptors.init(_device, 1000, frame_sizes);

        _mainDeletionQueue.push_function([&]() {
            _frame._frameDescriptors.destroy_pools(_device);
        });
    }
}

void VulkanEngine::init_background_pipelines() {
    VkPipelineLayoutCreateInfo computeLayout{};
    computeLayout.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
    computeLayout.pNext = nullptr;
    computeLayout.pSetLayouts = &_drawImageDescriptorLayout;
    computeLayout.setLayoutCount = 1;

    VK_CHECK(vkCreatePipelineLayout(_device, &computeLayout, nullptr,
                                    &_gradientPipelineLayout));

    VkShaderModule computeDrawShader;
    if (!vkutil::load_shader_module("./shaders/gradient.comp.spv", _device,
                                    &computeDrawShader)) {
        fmt::println("Error when building the compute shader \n");
    }

    VkPipelineShaderStageCreateInfo stageinfo{};
    stageinfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    stageinfo.pNext = nullptr;
    stageinfo.stage = VK_SHADER_STAGE_COMPUTE_BIT;
    stageinfo.module = computeDrawShader;
    stageinfo.pName = "main";

    VkComputePipelineCreateInfo computePipelineCreateInfo{};
    computePipelineCreateInfo.sType =
            VK_STRUCTURE_TYPE_COMPUTE_PIPELINE_CREATE_INFO;
    computePipelineCreateInfo.pNext = nullptr;
    computePipelineCreateInfo.layout = _gradientPipelineLayout;
    computePipelineCreateInfo.stage = stageinfo;

    VK_CHECK(vkCreateComputePipelines(_device, VK_NULL_HANDLE, 1,
                                      &computePipelineCreateInfo, nullptr,
                                      &_gradientPipeline));

    vkDestroyShaderModule(_device, computeDrawShader, nullptr);

    _mainDeletionQueue.push_function([&]() {
        vkDestroyPipelineLayout(_device, _gradientPipelineLayout, nullptr);
        vkDestroyPipeline(_device, _gradientPipeline, nullptr);
    });
}

void VulkanEngine::init_pipelines() {
    init_background_pipelines();

    init_triangle_pipeline();
    init_mesh_pipeline();

    metalRoughMaterial.build_pipelines(this);
}

void VulkanEngine::init(struct SDL_Window* window) {
    _window = window;

    // only one engine initialization is allowed with the application.
    assert(loadedEngine == nullptr);
    loadedEngine = this;

    init_vulkan();
    init_swapchain();
    init_commands();
    init_sync_structures();
    init_descriptors();
    init_pipelines();
    init_imgui();
    init_default_data();

    cameraController->setPosition(glm::vec3(0, 0, 5));

    glm::quat initialRotation = glm::quat(glm::vec3(0.0f, 0.0f, 0.0f)); // pitch, yaw, roll
    cameraController->setRotation(initialRotation);

    std::string structurePath = {std::string(ASSETS_DIR) + "/basicmesh.glb"};
    auto structureFile = loadGltf(this, structurePath);

    assert(structureFile.has_value());
    loadedScenes["structure"] = *structureFile;

    _isInitialized = true;
}

void VulkanEngine::init_vulkan() {
    auto system_info_ret = vkb::SystemInfo::get_system_info();
    if (!system_info_ret) {
        LOGE("Failed to retrieve system info. Error: {}",
             system_info_ret.error().message());
    }

    auto system_info = system_info_ret.value();

    LOGI("Available layers:")

    for (auto& layer : system_info.available_layers) {
        LOGI(layer.layerName);
    }

    LOGI("Available extensions:")

    for (auto& extension : system_info.available_extensions) {
        LOGI(extension.extensionName);
    }

    vkb::InstanceBuilder builder;

    auto inst_ret = builder.set_app_name("TODO: PUT APP NAME HERE")
                            .set_engine_name("rainsystem")
                            .request_validation_layers(bUseValidationLayers)
                            .set_debug_callback(debugCallback)
                            .require_api_version(1, 3, 0)
                            .build();

    if (!inst_ret) {
        LOGE("Failed to create Vulkan instance. Error: {}",
             inst_ret.error().message());
    }

    vkb::Instance vkb_inst = inst_ret.value();

    // grab the instance
    _instance = vkb_inst.instance;
    _debug_messenger = vkb_inst.debug_messenger;

    SDL_bool err = SDL_Vulkan_CreateSurface(_window, _instance, &_surface);
    if (!err) {
        LOGE("Failed to create Vulkan surface. Error: {}", SDL_GetError());
    }

    // vulkan 1.3 features
    VkPhysicalDeviceVulkan13Features features{};
    features.dynamicRendering = true;
    features.synchronization2 = true;

    // vulkan 1.2 features
    VkPhysicalDeviceVulkan12Features features12{};
    features12.bufferDeviceAddress = true;
    features12.descriptorIndexing = true;

    // use vkbootstrap to select a gpu.
    // We want a gpu that can write to the SDL surface and supports vulkan 1.3
    // with the correct features
    vkb::PhysicalDeviceSelector selector{vkb_inst};

    auto physical_device_ret = selector.set_minimum_version(1, 3)
                                       .set_required_features_13(features)
                                       .set_required_features_12(features12)
                                       .set_surface(_surface)
                                       .select();

    if (!physical_device_ret) {
        LOGE("Failed to select physical device. Error: {}",
             physical_device_ret.error().message());
    }

    vkb::PhysicalDevice physicalDevice = physical_device_ret.value();

    vkb::DeviceBuilder deviceBuilder{physicalDevice};

    auto dev_ret = deviceBuilder.build();
    if (!dev_ret) {
        LOGE("Failed to create logical device. Error: {}",
             dev_ret.error().message());
    }

    vkb::Device vkbDevice = dev_ret.value();

    // Get the VkDevice handle used in the rest of a vulkan application
    _device = vkbDevice.device;
    _chosenGPU = physicalDevice.physical_device;

    auto queue_ret = vkbDevice.get_queue(vkb::QueueType::graphics);
    if (!queue_ret) {
        LOGE("Failed to retrieve graphics queue. Error: {}",
             queue_ret.error().message());
    }

    _graphicsQueue = queue_ret.value();

    auto queue_family_ret = vkbDevice.get_queue_index(vkb::QueueType::graphics);
    if (!queue_family_ret) {
        LOGE("Failed to retrieve graphics queue family. Error: {}",
             queue_family_ret.error().message());
    }

    _graphicsQueueFamily = queue_family_ret.value();

    // initialize the memory allocator
    VmaAllocatorCreateInfo allocatorInfo = {};
    allocatorInfo.physicalDevice = _chosenGPU;
    allocatorInfo.device = _device;
    allocatorInfo.instance = _instance;
    allocatorInfo.flags = VMA_ALLOCATOR_CREATE_BUFFER_DEVICE_ADDRESS_BIT;
    vmaCreateAllocator(&allocatorInfo, &_allocator);

    _mainDeletionQueue.push_function(
            [&]() { vmaDestroyAllocator(_allocator); });
}

void VulkanEngine::init_commands() {
    // create a command pool for commands submitted to the graphics queue.
    // we also want the pool to allow for resetting of individual command
    // buffers
    VkCommandPoolCreateInfo commandPoolInfo = vkinit::command_pool_create_info(
            _graphicsQueueFamily,
            VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT);

    for (auto & _frame : _frames) {
        VK_CHECK(vkCreateCommandPool(_device, &commandPoolInfo, nullptr,
                                     &_frame._commandPool));

        // allocate the default command buffer that we will use for rendering
        VkCommandBufferAllocateInfo cmdAllocInfo =
                vkinit::command_buffer_allocate_info(_frame._commandPool,
                                                     1);

        VK_CHECK(vkAllocateCommandBuffers(_device, &cmdAllocInfo,
                                          &_frame._mainCommandBuffer));
    }

    VK_CHECK(vkCreateCommandPool(_device, &commandPoolInfo, nullptr,
                                 &_immCommandPool));

    // allocate the command buffer for immediate submits
    VkCommandBufferAllocateInfo cmdAllocInfo =
            vkinit::command_buffer_allocate_info(_immCommandPool, 1);

    VK_CHECK(vkAllocateCommandBuffers(_device, &cmdAllocInfo,
                                      &_immCommandBuffer));

    _mainDeletionQueue.push_function([=, this]() {
        vkDestroyCommandPool(_device, _immCommandPool, nullptr);
    });
}

void VulkanEngine::init_sync_structures() {
    VkFenceCreateInfo fenceCreateInfo =
            vkinit::fence_create_info(VK_FENCE_CREATE_SIGNALED_BIT);
    VkSemaphoreCreateInfo semaphoreCreateInfo = vkinit::semaphore_create_info();

    for (auto & _frame : _frames) {
        VK_CHECK(vkCreateFence(_device, &fenceCreateInfo, nullptr,
                               &_frame._renderFence));

        VK_CHECK(vkCreateSemaphore(_device, &semaphoreCreateInfo, nullptr,
                                   &_frame._swapchainSemaphore));
        VK_CHECK(vkCreateSemaphore(_device, &semaphoreCreateInfo, nullptr,
                                   &_frame._renderSemaphore));
    }

    VK_CHECK(vkCreateFence(_device, &fenceCreateInfo, nullptr, &_immFence));
    _mainDeletionQueue.push_function(
            [=, this]() { vkDestroyFence(_device, _immFence, nullptr); });
}

void VulkanEngine::create_swapchain(uint32_t width, uint32_t height) {
    vkb::SwapchainBuilder swapchainBuilder{_chosenGPU, _device, _surface};

    _swapchainImageFormat = VK_FORMAT_B8G8R8A8_UNORM;

    auto swap_ret =
            swapchainBuilder
                    //.use_default_format_selection()
                    .set_desired_format(VkSurfaceFormatKHR{
                            .format = _swapchainImageFormat,
                            .colorSpace = VK_COLOR_SPACE_SRGB_NONLINEAR_KHR})
                    // use vsync present mode
                    .set_desired_present_mode(VK_PRESENT_MODE_FIFO_KHR)
                    .set_desired_extent(width, height)
                    .add_image_usage_flags(VK_IMAGE_USAGE_TRANSFER_DST_BIT)
                    .build();

    if (!swap_ret) {
        LOGE("Failed to create swapchain. Error: {}",
             swap_ret.error().message());
    }

    vkb::Swapchain vkbSwapchain = swap_ret.value();

    _swapchainExtent = vkbSwapchain.extent;
    // store swapchain and its related images
    _swapchain = vkbSwapchain.swapchain;
    _swapchainImages = vkbSwapchain.get_images().value();
    _swapchainImageViews = vkbSwapchain.get_image_views().value();
}

void VulkanEngine::init_swapchain() {
    create_swapchain(_windowExtent.width, _windowExtent.height);

    // draw image size will match the window
    VkExtent3D drawImageExtent = {_windowExtent.width, _windowExtent.height, 1};

    // hardcoding the draw format to 32-bit float
    _drawImage.imageFormat = VK_FORMAT_R16G16B16A16_SFLOAT;
    _drawImage.imageExtent = drawImageExtent;

    VkImageUsageFlags drawImageUsages{};
    drawImageUsages |= VK_IMAGE_USAGE_TRANSFER_SRC_BIT;
    drawImageUsages |= VK_IMAGE_USAGE_TRANSFER_DST_BIT;
    drawImageUsages |= VK_IMAGE_USAGE_STORAGE_BIT;
    drawImageUsages |= VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;

    VkImageCreateInfo rimg_info = vkinit::image_create_info(
            _drawImage.imageFormat, drawImageUsages, drawImageExtent);

    // for the draw image, we want to allocate it from gpu local memory
    VmaAllocationCreateInfo rimg_allocinfo = {};
    rimg_allocinfo.usage = VMA_MEMORY_USAGE_GPU_ONLY;
    rimg_allocinfo.requiredFlags =
            VkMemoryPropertyFlags(VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);

    // allocate and create the image
    vmaCreateImage(_allocator, &rimg_info, &rimg_allocinfo, &_drawImage.image,
                   &_drawImage.allocation, nullptr);

    // build an image-view for the draw image to use for rendering
    VkImageViewCreateInfo rview_info = vkinit::imageview_create_info(
            _drawImage.imageFormat, _drawImage.image,
            VK_IMAGE_ASPECT_COLOR_BIT);

    VK_CHECK(vkCreateImageView(_device, &rview_info, nullptr,
                               &_drawImage.imageView));

    // add to deletion queues
    _mainDeletionQueue.push_function([=, this]() {
        vkDestroyImageView(_device, _drawImage.imageView, nullptr);
        vmaDestroyImage(_allocator, _drawImage.image, _drawImage.allocation);
    });
}

void VulkanEngine::destroy_swapchain() {
    vkDestroySwapchainKHR(_device, _swapchain, nullptr);

    // destroy swapchain resources
    for (auto & _swapchainImageView : _swapchainImageViews) {
        vkDestroyImageView(_device, _swapchainImageView, nullptr);
    }
}

void VulkanEngine::cleanup() {
    if (_isInitialized) {
        // make sure the gpu has stopped doing its things
        vkDeviceWaitIdle(_device);

        loadedScenes.clear();

        _mainDeletionQueue.flush();

        for (auto & _frame : _frames) {
            // already written from before
            vkDestroyCommandPool(_device, _frame._commandPool, nullptr);

            // destroy sync objects
            vkDestroyFence(_device, _frame._renderFence, nullptr);
            vkDestroySemaphore(_device, _frame._renderSemaphore, nullptr);
            vkDestroySemaphore(_device, _frame._swapchainSemaphore,
                               nullptr);
        }

        destroy_swapchain();

        vkDestroySurfaceKHR(_instance, _surface, nullptr);
        vkDestroyDevice(_device, nullptr);

        vkb::destroy_debug_utils_messenger(_instance, _debug_messenger);
        vkDestroyInstance(_instance, nullptr);
    }

    // clear engine pointer
    loadedEngine = nullptr;
}

AllocatedBuffer VulkanEngine::create_buffer(size_t allocSize,
                                            VkBufferUsageFlags usage,
                                            VmaMemoryUsage memoryUsage) const {
    // allocate buffer
    VkBufferCreateInfo bufferInfo = {
            .sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO};
    bufferInfo.pNext = nullptr;
    bufferInfo.size = allocSize;

    bufferInfo.usage = usage;

    VmaAllocationCreateInfo vmallocinfo = {};
    vmallocinfo.usage = memoryUsage;
    vmallocinfo.flags = VMA_ALLOCATION_CREATE_MAPPED_BIT;
    AllocatedBuffer newBuffer{};

    // allocate the buffer
    VK_CHECK(vmaCreateBuffer(_allocator, &bufferInfo, &vmallocinfo,
                             &newBuffer.buffer, &newBuffer.allocation,
                             &newBuffer.info));

    return newBuffer;
}

void VulkanEngine::destroy_buffer(const AllocatedBuffer& buffer) const {
    vmaDestroyBuffer(_allocator, buffer.buffer, buffer.allocation);
}

GPUMeshBuffers VulkanEngine::uploadMesh(std::span<uint32_t> indices,
                                        std::span<Vertex> vertices) {
    const size_t vertexBufferSize = vertices.size() * sizeof(Vertex);
    const size_t indexBufferSize = indices.size() * sizeof(uint32_t);

    GPUMeshBuffers newSurface{};

    // create vertex buffer
    newSurface.vertexBuffer =
            create_buffer(vertexBufferSize,
                          VK_BUFFER_USAGE_STORAGE_BUFFER_BIT |
                                  VK_BUFFER_USAGE_TRANSFER_DST_BIT |
                                  VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT,
                          VMA_MEMORY_USAGE_GPU_ONLY);

    // find the address of the vertex buffer
    VkBufferDeviceAddressInfo deviceAddressInfo{
            .sType = VK_STRUCTURE_TYPE_BUFFER_DEVICE_ADDRESS_INFO,
            .buffer = newSurface.vertexBuffer.buffer};
    newSurface.vertexBufferAddress =
            vkGetBufferDeviceAddress(_device, &deviceAddressInfo);

    // create index buffer
    newSurface.indexBuffer = create_buffer(
            indexBufferSize,
            VK_BUFFER_USAGE_INDEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT,
            VMA_MEMORY_USAGE_GPU_ONLY);

    AllocatedBuffer staging = create_buffer(vertexBufferSize + indexBufferSize,
                                            VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
                                            VMA_MEMORY_USAGE_CPU_ONLY);

    void* data = staging.allocation->GetMappedData();

    // copy vertex buffer
    memcpy(data, vertices.data(), vertexBufferSize);
    // copy index buffer
    memcpy((char*)data + vertexBufferSize, indices.data(), indexBufferSize);

    immediate_submit([&](VkCommandBuffer cmd) {
        VkBufferCopy vertexCopy{0};
        vertexCopy.dstOffset = 0;
        vertexCopy.srcOffset = 0;
        vertexCopy.size = vertexBufferSize;

        vkCmdCopyBuffer(cmd, staging.buffer, newSurface.vertexBuffer.buffer, 1,
                        &vertexCopy);

        VkBufferCopy indexCopy{0};
        indexCopy.dstOffset = 0;
        indexCopy.srcOffset = vertexBufferSize;
        indexCopy.size = indexBufferSize;

        vkCmdCopyBuffer(cmd, staging.buffer, newSurface.indexBuffer.buffer, 1,
                        &indexCopy);
    });

    destroy_buffer(staging);

    return newSurface;
}

void VulkanEngine::draw_background(VkCommandBuffer cmd) {
    // bind the gradient drawing compute pipeline
    vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_COMPUTE, _gradientPipeline);

    // bind the descriptor set containing the draw image for the compute
    // pipeline
    vkCmdBindDescriptorSets(cmd, VK_PIPELINE_BIND_POINT_COMPUTE,
                            _gradientPipelineLayout, 0, 1,
                            &_drawImageDescriptors, 0, nullptr);

    // execute the compute pipeline dispatch. We are using 16x16 workgroup size,
    // so we need to divide by it
    vkCmdDispatch(cmd, static_cast<uint32_t>(std::ceil(_drawExtent.width / 16.0)),
                  static_cast<uint32_t>(std::ceil(_drawExtent.height / 16.0)), 1);
}

void VulkanEngine::draw_imgui(VkCommandBuffer cmd,
                              VkImageView targetImageView) const {
    VkRenderingAttachmentInfo colorAttachment = vkinit::attachment_info(
            targetImageView, nullptr, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL);
    VkRenderingInfo renderInfo =
            vkinit::rendering_info(_swapchainExtent, &colorAttachment, nullptr);

    vkCmdBeginRendering(cmd, &renderInfo);

    ImGui_ImplVulkan_RenderDrawData(ImGui::GetDrawData(), cmd);

    vkCmdEndRendering(cmd);
}

void VulkanEngine::draw_geometry(VkCommandBuffer cmd) {
    // allocate a new uniform buffer for the scene data
    AllocatedBuffer gpuSceneDataBuffer = create_buffer(
            sizeof(GPUSceneData), VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
            VMA_MEMORY_USAGE_CPU_TO_GPU);

    // add it to the deletion queue of this frame, so it gets deleted once it's
    // been used
    get_current_frame()._deletionQueue.push_function(
            [=, this]() { destroy_buffer(gpuSceneDataBuffer); });

    // write the buffer
    auto* sceneUniformData =
            (GPUSceneData*)gpuSceneDataBuffer.allocation->GetMappedData();
    *sceneUniformData = sceneData;

    // create a descriptor set that binds that buffer and update it
    VkDescriptorSet globalDescriptor =
            get_current_frame()._frameDescriptors.allocate(
                    _device, _gpuSceneDataDescriptorLayout);

    DescriptorWriter writer;
    writer.write_buffer(0, gpuSceneDataBuffer.buffer, sizeof(GPUSceneData), 0,
                        VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER);
    writer.update_set(_device, globalDescriptor);

    VkRenderingAttachmentInfo colorAttachment = vkinit::attachment_info(
            _drawImage.imageView, nullptr, VK_IMAGE_LAYOUT_GENERAL);

    VkRenderingInfo renderInfo =
            vkinit::rendering_info(_drawExtent, &colorAttachment, nullptr);
    vkCmdBeginRendering(cmd, &renderInfo);

    vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, _trianglePipeline);

    // set dynamic viewport and scissor
    VkViewport viewport = {};
    viewport.x = 0;
    viewport.y = 0;
    viewport.width = static_cast<float>(_drawExtent.width);
    viewport.height = static_cast<float>(_drawExtent.height);
    viewport.minDepth = 0.f;
    viewport.maxDepth = 1.f;

    vkCmdSetViewport(cmd, 0, 1, &viewport);

    VkRect2D scissor = {};
    scissor.offset.x = 0;
    scissor.offset.y = 0;
    scissor.extent.width = _drawExtent.width;
    scissor.extent.height = _drawExtent.height;

    vkCmdSetScissor(cmd, 0, 1, &scissor);

    // bind a texture
    VkDescriptorSet imageSet = get_current_frame()._frameDescriptors.allocate(
            _device, _singleImageDescriptorLayout);
    DescriptorWriter single_image_writer;
    single_image_writer.write_image(0, _errorCheckerboardImage.imageView,
                                    _defaultSamplerNearest,
                                    VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
                                    VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER);
    single_image_writer.update_set(_device, imageSet);

    vkCmdBindDescriptorSets(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS,
                            _meshPipelineLayout, 0, 1, &imageSet, 0, nullptr);

    for (const RenderObject& draw : mainDrawContext.OpaqueSurfaces) {
        vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS,
                          draw.material->pipeline->pipeline);
        vkCmdBindDescriptorSets(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS,
                                draw.material->pipeline->layout, 0, 1,
                                &globalDescriptor, 0, nullptr);
        vkCmdBindDescriptorSets(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS,
                                draw.material->pipeline->layout, 1, 1,
                                &draw.material->materialSet, 0, nullptr);

        vkCmdBindIndexBuffer(cmd, draw.indexBuffer, 0, VK_INDEX_TYPE_UINT32);

        GPUDrawPushConstants pushConstants{};
        pushConstants.vertexBuffer = draw.vertexBufferAddress;
        pushConstants.worldMatrix = draw.transform;
        vkCmdPushConstants(cmd, draw.material->pipeline->layout,
                           VK_SHADER_STAGE_VERTEX_BIT, 0,
                           sizeof(GPUDrawPushConstants), &pushConstants);

        vkCmdDrawIndexed(cmd, draw.indexCount, 1, draw.firstIndex, 0, 0);
    }

    vkCmdEndRendering(cmd);
}

void VulkanEngine::draw() {
    update_scene();

    // wait until the gpu has finished rendering the last frame. Timeout of 1
    // second
    VK_CHECK(vkWaitForFences(_device, 1, &get_current_frame()._renderFence,
                             true, 1000000000));

    get_current_frame()._deletionQueue.flush();
    get_current_frame()._frameDescriptors.clear_pools(_device);

    VK_CHECK(vkResetFences(_device, 1, &get_current_frame()._renderFence));

    // request image from the swapchain
    uint32_t swapchainImageIndex;
    VkResult e = vkAcquireNextImageKHR(_device, _swapchain, 1000000000,
                                       get_current_frame()._swapchainSemaphore,
                                       nullptr, &swapchainImageIndex);
    if (e == VK_ERROR_OUT_OF_DATE_KHR) {
        resize_requested = true;
        return;
    }

    // naming it cmd for shorter writing
    VkCommandBuffer cmd = get_current_frame()._mainCommandBuffer;

    // now that we are sure that the commands finished executing, we can safely
    // reset the command buffer to begin recording again.
    VK_CHECK(vkResetCommandBuffer(cmd, 0));

    // begin the command buffer recording. We will use this command buffer
    // exactly once, so we want to let vulkan know that
    VkCommandBufferBeginInfo cmdBeginInfo = vkinit::command_buffer_begin_info(
            VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT);

    _drawExtent.height = static_cast<uint32_t>(
            (float)std::min(_swapchainExtent.height,
                            _drawImage.imageExtent.height) *
            renderScale);
    _drawExtent.width = static_cast<uint32_t>(
            (float)std::min(_swapchainExtent.width,
                            _drawImage.imageExtent.width) *
            renderScale);

    VK_CHECK(vkBeginCommandBuffer(cmd, &cmdBeginInfo));

    // transition our main draw image into general layout, so we can write into
    // it, we will overwrite it all, so we don't care about what was the older
    // layout
    vkutil::transition_image(cmd, _drawImage.image, VK_IMAGE_LAYOUT_UNDEFINED,
                             VK_IMAGE_LAYOUT_GENERAL);

    draw_background(cmd);

    vkutil::transition_image(cmd, _drawImage.image, VK_IMAGE_LAYOUT_GENERAL,
                             VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL);

    draw_geometry(cmd);

    // transition the draw image and the swapchain image into their correct
    // transfer layouts
    vkutil::transition_image(cmd, _drawImage.image,
                             VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
                             VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL);
    vkutil::transition_image(cmd, _swapchainImages[swapchainImageIndex],
                             VK_IMAGE_LAYOUT_UNDEFINED,
                             VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);

    // execute a copy from the draw image into the swapchain
    vkutil::copy_image_to_image(cmd, _drawImage.image,
                                _swapchainImages[swapchainImageIndex],
                                _drawExtent, _swapchainExtent);

    // set swapchain image layout to Present, so we can show it on the screen
    vkutil::transition_image(cmd, _swapchainImages[swapchainImageIndex],
                             VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
                             VK_IMAGE_LAYOUT_PRESENT_SRC_KHR);

    // draw imgui into the swapchain image
    draw_imgui(cmd, _swapchainImageViews[swapchainImageIndex]);

    // set swapchain image layout to Present, so we can draw it
    // vkutil::transition_image(cmd, _swapchainImages[swapchainImageIndex],
    // VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
    // VK_IMAGE_LAYOUT_PRESENT_SRC_KHR);

    // finalize the command buffer (we can no longer add commands, but it can
    // now be executed)
    VK_CHECK(vkEndCommandBuffer(cmd));

    // prepare the submission to the queue.
    // we want to wait on the _presentSemaphore, as that semaphore is signaled
    // when the swapchain is ready we will signal the _renderSemaphore, to
    // signal that rendering has finished

    VkCommandBufferSubmitInfo cmdinfo = vkinit::command_buffer_submit_info(cmd);

    VkSemaphoreSubmitInfo waitInfo = vkinit::semaphore_submit_info(
            VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT_KHR,
            get_current_frame()._swapchainSemaphore);
    VkSemaphoreSubmitInfo signalInfo =
            vkinit::semaphore_submit_info(VK_PIPELINE_STAGE_2_ALL_GRAPHICS_BIT,
                                          get_current_frame()._renderSemaphore);

    VkSubmitInfo2 submit =
            vkinit::submit_info(&cmdinfo, &signalInfo, &waitInfo);

    // submit command buffer to the queue and execute it.
    //  _renderFence will now block until the graphic commands finish execution
    VK_CHECK(vkQueueSubmit2(_graphicsQueue, 1, &submit,
                            get_current_frame()._renderFence));

    // prepare present
    //  this will put the image we just rendered to into the visible window.
    //  we want to wait on the _renderSemaphore for that,
    //  as its necessary that drawing commands have finished before the image is
    //  displayed to the user
    VkPresentInfoKHR presentInfo = {};
    presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
    presentInfo.pNext = nullptr;
    presentInfo.pSwapchains = &_swapchain;
    presentInfo.swapchainCount = 1;

    presentInfo.pWaitSemaphores = &get_current_frame()._renderSemaphore;
    presentInfo.waitSemaphoreCount = 1;

    presentInfo.pImageIndices = &swapchainImageIndex;

    VkResult presentResult = vkQueuePresentKHR(_graphicsQueue, &presentInfo);
    if (presentResult == VK_ERROR_OUT_OF_DATE_KHR) {
        resize_requested = true;
    }

    // increase the number of frames drawn
    _frameNumber++;
}

void VulkanEngine::resize_swapchain() {
    vkDeviceWaitIdle(_device);

    destroy_swapchain();

    int w, h;
    SDL_GetWindowSize(_window, &w, &h);
    _windowExtent.width = static_cast<uint32_t>(w);
    _windowExtent.height = static_cast<uint32_t>(h);

    create_swapchain(_windowExtent.width, _windowExtent.height);

    resize_requested = false;
}

void VulkanEngine::immediate_submit(
        std::function<void(VkCommandBuffer cmd)>&& function) {
    VK_CHECK(vkResetFences(_device, 1, &_immFence));
    VK_CHECK(vkResetCommandBuffer(_immCommandBuffer, 0));

    VkCommandBuffer cmd = _immCommandBuffer;

    VkCommandBufferBeginInfo cmdBeginInfo = vkinit::command_buffer_begin_info(
            VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT);

    VK_CHECK(vkBeginCommandBuffer(cmd, &cmdBeginInfo));

    function(cmd);

    VK_CHECK(vkEndCommandBuffer(cmd));

    VkCommandBufferSubmitInfo cmdinfo = vkinit::command_buffer_submit_info(cmd);
    VkSubmitInfo2 submit = vkinit::submit_info(&cmdinfo, nullptr, nullptr);

    // submit command buffer to the queue and execute it.
    //  _renderFence will now block until the graphic commands finish execution
    VK_CHECK(vkQueueSubmit2(_graphicsQueue, 1, &submit, _immFence));

    VK_CHECK(vkWaitForFences(_device, 1, &_immFence, true, 9999999999));
}

void VulkanEngine::update() {
    if (resize_requested) {
        resize_swapchain();
    }

    draw();
}

AllocatedImage VulkanEngine::create_image(VkExtent3D size, VkFormat format,
                                          VkImageUsageFlags usage,
                                          bool mipmapped) const {
    AllocatedImage newImage{};
    newImage.imageFormat = format;
    newImage.imageExtent = size;

    VkImageCreateInfo img_info = vkinit::image_create_info(format, usage, size);
    if (mipmapped) {
        img_info.mipLevels = static_cast<uint32_t>(std::floor(std::log2(
                                     std::max(size.width, size.height)))) +
                             1;
    }

    // always allocate images on dedicated GPU memory
    VmaAllocationCreateInfo allocinfo = {};
    allocinfo.usage = VMA_MEMORY_USAGE_GPU_ONLY;
    allocinfo.requiredFlags =
            VkMemoryPropertyFlags(VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);

    // allocate and create the image
    VK_CHECK(vmaCreateImage(_allocator, &img_info, &allocinfo, &newImage.image,
                            &newImage.allocation, nullptr));

    // if the format is a depth format, we will need to have it use the correct
    // aspect flag
    VkImageAspectFlags aspectFlag = VK_IMAGE_ASPECT_COLOR_BIT;
    if (format == VK_FORMAT_D32_SFLOAT) {
        aspectFlag = VK_IMAGE_ASPECT_DEPTH_BIT;
    }

    // build an image-view for the image
    VkImageViewCreateInfo view_info =
            vkinit::imageview_create_info(format, newImage.image, aspectFlag);
    view_info.subresourceRange.levelCount = img_info.mipLevels;

    VK_CHECK(vkCreateImageView(_device, &view_info, nullptr,
                               &newImage.imageView));

    return newImage;
}

AllocatedImage VulkanEngine::create_image(void* data, VkExtent3D size,
                                          VkFormat format,
                                          VkImageUsageFlags usage,
                                          bool mipmapped) {
    size_t data_size = size.depth * size.width * size.height * 4;
    AllocatedBuffer uploadbuffer =
            create_buffer(data_size, VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
                          VMA_MEMORY_USAGE_CPU_TO_GPU);

    memcpy(uploadbuffer.info.pMappedData, data, data_size);

    AllocatedImage new_image =
            create_image(size, format,
                         usage | VK_IMAGE_USAGE_TRANSFER_DST_BIT |
                                 VK_IMAGE_USAGE_TRANSFER_SRC_BIT,
                         mipmapped);

    immediate_submit([&](VkCommandBuffer cmd) {
        vkutil::transition_image(cmd, new_image.image,
                                 VK_IMAGE_LAYOUT_UNDEFINED,
                                 VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);

        VkBufferImageCopy copyRegion = {};
        copyRegion.bufferOffset = 0;
        copyRegion.bufferRowLength = 0;
        copyRegion.bufferImageHeight = 0;

        copyRegion.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        copyRegion.imageSubresource.mipLevel = 0;
        copyRegion.imageSubresource.baseArrayLayer = 0;
        copyRegion.imageSubresource.layerCount = 1;
        copyRegion.imageExtent = size;

        // copy the buffer into the image
        vkCmdCopyBufferToImage(cmd, uploadbuffer.buffer, new_image.image,
                               VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1,
                               &copyRegion);

        vkutil::transition_image(cmd, new_image.image,
                                 VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
                                 VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
    });

    destroy_buffer(uploadbuffer);

    return new_image;
}

void VulkanEngine::destroy_image(const AllocatedImage& img) const {
    vkDestroyImageView(_device, img.imageView, nullptr);
    vmaDestroyImage(_allocator, img.image, img.allocation);
}

void GLTFMetallic_Roughness::build_pipelines(VulkanEngine* engine) {
    VkShaderModule meshFragShader;
    if (!vkutil::load_shader_module("./shaders/mesh.frag.spv", engine->_device,
                                    &meshFragShader)) {
        fmt::println("Error when building the triangle fragment shader module");
    }

    VkShaderModule meshVertexShader;
    if (!vkutil::load_shader_module("./shaders/mesh.vert.spv", engine->_device,
                                    &meshVertexShader)) {
        fmt::println("Error when building the triangle vertex shader module");
    }

    VkPushConstantRange matrixRange{};
    matrixRange.offset = 0;
    matrixRange.size = sizeof(GPUDrawPushConstants);
    matrixRange.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;

    DescriptorLayoutBuilder layoutBuilder;
    layoutBuilder.add_binding(0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER);
    layoutBuilder.add_binding(1, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER);
    layoutBuilder.add_binding(2, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER);

    materialLayout = layoutBuilder.build(
            engine->_device,
            VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT);

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

    opaquePipeline.layout = newLayout;
    transparentPipeline.layout = newLayout;

    // build the stage-create-info for both vertex and fragment stages. This
    // lets the pipeline know the shader modules per stage
    PipelineBuilder pipelineBuilder;
    pipelineBuilder.set_shaders(meshVertexShader, meshFragShader);
    pipelineBuilder.set_input_topology(VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST);
    pipelineBuilder.set_polygon_mode(VK_POLYGON_MODE_FILL);
    pipelineBuilder.set_cull_mode(VK_CULL_MODE_NONE, VK_FRONT_FACE_CLOCKWISE);
    pipelineBuilder.set_multisampling_none();
    pipelineBuilder.disable_blending();
    pipelineBuilder.enable_depthtest(true, VK_COMPARE_OP_GREATER_OR_EQUAL);

    // render format
    pipelineBuilder.set_color_attachment_format(engine->_drawImage.imageFormat);
    pipelineBuilder.set_depth_format(engine->_depthImage.imageFormat);

    // use the triangle layout we created
    pipelineBuilder._pipelineLayout = newLayout;

    // finally build the pipeline
    opaquePipeline.pipeline = pipelineBuilder.build_pipeline(engine->_device);

    // create the transparent variant
    pipelineBuilder.enable_blending_additive();

    pipelineBuilder.enable_depthtest(false, VK_COMPARE_OP_GREATER_OR_EQUAL);

    transparentPipeline.pipeline =
            pipelineBuilder.build_pipeline(engine->_device);

    vkDestroyShaderModule(engine->_device, meshFragShader, nullptr);
    vkDestroyShaderModule(engine->_device, meshVertexShader, nullptr);
}

MaterialInstance GLTFMetallic_Roughness::write_material(
        VkDevice device, MaterialPass pass, const MaterialResources& resources,
        DescriptorAllocatorGrowable& descriptorAllocator) {
    MaterialInstance matData{};
    matData.passType = pass;
    if (pass == MaterialPass::Transparent) {
        matData.pipeline = &transparentPipeline;
    } else {
        matData.pipeline = &opaquePipeline;
    }

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

void MeshNode::Draw(const glm::mat4& topMatrix, DrawContext& ctx) {
    glm::mat4 nodeMatrix = topMatrix * worldTransform;

    for (auto& s : mesh->surfaces) {
        RenderObject def{};
        def.indexCount = s.count;
        def.firstIndex = s.startIndex;
        def.indexBuffer = mesh->meshBuffers.indexBuffer.buffer;
        def.material = &s.material->data;

        def.transform = nodeMatrix;
        def.vertexBufferAddress = mesh->meshBuffers.vertexBufferAddress;

        ctx.OpaqueSurfaces.push_back(def);
    }

    ENode::Draw(topMatrix, ctx);
}

void VulkanEngine::update_scene() {

    glm::mat4 view = mainCamera->getViewMatrix();

    glm::mat4 projection = glm::perspective(
            glm::radians(70.f),
            (float)_windowExtent.width / (float)_windowExtent.height, 0.1f,
            10000.f);

    // to opengl and gltf axis
    projection[1][1] *= -1;

    sceneData.view = view;
    sceneData.proj = projection;
    sceneData.viewproj = projection * view;

    mainDrawContext.OpaqueSurfaces.clear();

    sceneData.ambientColor = glm::vec4(.1f);
    sceneData.sunlightColor = glm::vec4(1.f);
    sceneData.sunlightDirection = glm::vec4(0, 1, 0.5, 1.f);

    for (const auto& [key, mesh] : meshes) {
        std::shared_ptr<LoadedGLTF> loadedMesh = mesh;
        loadedMesh->Draw(transforms[key], mainDrawContext);
    }
}

int64_t VulkanEngine::registerMesh(std::string filePath) {
    std::random_device rd;

    // Use the Mersenne Twister engine for high-quality random numbers
    std::mt19937_64 generator(rd());

    // Create a uniform distribution for int64_t
    std::uniform_int_distribution<int64_t> distribution;

    // Generate and print a random int64_t value
    int64_t random_int64 = distribution(generator);

    std::string structurePath = {std::string(ASSETS_DIR) + filePath};
    auto structureFile = loadGltf(this, structurePath);

    assert(structureFile.has_value());

    meshes[random_int64] = *structureFile;
    transforms[random_int64] = glm::mat4(1.0f);

    return random_int64;
}

void VulkanEngine::unregisterMesh(int64_t id) {
    meshes.erase(id);
    transforms.erase(id);
}

void VulkanEngine::setMeshTransform(int64_t id, glm::mat4 mat) {
    transforms[id] = mat;
}
