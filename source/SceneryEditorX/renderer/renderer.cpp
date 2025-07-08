/**
* -------------------------------------------------------
* Scenery Editor X
* -------------------------------------------------------
* Copyright (c) 2025 Thomas Ray 
* Copyright (c) 2025 Coalition of Freeware Developers
* -------------------------------------------------------
* renderer.cpp
* -------------------------------------------------------
* Created: 22/6/2025
* -------------------------------------------------------
*/
#include <SceneryEditorX/core/application.h>
#include <SceneryEditorX/logging/profiler.hpp>
#include <SceneryEditorX/renderer/render_context.h>
#include <SceneryEditorX/renderer/renderer.h>
#include <SceneryEditorX/renderer/vulkan/vk_swapchain.h>
#include <SceneryEditorX/renderer/vulkan/vk_util.h>

#include "buffers/index_buffer.h"
#include "buffers/storage_buffer.h"
#include "buffers/uniform_buffer.h"
#include "buffers/vertex_buffer.h"

#include "shaders/shader.h"

/// -------------------------------------------------------

namespace SceneryEditorX
{
    struct RendererProperties
    {
        Ref<Image> BRDFLut;
        Ref<VertexBuffer> QuadVertexBuffer;
        Ref<IndexBuffer> QuadIndexBuffer;

        Shader::ShaderMaterialDescriptorSet QuadDescriptorSet;
        std::unordered_map<SceneRenderer*, std::vector<Shader::ShaderMaterialDescriptorSet>> RendererDescriptorSet;
        VkDescriptorSet ActiveRendererDescriptorSet = nullptr;
        std::vector<VkDescriptorPool> DescriptorPools;
        VkDescriptorPool MaterialDescriptorPool;
        std::vector<uint32_t> DescriptorPoolAllocationCount;

        /// UniformBufferSet -> Shader Hash -> Frame -> WriteDescriptor
		std::unordered_map<UniformBufferSet*, std::unordered_map<uint64_t, std::vector<std::vector<VkWriteDescriptorSet>>>> UniformBufferWriteDescriptorCache;
		std::unordered_map<StorageBufferSet*, std::unordered_map<uint64_t, std::vector<std::vector<VkWriteDescriptorSet>>>> StorageBufferWriteDescriptorCache;

        /// Default samplers
        VkSampler SamplerClamp = nullptr;
        VkSampler SamplerPoint = nullptr;

        int32_t SelectedDrawCall = -1;
        int32_t DrawCallCount = 0;

        Ref<ShaderLibrary> m_ShaderLibrary;

        Ref<Texture2D> WhiteTexture;
        Ref<Texture2D> BlackTexture;
        Ref<Texture2D> BRDFLutTexture;
        Ref<Texture2D> HilbertLut;
        Ref<TextureCube> BlackCubeTexture;
        Ref<Environment> EmptyEnvironment;

        std::unordered_map<std::string, std::string> GlobalShaderMacros;
    };

    /// Static variable
    LOCAL RendererProperties *s_Data = nullptr;
    LOCAL RenderData* m_renderData = nullptr;

    /// -------------------------------------------------------

    Ref<RenderContext> Renderer::GetContext()
    {
        return RenderContext::Get();
    }

    Ref<Image> && Renderer::GetBRDFLutTexture()
    {
        return s_Data->BRDFLutTexture;
    }

