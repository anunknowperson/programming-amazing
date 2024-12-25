#include "view/Shader.h"

#include <fmt/core.h>
#include <fstream>
#include <vector>

// Конструктор, принимает путь к файлу шейдера и устройство Vulkan
Shader::Shader(const std::filesystem::path& shaderPath, VkDevice device)
    : _device(device), _shaderModule(VK_NULL_HANDLE) {
    if (!loadShaderModule(shaderPath, &_shaderModule)) {
        fmt::print("Ошибка при создании шейдера из {}\n", shaderPath.string());
    }
}

// Загрузка данных шейдера и создание VkShaderModule
bool Shader::loadShaderModule(const std::filesystem::path& filePath,
                              VkShaderModule* outShaderModule) {
    std::ifstream file(filePath, std::ios::ate | std::ios::binary);
    if (!file.is_open()) {
        fmt::print("Не удалось открыть файл шейдера {}\n", filePath.string());
        return false;
    }

    const auto fileSize = file.tellg();
    std::vector<char> buffer(static_cast<size_t>(fileSize));

    file.seekg(0);
    file.read(buffer.data(), fileSize);
    file.close();

    VkShaderModuleCreateInfo createInfo{
            .sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO,
            .codeSize = buffer.size(),
            .pCode = reinterpret_cast<const uint32_t*>(buffer.data())
    };

    if (vkCreateShaderModule(_device, &createInfo, nullptr, outShaderModule) !=
        VK_SUCCESS) {
        fmt::print("Не удалось создать модуль шейдера\n");
        return false;
    }

    return true;
}

// Получение созданного модуля шейдера
VkShaderModule Shader::getShaderModule() const {
    return _shaderModule;
}

// Деструктор для освобождения ресурсов
Shader::~Shader() {
    if (_shaderModule != VK_NULL_HANDLE) {
        vkDestroyShaderModule(_device, _shaderModule, nullptr);
    }
}
