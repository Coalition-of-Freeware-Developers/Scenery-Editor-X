/**
* -------------------------------------------------------
* Scenery Editor X
* -------------------------------------------------------
* Copyright (c) 2025 Thomas Ray 
* Copyright (c) 2025 Coalition of Freeware Developers
* -------------------------------------------------------
* vk_core.h
* -------------------------------------------------------
* Created: 21/3/2025
* -------------------------------------------------------
*/
#pragma once
#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>
#include <SceneryEditorX/core/window/window.h>
#include <SceneryEditorX/renderer/vulkan/vk_device.h>

/// -------------------------------------------------------

struct GLFWwindow;

namespace SceneryEditorX
{
    VkSampler CreateSampler(const VkSamplerCreateInfo &samplerCreateInfo);
    VkSampler CreateSampler(VkSamplerCreateInfo samplerCreateInfo);
	void DestroySampler(VkSampler sampler);

    /// -------------------------------------------------------

	class GraphicsEngine : public RefCounted
    {
	public:
        GraphicsEngine();
        virtual ~GraphicsEngine() override;

        virtual void Init(const Ref<Window> &window);
        //virtual void CreateInstance(const Ref<Window> &window);

        //Ref<Window> GetWindow() { return editorWindow; }

        //MemoryAllocator &GetMemAllocator() { return allocatorManager; }
		//CommandBuffer GetCommandBuffer() { return cmdBufferManger; }

		LOCAL Ref<GraphicsEngine> Get(); ///< Static accessor method to get the singleton instance
        GLOBAL void WaitIdle(const Ref<VulkanDevice> &device);

		/// -------------------------------------------------------

	private:
        INTERNAL Ref<GraphicsEngine> gfxContext;
        GLOBAL RenderContext *renderContext; ///< The render context for the graphics engine
	    VkAllocationCallbacks *allocator = nullptr;

        LOCAL void glfwSetWindowUserPointer(const Ref<Window> &window, GLFWwindow *pointer);

	    friend struct ImageResource;

		/// -------------------------------------------------------

        uint32_t apiVersion;

        /// -------------------------------------------------------

        void CleanUp();
        GLOBAL void FramebufferResizeCallback(GLFWwindow *window, int width, int height);

		/// -------------------------------------------------------
	};

} // namespace SceneryEditorX

/// -------------------------------------------------------
