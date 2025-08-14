/**
* -------------------------------------------------------
* Scenery Editor X
* -------------------------------------------------------
* Copyright (c) 2025 Thomas Ray
* Copyright (c) 2025 Coalition of Freeware Developers
* -------------------------------------------------------
* compute_pipeline.cpp
* -------------------------------------------------------
* Created: 25/7/2025
* -------------------------------------------------------
*/
#include <SceneryEditorX/core/time/timer.h>
#include <SceneryEditorX/renderer/compute_pipeline.h>
#include <SceneryEditorX/renderer/vulkan/vk_util.h>
#include <utility>

/// -------------------------------------------------------

namespace SceneryEditorX
{

	static VkFence s_ComputeFence = nullptr;

	ComputePipeline::ComputePipeline(const Ref<Shader> &computeShader) : m_Shader(computeShader.As<Shader>())
	{
		Ref<ComputePipeline> instance(this);
		Renderer::Submit([instance]() mutable
		{
			instance->CreateRenderThreadPipeline();
		});
		Renderer::RegisterShaderDependency(computeShader, this);
	}

	void ComputePipeline::CreatePipeline()
	{
        Renderer::Submit([instance = Ref(this)]() mutable { instance->CreateRenderThreadPipeline(); });
	}

	void ComputePipeline::BufferMemoryBarrier(Ref<CommandBuffer> commandBuffer, Ref<StorageBuffer> storageBuffer, ResourceAccessFlags fromAccess, ResourceAccessFlags toAccess)
	{
		BufferMemoryBarrier(std::move(commandBuffer), std::move(storageBuffer), PipelineStage::ComputeShader, fromAccess, PipelineStage::ComputeShader, toAccess);
	}

	void ComputePipeline::BufferMemoryBarrier(const Ref<CommandBuffer> commandBuffer, const Ref<StorageBuffer> storageBuffer, PipelineStage fromStage, ResourceAccessFlags fromAccess, PipelineStage toStage, ResourceAccessFlags toAccess)
	{
		Renderer::Submit([vulkanRenderCommandBuffer = commandBuffer.As<CommandBuffer>(), StorageBuffer = storageBuffer.As<StorageBuffer>(), fromStage, fromAccess, toStage, toAccess]() mutable
		{
			VkBufferMemoryBarrier bufferMemoryBarrier = {};
			bufferMemoryBarrier.sType = VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER;
			bufferMemoryBarrier.buffer = StorageBuffer->GetVulkanBuffer();
			bufferMemoryBarrier.offset = 0;
			bufferMemoryBarrier.size = VK_WHOLE_SIZE;
			bufferMemoryBarrier.srcAccessMask = (VkAccessFlags)fromAccess;
			bufferMemoryBarrier.dstAccessMask = (VkAccessFlags)toAccess;
			vkCmdPipelineBarrier(
				vulkanRenderCommandBuffer->GetActiveCmdBuffer(),
				(VkPipelineStageFlagBits)fromStage,
				(VkPipelineStageFlagBits)toStage,
				0,
				0, nullptr,
				1, &bufferMemoryBarrier,
				0, nullptr);
		});
	}

	void ComputePipeline::ImageMemoryBarrier(Ref<CommandBuffer> commandBuffer, Ref<Image2D> image, ResourceAccessFlags fromAccess, ResourceAccessFlags toAccess)
	{
		ImageMemoryBarrier(std::move(commandBuffer), std::move(image), PipelineStage::ComputeShader, fromAccess, PipelineStage::ComputeShader, toAccess);
	}

	void ComputePipeline::ImageMemoryBarrier(const Ref<CommandBuffer> commandBuffer, const Ref<Image2D> image, PipelineStage fromStage, ResourceAccessFlags fromAccess, PipelineStage toStage, ResourceAccessFlags toAccess)
	{
		Renderer::Submit([vulkanRenderCommandBuffer = commandBuffer.As<CommandBuffer>(), vulkanImage = image.As<Image2D>(), fromStage, fromAccess, toStage, toAccess]() mutable
		{
			const VkImageLayout imageLayout = vulkanImage->GetDescriptorInfoVulkan().imageLayout;

			VkImageMemoryBarrier imageMemoryBarrier = {};
			imageMemoryBarrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
			imageMemoryBarrier.oldLayout = imageLayout;
			imageMemoryBarrier.newLayout = imageLayout;
			imageMemoryBarrier.image = vulkanImage->GetImageInfo().image;
			///< TODO: get layer count from image; also take SubresourceRange as parameter
			imageMemoryBarrier.subresourceRange = {
			    .aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
			    .baseMipLevel = 0,
			    .levelCount=vulkanImage->GetSpecification().mips,
			    .baseArrayLayer=0,
			    .layerCount=1
            };
			imageMemoryBarrier.srcAccessMask = (VkAccessFlags)fromAccess;
			imageMemoryBarrier.dstAccessMask = (VkAccessFlags)toAccess;
			vkCmdPipelineBarrier(
				vulkanRenderCommandBuffer->GetActiveCmdBuffer(),
				(VkPipelineStageFlagBits)fromStage,
				(VkPipelineStageFlagBits)toStage,
				0,
				0, nullptr,
				0, nullptr,
				1, &imageMemoryBarrier);
		});
	}