    void Renderer::Init()
    {
        /// Initialize the rendering system. This includes setting up the render context, command buffers, etc.

        /// Get the render context
        /// Initialize the context if needed
        if (const auto context = GetContext())
            context->Init();

		s_Data = hnew RendererProperties;
        const auto &config = GetRenderData();
        s_Data->DescriptorPools.resize(config.framesInFlight);
        s_Data->DescriptorPoolAllocationCount.resize(config.framesInFlight);

		/// Create Descriptor pools
        Submit([]() mutable
        {
            /// Create Descriptor Pool
			const VkDescriptorPoolSize pool_sizes[] =
			{
				{ VK_DESCRIPTOR_TYPE_SAMPLER, 1000 },
				{ VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1000 },
				{ VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE, 1000 },
				{ VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, 1000 },
				{ VK_DESCRIPTOR_TYPE_UNIFORM_TEXEL_BUFFER, 1000 },
				{ VK_DESCRIPTOR_TYPE_STORAGE_TEXEL_BUFFER, 1000 },
				{ VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 1000 },
				{ VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 1000 },
				{ VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC, 1000 },
				{ VK_DESCRIPTOR_TYPE_STORAGE_BUFFER_DYNAMIC, 1000 },
				{ VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT, 1000 }
			};
			VkDescriptorPoolCreateInfo pool_info = {};
            pool_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
            pool_info.flags = VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT;
            pool_info.maxSets = 100000;
            pool_info.poolSizeCount = (uint32_t)IM_ARRAYSIZE(pool_sizes);
            pool_info.pPoolSizes = pool_sizes;
            const VkDevice device = RenderContext::GetCurrentDevice()->GetDevice();
            const uint32_t framesInFlight = GetRenderData().framesInFlight;
            for (uint32_t i = 0; i < framesInFlight; i++)
            {
                VK_CHECK_RESULT(vkCreateDescriptorPool(device, &pool_info, nullptr, &s_Data->DescriptorPools[i]))
                s_Data->DescriptorPoolAllocationCount[i] = 0;
            }

            VK_CHECK_RESULT(vkCreateDescriptorPool(device, &pool_info, nullptr, &s_Data->MaterialDescriptorPool))
        });

		
		/// Create fullscreen quad
		float x = -1;
		float y = -1;
		float width = 2, height = 2;
		struct QuadVertex
		{
			glm::vec3 Position;
			glm::vec2 TexCoord;
		};

		QuadVertex* data = hnew QuadVertex[4];

		data[0].Position = glm::vec3(x, y, 0.0f);
		data[0].TexCoord = glm::vec2(0, 0);

		data[1].Position = glm::vec3(x + width, y, 0.0f);
		data[1].TexCoord = glm::vec2(1, 0);

		data[2].Position = glm::vec3(x + width, y + height, 0.0f);
		data[2].TexCoord = glm::vec2(1, 1);

		data[3].Position = glm::vec3(x, y + height, 0.0f);
		data[3].TexCoord = glm::vec2(0, 1);

		s_Data->QuadVertexBuffer = VertexBuffer::Create(data, 4 * sizeof(QuadVertex));
		uint32_t indices[6] = { 0, 1, 2, 2, 3, 0, };
		s_Data->QuadIndexBuffer = IndexBuffer::Create(indices, 6 * sizeof(uint32_t));

		s_Data->BRDFLut = Renderer::GetBRDFLutTexture();
    }

    void Renderer::Shutdown()
    {
        // Shutdown the rendering system
        // Clean up resources, destroy Vulkan objects, etc.
    }

    void Renderer::BeginFrame()
    {
        // Begin a new frame
        // This would typically involve waiting for fences, acquiring swapchain images, etc.
    }

    void Renderer::EndFrame()
    {
        // End the current frame
        // This would typically involve submitting command buffers, presenting images, etc.
    }

    void Renderer::SubmitFrame()
    {
        // Submit the current frame to the GPU
        // This would involve submitting command buffers to the appropriate queues
    }

    CommandQueue &Renderer::GetRenderResourceReleaseQueue(uint32_t index)
    {
        return resourceFreeQueue[index];
    }

    uint32_t Renderer::GetRenderQueueIndex()
    {
        return m_renderData->s_RenderQueueIndex;
    }

    uint32_t Renderer::GetRenderQueueSubmissionIndex()
    {
        return m_renderData->s_RenderQueueSubmissionIndex;
    }

    uint32_t Renderer::GetCurrentFrameIndex()
    {
        return m_renderData->frameIndex;
    }

    RenderData &Renderer::GetRenderData()
    {
        return *m_renderData;
    }

	void Renderer::SetRenderData(const RenderData &renderData)
    {
        *m_renderData = renderData;
    }

    void Renderer::RenderThreadFunc(ThreadManager *renderThread)
    {
        SEDX_PROFILE_THREAD("Render Thread");
        while (renderThread->isRunning())
            WaitAndRender(renderThread);
    }

    void Renderer::WaitAndRender(ThreadManager *renderThread)
    {
        renderThread->WaitAndSet(ThreadManager::State::Kick, ThreadManager::State::Busy);
        m_renderData->s_cmdQueue[GetRenderQueueIndex()]->Execute();

		/// Rendering has completed, set state to idle
        renderThread->Set(ThreadManager::State::Idle);

        SubmitFrame();
    }

    void Renderer::SwapQueues()
    {
        m_renderData->s_cmdQueueSubmissionIdx = (m_renderData->s_cmdQueueSubmissionIdx + 1) % m_renderData->s_cmdQueueCount;
    }

    uint32_t Renderer::GetCurrentRenderThreadFrameIndex()
    {
        /// Swapchain owns the Render Thread frame index
        return Application::Get().GetWindow().GetSwapChain().GetCurrentBufferIndex();
    }

    uint32_t Renderer::GetDescriptorAllocationCount(uint32_t frameIndex)
    {
        return m_renderData->DescriptorPoolAllocationCount[frameIndex];
    }

    VkSampler Renderer::CreateSampler(VkSamplerCreateInfo samplerCreateInfo)
    {
        auto device = RenderContext::GetCurrentDevice();

		VkSampler sampler;
        vkCreateSampler(device->GetDevice(), &samplerCreateInfo, nullptr, &sampler);

		SceneryEditorX::Util::GetResourceAllocationCounts().Samplers++;

		return sampler;
    }

    /// -------------------------------------------------------

} // namespace SceneryEditorX

/// -------------------------------------------------------
