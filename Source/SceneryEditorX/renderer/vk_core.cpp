/**
* -------------------------------------------------------
* Scenery Editor X
* -------------------------------------------------------
* Copyright (c) 2025 Thomas Ray 
* Copyright (c) 2025 Coalition of Freeware Developers
* -------------------------------------------------------
* vk_core.cpp
* -------------------------------------------------------
* Created: 17/3/2025
* -------------------------------------------------------
*/

#include <SceneryEditorX/renderer/vk_core.h>
#include <SceneryEditorX/renderer/vk_util.h>
#include <volk.h>

// -------------------------------------------------------

/**
 * @brief Initialize Vulkan layers.
 */
#ifdef SEDX_DEBUG
	std::vector<const char*> layers = {"VK_LAYER_KHRONOS_validation"};
#endif

/**
 * @brief Callback function for Vulkan debug messages.
 *
 * This function is called by Vulkan when a validation error occurs.
 * It prints the validation error message to the console.
 *
 * @param severity The severity of the message.
 * @param type The type of the message.
 * @param pCallbackData The callback data containing the message.
 * @param pUserData User data provided when the callback was registered.
 * @return VkBool32 Always returns VK_FALSE.
 */
static VKAPI_ATTR VkBool32 VKAPI_CALL DebugCallback(VkDebugUtilsMessageSeverityFlagBitsEXT severity,
													VkDebugUtilsMessageTypeFlagsEXT type,
													const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData,
													void* pUserData)
{
	printf("Debug callback: %s\n",pCallbackData->pMessage);
	printf("  Severity %s\n",getDebugSeverityStr(severity));
	printf("  Type %s\n",getDebugType(type));
	printf("  Objects ");

	for (uint32_t i = 0; i < pCallbackData->objectCount; i++)
	{
		printf("%llx ",pCallbackData->pObjects[i].objectHandle);
	}

	printf("\n");

	return VK_FALSE; // Return false to continue execution
}

static constexpr std::array<const char*,3> instanceExtensions =
{
	{
		VK_KHR_SURFACE_EXTENSION_NAME,
		VK_KHR_GET_PHYSICAL_DEVICE_PROPERTIES_2_EXTENSION_NAME,
		VK_KHR_WIN32_SURFACE_EXTENSION_NAME
	}
};

/**
 * @brief Initialize Vulkan.
 *
 * This function initializes Vulkan by creating an instance and setting up a debug callback.
 *
 * @param windowTitleTChar The name of the application.
 * @param window The handle to the window.
 *
 * @return true if Vulkan is initialized successfully, false otherwise.
 */
void GraphicsEngine::initEngine()
{
	create_instance();
	create_debug_callback();
	create_surface();
	//physical_device.Init(m_instance,surface);
	//queue_family = physical_device.selectDevice(VK_QUEUE_GRAPHICS_BIT,true);
	select_physical_device();
	create_device();
	create_destruction_handler();
	//create_swap_chain();
	create_command_pool();
	create_sync_objects();
	create_descriptors();
	//ImgQueue.InitQueue(device,swap_chain,queue_family,0);
	//CreateCommandBuffers(1,&CpyCmdBuff);
}

