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
		Pipeline() = default;
        explicit Pipeline(const PipelineData &data);
        virtual ~Pipeline() override;
        void Invalidate();

		PipelineData &GetSpecification() { return m_PipelineSpecs; }
	    const PipelineData &GetSpecification() const { return m_PipelineSpecs; }

	    Ref<Shader> GetShader() const { return m_PipelineSpecs.shader; }
        bool DynamicLineWidth() const;

        struct Stage
        {
            ShaderStage::Stage stage;
            std::filesystem::path path;
            std::string entryPoint = "main";
        };

        VkExtent2D GetFloatSwapExtent();
        VkPipeline GetPipeline() const { return m_Pipeline; }
		VkPipelineLayout GetPipelineLayout() const { return m_PipelineLayout; }
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
