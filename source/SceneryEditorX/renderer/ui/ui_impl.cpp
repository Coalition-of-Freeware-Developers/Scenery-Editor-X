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
 * ui_impl.cpp
 * -------------------------------------------------------
 * Created: 15/03/2026
 * -------------------------------------------------------
 */
#include "ui_impl.h"
#include "source/imgui/imgui_internal.h"
#include "source/imgui/backends/imgui_impl_vulkan.h"
#include <array>
#include <unordered_map>
#include <vector>
#include <volk/volk.h>
#include <SceneryEditorX/logging/asserts.h>
#include <SceneryEditorX/renderer/renderer.h>
#include <SceneryEditorX/renderer/vulkan/device.h>
#include <SceneryEditorX/renderer/vulkan/image_resource.h>
#include <SceneryEditorX/renderer/vulkan/queue_manager.h>
#include <SceneryEditorX/renderer/vulkan/render_context.h>
#include <SceneryEditorX/renderer/vulkan/render_data.h>
#include <SceneryEditorX/renderer/vulkan/swapchain.h>

// -------------------------------------------------------

namespace SceneryEditorX::UI
{
	namespace
	{
		/**
		 * Image descriptors handed to ImGui::Image() live for this many UI frames before they are freed.
		 * When the main thread builds UI frame N the render thread has already waited on the fence of frame
		 * N - 1 - MAX_FRAMES_IN_FLIGHT, so anything older than that is no longer referenced by the GPU.
		 */
		constexpr uint32_t IMAGE_DESCRIPTOR_LIFETIME = MAX_FRAMES_IN_FLIGHT + 2;

		/* Descriptor sets the backend may hand out: font atlas pages plus every image shown in a frame. */
		constexpr uint32_t DESCRIPTOR_POOL_SIZE = 1024;

		struct BackendState
		{
			bool initialized = false;
			bool linearColorTarget = false;
			VkFormat colorFormat = VK_FORMAT_UNDEFINED;	// Must outlive the backend: its pipeline info points at it
			VkSampler imageSampler = VK_NULL_HANDLE;
			uint64_t frame = 0;
			std::array<std::vector<VkDescriptorSet>, IMAGE_DESCRIPTOR_LIFETIME> imageDescriptors;
			std::unordered_map<VkImageView, VkDescriptorSet> frameImages;	// This frame's descriptors, so repeated Image() calls share one
		};

		BackendState s_Backend;

		bool IsSRGBFormat(const VkFormat format)
		{
			switch (format)
			{
				case VK_FORMAT_B8G8R8A8_SRGB:
				case VK_FORMAT_R8G8B8A8_SRGB:
				case VK_FORMAT_A8B8G8R8_SRGB_PACK32:
					return true;
				default:
					return false;
			}
		}

		void CheckVkResult(const VkResult result)
		{
			if (result == VK_SUCCESS)
				return;

			SEDX_CORE_ERROR_TAG("UI", "Vulkan error in the ImGui backend: {}", static_cast<int>(result));
		}

		PFN_vkVoidFunction LoadVulkanFunction(const char *functionName, void *userData)
		{
			return vkGetInstanceProcAddr(static_cast<VkInstance>(userData), functionName);
		}

		void FreeImageDescriptors(std::vector<VkDescriptorSet> &descriptors)
		{
			for (const VkDescriptorSet descriptor : descriptors)
				ImGui_ImplVulkan_RemoveTexture(descriptor);

			descriptors.clear();
		}
	}

	// -------------------------------------------------------

