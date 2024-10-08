#pragma once
#ifndef SHADER_MANAGER_H
#define SHADER_MANAGER_H

#include <fstream>
#include <vector>
#include <vulkan/vulkan.h>

class ShaderManager {

private:

    VkDevice logicalDevice;

public:

    ShaderManager(VkDevice &logicalDevice);

    VkShaderModule createShaderModule(const std::vector<char>& code);

    static std::vector<char> readFile(const std::string& filename);

};

#endif // SHADER_MANAGER_H