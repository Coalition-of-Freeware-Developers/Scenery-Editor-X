/**
 * -------------------------------------------------------
 * Scenery Editor X
 * -------------------------------------------------------
 * Copyright (c) 2026 Thomas Ray 
 * Copyright (c) 2026 Coalition of Freeware Developers
 * -------------------------------------------------------
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 * -------------------------------------------------------
 * graphics_debug.cpp
 * -------------------------------------------------------
 * Created: 16/02/2026
 * -------------------------------------------------------
 */
#include "graphics_debug.h"
#include <cstring>
#include <format>
#include <mutex>
#include <set>
#include <string>
#include <unordered_map>
#include <utility>
#include <SceneryEditorX/renderer/vulkan/command_list.h>
#include <SceneryEditorX/renderer/vulkan/render_context.h>
#include <volk/volk.h>
#include <vulkan/vulkan_core.h>

#ifndef RENDERDOC_ENABLED
#define RENDERDOC_TOOLING_ENABLED 0
#else
#define RENDERDOC_TOOLING_ENABLED 1
#endif

#if RENDERDOC_TOOLING_ENABLED == 1
#include "renderdoc/app/renderdoc_app.h"
#endif

// -----------------------------------------------------------------

namespace SceneryEditorX
{
	VkDebugUtilsMessengerEXT s_Messenger = VK_NULL_HANDLE;

	// Function Pointers
	PFN_vkCreateDebugUtilsMessengerEXT    s_CreateDebugMessenger			= nullptr; 
	PFN_vkDestroyDebugUtilsMessengerEXT   s_DestroyDebugMessenger			= nullptr;
	PFN_vkSetDebugUtilsObjectTagEXT       s_SetDebugObjectTag				= nullptr;
	PFN_vkSetDebugUtilsObjectNameEXT      s_SetDebugObjectName				= nullptr;
	PFN_vkCmdBeginDebugUtilsLabelEXT      s_MarkerBegin						= nullptr;
	PFN_vkCmdEndDebugUtilsLabelEXT        s_MarkerEnd						= nullptr;

	thread_local std::string s_CreationContext; // object being created on this thread (Debugging::SetCreationContext)

	// GPU crash breadcrumbs (Debugging::SetBreadcrumbsEnabled): what the device was created with
	bool s_BreadcrumbCheckpoints = false;	// VK_NV_device_diagnostic_checkpoints
	bool s_BreadcrumbDeviceFault = false;	// VK_EXT_device_fault
	bool s_BreadcrumbAddressBinding = false;	// VK_EXT_device_address_binding_report
	bool s_DeviceLostReported = false;

	// Every GPU address range bound or unbound (VK_EXT_device_address_binding_report) and the debug names given to
	// objects, so a device fault's address can be traced to the buffer or image there (or the one that was there)
	struct AddressBinding
	{
		uint64_t base = 0;
		uint64_t size = 0;
		uint64_t handle = 0;
		VkObjectType type = VK_OBJECT_TYPE_UNKNOWN;
		bool bound = true;
		std::string name;	// The object's debug name when the range was bound or unbound
	};
	std::mutex s_BreadcrumbMutex;
	std::vector<AddressBinding> s_AddressBindings;
	std::unordered_map<uint64_t, std::string> s_ObjectNames;

	void RecordObjectName(const uint64_t handle, const char *name)
	{
		if (!s_BreadcrumbAddressBinding || handle == 0 || name == nullptr)
			return;
		std::lock_guard lock(s_BreadcrumbMutex);
		s_ObjectNames[handle] = name;
	}

	void RecordAddressBinding(const VkDebugUtilsMessengerCallbackDataEXT *data)
	{
		const auto *binding = static_cast<const VkDeviceAddressBindingCallbackDataEXT *>(data->pNext);
		while (binding && binding->sType != VK_STRUCTURE_TYPE_DEVICE_ADDRESS_BINDING_CALLBACK_DATA_EXT)
			binding = static_cast<const VkDeviceAddressBindingCallbackDataEXT *>(binding->pNext);
		if (!binding)
			return;

		AddressBinding entry;
		entry.base = binding->baseAddress;
		entry.size = binding->size;
		entry.bound = binding->bindingType == VK_DEVICE_ADDRESS_BINDING_TYPE_BIND_EXT;
		if (data->objectCount > 0)
		{
			entry.handle = data->pObjects[0].objectHandle;
			entry.type = data->pObjects[0].objectType;
			if (data->pObjects[0].pObjectName)
				entry.name = data->pObjects[0].pObjectName;
		}

		std::lock_guard lock(s_BreadcrumbMutex);
		if (entry.name.empty())
		{
			if (const auto it = s_ObjectNames.find(entry.handle); it != s_ObjectNames.end())
				entry.name = it->second;
		}
		s_AddressBindings.push_back(std::move(entry));
	}

#ifndef RENDERDOC_ENABLED
	#define RENDERDOC_TOOLING_ENABLED 0
#else
	#define RENDERDOC_TOOLING_ENABLED 1
#endif

