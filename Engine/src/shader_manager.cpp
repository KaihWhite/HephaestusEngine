#pragma

#include <fstream>
#include <vector>
#include <vulkan/vulkan.h>

class ShaderManager {

	public:

	ShaderManager::ShaderManager(VkDevice &device) {
		this->device = device;
	}

	void ShaderManager::createGraphicsPipeline() {
		auto vertShaderCode = readFile("shaders/vert.spv");
		auto fragShaderCode = readFile("shaders/frag.spv");

		VkShaderModule vertShaderModule = createShaderModule(vertShaderCode);
		VkShaderModule fragShaderModule = createShaderModule(fragShaderCode);

		vkDestroyShaderModule(device, fragShaderModule, nullptr);
		vkDestroyShaderModule(device, vertShaderModule, nullptr);

		// vertex shader stage info
		VkPipelineShaderStageCreateInfo vertShaderStageInfo{};
		vertShaderStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO; // struct type
		vertShaderStageInfo.stage = VK_SHADER_STAGE_VERTEX_BIT; // shader stage
		vertShaderStageInfo.module = vertShaderModule; // shader module
		vertShaderStageInfo.pName = "main"; // entry point - main function

		// fragment shader stage info
		VkPipelineShaderStageCreateInfo fragShaderStageInfo{};
		fragShaderStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
		fragShaderStageInfo.stage = VK_SHADER_STAGE_FRAGMENT_BIT;
		fragShaderStageInfo.module = fragShaderModule;
		fragShaderStageInfo.pName = "main";

		VkPipelineShaderStageCreateInfo shaderStages[] = { vertShaderStageInfo, fragShaderStageInfo }; // 
	}

	private:

	VkDevice device;

	VkShaderModule ShaderManager::createShaderModule(const std::vector<char>& code) {
		VkShaderModuleCreateInfo createInfo{};
		createInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
		createInfo.codeSize = code.size();
		createInfo.pCode = reinterpret_cast<const uint32_t*>(code.data());

		VkShaderModule shaderModule;
		if (vkCreateShaderModule(device, &createInfo, nullptr, &shaderModule) != VK_SUCCESS) {
			throw std::runtime_error("failed to create shader module!");
		}

		return shaderModule;
	}

	static std::vector<char> ShaderManager::readFile(const std::string& filename) {
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
};