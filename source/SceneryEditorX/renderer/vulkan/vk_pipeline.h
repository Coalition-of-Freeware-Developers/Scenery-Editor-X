/**
* -------------------------------------------------------
* Scenery Editor X
* -------------------------------------------------------
* Copyright (c) 2025 Thomas Ray 
* Copyright (c) 2025 Coalition of Freeware Developers
* -------------------------------------------------------
* vk_pipeline.h
* -------------------------------------------------------
* Created: 15/4/2025
* -------------------------------------------------------
*/
#pragma once
#include <SceneryEditorX/renderer/render_context.h>
#include <SceneryEditorX/renderer/shaders/shader.h>
#include <SceneryEditorX/renderer/vulkan/resource.h>

/// -------------------------------------------------------

namespace SceneryEditorX
{

    struct PipelineResource : Resource
    {
        VkPipeline pipeline;
        VkPipelineLayout layout;

        virtual ~PipelineResource() override
        {
            vkDestroyPipeline(RenderContext::GetCurrentDevice()->GetDevice(), pipeline, nullptr);
            vkDestroyPipelineLayout(RenderContext::GetCurrentDevice()->GetDevice(), layout, nullptr);
        }
    };

    struct PipelineData
    {
        Ref<Shader> shader;
        Ref<Framebuffer> dstFramebuffer;
        bool backfaceCulling = true;
        bool depthTest = true;
        bool depthWrite = true;
        bool wireframe = false;
        float lineWidth = 1.0f;
        std::string name;
    };

	class Pipeline : public RefCounted
	{
	public:
	    Pipeline();
        virtual ~Pipeline() override;

        struct Stage
        {
            ShaderStage::Stage stage;
            std::filesystem::path path;
            std::string entryPoint = "main";
        };

	    virtual void Create();

        VkExtent2D GetFloatSwapExtent() const;
        VkPipeline GetPipeline() const { return pipeline; }
		VkPipelineLayout GetVulkanPipelineLayout() const { return pipelineLayout; }
        [[nodiscard]] virtual Ref<Shader> GetShader() const { return shaderPtr; }
        bool dynamicLineWidth() const;

	private:
        Ref<Shader> shaderPtr;
        Ref<SwapChain> vkSwapChain;
        PipelineType type;
        Ref<PipelineResource> resource;
        std::vector<Stage> stages;
        std::vector<std::vector<char>> stageBytes;

        VkPipeline pipeline = VK_NULL_HANDLE;
        VkPipelineLayout pipelineLayout = VK_NULL_HANDLE;
        VkPipelineCache pipelineCache = VK_NULL_HANDLE;
	};

} // namespace SceneryEditorX

/// -------------------------------------------------------