	/**
	 * @namespace ValidationLayer
	 * @brief Contains helper functions and data for configuring and managing the Vulkan validation layer integration. 
	 * This includes checking for layer support, defining layer settings, 
	 * and providing access to those settings for use during Vulkan instance and device creation.
	 */
	namespace ValidationLayer
	{
		// layers configuration: https://vulkan.lunarg.com/doc/view/1.3.296.0/windows/layer_configuration.html

		// The layer checks these at vkCreateInstance and reports wrong types, unknown values and deprecated settings
		// (message id VALIDATION-SETTINGS), so keep them in step with the installed layer.
		static const char *s_LayerName = "VK_LAYER_KHRONOS_validation";
		static const VkBool32 SETTING_VALIDATE_CORE				= VK_TRUE;
		static const VkBool32 SETTING_VALIDATE_SYNC				= VK_TRUE;
		static const VkBool32 SETTING_VALIDATE_BEST_PRACTICES	= VK_TRUE;
		static const VkBool32 SETTING_GPU_ASSISTED				= VK_TRUE;
		static const VkBool32 SETTING_THREAD_SAFETY				= VK_TRUE;
		static const VkBool32 SETTING_ENABLE_MESSAGE_LIMIT		= VK_TRUE;
		static const uint32_t SETTING_DUPLICATE_MESSAGE_LIMIT	= 10;
		static const char *s_Setting_DebugAction[] = {"VK_DBG_LAYER_ACTION_LOG_MSG"};
		static const char *s_Setting_ReportFlags[] = {"info", "warn", "perf", "error"};

		static std::vector<VkLayerSettingEXT> s_SettingsStorage; // persistent storage for VkLayerSettingEXT

		/**
		 * @brief Checks if the Vulkan validation layer is supported on the current system.
		 * @return true if the validation layer is supported, false otherwise.
		 */
		static bool IsLayerSupported()
		{
			uint32_t layerCount = 0;
			vkEnumerateInstanceLayerProperties(&layerCount, nullptr);

			if (layerCount == 0)
			{
				return false;
			}

			std::vector<VkLayerProperties> layers(layerCount);
			vkEnumerateInstanceLayerProperties(&layerCount, layers.data());

			for (const VkLayerProperties &layer : layers)
			{
				if (strcmp(s_LayerName, layer.layerName) == 0)
				{
					return true;
				}
			}

			return false;
		}