	bool InitializeBackend()
	{
		if (s_Backend.initialized)
			return true;

		SEDX_CORE_ASSERT(ImGui::GetCurrentContext(), "UI backend needs an ImGui context");

		const Ref<Device> device = RenderContext::Get()->GetDevice();
		Swapchain *swapchain = Renderer::GetSwapChain();
		if (!device.IsValid() || !swapchain)
		{
			SEDX_CORE_ERROR_TAG("UI", "Cannot initialize the ImGui renderer backend: device or swapchain is missing");
			return false;
		}

		const Ref<QueueManager> queueManager = device->GetQueueManager();
		const Ref<Queue> *graphicsQueue = queueManager->GetQueue(QueueType::Graphics);
		if (!graphicsQueue || !*graphicsQueue)
		{
			SEDX_CORE_ERROR_TAG("UI", "Cannot initialize the ImGui renderer backend: no graphics queue");
			return false;
		}

		// AppCore builds with VK_NO_PROTOTYPES, so the backend loads its entry points through the instance.
		VkInstance instance = RenderContext::GetInstance();
		if (!ImGui_ImplVulkan_LoadFunctions(VK_API_VERSION_1_3, LoadVulkanFunction, instance))
		{
			SEDX_CORE_ERROR_TAG("UI", "Failed to load Vulkan functions for the ImGui backend");
			return false;
		}

		s_Backend.colorFormat = swapchain->GetImageFormat();
		s_Backend.linearColorTarget = IsSRGBFormat(s_Backend.colorFormat);

		const uint32_t imageCount = static_cast<uint32_t>(swapchain->GetImages().size());

		ImGui_ImplVulkan_InitInfo initInfo{};
		initInfo.ApiVersion			= VK_API_VERSION_1_3;
		initInfo.Instance			= instance;
		initInfo.PhysicalDevice		= Device::GetPhysicalDevice();
		initInfo.Device				= device->GetLogicalDevice();
		initInfo.QueueFamily		= queueManager->GetFamilyIndexByType(QueueType::Graphics);
		initInfo.Queue				= (*graphicsQueue)->GetQueue();
		initInfo.DescriptorPoolSize = DESCRIPTOR_POOL_SIZE;
		initInfo.MinImageCount		= std::max(2u, imageCount);
		initInfo.ImageCount			= std::max(2u, imageCount);
		initInfo.UseDynamicRendering = true;
		initInfo.CheckVkResultFn	= CheckVkResult;
		initInfo.MinAllocationSize	= 1024 * 1024;

		// Main viewport: drawn into the renderer's swapchain pass (one color attachment, no depth).
		initInfo.PipelineInfoMain.PipelineRenderingCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_RENDERING_CREATE_INFO;
		initInfo.PipelineInfoMain.PipelineRenderingCreateInfo.colorAttachmentCount = 1;
		initInfo.PipelineInfoMain.PipelineRenderingCreateInfo.pColorAttachmentFormats = &s_Backend.colorFormat;

		// Secondary viewports: ask for the same format so linearized theme colors match across windows.
		initInfo.PipelineInfoForViewports.PipelineRenderingCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_RENDERING_CREATE_INFO;
		initInfo.PipelineInfoForViewports.PipelineRenderingCreateInfo.colorAttachmentCount = 1;
		initInfo.PipelineInfoForViewports.PipelineRenderingCreateInfo.pColorAttachmentFormats = &s_Backend.colorFormat;

		if (!ImGui_ImplVulkan_Init(&initInfo))
		{
			SEDX_CORE_ERROR_TAG("UI", "ImGui_ImplVulkan_Init failed");
			return false;
		}

		VkSamplerCreateInfo samplerCI{};
		samplerCI.sType			= VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
		samplerCI.magFilter		= VK_FILTER_LINEAR;
		samplerCI.minFilter		= VK_FILTER_LINEAR;
		samplerCI.mipmapMode	= VK_SAMPLER_MIPMAP_MODE_LINEAR;
		samplerCI.addressModeU	= VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
		samplerCI.addressModeV	= VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
		samplerCI.addressModeW	= VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
		samplerCI.maxAnisotropy = 1.0f;
		samplerCI.minLod		= 0.0f;
		samplerCI.maxLod		= 0.0f;
		CheckVkResult(vkCreateSampler(device->GetLogicalDevice(), &samplerCI, nullptr, &s_Backend.imageSampler));

		s_Backend.initialized = true;
		SEDX_CORE_INFO_TAG("UI", "ImGui {} Vulkan backend initialized (format {}, {} swapchain images, sRGB target: {})",
			IMGUI_VERSION, static_cast<int>(s_Backend.colorFormat), imageCount, s_Backend.linearColorTarget);
		return true;
	}

