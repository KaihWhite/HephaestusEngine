#pragma once

#include "../headers/engine.h"

class Engine {
public:
	// Window size
	const uint32_t WIDTH = 800;
	const uint32_t HEIGHT = 600;

	// TODO: Validation layer does not currently exist. Must fix this on my system before debugging will work
	// Validation layers - used for debugging
	const std::vector<const char*> validationLayers = {
	"VK_LAYER_KHRONOS_validation"
	};

	// If the program is in debug mode, enable validation layers
	#ifdef NDEBUG
		const bool enableValidationLayers = false;
	#else
		const bool enableValidationLayers = true;
	#endif

	// initialize the window and vulkan to start the engine
    void run() {
		initWindow();
		initVulkan();
		mainLoop();
		cleanup();
	}

private:
    GLFWwindow* window;
	VkInstance instance;

	VkPhysicalDevice physicalDevice = VK_NULL_HANDLE;

	const std::vector<const char*> deviceExtensions = {
	VK_KHR_SWAPCHAIN_EXTENSION_NAME
	};

	VkDevice logicalDevice;

	VkQueue graphicsQueue;
	VkQueue presentQueue;
	
	VkSurfaceKHR surface;

	VkSwapchainKHR swapChain;
	std::vector<VkImage> swapChainImages;
	std::vector<VkImageView> swapChainImageViews;
	VkFormat swapChainImageFormat;
	VkExtent2D swapChainExtent;

	ShaderManager* shaderManager;

