#include <filesystem>
#include <gtest/gtest.h>
#include <vulkan/vulkan.h>

#include "view/Shader.h"

class ShaderTest : public ::testing::Test {
protected:
    VkInstance instance;
    VkDevice device;
    Shader* shader;

    void SetUp() override {
        // Создание Vulkan instance
        VkApplicationInfo appInfo{};
        appInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
        appInfo.pApplicationName = "ShaderTest";
        appInfo.apiVersion = VK_API_VERSION_1_0;

        VkInstanceCreateInfo createInfo{};
        createInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
        createInfo.pApplicationInfo = &appInfo;

        if (vkCreateInstance(&createInfo, nullptr, &instance) != VK_SUCCESS) {
            FAIL() << "Не удалось создать Vulkan instance";
        }

        // Получение физического устройства и создание логического устройства
        uint32_t deviceCount = 0;
        vkEnumeratePhysicalDevices(instance, &deviceCount, nullptr);
        if (deviceCount == 0) {
            FAIL() << "Не найдено устройств с поддержкой Vulkan";
        }

        std::vector<VkPhysicalDevice> devices(deviceCount);
        vkEnumeratePhysicalDevices(instance, &deviceCount, devices.data());

        VkDeviceCreateInfo deviceCreateInfo{};
        deviceCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;

        if (vkCreateDevice(devices[0], &deviceCreateInfo, nullptr, &device) !=
            VK_SUCCESS) {
            FAIL() << "Не удалось создать логическое устройство Vulkan";
        }

        // Определение пути к файлу шейдера
        std::filesystem::path shaderPath = std::filesystem::current_path()
                                                   .parent_path()
                                                   .parent_path()
                                                   .parent_path()
                                                   .parent_path()
                                                   .parent_path() /
                                           "shaders" /
                                           "colored_triangle.frag.spv";
        if (!std::filesystem::exists(shaderPath)) {
            FAIL() << "Файл шейдера " << shaderPath << " не найден";
        }

        shader = new Shader(shaderPath.string(), device);
    }

    void TearDown() override {
        delete shader;
        vkDestroyDevice(device, nullptr);
        vkDestroyInstance(instance, nullptr);
    }
};

TEST_F(ShaderTest, ShaderCreation) {
    // Проверка успешного создания модуля шейдера
    EXPECT_NE(shader->getShaderModule(), VK_NULL_HANDLE);
}

TEST_F(ShaderTest, MultipleShaderCreationAndDestruction) {
    const int shaderCount = 10;
    std::vector<Shader*> shaders;

    std::filesystem::path shaderPath = std::filesystem::current_path()
                                               .parent_path()
                                               .parent_path()
                                               .parent_path()
                                               .parent_path()
                                               .parent_path() /
                                       "shaders" / "colored_triangle.frag.spv";
    ASSERT_TRUE(std::filesystem::exists(shaderPath))
            << "Файл шейдера " << shaderPath << " не найден";

    // Создание нескольких шейдеров
    for (int i = 0; i < shaderCount; ++i) {
        shaders.push_back(new Shader(shaderPath.string(), device));
        EXPECT_NE(shaders.back()->getShaderModule(), VK_NULL_HANDLE);
    }

    // Удаление созданных шейдеров
    for (auto* shader : shaders) {
        delete shader;
    }
}
