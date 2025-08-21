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
#include "vk_enums.h"
#include "SceneryEditorX/renderer/buffers/framebuffer.h"
#include "SceneryEditorX/renderer/buffers/vertex_buffer.h"
#include "SceneryEditorX/renderer/shaders/shader.h"

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
        VertexBufferLayout layout;
        VertexBufferLayout instanceLayout;
        VertexBufferLayout boneInfluenceLayout;
        PrimitiveTopology topology = PrimitiveTopology::Triangles;
        DepthCompareOperator depthOperator = DepthCompareOperator::GreaterOrEqual;
        bool backfaceCulling = true;
        bool depthTest = true;
        bool depthWrite = true;
        bool wireframe = false;
        float lineWidth = 1.0f;
        std::string debugName;
    };

	class Pipeline : public RefCounted
	{
	public:
        explicit Pipeline(PipelineData &data);
        virtual ~Pipeline() override;

		PipelineData &GetSpecification() { return pipelineSpecs; }
	    const PipelineData &GetSpecification() const { return pipelineSpecs; }
	    void Invalidate();
	    Ref<Shader> GetShader() const { return pipelineSpecs.shader; }
        bool DynamicLineWidth() const;

        struct Stage
        {
            ShaderStage::Stage stage;
            std::filesystem::path path;
            std::string entryPoint = "main";
        };

        VkExtent2D GetFloatSwapExtent();
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
        Ref<PipelineResource> resource;
        std::vector<std::vector<char>> stageBytes;

        VkPipeline pipeline = nullptr;
        VkPipelineLayout pipelineLayout = nullptr;
        VkPipelineCache pipelineCache = nullptr;
	};

}

/// -------------------------------------------------------
