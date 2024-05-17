﻿// vulkan_guide.h : Include file for standard system include files,
// or project specific include files.

#pragma once

#include "vk_types.h"
#include "vk_descriptors.h"
#include "vk_pipelines.h"
#include "vk_loader.h"


constexpr unsigned int FRAME_OVERLAP = 2;

struct DeletionQueue
{
    std::deque<std::function<void()>> deletors;

    void push_function(std::function<void()>&& function) {
        deletors.push_back(function);
    }

    void flush() {
        // reverse iterate the deletion queue to execute all the functions
        for (auto it = deletors.rbegin(); it != deletors.rend(); it++) {
            (*it)(); //call functors
        }

        deletors.clear();
    }
};

struct FrameData {

	VkCommandPool _commandPool;
	VkCommandBuffer _mainCommandBuffer;

	VkSemaphore _swapchainSemaphore, _renderSemaphore;
	VkFence _renderFence;

    DeletionQueue _deletionQueue;
    DescriptorAllocatorGrowable _frameDescriptors;
};

struct GPUSceneData {
    glm::mat4 view;
    glm::mat4 proj;
    glm::mat4 viewproj;
    glm::vec4 ambientColor;
    glm::vec4 sunlightDirection; // w for sun power
    glm::vec4 sunlightColor;
};

class VulkanEngine {
public:
	FrameData _frames[FRAME_OVERLAP];

	FrameData& get_current_frame() { return _frames[_frameNumber % FRAME_OVERLAP]; };

	VkQueue _graphicsQueue;
	uint32_t _graphicsQueueFamily;

	bool _isInitialized{ false };
	int _frameNumber {0};
	bool stop_rendering{ false };
	VkExtent2D _windowExtent{ 2560 , 1440 };

	struct SDL_Window* _window{ nullptr };

	static VulkanEngine& Get();

	//initializes everything in the engine
	void init(struct SDL_Window* window);

	//shuts down the engine
	void cleanup();

	//draw loop
	void draw();

	//run main loop
	void update();


	VkInstance _instance;// Vulkan library handle
	VkDebugUtilsMessengerEXT _debug_messenger;// Vulkan debug output handle
	VkPhysicalDevice _chosenGPU;// GPU chosen as the default device
	VkDevice _device; // Vulkan device for commands
	VkSurfaceKHR _surface;// Vulkan window surface

	VkSwapchainKHR _swapchain;
	VkFormat _swapchainImageFormat;

	std::vector<VkImage> _swapchainImages;
	std::vector<VkImageView> _swapchainImageViews;
	VkExtent2D _swapchainExtent;

    DeletionQueue _mainDeletionQueue;

    VmaAllocator _allocator;

    AllocatedImage _drawImage;
    AllocatedImage _depthImage;
    VkExtent2D _drawExtent;
    float renderScale = 1.f;

    DescriptorAllocator globalDescriptorAllocator;

    VkDescriptorSet _drawImageDescriptors;
    VkDescriptorSetLayout _drawImageDescriptorLayout;

    VkPipeline _gradientPipeline;
    VkPipelineLayout _gradientPipelineLayout;

    // immediate submit structures
            VkFence _immFence;
    VkCommandBuffer _immCommandBuffer;
    VkCommandPool _immCommandPool;

    VkPipelineLayout _trianglePipelineLayout;
    VkPipeline _trianglePipeline;

    VkPipelineLayout _meshPipelineLayout;
    VkPipeline _meshPipeline;

    GPUMeshBuffers rectangle;


    void immediate_submit(std::function<void(VkCommandBuffer cmd)>&& function);
    GPUMeshBuffers uploadMesh(std::span<uint32_t> indices, std::span<Vertex> vertices);

    std::vector<std::shared_ptr<MeshAsset>> testMeshes;

    bool resize_requested;

    GPUSceneData sceneData;

    VkDescriptorSetLayout _gpuSceneDataDescriptorLayout;


    AllocatedImage create_image(VkExtent3D size, VkFormat format, VkImageUsageFlags usage, bool mipmapped = false);
    AllocatedImage create_image(void* data, VkExtent3D size, VkFormat format, VkImageUsageFlags usage, bool mipmapped = false);
    void destroy_image(const AllocatedImage& img);

    AllocatedImage _whiteImage;
    AllocatedImage _blackImage;
    AllocatedImage _greyImage;
    AllocatedImage _errorCheckerboardImage;

    VkSampler _defaultSamplerLinear;
    VkSampler _defaultSamplerNearest;

    VkDescriptorSetLayout _singleImageDescriptorLayout;

private:

	static VKAPI_ATTR VkBool32 VKAPI_CALL debugCallback(
		VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
		VkDebugUtilsMessageTypeFlagsEXT messageType,
		const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData,
		void* pUserData);

	void init_vulkan();
	void init_swapchain();
	void init_commands();
	void init_sync_structures();

	void create_swapchain(uint32_t width, uint32_t height);
	void destroy_swapchain();


    void draw_background(VkCommandBuffer cmd);

    void init_descriptors();

    void init_pipelines();
    void init_background_pipelines();
    void init_imgui();

    void init_triangle_pipeline();

    void draw_imgui(VkCommandBuffer cmd, VkImageView targetImageView);


    void draw_geometry(VkCommandBuffer cmd);

    AllocatedBuffer create_buffer(size_t allocSize, VkBufferUsageFlags usage, VmaMemoryUsage memoryUsage);

    void destroy_buffer(const AllocatedBuffer &buffer);

    void resize_swapchain();

    void init_mesh_pipeline();

    void init_default_data();
};