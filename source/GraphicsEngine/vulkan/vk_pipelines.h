/**
* -------------------------------------------------------
* Scenery Editor X
* -------------------------------------------------------
* Copyright (c) 2025 Thomas Ray 
* Copyright (c) 2025 Coalition of Freeware Developers
* -------------------------------------------------------
* vk_pipelines.h
* -------------------------------------------------------
* Created: 15/4/2025
* -------------------------------------------------------
*/
#pragma once
#include <GraphicsEngine/renderer/shaders/shader.h>
#include <GraphicsEngine/vulkan/vk_core.h>

// -------------------------------------------------------

namespace SceneryEditorX
{
	class SwapChain;
	struct Viewport;

    // -------------------------------------------------------

	class Pipeline
	{
	public:
	    Pipeline() = default;
        virtual ~Pipeline();

	    void CreatePipeline();
        VkExtent2D GetFloatSwapExtent() const;
		VkPipeline GetPipeline() const { return pipeline; }
		VkPipelineLayout GetVulkanPipelineLayout() const { return pipelineLayout; }
        [[nodiscard]] virtual Ref<Shader> GetShader() const { return shaderPtr; }

    protected:
        /**
         * @brief Creates a shader module from shader code
         * @param device The logical device
         * @param code The shader code
         * @return The created shader module
         */
        INTERNAL VkShaderModule CreateShaderModule(VkDevice device, const std::vector<char>& code);

	private:
        Viewport *vkViewport = nullptr;
        Ref<Shader> shaderPtr;
        Ref<SwapChain> vkSwapChain;
        Ref<VulkanDevice> device;

        VkPipeline pipeline = VK_NULL_HANDLE;
        VkPipelineLayout pipelineLayout = VK_NULL_HANDLE;
        VkPipelineCache pipelineCache = VK_NULL_HANDLE;
	};

} // namespace SceneryEditorX

// -------------------------------------------------------