		/**
		 * @brief Retrieves the Vulkan validation layer settings.
		 * @return A reference to a vector containing the Vulkan validation layer settings.
		 */
		static std::vector<VkLayerSettingEXT> &GetSettings()
		{
			SEDX_CORE_ASSERT(Debugging::IsValidationLayerEnabled(), "Validation layer is not enabled");
			SEDX_CORE_ASSERT(IsLayerSupported(), "Please install the Vulkan SDK, ensure correct environment variables and restart your machine: https://vulkan.lunarg.com/sdk/home");

			// fill static settings
			s_SettingsStorage = {
				{.pLayerName = s_LayerName,
				 .pSettingName = "validate_core",
				 .type = VK_LAYER_SETTING_TYPE_BOOL32_EXT,
				 .valueCount = 1,
				 .pValues = &SETTING_VALIDATE_CORE},
				{.pLayerName = s_LayerName,
				 .pSettingName = "validate_sync",
				 .type = VK_LAYER_SETTING_TYPE_BOOL32_EXT,
				 .valueCount = 1,
				 .pValues = &SETTING_VALIDATE_SYNC},
				{.pLayerName = s_LayerName,
				 .pSettingName = "validate_best_practices",
				 .type = VK_LAYER_SETTING_TYPE_BOOL32_EXT,
				 .valueCount = 1,
				 .pValues = &SETTING_VALIDATE_BEST_PRACTICES},
				{.pLayerName = s_LayerName,
				 .pSettingName = "thread_safety",
				 .type = VK_LAYER_SETTING_TYPE_BOOL32_EXT,
				 .valueCount = 1,
				 .pValues = &SETTING_THREAD_SAFETY},
				{.pLayerName = s_LayerName,
				 .pSettingName = "debug_action",
				 .type = VK_LAYER_SETTING_TYPE_STRING_EXT,
				 .valueCount = 1,
				 .pValues = s_Setting_DebugAction},
				{.pLayerName = s_LayerName,
				 .pSettingName = "report_flags",
				 .type = VK_LAYER_SETTING_TYPE_STRING_EXT,
				 .valueCount = static_cast<uint32_t>(COUNT_OF(s_Setting_ReportFlags)),
				 .pValues = s_Setting_ReportFlags},
				{.pLayerName = s_LayerName,
				 .pSettingName = "enable_message_limit",
				 .type = VK_LAYER_SETTING_TYPE_BOOL32_EXT,
				 .valueCount = 1,
				 .pValues = &SETTING_ENABLE_MESSAGE_LIMIT},
				{.pLayerName = s_LayerName,
				 .pSettingName = "duplicate_message_limit",
				 .type = VK_LAYER_SETTING_TYPE_UINT32_EXT,
				 .valueCount = 1,
				 .pValues = &SETTING_DUPLICATE_MESSAGE_LIMIT}
			};

			// optionally add GPU-assisted validation
			if (Debugging::IsGpuAssistedValidationEnabled())
			{
				s_SettingsStorage.push_back({
					.pLayerName = s_LayerName,
					.pSettingName = "gpuav_enable",
					.type = VK_LAYER_SETTING_TYPE_BOOL32_EXT,
					.valueCount = 1,
					.pValues = &SETTING_GPU_ASSISTED
				});
			}

			return s_SettingsStorage;
		}

	}

	// -----------------------------------------------------------------

	/**
	 * @brief Vulkan validation layer callback function that receives debug messages from the Vulkan API and logs them using the engine's logging system.
	 * @param msgSeverity The severity of the message.
	 * @param msgType The type of the message.
	 * @param pCallbackData A pointer to a structure containing details about the validation message.
	 * @param pUserData A pointer to user-defined data.
	 * @return VK_FALSE to indicate that the Vulkan call that triggered the validation message should not be aborted. 
	 * The validation layer will continue as normal after this callback returns. 
	 */
	VKAPI_ATTR VkBool32 VKAPI_CALL VulkanLog(VkDebugUtilsMessageSeverityFlagBitsEXT msgSeverity, VkDebugUtilsMessageTypeFlagsEXT msgType, const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData,  void* pUserData)
	{
		// GPU address ranges (breadcrumbs): recorded for a device loss report, not logged
		if (msgType & VK_DEBUG_UTILS_MESSAGE_TYPE_DEVICE_ADDRESS_BINDING_BIT_EXT)
		{
			RecordAddressBinding(pCallbackData);
			return VK_FALSE;
		}

		// The loader reports its layer and driver search at info level on every start (~100 lines): keep only its warnings.
		const char *messageId = pCallbackData->pMessageIdName;
		if ((msgSeverity & VK_DEBUG_UTILS_MESSAGE_SEVERITY_INFO_BIT_EXT) && messageId && std::strcmp(messageId, "Loader Message") == 0)
			return VK_FALSE;

		// "Vulkan [message id]: message", then the named objects involved (or the object being created), so a
		// message can be traced back to the pass, pipeline or resource that caused it.
		std::string msg = "Vulkan";
		if (messageId && *messageId && std::strstr(pCallbackData->pMessage, messageId) == nullptr)
			msg += std::format(" [{}]", messageId);
		msg += ": ";
		msg += pCallbackData->pMessage;

		std::string objects;
		for (uint32_t i = 0; i < pCallbackData->objectCount; ++i)
		{
			const VkDebugUtilsObjectNameInfoEXT &object = pCallbackData->pObjects[i];
			if (object.pObjectName && *object.pObjectName)
				objects += std::format("{}'{}'", objects.empty() ? "" : ", ", object.pObjectName);
		}
		if (!objects.empty())
			msg += std::format(" (objects: {})", objects);
		if (!s_CreationContext.empty())
			msg += std::format(" (while creating '{}')", s_CreationContext);

		if (/*(msgSeverity & VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT) ||*/ (msgSeverity & VK_DEBUG_UTILS_MESSAGE_SEVERITY_INFO_BIT_EXT))
		{
			SEDX_CORE_INFO_TAG("Validation Layer", "{}", msg);
		}
		else if (msgSeverity & VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT)
		{
			SEDX_CORE_WARN_TAG("Validation Layer", "{}", msg);
		}
		else if (msgSeverity & VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT)
		{
			SEDX_CORE_ERROR_TAG("Validation Layer", "{}", msg);
		}

		return VK_FALSE;
	}

