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
 * vulkan_support.cpp
 * -------------------------------------------------------
 * Created: 20/10/2026
 * -------------------------------------------------------
 */
#include "vulkan_support.h"
#include <vector>
#include <volk/volk.h>

#if defined(_WIN32)
	#include <Windows.h>
#endif

// -------------------------------------------------------

namespace SceneryEditorX
{
	namespace
	{
		bool AtLeast13(const uint32_t version)
		{
			return VK_API_VERSION_MAJOR(version) > 1 || (VK_API_VERSION_MAJOR(version) == 1 && VK_API_VERSION_MINOR(version) >= 3);
		}

		std::string VersionText(const uint32_t version)
		{
			return std::to_string(VK_API_VERSION_MAJOR(version)) + "." + std::to_string(VK_API_VERSION_MINOR(version)) + "." +
				std::to_string(VK_API_VERSION_PATCH(version));
		}
	}

	VulkanSupportReport CheckVulkanSupport()
	{
		VulkanSupportReport report;

#if defined(_WIN32)
		// volk calls through vkGetInstanceProcAddr without checking it, so a vulkan-1.dll without one (damaged, or not
		// the Vulkan runtime) would crash it: count that as no runtime. A corrupt file would also make Windows show a
		// "Bad Image" box and wait for a click, which would hang setup's hidden --check-vulkan: no error boxes here.
		DWORD previousMode = 0;
		SetThreadErrorMode(SEM_FAILCRITICALERRORS | SEM_NOOPENFILEERRORBOX, &previousMode);
		HMODULE runtime = LoadLibraryW(L"vulkan-1.dll");
		SetThreadErrorMode(previousMode, nullptr);
		const bool usable = runtime && GetProcAddress(runtime, "vkGetInstanceProcAddr");
		if (runtime)
			FreeLibrary(runtime);
		if (!usable)
			return report;
#endif

		if (volkInitialize() != VK_SUCCESS)
			return report;

		// vkEnumerateInstanceVersion only exists from Vulkan 1.1 on
		report.runtimeVersion = VK_API_VERSION_1_0;
		if (vkEnumerateInstanceVersion)
			vkEnumerateInstanceVersion(&report.runtimeVersion);
		if (!AtLeast13(report.runtimeVersion))
		{
			report.status = VulkanSupport::LoaderTooOld;
			return report;
		}

		VkApplicationInfo application{};
		application.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
		application.pApplicationName = "Scenery Editor X";
		application.apiVersion = VK_API_VERSION_1_3;
		VkInstanceCreateInfo createInfo{};
		createInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
		createInfo.pApplicationInfo = &application;

		VkInstance instance = VK_NULL_HANDLE;
		if (vkCreateInstance(&createInfo, nullptr, &instance) != VK_SUCCESS)
		{
			report.status = VulkanSupport::NoDriver;
			return report;
		}
		volkLoadInstance(instance);

		uint32_t count = 0;
		std::vector<VkPhysicalDevice> devices;
		if (vkEnumeratePhysicalDevices(instance, &count, nullptr) == VK_SUCCESS && count > 0)
		{
			devices.resize(count);
			if (vkEnumeratePhysicalDevices(instance, &count, devices.data()) != VK_SUCCESS)
				count = 0;
			devices.resize(count);
		}

		uint32_t best = 0;
		for (VkPhysicalDevice device : devices)
		{
			VkPhysicalDeviceProperties properties{};
			vkGetPhysicalDeviceProperties(device, &properties);
			if (properties.apiVersion > best)
			{
				best = properties.apiVersion;
				report.bestDevice = std::string(properties.deviceName) + ", Vulkan " + VersionText(best);
			}
		}
		vkDestroyInstance(instance, nullptr);

		report.status = devices.empty() ? VulkanSupport::NoDriver : AtLeast13(best) ? VulkanSupport::Supported : VulkanSupport::DriverTooOld;
		return report;
	}

	std::string DescribeVulkanSupport(const VulkanSupportReport &report)
	{
		constexpr const char *drivers = "\n\nInstall the latest driver for your graphics card:\n"
			"  NVIDIA: https://www.nvidia.com/drivers\n"
			"  AMD: https://www.amd.com/support\n"
			"  Intel: https://www.intel.com/content/www/us/en/support/detect.html";

		switch (report.status)
		{
			case VulkanSupport::Supported:
				return {};
			case VulkanSupport::NoLoader:
				return std::string("Scenery Editor X needs Vulkan 1.3, but the Vulkan runtime (vulkan-1.dll) is not installed.") + drivers +
					"\n\nGraphics drivers install the Vulkan runtime. Running the Scenery Editor X setup again for all users can also install it.";
			case VulkanSupport::LoaderTooOld:
				return "Scenery Editor X needs Vulkan 1.3, but the Vulkan runtime on this PC is version " + VersionText(report.runtimeVersion) + "." + drivers +
					"\n\nRunning the Scenery Editor X setup again for all users can also update the runtime.";
			case VulkanSupport::NoDriver:
				return std::string("Scenery Editor X needs a graphics card with Vulkan 1.3, but no graphics driver on this PC offers Vulkan.") + drivers;
			case VulkanSupport::DriverTooOld:
				return "Scenery Editor X needs Vulkan 1.3, but the best graphics card found offers less (" + report.bestDevice + ")." + drivers +
					"\n\nIf the newest driver doesn't add Vulkan 1.3, the graphics card is too old for Scenery Editor X.";
		}
		return {};
	}
}

// -------------------------------------------------------
