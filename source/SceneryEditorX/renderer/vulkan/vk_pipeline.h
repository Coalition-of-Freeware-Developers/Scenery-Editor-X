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
#include <SceneryEditorX/renderer/buffers/framebuffer.h>
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
        //VertexBufferLayout layout;
        //VertexBufferLayout instanceLayout;
        //VertexBufferLayout boneInfluenceLayout;
        //PrimitiveTopology topology = PrimitiveTopology::Triangles;
        //DepthCompareOperator depthOperator = DepthCompareOperator::GreaterOrEqual;
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
        Pipeline(PipelineData &data);
        virtual ~Pipeline() override;

		virtual PipelineData &GetSpecification() { return pipelineSpecs; }
		virtual const PipelineData &GetSpecification() const { return pipelineSpecs; }
        virtual void Invalidate();
		virtual Ref<Shader> GetShader() const { return pipelineSpecs.shader; }
        bool dynamicLineWidth() const;

        struct Stage
        {
            ShaderStage::Stage stage;
            std::filesystem::path path;
            std::string entryPoint = "main";
        };

	    void Create();

        VkExtent2D GetFloatSwapExtent() const;
        VkPipeline GetPipeline() const { return pipeline; }
		VkPipelineLayout GetPipelineLayout() const { return pipelineLayout; }

	private:
        PipelineType point;
        std::vector<Stage> stages;
        std::string name;
        std::vector<VkFormat> vertexAttributes;
        std::vector<VkFormat> colorFormats;
        bool useDepth = false;
        VkFormat depthFormat;
        bool cullFront = false;
        bool lineTopology = false;

        PipelineData pipelineSpecs;
        //Ref<PipelineResource> resource;
        //std::vector<std::vector<char>> stageBytes;

        VkPipeline pipeline = nullptr;
        VkPipelineLayout pipelineLayout = nullptr;
        VkPipelineCache pipelineCache = nullptr;
	};

} // namespace SceneryEditorX

/// -------------------------------------------------------