	/**
	 * @brief Enables the Vulkan validation layer by creating a debug messenger that will receive validation messages from the Vulkan API.
	 * This function should be called after the Vulkan instance has been created and the necessary function pointers have been loaded.
	 * If the validation layer is not supported or if the debug messenger fails to initialize, this function will log a warning and continue without validation.
	 * @param instance The Vulkan instance for which to create the debug messenger.
	 */
	static void EnableValidationLayer(const VkInstance instance)
	{
		if (s_CreateDebugMessenger)
		{
			const VkDebugUtilsMessengerCreateInfoEXT createInfo = Debugging::GetMessengerCreateInfo(Debugging::IsBreadcrumbsEnabled());

			VkResult result = s_CreateDebugMessenger(instance, &createInfo, nullptr, &s_Messenger);
			if (result == VK_SUCCESS)
			{
				SEDX_CORE_INFO_TAG("Validation Layer", "Validation messenger initialized");
			}
			else
			{
				SEDX_CORE_WARN_TAG("Validation Layer", "Failed to create validation messenger (VkResult={})", static_cast<int>(result));
			}
		}
	}

	/**
	 * @brief Disables the Vulkan validation layer by destroying the debug messenger that was created to receive validation messages.
	 * @param instance The Vulkan instance for which to destroy the debug messenger.
	 */
	static void DestroyDebugMessenger(VkInstance instance)
	{
		if (!s_DestroyDebugMessenger || s_Messenger == VK_NULL_HANDLE)
			return;

		s_DestroyDebugMessenger(instance, s_Messenger, nullptr);
	    s_Messenger = VK_NULL_HANDLE;
	}

	/**
	 * @brief Converts a custom ResourceType to the corresponding Vulkan object type.
	 * @param type The custom resource type to convert.
	 * @return The corresponding Vulkan object type.
	 */
	static VkObjectType GetVulkanObjectType(const ResourceType type)
	{
		switch (type)
		{
		case ResourceType::Unknown:					return VK_OBJECT_TYPE_UNKNOWN;
		case ResourceType::PhysicalDevice:			return VK_OBJECT_TYPE_PHYSICAL_DEVICE;
		case ResourceType::Device:					return VK_OBJECT_TYPE_DEVICE;
		case ResourceType::Fence:					return VK_OBJECT_TYPE_FENCE;
		case ResourceType::Semaphore:				return VK_OBJECT_TYPE_SEMAPHORE;
		case ResourceType::Shader:					return VK_OBJECT_TYPE_SHADER_MODULE;
		case ResourceType::Sampler:					return VK_OBJECT_TYPE_SAMPLER;
		case ResourceType::QueryPool:				return VK_OBJECT_TYPE_QUERY_POOL;
		case ResourceType::Image:					return VK_OBJECT_TYPE_IMAGE;
		case ResourceType::ImageView:				return VK_OBJECT_TYPE_IMAGE_VIEW;
		case ResourceType::DeviceMemory:			return VK_OBJECT_TYPE_DEVICE_MEMORY;
		case ResourceType::Buffer:					return VK_OBJECT_TYPE_BUFFER;
		case ResourceType::UniformBufferSet:		return VK_OBJECT_TYPE_BUFFER;
		case ResourceType::CommandBuffer:			return VK_OBJECT_TYPE_COMMAND_BUFFER;
		case ResourceType::CommandList:				return VK_OBJECT_TYPE_COMMAND_BUFFER;
		case ResourceType::CommandPool:				return VK_OBJECT_TYPE_COMMAND_POOL;
		case ResourceType::DescriptorSet:			return VK_OBJECT_TYPE_DESCRIPTOR_SET;
		case ResourceType::DescriptorSetLayout:		return VK_OBJECT_TYPE_DESCRIPTOR_SET_LAYOUT;
		case ResourceType::Pipeline:				return VK_OBJECT_TYPE_PIPELINE;
		case ResourceType::PipelineLayout:			return VK_OBJECT_TYPE_PIPELINE_LAYOUT;
		case ResourceType::PipelineCache:			return VK_OBJECT_TYPE_PIPELINE_CACHE;
		case ResourceType::Queue:					return VK_OBJECT_TYPE_QUEUE;
		//case ResourceType::AccelerationStructure:	return VK_OBJECT_TYPE_ACCELERATION_STRUCTURE_KHR;
		case ResourceType::Event:					return VK_OBJECT_TYPE_EVENT;
		case ResourceType::RenderPass:				return VK_OBJECT_TYPE_RENDER_PASS;
		case ResourceType::DescriptorPool:			return VK_OBJECT_TYPE_DESCRIPTOR_POOL;
		case ResourceType::DebugCallback:			return VK_OBJECT_TYPE_DEBUG_REPORT_CALLBACK_EXT;
		case ResourceType::MaxEnum:					return VK_OBJECT_TYPE_MAX_ENUM;
		default:
			return VK_OBJECT_TYPE_UNKNOWN;
		}
	}
		