	/*-----------------------------Initialization and Cleanup-----------------------------*/
    void initWindow() {
		glfwInit();

		glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
		glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);
		this->window = glfwCreateWindow(WIDTH, HEIGHT, "Our Engine", nullptr, nullptr);
	}

    void initVulkan() {
		createInstance();
		createSurface();
		pickPhysicalDevice();
		createLogicalDevice();
		createSwapChain();
		createImageViews();
		createGraphicsPipeline();
	}

    void mainLoop() {
        while (!glfwWindowShouldClose(window)) {
			glfwPollEvents();
		}
	}

    void cleanup() {

		for (auto imageView : swapChainImageViews) {
			vkDestroyImageView(logicalDevice, imageView, nullptr);
		}

		vkDestroySwapchainKHR(logicalDevice, swapChain, nullptr);

		vkDestroyDevice(logicalDevice, nullptr);

		vkDestroySurfaceKHR(instance, surface, nullptr);

		vkDestroyInstance(instance, nullptr);

		glfwDestroyWindow(window);

		glfwTerminate();
	}
	/*---------------------------------------------------------------------------------*/

	/*-------------------------------Find Physical Device------------------------------*/
	// Find and set the physical device (GPU) that will be used to run the program
	void pickPhysicalDevice() {
		uint32_t deviceCount = 0;
		vkEnumeratePhysicalDevices(instance, &deviceCount, nullptr);

		if (deviceCount == 0) {
			throw std::runtime_error("failed to find GPUs with Vulkan support!");
		}

		std::vector<VkPhysicalDevice> devices(deviceCount);
		vkEnumeratePhysicalDevices(instance, &deviceCount, devices.data());

		std::multimap<int, VkPhysicalDevice> candidates;

		for (const auto& device : devices) {
			int score = ratePhysicalDevice(device);
			candidates.insert(std::make_pair(score, device));
		}

		if (candidates.rbegin()->first > 0) {
			this->physicalDevice = candidates.rbegin()->second;
		}
		else {
			throw std::runtime_error("failed to find a suitable Gpu!");
		}


		if (this->physicalDevice == VK_NULL_HANDLE) {
			throw std::runtime_error("failed to find a suitable GPU!");
		}
	}

	// Rate a GPU based on its properties and features and return a score representing its performance capabilities
	int ratePhysicalDevice(VkPhysicalDevice device) {
		VkPhysicalDeviceProperties deviceProperties;
		vkGetPhysicalDeviceProperties(device, &deviceProperties);

		VkPhysicalDeviceFeatures deviceFeatures;
		vkGetPhysicalDeviceFeatures(device, &deviceFeatures);

		QueueFamilyIndices indices = findQueueFamilies(device);

		int score = 0;

		// Discrete GPUs have a significant performance advantage
		if (deviceProperties.deviceType == VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU) {
			score += 1000;
		}

		// Maximum possible size of textures affects graphics quality
		score += deviceProperties.limits.maxImageDimension2D;

		bool extensionsSupported = checkDeviceExtensionSupport(device);

		bool adequateSwapChain = false;
		if (extensionsSupported) {
			SwapChainSupportDetails swapChainSupport = querySwapChainSupport(device);
			adequateSwapChain = !swapChainSupport.formats.empty() && !swapChainSupport.presentModes.empty();
		}

		// Application can't function without geometry shaders, available queues, and required extensions
		if (!deviceFeatures.geometryShader || !indices.isComplete() || !adequateSwapChain) {
			return 0;
		}

		return score;
	}

	// Check if the GPU supports the required extensions
	bool checkDeviceExtensionSupport(VkPhysicalDevice device) {
		uint32_t extensionCount;
		vkEnumerateDeviceExtensionProperties(device, nullptr, &extensionCount, nullptr);

		std::vector<VkExtensionProperties> availableExtensions(extensionCount);
		vkEnumerateDeviceExtensionProperties(device, nullptr, &extensionCount, availableExtensions.data());

		std::set<std::string> requiredExtensions(deviceExtensions.begin(), deviceExtensions.end());

		for (const auto& extension : availableExtensions) {
			requiredExtensions.erase(extension.extensionName);
		}

		return requiredExtensions.empty();
	}

	/*---------------------------------------------------------------------------------*/

	/*------------------------------Create Vulkan Instance-----------------------------*/
	void createInstance() {

		if (enableValidationLayers && !checkValidationLayerSupport()) {
			throw std::runtime_error("validation layers requested, but not available!");
		}

		VkApplicationInfo appInfo{};
		appInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
		appInfo.pApplicationName = "Hello Triangle";
		appInfo.applicationVersion = VK_MAKE_VERSION(1, 0, 0);
		appInfo.pEngineName = "No Engine";
		appInfo.engineVersion = VK_MAKE_VERSION(1, 0, 0);
		appInfo.apiVersion = VK_API_VERSION_1_0;

		VkInstanceCreateInfo createInfo{};
		createInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
		createInfo.pApplicationInfo = &appInfo;

		uint32_t glfwExtensionCount = 0;
		const char** glfwExtensions;

		glfwExtensions = glfwGetRequiredInstanceExtensions(&glfwExtensionCount);

		createInfo.enabledExtensionCount = glfwExtensionCount;

		createInfo.ppEnabledExtensionNames = glfwExtensions;

		if (enableValidationLayers) {
			createInfo.enabledLayerCount = static_cast<uint32_t>(validationLayers.size());
			createInfo.ppEnabledLayerNames = validationLayers.data();
		}
		else {
			createInfo.enabledLayerCount = 0;
		}

		if (vkCreateInstance(&createInfo, nullptr, &instance) != VK_SUCCESS) {
			throw std::runtime_error("failed to create instance!");
		}
	}

	bool checkValidationLayerSupport() {
		uint32_t layerCount;
		vkEnumerateInstanceLayerProperties(&layerCount, nullptr);

		std::vector<VkLayerProperties> availableLayers(layerCount);
		vkEnumerateInstanceLayerProperties(&layerCount, availableLayers.data());

		// std::cout << "layerCount: " << layerCount << std::endl;

		for (const char* layerName : validationLayers) {
			bool layerFound = false;

			for (const auto& layerProperties : availableLayers) {
				// std::cout << "layerName: " << layerProperties.layerName << std::endl;
				if (strcmp(layerName, layerProperties.layerName) == 0) {
					layerFound = true;
					break;
				}
			}

			if (!layerFound) {
				return false;
			}
		}

		return true;
	}

	void createSurface() { // The vulkan surface that will be drawn to, and then presented to the window (allows Vulkan to be platform agnostic)
		if (glfwCreateWindowSurface(instance, window, nullptr, &surface) != VK_SUCCESS) {
			throw std::runtime_error("failed to create window surface!");
		}
	}

	// Create the logical device that interfaces with the physical device -- this creates the queues that will be used to interface with the physical device
	void createLogicalDevice() {
		QueueFamilyIndices indices = findQueueFamilies(physicalDevice);

		// Continue here at "Creating the presentation queue" in the Window Surface section of the tutorial

		std::vector<VkDeviceQueueCreateInfo> queueCreateInfos;
		std::set<uint32_t> uniqueQueueFamilies = { indices.graphicsFamily.value(), indices.presentFamily.value() };

		float queuePriority = 1.0f; // assignable priority of this queue, 0.0f to 1.0f, that influences the scheduling of command buffer execution (within the family?). Required even if there is only one queue
		for (uint32_t queueFamily : uniqueQueueFamilies) {
			VkDeviceQueueCreateInfo queueCreateInfo{};
			queueCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
			queueCreateInfo.queueFamilyIndex = queueFamily;
			queueCreateInfo.queueCount = 1; // apparently current drivers only support a few queues per family, but "you don't really need more than one", I guess I'll see for myself
			queueCreateInfo.pQueuePriorities = &queuePriority;
			queueCreateInfos.push_back(queueCreateInfo);
		}
		
		/*
		* Old code for creating a single queue, now we create a queue for each family that we need
		VkDeviceQueueCreateInfo queueCreateInfo{};
		queueCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
		queueCreateInfo.queueFamilyIndex = indices.graphicsFamily.value();
		queueCreateInfo.queueCount = 1; 

		float queuePriority = 1.0f; 
		queueCreateInfo.pQueuePriorities = &queuePriority;
		*/
		
		VkPhysicalDeviceFeatures deviceFeatures{}; // Used to enable or disable available features on chosen physical device

		VkDeviceCreateInfo createInfo{};
		createInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
		createInfo.pQueueCreateInfos = queueCreateInfos.data();
		createInfo.queueCreateInfoCount = static_cast<uint32_t>(queueCreateInfos.size());
		createInfo.pEnabledFeatures = &deviceFeatures; // setting our enabled features
		createInfo.enabledExtensionCount = static_cast<uint32_t>(deviceExtensions.size());
		createInfo.ppEnabledExtensionNames = deviceExtensions.data(); // setting the device specific extensions

		// Validation layers -- used for debugging
		if (enableValidationLayers) {
			createInfo.enabledLayerCount = static_cast<uint32_t>(validationLayers.size());
			createInfo.ppEnabledLayerNames = validationLayers.data();
		}
		else {
			createInfo.enabledLayerCount = 0;
		}

		// Create logical device
		if (vkCreateDevice(physicalDevice, &createInfo, nullptr, &logicalDevice) != VK_SUCCESS) {
			throw std::runtime_error("failed to create logical device!");
		}

		// until I optimize the findQueueFamilies function to choose different and independent queues for different operations, graphicsQueue and presentQueue will hold the same value (point to the same queue)
		vkGetDeviceQueue(logicalDevice, indices.graphicsFamily.value(), 0, &graphicsQueue); // assign the graphics queue that was created with the logical device (will likely move this later on)
		vkGetDeviceQueue(logicalDevice, indices.presentFamily.value(), 0, &presentQueue); // assign the present queue that was created with the logical device (will likely move this later on)
	}

	// Create the basic graphics pipeline that will be used to render the 2d images -- a different pipeline has to be created for any different rendering style so I'll likely have to create a new one for 3d rendering and more
	void createGraphicsPipeline() {
		shaderManager = new ShaderManager(logicalDevice); // might store this on the heap later on when I need access to uniforms in the shaders
		shaderManager->createGraphicsPipeline();
	}
	/*---------------------------------------------------------------------------------*/

	/*-------------------------------Queues and Swapchain------------------------------*/
	struct QueueFamilyIndices {
		std::optional<uint32_t> graphicsFamily;
		std::optional<uint32_t> presentFamily;

		bool isComplete() {
			return graphicsFamily.has_value() && presentFamily.has_value();
		}
	};

	struct SwapChainSupportDetails {
		VkSurfaceCapabilitiesKHR capabilities;
		std::vector<VkSurfaceFormatKHR> formats;
		std::vector<VkPresentModeKHR> presentModes;
	};

	//TODO: Optimize this function to choose the best available queue families for the operations we need. Currently it just chooses the first one that supports the operations meaning that one queue might be fulfilling multiple tasks, which isn't optimal
	// Find the queue families that are supported by the physical device for specific operations
	QueueFamilyIndices findQueueFamilies(VkPhysicalDevice device) {
		QueueFamilyIndices indices;

		// Logic to find queue family indices to populate struct with
		uint32_t queueFamilyCount = 0;
		vkGetPhysicalDeviceQueueFamilyProperties(device, &queueFamilyCount, nullptr);

		std::vector<VkQueueFamilyProperties> queueFamilies(queueFamilyCount);
		vkGetPhysicalDeviceQueueFamilyProperties(device, &queueFamilyCount, queueFamilies.data());

		VkBool32 presentSupport;
		int i = 0;
		for (const auto& queueFamily : queueFamilies) {
			if (queueFamily.queueFlags & VK_QUEUE_GRAPHICS_BIT) { // find the first queue family that supports graphics operations
				indices.graphicsFamily = i;
			}

			presentSupport = false;
			vkGetPhysicalDeviceSurfaceSupportKHR(device, i, surface, &presentSupport);

			if (presentSupport) { // find the first queue family that supports presenting to the surface. This may be the same as the graphics queue family
				indices.presentFamily = i;
			}

			if (indices.isComplete()) {
				break;
			}

			i++;
		}

		return indices;
	}
	
	// Return the swap chain support details for the physical device
	SwapChainSupportDetails querySwapChainSupport(VkPhysicalDevice device) {
		SwapChainSupportDetails details;

		vkGetPhysicalDeviceSurfaceCapabilitiesKHR(device, surface, &details.capabilities); // get the capabilities of the surface and GPU

		uint32_t formatCount;
		vkGetPhysicalDeviceSurfaceFormatsKHR(device, surface, &formatCount, nullptr); // get surface formats supported by the GPU

		// At least one format must be supported
		if (formatCount != 0) {
			details.formats.resize(formatCount);
			vkGetPhysicalDeviceSurfaceFormatsKHR(device, surface, &formatCount, details.formats.data());
		}

		uint32_t presentModeCount;
		vkGetPhysicalDeviceSurfacePresentModesKHR(device, surface, &presentModeCount, nullptr);

		// At least one present mode must be supported
		if (presentModeCount != 0) {
			details.presentModes.resize(presentModeCount);
			vkGetPhysicalDeviceSurfacePresentModesKHR(device, surface, &presentModeCount, details.presentModes.data());
		}

		return details;
	}
	
	// Surface Format specifies the color channels, types, and color(bit) depth
	VkSurfaceFormatKHR chooseSwapSurfaceFormat(const std::vector<VkSurfaceFormatKHR>& availableFormats) {
		for (const auto& availableFormat : availableFormats) {
			if (availableFormat.format == VK_FORMAT_B8G8R8A8_SRGB && availableFormat.colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR) {
				return availableFormat; // desired format is SRGB, 8 bit depth (per channel), 32 bit total
			}
		}
		// TODO: rank available formats and choose the best one
		return availableFormats[0]; // For now, if the desired format is not available, just return the first available format
	}

	// Presentation mode specifies the conditions for "swapping" the image to the screen, known as Vertical Sync (Vsync).
	VkPresentModeKHR chooseSwapPresentMode(const std::vector<VkPresentModeKHR>& availablePresentModes) {
		for (const auto& availablePresentMode : availablePresentModes) {
			if (availablePresentMode == VK_PRESENT_MODE_MAILBOX_KHR) { // prefered mode is triple buffering, but Vsync off is VK_PRESENT_MODE_IMMEDIATE_KHR
				return availablePresentMode; // TODO: allow user to choose their prefered presentation mode (VySync on/off)
			}
		}
		return VK_PRESENT_MODE_FIFO_KHR; // guaranteed to be available -- regular Vsync
	}

	// Swap extent is the resolution of the swap chain images. It is almost always equal to the resolution of the window we're drawing to in pixels
	VkExtent2D chooseSwapExtent(const VkSurfaceCapabilitiesKHR& capabilities) {
		if (capabilities.currentExtent.width != std::numeric_limits<uint32_t>::max()) {
			return capabilities.currentExtent;
		}
		else {
			int width, height;
			glfwGetFramebufferSize(window, &width, &height);

			VkExtent2D actualExtent = {
				static_cast<uint32_t>(width),
				static_cast<uint32_t>(height)
			};

			actualExtent.width = std::clamp(actualExtent.width, capabilities.minImageExtent.width, capabilities.maxImageExtent.width);
			actualExtent.height = std::clamp(actualExtent.height, capabilities.minImageExtent.height, capabilities.maxImageExtent.height);

			return actualExtent;
		}
	}

	void createSwapChain() {
		SwapChainSupportDetails swapChainSupport = querySwapChainSupport(physicalDevice);

		VkSurfaceFormatKHR surfaceFormat = chooseSwapSurfaceFormat(swapChainSupport.formats);
		VkPresentModeKHR presentMode = chooseSwapPresentMode(swapChainSupport.presentModes);
		VkExtent2D extent = chooseSwapExtent(swapChainSupport.capabilities);

		uint32_t imageCount = swapChainSupport.capabilities.minImageCount + 1; // minimum number of images in the swap chain
		if (swapChainSupport.capabilities.maxImageCount > 0 && imageCount > swapChainSupport.capabilities.maxImageCount) {
			imageCount = swapChainSupport.capabilities.maxImageCount;
		}

		VkSwapchainCreateInfoKHR createInfo{};
		createInfo.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
		createInfo.surface = surface;

		createInfo.minImageCount = imageCount;
		createInfo.imageFormat = surfaceFormat.format;
		createInfo.imageColorSpace = surfaceFormat.colorSpace;
		createInfo.imageExtent = extent;
		createInfo.imageArrayLayers = 1;
		createInfo.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT; // This field specifies what kind of operations the images in the swap chain will be used for
		/* Since for now we are rendering directly to the images in the swap chain, they are used as color attachment. If, say, we wanted to perform post-processing 
		 by rendering images to a different, separate image first, we would use a value like VK_IMAGE_USAGE_TRANSFER_DST_BIT instead and use a memory operation to
		 transfer the rendered image to a swap chain image. */

		QueueFamilyIndices indices = findQueueFamilies(physicalDevice);
		uint32_t queueFamilyIndices[] = { indices.graphicsFamily.value(), indices.presentFamily.value() };

		if (indices.graphicsFamily != indices.presentFamily) {
			// TODO: Implement ownership transfer of swap chain images if the graphics and present queues are different so that Exclusive mode can be used, which is more efficient
			createInfo.imageSharingMode = VK_SHARING_MODE_CONCURRENT; // until then, use Concurrent mode
			createInfo.queueFamilyIndexCount = 2; // must have two distinct queues for concurrent mode -- required with concurrent mode
			createInfo.pQueueFamilyIndices = queueFamilyIndices; // which queues will be shared for concurrent mode -- required with concurrent mode
		}
		else {
			createInfo.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
			createInfo.queueFamilyIndexCount = 0; // Optional for exclusive mode
			createInfo.pQueueFamilyIndices = nullptr; // Optional for exclusive mode
		}

		createInfo.preTransform = swapChainSupport.capabilities.currentTransform; // specifies the transform to be applied to the image before presentation -- like a rotation or flip -- right now it is set to do nothing
		createInfo.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR; // specifies if the alpha channel should be used for blending with other windows in the system -- right now it is set to ignore the alpha channel
		createInfo.presentMode = presentMode;
		createInfo.clipped = VK_TRUE; // set to not care about the color of obscured pixels (like those behind another window)
		createInfo.oldSwapchain = VK_NULL_HANDLE; // used to create a new swap chain if the old one becomes invalid (like window resizing) TODO: Implement this

		if (vkCreateSwapchainKHR(logicalDevice, &createInfo, nullptr, &swapChain) != VK_SUCCESS) {
			throw std::runtime_error("failed to create swap chain!");
		}

		vkGetSwapchainImagesKHR(logicalDevice, swapChain, &imageCount, nullptr);
		swapChainImages.resize(imageCount);
		vkGetSwapchainImagesKHR(logicalDevice, swapChain, &imageCount, swapChainImages.data());
		swapChainImageFormat = surfaceFormat.format;
		swapChainExtent = extent;
	}

	void createImageViews() {
		swapChainImageViews.resize(swapChainImages.size());
		for (size_t i = 0; i < swapChainImages.size(); i++) {
			VkImageViewCreateInfo createInfo{};
			createInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
			createInfo.image = swapChainImages[i];

			createInfo.viewType = VK_IMAGE_VIEW_TYPE_2D; // determines how textures are treated/interpreted: 1D, 2D, 3D, or cube map
			createInfo.format = swapChainImageFormat;

			createInfo.components.r = VK_COMPONENT_SWIZZLE_IDENTITY; // These 4 fields allow for remapping of color channels (like swapping red and blue channels)
			createInfo.components.g = VK_COMPONENT_SWIZZLE_IDENTITY;
			createInfo.components.b = VK_COMPONENT_SWIZZLE_IDENTITY;
			createInfo.components.a = VK_COMPONENT_SWIZZLE_IDENTITY;

			// The subresourceRange field describes what the image's purpose is and which part of the image should be accessed. We are using the image as a color target without any mipmapping levels or multiple layers.
			createInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
			createInfo.subresourceRange.baseMipLevel = 0;
			createInfo.subresourceRange.levelCount = 1;
			createInfo.subresourceRange.baseArrayLayer = 0;
			createInfo.subresourceRange.layerCount = 1;

			if (vkCreateImageView(logicalDevice, &createInfo, nullptr, &swapChainImageViews[i]) != VK_SUCCESS) {
				throw std::runtime_error("failed to create image views!");
			}
		}
	}
	/*---------------------------------------------------------------------------------*/
};

int main() {
	Engine engine;

	try {
		engine.run();
	}
	catch (const std::exception& e) {
		std::cerr << e.what() << std::endl;
		return EXIT_FAILURE;
	}

    return EXIT_SUCCESS;
}