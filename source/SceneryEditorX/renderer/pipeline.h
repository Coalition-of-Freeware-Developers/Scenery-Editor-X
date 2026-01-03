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
#include "enums.h"
#include "SceneryEditorX/renderer/framebuffer.h"
#include "SceneryEditorX/renderer/vertex_buffer.h"
#include "SceneryEditorX/renderer/shaders/shader.h"
#include <SceneryEditorX/renderer/texture.h>

// -------------------------------------------------------

namespace SceneryEditorX
{

    struct PipelineResource : Resource
    {
        VkPipeline pipeline;
        VkPipelineLayout layout;

        virtual ~PipelineResource() override
        {
			// Guard against teardown order: RenderContext device can be null at shutdown
            if (auto deviceRef = RenderContext::GetCurrentDevice(); deviceRef && deviceRef->GetDevice() != VK_NULL_HANDLE)
			{
				if (pipeline != VK_NULL_HANDLE)
					vkDestroyPipeline (deviceRef->GetDevice(), pipeline, nullptr);
				if (layout != VK_NULL_HANDLE)
					vkDestroyPipelineLayout (deviceRef->GetDevice(), layout, nullptr);
			}
			pipeline = VK_NULL_HANDLE;
			layout = VK_NULL_HANDLE;
        }
    };

    struct PipelineData
    {
        Ref<Shader> shader;
        Ref<Framebuffer> dstFramebuffer;
        VertexBufferLayout layout;
        VertexBufferLayout instanceLayout;
        VertexBufferLayout boneInfluenceLayout;
        PrimitiveTopology topology	= PrimitiveTopology::Triangles;
        CompareFunc depthOperator	= CompareFunc::GreaterOrEqual;
        bool backfaceCulling		= true;
        bool depthTest				= true;
        bool depthWrite				= true;
        bool wireframe				= false;
        float lineWidth				= 1.0f;
        std::string debugName;
    };

	struct PipelineStatistics
    {
        uint64_t InputAssemblyVertices = 0;
        uint64_t InputAssemblyPrimitives = 0;
        uint64_t VertexShaderInvocations = 0;
        uint64_t ClippingInvocations = 0;
        uint64_t ClippingPrimitives = 0;
        uint64_t FragmentShaderInvocations = 0;
        uint64_t ComputeShaderInvocations = 0;
    };

	class Pipeline : public RefCounted
	{
	public:
	    Pipeline(const PipelineData &data);
        virtual ~Pipeline() override;

        void Invalidate();
        static Ref<Pipeline> Get();

		PipelineData &GetSpecification() { return m_PipelineSpecs; }
	    const PipelineData &GetSpecification() const { return m_PipelineSpecs; }

	    Ref<Shader> GetShader() const { return m_PipelineSpecs.shader; }
        bool DynamicLineWidth() const;
        VkExtent2D GetFloatSwapExtent();
        VkPipeline GetPipeline() { return m_Pipeline; }
        VkPipelineLayout GetPipelineLayout() { return m_PipelineLayout; }

        struct Stage
        {
            ShaderStage::Stage stage;
            std::filesystem::path path;
            std::string entryPoint = "main";
        };

        std::array<Color, MAX_RENDER_TARGET_COUNT> clearColor;
        std::array<Texture*, MAX_RENDER_TARGET_COUNT> render_TargetColorTextures;

	private:
        PipelineType m_Point;
        std::vector<Stage> m_Stages;
        std::string m_Name;
        std::vector<VkFormat> m_VertexAttributes;
        std::vector<VkFormat> m_ColorFormats;

        bool m_UseDepth = false;
        VkFormat m_DepthFormat;

        bool m_CullFront = false;
        bool m_LineTopology = false;

	    uint32_t m_Width = 0;
        uint32_t m_Height = 0;
        uint64_t m_Hash = 0;

        PipelineData m_PipelineSpecs;
        Ref<PipelineResource> m_PipeResource;
        std::vector<std::vector<char>> m_StageBytes;

        void *m_Resource = nullptr;
        void *m_Resource_Layout = nullptr;

        VkPipeline m_Pipeline = nullptr;
        VkPipelineLayout m_PipelineLayout = nullptr;
        VkPipelineCache m_PipelineCache = nullptr;
	};

}

// -------------------------------------------------------