	// -----------------------------------------------------------------

	void Debugging::SetValidationLayerEnabled(const bool enabled) { m_ValidationLayer = enabled; }

	const char *Debugging::GetValidationLayerName() { return ValidationLayer::s_LayerName; }

	bool Debugging::IsValidationLayerSupported() { return ValidationLayer::IsLayerSupported(); }

	void Debugging::Initialize(const VkInstance instance)
	{
		if (!IsValidationLayerEnabled())
			return;

		s_CreateDebugMessenger = reinterpret_cast<PFN_vkCreateDebugUtilsMessengerEXT>(vkGetInstanceProcAddr(instance, "vkCreateDebugUtilsMessengerEXT"));
		s_DestroyDebugMessenger = reinterpret_cast<PFN_vkDestroyDebugUtilsMessengerEXT>(vkGetInstanceProcAddr(instance, "vkDestroyDebugUtilsMessengerEXT"));
		s_SetDebugObjectTag = reinterpret_cast<PFN_vkSetDebugUtilsObjectTagEXT>(vkGetInstanceProcAddr(instance, "vkSetDebugUtilsObjectTagEXT"));
		s_SetDebugObjectName = reinterpret_cast<PFN_vkSetDebugUtilsObjectNameEXT>(vkGetInstanceProcAddr(instance, "vkSetDebugUtilsObjectNameEXT"));
		s_MarkerBegin = reinterpret_cast<PFN_vkCmdBeginDebugUtilsLabelEXT>(vkGetInstanceProcAddr(instance, "vkCmdBeginDebugUtilsLabelEXT"));
		s_MarkerEnd = reinterpret_cast<PFN_vkCmdEndDebugUtilsLabelEXT>(vkGetInstanceProcAddr(instance, "vkCmdEndDebugUtilsLabelEXT"));

		if (!s_CreateDebugMessenger || !s_DestroyDebugMessenger)
		{
			SEDX_CORE_WARN_TAG("ValidationLayer", "Debug utils extension is unavailable. Validation callbacks are disabled.");
			return;
		}

		EnableValidationLayer(instance);
	}

	void Debugging::Shutdown(const VkInstance instance) { DestroyDebugMessenger(instance); }

	VkDebugUtilsMessengerCreateInfoEXT Debugging::GetMessengerCreateInfo(const bool addressBindings)
	{
		// Verbose messages are loader chatter and were never logged; info is where the layer reports its active settings.
		VkDebugUtilsMessengerCreateInfoEXT createInfo = {};
		createInfo.sType           = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT;
		createInfo.messageSeverity = VK_DEBUG_UTILS_MESSAGE_SEVERITY_INFO_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT;
		createInfo.messageType     = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT;
		if (addressBindings)
			createInfo.messageType |= VK_DEBUG_UTILS_MESSAGE_TYPE_DEVICE_ADDRESS_BINDING_BIT_EXT;	// GPU address ranges for a device loss report
		createInfo.pfnUserCallback = VulkanLog;
		return createInfo;
	}

	void Debugging::SetCreationContext(const char *name) { s_CreationContext = name ? name : ""; }

