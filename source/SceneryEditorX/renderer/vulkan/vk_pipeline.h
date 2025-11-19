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

		PipelineData &GetSpecification() { return m_pipelineSpecs; }
	    const PipelineData &GetSpecification() const { return m_pipelineSpecs; }

	    Ref<Shader> GetShader() const { return m_pipelineSpecs.shader; }
        bool DynamicLineWidth() const;

        struct Stage
        {
            ShaderStage::Stage stage;
            std::filesystem::path path;
            std::string entryPoint = "main";
        };

        VkExtent2D GetFloatSwapExtent();
        VkPipeline GetPipeline() const { return m_pipeline; }
		VkPipelineLayout GetPipelineLayout() const { return m_pipelineLayout; }
        std::array<Color, MAX_RENDER_TARGET_COUNT> clear_color;
        std::array<Texture*, MAX_RENDER_TARGET_COUNT> render_target_color_textures;

	private:
        PipelineType m_point;
        std::vector<Stage> m_stages;
        std::string m_name;
        std::vector<VkFormat> m_vertexAttributes;
        std::vector<VkFormat> m_colorFormats;
        bool m_useDepth = false;
        VkFormat m_depthFormat;
        bool m_cullFront = false;
        bool m_lineTopology = false;

	    uint32_t m_width = 0;
        uint32_t m_height = 0;
        uint64_t m_hash = 0;

        PipelineData m_pipelineSpecs;
        Ref<PipelineResource> m_pipeResource;
        std::vector<std::vector<char>> m_stageBytes;

        void *m_resource = nullptr;
        void *m_resource_layout = nullptr;

        VkPipeline m_pipeline = nullptr;
        VkPipelineLayout m_pipelineLayout = nullptr;
        VkPipelineCache m_pipelineCache = nullptr;
	};

}

// -------------------------------------------------------
