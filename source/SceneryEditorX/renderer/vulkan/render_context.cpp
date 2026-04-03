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
 * render_context.cpp
 * -------------------------------------------------------
 * Created: 09/02/2026
 * -------------------------------------------------------
 */
#include "render_context.h"
#include "debug/graphics_debug.h"
#include <SceneryEditorX/core/application/application_data.h>
#include <SceneryEditorX/logging/asserts.h>
#include <SceneryEditorX/utils/repeat_call_tracker.h>
#include <volk/volk.h>

// -------------------------------------------------------

namespace SceneryEditorX
{

	// Static instance of the render context
	static Ref<RenderContext> s_Instance = nullptr;
	constexpr uint32_t STAGING_BUFFER_SIZE = 256 * 1024 * 1024;
	constexpr uint32_t TIME_STAMP_PER_POOL = 64;

	static bool s_IsInitialized = false;

	// -------------------------------------------------------

	/**
	 * @brief  
	 * @return 
	 */
	static uint32_t GetVulkanAPIVersion()
	{
		uint32_t apiVersion = 0;
		if (VkResult result = vkEnumerateInstanceVersion(&apiVersion); result != VK_SUCCESS)
		{
			SEDX_CORE_WARN("Failed to enumerate Vulkan instance version! Is Vulkan runtime installed?");
		}
		return apiVersion;
	}

	// -------------------------------------------------------

	RenderContext::~RenderContext()
	{
		if (m_Device.IsValid())
		{
			//m_MemAllocator->Shutdown(); // Shutdown memory allocator first
		}

		//m_MemAllocator = nullptr; // Destroy memory allocator before device

		if (m_Instance != VK_NULL_HANDLE)
		{
		 Debugging::Shutdown(m_Instance);
			vkDestroyInstance(m_Instance, nullptr);
			m_Instance = VK_NULL_HANDLE;
		}
	}

	RenderContext::RenderContext(RenderContext &&other) noexcept
	{
		// Move resources from the other Device
		m_Device = other.m_Device;
		m_Instance = other.m_Instance;

		// Nullify the moved-from object
		other.m_Device = nullptr;
		other.m_Instance = VK_NULL_HANDLE;
	}

	RenderContext &RenderContext::operator=(RenderContext &&other) noexcept
	{
		if (this != &other)
		{
			// Move resources from the other Device
			m_Device = other.m_Device;
			m_Instance = other.m_Instance;

			// Nullify the moved-from object
			m_Device = nullptr;
			m_Instance = VK_NULL_HANDLE;
		}

		return *this;
	}

