#pragma once

#include <filesystem>
#include <vulkan/vulkan.h>

// Класс Shader для загрузки, хранения и управления шейдерами в Vulkan
class Shader {
public:
    // Конструктор, принимает путь к файлу шейдера и устройство Vulkan
    Shader(const std::filesystem::path& shaderPath, VkDevice device);

    // Метод для получения Vulkan-модуля шейдера
    [[nodiscard]] VkShaderModule getShaderModule() const;

    // Деструктор для очистки ресурсов
    ~Shader();

private:
    // Метод для загрузки данных шейдера и создания модуля шейдера
    bool loadShaderModule(const std::filesystem::path& filePath,
                          VkShaderModule* outShaderModule);

    VkDevice _device;  // Устройство Vulkan, необходимое для создания шейдеров
    VkShaderModule _shaderModule;  // VkShaderModule, содержащий данные шейдера
};
