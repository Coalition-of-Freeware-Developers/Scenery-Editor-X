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
#include <SceneryEditorX/platform/editor_config.hpp>
#include <SceneryEditorX/platform/file_manager.hpp>
#include <SceneryEditorX/scene/asset_manager.h>
#include <SceneryEditorX/vulkan/vk_core.h>
#include <SceneryEditorX/renderer/shaders/shader.h>

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

        [[nodiscard]] virtual Ref<Shader> GetShader() const = 0;
        void CreatePipeline();
		VkPipeline GetPipeline() const { return pipeline; }
		VkPipelineLayout GetVulkanPipelineLayout() const { return pipelineLayout; }

	private:
        Viewport *vkViewport = nullptr;
        Ref<Shader> vkShaderPtr;
        Ref<SwapChain> vkSwapChain;
        Ref<VulkanDevice> device;

        VkPipeline pipeline = nullptr;
        VkPipelineLayout pipelineLayout = nullptr;
        VkPipelineCache pipelineCache = nullptr;
	};

} // namespace SceneryEditorX1

// -------------------------------------------------------