void GraphicsEngine::create_instance()
{
	if (volkInitialize() != VK_SUCCESS)
	{
		throw std::runtime_error("VOLK ERROR: Failed to initialize Volk!");
	}

	/*
	 * -------------------------------------------------------
	 * Initialize Vulkan Application Info (simplified)
	 * -------------------------------------------------------
	 */
	constexpr VkApplicationInfo appInfo = {
		.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO,           // Vulkan structure type
		.pNext = nullptr,                                      // Pointer to extension-specific structure
		.pApplicationName = "Scenery Editor X",                // Application name
		.applicationVersion = VK_MAKE_API_VERSION(1, 0, 0, 0), // Application version
		.pEngineName = "Editor X Engine",                      // Engine name
		.engineVersion = VK_MAKE_API_VERSION(1, 0, 0, 0),      // Engine version
		.apiVersion = VK_API_VERSION_1_3,                      // Vulkan API version
	};

	VkInstanceCreateInfo createInfo = {
		.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO,
		.flags = 0,
		.pApplicationInfo = &appInfo,
	};

	VkDebugUtilsMessengerCreateInfoEXT debugMessengerInfo = {
		.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT,
		.pNext = VK_NULL_HANDLE,
		.flags = 0,
		.messageSeverity = VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT |
						   VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT |
						   VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT,
		.messageType = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT | 
		               VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT |
					   VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT,
		.pfnUserCallback = &DebugCallback,
		.pUserData = VK_NULL_HANDLE, // Optional
	};

	/*
	 * -------------------------------------------------------
	 * Initialize Vulkan Instance Info (simplified)
	 * -------------------------------------------------------
	 */
	VkInstanceCreateInfo InstanceCreateInfo;
	InstanceCreateInfo = {
		.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO,							// Vulkan structure type
		.pNext = &debugMessengerInfo,												// Pointer to extension-specific structure
		.flags = 0,																	// Reserved for future use
		.pApplicationInfo = &appInfo,												// Application info
		.enabledLayerCount = static_cast<uint32_t>(layers.size()),					// Number of enabled layers
		.ppEnabledLayerNames = layers.data(),										// Enabled layers
		.enabledExtensionCount = static_cast<uint32_t>(instanceExtensions.size()),  // Number of enabled extensions
		.ppEnabledExtensionNames = instanceExtensions.data()                        // Enabled extensions
	};


	if (vkCreateInstance(&InstanceCreateInfo, nullptr, &m_Instance) != VK_SUCCESS)
	{
		EDITOR_LOG_INFO("Failed to create Vulkan instance.");
		//ErrMsg("Failed to create Vulkan instance.");
		exit(1);
	}

	VkResult result = vkCreateInstance(&InstanceCreateInfo, nullptr, &m_Instance);
	VK_CHECK_RESULT(result,"Vulkan Instance");
	EDITOR_LOG_INFO("Vulkan instance created successfully.");
	EDITOR_LOG_INFO("Vulkan API Version: {}.{}.{}",
						VK_VERSION_MAJOR(appInfo.apiVersion),
						VK_VERSION_MINOR(appInfo.apiVersion),
						VK_VERSION_PATCH(appInfo.apiVersion));
}

/**
 * @brief - Create a debug callback.
 *
 * @param m_instance - The Vulkan instance.
 * @param DebugMessenger - The Vulkan debug messenger.
 * @return VkDebugUtilsMessengerEXT The Vulkan debug messenger.
 */
void GraphicsEngine::create_debug_callback()
{
	VkDebugUtilsMessengerCreateInfoEXT debugMessengerInfo = {
		.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT,
		.pNext = VK_NULL_HANDLE,
		.flags = 0,
		.messageSeverity = VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT |
						   VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT |
						   VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT,
		.messageType = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT |
					   VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT,
		.pfnUserCallback = &DebugCallback,
		.pUserData = VK_NULL_HANDLE, // Optional
	};

	PFN_vkCreateDebugUtilsMessengerEXT vkCreateDebugUtilsMessenger = VK_NULL_HANDLE;
	vkCreateDebugUtilsMessenger = (PFN_vkCreateDebugUtilsMessengerEXT)vkGetInstanceProcAddr(m_Instance,"vkCreateDebugUtilsMessengerEXT");
	if (!vkCreateDebugUtilsMessenger)
	{
		EDITOR_LOG_ERROR("Cannot find address of vkCreateDebugUtilsMessenger");
		//ErrMsg("Cannot find address of vkCreateDebugUtilsMessenger");
		exit(1);
	}

	VkResult result = vkCreateDebugUtilsMessenger(m_Instance,&debugMessengerInfo,VK_NULL_HANDLE,&m_DebugMessenger);
	VK_CHECK_RESULT(result,"Vulkan Debug Utility Messanger");
}

void GraphicsEngine::create_surface()
{

}

void GraphicsEngine::select_physical_device()
{}

void GraphicsEngine::create_device()
{}

void GraphicsEngine::create_destruction_handler()
{}

void GraphicsEngine::create_command_pool()
{}

void GraphicsEngine::create_sync_objects()
{}

void GraphicsEngine::create_descriptors()
{}
