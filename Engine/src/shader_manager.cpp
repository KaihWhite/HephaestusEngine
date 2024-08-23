#pragma once

#include "../headers/shader_manager.h"
#include <fstream>
#include <vector>
#include <vulkan/vulkan.h>


ShaderManager::ShaderManager(VkDevice &logicalDevice) {
	this->logicalDevice = logicalDevice;
}

VkShaderModule ShaderManager::createShaderModule(const std::vector<char>& code) {
	VkShaderModuleCreateInfo createInfo{};
	createInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
	createInfo.codeSize = code.size();
	createInfo.pCode = reinterpret_cast<const uint32_t*>(code.data());

	VkShaderModule shaderModule;
	if (vkCreateShaderModule(logicalDevice, &createInfo, nullptr, &shaderModule) != VK_SUCCESS) {
		throw std::runtime_error("failed to create shader module!");
	}

	return shaderModule;
}

std::vector<char> ShaderManager::readFile(const std::string& filename) {
	std::ifstream file(filename, std::ios::ate | std::ios::binary); // check two flags: ate - start reading at the end of the file, binary - read the file as binary
	if (!file.is_open()) {
		throw std::runtime_error("failed to open file!");
	}

	size_t fileSize = (size_t)file.tellg(); // get the size of the file from the current position of the file pointer, which is at the end.
	std::vector<char> buffer(fileSize); // create a buffer of the size of the file

	file.seekg(0); // move the file pointer to the beginning of the file
	file.read(buffer.data(), fileSize); // read the file into the buffer

	file.close();

	return buffer;
}