	void ShutdownBackend()
	{
		if (!s_Backend.initialized)
			return;

		// Every frame that could reference our pipelines, descriptors or platform windows must be finished.
		QueueManager::WaitIdleAll();

		for (auto &descriptors : s_Backend.imageDescriptors)
			FreeImageDescriptors(descriptors);
		s_Backend.frameImages.clear();

		if (s_Backend.imageSampler != VK_NULL_HANDLE)
		{
			vkDestroySampler(RenderContext::Get()->GetDevice()->GetLogicalDevice(), s_Backend.imageSampler, nullptr);
			s_Backend.imageSampler = VK_NULL_HANDLE;
		}

		ImGui_ImplVulkan_Shutdown();
		s_Backend.initialized = false;
	}

	bool IsBackendInitialized()
	{
		return s_Backend.initialized;
	}

	void NewFrame()
	{
		if (!s_Backend.initialized)
			return;

		ImGui_ImplVulkan_NewFrame();

		++s_Backend.frame;
		FreeImageDescriptors(s_Backend.imageDescriptors[s_Backend.frame % IMAGE_DESCRIPTOR_LIFETIME]);
		s_Backend.frameImages.clear();
	}

	bool IsLinearColorTarget()
	{
		return s_Backend.linearColorTarget;
	}

	void RenderMainViewport(VkCommandBuffer commandBuffer)
	{
		if (!s_Backend.initialized || commandBuffer == VK_NULL_HANDLE || !ImGui::GetCurrentContext())
			return;

		ImDrawData *drawData = ImGui::GetDrawData();
		if (!drawData || drawData->DisplaySize.x <= 0.0f || drawData->DisplaySize.y <= 0.0f)
			return;

		ImGui_ImplVulkan_RenderDrawData(drawData, commandBuffer);
	}

	ImTextureID GetTextureID(ImageResource *image)
	{
		if (!s_Backend.initialized || !image || image->GetImageView(0) == VK_NULL_HANDLE)
			return ImTextureID_Invalid;

		// Per-mip views start at index 1; sample mip 0 only so a partially filled mip chain never shows up.
		const VkImageView view = (image->HasPerMipViews() && image->GetMipCount() > 1) ? image->GetImageView(1) : image->GetImageView(0);

		if (const auto it = s_Backend.frameImages.find(view); it != s_Backend.frameImages.end())
			return static_cast<ImTextureID>(reinterpret_cast<uintptr_t>(it->second));

		// Allocated fresh every frame: render targets are recreated on resize and a recycled VkImageView
		// handle could otherwise alias a cached descriptor that still points at the destroyed view.
		const VkDescriptorSet descriptor = ImGui_ImplVulkan_AddTexture(s_Backend.imageSampler, view, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
		if (descriptor == VK_NULL_HANDLE)
			return ImTextureID_Invalid;

		s_Backend.imageDescriptors[s_Backend.frame % IMAGE_DESCRIPTOR_LIFETIME].push_back(descriptor);
		s_Backend.frameImages.emplace(view, descriptor);
		return static_cast<ImTextureID>(reinterpret_cast<uintptr_t>(descriptor));
	}

	void ReportImGuiAssert(const char *expression, const char *file, const int line)
	{
		SEDX_CORE_FATAL_TAG("ImGui", "Assertion failed: {} ({}:{})", expression, file, line);
		if (const auto &logger = Log::GetCoreLogger())
			logger->flush();

	#ifdef SEDX_PLATFORM_WINDOWS
		if (IsDebuggerPresent())
			__debugbreak();
	#endif
		std::abort();
	}

	bool IsWindowFocused(const char *windowName, const bool checkWindow)
	{
		ImGuiWindow *currentNavWindow = GImGui->NavWindow;
		if (!currentNavWindow)
			return false;

		if (checkWindow)
		{
			// Get the actual nav window (not e.g a table)
			ImGuiWindow *lastWindow = nullptr;
			while (lastWindow != currentNavWindow)
			{
				lastWindow = currentNavWindow;
				currentNavWindow = currentNavWindow->RootWindow;
			}
		}

		return currentNavWindow == ImGui::FindWindowByName(windowName);
	}

}

// -------------------------------------------------------