	void ComputePipeline::CreateRenderThreadPipeline()
	{
		VkDevice device = RenderContext::GetCurrentDevice();

		///< TODO: Abstract into some sort of compute pipeline
		const auto descriptorSetLayouts = m_Shader->GetAllDescriptorSetLayouts();

		VkPipelineLayoutCreateInfo pipelineLayoutCreateInfo {};
		pipelineLayoutCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
		pipelineLayoutCreateInfo.setLayoutCount = (uint32_t)descriptorSetLayouts.size();
		pipelineLayoutCreateInfo.pSetLayouts = descriptorSetLayouts.data();

		const auto& pushConstantRanges = m_Shader->GetPushConstantRanges();
		std::vector<VkPushConstantRange> vulkanPushConstantRanges(pushConstantRanges.size());
		if (!pushConstantRanges.empty())
		{
			///< TODO: should come from shader
			for (uint32_t i = 0; i < pushConstantRanges.size(); i++)
			{
				const auto &[ShaderStage, Offset, Size] = pushConstantRanges[i];
				auto &[stageFlags, offset, size] = vulkanPushConstantRanges[i];

				stageFlags = ShaderStage;
				offset = Offset;
				size = Size;
			}

			pipelineLayoutCreateInfo.pushConstantRangeCount = (uint32_t)vulkanPushConstantRanges.size();
			pipelineLayoutCreateInfo.pPushConstantRanges = vulkanPushConstantRanges.data();
		}

		VK_CHECK_RESULT(vkCreatePipelineLayout(device, &pipelineLayoutCreateInfo, nullptr, &m_ComputePipelineLayout))

		VkComputePipelineCreateInfo computePipelineCreateInfo {};
		computePipelineCreateInfo.sType = VK_STRUCTURE_TYPE_COMPUTE_PIPELINE_CREATE_INFO;
		computePipelineCreateInfo.layout = m_ComputePipelineLayout;
		computePipelineCreateInfo.flags = 0;
		const auto& shaderStages = m_Shader->GetPipelineShaderStageCreateInfos();
		computePipelineCreateInfo.stage = shaderStages[0];

		VkPipelineCacheCreateInfo pipelineCacheCreateInfo = {};
		pipelineCacheCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_CACHE_CREATE_INFO;

		VK_CHECK_RESULT(vkCreatePipelineCache(device, &pipelineCacheCreateInfo, nullptr, &m_PipelineCache));
		VK_CHECK_RESULT(vkCreateComputePipelines(device, m_PipelineCache, 1, &computePipelineCreateInfo, nullptr, &m_ComputePipeline))

		SetDebugUtilsObjectName(device, VK_OBJECT_TYPE_PIPELINE, m_Shader->GetName(), m_ComputePipeline);
	}

	void ComputePipeline::Execute(const VkDescriptorSet* descriptorSets, uint32_t descriptorSetCount, uint32_t groupCountX, uint32_t groupCountY, uint32_t groupCountZ)
	{
		const VkDevice device = RenderContext::GetCurrentDevice();
		const VkQueue computeQueue = RenderContext::GetLogicDevice()->GetComputeQueue();
		//vkQueueWaitIdle(computeQueue); // TODO: don't

		VkCommandBuffer computeCommandBuffer = RenderContext::GetLogicDevice()->GetCommandBuffer(true, true);

		SetVulkanCheckpoint(computeCommandBuffer, "ComputePipeline::Execute");

		vkCmdBindPipeline(computeCommandBuffer, VK_PIPELINE_BIND_POINT_COMPUTE, m_ComputePipeline);
		for (uint32_t i = 0; i < descriptorSetCount; i++)
		{
			vkCmdBindDescriptorSets(computeCommandBuffer, VK_PIPELINE_BIND_POINT_COMPUTE, m_ComputePipelineLayout, 0, 1, &descriptorSets[i], 0, 0);
			vkCmdDispatch(computeCommandBuffer, groupCountX, groupCountY, groupCountZ);
		}

		vkEndCommandBuffer(computeCommandBuffer);
		if (!s_ComputeFence)
		{

			VkFenceCreateInfo fenceCreateInfo {};
			fenceCreateInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
			fenceCreateInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT;
			VK_CHECK_RESULT(vkCreateFence(device, &fenceCreateInfo, nullptr, &s_ComputeFence))

			SetDebugUtilsObjectName(device, VK_OBJECT_TYPE_FENCE, std::format("Compute pipeline fence"), s_ComputeFence);
		}

		///< Make sure previous compute shader in pipeline has completed (TODO: this shouldn't be needed for all cases)
		vkWaitForFences(device, 1, &s_ComputeFence, VK_TRUE, UINT64_MAX);
		vkResetFences(device, 1, &s_ComputeFence);

		VkSubmitInfo computeSubmitInfo {};
		computeSubmitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
		computeSubmitInfo.commandBufferCount = 1;
		computeSubmitInfo.pCommandBuffers = &computeCommandBuffer;
		VK_CHECK_RESULT(vkQueueSubmit(computeQueue, 1, &computeSubmitInfo, s_ComputeFence));

		/// Wait for execution of compute shader to complete
		/// Currently this is here for "safety"
		{
			SEDX_SCOPE_TIMER("Compute shader execution");
			vkWaitForFences(device, 1, &s_ComputeFence, VK_TRUE, UINT64_MAX);
		}
	}