	void Debugging::SetResourceName(void *resource, const ResourceType type, const char *name)
	{
		RecordObjectName(reinterpret_cast<uint64_t>(resource), name);
		if (!IsValidationLayerEnabled() || !s_SetDebugObjectName)
			return;

		// RenderContext::m_Device is null while Device::Device() is still constructing its
		// sub-systems (QueueManager, CommandLists, FrameSync objects). Guard here so that
		// those early-boot resources are silently skipped instead of crashing.
		Ref<Device> device = RenderContext::Get()->GetDevice();
		if (!device)
			return;

		SEDX_CORE_ASSERT(resource != nullptr);

		VkDebugUtilsObjectNameInfoEXT nameInfo = {};
		nameInfo.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_OBJECT_NAME_INFO_EXT;
		nameInfo.pNext = nullptr;
		nameInfo.objectType = GetVulkanObjectType(type);
		nameInfo.objectHandle = reinterpret_cast<uint64_t>(resource);
		nameInfo.pObjectName = name;

		s_SetDebugObjectName(device->GetLogicalDevice(), &nameInfo);
	}

	void Debugging::SetResourceName(VkDevice device, void *resource, const ResourceType type, const char *name)
	{
		RecordObjectName(reinterpret_cast<uint64_t>(resource), name);
		if (!IsValidationLayerEnabled() || !s_SetDebugObjectName)
			return;

		if (device == VK_NULL_HANDLE || resource == nullptr)
			return;

		VkDebugUtilsObjectNameInfoEXT nameInfo = {};
		nameInfo.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_OBJECT_NAME_INFO_EXT;
		nameInfo.pNext = nullptr;
		nameInfo.objectType = GetVulkanObjectType(type);
		nameInfo.objectHandle = reinterpret_cast<uint64_t>(resource);
		nameInfo.pObjectName = name;

		s_SetDebugObjectName(device, &nameInfo);
	}

	LayerSettingsData Debugging::GetLayerSettings()
	{
		LayerSettingsData data{};

		if (IsValidationLayerEnabled())
		{
			data.settings = ValidationLayer::GetSettings();
			data.createInfo.sType = VK_STRUCTURE_TYPE_LAYER_SETTINGS_CREATE_INFO_EXT;
			data.createInfo.pNext = nullptr;
			data.createInfo.pSettings = data.settings.data();
			data.createInfo.settingCount = static_cast<uint32_t>(data.settings.size());
		}

		return data;
	}

	void Debugging::BeginMarker(const CommandList *cmdList, const char *name, const xMath::Vec4 &color)
	{
		if (!IsGpuMarkingEnabled() || s_MarkerBegin == nullptr || cmdList == nullptr)
		{
			return;
		}

		VkDebugUtilsLabelEXT label = {};
		label.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_LABEL_EXT;
		label.pNext = nullptr;
		label.pLabelName = name;
		label.color[0] = color.x;
		label.color[1] = color.y;
		label.color[2] = color.z;
		label.color[3] = color.w;

		s_MarkerBegin(cmdList->GetCommandBuffer(), &label);
	}

	void Debugging::EndMarker(const CommandList *cmdList)
	{
		if (!IsGpuMarkingEnabled() || s_MarkerEnd == nullptr || cmdList == nullptr)
		{
			return;
		}

		s_MarkerEnd(cmdList->GetCommandBuffer());
	}

	Debugging::BreadcrumbExtensions Debugging::AddBreadcrumbExtensions(VkPhysicalDevice physicalDevice, std::vector<const char *> &extensions)
	{
		uint32_t count = 0;
		vkEnumerateDeviceExtensionProperties(physicalDevice, nullptr, &count, nullptr);
		std::vector<VkExtensionProperties> available(count);
		vkEnumerateDeviceExtensionProperties(physicalDevice, nullptr, &count, available.data());
		const auto add = [&](const char *name)
		{
			for (const VkExtensionProperties &extension : available)
			{
				if (std::strcmp(extension.extensionName, name) == 0)
				{
					extensions.push_back(name);
					return true;
				}
			}
			return false;
		};

		s_BreadcrumbCheckpoints = add(VK_NV_DEVICE_DIAGNOSTIC_CHECKPOINTS_EXTENSION_NAME);
		s_BreadcrumbDeviceFault = add(VK_EXT_DEVICE_FAULT_EXTENSION_NAME);
		// The binding reports arrive through the debug messenger, which only exists with the validation layer
		s_BreadcrumbAddressBinding = IsValidationLayerEnabled() && add(VK_EXT_DEVICE_ADDRESS_BINDING_REPORT_EXTENSION_NAME);
		SEDX_CORE_INFO_TAG("GPU Crash", "Breadcrumbs: checkpoints {}, device fault report {}, address bindings {}",
			s_BreadcrumbCheckpoints ? "on" : "not supported", s_BreadcrumbDeviceFault ? "on" : "not supported",
			s_BreadcrumbAddressBinding ? "on" : "off (needs the validation layer's messenger)");
		return { .deviceFault = s_BreadcrumbDeviceFault, .addressBinding = s_BreadcrumbAddressBinding };
	}

