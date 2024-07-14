#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>
#include <vulkan/vulkan.h>
#include <iostream>
#include <stdexcept>
#include <cstdlib>
#include <vector>
#include <map>
#include <set>
#include <optional>

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
	}

    void mainLoop() {
        while (!glfwWindowShouldClose(window)) {
			glfwPollEvents();
		}
	}

    void cleanup() {
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

		// Application can't function without geometry shaders, available queues, and required extensions
		if (!deviceFeatures.geometryShader || !indices.isComplete() || !extensionsSupported) {
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

	/*--------------------Create Vulkan Instance and Logical Device--------------------*/
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

		if (formatCount != 0) {
			details.formats.resize(formatCount);
			vkGetPhysicalDeviceSurfaceFormatsKHR(device, surface, &formatCount, details.formats.data());
		}

		// Continue here from "Querying details of swap chain support" in the Swap Chain section of the tutorial

		return details;
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