	void ComputePipeline::Begin(const Ref<CommandBuffer> commandBuffer)
	{
		SEDX_CORE_ASSERT(!m_ActiveComputeCommandBuffer);

		if (commandBuffer)
		{
			uint32_t frameIndex = Renderer::GetCurrentFrameIndex();
			m_ActiveComputeCommandBuffer = commandBuffer.As<CommandBuffer>()->GetCommandBuffer(frameIndex);
			m_UsingGraphicsQueue = true;
		}
		else
		{
            m_ActiveComputeCommandBuffer = RenderContext::GetLogicDevice()->GetCommandBuffer(true, true);
			m_UsingGraphicsQueue = false;
		}
		vkCmdBindPipeline(m_ActiveComputeCommandBuffer, VK_PIPELINE_BIND_POINT_COMPUTE, m_ComputePipeline);
	}

	void ComputePipeline::Begin_RenderThread(const Ref<CommandBuffer> commandBuffer)
	{
		SEDX_CORE_ASSERT(!m_ActiveComputeCommandBuffer);

		if (commandBuffer)
		{
			uint32_t frameIndex = Renderer::GetCurrentRenderThreadFrameIndex();
			m_ActiveComputeCommandBuffer = commandBuffer.As<CommandBuffer>()->GetCommandBuffer(frameIndex);
			m_UsingGraphicsQueue = true;
		}
		else
		{
            m_ActiveComputeCommandBuffer = RenderContext::GetLogicDevice()->GetCommandBuffer(true, true);
			m_UsingGraphicsQueue = false;
		}
		vkCmdBindPipeline(m_ActiveComputeCommandBuffer, VK_PIPELINE_BIND_POINT_COMPUTE, m_ComputePipeline);
	}


	void ComputePipeline::Dispatch(const UVec3& workGroups) const
	{
		SEDX_CORE_ASSERT(m_ActiveComputeCommandBuffer);
		vkCmdDispatch(m_ActiveComputeCommandBuffer, workGroups.x, workGroups.y, workGroups.z);
	}

	void ComputePipeline::End()
	{
		SEDX_CORE_ASSERT(m_ActiveComputeCommandBuffer);

		const VkDevice device = RenderContext::GetCurrentDevice();
		if (!m_UsingGraphicsQueue)
		{
            const VkQueue computeQueue = RenderContext::GetLogicDevice()->GetComputeQueue();
			vkEndCommandBuffer(m_ActiveComputeCommandBuffer);

			if (!s_ComputeFence)
			{
				VkFenceCreateInfo fenceCreateInfo {};
				fenceCreateInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
				fenceCreateInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT;
				VK_CHECK_RESULT(vkCreateFence(device, &fenceCreateInfo, nullptr, &s_ComputeFence))
				SetDebugUtilsObjectName(device, VK_OBJECT_TYPE_FENCE, "Compute pipeline fence", s_ComputeFence);
			}
			vkWaitForFences(device, 1, &s_ComputeFence, VK_TRUE, UINT64_MAX);
			vkResetFences(device, 1, &s_ComputeFence);

			VkSubmitInfo computeSubmitInfo {};
			computeSubmitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
			computeSubmitInfo.commandBufferCount = 1;
			computeSubmitInfo.pCommandBuffers = &m_ActiveComputeCommandBuffer;
			VK_CHECK_RESULT(vkQueueSubmit(computeQueue, 1, &computeSubmitInfo, s_ComputeFence))

			///< Wait for execution of compute shader to complete
			///< Currently this is here for "safety"
			{
				SEDX_SCOPE_TIMER("Compute shader execution")
				vkWaitForFences(device, 1, &s_ComputeFence, VK_TRUE, UINT64_MAX);
			}
		}
		m_ActiveComputeCommandBuffer = nullptr;
	}

	void ComputePipeline::SetPushConstants(const Buffer &constants) const
	{
		vkCmdPushConstants(m_ActiveComputeCommandBuffer, m_ComputePipelineLayout, VK_SHADER_STAGE_COMPUTE_BIT, 0, constants.size, constants.data);
	}

}

/// -------------------------------------------------------