	void Debugging::SetBreadcrumb(const CommandList *cmdList, const char *marker)
	{
		if (!s_BreadcrumbCheckpoints || cmdList == nullptr || vkCmdSetCheckpointNV == nullptr)
			return;

		vkCmdSetCheckpointNV(cmdList->GetCommandBuffer(), marker);
	}

	const char *Debugging::InternBreadcrumb(const std::string_view name)
	{
		static std::mutex mutex;
		static std::set<std::string, std::less<>> names;
		std::lock_guard lock(mutex);
		auto it = names.find(name);
		if (it == names.end())
			it = names.emplace(name).first;
		return it->c_str();
	}

	namespace
	{
		/* @brief Logs the objects bound (now or before) at a faulting GPU address, newest last (address binding reports). */
		void ReportAddressOwners(const uint64_t address, const uint64_t precision)
		{
			if (!s_BreadcrumbAddressBinding)
				return;
			std::lock_guard lock(s_BreadcrumbMutex);
			const uint64_t low = address > precision ? address - precision : 0;
			const uint64_t high = address + precision;
			std::vector<const AddressBinding *> owners;
			for (const AddressBinding &binding : s_AddressBindings)
			{
				if (binding.base <= high && low < binding.base + binding.size)
					owners.push_back(&binding);
			}
			if (owners.empty())
			{
				SEDX_CORE_ERROR_TAG("GPU Crash", "    nothing was ever bound there ({} ranges recorded)", s_AddressBindings.size());
				return;
			}
			const size_t first = owners.size() > 8 ? owners.size() - 8 : 0;
			for (size_t i = first; i < owners.size(); ++i)
			{
				const AddressBinding &binding = *owners[i];
				std::string name = binding.name;
				if (name.empty())
				{
					if (const auto it = s_ObjectNames.find(binding.handle); it != s_ObjectNames.end())
						name = it->second;
				}
				SEDX_CORE_ERROR_TAG("GPU Crash", "    {} {} 0x{:x} '{}' at 0x{:016x} (+0x{:x})", binding.bound ? "bound" : "UNBOUND",
					binding.type == VK_OBJECT_TYPE_BUFFER ? "buffer" : binding.type == VK_OBJECT_TYPE_IMAGE ? "image" : "object",
					binding.handle, name, binding.base, binding.size);
			}
		}
	}