	void RenderContext::Init()
	{
		SEDX_TRACK_CALL("RenderContext::Init");
		if (s_IsInitialized) // Idempotent guard: avoid double-initialization if called from multiple entry points
		{
			SEDX_CORE_INFO_TAG("RenderContext", "RenderContext::Init() called but already initialized. Skipping.");
			return;
		}

		try
		{
			SEDX_CORE_TRACE("Initializing RenderContext");

			VkResult volkRes = volkInitialize();
			if (volkRes != VK_SUCCESS)
			{
				SEDX_CORE_ERROR_TAG("RenderContext", "volkInitialize failed: {}", static_cast<int>(volkRes));
				return;
			}
		
		#ifdef SEDX_DEBUG
			Debugging::SetValidationLayerEnabled(true);
		#else
			Debugging::SetValidationLayerEnabled(false);
		#endif

			if (Debugging::IsValidationLayerEnabled() && !Debugging::IsValidationLayerSupported())
			{
				SEDX_CORE_WARN_TAG("Validation Layer", "Requested validation layer '{}' is not available. Continuing without validation.", Debugging::GetValidationLayerName());
				Debugging::SetValidationLayerEnabled(false);
			}

			// Create the singleton instance if it doesn't exist
			if (!s_Instance)
			{
				s_Instance = CreateRef<RenderContext>();
			}

			/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
			/// Application Info
			/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

			AppData appData;
			VkApplicationInfo appInfo{};
			appInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
			appInfo.pNext = nullptr;
			appInfo.pApplicationName = "Scenery Editor X";
			appInfo.applicationVersion = AppData::version;
			appInfo.pEngineName = "X-Plane 12 Graphics Emulator";
			appInfo.engineVersion = VK_MAKE_API_VERSION(1, 0, 0, 0);
			appInfo.apiVersion = VK_API_VERSION_1_3;

			VkInstanceCreateInfo createInfo{};
			createInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
			createInfo.pApplicationInfo = &appInfo;

			/**
			 * Enable required surface extensions so windowing libraries (SDL) can
			 * create platform-specific surfaces. On Windows we need VK_KHR_surface
			 * and VK_KHR_win32_surface. If you later add runtime queries for
			 * required extensions, prefer those instead of hard-coding.
			 */
			std::vector<const char *> instanceExtensions = {VK_EXT_SWAPCHAIN_COLOR_SPACE_EXTENSION_NAME,
															VK_KHR_EXTERNAL_MEMORY_CAPABILITIES_EXTENSION_NAME,
															VK_KHR_EXTERNAL_FENCE_CAPABILITIES_EXTENSION_NAME,
															VK_KHR_GET_PHYSICAL_DEVICE_PROPERTIES_2_EXTENSION_NAME,
															VK_KHR_SURFACE_EXTENSION_NAME};

		#ifdef SEDX_PLATFORM_WINDOWS
			instanceExtensions.push_back(VK_KHR_WIN32_SURFACE_EXTENSION_NAME);
		#elif defined(SEDX_PLATFORM_LINUX)
			instanceExtensions.push_back(VK_KHR_XLIB_SURFACE_EXTENSION_NAME);
		#elif defined(SEDX_PLATFORM_MACOS)
			instanceExtensions.push_back(VK_EXT_METAL_SURFACE_EXTENSION_NAME);
		#endif

			std::vector<const char *> instanceLayers;
			LayerSettingsData layerSettings{};
			if (Debugging::IsValidationLayerEnabled())
			{
				instanceLayers.push_back(Debugging::GetValidationLayerName());
		#ifdef VK_EXT_DEBUG_UTILS_EXTENSION_NAME
				instanceExtensions.push_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);
		#endif
		#ifdef VK_EXT_LAYER_SETTINGS_EXTENSION_NAME
				instanceExtensions.push_back(VK_EXT_LAYER_SETTINGS_EXTENSION_NAME);
				layerSettings = Debugging::GetLayerSettings();
				createInfo.pNext = &layerSettings.createInfo;
		#endif
				SEDX_CORE_INFO_TAG("Validation Layer", "Enabled '{}' integration", Debugging::GetValidationLayerName());
			}
			else
			{
				SEDX_CORE_INFO_TAG("Validation Layer", "Validation layer integration is disabled");
			}

			const char *deviceExtensions[] = {
				VK_KHR_SWAPCHAIN_EXTENSION_NAME,
				VK_EXT_MEMORY_BUDGET_EXTENSION_NAME, // to obtain precise memory usage information from Vulkan Memory Allocator
				VK_KHR_FRAGMENT_SHADING_RATE_EXTENSION_NAME,
				VK_EXT_HDR_METADATA_EXTENSION_NAME,
				VK_KHR_ROBUSTNESS_2_EXTENSION_NAME,
				VK_KHR_EXTERNAL_MEMORY_EXTENSION_NAME,
				VK_KHR_EXTERNAL_SEMAPHORE_EXTENSION_NAME,
				VK_KHR_EXTERNAL_MEMORY_WIN32_EXTENSION_NAME,
				VK_KHR_WIN32_KEYED_MUTEX_EXTENSION_NAME,
				VK_KHR_TIMELINE_SEMAPHORE_EXTENSION_NAME,
				VK_KHR_DEDICATED_ALLOCATION_EXTENSION_NAME,
				VK_KHR_SYNCHRONIZATION_2_EXTENSION_NAME,
				VK_KHR_GET_MEMORY_REQUIREMENTS_2_EXTENSION_NAME,
				VK_EXT_MUTABLE_DESCRIPTOR_TYPE_EXTENSION_NAME,
			};

			createInfo.enabledExtensionCount = static_cast<uint32_t>(instanceExtensions.size());
			createInfo.ppEnabledExtensionNames = instanceExtensions.data();
			createInfo.enabledLayerCount = static_cast<uint32_t>(instanceLayers.size());
			createInfo.ppEnabledLayerNames = instanceLayers.data();

			VkResult result = vkCreateInstance(&createInfo, nullptr, &s_Instance->m_Instance);
			SEDX_VK_RESULT_ASSERT(result, "Failed to create Vulkan instance")

			// Initialize volk instance-level function pointers
			volkLoadInstance(s_Instance->m_Instance);
			Debugging::Initialize(s_Instance->m_Instance);

			/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
			/// Instance Extensions and Validation Layers
			/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

			// Get all available layers
			uint32_t layerCount = 0;
			vkEnumerateInstanceLayerProperties(&layerCount, nullptr);
			std::vector<VkLayerProperties> layerNames(layerCount); // Properties of all available Vulkan validation layers on the system.
			vkEnumerateInstanceLayerProperties(&layerCount, layerNames.data());


			/*
			// Check for validation layer availability
			if (EnableValidationLayers)
			{
				// More thorough validation layer checking
				GraphicsChecks layerChecker;

				// Check for all validation layers and log available ones
				std::vector<const char *> layersToCheck = {validationLayer[0]};
				GraphicsChecks::CheckLayers(layersToCheck);

				// Specifically check for Khronos validation layer
				khronosAvailable = layerChecker.CheckValidationLayerSupport();

				if (!khronosAvailable)
					SEDX_CORE_ERROR_TAG("Graphics Engine", "Khronos validation layer requested but not available!");
				else
					SEDX_CORE_TRACE_TAG("Graphics Engine", "Validation layers are available and will be enabled");
			}


			// Get all available extensions
			Extensions extensions;
			vkEnumerateInstanceExtensionProperties(nullptr, &extensions.extensionCount, nullptr);
			extensions.instanceExtensions.resize(extensions.extensionCount);

			vkEnumerateInstanceExtensionProperties(nullptr, &extensions.extensionCount, extensions.instanceExtensions.data());
			if (EnableValidationLayers)
			{
				for (const char *valLayer : validationLayer)
				{
					uint32_t count = 0;
					if (VkResult extResult = vkEnumerateInstanceExtensionProperties(valLayer, &count, nullptr); extResult == VK_SUCCESS && count > 0)
					{
						const size_t layerSize = extensions.instanceExtensions.size();
						extensions.instanceExtensions.resize(layerSize + count);
						vkEnumerateInstanceExtensionProperties(valLayer, &count, extensions.instanceExtensions.data() + layerSize);
						SEDX_CORE_TRACE_TAG("Render Context", "Added {} extensions from validation layer {}", count, valLayer);
					}
				}
				if (!khronosAvailable)
					SEDX_CORE_ERROR_TAG("Render Context", "Khronos validation layer not available!");
			}
			*/

			s_Instance->m_Device = CreateRef<Device>(s_Instance->m_Instance);
			if (!s_Instance->m_Device || s_Instance->m_Device->GetLogicalDevice() == VK_NULL_HANDLE)
			{
				SEDX_CORE_ERROR_TAG("RenderContext", "Failed to create valid Vulkan device!");
				return;
			}

			s_IsInitialized = true;
		}
		catch (const std::exception &e)
		{
			SEDX_CORE_ERROR("Exception during RenderContext initialization: {}", e.what());
		}
		catch (...)
		{
			SEDX_CORE_ERROR("Unknown exception during RenderContext initialization");
		}
	}

	Ref<RenderContext> RenderContext::Get()
	{
		if (!s_Instance)
		{
			s_Instance = CreateRef<RenderContext>();
		}

		return s_Instance;
	}

	bool RenderContext::IsInitialized()
	{
		return s_IsInitialized;
	}

	VkInstance RenderContext::GetInstance()
	{
		auto rc = Get();
		if (!rc || rc->m_Instance == VK_NULL_HANDLE)
		{
			SEDX_CORE_WARN("GetInstance() called before Vulkan instance creation");
			return VK_NULL_HANDLE;
		}

		return rc->m_Instance;
	}

} // namespace SceneryEditorX

// -------------------------------------------------------
