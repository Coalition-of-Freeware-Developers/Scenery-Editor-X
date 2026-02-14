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
#include "SceneryEditorX/core/application/application_data.h"
#include "SceneryEditorX/utils/repeat_call_tracker.h"
#include <volk/volk.h>

#ifdef SEDX_PLATFORM_WINDOWS
    #include <Windows.h>
#endif

// -------------------------------------------------------

namespace SceneryEditorX
{

    // Static instance of the render context
    static Scope<RenderContext> s_Instance = nullptr;
    constexpr uint32_t StagingBufferSize = 256 * 1024 * 1024;
    constexpr uint32_t TimeStampPerPool = 64;

    // Static member definitions
    VkInstance RenderContext::m_Instance = VK_NULL_HANDLE;
    Ref<Device> RenderContext::m_Device = nullptr;

    // -------------------------------------------------------
	
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

	RenderContext::RenderContext() = default;

    RenderContext::~RenderContext()
    {
        if (m_Device.IsValid())
        {
            //m_MemAllocator->Shutdown(); // Shutdown memory allocator first
        }

        //m_MemAllocator = nullptr; // Destroy memory allocator before device

        if (m_Instance != VK_NULL_HANDLE)
        {
            vkDestroyInstance(m_Instance, nullptr);
            m_Instance = VK_NULL_HANDLE;
        }

    }

    Ref<RenderContext> RenderContext::Get()
    {
        if (!s_Instance)
        {
            s_Instance = CreateScope<RenderContext>();
        }

        return s_Instance.get();
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

    VkInstance RenderContext::GetInstance()
    {
        if (auto rc = Get(); !rc || m_Instance == VK_NULL_HANDLE)
        {
            SEDX_CORE_WARN("GetInstance() called before Vulkan instance creation");
            return VK_NULL_HANDLE;
        }

        return m_Instance;
    }

    void RenderContext::Init()
    {
        SEDX_TRACK_CALL("RenderContext::Init");
        if (m_IsInitialized) // Idempotent guard: avoid double-initialization if called from multiple entry points
        {
            SEDX_CORE_INFO_TAG("Graphics Engine", "RenderContext::Init() called but already initialized. Skipping.");
            return;
        }
        try
        {
            SEDX_CORE_INFO("Initializing RenderContext");
            /////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
            /// Application Info
            /////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

            AppData appData;
            uint32_t apiVersion = GetVulkanAPIVersion(); // Get the users highest available version

            // Initialize volk loader then create an instance via the RAII wrapper.
            volkInitialize();

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
             * Enable required surface extensions so windowing libraries (SFML) can
             * create platform-specific surfaces. On Windows we need VK_KHR_surface
             * and VK_KHR_win32_surface. If you later add runtime queries for
             * required extensions, prefer those instead of hard-coding.
             */
            const char *extensions[] = {
                VK_KHR_SURFACE_EXTENSION_NAME, 
                VK_KHR_WIN32_SURFACE_EXTENSION_NAME
            };
            createInfo.enabledExtensionCount = sizeof(extensions) / sizeof(extensions[0]);
            createInfo.ppEnabledExtensionNames = extensions;

            if (VkResult res = vkCreateInstance(&createInfo, nullptr, &m_Instance); res != VK_SUCCESS)
            {
                SEDX_CORE_ERROR("Failed to create Vulkan instance: {}", res);
                m_Instance = VK_NULL_HANDLE;
                return;
            }

            // Initialize volk instance-level function pointers
            volkLoadInstance(m_Instance);

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
                layerChecker.CheckLayers(layersToCheck);

                // Specifically check for Khronos validation layer
                khronosAvailable = layerChecker.CheckValidationLayerSupport();

                if (!khronosAvailable)
                    SEDX_CORE_ERROR_TAG("Graphics Engine", "Khronos validation layer requested but not available!");
                else
                    SEDX_CORE_INFO_TAG("Graphics Engine", "Validation layers are available and will be enabled");
            }*/

            /*
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
            }*/

			m_Device = CreateRef<Device>(m_Instance);
			if (!m_Device || m_Device->GetLogicalDevice() == VK_NULL_HANDLE)
            {
                SEDX_CORE_ERROR_TAG("RenderContext", "Failed to create valid Vulkan device!");
                return;
            }

            m_IsInitialized = true;

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



}

// -------------------------------------------------------