	void Debugging::ReportDeviceLost(VkDevice device, const std::vector<VkQueue> &queues)
	{
		if (s_DeviceLostReported || device == VK_NULL_HANDLE)
			return;
		s_DeviceLostReported = true;

		if (!s_BreadcrumbCheckpoints && !s_BreadcrumbDeviceFault)
		{
			SEDX_CORE_ERROR_TAG("GPU Crash", "Device lost. Run with --gpu-breadcrumbs to log the GPU's last passes and fault report.");
			return;
		}

		// The checkpoints the GPU passed: TOP_OF_PIPE = the work after it started, BOTTOM_OF_PIPE = the work before it finished
		if (s_BreadcrumbCheckpoints && vkGetQueueCheckpointDataNV)
		{
			if (queues.empty())
				SEDX_CORE_ERROR_TAG("GPU Crash", "No queue to read the checkpoints of");
			for (size_t q = 0; q < queues.size(); ++q)
			{
				if (queues[q] == VK_NULL_HANDLE)
				{
					SEDX_CORE_ERROR_TAG("GPU Crash", "Queue {}: no handle", q);
					continue;
				}
				uint32_t count = 0;
				vkGetQueueCheckpointDataNV(queues[q], &count, nullptr);
				std::vector<VkCheckpointDataNV> checkpoints(count, VkCheckpointDataNV{ .sType = VK_STRUCTURE_TYPE_CHECKPOINT_DATA_NV });
				vkGetQueueCheckpointDataNV(queues[q], &count, checkpoints.data());
				SEDX_CORE_ERROR_TAG("GPU Crash", "Queue {}: {} checkpoint(s)", q, count);
				for (const VkCheckpointDataNV &checkpoint : checkpoints)
				{
					const char *stage = checkpoint.stage == VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT ? "last started"
									  : checkpoint.stage == VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT ? "last finished" : "stage";
					SEDX_CORE_ERROR_TAG("GPU Crash", "  {} (0x{:x}): {}", stage, static_cast<uint32_t>(checkpoint.stage),
						checkpoint.pCheckpointMarker ? static_cast<const char *>(checkpoint.pCheckpointMarker) : "<none>");
				}
			}
		}

		if (s_BreadcrumbAddressBinding)
		{
			std::lock_guard lock(s_BreadcrumbMutex);
			SEDX_CORE_ERROR_TAG("GPU Crash", "{} GPU address ranges bound or unbound so far", s_AddressBindings.size());
		}

		if (s_BreadcrumbDeviceFault && vkGetDeviceFaultInfoEXT)
		{
			VkDeviceFaultCountsEXT counts{ .sType = VK_STRUCTURE_TYPE_DEVICE_FAULT_COUNTS_EXT };
			if (vkGetDeviceFaultInfoEXT(device, &counts, nullptr) != VK_SUCCESS)
			{
				SEDX_CORE_ERROR_TAG("GPU Crash", "No device fault report");
				return;
			}

			std::vector<VkDeviceFaultAddressInfoEXT> addresses(counts.addressInfoCount);
			std::vector<VkDeviceFaultVendorInfoEXT> vendorInfos(counts.vendorInfoCount);
			counts.vendorBinarySize = 0;
			VkDeviceFaultInfoEXT info{ .sType = VK_STRUCTURE_TYPE_DEVICE_FAULT_INFO_EXT };
			info.pAddressInfos = addresses.data();
			info.pVendorInfos = vendorInfos.data();
			const VkResult result = vkGetDeviceFaultInfoEXT(device, &counts, &info);
			SEDX_CORE_ERROR_TAG("GPU Crash", "Device fault ({}): {}", static_cast<int>(result), info.description);
			for (const VkDeviceFaultAddressInfoEXT &address : addresses)
			{
				const char *type = "unknown";
				switch (address.addressType)
				{
					case VK_DEVICE_FAULT_ADDRESS_TYPE_NONE_EXT:							type = "none"; break;
					case VK_DEVICE_FAULT_ADDRESS_TYPE_READ_INVALID_EXT:					type = "invalid read"; break;
					case VK_DEVICE_FAULT_ADDRESS_TYPE_WRITE_INVALID_EXT:				type = "invalid write"; break;
					case VK_DEVICE_FAULT_ADDRESS_TYPE_EXECUTE_INVALID_EXT:				type = "invalid execute"; break;
					case VK_DEVICE_FAULT_ADDRESS_TYPE_INSTRUCTION_POINTER_UNKNOWN_EXT:	type = "instruction pointer (unknown)"; break;
					case VK_DEVICE_FAULT_ADDRESS_TYPE_INSTRUCTION_POINTER_INVALID_EXT:	type = "instruction pointer (invalid)"; break;
					case VK_DEVICE_FAULT_ADDRESS_TYPE_INSTRUCTION_POINTER_FAULT_EXT:	type = "instruction pointer (fault)"; break;
					default: break;
				}
				SEDX_CORE_ERROR_TAG("GPU Crash", "  address 0x{:016x} +/- 0x{:x}: {}", address.reportedAddress, address.addressPrecision, type);
				ReportAddressOwners(address.reportedAddress, address.addressPrecision);
			}
			for (const VkDeviceFaultVendorInfoEXT &vendor : vendorInfos)
				SEDX_CORE_ERROR_TAG("GPU Crash", "  vendor: {} (code 0x{:x}, data 0x{:x})", vendor.description, vendor.vendorFaultCode, vendor.vendorFaultData);
		}
	}

	bool Debugging::SetRenderdocEnabled()
	{
#if RENDERDOC_TOOLING_ENABLED == 1
		SEDX_CORE_INFO_TAG("Debugging", "RenderDoc tooling is enabled.");
		return m_Renderdoc_Enabled = true;
#else
		return m_Renderdoc_Enabled = false;
#endif
	}

} // namespace SceneryEditorX

// -----------------------------------------------------------------
